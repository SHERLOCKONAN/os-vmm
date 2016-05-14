#ifndef PROC_H
#define PROC_H
#include <stdio.h>
#include "global.h"
#include "page_table.h"
typedef struct _proc_info{
	pid_t pid;
	FILE* file;
	struct _page_table table;
	struct _page_table tables[8];
	struct _proc_info *next;
}*proc_info;


int addProc(pid_t pid);
void rmProc(pid_t pid);
page_table getTable(pid_t pid);
page_table getTables(pid_t pid);
FILE* getFile(pid_t pid);

ERROR_CODE proc_read(pid_t pid, ushort address, uchar* res);
ERROR_CODE proc_write(pid_t pid, ushort address, uchar data);
ERROR_CODE proc_execute(pid_t pid, ushort address);

void proc_print(pid_t pid);

#endif	//PROC_H
