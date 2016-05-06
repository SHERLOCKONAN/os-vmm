#include "vmm.h"
#include <string.h>
#include <stdio.h>

extern Ptr_MemoryAccessRequest ptr_memAccReq;
extern Ptr_PageTableItem pageTable;
extern BYTE actMem[ACTUAL_MEMORY_SIZE];
extern FILE *ptr_auxMem;
extern Ptr_PageTableItem actMemInfo[BLOCK_SUM];

void do_response_init() {
}
