/*
 * Copyright (C) 2013 duhuanpeng <u74147@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/** @file
 *  This File contains declaration for early output support
 */
#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <asm/io.h>

static inline void PUTC_LL(char ch)
{
	return;
}

#endif
