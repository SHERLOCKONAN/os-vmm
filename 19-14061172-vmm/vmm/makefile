all: vmm.out fifo.out
vmm.out: vmm.h vmm.c
	gcc vmm.c -o vmm.out
fifo.out: vmm.h fifo.c
	gcc fifo.c -o fifo.out