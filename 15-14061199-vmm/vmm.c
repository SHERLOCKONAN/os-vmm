#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vmm.h"
#include <string.h>
#include "global.h"
#include "page_table.h"
/* 实存 */
mem_manage mmg;
uchar space[BLOCK_SUM * PAGE_SIZE];

/* 页表 */
PageTableItem pageTable[PAGE_NUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

pid_t get_pid_by_block_num(ushort block_num)
{
	return mmg.items[block_num].pid;
}


ushort get_vpage(ushort block_num)
{
	return mmg.items[block_num].v_page;
}

void set_mm_ctrl(ushort block_num, pid_t pid, ushort vpage)
{
	mmg.items[block_num].pid = pid;
	mmg.items[block_num].age = 0;
	mmg.items[block_num].v_page = vpage;
}

void age(ushort block_num)
{
	static int count = 0;
	if(count++ == 2){
		int i;
		count = 0;
		for(i = block_num&0x7;i < BLOCK_SUM; i += 0x8){
			mmg.items[i].age >>= 1;
			printf("%llu ",mmg.items[i].age);
		}
	}
	mmg.items[block_num].age|=(1llu<<63);
}

ulong64 get_age(ushort block_num)
{
	return mmg.items[block_num].age;
}

ushort get_pre(ushort vpage)
{
	int i;
	ushort res = 0xffff;
	for(i=vpage&0x7;i<32;i+=0x8){
		if(mmg.items[i].pid == 0)
			return i;
		else if(res == 0xffff){
			res = i;
		}
		else{
			if(mmg.items[i].age<mmg.items[res].age){
				res = i;
			}
		}
	}
	return res;
}

ushort read_byte(ushort block_num, ushort offset)
{
	age(block_num);
	ushort addr = block_num << OFFSET_BIT_NUM | (offset&0xf);
	return space[addr];
}
void write_byte(ushort block_num, ushort offset, ushort data)
{
	age(block_num);
	ushort addr = block_num << OFFSET_BIT_NUM | (offset&0xf);
	space[addr] = data;
}
void load_page(ushort block_num, FILE* file, ulong offset)
{
	fseek(file,offset,SEEK_SET);
	fread(space + (block_num<<OFFSET_BIT_NUM), PAGE_SIZE, 1, file);
	if(offset >= 56*16){
//		printf("load a page\n");
//		for(i = 0;i<8;++i){
			extern ushort page_storage_page(table_item);
//			printf("%5d ",page_storage_page(&((page_table)(space+(block_num<<OFFSET_BIT_NUM)))->items[i]));
//		}
		printf("\n");
	}
	
}
void store_page(ushort block_num, FILE* file ,ulong offset)
{
	fseek(file,offset,SEEK_SET);
	fwrite(space + (block_num<<OFFSET_BIT_NUM), PAGE_SIZE, 1, file);
}
/* 初始化环境 */
void do_init_mm()
{
	int i;//, j;
//	srandom(time(NULL));
	/* 初始化实存 */
	for(i = 0;i < BLOCK_SUM;++i){
		mmg.items[i].pid = 0;
	}
//	for (i = 0; i < PAGE_SUM; i++)
//	{
//		pageTable[i].pageNum = i;
//		pageTable[i].filled = FALSE;
//		pageTable[i].edited = FALSE;
//		pageTable[i].count = 0;
//		/* 使用随机数设置该页的保护类型 */
//		switch (random() % 7)
//		{
//			case 0:
//			{
//				pageTable[i].proType = READABLE;
//				break;
//			}
//			case 1:
//			{
//				pageTable[i].proType = WRITABLE;
//				break;
//			}
//			case 2:
//			{
//				pageTable[i].proType = EXECUTABLE;
//				break;
//			}
//			case 3:
//			{
//				pageTable[i].proType = READABLE | WRITABLE;
//				break;
//			}
//			case 4:
//			{
//				pageTable[i].proType = READABLE | EXECUTABLE;
//				break;
//			}
//			case 5:
//			{
//				pageTable[i].proType = WRITABLE | EXECUTABLE;
//				break;
//			}
//			case 6:
//			{
//				pageTable[i].proType = READABLE | WRITABLE | EXECUTABLE;
//				break;
//			}
//			default:
//				break;
//		}
//		/* 设置该页对应的辅存地址 */
//		pageTable[i].auxAddr = i * PAGE_SIZE * 2;
//	}
//	for (j = 0; j < BLOCK_SUM; j++)
//	{
//		/* 随机选择一些物理块进行页面装入 */
//		if (random() % 2 == 0)
//		{
//			do_page_in(&pageTable[j], j);
//			pageTable[j].blockNum = j;
//			pageTable[j].filled = TRUE;
//			blockStatus[j] = TRUE;
//		}
//		else
//			blockStatus[j] = FALSE;
//	}
}


/* 响应请求 */
//void do_response()
//{
//	Ptr_PageTableItem ptr_pageTabIt;
//	unsigned int pageNum, offAddr;
//	unsigned int actAddr;
//	
//	/* 检查地址是否越界 */
//		/*无符号数能小于零?*/
//	if (/*ptr_memAccReq->virAddr < 0 ||*/ ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
//	{
//		do_error(ERROR_OVER_BOUNDARY);
//		return;
//	}
//	
//	/* 计算页号和页内偏移值 */
//	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
//	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
//	printf("页号为：%u\t页内偏移为：%u\n", pageNum, offAddr);
//
//	/* 获取对应页表项 */
//	ptr_pageTabIt = &pageTable[pageNum];
//	
//	/* 根据特征位决定是否产生缺页中断 */
//	if (!ptr_pageTabIt->filled)
//	{
//		do_page_fault(ptr_pageTabIt);
//	}
//	
//	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
//	printf("实地址为：%u\n", actAddr);
//	
//	/* 检查页面访问权限并处理访存请求 */
//	switch (ptr_memAccReq->reqType)
//	{
//		case REQUEST_READ: //读请求
//		{
//			ptr_pageTabIt->count++;
//			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
//			{
//				do_error(ERROR_READ_DENY);
//				return;
//			}
//			/* 读取实存中的内容 */
//			printf("读操作成功：值为%02X\n", actMem[actAddr]);
//			break;
//		}
//		case REQUEST_WRITE: //写请求
//		{
//			ptr_pageTabIt->count++;
//			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
//			{
//				do_error(ERROR_WRITE_DENY);	
//				return;
//			}
//			/* 向实存中写入请求的内容 */
//			actMem[actAddr] = ptr_memAccReq->value;
//			ptr_pageTabIt->edited = TRUE;			
//			printf("写操作成功\n");
//			break;
//		}
//		case REQUEST_EXECUTE: //执行请求
//		{
//			ptr_pageTabIt->count++;
//			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
//			{
//				do_error(ERROR_EXECUTE_DENY);
//				return;
//			}			
//			printf("执行成功\n");
//			break;
//		}
//		default: //非法请求类型
//		{	
//			do_error(ERROR_INVALID_REQUEST);
//			return;
//		}
//	}
//}

/* 处理缺页中断 */
//void do_page_fault(Ptr_PageTableItem ptr_pageTabIt)
//{
//	unsigned int i;
//	printf("产生缺页中断，开始进行调页...\n");
//	for (i = 0; i < BLOCK_SUM; i++)
//	{
//		if (!blockStatus[i])
//		{
//			/* 读辅存内容，写入到实存 */
//			do_page_in(ptr_pageTabIt, i);
//			
//			/* 更新页表内容 */
//			ptr_pageTabIt->blockNum = i;
//			ptr_pageTabIt->filled = TRUE;
//			ptr_pageTabIt->edited = FALSE;
//			ptr_pageTabIt->count = 0;
//			
//			blockStatus[i] = TRUE;
//			return;
//		}
//	}
//	/* 没有空闲物理块，进行页面替换 */
//	do_LFU(ptr_pageTabIt);
//}
//
///* 根据LFU算法进行页面替换 */
//void do_LFU(Ptr_PageTableItem ptr_pageTabIt)
//{
//	unsigned int i, min, page;
//	printf("没有空闲物理块，开始进行LFU页面替换...\n");
//	for (i = 0, min = 0xFFFFFFFF, page = 0; i < PAGE_NUM; i++)
//	{
//		if (pageTable[i].count < min)
//		{
//			min = pageTable[i].count;
//			page = i;
//		}
//	}
//	printf("选择第%u页进行替换\n", page);
//	if (pageTable[page].edited)
//	{
//		/* 页面内容有修改，需要写回至辅存 */
//		printf("该页内容有修改，写回至辅存\n");
//		do_page_out(&pageTable[page]);
//	}
//	pageTable[page].filled = FALSE;
//	pageTable[page].count = 0;
//
//
//	/* 读辅存内容，写入到实存 */
//	do_page_in(ptr_pageTabIt, pageTable[page].blockNum);
//	
//	/* 更新页表内容 */
//	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
//	ptr_pageTabIt->filled = TRUE;
//	ptr_pageTabIt->edited = FALSE;
//	ptr_pageTabIt->count = 0;
//	printf("页面替换成功\n");
//}
//
/* 将辅存内容写入实存 */
//void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
//{
//	unsigned int readNum;
//	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
//	{
//#ifdef DEBUG
//		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
//#endif
//		do_error(ERROR_FILE_SEEK_FAILED);
//		exit(1);
//	}
//	if ((readNum = fread(actMem + blockNum * PAGE_SIZE, 
//		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
//	{
//#ifdef DEBUG
//		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
//		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
//		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
//#endif
//		do_error(ERROR_FILE_READ_FAILED);
//		exit(1);
//	}
//	printf("调页成功：辅存地址%lu-->>物理块%u\n", ptr_pageTabIt->auxAddr, blockNum);
//}

/* 将被替换页面的内容写回辅存 */
//void do_page_out(Ptr_PageTableItem ptr_pageTabIt)
//{
//	unsigned int writeNum;
//	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
//	{
//#ifdef DEBUG
//		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt, ftell(ptr_auxMem));
//#endif
//		do_error(ERROR_FILE_SEEK_FAILED);
//		exit(1);
//	}
//	if ((writeNum = fwrite(actMem + ptr_pageTabIt->blockNum * PAGE_SIZE, 
//		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
//	{
//#ifdef DEBUG
//		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
//		printf("DEBUG: writeNum=%u\n", writeNum);
//		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
//#endif
//		do_error(ERROR_FILE_WRITE_FAILED);
//		exit(1);
//	}
//	printf("写回成功：物理块%lu-->>辅存地址%03X\n", ptr_pageTabIt->auxAddr, ptr_pageTabIt->blockNum);
//}

/* 错误处理 */
void do_error(ERROR_CODE code)
{
	switch (code)
	{
		case ERROR_PROC_INVALID:
			printf("错误指令：没有这个进程\n");
			break;
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
void do_request_rand()
{
	/* 随机产生请求地址 */
	ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
	/* 随机产生请求类型 */
	switch (random() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n地址：%lu\t类型：读取\n", ptr_memAccReq->virAddr);
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = random() % 0xFFu;
			printf("产生请求：\n地址：%lu\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n地址：%lu\t类型：执行\n", ptr_memAccReq->virAddr);
			break;
		}
		default:
			break;
	}	
}
int do_request()
{
	char temp[10];
	int t;
	//char op;
	if(EOF == fscanf(stdin," %s",temp)){
		printf("Input End, process quit");
		exit(0);
	}
	if(strcmp(temp,"print") == 0){
		do_print_info();
		return 1;
	}
	if(strlen(temp) != 1){
		printf("Input format: (r|w|x) address (data)?\n");
		return -1;
	}

	switch(temp[0]){
		case 'r':
			ptr_memAccReq->reqType = REQUEST_READ;
			fscanf(stdin,"%ld",&ptr_memAccReq->virAddr);
			printf("产生请求：\n地址：%lu\t类型：读取\n", ptr_memAccReq->virAddr);
			break;
		case 'w':
			ptr_memAccReq->reqType = REQUEST_WRITE;
			fscanf(stdin,"%ld",&ptr_memAccReq->virAddr);
			fscanf(stdin," %d",&t);ptr_memAccReq->value = (char)t;;
			printf("产生请求：\n地址：%lu\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		case 'x':
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			fscanf(stdin,"%ld",&ptr_memAccReq->virAddr);
			printf("产生请求：\n地址：%lu\t类型：执行\n", ptr_memAccReq->virAddr);
			break;
		default:
			printf("Input format: (r|w|x) address (data)?\n");
			return -1;
	}
	return 0;
}


/* 打印页表 */
void do_print_info()
{
	unsigned int i;
	char str[4];
	printf("页号\t块号\t装入\t修改\t保护\t计数\t辅存\n");
	for (i = 0; i < PAGE_NUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%s\t%lu\t%lu\n", i, pageTable[i].blockNum, pageTable[i].filled, 
			pageTable[i].edited, get_proType_str(str, pageTable[i].proType), 
			pageTable[i].count, pageTable[i].auxAddr);
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

//int main()//int argc, char* argv[])
//{
////	char c;
//	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
//	{
//		void* mem = NULL;
//		if(!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "wr+"))){
//			do_error(ERROR_FILE_OPEN_FAILED);
//			exit(1);
//		}
//		if(!(mem = malloc(VIRTUAL_MEMORY_SIZE*2))){
//			do_error(ERROR_FILE_OPEN_FAILED);
//			exit(1);
//		}
//		fwrite(mem,VIRTUAL_MEMORY_SIZE*2,1,ptr_auxMem);
//		free(mem);
//
//	}
//	
//	do_init_mm();
//	do_print_info();
//	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
//	/* 在循环中模拟访存请求与处理过程 */
//	while (TRUE)
//	{
//		while(do_request());
//		do_response();
////		printf("按Y打印页表，按其他键不打印...\n");
////		if ((c = getchar()) == 'y' || c == 'Y')
////			do_print_info();
////		while (c != '\n')
////			c = getchar();
////		printf("按X退出程序，按其他键继续...\n");
////		if ((c = getchar()) == 'x' || c == 'X')
////			break;
////		while (c != '\n')
////			c = getchar();
//		//sleep(5000);
//	}
//
//	if (fclose(ptr_auxMem) == EOF)
//	{
//		do_error(ERROR_FILE_CLOSE_FAILED);
//		exit(1);
//	}
//	return (0);
//}
