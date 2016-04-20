all: vmp vmm
vmp:	vmp.c	vmp.h vm.h vm.c fifosr.c fifosr.h
	cc -o vmp vmp.c vmp.h vm.h vm.c fifosr.c fifosr.h
vmm:	vmm.c	vmm.h vm.h vm.c fifosr.c fifosr.h
	cc -o vmm vmm.c vmm.h vm.h vm.c fifosr.c fifosr.h

vmm_s:	vmm_s.c	vmm_s.h 
	cc -o vmm_s vmm_s.c vmm_s.h 

clean:
	rm vmm vmp

destroy:
	rm vmm vmp vmm_s
