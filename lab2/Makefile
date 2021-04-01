obj-m :=main.o
all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
do:
	sudo insmod main.ko
rm:
	sudo rmmod main.ko
