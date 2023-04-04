# imx25_linux
toolchain is gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi

download tool link<BR>
<a href="download tool">https://blog.naver.com/5boon/223047897275</a><BR>
<BR>
History<BR>
-LCD tested.<BR>
-linux boot complete.<BR>
-device-tree modified.<BR>
<BR>
<BR>
# linux-4.15.7
linux for i.MX257<BR>
<BR>
Characteristic<BR>
 Use device tree<BR>
<BR>
How to make define config<BR>
 &#35; make imx25_default_defconfig<BR>
<BR>
Compiler<BR>
 arm-buildroot-linux-uclibcgnueabi-gcc.br_real (Buildroot 2021.05-git) 9.3.0<BR>
<BR>
use buildroot 2021.05<BR>
<BR>
<BR>
# linux-4.14.170
linux for i.MX257<BR>
<BR>
Characteristic<BR>
 Didn't use device-tree<BR>
<BR>
<BR>
# uboot-2018.01
uboot for i.MX257<BR>
<BR>
Specification<BR>
 CPU : i.MX25<BR>
 NAND : 128MB<BR>
 CONSOLE : serial uart2(uart2_RXD_MUX, uart2_TXD_MUX)<BR>
<BR>
How to make define config<BR>
 &#35; make mx25default_defconfig<BR>
<BR>
Compiler<BR>
 arm-buildroot-linux-uclibcgnueabi-gcc.br_real (Buildroot 2021.05-git) 9.3.0<BR>
<BR>
use buildroot 2021.05<BR>
