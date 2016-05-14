#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "vmm.h"

/* 页表 */
PageTableItem pageTable[PAGE_SUM];
//PageTableItem pageTable[PROCESS_SUM][PAGE_SUM/8][8]; 

Ptr_PageTableItem prt_pageTable_1[8];//一级页表
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

time_t timer,timerc;//

/* 初始化环境 */
void do_init()
{
	int i, j;
	srandom(time(NULL));
	for(i=0;i<8;i++)//
	{
		prt_pageTable_1[i]=&pageTable[8*i];
	}//将一级页表项指向二级页表的相应位置
	for (i = 0; i < PAGE_SUM; i++)
	{
		pageTable[i].pageNum = i;
		pageTable[i].filled = FALSE;
		pageTable[i].edited = FALSE;
		pageTable[i].count = 0;//LFU算法计数器
		pageTable[i].count_1 = 0;//页面老化算法计数器
		pageTable[i].count_LRU=0;//LRU算法计数器
		pageTable[i].R = 0;//访问位
		//随机设置某页属于哪些进程
		switch (random() % 7)
		{
			case 0:
			{
				pageTable[i].proccessNum = PRO_0;
				break;
			}
			case 1:
			{
				pageTable[i].proccessNum = PRO_1;
				break;
			}
			case 2:
			{
				pageTable[i].proccessNum = PRO_2;
				break;
			}
			case 3:
			{
				pageTable[i].proccessNum = PRO_0 | PRO_1;
				break;
			}
			case 4:
			{
				pageTable[i].proccessNum = PRO_0 | PRO_2;
				break;
			}
			case 5:
			{
				pageTable[i].proccessNum = PRO_1 | PRO_2;
				break;
			}
			case 6:
			{
				pageTable[i].proccessNum = PRO_0 | PRO_1 | PRO_2;
				break;
			}
			default:
				break;
		}
		/* 使用随机数设置该页的保护类型 */
		switch (random() % 7)
		{
			case 0:
			{
				pageTable[i].proType = READABLE;
				break;
			}
			case 1:
			{
				pageTable[i].proType = WRITABLE;
				break;
			}
			case 2:
			{
				pageTable[i].proType = EXECUTABLE;
				break;
			}
			case 3:
			{
				pageTable[i].proType = READABLE | WRITABLE;
				break;
			}
			case 4:
			{
				pageTable[i].proType = READABLE | EXECUTABLE;
				break;
			}
			case 5:
			{
				pageTable[i].proType = WRITABLE | EXECUTABLE;
				break;
			}
			case 6:
			{
				pageTable[i].proType = READABLE | WRITABLE | EXECUTABLE;
				break;
			}
			default:
				break;
		}
		/* 设置该页对应的辅存地址 */
		pageTable[i].auxAddr = i * PAGE_SIZE ;//12:15
	}
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (random() % 2 == 0)
		{
			do_page_in(&pageTable[j], j);
			pageTable[j].blockNum = j;
			pageTable[j].filled = TRUE;
			blockStatus[j] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
}


/* 响应请求 */
void do_response()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum, offAddr;
	unsigned int pageNum_1,pageNum_2;
	Ptr_PageTableItem ptr_pageTabIt_1,ptr_pageTabIt_2;
	unsigned int actAddr;
	int i=0;
	char s[4],str[4];
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* 计算页号和页内偏移值 */
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	pageNum_1=pageNum/8;//一级页表索引
	pageNum_2=pageNum%8;//二级页表索引
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("一级页表：%u  二级页表：%u 页内偏移为：%u\n",pageNum_1,pageNum_2,offAddr);
	printf("页号为：%u\t页内偏移为：%u\n", pageNum, offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = &pageTable[pageNum];
	ptr_pageTabIt_1=prt_pageTable_1[pageNum_1];//一级页表
	ptr_pageTabIt=&ptr_pageTabIt_1[pageNum_2];//二级页表
	/*if(ptr_pageTabIt==ptr_pageTabIt_2)
		printf("%d %d  yes\n",ptr_pageTabIt,ptr_pageTabIt_2);
	else
		printf("%d %d  no\n",ptr_pageTabIt,ptr_pageTabIt_2);*/
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}
	
	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);
	if(!(ptr_memAccReq->proccessNum&ptr_pageTabIt->proccessNum))//判断请求能否访问该页面
	{
		printf("该页属于进程：%s 进程%s不能访问\n",
				get_proNum_str(s,ptr_pageTabIt->proccessNum),get_proNum_str(str,ptr_memAccReq->proccessNum));
		return;
	}
	for(i = 0; i < PAGE_SUM; i++)
	{
		if(pageTable[i].filled)
		{
			pageTable[i].count_LRU++;
		}
	}
	ptr_pageTabIt->count_LRU=0;//更新LRU计数
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->R=1;
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
			ptr_pageTabIt->count++;
			ptr_pageTabIt->R=1;
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
			ptr_pageTabIt->count++;
			ptr_pageTabIt->R=1;
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
			ptr_pageTabIt->count = 0;
			ptr_pageTabIt->count_1 = 0;
			ptr_pageTabIt->count_LRU=0;
			ptr_pageTabIt->R = 1;
			
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	//do_LFU(ptr_pageTabIt);
	do_OldPage(ptr_pageTabIt);
	//do_LRU(ptr_pageTabIt);
}

/* 根据LFU算法进行页面替换 */
void do_LFU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, min, page;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for (i = 0, min = 0xFFFFFFFF, page = 0; i < PAGE_SUM; i++)
	{
		if (pageTable[i].count < min)
		{
			min = pageTable[i].count;
			page = i;
		}
	}
	printf("选择第%u页进行替换\n", page);
	if (pageTable[page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].filled = FALSE;
	pageTable[page].count = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[page].blockNum);///   ?
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;//  
	printf("页面替换成功\n");
}
/////jm
void do_OldPage(Ptr_PageTableItem ptr_pageTabIt)//页面老化算法
{
	unsigned int i, min, page;
	printf("没有空闲物理块，开始进行页面老化替换...\n");
	for (i = 0, min = 0xFFFFFFFF, page = 0; i < PAGE_SUM; i++)
	{
		if (pageTable[i].filled&&pageTable[i].count_1<min)//修改源码中的bug
		{
			min = pageTable[i].count_1;
			page = i;
		}//选择计数值最小的页面替换
	}
	printf("选择第%u页进行替换\n", page);
	if (pageTable[page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].filled = FALSE;
	pageTable[page].count = 0;
	pageTable[page].count_1=0;
	pageTable[page].R=0;
	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[page].blockNum);
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	ptr_pageTabIt->count_1=0;
	ptr_pageTabIt->R=1;
	printf("页面替换成功\n");
}
void do_LRU(Ptr_PageTableItem ptr_pageTabIt)//LRU算法
{
	unsigned int i, page;
	int max;
	printf("没有空闲物理块，开始进行LRU页面替换...\n");
	for (i = 0, max=-1, page = 0; i < PAGE_SUM; i++)
	{
		if (pageTable[i].filled&&pageTable[i].count_LRU > max)
		{
			max= pageTable[i].count_LRU;
			page = i;
		}//选择计数值最大的页面替换
	}
	printf("选择第%u页进行替换\n", page);
	if (pageTable[page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].filled = FALSE;
	pageTable[page].count = 0;
	pageTable[page].count_1=0;
	pageTable[page].count_LRU=0;
	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[page].blockNum);//
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count_LRU= 0;//  
	printf("页面替换成功\n");
}
///////
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
//#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%lu\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
//#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
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
	printf("写回成功：物理块%u-->>辅存地址%lu\n", ptr_pageTabIt->blockNum, ptr_pageTabIt->auxAddr);//%03x
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
	unsigned int i, j, k;
	char str[4],str2[4];
	printf("页号\t块号\t装入\t修改\t保护\t计数1\t\t计数2\t计数3\t辅存\t所属进程\n");
	for (i = 0; i < PAGE_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%s\t%lu\t%10u\t%d\t%lu\t%s\n", i, pageTable[i].blockNum, pageTable[i].filled, 
			pageTable[i].edited, get_proType_str(str, pageTable[i].proType), pageTable[i].count, 
			pageTable[i].count_1,pageTable[i].count_LRU,pageTable[i].auxAddr,get_proNum_str(str2,pageTable[i].proccessNum));
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
////jm
void do_print_actMem()//打印实存内容
{
	int i=0;
	printf("实存内容为：\n");
	for(int i=0;i<ACTUAL_MEMORY_SIZE;i++)
	{
		/*if(actMem[i]=='\0')
			printf(" ");
		else
		if(actMem[i]=='\n')
			printf("\\n");
		else
		    printf("%c",actMem[i]);
		//putchar(actMem[i]);*/
		if(i%4==0)
		printf("\n");
		printf("%c ",actMem[i]);
	}
	printf("\n");
}
void do_print_auxMem()//打印辅存内容
{
	char ch;
	FILE *fp;
	printf("辅存内容为：\n");
	fp=fopen(AUXILIARY_MEMORY,"r");
	ch=fgetc(fp);
	while(ch!=EOF)
	{
		printf("%02x ",ch);//putchar(ch);
		ch=fgetc(fp);
	}
	printf("\n");
	fclose(fp);
}
void initFile()
{
	int i;
	char* key="0123456789aBCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	char buffer[3*VIRTUAL_MEMORY_SIZE+1];
	//errno_t err;
	//err=fopen_s(&ptr_auxMem,AUXILIARY_MEMORY,"w+");
	int  err;
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "w+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	for(i=0;i<3*VIRTUAL_MEMORY_SIZE-3;i++)
	{
	    buffer[i]=key[rand()%62];
	    if(i%100==0)
		buffer[i]='\n';
	}
	buffer[3*VIRTUAL_MEMORY_SIZE-3]='j';
	buffer[3*VIRTUAL_MEMORY_SIZE-2]='j';
	buffer[3*VIRTUAL_MEMORY_SIZE-1]='m';
	buffer[3*VIRTUAL_MEMORY_SIZE]='\0';
	fwrite(buffer,sizeof(BYTE),3*VIRTUAL_MEMORY_SIZE,ptr_auxMem);
	printf("系统提示：初始化辅存模拟文件完成\n");
	fclose(ptr_auxMem);
}
////
int main(int argc, char* argv[])
{
	char c;
	int i;
	time(&timer);//系统开始的时间
	initFile();
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	
	do_init();
	do_print_info();

	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
///////////////////////////在main中创建管道
	struct stat statbuf;
	int fifo;
	if(stat("/tmp/vmm",&statbuf)==0)
	{
		/* 如果FIFO文件存在,删掉 */
		if(remove("/tmp/vmm")<0)
		{
			perror("remove failed");
			exit(1);
		}
	}

	if(mkfifo("/tmp/vmm",0666)<0)
	{
		perror("mkfifo failed");
		exit(1);
	}
	if((fifo=open("/tmp/vmm",O_RDONLY))<0)
	{
		perror("open failed");
		exit(1);
	}
	
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		//do_request();
		if((read(fifo,ptr_memAccReq,sizeof(MemoryAccessRequest)))<0)
		{
			perror("read failed");
			exit(1);
		}//读管道内容
		do_response();
		/////////
		time(&timerc);
		if((timerc-timer)>=Time)//经过一定时间，更新页面老化算法的计数器
		{
			for(i = 0; i < PAGE_SUM; i++)
			{
				pageTable[i].count_1=(pageTable[i].count_1>>1)|(pageTable[i].R<<31);
				pageTable[i].R=0;
			}
			timer=timerc;
		}
		printf("按1打印页表，按2打印页表和实存内容，按3打印页表、辅存和实存内容，按其他键不打印...\n");
		if ((c = getchar()) == '1')
			do_print_info();
		if (c== '2')
		{
			do_print_info();
			do_print_actMem();
		}
		if (c== '3')
		{
			do_print_info();
			do_print_actMem();
			do_print_auxMem();
		}
		while (c != '\n')
			c = getchar();
		printf("按X退出程序，按其他键继续...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		while (c != '\n')
			c = getchar();
		//sleep(5000);
	}

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	close(fifo);
	return (0);
}
