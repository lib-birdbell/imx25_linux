/*
 * arch/arm/mach-imx/mm-imx21.c
 *
 * Copyright (C) 2008 Juergen Beisert (kernel@pengutronix.de)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <linux/mm.h>
#include <linux/init.h>
#include <linux/pinctrl/machine.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>

#include "common.h"
#include "devices/devices-common.h"
#include "hardware.h"
#include "iomux-v1.h"

#include "mx25.h"////debug

/* MX25 memory map definition */
static struct map_desc imx25_io_desc[] __initdata = {
	{
	 .virtual = IRAM_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(IRAM_BASE_ADDR),
	 .length = IRAM_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = X_MEMC_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(X_MEMC_BASE_ADDR),
	 .length = X_MEMC_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = NFC_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(NFC_BASE_ADDR),
	 .length = NFC_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = ROMP_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(ROMP_BASE_ADDR),
	 .length = ROMP_SIZE,
	 .type = MT_DEVICE},
	imx_map_entry(MX25, ASIC, MT_DEVICE),
	/*
	{
	 .virtual = ASIC_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(ASIC_BASE_ADDR),
	 .length = ASIC_SIZE,
	 .type = MT_DEVICE},*/
	{
	 .virtual = AIPS1_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(AIPS1_BASE_ADDR),
	 .length = AIPS1_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = SPBA0_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(SPBA0_BASE_ADDR),
	 .length = SPBA0_SIZE,
	 .type = MT_DEVICE},
	{
	 .virtual = AIPS2_BASE_ADDR_VIRT,
	 .pfn = __phys_to_pfn(AIPS2_BASE_ADDR),
	 .length = AIPS2_SIZE,
	 .type = MT_DEVICE},
};

/*
 * Initialize the memory map. It is called during the
 * system startup to create static physical to virtual
 * memory map for the IO modules.
 */
void __init mx25_map_io(void)
{
	iotable_init(imx25_io_desc, ARRAY_SIZE(imx25_io_desc));
}
#if 0
void __init imx21_init_early(void)
{
	mxc_set_cpu_type(MXC_CPU_MX21);
	imx_iomuxv1_init(MX21_IO_ADDRESS(MX21_GPIO_BASE_ADDR),
			MX21_NUM_GPIO_PORT);
}

void __init mx21_init_irq(void)
{
	mxc_init_irq(MX21_IO_ADDRESS(MX21_AVIC_BASE_ADDR));
}

////debug start added by LSJ 20220518
extern void early_print(const char *str, ...);////debug
void __init mx25_init_irq(void)
{
	early_print("**mx25_init_irq() MX21_AVIC_BASE_ADDR=%xh -> 0x68000000\n", MX21_AVIC_BASE_ADDR);////debug
	early_print("**mx25_init_irq() !0x68000000 or 0xFC400000\n");////debug
	//mxc_init_irq(MX21_IO_ADDRESS(MX21_AVIC_BASE_ADDR));
	//mxc_init_irq(MX21_IO_ADDRESS(0x68000000));
	//mxc_init_irq((void __iomem *)0xfc400000);
	mxc25_init_irq((void __iomem *)0x68000000);
}
////debug end

static const struct resource imx21_audmux_res[] __initconst = {
	DEFINE_RES_MEM(MX21_AUDMUX_BASE_ADDR, SZ_4K),
};
#endif
extern void early_print(const char *str, ...);////debug
void __init imx25_soc_init(void)
{
	early_print("**imx25_soc_init()\n");////debug
	//mxc_arch_reset_init(MX21_IO_ADDRESS(MX21_WDOG_BASE_ADDR));
	mxc_device_init();

#if 0
	mxc_register_gpio("imx25-gpio", 0, MX25_GPIO1_BASE_ADDR, SZ_256, MX25_INT_GPIO, 0);
	mxc_register_gpio("imx25-gpio", 1, MX25_GPIO2_BASE_ADDR, SZ_256, MX25_INT_GPIO, 0);
	mxc_register_gpio("imx21-gpio", 2, MX21_GPIO3_BASE_ADDR, SZ_256, MX21_INT_GPIO, 0);
	mxc_register_gpio("imx21-gpio", 3, MX21_GPIO4_BASE_ADDR, SZ_256, MX21_INT_GPIO, 0);
	mxc_register_gpio("imx21-gpio", 4, MX21_GPIO5_BASE_ADDR, SZ_256, MX21_INT_GPIO, 0);
	mxc_register_gpio("imx21-gpio", 5, MX21_GPIO6_BASE_ADDR, SZ_256, MX21_INT_GPIO, 0);

	pinctrl_provide_dummies();
	imx_add_imx_dma("imx21-dma", MX21_DMA_BASE_ADDR,
			MX21_INT_DMACH0, 0); /* No ERR irq */
	platform_device_register_simple("imx21-audmux", 0, imx21_audmux_res,
					ARRAY_SIZE(imx21_audmux_res));
#endif
}