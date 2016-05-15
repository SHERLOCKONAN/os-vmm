#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vmm.h"
/* 产生访存请求 */
int main()
{
	unsigned long addr;
	unsigned int num;
	char c;
    char cmdType;
	char writeValue;
	FILE *fifo;
	Ptr_MemoryAccessRequest ptr_memAccReq;

	while (TRUE)
	{
		//输入请求所属进程
		printf("请输入请求所属的进程(1~3)：\n");
		scanf("%u", &num);
		ptr_memAccReq->proccessNum = num;
		//输入请求地址
		printf("请输入请求地址：\n");
		scanf("%lu", &addr);
		ptr_memAccReq->virAddr = addr % VIRTUAL_MEMORY_SIZE;
		//输入请求类型
		printf("请输入请求类型(r/w/e)：\n");
		scanf(" %c", &cmdType);
		getchar();
		switch (cmdType)
		{
			case 'r': //读请求
			{
				ptr_memAccReq->reqType = REQUEST_READ;
				printf("请求内容：\t地址：%lu\t类型：读取\n", ptr_memAccReq->virAddr);
				break;
			}
			case 'w': //写请求
			{
				ptr_memAccReq->reqType = REQUEST_WRITE;
				//手动输入待写入的值
				printf("请输入要写入的内容：\n");
				scanf(" %c", &writeValue); //只能取到第一个字符
				getchar();
				ptr_memAccReq->value = (int)writeValue % 0xFFu;
				//ptr_memAccReq->value = rand() % 0xFFu;
				printf("请求内容：\t地址：%lu\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr, ptr_memAccReq->value);
				break;
			}
			case 'e':
			{
				ptr_memAccReq->reqType = REQUEST_EXECUTE;
				printf("请求内容：\t地址：%lu\t类型：执行\n", ptr_memAccReq->virAddr);
				break;
			}
			default:
				break;
		}
	
		fifo = fopen(FIFO,"r+");
		if(fifo < 0)
		{
			printf("open fifo failed");
		}
		if(fwrite(ptr_memAccReq,sizeof(MemoryAccessRequest),1,fifo) < 0)
		{
			printf("write failed");
		}
		fclose(fifo);
		printf("按X退出程序，按其他键继续...\n");
		c = getchar();
		if (c == 'x' || c == 'X')
			break;
		while (c != '\n')
		{
			c = getchar();
		}
	}
	return 0;
}


