#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory /tmp/aeld for output"
else
	OUTDIR=$1
	echo "Using passed directory /tmp/aeld for output"
fi

mkdir -p /tmp/aeld

cd "$OUTDIR"
if [ ! -d "/tmp/aeld/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN /tmp/aeld"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e /tmp/aeld/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "/tmp/aeld/rootfs" ]
then
	echo "Deleting rootfs directory at /tmp/aeld/rootfs and starting over"
    sudo rm  -rf /tmp/aeld/rootfs
fi

# TODO: Create necessary base directories
mkdir -p /tmp/aeld/rootfs/{bin,dev,etc,home,lib,proc,sbin,sys,tmp,usr,var}
mkdir -p /tmp/aeld/rootfs/usr/{bin,sbin,lib}
mkdir -p /tmp/aeld/rootfs/var/log

cd "$OUTDIR"
if [ ! -d "/tmp/aeld/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=/tmp/aeld/rootfs install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a /tmp/aeld/busybox/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a /tmp/aeld/busybox/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
mkdir -p /tmp/aeld/rootfs/lib64
cp -a ${SYSROOT}/lib/ld-linux-aarch64.so.1 /tmp/aeld/rootfs/lib/
cp -a ${SYSROOT}/lib64/libm.so.6 /tmp/aeld/rootfs/lib64/
cp -a ${SYSROOT}/lib64/libresolv.so.2 /tmp/aeld/rootfs/lib64/
cp -a ${SYSROOT}/lib64/libc.so.6 /tmp/aeld/rootfs/lib64/


# TODO: Make device nodes
sudo mknod -m 666 /tmp/aeld/rootfs/dev/null c 1 3
sudo mknod -m 666 /tmp/aeld/rootfs/dev/console c 5 1


# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
cp writer /tmp/aeld/rootfs/home/

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp -a ${FINDER_APP_DIR}/finder.sh /tmp/aeld/rootfs/home/
cp -a ${FINDER_APP_DIR}/finder-test.sh /tmp/aeld/rootfs/home/
cp -a ${FINDER_APP_DIR}/conf/ /tmp/aeld/rootfs/home/
cp -a ${FINDER_APP_DIR}/autorun-qemu.sh /tmp/aeld/rootfs/home/

# TODO: Chown the root directory
sudo chown -R root:root /tmp/aeld/rootfs

# TODO: Create initramfs.cpio.gz
cd /tmp/aeld/rootfs
find . | cpio -H newc -ov --owner root:root > /tmp/aeld/initramfs.cpio
gzip -f /tmp/aeld/initramfs.cpio
echo $(pwd)
echo /tmp/aeld/linux-stable/arch/${ARCH}/boot/Image
sudo cp -f /tmp/aeld/linux-stable/arch/${ARCH}/boot/Image ../