#!/bin/sh

for i in $(seq 1 1 32)
do
   echo " $i"
   i2cget -fy 1 0x39 0xfc
   i2cget -fy 1 0x39 0xfd
   i2cget -fy 1 0x39 0xfe
   i2cget -fy 1 0x39 0xff
done

echo "gfifo lvl after interrupt:"
i2cget -fy 1 0x39 0xae
