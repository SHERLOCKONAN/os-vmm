#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vmm.h"
//jsz add
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
//...
/* 页表 */
PageTableItem pageTable[PROCESS_SUM][PAGE_SUM/SECOND_PAGE_SUM][SECOND_PAGE_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;
//jsz add
int fifo;//管道描述符
//...


/* 初始化环境 */
void do_init()
{
	//lyc modified
	int i, j;
	int p;
	srandom(time(NULL));
	for(p=0;p<PROCESS_SUM;p++)
	{
		for (i = 0; i < PAGE_SUM; i++)
		{
			pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].pageNum = i;
			pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].filled = FALSE;
			pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].edited = FALSE;
			pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].count = 0;
			//lyc add
			pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].pid=p;
			//lyc ..
			/* 使用随机数设置该页的保护类型 */
			switch (random() % 7)
			{
				case 0:
				{
					pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType = READABLE;
					break;
				}
				case 1:
				{
					pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType = WRITABLE;
					break;
				}
				case 2:
				{
					pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType = EXECUTABLE;
					break;
				}
				case 3:
				{
					pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType = READABLE | WRITABLE;
					break;
				}
				case 4:
				{
					pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType = READABLE | EXECUTABLE;
					break;
				}
				case 5:
				{
					pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType = WRITABLE | EXECUTABLE;
					break;
				}
				case 6:
				{
					pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType = READABLE | WRITABLE | EXECUTABLE;
					break;
				}
				default:
					break;
			}
			/* 设置该页对应的辅存地址 */
			pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].auxAddr = i * PAGE_SIZE * 2;
		}
	}
	
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (random() % 2 == 0)
		{
			p=random()%PROCESS_SUM;
			do_page_in(&pageTable[p][j/SECOND_PAGE_SUM][j%SECOND_PAGE_SUM], j);
			pageTable[p][j/SECOND_PAGE_SUM][j%SECOND_PAGE_SUM].blockNum = j;
			pageTable[p][j/SECOND_PAGE_SUM][j%SECOND_PAGE_SUM].filled = TRUE;
			blockStatus[j] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
	//lyc ..
}


/* 响应请求 */
void do_response()
{
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
	printf("一级页号为：%u\t二级页号为：%u\t页内偏移为：%u\n", pageNum/SECOND_PAGE_SUM, pageNum%SECOND_PAGE_SUM, offAddr);
	//lyc add
	/* 获取对应请求进程id号 */
	unsigned int pid=ptr_memAccReq->pid;
	//lyc ..
	//lyc modified
	/* 获取对应页表项 */
	ptr_pageTabIt = &pageTable[pid][pageNum/SECOND_PAGE_SUM][pageNum%SECOND_PAGE_SUM];
	
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		/* 先确定能否共享 */
		int flag=0;
		if(ptr_memAccReq->reqType==REQUEST_READ||ptr_memAccReq->reqType==REQUEST_EXECUTE)
		{
			for(int p=0;p<PROCESS_SUM;p++)
			{
				if(p!=pid)
				{
					for(int i=0;i<PAGE_SUM;i++)
					{//注意共享的完备条件：实地址相等，页面存在在实存，有相同的访存属性
						if(pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].auxAddr==ptr_pageTabIt->auxAddr&&pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].filled==1
							&&(((pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType&READABLE)&&ptr_memAccReq->reqType==REQUEST_READ&&(ptr_pageTabIt->proType&READABLE))
								||((pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType&EXECUTABLE)&&ptr_memAccReq->reqType==REQUEST_EXECUTE&&(ptr_pageTabIt->proType&EXECUTABLE))))
						{
							flag=1;
							ptr_pageTabIt=&pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM];
							break;
						}
					}
				}
				if(flag==1)
					break;
			}
		}
		if(flag==0)
		{
			do_page_fault(ptr_pageTabIt);
		}
		//lyc ..
	}
	
	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);
	
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->visitTime=clock();//jsz add
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%02X\n", actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->visitTime=clock();//jsz add
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
			ptr_pageTabIt->visitTime=clock();//jsz add
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
			
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_LRU(ptr_pageTabIt);//jsz change LFU->LRU
}

/* 根据LFU算法进行页面替换 */
void do_LFU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, min, page;
	//lyc add
	unsigned int p,chooseP;
	page = 0;
	min = 0xFFFFFFFF;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for(p=0;p<PROCESS_SUM;p++)
	{
		for (i = 0; i < PAGE_SUM; i++)
		{
			if (pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].count < min&&pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].filled==1)//lyc modified
			{
				min = pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].count;
				page = i;
				chooseP=p;
			}
		}
	}
	
	printf("选择%u号进程第%u页进行替换\n", chooseP,page);
	
	if (pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM]);
	}
	
	pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].filled = FALSE;
	pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].count = 0;
	printf("LFU TEST output:%d %d %d\n",chooseP,page,pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].filled);

	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
	//lyc ..
}
//jsz add
/*根据LRU算法进行页面替换*/

void do_LRU(Ptr_PageTableItem ptr_pageTabIt)
{
    unsigned int i, page;
    long min;
    unsigned int p,chooseP;
	page = 0;
	min = MAXLONG;
    printf("没有空闲物理块，开始进行LRU页面替换...\n");
    for(p=0;p<PROCESS_SUM;p++)
	{
		for (i = 0; i < PAGE_SUM; i++)
		{
			if (pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].visitTime < min&&pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].filled==1)//lyc modified
			{
				min = pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].visitTime;
				page = i;
				chooseP=p;
			}
		}
	}
    printf("选择%u号进程第%u页进行替换\n", chooseP,page);
	
	if (pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM]);
	}
	
	pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].filled = FALSE;
	pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].visitTime=clock();
	//printf("LRU TEST output:%d %d %d\n",chooseP,page,pageTable[chooseP][page].filled);

	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[chooseP][page/SECOND_PAGE_SUM][page%SECOND_PAGE_SUM].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	ptr_pageTabIt->visitTime=clock();
	printf("页面替换成功\n");
	//lyc ..
}
//...
/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(actMem + blockNum * PAGE_SIZE, 
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	printf("%u号进程调页成功：辅存地址%lu-->>物理块%u\n", ptr_pageTabIt->pid ,ptr_pageTabIt->auxAddr, blockNum);
}

/* 将被替换页面的内容写回辅存 */
void do_page_out(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int writeNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%u\n", ptr_pageTabIt, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(actMem + ptr_pageTabIt->blockNum * PAGE_SIZE, 
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
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

/* 产生访存请求 */
void do_request()
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

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];
	//lyc modified
	printf("一页\t二页\t块号\t装入\t修改\t保护\t计数\t辅存\t访时\t进程\n");
	unsigned int p;
	for(p=0;p<PROCESS_SUM;p++)
	{
		for (i = 0; i < PAGE_SUM; i++)
		{
			printf("%u\t%u\t%u\t%u\t%u\t%s\t%lu\t%lu\t%ld\t%u\n", i/SECOND_PAGE_SUM,i%SECOND_PAGE_SUM, pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].blockNum, pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].filled, 
				pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].edited, get_proType_str(str, pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].proType), 
				pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].count, pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].auxAddr,pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].visitTime,
				pageTable[p][i/SECOND_PAGE_SUM][i%SECOND_PAGE_SUM].pid);
		}
	}
	//lyc ..
}
//lyc add
void do_print_auxMem()
{
	unsigned char temp;
	for(int i=0;i<512;i++){
		fread(&temp,1,1,ptr_auxMem);
		printf("%u ",temp);
		if(i%16==15)
			printf("\n");
	}
}
void do_print_block_info()
{
	printf("块号\t装入\t修改\t保护\t计数\t辅存\t进程\t一页\t二页\t访时\n");
	char str[4];
	int flag=0;
	for(int i=0;i<BLOCK_SUM;i++)
	{
		for(int j=0;j<PROCESS_SUM;j++)
		{
			for(int k=0;k<PAGE_SUM;k++)
			{
				if(pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].blockNum==i&&pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].filled==1)//对应块已装入
				{
					printf("%u\t%u\t%u\t%s\t%lu\t%lu\t%u\t%u\t%u\t%ld\n",pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].blockNum, pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].filled, 
						pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].edited, get_proType_str(str, pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].proType), 
						pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].count, pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].auxAddr,
						pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].pid,k/SECOND_PAGE_SUM,k%SECOND_PAGE_SUM,pageTable[j][k/SECOND_PAGE_SUM][k%SECOND_PAGE_SUM].visitTime);
				}
			}
		}
	}
	do_print_auxMem();
}
//lyc ..

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
void error_info(char *s)//jsz add 错误信息反馈函数
{
    printf("发生错误，错误原因:%s\n",s);
}
void my_init()//jsz add 管道初始化函数
{
	struct stat statbuf;
    if(stat("/tmp/server",&statbuf)==0){//如果FIFO文件已存在，则移除
        if(remove("/tmp/server")<0)
            error_info("文件移除失败");
    }
    if(mkfifo("/tmp/server", 0666)<0)//创建FIFO文件
        error_info("mkfifo失败");
    //if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)//以非阻塞形式打开
    if((fifo=open("/tmp/server",O_RDONLY))<0)//以阻塞方式打开
		error_info("fifo文件打开失败");
}
int read_request()//jsz add 从管道中读取请求
{
	int count;
	if((count=read(fifo,ptr_memAccReq,DATALEN))<0)//count为读到的字节数
		error_info("读取fifo文件失败");
	if(count==0)//如果没有读到请求则不显示
		return 0;
	//判断是否是命令
	if(ptr_memAccReq->command<0){
		return ptr_memAccReq->command;
	}
	//不是命令则创建请求
	printf("------------------------------------------\n");
	switch (ptr_memAccReq->reqType)//显示请求类型
    {
        case REQUEST_READ: //读请求
        {
            printf("获取%u号进程请求：\n地址：%lu\t类型：读取\n", ptr_memAccReq->pid,ptr_memAccReq->virAddr);
            break;
        }
        case REQUEST_WRITE: //写请求
        {
            printf("获取%u号进程请求：\n地址：%lu\t类型：写入\t值：%02X\n",ptr_memAccReq->pid, ptr_memAccReq->virAddr, ptr_memAccReq->value);
            break;
        }
        case REQUEST_EXECUTE:
        {
            printf("获取%u号进程请求：\n地址：%lu\t类型：执行\n",ptr_memAccReq->pid, ptr_memAccReq->virAddr);
            break;
        }
        default:
            break;
    }
    return count;//返回读到的个数
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
	
	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	my_init();//管道初始化
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		//do_request();
		int returnvalue;
		returnvalue=read_request();//从管道中读取请求
        if(returnvalue>0){
        	do_response();
       	}
        else if(returnvalue==0){
        	//printf("#jsz note#没有读取到新请求\n");
        }
        else if(returnvalue==-1){//y命令
			printf("#打印页表#\n");
			do_print_block_info();
		}
		else if(returnvalue==-2){//x命令
			printf("#退出程序#\n");
			break;
		}
        /*
		printf("按Y打印页表，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_block_info();
			//do_print_info();
		while (c != '\n')
			c = getchar();
		printf("按X退出程序，按其他键继续...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		while (c != '\n')
			c = getchar();
		*/
		
		//sleep(1);
	}

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}
