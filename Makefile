obj-m += uart_driver.o  # Name of the module file

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod uart_driver.ko

unload:
	sudo rmmod uart_driver

dmesg:
	dmesg | tail -20

