#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "vmm.h"
#define number_process 4
//#define DEBUG
/* 页表 */
FirstLevelPageTable pageTable[number_process];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE * number_process];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

PageRecord process_page[number_process][PAGE_SUM * number_process];
int page_number_process[number_process] = {};
int LRU_NUM=0;
int block_auxAddr[BLOCK_SUM];

/* 初始化环境 */
void do_init()
{

	int i, j, k;
	srandom(time(NULL));
	for(k=0;k<32;k++)
	{
		block_auxAddr[k]=-1;
	}
	for(k = 0; k < number_process; k++){
		for (i = 0; i < PAGE_SUM; i++)
		{
			pageTable[k].SecondPageTable[i].pageNum = i;
			pageTable[k].SecondPageTable[i].filled = FALSE;
			pageTable[k].SecondPageTable[i].edited = FALSE;
			pageTable[k].SecondPageTable[i].count = LRU_NUM;
			/* 使用随机数设置该页的保护类型 */
			switch (random() % 7)
			{
				case 0:
				{
					pageTable[k].SecondPageTable[i].proType = READABLE;
					break;
				}
				case 1:
				{
					pageTable[k].SecondPageTable[i].proType = WRITABLE;
					break;
				}
				case 2:
				{
					pageTable[k].SecondPageTable[i].proType = EXECUTABLE;
					break;
				}
				case 3:
				{
					pageTable[k].SecondPageTable[i].proType = READABLE | WRITABLE;
					break;
				}
				case 4:
				{
					pageTable[k].SecondPageTable[i].proType = READABLE | EXECUTABLE;
					break;
				}
				case 5:
				{
					pageTable[k].SecondPageTable[i].proType = WRITABLE | EXECUTABLE;
					break;
				}
				case 6:
				{
					pageTable[k].SecondPageTable[i].proType = READABLE | WRITABLE | EXECUTABLE;
					break;
				}
				default:
					break;
			}
			/* 设置该页对应的辅存地址 */
			pageTable[k].SecondPageTable[i].auxAddr =k * PAGE_SUM * PAGE_SIZE +  i * PAGE_SIZE;
			int processNo = random() % number_process;
			pageTable[k].SecondPageTable[i].processNum = processNo;
			process_page[processNo][page_number_process[processNo]].FirstLevelNumber = k;
			process_page[processNo][page_number_process[processNo]++].SecondLevelNumber = i;
		}
	}
	int precord;
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (random() % 2 == 0)
		{
			precord = random() % number_process;
			do_page_in(&pageTable[precord].SecondPageTable[j%64], j);
			pageTable[precord].SecondPageTable[j%64].blockNum = j;
			pageTable[precord].SecondPageTable[j%64].filled = TRUE;
			blockStatus[j%64] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
}


/* 响应请求 */
void do_response()
{   int i,j=0;
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum, offAddr;
	unsigned int actAddr;

	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}

	/* 计算页号和页内偏移值 */
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("一级页表号为 ：%lu\t二级页号为：%u\t页内偏移为：%u\n",ptr_memAccReq->virFirstLevelPage, pageNum, offAddr);

    for(i=0;i<page_number_process[ptr_memAccReq->processNum];i++)
        {
            if(process_page[ptr_memAccReq->processNum][i].FirstLevelNumber==ptr_memAccReq->virFirstLevelPage 
				&& process_page[ptr_memAccReq->processNum][i].SecondLevelNumber==pageNum)
            {
                j++;
                break;
            }
        }
    if(j==0)
    {
        do_error(ERROR_PROCESS_NUMBER);
        return;
    }


	/* 获取对应页表项 */
	ptr_pageTabIt = &pageTable[ptr_memAccReq->virFirstLevelPage].SecondPageTable[pageNum];

	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}

	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);

	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count=++LRU_NUM;
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%c\n", actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count=++LRU_NUM;
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;
			printf("写操作成功\n");
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->count=++LRU_NUM;
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				return;
			}
			printf("执行成功\n");
			break;
		}
		default: //非法请求类型
		{
			do_error(ERROR_INVALID_REQUEST);
			return;
		}
	}
}

/* 处理缺页中断 */
void do_page_fault(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i;
	printf("产生缺页中断，开始进行调页...\n");
	for (i = 0; i < BLOCK_SUM; i++)
	{
		if (!blockStatus[i])
		{
			/* 读辅存内容，写入到实存 */
			do_page_in(ptr_pageTabIt, i);

			/* 更新页表内容 */
			ptr_pageTabIt->blockNum = i;
			ptr_pageTabIt->filled = TRUE;
			ptr_pageTabIt->edited = FALSE;
			ptr_pageTabIt->count = ++LRU_NUM;

			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_LRU(ptr_pageTabIt);
}

/* 根据LRU算法进行页面替换 */
void do_LRU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, min,FirstLevelPage, SecondLevelPage,j;
	printf("没有空闲物理块，开始进行LRU页面替换...\n");
	for (i = 0, min = 0xFFFFFFFF, FirstLevelPage = 0, SecondLevelPage = 0; i < number_process; i++)
	{
		for(j=0;j<PAGE_SUM;j++)
		{
			if (pageTable[i].SecondPageTable[j].count < min && pageTable[i].SecondPageTable[j].filled==TRUE)
			{
				min = pageTable[i].SecondPageTable[j].count;
				FirstLevelPage = i;
				SecondLevelPage = j;
			}
		}
	}
	printf("选择第一级%u页第二页%u页进行替换\n", FirstLevelPage,SecondLevelPage);
	if (pageTable[FirstLevelPage].SecondPageTable[SecondLevelPage].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[FirstLevelPage].SecondPageTable[SecondLevelPage]);
	}
	pageTable[FirstLevelPage].SecondPageTable[SecondLevelPage].filled = FALSE;
	pageTable[FirstLevelPage].SecondPageTable[SecondLevelPage].count = ++LRU_NUM;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[FirstLevelPage].SecondPageTable[SecondLevelPage].blockNum);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[FirstLevelPage].SecondPageTable[SecondLevelPage].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = LRU_NUM;
	printf("页面替换成功\n");
}

/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	int i;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(actMem + blockNum * PAGE_SIZE,
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	block_auxAddr[blockNum] = ptr_pageTabIt->auxAddr;
	printf("调页成功：辅存地址%lu-->>物理块%u 调入内容为\n", ptr_pageTabIt->auxAddr, blockNum);
	for(i=0;i<4;i++)
	{
        printf("%c",*(actMem + blockNum * PAGE_SIZE+i));
	}
	printf("\n");
}

/* 将被替换页面的内容写回辅存 */
void do_page_out(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int writeNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(actMem + ptr_pageTabIt->blockNum * PAGE_SIZE,
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: writeNum=%u\n", writeNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	block_auxAddr[ptr_pageTabIt->blockNum] = -1;
	printf("写回成功：物理块%d-->>辅存地址%lu\n", ptr_pageTabIt->blockNum, ptr_pageTabIt->auxAddr);
}

/* 错误处理 */
void do_error(ERROR_CODE code)
{
	switch (code)
	{
		case ERROR_READ_DENY:
		{
			printf("访存失败：该地址内容不可读\n");
			break;
		}
		case ERROR_WRITE_DENY:
		{
			printf("访存失败：该地址内容不可写\n");
			break;
		}
		case ERROR_EXECUTE_DENY:
		{
			printf("访存失败：该地址内容不可执行\n");
			break;
		}
		case ERROR_INVALID_REQUEST:
		{
			printf("访存失败：非法访存请求\n");
			break;
		}
		case ERROR_OVER_BOUNDARY:
		{
			printf("访存失败：地址越界\n");
			break;
		}
		case ERROR_FILE_OPEN_FAILED:
		{
			printf("系统错误：打开文件失败\n");
			break;
		}
		case ERROR_FILE_CLOSE_FAILED:
		{
			printf("系统错误：关闭文件失败\n");
			break;
		}
		case ERROR_FILE_SEEK_FAILED:
		{
			printf("系统错误：文件指针定位失败\n");
			break;
		}
		case ERROR_FILE_READ_FAILED:
		{
			printf("系统错误：读取文件失败\n");
			break;
		}
		case ERROR_FILE_WRITE_FAILED:
		{
			printf("系统错误：写入文件失败\n");
			break;
		}
		case ERROR_PROCESS_NUMBER:
		{
			printf("访存失败：该页面不能被指定进程访问\n");
			break;
		}
		default:
		{
			printf("未知错误：没有这个错误代码\n");
		}
	}
}


/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];
	printf("一级页号二级页号块号\t装入\t修改\t保护\t计数\t辅存\t进程号\n");
	for(j = 0; j < number_process; j++){
		for (i = 0; i < PAGE_SUM; i++)
		{
			printf("%u \t%u \t%u\t%u\t%u\t%s\t%lu\t%lu\t%u\n", j, i, pageTable[j].SecondPageTable[i].blockNum, pageTable[j].SecondPageTable[i].filled,
				pageTable[j].SecondPageTable[i].edited, get_proType_str(str, pageTable[j].SecondPageTable[i].proType),
				pageTable[j].SecondPageTable[i].count, pageTable[j].SecondPageTable[i].auxAddr,pageTable[j].SecondPageTable[i].processNum);
		}
	}
}

/* 获取页面保护类型字符串 */
char *get_proType_str(char *str, BYTE type)
{
	if (type & READABLE)
		str[0] = 'r';
	else
		str[0] = '-';
	if (type & WRITABLE)
		str[1] = 'w';
	else
		str[1] = '-';
	if (type & EXECUTABLE)
		str[2] = 'x';
	else
		str[2] = '-';
	str[3] = '\0';
	return str;
}

int main(int argc, char* argv[])
{
	char c;
	unsigned int i;
	int fifo, last_mtime, mtime, size, out;
	char string[10000]="\0";
	unsigned int proccessNum, a;
	unsigned long virAddr, virFirstLevelPage;
    char value;
	FILE *fd;
	struct stat statbuf;
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}

	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		char k;
        unsigned int i,j;
        if((fd = fopen("/tmp/fifo","w")) == NULL)
       		printf("I'm so sorry,Open file error.\n");
	fclose(fd);
	fifo=open("/tmp/fifo",O_RDONLY);
	fstat(fifo,&statbuf);
	last_mtime=statbuf.st_mtime;
	printf("等待请求中\n");
	while(TRUE){
		int num;
		FILE *ptr_temp;
		char temp[4];
		int ite;
		int readNum;
		fifo=open("/tmp/fifo",O_RDONLY);
		fstat(fifo,&statbuf);
		mtime=statbuf.st_mtime;
		size = statbuf.st_size;
		if(mtime!=last_mtime && size!=0){
			for(ite=0;ite<100;ite++)
			{
				string[ite]='\0';
			}
			out=read(fifo,string,100);
			//printf("%s",string);
			sscanf(string,"%d\t%lu\t%c\t%d\t%lu",&a, &virAddr, &value, &proccessNum, &virFirstLevelPage);
			if(a==0)
				ptr_memAccReq->reqType = REQUEST_READ;
			else if(a==1)
				ptr_memAccReq->reqType = REQUEST_WRITE;
			else
				ptr_memAccReq->reqType = REQUEST_EXECUTE;
			ptr_memAccReq->virAddr = virAddr;
			ptr_memAccReq->processNum = proccessNum;
			ptr_memAccReq->value = value;
			ptr_memAccReq->virFirstLevelPage = virFirstLevelPage;
			/*printf("%d\n",a);
			printf("%lu\n",ptr_memAccReq->virAddr);
			printf("%c\n",value);
			printf("%d\n",ptr_memAccReq->processNum);
			printf("%lu\n",ptr_memAccReq->virFirstLevelPage);*/
			do_response();
			printf("按A打印页表,按B打印实存,按C打印辅存,按D打印进程能够访问的页面号,按Y键退出程序,按其他键不打印\n");
			if ((c = getchar()) == 'a' || c == 'A')
				do_print_info();

			else if(c=='b'||c=='B')
			{
				int num1;
				int num2;
				printf("物理块号\t辅存地址\t一级页号\t二级页号\t内容\t\n");
				for(i=0;i<BLOCK_SUM;i++)
				{
					if(block_auxAddr[i]==-1)
						printf("%d\t\t----\t\t----\t\t----\t\t----\n",i);
					else
					{
						num1 = block_auxAddr[i]/(64*4);
						num2 = ((block_auxAddr[i]-num1*64*4)/4);
						printf("%d\t\t%d\t\t%d\t\t%d\t\t",i,block_auxAddr[i],num1,num2);
						for(j=0;j<4;j++)
						{
							printf("%c",actMem[j+i*4]);
						}
						printf("\n");
					}
				}
			}

			else if(c=='c'||c=='C')
			{
				printf("请输入你要访问的辅存地址\n");
				scanf("%d",&num);
				if (fseek(ptr_auxMem, num, SEEK_SET) < 0)
				{
				#ifdef DEBUG
					printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
				#endif
					do_error(ERROR_FILE_SEEK_FAILED);
					exit(1);
				}
				if ((readNum = fread(temp,sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
				{
				#ifdef DEBUG
					printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
					printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
					printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
				#endif
					do_error(ERROR_FILE_READ_FAILED);
					exit(1);
				}

				for(j=0;j<4;j++)
				{
					printf("%c",temp[j]);
				}
				printf("\n");
			}
			else if(c=='D'||c=='d')
			{
			    for(j=0;j<number_process;j++)
			    {
			        printf("%d号进程能够访问：\n",j);
			        for(i=0;i<page_number_process[j];i++)
			        {
			            printf("一级页表 : %d 二级页表 : %d\n",process_page[j][i].FirstLevelNumber , process_page[j][i].SecondLevelNumber);
			        }
			        printf("\n");
			    }
			}
			else if(c=='Y'||c=='y')
			{
				return 0;
			}
			else
			{
				;
			}
			printf("等待请求中\n");
			while (c != '\n')
				c = getchar();
		}
		last_mtime=statbuf.st_mtime;
		sleep(3);
	}
	}

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}
