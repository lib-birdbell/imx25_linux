/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NVKM_FB_REGS_04_H__
#define __NVKM_FB_REGS_04_H__

#define NV04_PFB_BOOT_0						0x00100000
#	define NV04_PFB_BOOT_0_RAM_AMOUNT			0x00000003
#	define NV04_PFB_BOOT_0_RAM_AMOUNT_32MB			0x00000000
#	define NV04_PFB_BOOT_0_RAM_AMOUNT_4MB			0x00000001
#	define NV04_PFB_BOOT_0_RAM_AMOUNT_8MB			0x00000002
#	define NV04_PFB_BOOT_0_RAM_AMOUNT_16MB			0x00000003
#	define NV04_PFB_BOOT_0_RAM_WIDTH_128			0x00000004
#	define NV04_PFB_BOOT_0_RAM_TYPE				0x00000028
#	define NV04_PFB_BOOT_0_RAM_TYPE_SGRAM_8MBIT		0x00000000
#	define NV04_PFB_BOOT_0_RAM_TYPE_SGRAM_16MBIT		0x00000008
#	define NV04_PFB_BOOT_0_RAM_TYPE_SGRAM_16MBIT_4BANK	0x00000010
#	define NV04_PFB_BOOT_0_RAM_TYPE_SDRAM_16MBIT		0x00000018
#	define NV04_PFB_BOOT_0_RAM_TYPE_SDRAM_64MBIT		0x00000020
#	define NV04_PFB_BOOT_0_RAM_TYPE_SDRAM_64MBITX16		0x00000028
#	define NV04_PFB_BOOT_0_UMA_ENABLE			0x00000100
#	define NV04_PFB_BOOT_0_UMA_SIZE				0x0000f000
#define NV04_PFB_CFG0						0x00100200

#endif
