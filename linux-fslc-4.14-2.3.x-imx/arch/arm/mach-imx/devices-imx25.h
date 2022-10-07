/* Made by LSJ 20220923 */
#include "devices/devices-common.h"

extern const struct imx_imx_uart_1irq_data imx25_imx_uart_data[];
#define imx25_add_imx_uart(id, pdata)	\
	imx_add_imx_uart_1irq(&imx25_imx_uart_data[id], pdata)
#define imx25_add_imx_uart0(pdata)	imx25_add_imx_uart(0, pdata)
#define imx25_add_imx_uart1(pdata)	imx25_add_imx_uart(1, pdata)
#define imx25_add_imx_uart2(pdata)	imx25_add_imx_uart(2, pdata)
#define imx25_add_imx_uart3(pdata)	imx25_add_imx_uart(3, pdata)
#define imx25_add_imx_uart4(pdata)	imx25_add_imx_uart(4, pdata)

extern const struct imx_mxc_nand_data imx25_mxc_nand_data;
#define imx25_add_mxc_nand(pdata)	\
	imx_add_mxc_nand(&imx25_mxc_nand_data, pdata)
