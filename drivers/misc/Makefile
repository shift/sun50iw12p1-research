# HY300 Projector Misc Drivers
obj-$(CONFIG_SUNXI_MIPSLOADER) += sunxi-mipsloader.o
obj-$(CONFIG_SUNXI_NSI) += sunxi-nsi.o
obj-$(CONFIG_HY300_KEYSTONE_MOTOR) += hy300-keystone-motor.o

KDIR := $(PWD)/../../linux-6.16.7
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean