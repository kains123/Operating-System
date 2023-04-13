#!/bin/bash

make clean
make
make fs.img
make CPUS=1
qemu-system-i386 -nographic -serial mon:stdio -hdb fs.img xv6.img -smp 1 -m 512