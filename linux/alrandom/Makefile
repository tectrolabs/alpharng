obj-m		+= alrandom.o 
KERN_SRC	:= /lib/modules/$(shell uname -r)/build/
PWD		:= $(shell pwd)

all:
	make -C $(KERN_SRC) M=$(PWD) modules

install:
	make -C $(KERN_SRC) M=$(PWD) modules_install
	depmod -a

clean:
	make -C $(KERN_SRC) M=$(PWD) clean
