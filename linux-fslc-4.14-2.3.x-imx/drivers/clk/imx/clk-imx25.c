/*
 * Copyright (C) 2009 by Sascha Hauer, Pengutronix
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <dt-bindings/clock/imx25-clock.h>	////debug
#include <soc/imx/timer.h>	////debug added by LSJ 20220518
////reference
//https://mokeedev.review/plugins/gitiles/MoKee/android_kernel_smartisan_sdm660/+/8ab418d3651b14d38498d868617a7280ccc6de08/arch/arm/mach-imx/clk-imx25.c

#define MX25_CCM_BASE_ADDR	0x53F80000
#define GPT1_BASE_ADDR      (0x53F00000 + 0x00090000)
//#define MX25_CCM_BASE_ADDR	0x10027000
#define MX25_GPT1_BASE_ADDR	(GPT1_BASE_ADDR)
//#define MX25_INT_GPT1		(NR_IRQS_LEGACY + 26)
#define MX25_INT_GPT1		54

#include "clk.h"

#define	CCM_MPCTL	(ccm + 0x00)
#define	CCM_UPCTL	(ccm + 0x04)
#define CCM_CCTL	(ccm + 0x08)
#define CCM_CGCR0	(ccm + 0x0C)
#define CCM_CGCR1	(ccm + 0x10)
#define CCM_CGCR2	(ccm + 0x14)
#define CCM_PCDR0	(ccm + 0x18)
#define CCM_PCDR1	(ccm + 0x1C)
#define CCM_PCDR2	(ccm + 0x20)
#define CCM_PCDR3	0x24
#define CCM_RCSR	0x28
#define CCM_CRDR	0x2C
#define CCM_DCVR0	0x30
#define CCM_DCVR1	0x34
#define CCM_DCVR2	0x38
#define CCM_DCVR3	0x3c
#define CCM_LTR0	0x40
#define CCM_LTR1	0x44
#define CCM_LTR2	0x48
#define CCM_LTR3	0x4c
#define CCM_MCR		(ccm + 0x64)


#define ccm(x)	(ccm_base + (x))

static struct clk_onecell_data clk_data;

static const char *cpu_sel_clks[] = { "mpll", "mpll_cpu_3_4", };
static const char *per_sel_clks[] = { "hclk", "upll", };
static const char *cko_sel_clks[] = { "dummy", "osc", "cpu", "hclk",
				      "ipg", "dummy", "dummy", "dummy",
				      "dummy", "dummy", "per0", "per2",
				      "per13", "per14", "usbotg_ahb", "dummy",};

enum mx25_clks {
	dummy, osc, mpll, upll, mpll_cpu_3_4, cpu_sel, cpu, ahb, usb_div, ipg,
	per0_sel, per1_sel, per2_sel, per3_sel, per4_sel, per5_sel, per6_sel,
	per7_sel, per8_sel, per9_sel, per10_sel, per11_sel, per12_sel,
	per13_sel, per14_sel, per15_sel, per0, per1, per2, per3, per4, per5,
	per6, per7, per8, per9, per10, per11, per12, per13, per14, per15,
	csi_ipg_per, epit_ipg_per, esai_ipg_per, esdhc1_ipg_per, esdhc2_ipg_per,
	gpt_ipg_per, i2c_ipg_per, lcdc_ipg_per, nfc_ipg_per, owire_ipg_per,
	pwm_ipg_per, sim1_ipg_per, sim2_ipg_per, ssi1_ipg_per, ssi2_ipg_per,
	uart_ipg_per, ata_ahb, reserved1, csi_ahb, emi_ahb, esai_ahb, esdhc1_ahb,
	esdhc2_ahb, fec_ahb, lcdc_ahb, rtic_ahb, sdma_ahb, slcdc_ahb, usbotg_ahb,
	reserved2, reserved3, reserved4, reserved5, can1_ipg, can2_ipg,	csi_ipg,
	cspi1_ipg, cspi2_ipg, cspi3_ipg, dryice_ipg, ect_ipg, epit1_ipg, epit2_ipg,
	reserved6, esdhc1_ipg, esdhc2_ipg, fec_ipg, reserved7, reserved8, reserved9,
	gpt1_ipg, gpt2_ipg, gpt3_ipg, gpt4_ipg, reserved10, reserved11, reserved12,
	iim_ipg, reserved13, reserved14, kpp_ipg, lcdc_ipg, reserved15, pwm1_ipg,
	pwm2_ipg, pwm3_ipg, pwm4_ipg, rngb_ipg, reserved16, scc_ipg, sdma_ipg,
	sim1_ipg, sim2_ipg, slcdc_ipg, spba_ipg, ssi1_ipg, ssi2_ipg, tsc_ipg,
	uart1_ipg, uart2_ipg, uart3_ipg, uart4_ipg, uart5_ipg, reserved17,
	wdt_ipg, cko_div, cko_sel, cko, clk_max
};

//static struct clk *clk[clk_max];
static struct clk *clk[IMX25_CLK_MAX];

static struct clk ** const uart_clks[] __initconst = {
	&clk[uart_ipg_per],
	&clk[uart1_ipg],
	&clk[uart2_ipg],
	&clk[uart3_ipg],
	&clk[uart4_ipg],
	&clk[uart5_ipg],
	NULL
};

static int __init __mx25_clocks_init(void __iomem *ccm_base)
{
	BUG_ON(!ccm_base);
#if 0
	clk[dummy] = imx_clk_fixed("dummy", 0);
	clk[mpll] = imx_clk_pllv1(IMX_PLLV1_IMX25, "mpll", "osc", ccm(CCM_MPCTL));
	clk[upll] = imx_clk_pllv1(IMX_PLLV1_IMX25, "upll", "osc", ccm(CCM_UPCTL));
	clk[mpll_cpu_3_4] = imx_clk_fixed_factor("mpll_cpu_3_4", "mpll", 3, 4);
	clk[cpu_sel] = imx_clk_mux("cpu_sel", ccm(CCM_CCTL), 14, 1, cpu_sel_clks, ARRAY_SIZE(cpu_sel_clks));
	clk[cpu] = imx_clk_divider("cpu", "cpu_sel", ccm(CCM_CCTL), 30, 2);
	clk[ahb] = imx_clk_divider("ahb", "cpu", ccm(CCM_CCTL), 28, 2);
	clk[usb_div] = imx_clk_divider("usb_div", "upll", ccm(CCM_CCTL), 16, 6); 
	clk[ipg] = imx_clk_fixed_factor("ipg", "ahb", 1, 2);
	clk[per0_sel] = imx_clk_mux("per0_sel", ccm(CCM_MCR), 0, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per1_sel] = imx_clk_mux("per1_sel", ccm(CCM_MCR), 1, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per2_sel] = imx_clk_mux("per2_sel", ccm(CCM_MCR), 2, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per3_sel] = imx_clk_mux("per3_sel", ccm(CCM_MCR), 3, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per4_sel] = imx_clk_mux("per4_sel", ccm(CCM_MCR), 4, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per5_sel] = imx_clk_mux("per5_sel", ccm(CCM_MCR), 5, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per6_sel] = imx_clk_mux("per6_sel", ccm(CCM_MCR), 6, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per7_sel] = imx_clk_mux("per7_sel", ccm(CCM_MCR), 7, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per8_sel] = imx_clk_mux("per8_sel", ccm(CCM_MCR), 8, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per9_sel] = imx_clk_mux("per9_sel", ccm(CCM_MCR), 9, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per10_sel] = imx_clk_mux("per10_sel", ccm(CCM_MCR), 10, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per11_sel] = imx_clk_mux("per11_sel", ccm(CCM_MCR), 11, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per12_sel] = imx_clk_mux("per12_sel", ccm(CCM_MCR), 12, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per13_sel] = imx_clk_mux("per13_sel", ccm(CCM_MCR), 13, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per14_sel] = imx_clk_mux("per14_sel", ccm(CCM_MCR), 14, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per15_sel] = imx_clk_mux("per15_sel", ccm(CCM_MCR), 15, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[cko_div] = imx_clk_divider("cko_div", "cko_sel", ccm(CCM_MCR), 24, 6);
	clk[cko_sel] = imx_clk_mux("cko_sel", ccm(CCM_MCR), 20, 4, cko_sel_clks, ARRAY_SIZE(cko_sel_clks));
	clk[cko] = imx_clk_gate("cko", "cko_div", ccm(CCM_MCR),  30);
	clk[per0] = imx_clk_divider("per0", "per0_sel", ccm(CCM_PCDR0), 0, 6);
	clk[per1] = imx_clk_divider("per1", "per1_sel", ccm(CCM_PCDR0), 8, 6);
	clk[per2] = imx_clk_divider("per2", "per2_sel", ccm(CCM_PCDR0), 16, 6);
	clk[per3] = imx_clk_divider("per3", "per3_sel", ccm(CCM_PCDR0), 24, 6);
	clk[per4] = imx_clk_divider("per4", "per4_sel", ccm(CCM_PCDR1), 0, 6);
	clk[per5] = imx_clk_divider("per5", "per5_sel", ccm(CCM_PCDR1), 8, 6);
	clk[per6] = imx_clk_divider("per6", "per6_sel", ccm(CCM_PCDR1), 16, 6);
	clk[per7] = imx_clk_divider("per7", "per7_sel", ccm(CCM_PCDR1), 24, 6);
	clk[per8] = imx_clk_divider("per8", "per8_sel", ccm(CCM_PCDR2), 0, 6);
	clk[per9] = imx_clk_divider("per9", "per9_sel", ccm(CCM_PCDR2), 8, 6);
	clk[per10] = imx_clk_divider("per10", "per10_sel", ccm(CCM_PCDR2), 16, 6);
	clk[per11] = imx_clk_divider("per11", "per11_sel", ccm(CCM_PCDR2), 24, 6);
	clk[per12] = imx_clk_divider("per12", "per12_sel", ccm(CCM_PCDR3), 0, 6);
	clk[per13] = imx_clk_divider("per13", "per13_sel", ccm(CCM_PCDR3), 8, 6);
	clk[per14] = imx_clk_divider("per14", "per14_sel", ccm(CCM_PCDR3), 16, 6);
	clk[per15] = imx_clk_divider("per15", "per15_sel", ccm(CCM_PCDR3), 24, 6);
	clk[csi_ipg_per] = imx_clk_gate("csi_ipg_per", "per0", ccm(CCM_CGCR0), 0);
	clk[epit_ipg_per] = imx_clk_gate("epit_ipg_per", "per1", ccm(CCM_CGCR0),  1);
	clk[esai_ipg_per] = imx_clk_gate("esai_ipg_per", "per2", ccm(CCM_CGCR0),  2);
	clk[esdhc1_ipg_per] = imx_clk_gate("esdhc1_ipg_per", "per3", ccm(CCM_CGCR0),  3);
	clk[esdhc2_ipg_per] = imx_clk_gate("esdhc2_ipg_per", "per4", ccm(CCM_CGCR0),  4);
	clk[gpt_ipg_per] = imx_clk_gate("gpt_ipg_per", "per5", ccm(CCM_CGCR0),  5);
	clk[i2c_ipg_per] = imx_clk_gate("i2c_ipg_per", "per6", ccm(CCM_CGCR0),  6);
	clk[lcdc_ipg_per] = imx_clk_gate("lcdc_ipg_per", "per7", ccm(CCM_CGCR0),  7);
	clk[nfc_ipg_per] = imx_clk_gate("nfc_ipg_per", "per8", ccm(CCM_CGCR0),  8);
	clk[owire_ipg_per] = imx_clk_gate("owire_ipg_per", "per9", ccm(CCM_CGCR0),  9);
	clk[pwm_ipg_per] = imx_clk_gate("pwm_ipg_per", "per10", ccm(CCM_CGCR0),  10);
	clk[sim1_ipg_per] = imx_clk_gate("sim1_ipg_per", "per11", ccm(CCM_CGCR0),  11);
	clk[sim2_ipg_per] = imx_clk_gate("sim2_ipg_per", "per12", ccm(CCM_CGCR0),  12);
	clk[ssi1_ipg_per] = imx_clk_gate("ssi1_ipg_per", "per13", ccm(CCM_CGCR0), 13);
	clk[ssi2_ipg_per] = imx_clk_gate("ssi2_ipg_per", "per14", ccm(CCM_CGCR0), 14);
	clk[uart_ipg_per] = imx_clk_gate("uart_ipg_per", "per15", ccm(CCM_CGCR0), 15);
	clk[ata_ahb] = imx_clk_gate("ata_ahb", "ahb", ccm(CCM_CGCR0), 16);
	/* CCM_CGCR0(17): reserved */
	clk[csi_ahb] = imx_clk_gate("csi_ahb", "ahb", ccm(CCM_CGCR0), 18);
	clk[emi_ahb] = imx_clk_gate("emi_ahb", "ahb", ccm(CCM_CGCR0), 19);
	clk[esai_ahb] = imx_clk_gate("esai_ahb", "ahb", ccm(CCM_CGCR0), 20);
	clk[esdhc1_ahb] = imx_clk_gate("esdhc1_ahb", "ahb", ccm(CCM_CGCR0), 21);
	clk[esdhc2_ahb] = imx_clk_gate("esdhc2_ahb", "ahb", ccm(CCM_CGCR0), 22);
	clk[fec_ahb] = imx_clk_gate("fec_ahb", "ahb", ccm(CCM_CGCR0), 23);
	clk[lcdc_ahb] = imx_clk_gate("lcdc_ahb", "ahb", ccm(CCM_CGCR0), 24);
	clk[rtic_ahb] = imx_clk_gate("rtic_ahb", "ahb", ccm(CCM_CGCR0), 25);
	clk[sdma_ahb] = imx_clk_gate("sdma_ahb", "ahb", ccm(CCM_CGCR0), 26);
	clk[slcdc_ahb] = imx_clk_gate("slcdc_ahb", "ahb", ccm(CCM_CGCR0), 27);
	clk[usbotg_ahb] = imx_clk_gate("usbotg_ahb", "ahb", ccm(CCM_CGCR0), 28);
	/* CCM_CGCR0(29-31): reserved */
	/* CCM_CGCR1(0): reserved in datasheet, used as audmux in FSL kernel */
	clk[can1_ipg] = imx_clk_gate("can1_ipg", "ipg", ccm(CCM_CGCR1),  2);
	clk[can2_ipg] = imx_clk_gate("can2_ipg", "ipg", ccm(CCM_CGCR1),  3);
	clk[csi_ipg] = imx_clk_gate("csi_ipg", "ipg", ccm(CCM_CGCR1),  4);
	clk[cspi1_ipg] = imx_clk_gate("cspi1_ipg", "ipg", ccm(CCM_CGCR1),  5);
	clk[cspi2_ipg] = imx_clk_gate("cspi2_ipg", "ipg", ccm(CCM_CGCR1),  6);
	clk[cspi3_ipg] = imx_clk_gate("cspi3_ipg", "ipg", ccm(CCM_CGCR1),  7);
	clk[dryice_ipg] = imx_clk_gate("dryice_ipg", "ipg", ccm(CCM_CGCR1),  8);
	clk[ect_ipg] = imx_clk_gate("ect_ipg", "ipg", ccm(CCM_CGCR1),  9);
	clk[epit1_ipg] = imx_clk_gate("epit1_ipg", "ipg", ccm(CCM_CGCR1),  10);
	clk[epit2_ipg] = imx_clk_gate("epit2_ipg", "ipg", ccm(CCM_CGCR1),  11);
	/* CCM_CGCR1(12): reserved in datasheet, used as esai in FSL kernel */
	clk[esdhc1_ipg] = imx_clk_gate("esdhc1_ipg", "ipg", ccm(CCM_CGCR1), 13);
	clk[esdhc2_ipg] = imx_clk_gate("esdhc2_ipg", "ipg", ccm(CCM_CGCR1), 14);
	clk[fec_ipg] = imx_clk_gate("fec_ipg", "ipg", ccm(CCM_CGCR1), 15);
	/* CCM_CGCR1(16): reserved in datasheet, used as gpio1 in FSL kernel */
	/* CCM_CGCR1(17): reserved in datasheet, used as gpio2 in FSL kernel */
	/* CCM_CGCR1(18): reserved in datasheet, used as gpio3 in FSL kernel */
	clk[gpt1_ipg] = imx_clk_gate("gpt1_ipg", "ipg", ccm(CCM_CGCR1), 19);
	clk[gpt2_ipg] = imx_clk_gate("gpt2_ipg", "ipg", ccm(CCM_CGCR1), 20);
	clk[gpt3_ipg] = imx_clk_gate("gpt3_ipg", "ipg", ccm(CCM_CGCR1), 21);
	clk[gpt4_ipg] = imx_clk_gate("gpt4_ipg", "ipg", ccm(CCM_CGCR1), 22);
	/* CCM_CGCR1(23): reserved in datasheet, used as i2c1 in FSL kernel */
	/* CCM_CGCR1(24): reserved in datasheet, used as i2c2 in FSL kernel */
	/* CCM_CGCR1(25): reserved in datasheet, used as i2c3 in FSL kernel */
	clk[iim_ipg] = imx_clk_gate("iim_ipg", "ipg", ccm(CCM_CGCR1), 26);
	/* CCM_CGCR1(27): reserved in datasheet, used as iomuxc in FSL kernel */
	/* CCM_CGCR1(28): reserved in datasheet, used as kpp in FSL kernel */
	clk[kpp_ipg] = imx_clk_gate("kpp_ipg", "ipg", ccm(CCM_CGCR1), 28);
	clk[lcdc_ipg] = imx_clk_gate("lcdc_ipg", "ipg", ccm(CCM_CGCR1), 29);
	/* CCM_CGCR1(30): reserved in datasheet, used as owire in FSL kernel */
	clk[pwm1_ipg] = imx_clk_gate("pwm1_ipg", "ipg", ccm(CCM_CGCR1), 31);
	clk[pwm2_ipg] = imx_clk_gate("pwm2_ipg", "ipg", ccm(CCM_CGCR2),  0);
	clk[pwm3_ipg] = imx_clk_gate("pwm3_ipg", "ipg", ccm(CCM_CGCR2),  1);
	clk[pwm4_ipg] = imx_clk_gate("pwm4_ipg", "ipg", ccm(CCM_CGCR2),  2);
	clk[rngb_ipg] = imx_clk_gate("rngb_ipg", "ipg", ccm(CCM_CGCR2),  3);
	/* CCM_CGCR2(4): reserved in datasheet, used as rtic in FSL kernel */
	clk[scc_ipg] = imx_clk_gate("scc_ipg", "ipg", ccm(CCM_CGCR2),  5);
	clk[sdma_ipg] = imx_clk_gate("sdma_ipg", "ipg", ccm(CCM_CGCR2),  6);
	clk[sim1_ipg] = imx_clk_gate("sim1_ipg", "ipg", ccm(CCM_CGCR2),  7);
	clk[sim2_ipg] = imx_clk_gate("sim2_ipg", "ipg", ccm(CCM_CGCR2),  8);
	clk[slcdc_ipg] = imx_clk_gate("slcdc_ipg", "ipg", ccm(CCM_CGCR2),  9);
	clk[spba_ipg] = imx_clk_gate("spba_ipg", "ipg", ccm(CCM_CGCR2),  10);
	clk[ssi1_ipg] = imx_clk_gate("ssi1_ipg", "ipg", ccm(CCM_CGCR2), 11);
	clk[ssi2_ipg] = imx_clk_gate("ssi2_ipg", "ipg", ccm(CCM_CGCR2), 12);
	clk[tsc_ipg] = imx_clk_gate("tsc_ipg", "ipg", ccm(CCM_CGCR2), 13);
	clk[uart1_ipg] = imx_clk_gate("uart1_ipg", "ipg", ccm(CCM_CGCR2), 14);
	clk[uart2_ipg] = imx_clk_gate("uart2_ipg", "ipg", ccm(CCM_CGCR2), 15);
	clk[uart3_ipg] = imx_clk_gate("uart3_ipg", "ipg", ccm(CCM_CGCR2), 16);
	clk[uart4_ipg] = imx_clk_gate("uart4_ipg", "ipg", ccm(CCM_CGCR2), 17);
	clk[uart5_ipg] = imx_clk_gate("uart5_ipg", "ipg", ccm(CCM_CGCR2), 18);
	/* CCM_CGCR2(19): reserved in datasheet, but used as wdt in FSL kernel */
	clk[wdt_ipg] = imx_clk_gate("wdt_ipg", "ipg", ccm(CCM_CGCR2), 19);

	imx_check_clocks(clk, ARRAY_SIZE(clk));

	clk_prepare_enable(clk[emi_ahb]);

	/* Clock source for gpt must be derived from AHB */
	clk_set_parent(clk[per5_sel], clk[ahb]);

	/*
	 * Let's initially set up CLKO parent as ipg, since this configuration
	 * is used on some imx25 board designs to clock the audio codec.
	 */
	clk_set_parent(clk[cko_sel], clk[ipg]);

	imx_register_uart_clocks(uart_clks);
#endif
	return 0;
}

////debug added by LSJ 20220517
extern void early_print(const char *str, ...);////debug
static void __iomem *ccm __initdata;
#if 1
static void __init _mx25_clocks_init(unsigned long lref, unsigned long href)
{
	BUG_ON(!ccm);
printk(KERN_ERR "**_mx25_clocks_init() lref=%lu, href=%lu\n", lref, href);////debug
	clk[IMX25_CLK_DUMMY] = imx_clk_fixed("dummy", 0);
	clk[IMX25_CLK_CKIL] = imx_obtain_fixed_clock("ckil", lref);
	clk[IMX25_CLK_CKIH] = imx_obtain_fixed_clock("ckih", href);
	clk[IMX25_CLK_MPLL] = imx_clk_pllv1(IMX_PLLV1_IMX25, "mpll", "ckih", CCM_MPCTL);
	clk[IMX25_CLK_UPLL] = imx_clk_pllv1(IMX_PLLV1_IMX25, "upll", "ckih", CCM_UPCTL);
	clk[IMX25_CLK_MPLL_GATE] = imx_clk_gate("mpll_gate", "mpll", CCM_CCTL, 22);
	clk[IMX25_CLK_MPLL_CPU_3_4] = imx_clk_fixed_factor("mpll_cpu_3_4", "mpll", 3, 4);
	clk[IMX25_CLK_ARM_SRC_SEL] = imx_clk_mux("arm_src_sel", CCM_CCTL, 14, 1, cpu_sel_clks, ARRAY_SIZE(cpu_sel_clks));
	clk[IMX25_CLK_FCLK] = imx_clk_divider("fclk", "arm_src_sel", CCM_CCTL, 30, 2);
	clk[IMX25_CLK_HCLK] = imx_clk_divider("hclk", "fclk", CCM_CCTL, 28, 2);
	clk[IMX25_CLK_IPG] = imx_clk_fixed_factor("ipg", "hclk", 1, 2);
	clk[IMX25_CLK_PER0_SEL] = imx_clk_mux("per0_sel", CCM_MCR, 0, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER1_SEL] = imx_clk_mux("per1_sel", CCM_MCR, 1, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER2_SEL] = imx_clk_mux("per2_sel", CCM_MCR, 2, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER3_SEL] = imx_clk_mux("per3_sel", CCM_MCR, 3, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER4_SEL] = imx_clk_mux("per4_sel", CCM_MCR, 4, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER5_SEL] = imx_clk_mux("per5_sel", CCM_MCR, 5, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER6_SEL] = imx_clk_mux("per6_sel", CCM_MCR, 6, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER7_SEL] = imx_clk_mux("per7_sel", CCM_MCR, 7, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER8_SEL] = imx_clk_mux("per8_sel", CCM_MCR, 8, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER9_SEL] = imx_clk_mux("per9_sel", CCM_MCR, 9, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER10_SEL] = imx_clk_mux("per10_sel", CCM_MCR, 10, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER11_SEL] = imx_clk_mux("per11_sel", CCM_MCR, 11, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER12_SEL] = imx_clk_mux("per12_sel", CCM_MCR, 12, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER13_SEL] = imx_clk_mux("per13_sel", CCM_MCR, 13, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER14_SEL] = imx_clk_mux("per14_sel", CCM_MCR, 14, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER15_SEL] = imx_clk_mux("per15_sel", CCM_MCR, 15, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[IMX25_CLK_PER1] = imx_clk_divider("per1", "per1_sel", CCM_PCDR0, 8, 6);
	clk[IMX25_CLK_PER5] = imx_clk_divider("per5", "per5_sel", CCM_PCDR1, 8, 6);
	clk[IMX25_CLK_PER8] = imx_clk_divider("per8", "per8_sel", CCM_PCDR2, 0, 6);
	clk[IMX25_CLK_GPT_IPG_PER] = imx_clk_gate("gpt_ipg_per", "per5", CCM_CGCR0,  5);
	clk[IMX25_CLK_GPT1_IPG_GATE] = imx_clk_gate("gpt1_ipg", "gpt_ipg_per", CCM_CGCR1, 19);
	clk[IMX25_CLK_UART1_IPG_GATE] = imx_clk_gate("uart1_ipg", "gpt_ipg_per", CCM_CGCR2, 46-32);
	clk[IMX25_CLK_UART1_IPG_GATE] = imx_clk_gate("uart2_ipg", "gpt_ipg_per", CCM_CGCR2, 45-32);
	clk[IMX25_CLK_NFC_GATE] = imx_clk_gate("nfc_gate", "per8", CCM_CGCR0, 8);
	//clk[IMX25_CLK_GPT1_IPG_GATE] = imx_clk_gate("gpt1_ipg_gate", "ipg", CCM_CGCR1, 19);
	//clk[IMX25_CLK_PER5] = imx_clk_divider("per5", "mpll_gate", CCM_PCDR1, 8, 6);
	//clk[IMX25_CLK_PER1] = imx_clk_divider("per1", "mpll_gate", CCM_PCDR0, 8, 6);
	//imx_clk_set_rate(clk[IMX25_CLK_PER1], 13300000);
	//printk(KERN_ERR "**_mx25_clocks_init() CKIL=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_CKIL]));
	//printk(KERN_ERR "**_mx25_clocks_init() CKIH=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_CKIH]));
	//printk(KERN_ERR "**_mx25_clocks_init() MPLL_GATE=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_MPLL_GATE]));
	//printk(KERN_ERR "**_mx25_clocks_init() MPLL=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_MPLL]));
	//printk(KERN_ERR "**_mx25_clocks_init() UPLL=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_UPLL]));
	printk(KERN_ERR "**_mx25_clocks_init() MPLL_3_4=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_MPLL_CPU_3_4]));
	printk(KERN_ERR "**_mx25_clocks_init() ARM_SRC_SEL=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_ARM_SRC_SEL]));
	printk(KERN_ERR "**_mx25_clocks_init() FCLK(cpu)=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_FCLK]));
	//printk(KERN_ERR "**_mx25_clocks_init() PER1=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_PER1]));
	printk(KERN_ERR "**_mx25_clocks_init() PER5=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_PER5]));
	printk(KERN_ERR "**_mx25_clocks_init() GPT1=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_GPT1_IPG_GATE]));
	printk(KERN_ERR "**_mx25_clocks_init() IPG=%lu\n", clk_get_rate((struct clk*)clk[IMX25_CLK_IPG]));
#if 0
	clk[mpll_cpu_3_4] = imx_clk_fixed_factor("mpll_cpu_3_4", "mpll", 3, 4);
	clk[cpu_sel] = imx_clk_mux("cpu_sel", ccm(CCM_CCTL), 14, 1, cpu_sel_clks, ARRAY_SIZE(cpu_sel_clks));
	clk[cpu] = imx_clk_divider("cpu", "cpu_sel", ccm(CCM_CCTL), 30, 2);
	clk[ahb] = imx_clk_divider("ahb", "cpu", ccm(CCM_CCTL), 28, 2);
	clk[usb_div] = imx_clk_divider("usb_div", "upll", ccm(CCM_CCTL), 16, 6); 
	clk[ipg] = imx_clk_fixed_factor("ipg", "ahb", 1, 2);
	clk[per0_sel] = imx_clk_mux("per0_sel", ccm(CCM_MCR), 0, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per1_sel] = imx_clk_mux("per1_sel", ccm(CCM_MCR), 1, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per2_sel] = imx_clk_mux("per2_sel", ccm(CCM_MCR), 2, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per3_sel] = imx_clk_mux("per3_sel", ccm(CCM_MCR), 3, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per4_sel] = imx_clk_mux("per4_sel", ccm(CCM_MCR), 4, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per5_sel] = imx_clk_mux("per5_sel", ccm(CCM_MCR), 5, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per6_sel] = imx_clk_mux("per6_sel", ccm(CCM_MCR), 6, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per7_sel] = imx_clk_mux("per7_sel", ccm(CCM_MCR), 7, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per8_sel] = imx_clk_mux("per8_sel", ccm(CCM_MCR), 8, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per9_sel] = imx_clk_mux("per9_sel", ccm(CCM_MCR), 9, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per10_sel] = imx_clk_mux("per10_sel", ccm(CCM_MCR), 10, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per11_sel] = imx_clk_mux("per11_sel", ccm(CCM_MCR), 11, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per12_sel] = imx_clk_mux("per12_sel", ccm(CCM_MCR), 12, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per13_sel] = imx_clk_mux("per13_sel", ccm(CCM_MCR), 13, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per14_sel] = imx_clk_mux("per14_sel", ccm(CCM_MCR), 14, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[per15_sel] = imx_clk_mux("per15_sel", ccm(CCM_MCR), 15, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	clk[cko_div] = imx_clk_divider("cko_div", "cko_sel", ccm(CCM_MCR), 24, 6);
	clk[cko_sel] = imx_clk_mux("cko_sel", ccm(CCM_MCR), 20, 4, cko_sel_clks, ARRAY_SIZE(cko_sel_clks));
	clk[cko] = imx_clk_gate("cko", "cko_div", ccm(CCM_MCR),  30);
	clk[per0] = imx_clk_divider("per0", "per0_sel", ccm(CCM_PCDR0), 0, 6);
	clk[per1] = imx_clk_divider("per1", "per1_sel", ccm(CCM_PCDR0), 8, 6);
	clk[per2] = imx_clk_divider("per2", "per2_sel", ccm(CCM_PCDR0), 16, 6);
	clk[per3] = imx_clk_divider("per3", "per3_sel", ccm(CCM_PCDR0), 24, 6);
	clk[per4] = imx_clk_divider("per4", "per4_sel", ccm(CCM_PCDR1), 0, 6);
	clk[per5] = imx_clk_divider("per5", "per5_sel", ccm(CCM_PCDR1), 8, 6);
	clk[per6] = imx_clk_divider("per6", "per6_sel", ccm(CCM_PCDR1), 16, 6);
	clk[per7] = imx_clk_divider("per7", "per7_sel", ccm(CCM_PCDR1), 24, 6);
	clk[per8] = imx_clk_divider("per8", "per8_sel", ccm(CCM_PCDR2), 0, 6);
	clk[per9] = imx_clk_divider("per9", "per9_sel", ccm(CCM_PCDR2), 8, 6);
	clk[per10] = imx_clk_divider("per10", "per10_sel", ccm(CCM_PCDR2), 16, 6);
	clk[per11] = imx_clk_divider("per11", "per11_sel", ccm(CCM_PCDR2), 24, 6);
	clk[per12] = imx_clk_divider("per12", "per12_sel", ccm(CCM_PCDR3), 0, 6);
	clk[per13] = imx_clk_divider("per13", "per13_sel", ccm(CCM_PCDR3), 8, 6);
	clk[per14] = imx_clk_divider("per14", "per14_sel", ccm(CCM_PCDR3), 16, 6);
	clk[per15] = imx_clk_divider("per15", "per15_sel", ccm(CCM_PCDR3), 24, 6);
	clk[csi_ipg_per] = imx_clk_gate("csi_ipg_per", "per0", ccm(CCM_CGCR0), 0);
	clk[epit_ipg_per] = imx_clk_gate("epit_ipg_per", "per1", ccm(CCM_CGCR0),  1);
	clk[esai_ipg_per] = imx_clk_gate("esai_ipg_per", "per2", ccm(CCM_CGCR0),  2);
	clk[esdhc1_ipg_per] = imx_clk_gate("esdhc1_ipg_per", "per3", ccm(CCM_CGCR0),  3);
	clk[esdhc2_ipg_per] = imx_clk_gate("esdhc2_ipg_per", "per4", ccm(CCM_CGCR0),  4);
	clk[gpt_ipg_per] = imx_clk_gate("gpt_ipg_per", "per5", ccm(CCM_CGCR0),  5);
	clk[i2c_ipg_per] = imx_clk_gate("i2c_ipg_per", "per6", ccm(CCM_CGCR0),  6);
	clk[lcdc_ipg_per] = imx_clk_gate("lcdc_ipg_per", "per7", ccm(CCM_CGCR0),  7);
	clk[nfc_ipg_per] = imx_clk_gate("nfc_ipg_per", "per8", ccm(CCM_CGCR0),  8);
	clk[owire_ipg_per] = imx_clk_gate("owire_ipg_per", "per9", ccm(CCM_CGCR0),  9);
	clk[pwm_ipg_per] = imx_clk_gate("pwm_ipg_per", "per10", ccm(CCM_CGCR0),  10);
	clk[sim1_ipg_per] = imx_clk_gate("sim1_ipg_per", "per11", ccm(CCM_CGCR0),  11);
	clk[sim2_ipg_per] = imx_clk_gate("sim2_ipg_per", "per12", ccm(CCM_CGCR0),  12);
	clk[ssi1_ipg_per] = imx_clk_gate("ssi1_ipg_per", "per13", ccm(CCM_CGCR0), 13);
	clk[ssi2_ipg_per] = imx_clk_gate("ssi2_ipg_per", "per14", ccm(CCM_CGCR0), 14);
	clk[uart_ipg_per] = imx_clk_gate("uart_ipg_per", "per15", ccm(CCM_CGCR0), 15);
	clk[ata_ahb] = imx_clk_gate("ata_ahb", "ahb", ccm(CCM_CGCR0), 16);
	/* CCM_CGCR0(17): reserved */
	clk[csi_ahb] = imx_clk_gate("csi_ahb", "ahb", ccm(CCM_CGCR0), 18);
	clk[emi_ahb] = imx_clk_gate("emi_ahb", "ahb", ccm(CCM_CGCR0), 19);
	clk[esai_ahb] = imx_clk_gate("esai_ahb", "ahb", ccm(CCM_CGCR0), 20);
	clk[esdhc1_ahb] = imx_clk_gate("esdhc1_ahb", "ahb", ccm(CCM_CGCR0), 21);
	clk[esdhc2_ahb] = imx_clk_gate("esdhc2_ahb", "ahb", ccm(CCM_CGCR0), 22);
	clk[fec_ahb] = imx_clk_gate("fec_ahb", "ahb", ccm(CCM_CGCR0), 23);
	clk[lcdc_ahb] = imx_clk_gate("lcdc_ahb", "ahb", ccm(CCM_CGCR0), 24);
	clk[rtic_ahb] = imx_clk_gate("rtic_ahb", "ahb", ccm(CCM_CGCR0), 25);
	clk[sdma_ahb] = imx_clk_gate("sdma_ahb", "ahb", ccm(CCM_CGCR0), 26);
	clk[slcdc_ahb] = imx_clk_gate("slcdc_ahb", "ahb", ccm(CCM_CGCR0), 27);
	clk[usbotg_ahb] = imx_clk_gate("usbotg_ahb", "ahb", ccm(CCM_CGCR0), 28);
	/* CCM_CGCR0(29-31): reserved */
	/* CCM_CGCR1(0): reserved in datasheet, used as audmux in FSL kernel */
	clk[can1_ipg] = imx_clk_gate("can1_ipg", "ipg", ccm(CCM_CGCR1),  2);
	clk[can2_ipg] = imx_clk_gate("can2_ipg", "ipg", ccm(CCM_CGCR1),  3);
	clk[csi_ipg] = imx_clk_gate("csi_ipg", "ipg", ccm(CCM_CGCR1),  4);
	clk[cspi1_ipg] = imx_clk_gate("cspi1_ipg", "ipg", ccm(CCM_CGCR1),  5);
	clk[cspi2_ipg] = imx_clk_gate("cspi2_ipg", "ipg", ccm(CCM_CGCR1),  6);
	clk[cspi3_ipg] = imx_clk_gate("cspi3_ipg", "ipg", ccm(CCM_CGCR1),  7);
	clk[dryice_ipg] = imx_clk_gate("dryice_ipg", "ipg", ccm(CCM_CGCR1),  8);
	clk[ect_ipg] = imx_clk_gate("ect_ipg", "ipg", ccm(CCM_CGCR1),  9);
	clk[epit1_ipg] = imx_clk_gate("epit1_ipg", "ipg", ccm(CCM_CGCR1),  10);
	clk[epit2_ipg] = imx_clk_gate("epit2_ipg", "ipg", ccm(CCM_CGCR1),  11);
	/* CCM_CGCR1(12): reserved in datasheet, used as esai in FSL kernel */
	clk[esdhc1_ipg] = imx_clk_gate("esdhc1_ipg", "ipg", ccm(CCM_CGCR1), 13);
	clk[esdhc2_ipg] = imx_clk_gate("esdhc2_ipg", "ipg", ccm(CCM_CGCR1), 14);
	clk[fec_ipg] = imx_clk_gate("fec_ipg", "ipg", ccm(CCM_CGCR1), 15);
	/* CCM_CGCR1(16): reserved in datasheet, used as gpio1 in FSL kernel */
	/* CCM_CGCR1(17): reserved in datasheet, used as gpio2 in FSL kernel */
	/* CCM_CGCR1(18): reserved in datasheet, used as gpio3 in FSL kernel */
	clk[gpt1_ipg] = imx_clk_gate("gpt1_ipg", "ipg", ccm(CCM_CGCR1), 19);
	clk[gpt2_ipg] = imx_clk_gate("gpt2_ipg", "ipg", ccm(CCM_CGCR1), 20);
	clk[gpt3_ipg] = imx_clk_gate("gpt3_ipg", "ipg", ccm(CCM_CGCR1), 21);
	clk[gpt4_ipg] = imx_clk_gate("gpt4_ipg", "ipg", ccm(CCM_CGCR1), 22);
	/* CCM_CGCR1(23): reserved in datasheet, used as i2c1 in FSL kernel */
	/* CCM_CGCR1(24): reserved in datasheet, used as i2c2 in FSL kernel */
	/* CCM_CGCR1(25): reserved in datasheet, used as i2c3 in FSL kernel */
	clk[iim_ipg] = imx_clk_gate("iim_ipg", "ipg", ccm(CCM_CGCR1), 26);
	/* CCM_CGCR1(27): reserved in datasheet, used as iomuxc in FSL kernel */
	/* CCM_CGCR1(28): reserved in datasheet, used as kpp in FSL kernel */
	clk[kpp_ipg] = imx_clk_gate("kpp_ipg", "ipg", ccm(CCM_CGCR1), 28);
	clk[lcdc_ipg] = imx_clk_gate("lcdc_ipg", "ipg", ccm(CCM_CGCR1), 29);
	/* CCM_CGCR1(30): reserved in datasheet, used as owire in FSL kernel */
	clk[pwm1_ipg] = imx_clk_gate("pwm1_ipg", "ipg", ccm(CCM_CGCR1), 31);
	clk[pwm2_ipg] = imx_clk_gate("pwm2_ipg", "ipg", ccm(CCM_CGCR2),  0);
	clk[pwm3_ipg] = imx_clk_gate("pwm3_ipg", "ipg", ccm(CCM_CGCR2),  1);
	clk[pwm4_ipg] = imx_clk_gate("pwm4_ipg", "ipg", ccm(CCM_CGCR2),  2);
	clk[rngb_ipg] = imx_clk_gate("rngb_ipg", "ipg", ccm(CCM_CGCR2),  3);
	/* CCM_CGCR2(4): reserved in datasheet, used as rtic in FSL kernel */
	clk[scc_ipg] = imx_clk_gate("scc_ipg", "ipg", ccm(CCM_CGCR2),  5);
	clk[sdma_ipg] = imx_clk_gate("sdma_ipg", "ipg", ccm(CCM_CGCR2),  6);
	clk[sim1_ipg] = imx_clk_gate("sim1_ipg", "ipg", ccm(CCM_CGCR2),  7);
	clk[sim2_ipg] = imx_clk_gate("sim2_ipg", "ipg", ccm(CCM_CGCR2),  8);
	clk[slcdc_ipg] = imx_clk_gate("slcdc_ipg", "ipg", ccm(CCM_CGCR2),  9);
	clk[spba_ipg] = imx_clk_gate("spba_ipg", "ipg", ccm(CCM_CGCR2),  10);
	clk[ssi1_ipg] = imx_clk_gate("ssi1_ipg", "ipg", ccm(CCM_CGCR2), 11);
	clk[ssi2_ipg] = imx_clk_gate("ssi2_ipg", "ipg", ccm(CCM_CGCR2), 12);
	clk[tsc_ipg] = imx_clk_gate("tsc_ipg", "ipg", ccm(CCM_CGCR2), 13);
	clk[uart1_ipg] = imx_clk_gate("uart1_ipg", "ipg", ccm(CCM_CGCR2), 14);
	clk[uart2_ipg] = imx_clk_gate("uart2_ipg", "ipg", ccm(CCM_CGCR2), 15);
	clk[uart3_ipg] = imx_clk_gate("uart3_ipg", "ipg", ccm(CCM_CGCR2), 16);
	clk[uart4_ipg] = imx_clk_gate("uart4_ipg", "ipg", ccm(CCM_CGCR2), 17);
	clk[uart5_ipg] = imx_clk_gate("uart5_ipg", "ipg", ccm(CCM_CGCR2), 18);
	/* CCM_CGCR2(19): reserved in datasheet, but used as wdt in FSL kernel */
	clk[wdt_ipg] = imx_clk_gate("wdt_ipg", "ipg", ccm(CCM_CGCR2), 19);
	/*
	clk[IMX25_CLK_FPM] = imx_clk_fixed_factor("fpm", "ckil", 512, 1);
	clk[IMX25_CLK_CKIH_DIV1P5] = imx_clk_fixed_factor("ckih_div1p5", "ckih_gate", 2, 3);

	clk[IMX21_CLK_SPLL_GATE] = imx_clk_gate("spll_gate", "spll", CCM_CSCR, 1);
	clk[IMX21_CLK_FPM_GATE] = imx_clk_gate("fpm_gate", "fpm", CCM_CSCR, 2);
	clk[IMX21_CLK_CKIH_GATE] = imx_clk_gate_dis("ckih_gate", "ckih", CCM_CSCR, 3);
	clk[IMX21_CLK_MPLL_OSC_SEL] = imx_clk_mux("mpll_osc_sel", CCM_CSCR, 4, 1, mpll_osc_sel_clks, ARRAY_SIZE(mpll_osc_sel_clks));
	clk[IMX21_CLK_IPG] = imx_clk_divider("ipg", "hclk", CCM_CSCR, 9, 1);
	clk[IMX21_CLK_HCLK] = imx_clk_divider("hclk", "fclk", CCM_CSCR, 10, 4);
	clk[IMX21_CLK_MPLL_SEL] = imx_clk_mux("mpll_sel", CCM_CSCR, 16, 1, mpll_sel_clks, ARRAY_SIZE(mpll_sel_clks));
	clk[IMX21_CLK_SPLL_SEL] = imx_clk_mux("spll_sel", CCM_CSCR, 17, 1, spll_sel_clks, ARRAY_SIZE(spll_sel_clks));
	clk[IMX21_CLK_SSI1_SEL] = imx_clk_mux("ssi1_sel", CCM_CSCR, 19, 1, ssi_sel_clks, ARRAY_SIZE(ssi_sel_clks));
	clk[IMX21_CLK_SSI2_SEL] = imx_clk_mux("ssi2_sel", CCM_CSCR, 20, 1, ssi_sel_clks, ARRAY_SIZE(ssi_sel_clks));
	clk[IMX21_CLK_USB_DIV] = imx_clk_divider("usb_div", "spll_gate", CCM_CSCR, 26, 3);
	clk[IMX21_CLK_FCLK] = imx_clk_divider("fclk", "mpll_gate", CCM_CSCR, 29, 3);

	clk[mpll] = imx_clk_pllv1(IMX_PLLV1_IMX25, "mpll", "osc", ccm(CCM_MPCTL);

	clk[IMX21_CLK_SPLL] = imx_clk_pllv1(IMX_PLLV1_IMX21, "spll", "spll_sel", CCM_SPCTL0);

	clk[IMX21_CLK_NFC_DIV] = imx_clk_divider("nfc_div", "fclk", CCM_PCDR0, 12, 4);
	clk[IMX21_CLK_SSI1_DIV] = imx_clk_divider("ssi1_div", "ssi1_sel", CCM_PCDR0, 16, 6);
	clk[IMX21_CLK_SSI2_DIV] = imx_clk_divider("ssi2_div", "ssi2_sel", CCM_PCDR0, 26, 6);

	clk[IMX21_CLK_PER2] = imx_clk_divider("per2", "mpll_gate", CCM_PCDR1, 8, 6);
	clk[IMX21_CLK_PER3] = imx_clk_divider("per3", "mpll_gate", CCM_PCDR1, 16, 6);
	clk[IMX21_CLK_PER4] = imx_clk_divider("per4", "mpll_gate", CCM_PCDR1, 24, 6);

	clk[IMX21_CLK_UART1_IPG_GATE] = imx_clk_gate("uart1_ipg_gate", "ipg", CCM_PCCR0, 0);
	clk[IMX21_CLK_UART2_IPG_GATE] = imx_clk_gate("uart2_ipg_gate", "ipg", CCM_PCCR0, 1);
	clk[IMX21_CLK_UART3_IPG_GATE] = imx_clk_gate("uart3_ipg_gate", "ipg", CCM_PCCR0, 2);
	clk[IMX21_CLK_UART4_IPG_GATE] = imx_clk_gate("uart4_ipg_gate", "ipg", CCM_PCCR0, 3);
	clk[IMX21_CLK_CSPI1_IPG_GATE] = imx_clk_gate("cspi1_ipg_gate", "ipg", CCM_PCCR0, 4);
	clk[IMX21_CLK_CSPI2_IPG_GATE] = imx_clk_gate("cspi2_ipg_gate", "ipg", CCM_PCCR0, 5);
	clk[IMX21_CLK_SSI1_GATE] = imx_clk_gate("ssi1_gate", "ipg", CCM_PCCR0, 6);
	clk[IMX21_CLK_SSI2_GATE] = imx_clk_gate("ssi2_gate", "ipg", CCM_PCCR0, 7);
	clk[IMX21_CLK_SDHC1_IPG_GATE] = imx_clk_gate("sdhc1_ipg_gate", "ipg", CCM_PCCR0, 9);
	clk[IMX21_CLK_SDHC2_IPG_GATE] = imx_clk_gate("sdhc2_ipg_gate", "ipg", CCM_PCCR0, 10);
	clk[IMX21_CLK_GPIO_GATE] = imx_clk_gate("gpio_gate", "ipg", CCM_PCCR0, 11);
	clk[IMX21_CLK_I2C_GATE] = imx_clk_gate("i2c_gate", "ipg", CCM_PCCR0, 12);
	clk[IMX21_CLK_DMA_GATE] = imx_clk_gate("dma_gate", "ipg", CCM_PCCR0, 13);
	clk[IMX21_CLK_USB_GATE] = imx_clk_gate("usb_gate", "usb_div", CCM_PCCR0, 14);
	clk[IMX21_CLK_EMMA_GATE] = imx_clk_gate("emma_gate", "ipg", CCM_PCCR0, 15);
	clk[IMX21_CLK_SSI2_BAUD_GATE] = imx_clk_gate("ssi2_baud_gate", "ipg", CCM_PCCR0, 16);
	clk[IMX21_CLK_SSI1_BAUD_GATE] = imx_clk_gate("ssi1_baud_gate", "ipg", CCM_PCCR0, 17);
	clk[IMX21_CLK_LCDC_IPG_GATE] = imx_clk_gate("lcdc_ipg_gate", "ipg", CCM_PCCR0, 18);
	clk[IMX21_CLK_NFC_GATE] = imx_clk_gate("nfc_gate", "nfc_div", CCM_PCCR0, 19);
	clk[IMX21_CLK_SLCDC_HCLK_GATE] = imx_clk_gate("slcdc_hclk_gate", "hclk", CCM_PCCR0, 21);
	clk[IMX21_CLK_PER4_GATE] = imx_clk_gate("per4_gate", "per4", CCM_PCCR0, 22);
	clk[IMX21_CLK_BMI_GATE] = imx_clk_gate("bmi_gate", "hclk", CCM_PCCR0, 23);
	clk[IMX21_CLK_USB_HCLK_GATE] = imx_clk_gate("usb_hclk_gate", "hclk", CCM_PCCR0, 24);
	clk[IMX21_CLK_SLCDC_GATE] = imx_clk_gate("slcdc_gate", "hclk", CCM_PCCR0, 25);
	clk[IMX21_CLK_LCDC_HCLK_GATE] = imx_clk_gate("lcdc_hclk_gate", "hclk", CCM_PCCR0, 26);
	clk[IMX21_CLK_EMMA_HCLK_GATE] = imx_clk_gate("emma_hclk_gate", "hclk", CCM_PCCR0, 27);
	clk[IMX21_CLK_BROM_GATE] = imx_clk_gate("brom_gate", "hclk", CCM_PCCR0, 28);
	clk[IMX21_CLK_DMA_HCLK_GATE] = imx_clk_gate("dma_hclk_gate", "hclk", CCM_PCCR0, 30);
	clk[IMX21_CLK_CSI_HCLK_GATE] = imx_clk_gate("csi_hclk_gate", "hclk", CCM_PCCR0, 31);

	clk[IMX21_CLK_CSPI3_IPG_GATE] = imx_clk_gate("cspi3_ipg_gate", "ipg", CCM_PCCR1, 23);
	clk[IMX21_CLK_WDOG_GATE] = imx_clk_gate("wdog_gate", "ipg", CCM_PCCR1, 24);
	clk[IMX21_CLK_GPT1_IPG_GATE] = imx_clk_gate("gpt1_ipg_gate", "ipg", CCM_PCCR1, 25);
	clk[IMX21_CLK_GPT2_IPG_GATE] = imx_clk_gate("gpt2_ipg_gate", "ipg", CCM_PCCR1, 26);
	clk[IMX21_CLK_GPT3_IPG_GATE] = imx_clk_gate("gpt3_ipg_gate", "ipg", CCM_PCCR1, 27);
	clk[IMX21_CLK_PWM_IPG_GATE] = imx_clk_gate("pwm_ipg_gate", "ipg", CCM_PCCR1, 28);
	clk[IMX21_CLK_RTC_GATE] = imx_clk_gate("rtc_gate", "ipg", CCM_PCCR1, 29);
	clk[IMX21_CLK_KPP_GATE] = imx_clk_gate("kpp_gate", "ipg", CCM_PCCR1, 30);
	clk[IMX21_CLK_OWIRE_GATE] = imx_clk_gate("owire_gate", "ipg", CCM_PCCR1, 31);
*/
#endif
	imx_check_clocks(clk, ARRAY_SIZE(clk));
early_print("**_mx25_clocks_init() end\n");////debug
}
#endif

int __init mx25_clocks_init(unsigned long lref, unsigned long href)
{
early_print("**mx25_clocks_init()\n");////debug
	ccm = ioremap(MX25_CCM_BASE_ADDR, SZ_2K);

	_mx25_clocks_init(lref, href);

	//clk_register_clkdev(clk[IMX25_CLK_GPT1_IPG_GATE], "ipg", "imx-gpt.0");
	//clk_register_clkdev(clk[IMX25_CLK_PER5], "per", "imx-gpt.0");
	clk_register_clkdev(clk[IMX25_CLK_GPT1_IPG_GATE], "ipg", "imx-gpt.0");
	clk_register_clkdev(clk[IMX25_CLK_PER5], "per", "imx-gpt.0");
	clk_register_clkdev(clk[IMX25_CLK_PER1], "per", "imx21-uart.0");
	clk_register_clkdev(clk[IMX25_CLK_UART1_IPG_GATE], "ipg", "imx21-uart.0");
	clk_register_clkdev(clk[IMX25_CLK_PER1], "per", "imx21-uart.1");
	clk_register_clkdev(clk[IMX25_CLK_UART2_IPG_GATE], "ipg", "imx21-uart.1");
	clk_register_clkdev(clk[IMX25_CLK_NFC_GATE], "per", "imx25-nand.0");
	/*
	clk_register_clkdev(clk[IMX21_CLK_PER1], "per", "imx21-uart.2");
	clk_register_clkdev(clk[IMX21_CLK_UART3_IPG_GATE], "ipg", "imx21-uart.2");
	clk_register_clkdev(clk[IMX21_CLK_PER1], "per", "imx21-uart.3");
	clk_register_clkdev(clk[IMX21_CLK_UART4_IPG_GATE], "ipg", "imx21-uart.3");
	clk_register_clkdev(clk[IMX21_CLK_PER2], "per", "imx21-cspi.0");
	clk_register_clkdev(clk[IMX21_CLK_CSPI1_IPG_GATE], "ipg", "imx21-cspi.0");
	clk_register_clkdev(clk[IMX21_CLK_PER2], "per", "imx21-cspi.1");
	clk_register_clkdev(clk[IMX21_CLK_CSPI2_IPG_GATE], "ipg", "imx21-cspi.1");
	clk_register_clkdev(clk[IMX21_CLK_PER2], "per", "imx21-cspi.2");
	clk_register_clkdev(clk[IMX21_CLK_CSPI3_IPG_GATE], "ipg", "imx21-cspi.2");
	clk_register_clkdev(clk[IMX21_CLK_PER3], "per", "imx21-fb.0");
	clk_register_clkdev(clk[IMX21_CLK_LCDC_IPG_GATE], "ipg", "imx21-fb.0");
	clk_register_clkdev(clk[IMX21_CLK_LCDC_HCLK_GATE], "ahb", "imx21-fb.0");
	clk_register_clkdev(clk[IMX21_CLK_USB_GATE], "per", "imx21-hcd.0");
	clk_register_clkdev(clk[IMX21_CLK_USB_HCLK_GATE], "ahb", "imx21-hcd.0");
	clk_register_clkdev(clk[IMX21_CLK_NFC_GATE], NULL, "imx21-nand.0");
	clk_register_clkdev(clk[IMX21_CLK_DMA_HCLK_GATE], "ahb", "imx21-dma");
	clk_register_clkdev(clk[IMX21_CLK_DMA_GATE], "ipg", "imx21-dma");
	clk_register_clkdev(clk[IMX21_CLK_WDOG_GATE], NULL, "imx2-wdt.0");
	clk_register_clkdev(clk[IMX21_CLK_I2C_GATE], NULL, "imx21-i2c.0");
	clk_register_clkdev(clk[IMX21_CLK_OWIRE_GATE], NULL, "mxc_w1.0");
*/
	//mxc_timer_init(MX25_GPT1_BASE_ADDR, MX25_INT_GPT1, GPT_TYPE_IMX21);
	mxc_timer_init(MX25_GPT1_BASE_ADDR, MX25_INT_GPT1, GPT_TYPE_IMX25);
early_print("**mx25_clocks_init() end\n");////debug
	return 0;
}
static void __init mx25_clocks_init_dt(struct device_node *np)
{
	void __iomem *ccm;

	ccm = of_iomap(np, 0);
	__mx25_clocks_init(ccm);

	clk_data.clks = clk;
	clk_data.clk_num = ARRAY_SIZE(clk);
	of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);
}
CLK_OF_DECLARE(imx25_ccm, "fsl,imx25-ccm", mx25_clocks_init_dt);

////debug start added by LSJ 20220517
#if 0
/* Top-level clocks */

static struct clk osc24m_clk = {
	.name = "osc24m",
	.rate = OSC24M_CLK_FREQ,
	.flags = RATE_PROPAGATES,
};

static struct clk osc32k_clk = {
	.name = "osc32k",
	.rate = OSC32K_CLK_FREQ,
	.flags = RATE_PROPAGATES,
};

static struct clk mpll_clk = {
	.name = "mpll",
	.parent = &osc24m_clk,
	.recalc = _clk_pll_recalc,
	.flags = RATE_PROPAGATES,
};

static struct clk upll_clk = {
	.name = "upll",
	.parent = &osc24m_clk,
	.recalc = _clk_pll_recalc,
	.enable = _clk_upll_enable,
	.disable = _clk_upll_disable,
	.flags = RATE_PROPAGATES,
};

static void _clk_24610k_recalc(struct clk *clk)
{
	long long temp = clk->parent->rate * 2461LL;

	do_div(temp, 24000);

	clk->rate = temp;	/* Always (UPLL * 24.61 / 240) */
}

static struct clk upll_24610k_clk = {
	.name = "upll_24610k",
	.parent = &upll_clk,
	.recalc = _clk_24610k_recalc,
	.flags = RATE_PROPAGATES,
};

/* Mid-level clocks */

static struct clk cpu_clk = {	/* ARM clock */
	.name = "cpu_clk",
	.parent = &mpll_clk,
	.set_rate = _clk_cpu_set_rate,
	.recalc = _clk_cpu_recalc,
	.round_rate = _clk_cpu_round_rate,
	.flags = RATE_PROPAGATES,
};

static struct clk ahb_clk = {	/* a.k.a. HCLK */
	.name = "ahb_clk",
	.parent = &cpu_clk,
	.recalc = _clk_ahb_recalc,
	.flags = RATE_PROPAGATES,
};

static struct clk ipg_clk = {
	.name = "ipg_clk",
	.parent = &ahb_clk,
	.recalc = _clk_ipg_recalc,
	.flags = RATE_PROPAGATES,
};

/* Bottom-level clocks */

struct clk usb_ahb_clk = {
	.name = "usb_ahb_clk",
	.id = 0,
	.parent = &ahb_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR0,
	.enable_shift = MXC_CCM_CGCR0_HCLK_USBOTG_OFFSET,
	.disable = _clk_disable,
};

struct clk rtic_clk = {
	.name = "rtic_clk",
	.id = 0,
	.parent = &ahb_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR0,
	.enable_shift = MXC_CCM_CGCR0_HCLK_RTIC_OFFSET,
	.disable = _clk_disable,
};

struct clk emi_clk = {
	.name = "emi_clk",
	.id = 0,
	.parent = &ahb_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR0,
	.enable_shift = MXC_CCM_CGCR0_HCLK_EMI_OFFSET,
	.disable = _clk_disable,
};

struct clk brom_clk = {
	.name = "brom_clk",
	.id = 0,
	.parent = &ahb_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR0,
	.enable_shift = MXC_CCM_CGCR0_HCLK_BROM_OFFSET,
	.disable = _clk_disable,
};

static struct clk per_clk[] = {
	{
	 .name = "per_csi_clk",
	 .id = 0,
	 .parent = &upll_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_epit_clk",
	 .id = 1,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_esai_clk",
	 .id = 2,
	 .parent = &ahb_clk,	/* can be AHB or UPLL or 24.61MHz */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent3,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_esdhc1_clk",
	 .id = 3,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_esdhc2_clk",
	 .id = 4,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_gpt_clk",
	 .id = 5,
	 .parent = &ahb_clk,	/* Must be AHB */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_i2c_clk",
	 .id = 6,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_lcdc_clk",
	 .id = 7,
	 .parent = &upll_clk,	/* Must be UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_nfc_clk",
	 .id = 8,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_owire_clk",
	 .id = 9,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_pwm_clk",
	 .id = 10,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_sim1_clk",
	 .id = 11,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_sim2_clk",
	 .id = 12,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_ssi1_clk",
	 .id = 13,
	 .parent = &ahb_clk,	/* can be AHB or UPLL or 24.61MHz */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent3,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_ssi2_clk",
	 .id = 14,
	 .parent = &ahb_clk,	/* can be AHB or UPLL or 24.61MHz */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent3,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
	{
	 .name = "per_uart_clk",
	 .id = 15,
	 .parent = &ahb_clk,	/* can be AHB or UPLL */
	 .round_rate = _clk_perclkx_round_rate,
	 .set_rate = _clk_perclkx_set_rate,
	 .set_parent = _clk_perclkx_set_parent,
	 .recalc = _clk_perclkx_recalc,
	 .enable = _perclk_enable,
	 .disable = _perclk_disable,
	 .flags = RATE_PROPAGATES,},
};

struct clk nfc_clk = {
	.name = "nfc_clk",
	.id = 0,
	.parent = &per_clk[8],
};

struct clk audmux_clk = {
	.name = "audmux_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR1,
	.enable_shift = MXC_CCM_CGCR1_AUDMUX_OFFSET,
	.disable = _clk_disable,
};

struct clk ata_clk[] = {
	{
	 .name = "ata_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_ATA_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &ata_clk[1],},
	{
	 .name = "ata_ahb_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_ATA_OFFSET,
	 .disable = _clk_disable,},
};

struct clk can_clk[] = {
	{
	 .name = "can_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_CAN1_OFFSET,
	 .disable = _clk_disable,},
	{
	 .name = "can_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_CAN2_OFFSET,
	 .disable = _clk_disable,},
};

struct clk csi_clk[] = {
	{
	 .name = "csi_clk",
	 .id = 0,
	 .parent = &per_clk[0],
	 .secondary = &csi_clk[1],},
	{
	 .name = "csi_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_CSI_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &csi_clk[2],},
	{
	 .name = "csi_ahb_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_CSI_OFFSET,
	 .disable = _clk_disable,},
};

struct clk cspi_clk[] = {
	{
	 .name = "cspi_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_CSPI1_OFFSET,
	 .disable = _clk_disable,},
	{
	 .name = "cspi_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_CSPI2_OFFSET,
	 .disable = _clk_disable,},
	{
	 .name = "cspi_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_CSPI3_OFFSET,
	 .disable = _clk_disable,},
};

struct clk dryice_clk = {
	.name = "dryice_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR1,
	.enable_shift = MXC_CCM_CGCR1_DRYICE_OFFSET,
	.disable = _clk_disable,
};

struct clk ect_clk = {
	.name = "ect_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR1,
	.enable_shift = MXC_CCM_CGCR1_ECT_OFFSET,
	.disable = _clk_disable,
};

struct clk epit1_clk[] = {
	{
	 .name = "epit_clk",
	 .id = 0,
	 .parent = &per_clk[1],
	 .secondary = &epit1_clk[1],},
	{
	 .name = "epit_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_EPIT1_OFFSET,
	 .disable = _clk_disable,},
};

struct clk epit2_clk[] = {
	{
	 .name = "epit_clk",
	 .id = 1,
	 .parent = &per_clk[1],
	 .secondary = &epit2_clk[1],},
	{
	 .name = "epit_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_EPIT2_OFFSET,
	 .disable = _clk_disable,},
};

struct clk esai_clk[] = {
	{
	 .name = "esai_clk",
	 .id = 0,
	 .parent = &per_clk[2],
	 .secondary = &esai_clk[1],},
	{
	 .name = "esai_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_ESAI_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &esai_clk[2],},
	{
	 .name = "esai_ahb_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_ESAI_OFFSET,
	 .disable = _clk_disable,},
};

struct clk esdhc1_clk[] = {
	{
	 .name = "esdhc_clk",
	 .id = 0,
	 .parent = &per_clk[3],
	 .secondary = &esdhc1_clk[1],},
	{
	 .name = "esdhc_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_ESDHC1_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &esdhc1_clk[2],},
	{
	 .name = "esdhc_ahb_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_ESDHC1_OFFSET,
	 .disable = _clk_disable,},
};

struct clk esdhc2_clk[] = {
	{
	 .name = "esdhc_clk",
	 .id = 1,
	 .parent = &per_clk[4],
	 .secondary = &esdhc2_clk[1],},
	{
	 .name = "esdhc_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_ESDHC2_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &esdhc2_clk[2],},
	{
	 .name = "esdhc_ahb_clk",
	 .id = 1,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_ESDHC2_OFFSET,
	 .disable = _clk_disable,},
};

struct clk fec_clk[] = {
	{
	 .name = "fec_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_FEC_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &fec_clk[1],},
	{
	 .name = "fec_ahb_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_FEC_OFFSET,
	 .disable = _clk_disable,},
};

struct clk gpio_clk[] = {
	{
	 .name = "gpio_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_GPIO1_OFFSET,
	 .disable = _clk_disable,},
	{
	 .name = "gpio_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_GPIO2_OFFSET,
	 .disable = _clk_disable,},
	{
	 .name = "gpio_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_GPIO3_OFFSET,
	 .disable = _clk_disable,},
};

static struct clk gpt1_clk[] = {
	{
	 .name = "gpt_clk",
	 .id = 0,
	 .parent = &per_clk[5],
	 .secondary = &gpt1_clk[1],},
	{
	 .name = "gpt_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_GPT1_OFFSET,
	 .disable = _clk_disable,},
};

static struct clk gpt2_clk[] = {
	{
	 .name = "gpt_clk",
	 .id = 1,
	 .parent = &per_clk[5],
	 .secondary = &gpt1_clk[1],},
	{
	 .name = "gpt_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_GPT2_OFFSET,
	 .disable = _clk_disable,},
};

static struct clk gpt3_clk[] = {
	{
	 .name = "gpt_clk",
	 .id = 2,
	 .parent = &per_clk[5],
	 .secondary = &gpt1_clk[1],},
	{
	 .name = "gpt_ipg_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_GPT3_OFFSET,
	 .disable = _clk_disable,},
};

static struct clk gpt4_clk[] = {
	{
	 .name = "gpt_clk",
	 .id = 3,
	 .parent = &per_clk[5],
	 .secondary = &gpt1_clk[1],},
	{
	 .name = "gpt_ipg_clk",
	 .id = 3,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_GPT4_OFFSET,
	 .disable = _clk_disable,},
};

struct clk i2c_clk[] = {
	{
	 .name = "i2c_clk",
	 .id = 0,
	 .parent = &per_clk[6],},
	{
	 .name = "i2c_clk",
	 .id = 1,
	 .parent = &per_clk[6],},
	{
	 .name = "i2c_clk",
	 .id = 2,
	 .parent = &per_clk[6],},
};

struct clk iim_clk = {
	.name = "iim_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR1,
	.enable_shift = MXC_CCM_CGCR1_IIM_OFFSET,
	.disable = _clk_disable,
};

struct clk iomuxc_clk = {
	.name = "iomuxc_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR1,
	.enable_shift = MXC_CCM_CGCR1_IOMUXC_OFFSET,
	.disable = _clk_disable,
};

struct clk kpp_clk = {
	.name = "kpp_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR1,
	.enable_shift = MXC_CCM_CGCR1_KPP_OFFSET,
	.disable = _clk_disable,
};

struct clk lcdc_clk[] = {
	{
	 .name = "lcdc_clk",
	 .id = 0,
	 .parent = &per_clk[7],
	 .secondary = &lcdc_clk[1],
	 .round_rate = _clk_parent_round_rate,
	 .set_rate = _clk_parent_set_rate,},
	{
	 .name = "lcdc_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_LCDC_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &lcdc_clk[2],},
	{
	 .name = "lcdc_ahb_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_LCDC_OFFSET,
	 .disable = _clk_disable,},
};

struct clk owire_clk[] = {
	{
	 .name = "owire_clk",
	 .id = 0,
	 .parent = &per_clk[9],
	 .secondary = &owire_clk[1],},
	{
	 .name = "owire_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_OWIRE_OFFSET,
	 .disable = _clk_disable,},
};

struct clk pwm1_clk[] = {
	{
	 .name = "pwm_clk",
	 .id = 0,
	 .parent = &per_clk[10],
	 .secondary = &pwm1_clk[1],},
	{
	 .name = "pwm_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR1,
	 .enable_shift = MXC_CCM_CGCR1_PWM1_OFFSET,
	 .disable = _clk_disable,},
};

struct clk pwm2_clk[] = {
	{
	 .name = "pwm_clk",
	 .id = 1,
	 .parent = &per_clk[10],
	 .secondary = &pwm2_clk[1],},
	{
	 .name = "pwm_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_PWM2_OFFSET,
	 .disable = _clk_disable,},
};

struct clk pwm3_clk[] = {
	{
	 .name = "pwm_clk",
	 .id = 2,
	 .parent = &per_clk[10],
	 .secondary = &pwm3_clk[1],},
	{
	 .name = "pwm_ipg_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_PWM3_OFFSET,
	 .disable = _clk_disable,},
};

struct clk pwm4_clk[] = {
	{
	 .name = "pwm_clk",
	 .id = 3,
	 .parent = &per_clk[10],
	 .secondary = &pwm4_clk[1],},
	{
	 .name = "pwm_ipg_clk",
	 .id = 3,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_PWM3_OFFSET,
	 .disable = _clk_disable,},
};

struct clk rng_clk = {
	.name = "rng_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR2,
	.enable_shift = MXC_CCM_CGCR2_RNGB_OFFSET,
	.disable = _clk_disable,
};

struct clk scc_clk = {
	.name = "scc_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR2,
	.enable_shift = MXC_CCM_CGCR2_SCC_OFFSET,
	.disable = _clk_disable,
};

struct clk sdma_clk[] = {
	{
	 .name = "sdma_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_SDMA_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &sdma_clk[1],},
	{
	 .name = "sdma_ahb_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_SDMA_OFFSET,
	 .disable = _clk_disable,},
};

struct clk sim1_clk[] = {
	{
	 .name = "sim1_clk",
	 .id = 0,
	 .parent = &per_clk[11],
	 .secondary = &sim1_clk[1],},
	{
	 .name = "sim_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_SIM1_OFFSET,
	 .disable = _clk_disable,},
};

struct clk sim2_clk[] = {
	{
	 .name = "sim2_clk",
	 .id = 1,
	 .parent = &per_clk[12],
	 .secondary = &sim2_clk[1],},
	{
	 .name = "sim_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_SIM2_OFFSET,
	 .disable = _clk_disable,},
};

struct clk slcdc_clk[] = {
	{
	 .name = "slcdc_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_SLCDC_OFFSET,
	 .disable = _clk_disable,
	 .secondary = &slcdc_clk[1],},
	{
	 .name = "slcdc_ahb_clk",
	 .id = 0,
	 .parent = &ahb_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR0,
	 .enable_shift = MXC_CCM_CGCR0_HCLK_SLCDC_OFFSET,
	 .disable = _clk_disable,},
};

struct clk spba_clk = {
	.name = "spba_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR2,
	.enable_shift = MXC_CCM_CGCR2_SPBA_OFFSET,
	.disable = _clk_disable,
};

struct clk ssi1_clk[] = {
	{
	 .name = "ssi_clk",
	 .id = 0,
	 .parent = &per_clk[13],
	 .secondary = &ssi1_clk[1],},
	{
	 .name = "ssi_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_SSI1_OFFSET,
	 .disable = _clk_disable,},
};

struct clk ssi2_clk[] = {
	{
	 .name = "ssi_clk",
	 .id = 1,
	 .parent = &per_clk[14],
	 .secondary = &ssi2_clk[1],},
	{
	 .name = "ssi_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_SSI2_OFFSET,
	 .disable = _clk_disable,},
};

struct clk tchscrn_clk = {
	.name = "tchscrn_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR2,
	.enable_shift = MXC_CCM_CGCR2_TCHSCRN_OFFSET,
	.disable = _clk_disable,
};

struct clk uart1_clk[] = {
	{
	 .name = "uart_clk",
	 .id = 0,
	 .parent = &per_clk[15],
	 .secondary = &uart1_clk[1],},
	{
	 .name = "uart_ipg_clk",
	 .id = 0,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_UART1_OFFSET,
	 .disable = _clk_disable,},
};

struct clk uart2_clk[] = {
	{
	 .name = "uart_clk",
	 .id = 1,
	 .parent = &per_clk[15],
	 .secondary = &uart2_clk[1],},
	{
	 .name = "uart_ipg_clk",
	 .id = 1,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_UART2_OFFSET,
	 .disable = _clk_disable,},
};

struct clk uart3_clk[] = {
	{
	 .name = "uart_clk",
	 .id = 2,
	 .parent = &per_clk[15],
	 .secondary = &uart3_clk[1],},
	{
	 .name = "uart_ipg_clk",
	 .id = 2,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_UART3_OFFSET,
	 .disable = _clk_disable,},
};

struct clk uart4_clk[] = {
	{
	 .name = "uart_clk",
	 .id = 3,
	 .parent = &per_clk[15],
	 .secondary = &uart4_clk[1],},
	{
	 .name = "uart_ipg_clk",
	 .id = 3,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_UART4_OFFSET,
	 .disable = _clk_disable,},
};

struct clk uart5_clk[] = {
	{
	 .name = "uart_clk",
	 .id = 4,
	 .parent = &per_clk[15],
	 .secondary = &uart5_clk[1],},
	{
	 .name = "uart_ipg_clk",
	 .id = 4,
	 .parent = &ipg_clk,
	 .enable = _clk_enable,
	 .enable_reg = MXC_CCM_CGCR2,
	 .enable_shift = MXC_CCM_CGCR2_UART5_OFFSET,
	 .disable = _clk_disable,},
};

struct clk wdog_clk = {
	.name = "wdog_clk",
	.id = 0,
	.parent = &ipg_clk,
	.enable = _clk_enable,
	.enable_reg = MXC_CCM_CGCR2,
	.enable_shift = MXC_CCM_CGCR2_WDOG_OFFSET,
	.disable = _clk_disable,
};


static struct clk *mxc_clks[] = {
	&osc24m_clk,
	&osc32k_clk,
	&mpll_clk,
	&upll_clk,
	&cpu_clk,
	&ahb_clk,
	&ipg_clk,
	&usb_ahb_clk,
	&per_clk[0],
	&per_clk[1],
	&per_clk[2],
	&per_clk[3],
	&per_clk[4],
	&per_clk[5],
	&per_clk[6],
	&per_clk[7],
	&per_clk[8],
	&per_clk[9],
	&per_clk[10],
	&per_clk[11],
	&per_clk[12],
	&per_clk[13],
	&per_clk[14],
	&per_clk[15],
	&nfc_clk,
	&audmux_clk,
	&ata_clk[0],
	&ata_clk[1],
	&can_clk[0],
	&can_clk[1],
	&csi_clk[0],
	&csi_clk[1],
	&csi_clk[2],
	&cspi_clk[0],
	&cspi_clk[1],
	&cspi_clk[2],
	&dryice_clk,
	&ect_clk,
	&epit1_clk[0],
	&epit1_clk[1],
	&epit2_clk[0],
	&epit2_clk[1],
	&esai_clk[0],
	&esai_clk[1],
	&esai_clk[2],
	&esdhc1_clk[0],
	&esdhc1_clk[1],
	&esdhc1_clk[2],
	&esdhc2_clk[0],
	&esdhc2_clk[1],
	&esdhc2_clk[2],
	&fec_clk[0],
	&fec_clk[1],
	&gpio_clk[0],
	&gpio_clk[1],
	&gpio_clk[2],
	&gpt1_clk[0],
	&gpt1_clk[1],
	&gpt2_clk[0],
	&gpt2_clk[1],
	&gpt3_clk[0],
	&gpt3_clk[1],
	&gpt4_clk[0],
	&gpt4_clk[1],
	&i2c_clk[0],
	&i2c_clk[1],
	&i2c_clk[2],
	&iim_clk,
	&iomuxc_clk,
	&kpp_clk,
	&lcdc_clk[0],
	&lcdc_clk[1],
	&lcdc_clk[2],
	&owire_clk[0],
	&owire_clk[1],
	&pwm1_clk[0],
	&pwm1_clk[1],
	&pwm2_clk[0],
	&pwm2_clk[1],
	&pwm3_clk[0],
	&pwm3_clk[1],
	&pwm4_clk[0],
	&pwm4_clk[1],
	&rng_clk,
	&scc_clk,
	&sdma_clk[0],
	&sdma_clk[1],
	&sim1_clk[0],
	&sim1_clk[1],
	&sim2_clk[0],
	&sim2_clk[1],
	&slcdc_clk[0],
	&slcdc_clk[1],
	&spba_clk,
	&ssi1_clk[0],
	&ssi1_clk[1],
	&ssi2_clk[0],
	&ssi2_clk[1],
	&tchscrn_clk,
	&uart1_clk[0],
	&uart1_clk[1],
	&uart2_clk[0],
	&uart2_clk[1],
	&uart3_clk[0],
	&uart3_clk[1],
	&uart4_clk[0],
	&uart4_clk[1],
	&uart5_clk[0],
	&uart5_clk[1],
	&wdog_clk,
	&usb_clk,
	&clko_clk,
};


int __init mx25_clocks_init(unsigned long fref)
{
	int i;
	struct clk **clkp;

	for (clkp = mxc_clks; clkp < mxc_clks + ARRAY_SIZE(mxc_clks); clkp++)
		clk_register(*clkp);

	/* Turn off all possible clocks */
	__raw_writel((1 << MXC_CCM_CGCR0_HCLK_EMI_OFFSET), MXC_CCM_CGCR0);

	__raw_writel((1 << MXC_CCM_CGCR1_GPT1_OFFSET) |
		     (1 << MXC_CCM_CGCR1_IIM_OFFSET), MXC_CCM_CGCR1);
	__raw_writel(1 << MXC_CCM_CGCR2_SCC_OFFSET, MXC_CCM_CGCR2);

	/* Init all perclk sources to ahb clock*/
	for (i = 0; i < (sizeof(per_clk) / sizeof(struct clk)); i++)
		per_clk[i].set_parent(&per_clk[i], &ahb_clk);

	/* This will propagate to all children and init all the clock rates */
	propagate_rate(&osc24m_clk);
	propagate_rate(&osc32k_clk);

	/* GPT clock must be derived from AHB clock */
	clk_set_rate(&per_clk[5], ahb_clk.rate / 10);

	/* LCDC clock must be derived from UPLL clock */
	clk_set_parent(&per_clk[7], &upll_clk);
	clk_set_rate(&per_clk[7], upll_clk.rate);

	/* the NFC clock must be derived from AHB clock */
	clk_set_parent(&per_clk[8], &ahb_clk);
	clk_set_rate(&per_clk[8], ahb_clk.rate / 6);

	/* sim clock */
	clk_set_rate(&per_clk[11], ahb_clk.rate / 2);

	/* the csi clock must be derived from UPLL clock */
	clk_set_parent(&per_clk[0], &upll_clk);
	clk_set_rate(&per_clk[0], upll_clk.rate / 5);

	pr_info("Clock input source is %ld\n", osc24m_clk.rate);

	clk_enable(&emi_clk);
	clk_enable(&gpio_clk[0]);
	clk_enable(&gpio_clk[1]);
	clk_enable(&gpio_clk[2]);
	clk_enable(&iim_clk);
	clk_enable(&gpt1_clk[0]);
	clk_enable(&iomuxc_clk);
	clk_enable(&scc_clk);

	mxc_timer_init(&gpt1_clk[0], IO_ADDRESS(GPT1_BASE_ADDR), MXC_INT_GPT1);
	return 0;
}
////debug end
#endif
