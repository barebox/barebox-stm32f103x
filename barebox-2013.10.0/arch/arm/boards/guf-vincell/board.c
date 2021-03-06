/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * Copyright (C) 2011 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <environment.h>
#include <fcntl.h>
#include <fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <partition.h>
#include <sizes.h>
#include <bbu.h>
#include <gpio.h>
#include <io.h>
#include <i2c/i2c.h>
#include <linux/clk.h>

#include <generated/mach-types.h>

#include <mach/imx53-regs.h>
#include <mach/iomux-mx53.h>
#include <mach/devices-imx53.h>
#include <mach/generic.h>
#include <mach/imx-nand.h>
#include <mach/iim.h>
#include <mach/imx-nand.h>
#include <mach/bbu.h>
#include <mach/imx-flash-header.h>
#include <mach/imx5.h>

#include <asm/armlinux.h>
#include <asm/mmu.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_RMII,
};

static iomux_v3_cfg_t vincell_pads[] = {
	MX53_PAD_DISP0_DAT23__AUDMUX_AUD4_RXD,
	MX53_PAD_DISP0_DAT20__AUDMUX_AUD4_TXC,
	MX53_PAD_DISP0_DAT21__AUDMUX_AUD4_TXD,
	MX53_PAD_DISP0_DAT22__AUDMUX_AUD4_TXFS,
	MX53_PAD_GPIO_8__CAN1_RXCAN,
	MX53_PAD_GPIO_7__CAN1_TXCAN,
	MX53_PAD_KEY_ROW4__CAN2_RXCAN,
	MX53_PAD_KEY_COL4__CAN2_TXCAN,
	MX53_PAD_GPIO_3__CCM_CLKO2,
	MX53_PAD_KEY_COL1__ECSPI1_MISO,
	MX53_PAD_KEY_ROW0__ECSPI1_MOSI,
	MX53_PAD_GPIO_19__ECSPI1_RDY,
	MX53_PAD_KEY_COL0__ECSPI1_SCLK,
	MX53_PAD_KEY_ROW1__GPIO4_9, /* ECSPI1_SS0 */
	MX53_PAD_KEY_COL2__GPIO4_10, /* ECSPI1_SS1 */
	MX53_PAD_KEY_ROW2__GPIO4_11, /* ECSPI1_SS2 */
	MX53_PAD_KEY_COL3__GPIO4_12, /* ECSPI1_SS3 */
	MX53_PAD_CSI0_DAT10__ECSPI2_MISO,
	MX53_PAD_EIM_CS1__ECSPI2_MOSI,
	MX53_PAD_EIM_A25__ECSPI2_RDY,
	MX53_PAD_DISP0_DAT19__ECSPI2_SCLK,
	MX53_PAD_DISP0_DAT18__GPIO5_12, /* ECSPI2_SS0 */
	MX53_PAD_EIM_DA0__EMI_NAND_WEIM_DA_0,
	MX53_PAD_EIM_DA1__EMI_NAND_WEIM_DA_1,
	MX53_PAD_EIM_DA2__EMI_NAND_WEIM_DA_2,
	MX53_PAD_EIM_DA3__EMI_NAND_WEIM_DA_3,
	MX53_PAD_EIM_DA4__EMI_NAND_WEIM_DA_4,
	MX53_PAD_EIM_DA5__EMI_NAND_WEIM_DA_5,
	MX53_PAD_EIM_DA6__EMI_NAND_WEIM_DA_6,
	MX53_PAD_EIM_DA7__EMI_NAND_WEIM_DA_7,
	MX53_PAD_EIM_DA8__EMI_NAND_WEIM_DA_8,
	MX53_PAD_EIM_DA9__EMI_NAND_WEIM_DA_9,
	MX53_PAD_EIM_DA10__EMI_NAND_WEIM_DA_10,
	MX53_PAD_EIM_DA11__EMI_NAND_WEIM_DA_11,
	MX53_PAD_EIM_DA12__EMI_NAND_WEIM_DA_12,
	MX53_PAD_EIM_DA13__EMI_NAND_WEIM_DA_13,
	MX53_PAD_EIM_DA14__EMI_NAND_WEIM_DA_14,
	MX53_PAD_EIM_DA15__EMI_NAND_WEIM_DA_15,
	MX53_PAD_NANDF_ALE__EMI_NANDF_ALE,
	MX53_PAD_NANDF_CLE__EMI_NANDF_CLE,
	MX53_PAD_NANDF_CS0__EMI_NANDF_CS_0,
	MX53_PAD_NANDF_CS1__EMI_NANDF_CS_1,
	MX53_PAD_NANDF_CS2__EMI_NANDF_CS_2,
	MX53_PAD_NANDF_CS3__EMI_NANDF_CS_3,
	MX53_PAD_NANDF_RB0__EMI_NANDF_RB_0,
	MX53_PAD_NANDF_RE_B__EMI_NANDF_RE_B,
	MX53_PAD_NANDF_WE_B__EMI_NANDF_WE_B,
	MX53_PAD_NANDF_WP_B__EMI_NANDF_WP_B,
	MX53_PAD_EIM_A16__EMI_WEIM_A_16,
	MX53_PAD_EIM_A17__EMI_WEIM_A_17,
	MX53_PAD_EIM_A18__EMI_WEIM_A_18,
	MX53_PAD_EIM_A19__EMI_WEIM_A_19,
	MX53_PAD_EIM_A20__EMI_WEIM_A_20,
	MX53_PAD_EIM_CS0__EMI_WEIM_CS_0,
	MX53_PAD_EIM_D16__EMI_WEIM_D_16,
	MX53_PAD_EIM_D17__EMI_WEIM_D_17,
	MX53_PAD_EIM_D18__EMI_WEIM_D_18,
	MX53_PAD_EIM_D19__EMI_WEIM_D_19,
	MX53_PAD_EIM_D20__EMI_WEIM_D_20,
	MX53_PAD_EIM_D21__EMI_WEIM_D_21,
	MX53_PAD_EIM_D22__EMI_WEIM_D_22,
	MX53_PAD_EIM_D23__EMI_WEIM_D_23,
	MX53_PAD_EIM_D24__EMI_WEIM_D_24,
	MX53_PAD_EIM_D25__EMI_WEIM_D_25,
	MX53_PAD_EIM_D26__EMI_WEIM_D_26,
	MX53_PAD_EIM_D27__EMI_WEIM_D_27,
	MX53_PAD_EIM_D28__EMI_WEIM_D_28,
	MX53_PAD_EIM_D29__EMI_WEIM_D_29,
	MX53_PAD_EIM_D30__EMI_WEIM_D_30,
	MX53_PAD_EIM_D31__EMI_WEIM_D_31,
	MX53_PAD_EIM_EB0__EMI_WEIM_EB_0,
	MX53_PAD_EIM_EB1__EMI_WEIM_EB_1,
	MX53_PAD_EIM_OE__EMI_WEIM_OE,
	MX53_PAD_EIM_RW__EMI_WEIM_RW,
	MX53_PAD_SD1_CLK__ESDHC1_CLK,
	MX53_PAD_SD1_CMD__ESDHC1_CMD,
	MX53_PAD_SD1_DATA0__ESDHC1_DAT0,
	MX53_PAD_SD1_DATA1__ESDHC1_DAT1,
	MX53_PAD_SD1_DATA2__ESDHC1_DAT2,
	MX53_PAD_SD1_DATA3__ESDHC1_DAT3,
	MX53_PAD_DI0_PIN4__ESDHC1_WP,
	MX53_PAD_FEC_MDC__FEC_MDC,
	MX53_PAD_FEC_MDIO__FEC_MDIO,
	MX53_PAD_FEC_RXD0__FEC_RDATA_0,
	MX53_PAD_FEC_RXD1__FEC_RDATA_1,
	MX53_PAD_FEC_CRS_DV__FEC_RX_DV,
	MX53_PAD_FEC_RX_ER__FEC_RX_ER,
	MX53_PAD_FEC_TXD0__FEC_TDATA_0,
	MX53_PAD_FEC_TXD1__FEC_TDATA_1,
	MX53_PAD_FEC_REF_CLK__FEC_TX_CLK,
	MX53_PAD_FEC_TX_EN__FEC_TX_EN,
	MX53_PAD_SD2_CLK__GPIO1_10,
	MX53_PAD_SD2_DATA3__GPIO1_12,
	MX53_PAD_GPIO_2__GPIO1_2,
	MX53_PAD_GPIO_4__GPIO1_4,
	MX53_PAD_PATA_DATA0__GPIO2_0,
	MX53_PAD_PATA_DATA1__GPIO2_1,
	MX53_PAD_PATA_DATA10__GPIO2_10,
	MX53_PAD_PATA_DATA11__GPIO2_11,
	MX53_PAD_PATA_DATA13__GPIO2_13,
	MX53_PAD_PATA_DATA14__GPIO2_14,
	MX53_PAD_PATA_DATA15__GPIO2_15,
	MX53_PAD_PATA_DATA2__GPIO2_2,
	MX53_PAD_PATA_DATA3__GPIO2_3,
	MX53_PAD_EIM_EB2__GPIO2_30,
	MX53_PAD_PATA_DATA4__PATA_DATA_4,
	MX53_PAD_PATA_DATA5__GPIO2_5,
	MX53_PAD_PATA_DATA6__GPIO2_6,
	MX53_PAD_PATA_DATA7__GPIO2_7,
	MX53_PAD_PATA_DATA8__GPIO2_8,
	MX53_PAD_PATA_DATA9__GPIO2_9,
	MX53_PAD_GPIO_10__GPIO4_0,
	MX53_PAD_GPIO_11__GPIO4_1,
	MX53_PAD_GPIO_12__GPIO4_2,
	MX53_PAD_DISP0_DAT8__GPIO4_29,
	MX53_PAD_GPIO_13__GPIO4_3,
	MX53_PAD_DISP0_DAT16__GPIO5_10,
	MX53_PAD_DISP0_DAT17__GPIO5_11,
	MX53_PAD_CSI0_PIXCLK__GPIO5_18,
	MX53_PAD_CSI0_MCLK__GPIO5_19,
	MX53_PAD_CSI0_DATA_EN__GPIO5_20,
	MX53_PAD_CSI0_VSYNC__GPIO5_21,
	MX53_PAD_CSI0_DAT4__GPIO5_22,
	MX53_PAD_CSI0_DAT5__GPIO5_23,
	MX53_PAD_CSI0_DAT6__GPIO5_24,
	MX53_PAD_DISP0_DAT14__GPIO5_8,
	MX53_PAD_DISP0_DAT15__GPIO5_9,
	MX53_PAD_PATA_DIOW__GPIO6_17,
	MX53_PAD_PATA_DMACK__GPIO6_18,
	MX53_PAD_GPIO_17__GPIO7_12,
	MX53_PAD_GPIO_18__GPIO7_13,
	MX53_PAD_PATA_RESET_B__GPIO7_4,
	MX53_PAD_PATA_IORDY__GPIO7_5,
	MX53_PAD_PATA_DA_0__GPIO7_6,
	MX53_PAD_PATA_DA_2__GPIO7_8,
	MX53_PAD_CSI0_DAT9__I2C1_SCL,
	MX53_PAD_CSI0_DAT8__I2C1_SDA,
	MX53_PAD_KEY_COL3__I2C2_SCL,
	MX53_PAD_KEY_ROW3__I2C2_SDA,
	MX53_PAD_GPIO_5__I2C3_SCL,
	MX53_PAD_GPIO_6__I2C3_SDA,
	MX53_PAD_KEY_COL0__KPP_COL_0,
	MX53_PAD_KEY_COL1__KPP_COL_1,
	MX53_PAD_KEY_COL2__KPP_COL_2,
	MX53_PAD_KEY_COL3__KPP_COL_3,
	MX53_PAD_GPIO_19__KPP_COL_5,
	MX53_PAD_GPIO_9__KPP_COL_6,
	MX53_PAD_SD2_DATA1__KPP_COL_7,
	MX53_PAD_KEY_ROW0__KPP_ROW_0,
	MX53_PAD_KEY_ROW1__KPP_ROW_1,
	MX53_PAD_KEY_ROW2__KPP_ROW_2,
	MX53_PAD_KEY_ROW3__KPP_ROW_3,
	MX53_PAD_SD2_CMD__KPP_ROW_5,
	MX53_PAD_SD2_DATA2__KPP_ROW_6,
	MX53_PAD_SD2_DATA0__KPP_ROW_7,
	MX53_PAD_LVDS0_CLK_P__LDB_LVDS0_CLK,
	MX53_PAD_LVDS0_TX0_P__LDB_LVDS0_TX0,
	MX53_PAD_LVDS0_TX1_P__LDB_LVDS0_TX1,
	MX53_PAD_LVDS0_TX2_P__LDB_LVDS0_TX2,
	MX53_PAD_LVDS0_TX3_P__LDB_LVDS0_TX3,
	MX53_PAD_DISP0_DAT8__PWM1_PWMO,
	MX53_PAD_DISP0_DAT9__PWM2_PWMO,
	MX53_PAD_PATA_INTRQ__UART2_CTS,
	MX53_PAD_PATA_DIOR__UART2_RTS,
	MX53_PAD_PATA_BUFFER_EN__UART2_RXD_MUX,
	MX53_PAD_PATA_DMARQ__UART2_TXD_MUX,
	MX53_PAD_PATA_DA_1__UART3_CTS,
	MX53_PAD_PATA_CS_1__UART3_RXD_MUX,
	MX53_PAD_PATA_CS_0__UART3_TXD_MUX,
	MX53_PAD_CSI0_DAT17__UART4_CTS,
	MX53_PAD_CSI0_DAT16__UART4_RTS,
	MX53_PAD_CSI0_DAT13__UART4_RXD_MUX,
	MX53_PAD_CSI0_DAT12__UART4_TXD_MUX,
	MX53_PAD_CSI0_DAT19__UART5_CTS,
	MX53_PAD_CSI0_DAT18__UART5_RTS,
	MX53_PAD_CSI0_DAT15__UART5_RXD_MUX,
	MX53_PAD_CSI0_DAT14__UART5_TXD_MUX,
	MX53_PAD_GPIO_0__USBOH3_USBH1_PWR,
	MX53_PAD_DISP0_DAT12__USBOH3_USBH2_CLK,
	MX53_PAD_DISP0_DAT0__USBOH3_USBH2_DATA_0,
	MX53_PAD_DISP0_DAT1__USBOH3_USBH2_DATA_1,
	MX53_PAD_DISP0_DAT2__USBOH3_USBH2_DATA_2,
	MX53_PAD_DISP0_DAT3__USBOH3_USBH2_DATA_3,
	MX53_PAD_DISP0_DAT4__USBOH3_USBH2_DATA_4,
	MX53_PAD_DISP0_DAT5__USBOH3_USBH2_DATA_5,
	MX53_PAD_DISP0_DAT6__USBOH3_USBH2_DATA_6,
	MX53_PAD_DISP0_DAT7__USBOH3_USBH2_DATA_7,
	MX53_PAD_DI0_DISP_CLK__USBOH3_USBH2_DIR,
	MX53_PAD_DISP0_DAT11__USBOH3_USBH2_NXT,
	MX53_PAD_DISP0_DAT10__USBOH3_USBH2_STP,
	MX53_PAD_GPIO_1__WDOG2_WDOG_B,
};

#define LOCO_FEC_PHY_RST		IMX_GPIO_NR(7, 6)

static void vincell_fec_reset(void)
{
	gpio_direction_output(LOCO_FEC_PHY_RST, 0);
	mdelay(1);
	gpio_set_value(LOCO_FEC_PHY_RST, 1);
}

static struct imx_nand_platform_data nand_info = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct imx_dcd_v2_entry __dcd_entry_section dcd_entry[] = {
};

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("da9053", 0x48),
	},
};

static int vincell_devices_init(void)
{
	writel(0, MX53_M4IF_BASE_ADDR + 0xc);

	console_flush();
	imx53_init_lowlevel(1000);
	clk_set_rate(clk_lookup("nfc_podf"), 66666667);

	imx53_add_nand(&nand_info);
	imx51_iim_register_fec_ethaddr();
	imx53_add_fec(&fec_info);
	imx53_add_mmc0(NULL);
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	imx53_add_i2c0(NULL);

	vincell_fec_reset();

	armlinux_set_bootparams((void *)0x70000100);
	armlinux_set_architecture(3297);

	devfs_add_partition("nand0", SZ_1M, SZ_512K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_1M + SZ_512K, SZ_512K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	imx53_bbu_internal_nand_register_handler("nand",
		BBU_HANDLER_FLAG_DEFAULT, dcd_entry, sizeof(dcd_entry), 3 * SZ_128K, 0xf8020000);

	return 0;
}

device_initcall(vincell_devices_init);

static int vincell_part_init(void)
{
	devfs_add_partition("disk0", 0x00000, 0x80000, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("disk0", 0x80000, 0x80000, DEVFS_PARTITION_FIXED, "env0");

	return 0;
}
late_initcall(vincell_part_init);

static int vincell_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(vincell_pads, ARRAY_SIZE(vincell_pads));

	barebox_set_model("Garz & Fricke VINCELL");
	barebox_set_hostname("vincell");

	imx53_add_uart1();

	return 0;
}

console_initcall(vincell_console_init);
