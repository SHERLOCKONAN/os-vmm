#include "vmm.h"
#include <string.h>
#include <stdio.h>

extern Ptr_MemoryAccessRequest ptr_memAccReq;
extern Ptr_PageTableItem pageTable;
extern BYTE actMem[ACTUAL_MEMORY_SIZE];
extern FILE *ptr_auxMem;
extern Ptr_PageTableItem actMemInfo[BLOCK_SUM];

typedef int vector[10];

void do_response_init() {
	vector x;
	printf("%d\n",sizeof(x));
}

void do_response() {
}
