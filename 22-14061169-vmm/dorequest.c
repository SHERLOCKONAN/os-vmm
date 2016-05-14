#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "vmm.h"


void error_info(char *s)//jsz add 错误信息反馈函数
{
    printf("发生错误，错误原因:%s\n",s);
}
void do_request(Ptr_MemoryAccessRequest ptr_memAccReq)//随机产生访存请求
{
	
	/* 随机产生请求地址 */
	ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
	//lyc add
	ptr_memAccReq->pid=random()%PROCESS_SUM;//模拟5个进程
	/* 随机产生请求类型 */
	switch (random() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("%u号进程产生请求：\n地址：%lu\t类型：读取\n", ptr_memAccReq->pid,ptr_memAccReq->virAddr);
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = random() % 0xFFu;
			printf("%u号进程产生请求：\n地址：%lu\t类型：写入\t值：%02X\n", ptr_memAccReq->pid, ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("%u号进程产生请求：\n地址：%lu\t类型：执行\n", ptr_memAccReq->pid, ptr_memAccReq->virAddr);
			break;
		}
		default:
			break;
	}
	//lyc ..
}
void writereq(Ptr_MemoryAccessRequest ptr_memAccReq)//将访存请求写入FIFO文件
{
	int fd;
    //struct stat statbuf;
	if((fd=open("/tmp/server",O_WRONLY))<0)//以只写方式打开FIFO文件，文件标识符为fd
        error_info("打开文件失败");
    if(write(fd,ptr_memAccReq,DATALEN)<0)
        error_info("写入失败");
    printf("写入成功\n");
    //写入完成
    close(fd);
}
/* 产生访存请求 */
int readrequest(Ptr_MemoryAccessRequest ptr_memAccReq)
{
	printf("请输入访存请求(输入x结束程序):\n");
	char c;
	unsigned long addr;
	BYTE value;
	int i,num;
	c=getchar();//读取请求类型
	//printf("#c的值是%c#\n",c);
	if(c!='s'&&c!='x'&&c!='y'){//如果c不是s或x则读取请求地址
		scanf(" %lu",&addr);//读取请求地址
		ptr_memAccReq->command=0;//不是命令	
	}
	switch(c){
		case 'r':
			ptr_memAccReq->reqType = REQUEST_READ;
			ptr_memAccReq->virAddr = addr;
			scanf(" %u",&(ptr_memAccReq->pid));
			printf("产生请求：\n地址：%lu\t类型：读取\tPID:%u\n", ptr_memAccReq->virAddr,ptr_memAccReq->pid);
            break;
        case 'w':
			ptr_memAccReq->reqType = REQUEST_WRITE;
			ptr_memAccReq->virAddr = addr;
            scanf(" %c",&value);//读入要写入的值
            ptr_memAccReq->value = value;
            scanf(" %u",&(ptr_memAccReq->pid));
            printf("产生请求：\n地址：%lu\t类型：写入\t值：%02X\tPID:%u\n", ptr_memAccReq->virAddr, ptr_memAccReq->value,ptr_memAccReq->pid);
            break;
        case 'e':
        	ptr_memAccReq->reqType = REQUEST_EXECUTE;
        	ptr_memAccReq->virAddr = addr;
        	scanf(" %u",&(ptr_memAccReq->pid));
            printf("产生请求：\n地址：%lu\t类型：执行\tPID:%u\n", ptr_memAccReq->virAddr,ptr_memAccReq->pid);
            break;
        case 's'://随机产生请求
        	ptr_memAccReq->command=0;//不是命令
        	scanf(" %d",&num);
        	printf("将产生%d条随机请求\n",num);
        	for(i=1;i<=num;i++){
        		do_request(ptr_memAccReq);
        		writereq(ptr_memAccReq);	
        	}
        	printf("%d条随机请求产生完毕\n",num);
        	break;
        case 'x':
        	ptr_memAccReq->command=-2;
        	printf("#发送退出程序命令#");
        	break;
        case 'y'://发送y命令
        	ptr_memAccReq->command=-1;
        	printf("#发送打印页表命令#");
        	break;
        default:
        	printf("请求格式有误，请重新输入\n");
    }
    if(c=='x'||c=='y'){//将命令写入fifo文件
		writereq(ptr_memAccReq);
	}
 	if(c=='r'||c=='w'||c=='e'){//将单条请求写入fifo文件
    	writereq(ptr_memAccReq);
    }
    if(c=='x')
    	return -1;//退出本程序
    while(c=getchar()!='\n')//越过行尾回车
    	;
    return 0;
    
}
int main() {
    Ptr_MemoryAccessRequest ptr_memAccReq;
    ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
    srandom(time(NULL));   
    
    //向FIFO文件中写入内容
    while(1){
   	 	if(readrequest(ptr_memAccReq)==-1)//从标准输入读入访存请求
   	 		break;
        //sleep(1);
    }
    
    printf("程序正常结束\n");
    return 0;
}
