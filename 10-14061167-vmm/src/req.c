#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "vmm.h"
MemoryAccessRequest memAccReq;
Ptr_MemoryAccessRequest ptr_memAccReq=&memAccReq;
int do_request()/////////////////////手动输入请求
{
	char i=0,c;
	char str[4];
	printf("按1手动输入请求，按其他随机产生请求...\n");
	scanf("%c",&i);
	scanf("%c",&c);
	while(c!='\n')
		c=getchar();
	if(i=='1')
	{
		printf("输入请求地址\n");
		scanf("%lu",&ptr_memAccReq->virAddr);
		printf("输入请求属于的哪个进程：\n");////
		scanf("%d",&ptr_memAccReq->proccessNum);///
		switch(ptr_memAccReq->proccessNum)
		{
			case 0:
				ptr_memAccReq->proccessNum=PRO_0;
				break;
			case 1:
				ptr_memAccReq->proccessNum=PRO_1;
				break;
			case 2:
				ptr_memAccReq->proccessNum=PRO_2;
				break;
			default: return -1;
		}
		printf("输入请求类型，0：读； 1：写； 2：执行\n");
		scanf("%c",&i);
		scanf("%c",&i);
		scanf("%c",&c);
		switch(i)
		{
			case '0': //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n地址：%lu\t类型：读取\t属于%s号进程\n", 
				ptr_memAccReq->virAddr,get_proNum_str(str,ptr_memAccReq->proccessNum));
			break;
		}
		case '1': //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			printf("输入写入的值\n");
			scanf("%c",&ptr_memAccReq->value);
			scanf("%c",&i);
			while(i!='\n')
				i=getchar();
			printf("产生请求：\n地址：%lu\t类型：写入\t值：%02X\t属于%s号进程\n",
				 ptr_memAccReq->virAddr, ptr_memAccReq->value,get_proNum_str(str,ptr_memAccReq->proccessNum));
			break;
		}
		case '2':
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n地址：%lu\t类型：执行\t属于%s号进程\n", 
				ptr_memAccReq->virAddr,get_proNum_str(str,ptr_memAccReq->proccessNum));
			break;
		}
		default:
			return -1;
		}
	}
	else
	{
		/* 随机产生请求地址 */
		ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
		/* 随机产生请求属于哪个进程 */
		switch(random()%3)
		{
			case 0:
				ptr_memAccReq->proccessNum=PRO_0;
				break;
			case 1:
				ptr_memAccReq->proccessNum=PRO_1;
				break;
			case 2:
				ptr_memAccReq->proccessNum=PRO_2;
				break;
			default: break;
		}
		/* 随机产生请求类型 */
		switch (random() % 3)
		{
			case 0: //读请求
			{
				ptr_memAccReq->reqType = REQUEST_READ;
				printf("产生请求：\n地址：%lu\t类型：读取\t属于%s号进程\n", 
					ptr_memAccReq->virAddr,get_proNum_str(str,ptr_memAccReq->proccessNum));
				break;
			}
			case 1: //写请求
			{
				ptr_memAccReq->reqType = REQUEST_WRITE;
				/* 随机产生待写入的值 */
				ptr_memAccReq->value = random() % 0xFFu;
				printf("产生请求：\n地址：%lu\t类型：写入\t值：%02X\t属于%s号进程\n", 
					ptr_memAccReq->virAddr, ptr_memAccReq->value,get_proNum_str(str,ptr_memAccReq->proccessNum));
				break;
			}
			case 2:
			{
				ptr_memAccReq->reqType = REQUEST_EXECUTE;
				printf("产生请求：\n地址：%lu\t类型：执行\t属于%s号进程\n", 
					ptr_memAccReq->virAddr,get_proNum_str(str,ptr_memAccReq->proccessNum));
				break;
			}
			default:
				break;
		}
	}	
	return 0;
}
char *get_proNum_str(char *str, int type)//获取页面所属进程号
{
	if (type & PRO_0)
		str[0] = '0';
	else
		str[0] = '-';
	if (type & PRO_1)
		str[1] = '1';
	else
		str[1] = '-';
	if (type & PRO_2)
		str[2] = '2';
	else
		str[2] = '-';
	str[3] = '\0';
	return str;
}
int main()
{
	int fd;
	if((fd=open("/tmp/vmm",O_WRONLY))<0)//打开管道
	{
		perror("open failed");
		exit(1);
	}
	while(1)
	{
		if(do_request()<0)
		{
			printf("invalid request!\n");
			continue;
		}
		else
		{
			if(write(fd,ptr_memAccReq,sizeof(MemoryAccessRequest))<0)//在request终端写管道
			{
				perror("write failed!");
				exit(1);
			}
		}
	}
	close(fd);
}
