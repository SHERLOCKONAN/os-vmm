CFLAGS+=-g
#CFLAGS+=-m32
#CFLAGS+=-D DEBUG
vmm: vmm.o proc.o vmm_v.o page_table.o 

vmm.o: vmm.h vmm.c

vmm_v.o: vmm_v.c

page_table.o: page_table.c page_table.h

proc.o: proc.c proc.h

clean:
	rm vmm *.o
