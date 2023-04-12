export ARCH:=arm64
export CROSS_COMPILE:=aarch64-linux-

DTBO = apds9960.dtbo
DTBS = apds9960-overlay.dts

CMP = gcc
OPT_LEVEL = 0
CMPFLAGS = -Wall -Wextra -g -O$(OPT_LEVEL)
APP_IN = gesture-notification.c
APP_OUT = gesture-notification

obj-m+=apds9960.o

KDIR?=/home/oleksii/linx/raspkrn/output/build/linux-custom
PWD:=$(shell pwd)

all : module overlay userapp
	@echo "buiding all binaries for apds9960"

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
overlay:
	@dtc -I dts -O dtb -o $(DTBO) $(DTBS) -@

userapp:
	@$(CROSS_COMPILE)$(CMP) $(APP_IN) $(CMPFLAGS) -o $(APP_OUT)
	@echo "Userspace app is done, size is:"
	@size $(APP_OUT)
	@echo

.PHONY : clean

clean:
	@rm -rf *.o *~core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod
	@rm -rf ./arch ./include ./kernel *.symvers *.order
	@rm -rf $(DTBO)
	@rm -rf $(APP_OUT)

# END OF FILE
