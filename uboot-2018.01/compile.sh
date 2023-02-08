#!/bin/sh
CPUS=`grep -c processor /proc/cpuinfo`
make mx25default_defconfig
make ARCH=arm CROSS_COMPILE="/home/sjlee/ED-785/buildroot-2021.05_785/output/host/bin/arm-buildroot-linux-uclibcgnueabi-" -j$CPUS
