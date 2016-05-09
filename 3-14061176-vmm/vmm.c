#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vmm.h"
//#include <errno.h>
FILE *aux;
char s[4100];
/* 页表 */
PageTableItem fullpageTable[PROCESS_NUM][STAGE1_SIZE][STAGE2_SIZE];
PageTableItem (*pageTable)[8][8];
//PageTableItem fullpageTable;

/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

Ptr_PageTableItem actMemInfo[BLOCK_SUM];

int last_time;
/* 初始化环境 */
void do_init()
{
	int i, j, row, cul, t;
	srandom(time(NULL));
	for (t = 0; t < PROCESS_NUM; t++)
	{
		for (i = 0; i < STAGE1_SIZE; i++)
		{
			for(j = 0; j < STAGE2_SIZE; j++)
			{
				fullpageTable[t][i][j].pageNum = i*STAGE1_SIZE + j;
				fullpageTable[t][i][j].filled = FALSE;
				fullpageTable[t][i][j].edited = FALSE;
				fullpageTable[t][i][j].count = 0;
				/* 使用随机数设置该页的保护类型 */
				switch (random() % 7)
				{
					case 0:
					{
						fullpageTable[t][i][j].proType = READABLE;
						break;
					}
					case 1:
					{
						fullpageTable[t][i][j].proType = WRITABLE;
						break;
					}
					case 2:
					{
						fullpageTable[t][i][j].proType = EXECUTABLE;
						break;
					}
					case 3:
					{
						fullpageTable[t][i][j].proType = READABLE | WRITABLE;
						break;
					}
					case 4:
					{
						fullpageTable[t][i][j].proType = READABLE | EXECUTABLE;
						break;
					}
					case 5:
					{
						fullpageTable[t][i][j].proType = WRITABLE | EXECUTABLE;
						break;
					}
					case 6:
					{
						fullpageTable[t][i][j].proType = READABLE | WRITABLE | EXECUTABLE;
						break;
					}
					default:
						break;
				}
				/* 设置该页对应的辅存地址 */
				fullpageTable[t][i][j].auxAddr = (t*PAGE_SUM+i*STAGE1_SIZE + j) * PAGE_SIZE;

			}

		}
	}

	pageTable=fullpageTable;

	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		row = j/STAGE2_SIZE;
		cul = j%STAGE2_SIZE;
		if (random() % 2 == 0)
		{
			do_page_in(&pageTable[0][row][cul], j);
			pageTable[0][row][cul].blockNum = j;
			pageTable[0][row][cul].filled = TRUE;
			blockStatus[j] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
	do_request_init();
	last_time = 1;
}

/* 响应请求 */
//#ifndef ORZ

void do_switch() {
	printf("switching to process(pid=%d)\n",ptr_memAccReq->value);
	pageTable=&(fullpageTable[ptr_memAccReq->value]);
}

void do_response()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum, offAddr;
	unsigned int actAddr;

	if(ptr_memAccReq->reqType==REQUEST_SWITCH) {
		do_switch();
		return;
	}
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}

	/* 计算页号和页内偏移值 */
	//I'm drunk......我也是醉了.....
	/* pageTable[i].auxAddr = i * PAGE_SIZE * 2;so i = auxAddr or virAddr / (PAGE_SIZE * 2)*/
	pageNum = ptr_memAccReq->virAddr / (PAGE_SIZE);
	offAddr = ptr_memAccReq->virAddr % (PAGE_SIZE);
	printf("页号为：%u\t页内偏移为：%u\n", pageNum, offAddr);

	/* 获取对应页表项 */
#ifndef ORZ
	ptr_pageTabIt = &pageTable[pageNum];
#else
	ptr_pageTabIt = (*pageTable)[pageNum/STAGE1_SIZE]+pageNum%STAGE1_SIZE;
#endif

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
			ptr_pageTabIt->count = last_time ++;
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%02X\n", actMem[actAddr]);
			ptr_memAccReq->value=actMem[actAddr];
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count = last_time ++;
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
			ptr_pageTabIt->count = last_time ++;
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				return;
			}
			printf("执行成功\n");
			break;
		}
		case REQUEST_OUTPUT_MEM: do_print_actMem();
			break;
		case REQUEST_OUTPUT_PAGETABLE: do_print_info();
			break;
		case REQUEST_OUTPUT_AUXMEM : do_print_auxMem();
			break;
		default: //非法请求类型
		{
			do_error(ERROR_INVALID_REQUEST);
			return;
		}
	}
}
//#endif

/* 处理缺页中断 */
void do_page_fault(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i,target=-1;
	printf("产生缺页中断，开始进行调页...\n");
	for (i = 0; i < BLOCK_SUM; i++)
	{
		//be not used
		if (!blockStatus[i])
		{
			target=i; break;
			/* 读辅存内容，写入到实存 */
			do_page_in(ptr_pageTabIt, i);

			/* 更新页表内容 */
			ptr_pageTabIt->blockNum = i;
			ptr_pageTabIt->filled = TRUE;
			ptr_pageTabIt->edited = FALSE;
			ptr_pageTabIt->count = 0;

			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	if(!~target) target=do_LFU(ptr_pageTabIt);

	do_page_in(ptr_pageTabIt,target);
	ptr_pageTabIt->blockNum=target;
	ptr_pageTabIt->filled=TRUE;
	ptr_pageTabIt->edited=FALSE;
	ptr_pageTabIt->count=0;
	blockStatus[target]=TRUE;
	//actMemInfo[target]=ptr_pageTabIt;
}

/* 根据LFU算法进行页面替换 */
/*void*/unsigned int do_LFU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, min, page;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
//	for (i = 0, min = 0xFFFFFFFF, page = 0; i < PAGE_SUM; i++)
	for(i=0,min=-1,page=0;i<BLOCK_SUM;++i)
	{
		if (/*pageTable*/actMemInfo[i]->count < min)
		{
			min = actMemInfo[i]->count;
			page = i;
		}
	}
	printf("选择第%u页进行替换\n", page);
	if (/*pageTable*/actMemInfo[page]->edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(/*pageTable*/actMemInfo[page]);
	}
	/*pageTable[page]*/actMemInfo[page]->filled = FALSE;
	/*pageTable[page]*/actMemInfo[page]->count = 0;


	return page;
#if 0
	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, /*pageTable[page].blockNum*/page);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
#endif
}

/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
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
	actMemInfo[blockNum]=ptr_pageTabIt;
	printf("调页成功：辅存地址%u-->>物理块%u\n", ptr_pageTabIt->auxAddr, blockNum);
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
	printf("写回成功：物理块%u-->>辅存地址%03X\n", ptr_pageTabIt->auxAddr, ptr_pageTabIt->blockNum);
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
		default:
		{
			printf("未知错误：没有这个错误代码\n");
		}
	}
}

#ifndef ORZ
/* 产生访存请求 */
void do_request()
{
	/* 随机产生请求地址 */
	ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
	/* 随机产生请求类型 */
	switch (random() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n地址：%u\t类型：读取\n", ptr_memAccReq->virAddr);
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = random() % 0xFFu;
			printf("产生请求：\n地址：%u\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n地址：%u\t类型：执行\n", ptr_memAccReq->virAddr);
			break;
		}
		default:
			break;
	}
}
#endif

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];

	printf("页号\t块号\t装入\t修改\t保护\t时间\t辅存\n");
#ifndef ORZ
	for (i = 0; i < PAGE_SUM; i++)
#else
	for (i=0;i<STAGE1_SIZE;++i) for(j=0;j<STAGE2_SIZE;++j)
#endif
	{
		Ptr_PageTableItem cur;
#ifndef ORZ
		cur=pageTable+i;
#else
		cur=(*pageTable)[i]+j;
#endif
		printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\n", i, cur->blockNum, cur->filled,
			cur->edited, get_proType_str(str, cur->proType),
			cur->count, cur->auxAddr);
	}
}

/* 打印实存 */
void do_print_actMem()
{
	unsigned int i,j;
	printf("实存信息如下\n");
	for(i = 0; i < 4; i++)
		printf("地址:\t内容:\t");
	printf("\n");
	for(i = 0; i < 32; i++)
	{
		for(j = 0; j < 4; j++)
		{
			printf("%d\t%c\t",j*32+i,actMem[j*32+i]);
		}

		printf("\n");
	}
}



void do_print_auxMem(){
	unsigned int i,j;
	aux = fopen(AUXILIARY_MEMORY, "r");
	fscanf(aux, "%s", s);
	close(aux);
	printf("辅存信息如下\n");
	for(i = 0; i < 4; i++)
		printf("地址:\t内容:\t");
	printf("\n");
	for(i = 0; i < 1024; i++)
	{
		for(j = 0; j < 4; j++)
		{
			printf("%d\t%c\t",j*64+i,s[j*64+i]);
		}

		printf("\n");
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

void initFile(){
	int i;
	char* key = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	char buffer[VIRTUAL_MEMORY_SIZE + 1];
	//errno_t err;
	//err = fopen_s(&ptr_auxMem, AUXILIARY_MEMORY, "w+");
	//fopen_s是WINDOWS 下广泛用的，window下任务printf，fopen， scanf 等不安全，在后面加了个_S

	FILE *temp;
	temp = fopen(AUXILIARY_MEMORY, "w");
	for(i = 0; i<VIRTUAL_MEMORY_SIZE-3; i++)
	{
		buffer[i] = key[rand() % 62];
	}
	buffer[VIRTUAL_MEMORY_SIZE - 3] = 'h';
	buffer[VIRTUAL_MEMORY_SIZE - 2] = 'q';
	buffer[VIRTUAL_MEMORY_SIZE - 1] = 'y';
	buffer[VIRTUAL_MEMORY_SIZE] = '\0';

	//Randomly generated 256 - bit string
	for(i=0;i<PROCESS_NUM;++i)
		fwrite(buffer, sizeof(BYTE),VIRTUAL_MEMORY_SIZE, temp);

	printf("System prompt: Initialization of auxiliary memory simulation file has been completed ");
	fclose(temp);
}

int main(int argc, char* argv[])
{
	char c;
	int i;
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
//#ifndef ORZ
	initFile();
	puts("Exec");
//#endif
	do_init();
#ifndef ORZ
	do_print_info();
#endif
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		do_request();
		do_response();
		do_request_sendback();
#ifndef ORZ
		printf("按Y打印页表，按A打印实存,按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info();
		else if(c == 'a' || c == 'A')
			do_print_actMem();
		while (c != '\n')
			c = getchar();
		printf("按X退出程序，按其他键继续...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		while (c != '\n')
			c = getchar();
#endif
		//sleep(5000);
	}

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}

