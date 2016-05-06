#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include "fifosr.h"
#include "vmp.h"
#define _STAT receive(&_st,1,fd2)



/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

/* cmd */
cmd *command;
char _st;
int fd,fd2;
char statbuf[1<<16];
int pid, spid;
/* 初始化 */
void do_init()
{
	command=(cmd*)malloc(sizeof(cmd));
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	if (connect(REQFIFO,'i',&fd)<0)
	{
		printf("Initiailization failed. \n");
		exit(1);
	}
	else {
		pid=getpid();
		if (write(fd,&pid,4)<0||arrival(RESFIFO,&fd2,0)<0||_STAT<0||_st=='f'||receive(&spid,4,fd2)<0)
		{
			printf("Initiailization failed. \n");
			disconnect(fd2);
			disconnect(fd);
			exit(1);
		}
		printf("sysid %d\n", spid);
	}
	disconnect(fd2);
	disconnect(fd);
	usage();
	signal(SIGINT,do_exit);
}

/*  exit and clear */
void do_exit()
{
	free(command);
	free(ptr_memAccReq);
	if (connect(REQFIFO,'e',&fd)<0)
	{
		exit(1);
	}
	else {
		if (write(fd,&pid,4)<0)
		{
			disconnect(fd2);
			disconnect(fd);
			exit(1);
		}
		else {
			if (arrival(RESFIFO,&fd2,spid)<0||_STAT<0||_st=='f')
			{
				disconnect(fd2);
				disconnect(fd);
				exit(1);				
			}
		}		
	}
	disconnect(fd2);
	disconnect(fd);
	printf("bye\n");
	exit(0);
}

/*  apply for status */
void do_stat()
{

	if (connect(REQFIFO,'s',&fd)<0)
	{
		printf("Pull status list failed. \n");		
		return;
	}
	else {
		if (write(fd,&pid,4)<0)
		{
			printf("Pull status list failed. \n");		
			disconnect(fd2);
			disconnect(fd);
			return;			
		}
		else 
		{
			if (arrival(RESFIFO,&fd2,spid)<0||_STAT<0||_st=='f'||receiveUntilChar(statbuf,'$',fd2)<0) 
			{
				printf("Pull status list failed. \n");		
				disconnect(fd2);
				disconnect(fd);				
			}
			else printf("%s\n", statbuf);
		}
		disconnect(fd2);
		disconnect(fd);
	}
}

/* 产生访存请求 */

int do_request_k()
{
	BYTE res=0;

	/* 请求类型 */
	ptr_memAccReq->pid=getpid();
	switch (ptr_memAccReq->reqType)
	{
		case 0: //读请求
		{
			printf("产生请求：\n地址：%u\t类型：读取\n", ptr_memAccReq->virAddr);
			break;
		}
		case 1: //写请求
		{
			printf("产生请求：\n地址：%u\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		}
		case 2:
		{
			printf("产生请求：\n地址：%u\t类型：执行\n", ptr_memAccReq->virAddr);
			break;
		}
		default:
			break;
	}
	if (connect(REQFIFO,'r',&fd)<0)
	{
		printf("Send request failed. \n");		
		return;
	}
	else {
		if (write(fd,&pid,4)<0||write(fd,ptr_memAccReq,sizeof(MemoryAccessRequest))<0)
		{
			disconnect(fd2);
			disconnect(fd);
			printf("Send data failed. \n");		
			return -1;			
		}
		else 
		{
			if (arrival(RESFIFO,&fd2,spid)<0||_STAT<0||_st=='f'||receive(&res,1,fd2)<0)
			{
				disconnect(fd2);
				disconnect(fd);
				printf("No permission. \n");		
				return -1;			
			}
		}		
		disconnect(fd2);
		disconnect(fd);
		return res;
	}	
}

int do_request(MemoryAccessRequestType mart, unsigned int vaddr, BYTE value)
{
	ptr_memAccReq->virAddr = vaddr;

	ptr_memAccReq->reqType = mart;

	ptr_memAccReq->value = value;

	return (do_request_k());
}

void do_rand(int num)
{
	int i,k=0;
	printf("create %d random reqs\n", num);
	ptr_memAccReq->pid=getpid();
	for (i=0;i<num;++i)
	{
		/* 随机产生请求地址 */
		ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
		/* 随机产生请求类型 */
		switch (random() %5 % 3)
		{
			case 0: //读请求
			{
				ptr_memAccReq->reqType = REQUEST_READ;
				break;
			}
			case 1: //写请求
			{
				ptr_memAccReq->reqType = REQUEST_WRITE;
				/* 随机产生待写入的值 */
				ptr_memAccReq->value = random() % 0xFFu;
				break;
			}
			case 2:
			{
				ptr_memAccReq->reqType = REQUEST_EXECUTE;
				break;
			}
			default:
				break;
		}
		//sleep(0.1);
		k=do_request_k();
		if (k>=0)
		{
			printf("return: %02X\n", (BYTE)k);
		}
	}
}

void do_reqs(cmd* command)
{
	int i,k=0;
	printf("create reqs in range(%u, %u)\n", command->value,command->value2);
	ptr_memAccReq->pid=getpid();
	for (i=command->value;i<=command->value2;++i)
	{
		/* 随机产生请求地址 */
		ptr_memAccReq->virAddr = i;
		/* 随机产生请求类型 */
		switch (command->cmdtype)
		{
			case READR: //读请求
			{
				ptr_memAccReq->reqType = REQUEST_READ;
				break;
			}
			// case 1: //写请求
			// {
			// 	ptr_memAccReq->reqType = REQUEST_WRITE;
			// 	/* 随机产生待写入的值 */
			// 	ptr_memAccReq->value = random() % 0xFFu;
			// 	break;
			// }
			// case 2:
			// {
			// 	ptr_memAccReq->reqType = REQUEST_EXECUTE;
			// 	break;
			// }
			default:
				break;
		}
		//sleep(0.1);
		k=do_request_k();
		if (k>=0)
		{
			printf("return: %02X\n", (BYTE)k);
		}
	}
}

void usage()
{
	printf("pid: %d\n",getpid());
	printf("request format: [rand num]|[read addr]|[write addr value]|[exec addr]\n");	
	printf("[help] to print this usage\n");	
	printf("[status] to print page data\n");	
	printf("[exit] to exit\n");	
}

int readcmd(cmd* t)
{
	unsigned int u,v;
	if (t==NULL) return 0;
	scanf("%s",statbuf);
	if (strcmp(statbuf,"exit")==0)
	{
		t->cmdtype=EXIT;
		return 1;
	}
	if (strcmp(statbuf,"help")==0)
	{
		t->cmdtype=USAGE;
		return 1;
	}
	if (strcmp(statbuf,"status")==0)
	{
		t->cmdtype=STAT;
		return 1;
	}
	if (strcmp(statbuf,"rand")==0)
	{
		scanf("%s",statbuf);
		u=atoi(statbuf);
		t->cmdtype=RAND;
		t->value=u;
		return 1;
	}
	if (strcmp(statbuf,"readr")==0)
	{
		scanf("%s",statbuf);
		u=atoi(statbuf);
		scanf("%s",statbuf);
		t->cmdtype=READR;
		t->value=u;
		t->value2=atoi(statbuf);
		return 1;
	}
	if (strcmp(statbuf,"read")==0)
	{
		scanf("%s",statbuf);
		u=atoi(statbuf);
		t->cmdtype=REQUEST;
		t->mart=REQUEST_READ;
		t->vaddr=u;
		return 1;
	}
	if (strcmp(statbuf,"write")==0)
	{
		scanf("%s",statbuf);
		u=atoi(statbuf);
		t->cmdtype=REQUEST;
		t->mart=REQUEST_WRITE;
		t->vaddr=u;
		scanf("%s",statbuf);
		v=atoi(statbuf);
		t->value=v;
		return 1;
	}
	if (strcmp(statbuf,"exec")==0)
	{
		scanf("%s",statbuf);
		u=atoi(statbuf);
		t->cmdtype=REQUEST;
		t->mart=REQUEST_EXECUTE;
		t->vaddr=u;
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	int res;
	printf("Initiailizing...\n");
	
	do_init();
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		if (!readcmd(command)) 
		{
			usage();
			continue;
		}
		switch (command->cmdtype)
		{
			case EXIT:
				do_exit();
				exit(0);
				break;
			case STAT:
				do_stat();
				break;
			case USAGE:
				usage();
				break;
			case RAND:
				do_rand(command->value);
				break;
			case READR:
				do_reqs(command);
				break;
			case REQUEST:
				res=do_request(command->mart,command->vaddr,command->value);
				if (res>=0) printf("return: %02X\n", (BYTE)res);
				else printf("req failed\n");
				break;
		}
		//sleep(5000);
	}
	return 0;
}
