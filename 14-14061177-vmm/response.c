#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vmm.h"

int main(int argc,char *argv[]){
	int fd;
	cmd rescmd;
	rescmd.RT=RESPONSE;
	if((fd=open("/tmp/doreq",O_WRONLY))<0){
		printf("response open fifo failed\n");
		return;
	}
	if(write(fd,&rescmd,CMDLEN)<0){
		printf("response write failed\n");
		return;
	}
	close(fd);
	return 0;		
}
