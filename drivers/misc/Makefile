obj-m += sunxi-mipsloader.o

KDIR := $(PWD)/../../linux-6.16.7
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean