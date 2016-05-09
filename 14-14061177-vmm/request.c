#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include "vmm.h"


int main(int argc,char *argv[]){
	int fd;
	char c;
	cmd reqcmd;
	reqcmd.RT=REQUEST;
	reqcmd.reqType = REQUEST_READ;
	reqcmd.virAddr = 103;
	if(argc==1)
	{
	/* 随机产生请求地址 */
	srandom(time(NULL));
	reqcmd.ProcessNum = random() % Process_SUM;
	reqcmd.virAddr =random() % 256+256*reqcmd.ProcessNum ;
	/* 随机产生请求类型 */
	switch (random() % 3)
	{
		case 0: //读请求
		{
			reqcmd.reqType = REQUEST_READ;
			printf("产生请求：\n进程号：%u\t地址：%u\t类型：读取\n", reqcmd.ProcessNum,reqcmd.virAddr);
			break;
		}
		case 1: //写请求
		{
			reqcmd.reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			reqcmd.value = random() % 0xFFu;
			printf("产生请求：\n进程号：%u\t地址：%u\t类型：写入\t值：%02X\n", reqcmd.ProcessNum,reqcmd.virAddr, reqcmd.value);
			break;
		}
		case 2:
		{
			reqcmd.reqType = REQUEST_EXECUTE;
			printf("产生请求：\n进程号：%u\t地址：%u\t类型：执行\n",reqcmd.ProcessNum, reqcmd.virAddr);
			break;
		}
		default:
			break;
	}
	}
	else{
	if(--argc>0 && (*++argv)[0]=='-')
	{
		if(c=*++argv[0])
			switch(c)
		{
			case 'r':
			reqcmd.reqType = REQUEST_READ;
			reqcmd.virAddr=atoi(*(++argv));
			reqcmd.ProcessNum=atoi(*(++argv));
			printf("产生请求：\n进程号：%u\t地址：%u\t类型：读取\n", reqcmd.ProcessNum,reqcmd.virAddr);
			argc--;
			break;
			case 'w':
			reqcmd.reqType = REQUEST_WRITE;
			reqcmd.virAddr=atoi(*(++argv));
			reqcmd.value=atoi(*(++argv));
			reqcmd.ProcessNum=atoi(*(++argv));
			printf("产生请求：\n进程号：%u\t地址：%u\t类型：写入\t值：%02X\n", reqcmd.ProcessNum,reqcmd.virAddr, reqcmd.value);
			argc--;
			break;
			case 'e':
			reqcmd.reqType = REQUEST_EXECUTE;
			reqcmd.virAddr=atoi(*(++argv));
			reqcmd.ProcessNum=atoi(*(++argv));
			printf("产生请求：\n进程号：%u\t地址：%u\t类型：执行\n",reqcmd.ProcessNum, reqcmd.virAddr);
			argc--;
			break;
			default:
				printf("Illegal option %c\n",c);
				return 1;
		}
	}
	}
	if((fd=open("/tmp/doreq",O_WRONLY))<0){
		printf("response open fifo failed\n");
		return;
	}
	if(write(fd,&reqcmd,CMDLEN)<0){
		printf("response write failed\n");
		return;
	}
	close(fd);
	printf("addr%ld\n",reqcmd.virAddr);
	return 0;		
}
