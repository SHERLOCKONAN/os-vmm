vmm: vmm.c vmm.h input
	gcc -o vmm vmm.c vmm.h
input: input.c  vmm.h 
	gcc -o input input.c vmm.h
clean:
	rm vmm input