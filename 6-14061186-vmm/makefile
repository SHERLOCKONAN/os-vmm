all: vmp vmm
vmp:	vmp.c	vmp.h vm.h vm.c fifosr.c fifosr.h
	cc -o vmp vmp.c vmp.h vm.h vm.c fifosr.c fifosr.h
vmm:	vmm.c	vmm.h vm.h vm.c fifosr.c fifosr.h
	cc -o vmm vmm.c vmm.h vm.h vm.c fifosr.c fifosr.h

clean:
	rm vmm vmp
