obj-m += server.o

KID := /lib/modules/`uname -r`/build  
PWD := $(shell pwd)  
  
all:  
	make -C $(KID) M=$(PWD) modules  
  
clean:  
	rm -fr  .*.cmd .tmp_versions  *.mod.c  *mod.o server.o
