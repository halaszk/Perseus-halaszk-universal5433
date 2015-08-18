#!/bin/bash

cp -pv .config .config.bkp;
make ARCH=arm CROSS_COMPILE=/home/dev/KERNEL/arm-eabi-4.6/bin/arm-eabi- mrproper;
cp -pv .config.bkp .config;
make clean;
rm -rf tmp/*
rm dt.img
rm ramdisk.gz
rm zImage

# clean ccache
read -t 5 -p "clean ccache, 5sec timeout (y/n)?";
if [ "$REPLY" == "y" ]; then
ccache -C;
fi;
