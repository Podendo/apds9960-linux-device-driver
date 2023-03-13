export ARCH:=arm64
export CROSS_COMPILE:=aarch64-linux-

DTBO = apds9960.dtbo
DTBS = apds9960-overlay.dts

obj-m+=apds9960.o

KDIR?=/home/oleksii/linx/raspkrn/output/build/linux-custom
PWD:=$(shell pwd)

all : module overlay

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

overlay:
	dtc -I dts -O dtb -o $(DTBO) $(DTBS) -@

.PHONY : clean

clean:
	rm -rf *.o *~core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod
	rm -rf ./arch ./include ./kernel *.symvers *.order
	rm -rf $(DTBO)

# END OF FILE
