#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include "vmm.h"

int main() {

    int No = 0, fd_0, fd_1;
    int type;
    unsigned long va;
    unsigned char writevalue;
    char c;

    fd_0 = open ("/tmp/server_0", O_WRONLY);

    fd_1 = open ("/tmp/server_1", O_RDONLY | O_NONBLOCK);

    MemoryAccessRequest r;
    r.exist = 1;
    r.ifnew = 1;
    r.address_fifo = 0;
    r.ifcomplete = 0;
    r.reqType = REQUEST_READ;
    r.virAddr = 0;
    r.value = 0x00u;

    do {

	write (fd_0, &r, DATALEN);

	No = 0;

	while (No == 0)
	    read (fd_1, &No, sizeof(int));
    
	if (No == 5)
	    sleep (10);
	
    } while (No == 5);
	
    close (fd_1);

    while (1) {

	
	do {

	    printf("输入请求类型和虚存地址:\n");
	    scanf("%d %lu", &type, &va);
	    if (type < 0 || type > 3)
		printf("输入有误！再次输入:\n");
	    if (type != 3 && (va < 0 || va >= VIRTUAL_MEMORY_SIZE))
		printf("输入有误！再次输入:\n");
	    getchar();

	} while ((type < 0 || type > 3) ||
		 (type != 3 && (va < 0 || va >= VIRTUAL_MEMORY_SIZE)));
		 
	if (type == 3) {
	    r.exist = 1;
	    r.ifnew = 0;
	    r.address_fifo = No;
	    r.ifcomplete = 1;
	    r.reqType = REQUEST_READ;
	    r.virAddr = 0;
	    r.value = 0x00u;
	} else if (type == 1) {
	    printf("输入写请求的值:\n");
	    scanf("%c", &writevalue);
	    c = 'a';
	    while (c != '\n'){
			c = getchar();
		}
	    r.exist = 1;
	    r.ifnew = 0;
	    r.address_fifo = No;
        r.ifcomplete = 0;
	    r.reqType = REQUEST_WRITE;
	    r.virAddr = va;
	    r.value = writevalue;
	} else {
	    r.exist = 1;
	    r.ifnew = 0;
	    r.address_fifo = No;
	    r.ifcomplete = 0;
	    r.reqType = (type == 0) ? REQUEST_READ : REQUEST_EXECUTE;
	    r.virAddr = va;
	    r.value = 0x00u;
	}

	write (fd_0, &r, DATALEN);

	if (r.ifcomplete == 1)
	    break;

    }	

    close (fd_0);

}
