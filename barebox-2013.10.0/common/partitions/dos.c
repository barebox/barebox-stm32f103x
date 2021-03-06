/*
 * Copyright (C) 2009...2011 Juergen Beisert, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <disks.h>
#include <init.h>
#include <asm/unaligned.h>
#include <dma.h>
#include <linux/err.h>

#include "parser.h"

/**
 * Guess the size of the disk, based on the partition table entries
 * @param dev device to create partitions for
 * @param table partition table
 * @return sector count
 */
static int disk_guess_size(struct device_d *dev, struct partition_entry *table)
{
	uint64_t size = 0;
	int i;

	for (i = 0; i < 4; i++) {
		if (table[i].partition_start != 0) {
			size += get_unaligned_le32(&table[i].partition_start) - size;
			size += get_unaligned_le32(&table[i].partition_size);
		}
	}

	return (int)size;
}

static void *read_mbr(struct block_device *blk)
{
	void *buf = dma_alloc(SECTOR_SIZE);
	int ret;

	ret = block_read(blk, buf, 0, 1);
	if (ret) {
		free(buf);
		return NULL;
	}

	return buf;
}

static int write_mbr(struct block_device *blk, void *buf)
{
	int ret;

	ret = block_write(blk, buf, 0, 1);
	if (ret)
		return ret;

	return block_flush(blk);
}

struct disk_signature_priv {
	uint32_t signature;
	struct block_device *blk;
};

static int dos_set_disk_signature(struct param_d *p, void *_priv)
{
	struct disk_signature_priv *priv = _priv;
	struct block_device *blk = priv->blk;
	void *buf;
	__le32 *disksigp;
	int ret;

	buf = read_mbr(blk);
	if (!buf)
		return -EIO;

	disksigp = buf + 0x1b8;

	*disksigp = cpu_to_le32(priv->signature);

	ret = write_mbr(blk, buf);

	free(buf);

	return ret;
}

static int dos_get_disk_signature(struct param_d *p, void *_priv)
{
	struct disk_signature_priv *priv = _priv;
	struct block_device *blk = priv->blk;
	void *buf;

	buf = read_mbr(blk);
	if (!buf)
		return -EIO;

	priv->signature = le32_to_cpup((__le32 *)(buf + 0x1b8));

	free(buf);

	return 0;
}

/**
 * Check if a DOS like partition describes this block device
 * @param blk Block device to register to
 * @param pd Where to store the partition information
 *
 * It seems at least on ARM this routine canot use temp. stack space for the
 * sector. So, keep the malloc/free.
 */
static void dos_partition(void *buf, struct block_device *blk,
			  struct partition_desc *pd)
{
	struct partition_entry *table;
	struct partition pentry;
	uint8_t *buffer = buf;
	int i;
	struct disk_signature_priv *dsp;

	table = (struct partition_entry *)&buffer[446];

	/* valid for x86 BIOS based disks only */
	if (IS_ENABLED(CONFIG_DISK_BIOS) && blk->num_blocks == 0)
		blk->num_blocks = disk_guess_size(blk->dev, table);

	for (i = 0; i < 4; i++) {
		pentry.first_sec = get_unaligned_le32(&table[i].partition_start);
		pentry.size = get_unaligned_le32(&table[i].partition_size);

		if (pentry.first_sec != 0) {
			pd->parts[pd->used_entries].first_sec = pentry.first_sec;
			pd->parts[pd->used_entries].size = pentry.size;
			pd->used_entries++;
		} else {
			dev_dbg(blk->dev, "Skipping empty partition %d\n", i);
		}
	}

	dsp = xzalloc(sizeof(*dsp));
	dsp->blk = blk;

	/*
	 * This parameter contains the NT disk signature. This allows to
	 * to specify the Linux rootfs using the following syntax:
	 *
	 *   root=PARTUUID=ssssssss-pp
	 *
	 * where ssssssss is a zero-filled hex representation of the 32-bit
	 * signature and pp is a zero-filled hex representation of the 1-based
	 * partition number.
	 */
	dev_add_param_int(blk->dev, "nt_signature",
			dos_set_disk_signature, dos_get_disk_signature,
			&dsp->signature, "%08x", dsp);
}

static struct partition_parser dos = {
	.parse = dos_partition,
	.type = filetype_mbr,
};

static int dos_partition_init(void)
{
	return partition_parser_register(&dos);
}
postconsole_initcall(dos_partition_init);
