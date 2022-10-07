/*
 * Copyright 2012 Sascha Hauer, Pengutronix
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <asm/mach-types.h>////
#include <asm/mach/arch.h>
#include <asm/mach/time.h>

#include "common.h"
#include "devices-imx25.h"
#include "hardware.h"

/* MX25 UART1 as console */
static const struct imxuart_platform_data uart_pdata __initconst = {
	//.flags = IMXUART_HAVE_RTSCTS,
};

#if 0
static void __init imx25_init_early(void)
{
	mxc_set_cpu_type(MXC_CPU_MX25);
}

static void __init imx25_dt_init(void)
{
	imx_aips_allow_unprivileged_access("fsl,imx25-aips");
}
/*////debug
static void __init mx25_init_irq(void)
{
	struct device_node *np;
	void __iomem *avic_base;

	np = of_find_compatible_node(NULL, NULL, "fsl,avic");
	avic_base = of_iomap(np, 0);
	BUG_ON(!avic_base);
	mxc_init_irq(avic_base);
}*/

static const char * const imx25_dt_board_compat[] __initconst = {
	"fsl,imx25",
	NULL
};

DT_MACHINE_START(IMX25_DT, "Freescale i.MX25 (Device Tree Support)")
	.init_early	= imx25_init_early,
	.init_machine	= imx25_dt_init,
	.init_late      = imx25_pm_init,
	//.init_irq	= mx25_init_irq,////debug
	.dt_compat	= imx25_dt_board_compat,
MACHINE_END
#endif

extern void __init early_print(const char *str, ...);
void __init mx25_init_irq(void){
	//int i;

	printk(KERN_ERR "**mx25_init_irq()\n");////debug

	mxc25_init_irq(MX25_IO_ADDRESS(MX25_ASIC_BASE_ADDR));
}

void __init mx25_timer_init(void){
	printk(KERN_ERR "**mx25_timer_init()\n");////debug
	mx25_clocks_init(32768, 24000000);
}

static struct mtd_partition mxc_nand_partitions[] = {
        {
         .name = "nand.bootloader",
         .offset = 0,
         .size = 2 * 1024 * 1024},
        {
         .name = "nand.bootlogo",
         .offset = MTDPART_OFS_APPEND,
         .size = 1 * 1024 * 1024},
        {
         .name = "nand.kernel",
         .offset = MTDPART_OFS_APPEND,
         .size = 5 * 1024 * 1024},
        {
         .name = "nand.rootfs",
         .offset = MTDPART_OFS_APPEND,
         .size = 115 * 1024 * 1024},
        {
         .name = "nand.configure",
         .offset = MTDPART_OFS_APPEND,
         .size = MTDPART_SIZ_FULL},
};

static const struct mxc_nand_platform_data
mx25_nand_board_info __initconst = {
	.width = 1,
	.parts = mxc_nand_partitions,
	.nr_parts = ARRAY_SIZE(mxc_nand_partitions),
	.hw_ecc = 1,
};

static void __init mx25_board_init(void)
{
	early_print("**mx25_board_init()\n");////debug

	imx25_soc_init();

	imx25_add_imx_uart0(&uart_pdata);
	imx25_add_imx_uart1(&uart_pdata);
	imx25_add_mxc_nand(&mx25_nand_board_info);

	/*

	mxc_gpio_setup_multiple_pins(mx21ads_pins, ARRAY_SIZE(mx21ads_pins),
			"mx21ads");

	imx21_add_imx_uart0(&uart_pdata_rts);
	imx21_add_imx_uart2(&uart_pdata_norts);
	imx21_add_imx_uart3(&uart_pdata_rts);
	imx21_add_mxc_nand(&mx21ads_nand_board_info);

	imx21_add_imx_fb(&mx21ads_fb_data);*/
}

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MX25_3DS data structure.
 */
/* *INDENT-OFF* */
MACHINE_START(MX25_3DS, "Freescale MX25 3-Stack Board")
	/* Maintainer: Freescale Semiconductor, Inc. */
	//.phys_io = AIPS1_BASE_ADDR,
	//.io_pg_offst = ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	//.boot_params = PHYS_OFFSET + 0x100,
	////.fixup = fixup_mxc_board,
	.map_io = mx25_map_io,
	//.init_irq = mxc_init_irq,
	.init_irq = mx25_init_irq,
	////.init_irq = mx25_init_irq,
	.init_time = mx25_timer_init,
	//.init_time = mx25_3stack_timer_init,
	.init_machine = mx25_board_init,
MACHINE_END
