#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "vmm.h"

/* 一级页表 */
fPageTableItem fpageTable[FPAGE_SUM];
/* 二级页表 */
PageTableItem pageTable[PAGE_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;



/* 初始化二级页表 */
void do_initpage(int pageblocknum){
	int i;
	for (i = pageblocknum * FPAGE_SIZE; i < (pageblocknum + 1) * FPAGE_SIZE; i++){
		pageTable[i].pageNum = i;
		pageTable[i].filled = FALSE;
		pageTable[i].edited = FALSE;
		pageTable[i].processNum = 0;
		pageTable[i].count = 0;
		pageTable[i].stackNum = 0;
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
		pageTable[i].auxAddr = i * PAGE_SIZE;
	}
}

/* 初始化环境 */
void do_init()
{
	int i, j, k;
	srandom(time(NULL));
	for (i = 0; i < FPAGE_SUM; i++){
		fpageTable[i].fpageNum = i;
		if (TRUE){
			fpageTable[i].filled = TRUE;
			do_initpage(i);
		}
		else{
			fpageTable[i].filled = FALSE;
		}
	}
	/*for (i = 0; i < PAGE_SUM; i++)
	{
		pageTable[i].pageNum = i;
		pageTable[i].filled = FALSE;
		pageTable[i].edited = FALSE;
		pageTable[i].count = 0;
		/* 使用随机数设置该页的保护类型 
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
		/* 设置该页对应的辅存地址 
		pageTable[i].auxAddr = i * PAGE_SIZE * 2;
	}*/
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (TRUE)
		{
			do_page_in(&pageTable[j], j);
			pageTable[j].blockNum = j;
			pageTable[j].filled = TRUE;
			pageTable[j].stackNum = 1;
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
	Ptr_fPageTableItem ptr_fpageTabIt;
	unsigned int pageNum, offAddr, fpageNum, foffAddr;
	unsigned int actAddr;
	unsigned int i;
	
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* 计算页号和页内偏移值 */
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	fpageNum = pageNum / FPAGE_SIZE;
	foffAddr = pageNum % FPAGE_SIZE;
	printf("一级页表页号为：%u\t页内偏移为：%u\t二级页表页内偏移为：%u\n", fpageNum, foffAddr, offAddr);

	/* 获取对应页表项 */
	ptr_fpageTabIt = &fpageTable[fpageNum];
	if(!ptr_fpageTabIt->filled){
		ptr_fpageTabIt->filled = TRUE;
		do_initpage(fpageNum);
	}
	ptr_pageTabIt = &pageTable[pageNum];
	
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}
	
	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);
	
	/* 检查页面所属进程是否为发出请求的进程 */
	if(ptr_pageTabIt->processNum != ptr_memAccReq->address_fifo){
		if(ptr_pageTabIt->processNum == 0){
			ptr_pageTabIt->processNum = ptr_memAccReq->address_fifo;
		}
		else{
			do_error(ERROR_WRONG_PROCESSNO);
			return;
		}
	}
	
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->stackNum = 0;
			for(i=0;i<PAGE_SUM;i++){
				if(pageTable[i].filled==TRUE){
					pageTable[i].stackNum++;
				}			
			}
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
			ptr_pageTabIt->stackNum = 0;
			for(i=0;i<PAGE_SUM;i++){
				if(pageTable[i].filled==TRUE){				
					pageTable[i].stackNum++;
				}			
			}
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
			ptr_pageTabIt->stackNum = 0;
			for(i=0;i<PAGE_SUM;i++){
				if(pageTable[i].filled==TRUE){
					pageTable[i].stackNum++;
				}			
			}
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
	unsigned int i,j;
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
			ptr_pageTabIt->processNum = ptr_memAccReq->address_fifo;
			
			
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_LRU(ptr_pageTabIt);
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
	do_page_in(ptr_pageTabIt, pageTable[page].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
}

/* 根据LRU算法进行页面替换 */
void do_LRU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, max, page,j;
	printf("没有空闲物理块，开始进行LRU页面替换...\n");
	
	for (i = 0, max = 0, page = 0; i < PAGE_SUM; i++)
	{
		if (pageTable[i].stackNum > max)
		{
			max = pageTable[i].stackNum;
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
	pageTable[page].stackNum = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[page].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	ptr_pageTabIt->processNum = ptr_memAccReq->address_fifo;
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
		case ERROR_WRONG_PROCESSNO:
		{
			printf("访存失败：页面进程号不匹配\n");
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

/* 产生手动访存请求 */
void do_manrequest()
{
	int rqtype,viraddr;
	char c;
	unsigned char writevalue;
	printf("输入访存请求:"); //格式为 数字+数字（+字符）
	//第一个数字必须是0,1,2，0为读请求，1代表写请求，2为执行，只有写请求才需要输入字符 
	//第二个数字代表地址（0~255） 
	//栗子 0 241 或 1 123 f  
	scanf("%d %d",&rqtype,&viraddr);
	if(rqtype == 1) scanf("%u",&writevalue);
	while (c != '\n')
		c = getchar();
	if(rqtype < 0 || rqtype > 2 || viraddr < 0 || viraddr >= VIRTUAL_MEMORY_SIZE){
		printf("请求无效，随机");
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
	else{
		ptr_memAccReq->virAddr = viraddr;
		switch (rqtype)
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
				ptr_memAccReq->value = writevalue;
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
}

/* 打印一级页表 */
void do_print_finfo(){
	unsigned int i;
	printf("页号\t装入\n");
	for (i = 0; i < FPAGE_SUM; i++){
		printf("%u\t%u\n", i, fpageTable[i].filled);
	}
}

/* 打印二级页表 */
void do_print_info()
{
	unsigned int i;
	char str[4];
	printf("页号\t块号\t进程号\t装入\t修改\t保护\t计数\t辅存\t计时\n");
	for (i = 0; i < PAGE_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%u\t%s\t%u\t%u\t%u\n", i, pageTable[i].blockNum, pageTable[i].processNum, pageTable[i].filled, 
			pageTable[i].edited, get_proType_str(str, pageTable[i].proType), 
			pageTable[i].count, pageTable[i].auxAddr, pageTable[i].stackNum);
	}
}

/* 打印实存 */
void do_print_actual()
{
	unsigned int i;
	printf("地址\t内容\n");
	for (i = 0; i < ACTUAL_MEMORY_SIZE; i++)
	{
		printf("%u\t%02X\n", i, actMem[i]);
	}
}

/* 打印虚存 */
void do_print_virtual()
{
	unsigned int i,readNum;
	BYTE temp[PAGE_SIZE];
	printf("地址\t内容\n");
	for (i = 0; i < PAGE_SUM; i++)
	{
		if (fseek(ptr_auxMem, i * PAGE_SIZE, SEEK_SET) < 0)
		{
	#ifdef DEBUG
			printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
	#endif
			do_error(ERROR_FILE_SEEK_FAILED);
			exit(1);
		}
		if ((readNum = fread(temp, sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
		{
	#ifdef DEBUG
			printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
			printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
			printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
	#endif
			do_error(ERROR_FILE_READ_FAILED);
			exit(1);
		}
		printf("%u\t%02X\n", i * PAGE_SIZE, temp[0]);
		printf("%u\t%02X\n", i * PAGE_SIZE + 1, temp[1]);
		printf("%u\t%02X\n", i * PAGE_SIZE + 2, temp[2]);
		printf("%u\t%02X\n", i * PAGE_SIZE + 3, temp[3]);
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

/* 初始化已结束进程所占页表的信息 */
void do_remove() {
	int i;
	for (i = 0; i < PAGE_SUM; i++){
		if(pageTable[i].processNum == ptr_memAccReq->address_fifo){
			pageTable[i].processNum = 0;
		}
	}
}

int main(int argc, char* argv[])
{
	struct stat statbuf_0, statbuf_1;
	char c;
	int i;
	int fd_0, fd_1;
	int visited[5];
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	
	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	
	if (stat ("/tmp/server_0", &statbuf_0) == 0) 
		remove ("/tmp/server_0");
	
	mkfifo ("/tmp/server_0", 0666);

	fd_0 = open ("/tmp/server_0", O_RDONLY | O_NONBLOCK); 

	if (stat ("/tmp/server_1", &statbuf_1) == 0) 
		remove ("/tmp/server_1");
	
	mkfifo ("/tmp/server_1", 0666); 
	
	for (i = 1; i <= 4; i++)
		visited[i] = 0;
		
	while (TRUE)
	{
		bzero(ptr_memAccReq, DATALEN);		

		while (ptr_memAccReq->exist == 0)
			read (fd_0, ptr_memAccReq, DATALEN);
		
		/*printf("%d %d %d %d\n", ptr_memAccReq->exist, 
					ptr_memAccReq->ifnew, 
					ptr_memAccReq->address_fifo, ptr_memAccReq->ifcomplete);*/
		
		if (ptr_memAccReq->ifnew == 1) {
		    fd_1 = open ("/tmp/server_1", O_WRONLY);
		    for (i = 1; i <= 4; i++)
			if (visited[i] == 0)
			    break;
		    if (i <= 4)
			visited[i] = 1;
		    else
			printf("进程数已达到最大，无法再添加进程!\n");
		    write (fd_1, &i, sizeof(int));
		    close (fd_1);			
		} else if (ptr_memAccReq->ifcomplete == 1){
		    visited[ptr_memAccReq->address_fifo] = 0;
		    do_remove();
		} else {
		    do_response();
		}
		
		printf("按Y打印一级页表，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_finfo();
		while (c != '\n')
			c = getchar();
		printf("按Y打印二级页表，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info();
		while (c != '\n')
			c = getchar();
		printf("按Y打印实存，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_actual();
		while (c != '\n')
			c = getchar();
		printf("按Y打印虚存，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_virtual();
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
	return (0);
}
