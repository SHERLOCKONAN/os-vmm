
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "vmm.h"

#define number_process 4

/* 产生访存请求 */
void do_request()
{
    unsigned int proccessNum, i;
	unsigned long virAddr, virFirstLevelPage;
    char value;
    char string[100];
	FILE *fifo;
	/* 随机产生请求地址 */
	virFirstLevelPage = random() % number_process;
	virAddr = random() % VIRTUAL_MEMORY_SIZE;
	proccessNum = random() % number_process;
	/* 随机产生请求类型 */
    i = random() % 3;
	if((fifo = fopen("/tmp/fifo","w")) == NULL)
        printf("I'm so sorry,Open file error.\n");
	switch (i)
	{
		case 0: //读请求
		{
			value = '1';
			printf("产生请求：\n一级页号:%lu\t地址：%lu\t类型：读取\t进程号:%d\n", virFirstLevelPage, virAddr, proccessNum);
            fprintf(fifo,"%d\t%lu\t%c\t%d\t%lu",i, virAddr, value, proccessNum, virFirstLevelPage);
		   	break;
		}
		case 1: //写请求
		{
			value = '1';
			/* 随机产生待写入的值 */
			value = random() % 0xFFu;
			printf("产生请求：\n一级页号:%lu\t地址：%lu\t类型：写入\t值：%02X\t进程号:%d\n", virFirstLevelPage, virAddr, value, proccessNum);
            fprintf(fifo,"%d\t%lu\t%c\t%d\t%lu",i, virAddr, value, proccessNum, virFirstLevelPage);
            break;
		}
		case 2:
		{
			value = '1';
			printf("产生请求：\n一级页号:%lu\t地址：%lu\t类型：执行\t进程号:%d\n", virFirstLevelPage, virAddr, proccessNum);
            fprintf(fifo,"%d\t%lu\t%c\t%d\t%lu",i, virAddr, value, proccessNum, virFirstLevelPage);
			break;
		}
		default:
			break;
	}
	fclose(fifo);
}
/*手动产生访存请求*/
void do_request_manual()
{
    unsigned int proccessNum, i;
	unsigned long virAddr, virFirstLevelPage;
    char value;
    char string[100];
    FILE *fifo;
	printf("请输入你想访问的一级页表:\n");
    scanf("%lu",&virFirstLevelPage);
    printf("请输入请求地址:\n");
    scanf("%lu",&virAddr);
    printf("请输入进程号:\n");
    scanf("%d",&proccessNum);
    printf("请输入请求类型(0代表读取，1代表写入，2代表执行):\n");
    scanf("%u",&i);
	if((fifo = fopen("/tmp/fifo","w")) == NULL)
        printf("I'm so sorry,Open file error.\n");
    switch (i)
	{
		case 0: //读请求
		{
			value = '1';
			printf("产生请求：\n一级页号:%lu\t地址：%lu\t类型：读取\t进程号:%d\n", virFirstLevelPage, virAddr, proccessNum);
            fprintf(fifo,"%d\t%lu\t%c\t%d\t%lu",i, virAddr, value, proccessNum, virFirstLevelPage);
		   	break;
		}
		case 1: //写请求
		{
			value = '1';
			printf("请输入待写入的值:");
			scanf("%c",&value);
			while(value=='\n') 
				scanf("%c",&value);
			while(getchar()!='\n');
			printf("产生请求：\n一级页号:%lu\t地址：%lu\t类型：写入\t值：%02X\t进程号:%d\n", virFirstLevelPage, virAddr, value, proccessNum);
            fprintf(fifo,"%d\t%lu\t%c\t%d\t%lu",i, virAddr, value, proccessNum, virFirstLevelPage);
            break;
		}
		case 2:
		{
			value = '1';
			printf("产生请求：\n一级页号:%lu\t地址：%lu\t类型：执行\t进程号:%d\n", virFirstLevelPage, virAddr, proccessNum);
            fprintf(fifo,"%d\t%lu\t%c\t%d\t%lu",i, virAddr, value, proccessNum, virFirstLevelPage);
            break;
		}
		default:
			break;
	}
	fclose(fifo);
}

int main(){
	int i;
	while(TRUE){
		printf("随机生成请输入0，手动输入请输入1:\n");
       	scanf("%d",&i);
        if(i==0)
            do_request();
        else if(i==1)
            do_request_manual();
		else
			return 0;
	}
	return 0;
}
