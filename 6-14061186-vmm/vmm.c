#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <signal.h>
#include "fifosr.h"
#include "vmm.h"

#define MOD_LRU
//#define MOD_LFU
 //#define CLOCKS_PER_SEC   ((clock_t)1000) 
/* process页表 */

Ptr_ProcessItem process[PROC_MAXN];

/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
unsigned int blockPid[BLOCK_SUM], blockVaddr[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

int fd, fd2, goon=1;

char statbuff[1<<15];



void synch()
{
	goon=0;
}

void echo_s()
{
	char s='s';
	write(fd2,&s,1);
}

void echo_f()
{
	char f='f';
	write(fd2,&f,1);
}


void echo_c(BYTE x)
{
	write(fd2,&x,1);
}


/* 初始化环境 */
void do_init(int auxID, Ptr_PageTableItem pageTable)
{
	int i, j;
	srandom(time(NULL));
	for (i = 0; i < PAGE_SUM; i++)
	{
		pageTable[i].pageNum = i;
		pageTable[i].filled = FALSE;
		pageTable[i].edited = FALSE;
		pageTable[i].count = 0;
		pageTable[i].time_now = 0;
		/* 使用随机数设置该页的保护类型 */

		pageTable[i].proType=(BYTE)(random()%7+1);

		pageTable[i].proType|=READABLE|WRITABLE;

		/* 设置该页对应的辅存地址 */
		pageTable[i].auxAddr = (auxID << VIRTUAL_MEMORY_LEN) | (i << PAGE_LEN);
	}
}

void do_init_m(Ptr_Index1Item *ptr)
{
	int i;
	for (i=0;i<IDX1_SUM;++i) ptr[i]=NULL;
}

Ptr_PageTableItem getPageItem(int pid, unsigned int vaddr)
{

	unsigned int pageNum;
	int i;

	Ptr_PageTableItem pageTable=NULL;

	for (i=0;i<PROC_MAXN;++i)
	if (process[i]!=NULL&&process[i]->pid==pid)
	{
		pageTable=process[i]->paget;
		break;
	}

	if (pageTable==NULL)
	{
		printf("pid %d not found.\n",pid);
		return NULL;
	}

	pageNum = vaddr >> PAGE_LEN;

	return &(pageTable[pageNum]);
}

Ptr_PageTableItem getPageItem_m(int pid, unsigned int idx1, unsigned int idx2, unsigned int pageNum)
{

	int i, auxID;

	Ptr_Index1Item *pageTable=NULL;
	Ptr_Index2Item *pageTable2=NULL;

	for (i=0;i<PROC_MAXN;++i)
	if (process[i]!=NULL&&process[i]->pid==pid)
	{
		pageTable=process[i]->paget;
		auxID=i;
		break;
	}

	if (pageTable==NULL)
	{
		printf("pid %d not found.\n",pid);
		return NULL;
	}

	if (pageTable[idx1]==NULL)
	{
		pageTable[idx1]=(Ptr_Index1Item)malloc(sizeof(Index1Item));	
		for (i=0;i<IDX2_SUM;++i) pageTable[idx1]->paget[i]=NULL;
	}
	
	pageTable2=pageTable[idx1]->paget;

	if (pageTable2[idx2]==NULL)
	{
		pageTable2[idx2]=(Ptr_Index2Item)malloc(sizeof(Index2Item));	
		for (i=0;i<ITEM_SUM;++i) 
		{
			pageTable2[idx2]->paget[i].pageNum=((idx1<<IDX1_LEN)|(idx2<<IDX2_LEN)|(i<<PAGE_LEN))>>PAGE_LEN;
			pageTable2[idx2]->paget[i].filled = FALSE;
			pageTable2[idx2]->paget[i].edited = FALSE;
			pageTable2[idx2]->paget[i].count = 0;
			pageTable2[idx2]->paget[i].time_now = 0;
			/* 使用随机数设置该页的保护类型 */

			pageTable2[idx2]->paget[i].proType=(BYTE)(random()%7+1);

			pageTable2[idx2]->paget[i].proType|=READABLE|WRITABLE;

			/* 设置该页对应的辅存地址 */
			pageTable2[idx2]->paget[i].auxAddr = (auxID << VIRTUAL_MEMORY_LEN) | (pageTable2[idx2]->paget[i].pageNum << PAGE_LEN);

		}
	}

	return (pageTable2[idx2]->paget)+pageNum;
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
	pageNum = ptr_memAccReq->virAddr >> PAGE_LEN;
	offAddr = ptr_memAccReq->virAddr & (PAGE_SIZE-1);
	printf("页号为：%u\t页内偏移为：%u\n", pageNum, offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = getPageItem(ptr_memAccReq->pid, ptr_memAccReq->virAddr);
	
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt, ptr_memAccReq);
	}
	
	actAddr = (ptr_pageTabIt->blockNum << PAGE_LEN) | offAddr;
	printf("实地址为：%u\n", actAddr);
	
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				echo_f();			
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%02X\n", actMem[actAddr]);
			echo_s();
			echo_c(actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);	
				echo_f();			
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;			
			printf("写操作成功\n");
			echo_s();
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				echo_f();			
				return;
			}			
			printf("执行成功\n");
			echo_s();
			break;
		}
		default: //非法请求类型
		{	
			do_error(ERROR_INVALID_REQUEST);
			echo_f();			
			return;
		}
	}
}

void do_response_m()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int idx1, idx2, pageNum, offAddr;
	unsigned int actAddr;

	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* 计算页号和页内偏移值 */
	offAddr = ptr_memAccReq->virAddr;
	idx1 = (offAddr & ((IDX1_SUM-1)<<IDX1_LEN) )>>IDX1_LEN;
	idx2 = (offAddr & ((IDX2_SUM-1)<<IDX2_LEN) )>>IDX2_LEN;
	pageNum = (offAddr & ((ITEM_SUM-1)<<PAGE_LEN) )>>PAGE_LEN;
	offAddr = offAddr & (PAGE_SIZE-1);
	
	printf("目录1：%u\t目录2：%d\t页号为：%u\t页内偏移为：%u\n", idx1, idx2, pageNum, offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = getPageItem_m(ptr_memAccReq->pid, idx1, idx2, pageNum);
	
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt, ptr_memAccReq);
	}
	
	actAddr = (ptr_pageTabIt->blockNum << PAGE_LEN) | offAddr;
	printf("实地址为：%u\n", actAddr);
	
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			#ifdef MOD_LRU
				ptr_pageTabIt->time_now  = clock();
			#endif
			#ifdef MOD_LFU
				ptr_pageTabIt->count++;
			#endif
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				echo_f();			
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%02X\n", actMem[actAddr]);
			echo_s();
			echo_c(actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			#ifdef MOD_LRU
				ptr_pageTabIt->time_now  = clock();
			#endif
			#ifdef MOD_LFU
				ptr_pageTabIt->count++;
			#endif
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);	
				echo_f();			
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;			
			printf("写操作成功\n");
			echo_s();
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			#ifdef MOD_LRU
				ptr_pageTabIt->time_now  = clock();
			#endif
			#ifdef MOD_LFU
				ptr_pageTabIt->count++;
			#endif
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				echo_f();			
				return;
			}			
			printf("执行成功\n");
			echo_s();
			break;
		}
		default: //非法请求类型
		{	
			do_error(ERROR_INVALID_REQUEST);
			echo_f();			
			return;
		}
	}
}

/* 处理缺页中断 */
void do_page_fault(Ptr_PageTableItem ptr_pageTabIt, Ptr_MemoryAccessRequest ptr)
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
			ptr_pageTabIt->time_now = 0;
			blockVaddr[i]=ptr->virAddr;
			blockPid[i]=ptr->pid;
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	#ifdef MOD_LRU
		do_LRU(ptr_pageTabIt,ptr);
	#endif
	#ifdef MOD_LFU
		do_LFU(ptr_pageTabIt,ptr);
	#endif
}

/* 根据LFU算法进行页面替换 */
void do_LFU(Ptr_PageTableItem ptr_pageTabIt,Ptr_MemoryAccessRequest req)
{
	unsigned int i, min, block;
	unsigned int idx1, idx2, pageNum, offAddr;
	Ptr_PageTableItem ptr, ptr2;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for (i = 0, min = 0xFFFFFFFF, block = 0; i < BLOCK_SUM; i++)
	if (blockStatus[i])
	{
		//ptr=getPageItem(blockPid[i],blockVaddr[i]);
		//singlePage

		offAddr = blockVaddr[i];
		idx1 = (offAddr & ((IDX1_SUM-1)<<IDX1_LEN) )>>IDX1_LEN;
		idx2 = (offAddr & ((IDX2_SUM-1)<<IDX2_LEN) )>>IDX2_LEN;
		pageNum = (offAddr & ((ITEM_SUM-1)<<PAGE_LEN) )>>PAGE_LEN;

		ptr=getPageItem_m(blockPid[i],idx1,idx2,pageNum);
		//multiplyPage
		// printf("%d %d\n", blockPid[i],blockVaddr[i]/PAGE_SIZE);
		// printf("%d %d\n", ptr->blockNum,ptr->filled);
		if (ptr->count < min && ptr->filled)
		{
//			printf("%d\n",i); 
			min = ptr->count;
			block = i;
			ptr2 = ptr;
		}
	}
	printf("选择第%ublock进行替换\n", block);
	//if (ptr2==NULL) printf("NULL\n");
	if (ptr2->edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(ptr2);
	}
	ptr2->filled = FALSE;
	ptr2->count = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, block);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = block;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	blockVaddr[block]=req->virAddr;
	blockPid[block]=req->pid;
	printf("页面替换成功\n");
}

/*LRU*/
void do_support_LRU()
{
	unsigned int i, min, block;
	unsigned int idx1, idx2, pageNum, offAddr;
	Ptr_PageTableItem ptr, ptr2;
	for (i = 0,block = 0; i < BLOCK_SUM; i++)
	if (blockStatus[i])
	{
		//ptr=getPageItem(blockPid[i],blockVaddr[i]);
		//singlePage

		offAddr = blockVaddr[i];
		idx1 = (offAddr & ((IDX1_SUM-1)<<IDX1_LEN) )>>IDX1_LEN;
		idx2 = (offAddr & ((IDX2_SUM-1)<<IDX2_LEN) )>>IDX2_LEN;
		pageNum = (offAddr & ((ITEM_SUM-1)<<PAGE_LEN) )>>PAGE_LEN;

		ptr=getPageItem_m(blockPid[i],idx1,idx2,pageNum);
		//multiplyPage
		// printf("%d %d\n", blockPid[i],blockVaddr[i]/PAGE_SIZE);
		// printf("%d %d\n", ptr->blockNum,ptr->filled);
		ptr->time_now = 0;
	}
}

void do_LRU(Ptr_PageTableItem ptr_pageTabIt,Ptr_MemoryAccessRequest req)
{
	clock_t min;
	unsigned int i, block;
	unsigned int idx1, idx2, pageNum, offAddr;
	Ptr_PageTableItem ptr, ptr2;
	printf("没有空闲物理块，开始进行LRU页面替换...\n");
	for (i = 0, min = clock(), block = 0; i < BLOCK_SUM; i++)
	if (blockStatus[i])
	{
		//ptr=getPageItem(blockPid[i],blockVaddr[i]);
		//singlePage

		offAddr = blockVaddr[i];
		idx1 = (offAddr & ((IDX1_SUM-1)<<IDX1_LEN) )>>IDX1_LEN;
		idx2 = (offAddr & ((IDX2_SUM-1)<<IDX2_LEN) )>>IDX2_LEN;
		pageNum = (offAddr & ((ITEM_SUM-1)<<PAGE_LEN) )>>PAGE_LEN;

		ptr=getPageItem_m(blockPid[i],idx1,idx2,pageNum);
		//multiplyPage
		// printf("%d %d\n", blockPid[i],blockVaddr[i]/PAGE_SIZE);
		// printf("%d %d\n", ptr->blockNum,ptr->filled);
		if (ptr->time_now < min && ptr->filled)
		{
//			printf("%d\n",i); 
			min = ptr->time_now;
			block = i;
			ptr2 = ptr;
		}
	}
	printf("选择第%ublock进行替换\n", block);
	//if (ptr2==NULL) printf("NULL\n");
	if (ptr2->edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(ptr2);
	}
	ptr2->filled = FALSE;
	ptr2->time_now= (clock_t)0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, block);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = block;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->time_now = (clock_t)0;
	blockVaddr[block]=req->virAddr;
	blockPid[block]=req->pid;
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
	if ((readNum = fread(actMem + (blockNum << PAGE_LEN), 
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
	if ((writeNum = fwrite(actMem + (ptr_pageTabIt->blockNum << PAGE_LEN), 
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
	printf("写回成功：物理块%u-->>辅存地址%03X\n", ptr_pageTabIt->blockNum, ptr_pageTabIt->auxAddr);
}


/* 产生访存请求 */
void do_request(int pid)
{
	/* 随机产生请求地址 */
	ptr_memAccReq->pid=pid;
	ptr_memAccReq->virAddr = random() & (VIRTUAL_MEMORY_SIZE-1);
	/* 随机产生请求类型 */
	switch (random() %5 % 3)
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

int getItemInfo(int id, Ptr_PageTableItem ptr, char *buf)
{
	char str[4];
	if (ptr==NULL)
		return sprintf(buf,"%u\t%u\t%u\t%u\t%s\t%u\t%u\n", id, 0, FALSE, 
			FALSE, get_proType_str(str, 0x00u), 
			0, 0);
	else
		#ifdef MOD_LRU
			return sprintf(buf,"%u\t%u\t%u\t%u\t%s\t%u\t%u\n", id, ptr->blockNum, ptr->filled, 
			ptr->edited, get_proType_str(str, ptr->proType), 
			ptr->time_now, ptr->auxAddr);
		#endif
		#ifdef MOD_LFU
			return sprintf(buf,"%u\t%u\t%u\t%u\t%s\t%u\t%u\n", id, ptr->blockNum, ptr->filled, 
			ptr->edited, get_proType_str(str, ptr->proType), 
			ptr->count, ptr->auxAddr);
		#endif		
}

int getPageInfo2(int x, Ptr_Index1Item ptr, char *buf)
{
	int i,j,cnt=0;
	for (i=0;i<IDX2_SUM;++i)
	{
		if (ptr->paget[i]!=NULL)
		{
			for (j=0;j<ITEM_SUM;++j)
				cnt+=getItemInfo(x*(IDX1_SIZE>>PAGE_LEN)+i*ITEM_SUM+j,&(ptr->paget[i]->paget[j]),buf+cnt);
		}
		else {
			for (j=0;j<ITEM_SUM;++j)
				cnt+=getItemInfo(x*(IDX1_SIZE>>PAGE_LEN)+i*ITEM_SUM+j,NULL,buf+cnt);
		}
	}
	return cnt;
}

int getPageInfo(int x, char *buf)
{
	int i,j,cnt=0;
	for (i=0;i<IDX1_SUM;++i)
	{
		if (process[x]->paget[i]!=NULL)
			cnt+=getPageInfo2(i,process[x]->paget[i],buf+cnt);
		else {
			for (j=0;j<(IDX1_SIZE>>PAGE_LEN);++j)
				cnt+=getItemInfo(i*(IDX1_SIZE>>PAGE_LEN)+j,NULL,buf+cnt);
		}
	}
	return cnt;
}

/* 打印页表 */
void do_print_info(int x, int writeback)
{
	unsigned int i, j, k, offset=0;
//	char str[4];
//	Ptr_PageTableItem pageTable;
	if (x<0||x>=PROC_MAXN)
	{
		for (i=0;i<PROC_MAXN;++i) 
			if (process[i]!=NULL&&process[i]->pid==x)
			{
				x=i;
				break;
			}
	}
	if (x<0||x>=PROC_MAXN) {
		printf("pid %d not found.\n",x);
		if (writeback) echo_f();
		return;
	}
	offset+=sprintf(statbuff+offset,"页号\t块号\t装入\t修改\t保护\t计数\t辅存\n");


	// pageTable=process[x]->paget;		
	// for (i = 0; i < PAGE_SUM; i++)
	// {
	// 	offset+=sprintf(statbuff+offset,"%u\t%u\t%u\t%u\t%s\t%u\t%u\n", i, pageTable[i].blockNum, pageTable[i].filled, 
	// 		pageTable[i].edited, get_proType_str(str, pageTable[i].proType), 
	// 		pageTable[i].count, pageTable[i].auxAddr);
	// }
	//singlePage


	offset+=getPageInfo(x,statbuff+offset);
	//multiplyPage
	statbuff[offset]=0;
	if (writeback)
	{
		echo_s();
		write(fd2,statbuff,offset);
		echo_c('$');
	}
	printf("%s\n", statbuff);
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

void dispose()
{
	int i;
	struct stat statb;	
	close(fd);
	close(fd2);
	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	for (i=0;i<PROC_MAXN;++i) 
		if (process[i]!=NULL) free_proc(i);
	if(stat(REQFIFO,&statb)==0){
		remove(REQFIFO);
	}
	if(stat(RESFIFO,&statb)==0){
		remove(RESFIFO);
	}
	printf("bye\n");
	exit(0);
}

void new_proc(int pid)
{
	int i;
	for (i=0;i<PROC_MAXN;++i)
	if (process[i]==NULL)
	{
		process[i]=(Ptr_ProcessItem)malloc(sizeof(ProcessItem));
		process[i]->pid=pid;
		//do_init(i,process[i]->paget);
		//singlePage
		do_init_m(process[i]->paget);
		//multiplyPage
		//do_print_info(i,0);
		echo_s();
		i=getpid();
		write(fd2,&i,4);		
		return;
	}
	printf("Insufficient Memory.\n");
	echo_f();
}

void m_free(Ptr_Index1Item ptr)
{
	int j,i;
	if (ptr==NULL) return;
	for (j=0;j<IDX2_SUM;++j)
	if (ptr->paget[j]!=NULL)
	{
		for (i=0;i<ITEM_SUM;++i)
		if (ptr->paget[j]->paget[i].filled)
		{
			blockStatus[ptr->paget[j]->paget[i].blockNum]=FALSE;
		} 
		free(ptr->paget[j]);
		ptr->paget[j]=NULL;
	}
	free(ptr);
}

void free_proc(int pid)
{
	int i,j;
	if (pid>=PROC_MAXN||pid<0)
	for (i=0;i<PROC_MAXN;++i)
	if (process[i]!=NULL&&process[i]->pid==pid)
	{
		j=i;
		break;
	}
	
	if (j<PROC_MAXN&&j>=0)
	{
		i=j;
		// for (j=0;j<PAGE_SUM;++j)
		// {
		// 	if (process[i]->paget[j].filled)
		// 	{
		// 		blockStatus[process[i]->paget[j].blockNum]=FALSE;
		// 	}
		// }
		// singlePage

		for (j=0;j<IDX1_SUM;++j)
		if (process[i]->paget[j]!=NULL)
		{
			m_free(process[i]->paget[j]);	
			process[i]->paget[j]=NULL;		
		}
		//multiplyPage
		free(process[i]);
		process[i]=NULL;
		printf("rm: pid %d\n", pid);
		echo_s();
		return;
	}
	printf("pid %d not found.\n",pid);
	echo_f();
}


void environmentInit()
{
	int i,j;
	struct stat statb;	
	char c=0;
	printf("Initiailizing...\n");
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "w+")))
		{
			do_error(ERROR_FILE_OPEN_FAILED);
			exit(1);
		}
		else {
			for (i=0;i<DISK_SIZE;++i) fwrite(&c,1,1,ptr_auxMem);
			fclose(ptr_auxMem);
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
	}
	printf("DISK STATUS: ready\n");
	signal(SIGINT,dispose);
	signal(SIGUSR1,synch);
	for (i=0;i<PROC_MAXN;++i) process[i]=NULL;
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	for (j = 0; j < BLOCK_SUM; j++)
	{
		blockStatus[j] = FALSE;
	}

	if(stat(REQFIFO,&statb)==0){
		/*  */
		if(remove(REQFIFO)<0)
		{
			do_error(ERROR_FILE_OPEN_FAILED);
			dispose();
		}
	}

	if(mkfifo(REQFIFO,0777)<0||(fd=open(REQFIFO,O_RDONLY))<0)
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		dispose();
	}

	printf("REQFIFO STATUS: ready\n");

	printf("started.\n");
}

char getReq()
{
	char c=' ';
	int count, pid=0;
	while ((count = read(fd,&c,1))<=0)
	{
		if (count<0)
		{
			do_error(ERROR_FILE_OPEN_FAILED);
			dispose();
		}
	}
	if (count==0||c!='i'&&c!='e'&&c!='s'&&c!='r')
		return ' ';

	if (c=='i') 
	{
		count=0;
		sleep(1);
		while ((fd2=open(RESFIFO,O_WRONLY))<0) 
		{
			++count;
			sleep(0.1);
			if (count>100)
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				dispose();
			}
		}		
	}
	else 
	{
		while (goon);
		if ((fd2=open(RESFIFO,O_WRONLY))<0) 
		{
			do_error(ERROR_FILE_OPEN_FAILED);
			dispose();
		}
		goon=1;
	}

	if((count = read(fd,&pid,4))<=0)
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		dispose();
	}

	printf("received from Pid %d: %c\n",pid,c);
	ptr_memAccReq->pid=pid;
	if (c=='r')
	{
		if((count = read(fd,ptr_memAccReq,sizeof(MemoryAccessRequest)))<=0)
		{
			do_error(ERROR_FILE_OPEN_FAILED);
			dispose();
		}

	}
	return c;
}

int main(int argc, char* argv[])
{
	char c=0;
	environmentInit();
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		#ifdef MOD_LRU
			signal(SIGALRM,do_support_LRU);
			alarm(1000);
		#endif
		switch (getReq())
		{
			case 'e':
				free_proc(ptr_memAccReq->pid);
				break;			
			case 'i':
				new_proc(ptr_memAccReq->pid);
				break;			
			case 's':
				do_print_info(ptr_memAccReq->pid,1);
				break;
			case 'r':
				//do_response();
				//singlePage

				do_response_m();
				//multiplyPage
				break;
			default:
				break;				
		}
		close(fd2);
		//sleep(5000);
	}

	return 0;
}
