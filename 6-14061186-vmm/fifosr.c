#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <signal.h>
#include "fifosr.h"

int receive(char *addr, int len, int fd)
{
	int cnt;
	cnt=read(fd,addr,len);
	return cnt;
}

int receiveUntilChar(char *addr,char endSign, int fd)
{
	char c;
	int cnt;
	while (1)
	{
		if((read(fd,&c,1))<0)
		{
			close(fd);
			return -1;
		}
		else 
		{
			if (c==endSign) 
			{
				addr[cnt]=0;
				break;
			}
			addr[cnt++]=c;
		}
	}
	return cnt;
}


int connect(char *fifo, char op, int *fd)
{
	struct stat statb;	
	if(stat(fifo,&statb)==0){
		if((*fd=open(fifo,O_WRONLY))<0)
			return -1;
		if(write(*fd,&op,1)<0)
		{
			close(*fd);
			return -1;
		}

		return 0;

	}
	return -1;
}

int arrival(char *fifo, int *fd, int spid)
{
	struct stat statb;	
	if(stat(fifo,&statb)==0){

		if(remove(fifo)<0)
		{
			return -1;
		}
	}

	if(mkfifo(fifo,0777)<0)
	{
		return -1;
	}

	if (spid) kill(spid,SIGUSR1);

	if ((*fd=open(fifo,O_RDONLY))<0) return -1;

	return 0;
}

void disconnect(int fd)
{
	close(fd);
}
