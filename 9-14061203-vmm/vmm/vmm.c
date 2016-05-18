#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vmm.h"

/* 页表 */
//PageTableItem pageTable[PAGE_SUM];//PAGE_SUM
PageTableItem pageTable[PAGE_SUM1][PAGE_SUM2];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;
/* 管道 */
FILE *fd;

/* 初始化文件 */
void initFile()
{
	int i;
	char* key = "0123456789ABCDEFGHIGKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	char buffer[VIRTUAL_MEMORY_SIZE + 1];
	ptr_auxMem = fopen(AUXILIARY_MEMORY, "w+");
	//随机生成256位字符串
	for (i=0;i<VIRTUAL_MEMORY_SIZE-3;i++)
	{
		buffer[i] = key[rand() % 62];
	}
	buffer[VIRTUAL_MEMORY_SIZE - 3] = 'l';
	buffer[VIRTUAL_MEMORY_SIZE - 2] = 'a';
	buffer[VIRTUAL_MEMORY_SIZE - 1] = 'n';
	buffer[VIRTUAL_MEMORY_SIZE] = '\0';
	fwrite(buffer, sizeof(BYTE), VIRTUAL_MEMORY_SIZE, ptr_auxMem);
	printf("系统提示：初始化辅存模拟文件完成\n");
	fclose(ptr_auxMem);
}

/* 初始化环境 */
void do_init()
{
	int i, j, k;
	time_n = 0;
	srand(time(NULL));
	for (i = 0; i < PAGE_SUM1; i++)//PAGE_SUM
	{
		for (j = 0; j < PAGE_SUM2; j++)
		{
			pageTable[i][j].pageNum = j + i*PAGE_SUM1;
			pageTable[i][j].filled = FALSE;
			pageTable[i][j].edited = FALSE;
			pageTable[i][j].clock_time = 0;
			pageTable[i][j].R = 0;
			pageTable[i][j].count = 0;
			/* 使用随机数设置该页的保护类型 */
			switch (rand() % 7)
			{
				case 0:
				{
					pageTable[i][j].proType = READABLE;
					pageTable[i][j].proccessNum = 1;
					break;
				}
				case 1:
				{
					pageTable[i][j].proType = WRITABLE;
					pageTable[i][j].proccessNum = 1;
					break;
				}
				case 2:
				{
					pageTable[i][j].proType = EXECUTABLE;
					pageTable[i][j].proccessNum = 1;
					break;
				}
				case 3:
				{
					pageTable[i][j].proType = READABLE | WRITABLE;
					pageTable[i][j].proccessNum = 2;
					break;
				}
				case 4:
				{
					pageTable[i][j].proType = READABLE | EXECUTABLE;
					pageTable[i][j].proccessNum = 2;
					break;
				}
				case 5:
				{
					pageTable[i][j].proType = WRITABLE | EXECUTABLE;
					pageTable[i][j].proccessNum = 2;
					break;
				}
				case 6:
				{
					pageTable[i][j].proType = READABLE | WRITABLE | EXECUTABLE;
					pageTable[i][j].proccessNum = 3;
					break;
				}
				default:
					break;
			}
			/* 设置该页对应的辅存地址 */
			//pageTable[i][j].auxAddr = j * PAGE_SIZE * 2 + i * PAGE_SUM1 * PAGE_SUM2;
			pageTable[i][j].auxAddr = j * PAGE_SIZE + i * 32;
		}
	}
	int rand1, rand2;
	for (k = 0; k < BLOCK_SUM; k++)
	{
		/* 随机选择一些物理块进行页面装入 */
		/*if (rand() % 2 == 0)
		{
			do_page_in(&pageTable[k], k);
			pageTable[k].blockNum = k;
			pageTable[k].filled = TRUE;
			blockStatus[k] = TRUE;
		}
		else
			blockStatus[k] = FALSE;*/
		rand1 = rand() % 4;
		rand2 = rand() % 8;
		if (rand() % 2 == 0)
		{
			do_page_in(&pageTable[rand1][rand2], k);
			pageTable[rand1][rand2].blockNum = k;
			pageTable[rand1][rand2].filled = TRUE;
			blockStatus[k] = TRUE;
			Time[time_n++]=pageTable[rand1][rand2].pageNum;
		}
		else
			blockStatus[k] = FALSE;
	}
}

/* 产生访存请求 */
/*void do_request()
{
	unsigned long addr;
	unsigned int num;
    char cmdType;
	char writeValue;
	close(fd[0]);//关闭管道写端口
	//通过管段向父进程写数据    进程号-地址长度-地址-类型-输入次数-(要写入的值)
	//输入请求所属进程
	printf("请输入请求所属的进程(0~2)：\n");
	scanf("%u", &num);
	ptr_memAccReq->proccessNum = num;
	//写入管道
	char str_num[5];
	sprintf(str_num,"%u", ptr_memAccReq->proccessNum);
	write(fd[1], str_num, strlen(str_num));
	//手动输入请求地址
	printf("请输入请求地址：\n");
	scanf("%lu", &addr);
	ptr_memAccReq->virAddr = addr % VIRTUAL_MEMORY_SIZE;
	//写入管道
	char str_addr[5];
	sprintf(str_addr,"%lu",ptr_memAccReq->virAddr);
	int addrLength;
	char al[5];
	addrLength = strlen(str_addr);
	sprintf(al,"%d",addrLength);
	write(fd[1], al, strlen(al));
	write(fd[1], str_addr, strlen(str_addr));
	//手动输入请求类型
	printf("请输入请求类型(r/w/e)：\n");
	scanf(" %c", &cmdType);
	//写入管道
	char *str_type = &cmdType;
	write(fd[1], str_type, strlen(str_type));
	switch (cmdType)
	{
		case 'r': //读请求
		{
			char times[5] = "3";
			write(fd[1], times, strlen(times));
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("请求内容：\t地址：%lu\t类型：读取\n", ptr_memAccReq->virAddr);
			break;
		}
		case 'w': //写请求
		{
			char times[5] = "4";
			write(fd[1], times, strlen(times));
			ptr_memAccReq->reqType = REQUEST_WRITE;
			//手动输入待写入的值
			printf("请输入要写入的内容：\n");
			scanf(" %c", &writeValue); //只能取到第一个字符
			ptr_memAccReq->value = (int)writeValue % 0xFFu;
			//写入管道
			char *str_value = &ptr_memAccReq->value;
			write(fd[1], str_value, strlen(str_value));
			//ptr_memAccReq->value = rand() % 0xFFu;
			printf("请求内容：\t地址：%lu\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		}
		case 'e':
		{
			char times[5] = "3";
			write(fd[1], times, strlen(times));
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("请求内容：\t地址：%lu\t类型：执行\n", ptr_memAccReq->virAddr);
			break;
		}
		default:
			break;
	}
}*/

/* 响应请求 */
void do_response()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum, offAddr;
	unsigned int pageNum1, pageNum2;
	unsigned int actAddr;

	/* 检查地址是否越界 */
	//if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
    if (ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	/* 计算页号和页内偏移值 */
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	pageNum1 = ptr_memAccReq->virAddr / PAGE_SIZE / 8;
	pageNum2 = ptr_memAccReq->virAddr / PAGE_SIZE % 8;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("一级索引：%u\t二级索引：%u\t页号为：%u\t页内偏移为：%u\n", pageNum/8, pageNum%8, pageNum, offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = &pageTable[pageNum1][pageNum2];
	/*书上的R位与右移
	ptr_pageTabIt->R = 1;//此页被访问，R位值置1
	ptr_pageTabIt->clock_time = ptr_pageTabIt->clock_time >> 1;//计数器右移1位
	ptr_pageTabIt->clock_time = ptr_pageTabIt->clock_time | 0x80000000;//“插入”R值
	ptr_pageTabIt->R = 0;//计时器已增加，R位置0
	*/
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}

	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);
	
	/* 检查页面访问权限并处理访存请求 */
	if (ptr_pageTabIt->proccessNum != ptr_memAccReq->proccessNum)
	{
		printf("所属进程不同，无法进行访存操作\n");
		return;
	}
	else
	{
		switch (ptr_memAccReq->reqType)
		{
			case REQUEST_READ: //读请求
			{
				//clock():函数返回从“开启这个程序进程”到“程序中调用clock()函数”时之间的CPU时钟计时单元数
				ptr_pageTabIt->clock_time = clock();//每过1毫秒，调用clock()函数返回的值就加1
				ptr_pageTabIt->count++;
				if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
				{
					do_error(ERROR_READ_DENY);
					return;
				}
				/*if (ptr_pageTabIt->proccessNum != ptr_memAccReq->proccessNum)
				{
					printf("所属进程不同，无法读取\n");
					return;
				}*/
				/* 读取实存中的内容 */
				printf("读操作成功：值为%02X\n", actMem[actAddr]);
				break;
			}
			case REQUEST_WRITE: //写请求
			{
				ptr_pageTabIt->clock_time = clock();
				ptr_pageTabIt->count++;
				if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
				{
					do_error(ERROR_WRITE_DENY);
					return;
				}
				/*if (ptr_pageTabIt->proccessNum != ptr_memAccReq->proccessNum)
				{
					printf("所属进程不同，无法写入\n");
					return;
				}*/
				/* 向实存中写入请求的内容 */
				actMem[actAddr] = ptr_memAccReq->value;
				ptr_pageTabIt->edited = TRUE;
				printf("写操作成功\n");
				break;
			}
			case REQUEST_EXECUTE: //执行请求
			{
				ptr_pageTabIt->clock_time = clock();
				ptr_pageTabIt->count++;
				if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
				{
					do_error(ERROR_EXECUTE_DENY);
					return;
				}
				/*if (ptr_pageTabIt->proccessNum != ptr_memAccReq->proccessNum)
				{
					printf("所属进程不同，无法执行\n");
					return;
				}*/
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
			ptr_pageTabIt->count = 0;
			ptr_pageTabIt->clock_time = 0;

			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_LFU(ptr_pageTabIt);
	//do_FIFO(ptr_pageTabIt);
}

/* 根据LFU算法进行页面替换 */
void do_LFU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, j, min, page, page1, page2;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for (i = 0, min = 0xFFFFFFFF, page = 0; i < PAGE_SUM1; i++)//PAGE_SUM
	{
		for (j = 0; j < PAGE_SUM2; j++)
		{
			if (pageTable[i][j].count < min)
			{
				min = pageTable[i][j].count;
				page = j + i * PAGE_SUM1;
				page1 = i;
				page2 = j;
			}
		}
	}
	printf("选择第%u页进行替换\n", page);
	if (pageTable[i][j].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[i][j]);
	}
	pageTable[i][j].filled = FALSE;
	pageTable[i][j].count = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[i][j].blockNum);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[i][j].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
}

/* 根据FIFO算法进行页面替换 */
void do_FIFO(Ptr_PageTableItem ptr_pageTabIt)
{
    unsigned int firstcome;
    int i;
    firstcome=Time[0];			//Time[PAGE_TOTAL]里面存着现在的主存里的页面存储状况，每一项是页号，第一项是先进来的页面
    printf("没有空闲物理块，开始进行FIFO页面替换...\n ");
    printf("选择第%u页进行替换\n",firstcome);
    if(pageTable[firstcome/PAGE_SUM1][firstcome%PAGE_SUM2].edited)
    {
    	/* 页面内容有修改，需要写回至辅存 */
        printf("该页内容有修改，写回至辅存\n");
        do_page_out(&pageTable[firstcome/PAGE_SUM1][firstcome%PAGE_SUM2]);
    }
    pageTable[firstcome/PAGE_SUM1][firstcome%PAGE_SUM2].edited=FALSE;
    pageTable[firstcome/PAGE_SUM1][firstcome%PAGE_SUM2].count=0;
    pageTable[firstcome/PAGE_SUM1][firstcome%PAGE_SUM2].filled=FALSE;

	/* 读辅存内容，写入到实存 */
    do_page_in(ptr_pageTabIt,pageTable[firstcome/PAGE_SUM1][firstcome%PAGE_SUM2].blockNum);

	/* 更新页表内容 */
    ptr_pageTabIt->blockNum = pageTable[firstcome/PAGE_SUM1][firstcome%PAGE_SUM2].blockNum;
    ptr_pageTabIt->filled = TRUE;
    ptr_pageTabIt->edited = FALSE;
    ptr_pageTabIt->count = 0;
    printf("页面替换成功\n");
    for(i=0;i<time_n-1;++i)
    {
		Time[i]=Time[i+1];
	}
	Time[time_n]=ptr_pageTabIt->pageNum;
}

void do_before_LRU()
{
	unsigned int i, j, k;
	Ptr_PageTableItem ptr;
	//要得到物理块对应的虚页
	for (i = 0; i < BLOCK_SUM; i++)
	{
		if (blockStatus[i])
		{
			for (j = 0; j < PAGE_SUM1; j++)
			{
				for (k = 0; k < PAGE_SUM2; k++)
				{
					if (pageTable[i][j].blockNum == i)
					{
						pageTable[i][j].clock_time = 0;
					}
				}
			}
		}
	}
}

/* 根据LRU近似算法进行页面替换 */
void do_LRU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, j, pre_time, page;
	pre_time = 0xFFFFFFFF;
	page = 0;
	printf("没有空闲物理块，开始进行LRU页面替换...\n");
	for (i = 0; i < PAGE_SUM1; i++)
	{
		for(j=0;j<PAGE_SUM2;j++){
			if ((pageTable[i][j].clock_time < pre_time)  &&  pageTable[i][j].filled)
			{
				pre_time = pageTable[i][j].clock_time;
				page = i*PAGE_SUM1+j;
			}
		}
	}
	printf("选择第%u页进行替换\n",pageTable[page/PAGE_SUM1][page%PAGE_SUM2].pageNum);
	if (pageTable[page/PAGE_SUM1][page%PAGE_SUM2].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[page/PAGE_SUM1][page%PAGE_SUM2]);
	}
	pageTable[page/PAGE_SUM1][page%PAGE_SUM2].filled = FALSE;
	pageTable[page/PAGE_SUM1][page%PAGE_SUM2].count = 0;
	pageTable[page/PAGE_SUM1][page%PAGE_SUM2].clock_time = (clock_t)0;


	/* 读辅存内容，写入到实存*/
	do_page_in(ptr_pageTabIt, pageTable[page/PAGE_SUM1][page%PAGE_SUM2].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page/PAGE_SUM1][page%PAGE_SUM2].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	ptr_pageTabIt->clock_time = (clock_t)0;
	printf("页面替换成功\n");
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
	ptr_pageTabIt->R = 1;//此页被访问，R值置为1
	printf("调页成功：辅存地址%lu-->>物理块%u\n", ptr_pageTabIt->auxAddr, blockNum);
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
	printf("写回成功：物理块%lu-->>辅存地址%03X\n", ptr_pageTabIt->auxAddr, ptr_pageTabIt->blockNum);
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

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j;
	char str[4];
	printf("页号\t块号\t装入\t修改\t保护\t计数\t辅存\t所属进程\n");
	for (i = 0; i < PAGE_SUM1; i++)//PAGE_SUM
	{
		for (j = 0; j < PAGE_SUM2; j++)
		{
			printf("%u\t%u\t%u\t%u\t%s\t%lu\t%lu\t%u\n", (j+i*PAGE_SUM2), pageTable[i][j].blockNum, pageTable[i][j].filled,
				pageTable[i][j].edited, get_proType_str(str, pageTable[i][j].proType),
				pageTable[i][j].count, pageTable[i][j].auxAddr, pageTable[i][j].proccessNum);
		}
	}
}

/* 打印辅存内容 */
void do_print_vir()
{
    char c;
    FILE *fp = NULL;
    fp=fopen(AUXILIARY_MEMORY, "r");
    if(fp == NULL)
    {
        do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
    }
    while(fscanf(fp,"%c",&c) != EOF)
    {
        printf("%c",c);
    }
    fclose(fp);
    fp=NULL;
    printf("\n");
}

/* 打印实存内容 */
void do_print_act()
{
    unsigned int i;
    for (i = 0 ; i< ACTUAL_MEMORY_SIZE ; i++)
    {
        printf("%02X\t", actMem[i]);
    }
    printf("\n");
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
		str[2] = 'e';
	else
		str[2] = '-';
	str[3] = '\0';
	return str;
}

//int main(int argc, char* argv[])
int main()
{
	int read;
	char c;
	/* 初始化文件 */
	initFile();
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	/* 初始化环境 */
	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	
	umask(0);//设置允许当前进程创建文件或者目录最大可操作的权限
	mkfifo(FIFO,S_IFIFO|0666);
	if((fd = fopen(FIFO,"r"))<0)
	{
		printf("open fifo failed");
	}
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		signal(SIGALRM,do_before_LRU); //让内核做好准备，一旦接受到SIGALARM信号,就执行do_before_LRU
		alarm(1000);//闹钟函数 时间一到，时钟就发送一个信号SIGALRM到进程
		read = fread(ptr_memAccReq,sizeof(MemoryAccessRequest),1,fd);
		do_response();
		printf("按Y打印页表，按F打印辅存内容，按S打印实存内容，按其他键不打印...\n");
	    while ((c = getchar())!= '\n')
        {
            if (c == 'y' || c == 'Y'){
                do_print_info();
            }
            if (c == 'f' || c == 'F'){
                do_print_vir();
            }
            if (c == 's' || c == 'S'){
                do_print_act();
            }
        }
	    printf("按X退出程序，按其他键继续...\n");
	    if ((c = getchar()) == 'x' || c == 'X')
		    break;
	    while (c != '\n')
	    {
		    c = getchar();
		}
	    //sleep(5000);
	}

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}

