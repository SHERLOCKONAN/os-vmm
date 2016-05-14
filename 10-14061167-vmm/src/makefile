all:vmm req
vmm:vmm.c vmm.h
	cc -g -o vmm vmm.c vmm.h
req:req.c vmm.h
	cc -g -o req req.c
clean:
	rm vmm req
