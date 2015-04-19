#!/bin/bash

# location
if [ "${1}" != "" ]; then
	export KERNELDIR=`readlink -f ${1}`;
else
	export KERNELDIR=`readlink -f .`;
fi;

export PARENT_DIR=`readlink -f ${KERNELDIR}/..`;
export INITRAMFS_SOURCE=`readlink -f $KERNELDIR/../initramfs`;
export INITRAMFS_TMP=${KERNELDIR}/tmp/initramfs_source;

# check if parallel installed, if not install
if [ ! -e /usr/bin/parallel ]; then
	echo "You must install 'parallel' by this script to continue.";
	sudo dpkg -i ${KERNELDIR}/utilities/parallel_20120422-1_all.deb
fi

# check if ccache installed, if not install
if [ ! -e /usr/bin/ccache ]; then
	echo "You must install 'ccache' to continue.";
	sudo apt-get install ccache
fi

# check if xmllint installed, if not install
if [ ! -e /usr/bin/xmllint ]; then
	echo "You must install 'xmllint' to continue.";
	sudo apt-get install libxml2-utils
fi

# kernel
export ARCH=arm;
export SUB_ARCH=arm;
export KERNEL_CONFIG="halaszk_trelte_defconfig";

# build script
export USER=`whoami`;
export TMPFILE=`mktemp -t`;

# system compiler
#export CROSS_COMPILE=/home/dev/KERNEL/arm-eabi-4.9.x/bin/arm-linux-gnueabihf-
export CROSS_COMPILE=/home/dev/KERNEL/arm-cortex-linux-gnueabi-linaro_4.9/bin/arm-cortex-linux-gnueabi-

#CPU Core
export NUMBEROFCPUS=`grep 'processor' /proc/cpuinfo | wc -l`;

# Colorize and add text parameters
export red=$(tput setaf 1)             #  red
export grn=$(tput setaf 2)             #  green
export blu=$(tput setaf 4)             #  blue
export cya=$(tput setaf 6)             #  cyan
export txtbld=$(tput bold)             #  Bold
export bldred=${txtbld}$(tput setaf 1) #  red
export bldgrn=${txtbld}$(tput setaf 2) #  green
export bldblu=${txtbld}$(tput setaf 4) #  blue
export bldcya=${txtbld}$(tput setaf 6) #  cyan
export txtrst=$(tput sgr0)             #  Reset

