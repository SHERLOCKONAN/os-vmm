#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "vmm.h"

Ptr_MemoryAccessRequest ptr_memAccReq;

/* 产生访存请求 */
void do_request()
{
	/* 随机产生请求地址 */
	unsigned fpagenum,npagenum,offAddr;

	fpagenum = rand() % 4;
	npagenum = rand() % 16;
	offAddr = rand() % 4;

	ptr_memAccReq->virAddr = fpagenum*FPage_SIZE+npagenum*NPage_SIZE+offAddr;
	ptr_memAccReq->ProcessNum=rand()%Process_SIZE;
	/* 随机产生请求类型 */
	switch (rand() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n地址：%u\t类型：读取\t进程号：%u\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum);
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = (rand() % 0x60u)+0x20u;
			printf("产生请求：\n地址：%u\t类型：写入\t值：%02X\t进程号：%u\n", ptr_memAccReq->virAddr, ptr_memAccReq->value,ptr_memAccReq->ProcessNum);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n地址：%u\t类型：执行\t进程号：%u\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum);
			break;
		}
		default:
			break;
	}
}

/***************************************手动输入请求*********************************************/
void in_request(){
	int i;
	/*产生地址*/
	fflush(stdin);
	printf("请输入请求地址：\n");
	scanf("%d",&i);
	if((i>=VIRTUAL_MEMORY_SIZE)||(i<0)){
		printf("地址超出范围。\n");
		fflush(stdin);
		return;
	}
	ptr_memAccReq->virAddr=i;
	/*请求进程*/
	printf("请输入请求进程号：（0-%d）\n",Process_SIZE-1);
	scanf("%d",&i);
	if(i>=0&&i<Process_SIZE)
    {
        ptr_memAccReq->ProcessNum=i;
    }
    else
    {
        printf("进程号不在范围内\n");
			return;
    }
	/*请求类型*/
	printf("请输入请求类型：（0为读请求，1为写请求，2为执行请求）\n");
	scanf("%d",&i);
	if(i==0){
		ptr_memAccReq->reqType = REQUEST_READ;
		printf("产生请求：\n地址：%u\t进程号：%d\t类型：读取\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum);

	}
	else if(i==1){
		ptr_memAccReq->reqType = REQUEST_WRITE;
		printf("请输入写入数据，有且仅有一个字符\n");
		char input;
		fflush(stdin);
		scanf("%c",&input);
		fflush(stdin);
		if((i<0x20u)||(i>=0x80u)){
			printf("写入数据错误\n");
			return;
		}
		ptr_memAccReq->value = input;
		printf("产生请求：\n地址：%u\t进程号：%d\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum, ptr_memAccReq->value);
	}
	else if(i==2){
		ptr_memAccReq->reqType = REQUEST_EXECUTE;
		printf("产生请求：\n地址：%u\t进程号：%d\t类型：执行\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum);
	}
	else{
		printf("请求类型错误！！！\n");
	}
	fflush(stdin);
}

int main(int argc,char *argv[]){
	FILE *fd = NULL;
	char c[100] = {'\0'};
	int i;
	struct stat statbuf;
	ptr_memAccReq = (Ptr_MemoryAccessRequest)malloc(sizeof(MemoryAccessRequest));
	if(stat("/tmp/mypipe",&statbuf)==0){
		/* 如果FIFO文件存在,删掉 */
		if(remove("/tmp/mypipe")<0){
			//printf("12121212121212\n");
			return 0;
		}
	}

	if(mkfifo("/tmp/mypipe",0666)<0){
		//printf("1313131313131313\n");
		return 0;
	}
	/* 在非阻塞模式下打开FIFO */
	if((fd=open("/tmp/mypipe",O_RDONLY|O_NONBLOCK))<0){
		//printf("141414141414141414\n");
		return 0;
	}
	while(TRUE){
		printf("按A切换至手动输入，按其他键自动生成...\n");
		fflush(stdin);
		scanf("%s", c);
			if(strcmp(c,"a\0") == 0 || strcmp(c,"A\0") == 0 ){
				in_request();
				if((fd = open("/tmp/mypipe",O_WRONLY))<0)
					return 0;

				if(write(fd,ptr_memAccReq,sizeof(MemoryAccessRequest))<0)
					return 0;

			close(fd);
			//return 0;
			}
			else{
				//printf("11111111\n");
				do_request();
				//printf("11111111\n");
				if((fd = open("/tmp/mypipe",O_WRONLY))<0){
				//	printf("11111111\n");

					return 0;
				}
				if((i = write(fd,ptr_memAccReq,sizeof(MemoryAccessRequest)))<0){
					//printf("11111111\n");
					return 0;
				}
				else{
					printf("write success");
				}
			}
			close(fd);
			//return 0;
	}
}
