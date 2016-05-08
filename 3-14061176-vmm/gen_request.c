#include "vmm.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>

#define EXIT 1
#define READ 2
#define WRITE 3
#define EXECUTE 4
#define SWITCH 5
#define OUTPUT_MEM 6
#define OUTPUT_PAGETABLE 7
#define OUTPUT_AUXMEM 8
MemoryAccessRequest request;
struct stat statbuf;

void send(){
    int fd;
    if ((fd=open(FIFO_FILE, O_WRONLY)) < 0)
        error_sys("stat open fifo failed");

    if (write(fd, &request,sizeof(MemoryAccessRequest))<0)
        error_sys("stat write failed");
    close(fd);
}

void receive(){
    int fd;

    if ((fd=open(FIFO_FILE_OUT, O_RDONLY)) < 0)
        error_sys("open fifo failed");

    if (read(fd,&request,sizeof(MemoryAccessRequest)) < 0)
        error_sys("read failed");
    close(fd);
}

void request_read(){
    int addr;
    scanf("%d",&addr);
    request.reqType = REQUEST_READ;
    request.virAddr = addr;
    send();
    receive();
    printf("%c\n", request.value);
}

void request_exit(){
    request.reqType = REQUEST_EXIT;
    send();
    exit(0);
}

void request_write(){
    int addr;
    char c;
    scanf("%d %c",&addr, &c);
    request.virAddr = addr;
    request.reqType = REQUEST_WRITE;
    request.value = c;

    send();
    receive();
}

void request_switch(){
    int id;
    scanf("%d",&id);

    request.reqType = REQUEST_SWITCH;
    request.value = id;
    send();
    receive();
}

void request_execute(){
    int addr;
    scanf("%d",&addr);
    request.virAddr = addr;
    request.reqType = REQUEST_EXECUTE;
    send();
    receive();
}

void request_output_mem(){
    request.reqType = REQUEST_OUTPUT_MEM;
    send();
    receive();
}

void request_output_pagetable(){
    request.reqType = REQUEST_OUTPUT_PAGETABLE;
    send();
    receive();
}

void request_output_auxmem(){
    request.reqType = REQUEST_OUTPUT_AUXMEM;
    send();
    receive();
}
int main(){
    int i;
    int type;
    while(~scanf("%d",&type)){
        switch (type) {
            case READ   : request_read(); break;
            case EXIT   : request_exit(); break;
            case WRITE  : request_write(); break;
            case SWITCH : request_switch(); break;
            case EXECUTE: request_execute(); break;
            case OUTPUT_MEM : request_output_mem(); break;
            case OUTPUT_PAGETABLE : request_output_pagetable(); break;
            case OUTPUT_AUXMEM : request_output_auxmem(); break;
            default     : break;
        }
    }
    return 0;
}
