#include "vm.h"

#ifndef VMP_H
#define VMP_H

#ifndef DEBUG
#define DEBUG
#endif
#undef DEBUG

/* cmd类型 */
typedef enum { 
	EXIT, 
	USAGE,
	STAT,
	RAND, 
	READR, 
	REQUEST
} cmdType;


typedef struct
{
	cmdType cmdtype;
	MemoryAccessRequestType mart;
	unsigned int vaddr;
	BYTE value;
	BYTE value2;
} cmd;

void usage();
void do_rand(int num);
int do_request(MemoryAccessRequestType mart, unsigned int vaddr, BYTE value);
int do_request_k();
void do_exit();
void do_stat();
void do_init();

#endif
