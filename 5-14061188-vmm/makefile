vmm: vmm.c vmm.h req
	gcc -o vmm vmm.c vmm.h
req: req.c vmm.h
	gcc -o req req.c vmm.h
clean:
	rm vmm vmm_auxMem req
