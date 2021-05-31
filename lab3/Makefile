obj-m :=virt_net_if.o
all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

remove:
	sudo ifconfig vni0 down
	sudo rmmod virt_net_if

install:
	sudo insmod virt_net_if.ko
	sudo ifconfig vni0 up
	sudo dhclient vni0

recompile: remove clean all install

show_logs:
	dmesg -w
