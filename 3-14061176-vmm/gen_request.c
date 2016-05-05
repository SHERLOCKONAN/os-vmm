#include "vmm.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/types.h>


#define EXIT 1
#define READ 2
#define WRITE 3
#define EXECUTE 4
#define SWITCH 5

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

void recieve(){
    int fd;
    if (stat(FIFO_FILE, &statbuf) == 0){
        if (remove(FIFO_FILE_OUT) < 0)
            error_sys("remove failed");
    }

    if (mkfifo(FIFO_FILE_OUT,0666) < 0)
        error_sys("mkfifo failed");

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
    recieve();
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

    request.reqType = REQUEST_WRITE;
    request.value = c;
    send();
    recieve();
}

void request_switch(){
    int id;
    scanf("%d",&id);
    request.reqType = REQUEST_SWITCH;
    request.value = id;
    send();
    recieve();
}

void request_execute(){
    request.reqType = REQUEST_EXECUTE;
    send();
    recieve();
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
            default     : break;
        }
    }
    return 0;
}
