#include "vmm.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/types.h>

struct stat statbuf;
int fifo;

extern Ptr_MemoryAccessRequest ptr_memAccReq;

void do_request_init() {
    if (stat(FIFO_FILE, &statbuf) == 0){
        if (remove(FIFO_FILE) < 0)
            error_sys("remove failed");
    }

    if (mkfifo(FIFO_FILE,0666) < 0)
        error_sys("mkfifo failed");
}

void do_request() {
    int fd;

    if ((fd=open(FIFO_FILE, O_RDONLY)) < 0)
        error_sys("open fifo failed");
    if (read(fd, ptr_memAccReq ,sizeof(MemoryAccessRequest)) < 0)
        error_sys("read failed");
    close(fd);
    if (ptr_memAccReq->reqType == REQUEST_EXIT)
        exit(0);
}

void do_request_sendback() {
    int fd;
    if ((fd=open(FIFO_FILE_OUT, O_WRONLY)) < 0)
        error_sys("open fifo failed");
    if (write(fd, ptr_memAccReq , sizeof(MemoryAccessRequest))<0)
        error_sys("write failed");
    close(fd);
}
