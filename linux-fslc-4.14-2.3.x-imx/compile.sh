#!/bin/sh
CPUS=`grep -c processor /proc/cpuinfo`
#make ARCH=arm imx_v4_v5_defconfig
#make ARCH=arm mx25_ed785_defconfig
#make ARCH=arm mx25_3ds_defconfig
#make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -j$CPUS $1 $2
#make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- uImage LOADADDR=0x80008000
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j$CPUS $1 $2
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- uImage LOADADDR=0x80008000
