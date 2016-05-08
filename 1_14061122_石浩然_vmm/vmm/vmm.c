#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include "vmm.h"


/* 页表 */
PageTableItem pageTable[PAGE_SUM];

/*******************一级页表********************/
FPageTableItem fpageTable[FPageNum];

/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
FILE *head_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

int Operate[64];

int N  =  0;

time_t rtime;
/* 初始化环境 */
void do_init()
{
	int i, j,k,h,l;
	srand(time(NULL));
	for (i = 0; i < PAGE_SUM; i++)
	{
		pageTable[i].pageNum = i;
		pageTable[i].filled = FALSE;
		pageTable[i].edited = FALSE;
		pageTable[i].count = 0;
		/* 使用随机数设置该页的可访问进程号 */
		for(l=0;l<Process_SIZE;l++)
        {
            switch (rand() % 2)
            {
                case 0:
                    {
                        pageTable[i].processNum[l]=FALSE;
                        break;
                    }
                case 1:
                    {
                        pageTable[i].processNum[l]=TRUE;
                        break;
                    }
            }
        }
        BOOL flag=FALSE;
        for(l=0;l<Process_SIZE;l++)
        {
            if(pageTable[i].processNum[l]==TRUE)
                flag=TRUE;
        }
        if(flag==FALSE)
        {
            int m=rand() % Process_SIZE;
            pageTable[i].processNum[m]=TRUE;
        }
		/* 使用随机数设置该页的保护类型 */
		switch (rand() % 7)
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
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (rand() % 2 == 0)
		{
			do_page_in(&pageTable[j], j);
			pageTable[j].blockNum = j;
			pageTable[j].filled = TRUE;
			time(&rtime);
			pageTable[j].time = rtime;
			blockStatus[j] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
	for (k=0; k < FPageNum; k++){
		for(h=0;h<(PAGE_SUM/FPageNum);h++){
			fpageTable[k].page[h]=&pageTable[(k*16)+h];
		}

	}
	for(h=0;h<64;h++){
		Operate[h]=-1;
	}
}

/******************************页面老化*****************************/
void do_old(Ptr_PageTableItem ptr_pageTabIt){

	int i = 0,min = 0,mark=0,page;

	printf("Missing page, the page execution aging algorithm ...\n");
	min  = pageTable[0].time;
	for(i = 1;i<64;i++){
		if(min>pageTable[i].time&&pageTable[i].filled==1){
			min  = pageTable[i].time;
			mark = i;
		}
	}
	printf("Number one page table %dNumber two page table %d\n",mark/16,mark%16);
	page = mark;
	if (pageTable[page].edited)
	{

		printf("Edit the page, write back\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].filled = FALSE;
	printf("filled status %d",pageTable[page].filled);
	pageTable[page].count = 0;


	/* ?á?¨′??úèY￡?D′è?μ?êμ′? */
	do_page_in(ptr_pageTabIt, pageTable[page].blockNum);

	/* ?üD?ò3±í?úèY */
	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	time(&rtime);
	ptr_pageTabIt->time = rtime;
	ptr_pageTabIt->count = 0;
	printf("Page replacement Success\n");
}

/* 响应请求 */
void do_response()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum, offAddr;
	unsigned int actAddr;

	/*检查请求是否越界*/
	if((ptr_memAccReq->reqType>2)||(ptr_memAccReq->reqType<0)){
		return;

	}


	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}

	/* 计算页号和页内偏移值 */
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("page number is：%u\tIn-page offset is：%u\n", pageNum, offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = &pageTable[pageNum];

	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}

	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("Real address is：%u\n", actAddr);

	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("Read operation is successful: value is%02X\n", actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;
			printf("write operation is successful\n");
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				return;
			}
			printf("execution succeed\n");
			break;
		}
		default: //非法请求类型
		{
			do_error(ERROR_INVALID_REQUEST);
			return;
		}
	}
}
/***************************************多级页表处理*******************************/
void do_responsemult(){
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int fpageNum,npageNum,offAddr;
	unsigned int actAddr;
	unsigned int temp;
	/*检查请求是否越界*/
	if((ptr_memAccReq->reqType>2)||(ptr_memAccReq->reqType<0)){
		return;
	}
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= PageTableItem_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}

	/* 计算页号和页内偏移值 */
	fpageNum = ptr_memAccReq->virAddr / FPage_SIZE;
	temp  = ptr_memAccReq->virAddr - fpageNum*FPage_SIZE;
	npageNum = temp / NPage_SIZE;
	offAddr = temp % NPage_SIZE;
	if((fpageNum>3)||(npageNum>15)||(offAddr>3)){
		do_error(ERROR_INVALID_REQUEST);
		return;
	}

	printf("number one page is：%u\tnumber two page is： %u\tinpage offect is：%u\n", fpageNum, npageNum,offAddr);

	/* 获取对应页表项 */

	ptr_pageTabIt = fpageTable[fpageNum].page[npageNum];

    /* 判断进程能否访问 */
    if(ptr_pageTabIt->processNum[ptr_memAccReq->ProcessNum]==FALSE)
    {
        do_error(ERROR_PROCESS_DENY);
        return;
    }


	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}
	else {
		Operate[N] = fpageNum*16+npageNum;
		N=(N+1)%64;
	}


	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("real address is：%u\n", actAddr);

	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("Read operation is successful: value is%02X\n", actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;
			printf("write operation is successful\n");
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				return;
			}
			printf("execution succeed\n");
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
	printf("Generate a page fault to begin paging ...\n");
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
			time(&rtime);
			ptr_pageTabIt->time = rtime;
			Operate[N] = i;
			N=(N+1)%64;
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	/***************************************************************************/
	//do_LFU(ptr_pageTabIt);
	do_LRU(ptr_pageTabIt);
}

/* 根据LFU算法进行页面替换 */
void do_LFU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, min, page;
	printf("No free physical block to begin LRU page replacement ...\n");
	for (i = 0, min = 0xFFFFFFFF, page = 0; i < PAGE_SUM; i++)
	{
		if (pageTable[i].count < min&&pageTable[i].filled==TRUE)
		{
			min = pageTable[i].count;
			page = i;
		}
	}
	unsigned int fpageNum,npageNum,offAddr;
	unsigned int temp;
	fpageNum = page / FPage_SIZE;
	temp  = page - fpageNum*FPage_SIZE;
	npageNum = temp / NPage_SIZE;
	offAddr = temp % NPage_SIZE;
	if((fpageNum>3)||(npageNum>15)||(offAddr>3)){
		do_error(ERROR_INVALID_REQUEST);
		return;
	}
	printf("choose %d block the %d page to replace\n",fpageNum, npageNum);
	if (pageTable[page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("Page content changes, write back to auxiliary storage\n");
		do_page_out(&pageTable[page]);
	}




	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[page].blockNum);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	pageTable[page].filled = FALSE;
	pageTable[page].count = 0;
	pageTable[page].edited=FALSE;
	pageTable[page].blockNum=0;
	printf("Page replacement Success\n");
}


/************************************LRU算法************************************/
void do_LRU(Ptr_PageTableItem ptr_pageTabIt){
	int lru[64]={0};
	int i = 0,j = 0;
	int flag = 0;
	int fpagenum=0,npagenum=0,page;
	printf("No free physical block to begin LRU page replacement ...\n");
	for(i = 0;i<64;i++)
	{
		lru[i] = -1;
	}
	for(i = N-1;i!=N;i--)
	{
		i = (i+64)%64;
		flag = 0;
		for(j = 0;lru[j]!=-1,j<64;j++)
			if(lru[j] == Operate[i]&&Operate[i]!=-1)
				flag = 1;
		for(j = 0;j<64;j++)
			if(lru[j]==-1)
				break;
		if(flag == 0&&j!=64&&Operate[i]!=-1&&pageTable[Operate[i]].filled==1)
			lru[j] = Operate[i];
        if(i == 0)
            i = 64;
	}

	flag = 0;
	for(j = 0;lru[j]!=-1,j<64;j++)
			if(lru[j] == Operate[i]&&Operate[i]!=-1)
				flag = 1;
	for(j = 0;j<64;j++)
			if(lru[j]==-1)
				break;
	if(flag == 0&&j!=64&&Operate[i]!=-1&&pageTable[Operate[i]].filled==1)
		lru[j] = Operate[i];
	for(i = 63;i>=0;i--)
		if(lru[i]!=-1)
		{
			//printf("%d,,,%d",lru[i],i);
			break;
		}
	//printf("%d,,,%d",lru[i],i);
	fpagenum = lru[i]/16;
	npagenum = lru[i]%16;
	/*for(i=0;i<64;i++){
		if(Operate[i]!=-1){
			lru[Operate[i]] = 1;
		}
	}

	for(i=0;i<64;i++){
		if((lru[i]==0)&&(pageTable[i].filled==1)){

			fpagenum = i/16;
			npagenum = i%16;
			flag = 1;
			break;
		}
	}
	if(flag = 0){
		fpagenum = rand()%4;
		npagenum = rand()%16;
	}*/
	printf("choose %d block the %d page to replace\n",fpagenum, npagenum);
	page = fpagenum*16+npagenum;
	Operate[N] = page;
	N = (N+1)%64;

	if (pageTable[page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("Page content changes, write back to auxiliary storage\n");
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
	printf("Page replacement Success\n");

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
	printf("Paging success：Auxiliary storage address%u-->>Physical block%u\n", ptr_pageTabIt->auxAddr, blockNum);
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
	printf("write back success：Physical block%u-->>Auxiliary storage address%03X\n", ptr_pageTabIt->blockNum, ptr_pageTabIt->auxAddr);
}

/* 错误处理 */
void do_error(ERROR_CODE code)
{
	switch (code)
	{
		case ERROR_READ_DENY:
		{
			printf("Fetch failed: the address is unreadable\n");
			break;
		}
		case ERROR_WRITE_DENY:
		{
			printf("Fetch failed: the contents of the address is not writable\n");
			break;
		}
		case ERROR_EXECUTE_DENY:
		{
			printf("Fetch failed: the contents of the address unenforceable\n");
			break;
		}
		case ERROR_INVALID_REQUEST:
		{
			printf("Fetch failed: Illegal memory access request\n");
			break;
		}
		case ERROR_OVER_BOUNDARY:
		{
			printf("Fetch failed: address transboundary\n");
			break;
		}
		case ERROR_PROCESS_DENY:
		{
			printf("Fetch failed: the address of the current process does not have permission to access\n");
			break;
		}
		case ERROR_FILE_OPEN_FAILED:
		{
			printf("System Error: Failed to open file\n");
			break;
		}
		case ERROR_FILE_CLOSE_FAILED:
		{
			printf("System Error: Failed to close file\n");
			break;
		}
		case ERROR_FILE_SEEK_FAILED:
		{
			printf("System Error: The file pointer is positioned fail\n");
			break;
		}
		case ERROR_FILE_READ_FAILED:
		{
			printf("System Error: Failed in reading file\n");
			break;
		}
		case ERROR_FILE_WRITE_FAILED:
		{
			printf("System Error: Failed to write file\n");
			break;
		}
		default:
		{
			printf("Unknown error: This error code is not\n");
		}
	}
}

/* 产生访存请求 */
void do_request()
{
	/* 随机产生请求地址 */
	unsigned fpagenum,npagenum,offAddr;

	fpagenum = rand() % 4;
	npagenum = rand() % 16;
	offAddr = rand() % 4;

	ptr_memAccReq->virAddr = fpagenum*FPage_SIZE+npagenum*NPage_SIZE+offAddr;
	ptr_memAccReq->ProcessNum=rand()%Process_SIZE;
	/* 随机产生请求类型 */
	switch (rand() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n地址：%u\t类型：读取\t进程号：%u\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum);
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = (rand() % 0x60u)+0x20u;
			printf("产生请求：\n地址：%u\t类型：写入\t值：%02X\t进程号：%u\n", ptr_memAccReq->virAddr, ptr_memAccReq->value,ptr_memAccReq->ProcessNum);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n地址：%u\t类型：执行\t进程号：%u\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum);
			break;
		}
		default:
			break;
	}
}

/***************************************手动输入请求*********************************************/
void in_request(){
	int i;
	/*产生地址*/
	fflush(stdin);
	printf("请输入请求地址：\n");
	scanf("%d",&i);
	if((i>=VIRTUAL_MEMORY_SIZE)||(i<0)){
		printf("地址超出范围。\n");
		fflush(stdin);
		return;
	}
	ptr_memAccReq->virAddr=i;
	/*请求进程*/
	printf("请输入请求进程号：（0-%d）\n",Process_SIZE-1);
	scanf("%d",&i);
	if(i>=0&&i<Process_SIZE)
    {
        ptr_memAccReq->ProcessNum=i;
    }
    else
    {
        printf("进程号不在范围内\n");
			return;
    }
	/*请求类型*/
	printf("请输入请求类型：（0为读请求，1为写请求，2为执行请求）\n");
	scanf("%d",&i);
	if(i==0){
		ptr_memAccReq->reqType = REQUEST_READ;
		printf("产生请求：\n地址：%u\t进程号：%d\t类型：读取\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum);

	}
	else if(i==1){
		ptr_memAccReq->reqType = REQUEST_WRITE;
		printf("请输入写入数据，有且仅有一个字符\n");
		char input;
		fflush(stdin);
		scanf("%c",&input);
		fflush(stdin);
		if((i<0x20u)||(i>=0x80u)){
			printf("写入数据错误\n");
			return;
		}
		ptr_memAccReq->value = input;
		printf("产生请求：\n地址：%u\t进程号：%d\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum, ptr_memAccReq->value);
	}
	else if(i==2){
		ptr_memAccReq->reqType = REQUEST_EXECUTE;
		printf("产生请求：\n地址：%u\t进程号：%d\t类型：执行\n", ptr_memAccReq->virAddr,ptr_memAccReq->ProcessNum);
	}
	else{
		printf("请求类型错误！！！\n");
	}
	fflush(stdin);
}

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];
	char str1[Process_SIZE+1];
	printf("one level number\ttwo level number\tblock number\tLoad\tmodify\tprotection\tcount\tprocess number\tauxiliary storage\n");
	for(i=0;i<FPageNum;i++){
		for(j=0;j<(PAGE_SUM/FPageNum);j++){
			printf("%u\t%u\t%u\t%u\t%u\t%s\t%u\t%s\t%u\n", i,j, pageTable[i*16+j].blockNum, pageTable[i*16+j].filled,
			pageTable[i*16+j].edited, get_proType_str(str, pageTable[i*16+j].proType),
			pageTable[i*16+j].count, get_processNum_str(str1,pageTable[i*16+j]),pageTable[i*16+j].auxAddr);
		}
	}
/*	for (i = 0; i < PAGE_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\n", i, pageTable[i].blockNum, pageTable[i].filled,
			pageTable[i].edited, get_proType_str(str, pageTable[i].proType),
			pageTable[i].count, pageTable[i].auxAddr);
	}*/
}
/********************打印实存*****************/
void do_print_actMem(){
	int i;
	for(i=0;i<ACTUAL_MEMORY_SIZE;i++){
		printf("%d:   %c\n",i,actMem[i] );
	}
}
/*******************打印辅储****************/
void do_print_auxMem(){
    int i;
    char c;
    FILE *ptr=head_auxMem;
    rewind(ptr);
	for(i=0;i<VIRTUAL_MEMORY_SIZE;i++){
        c=fgetc(ptr);
		printf("%d:   %c\n",i,c );
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
/********************获取进程号字符串*****************/
char *get_processNum_str(char *str,PageTableItem page)
{
	int i;
	for(i=0;i<Process_SIZE;i++)
    {
        if (page.processNum[i]==TRUE)
            str[i] = i+48;
        else
            str[i] = '-';
    }
	str[Process_SIZE] = '\0';
	return str;
}

int main(int argc, char* argv[])
{
	char c[10000];
	int i, j;
	FILE *fd;
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
    head_auxMem=ptr_auxMem;
	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	while((j = read(fd,ptr_memAccReq,sizeof(MemoryAccessRequest)))<=0){
			close(fd);
			if((fd=open("/tmp/mypipe",O_RDONLY|O_NONBLOCK))<0)
				printf("stat open fifo failed\n");

		}
	char strm[3]="m\n\0";
		char strp[3]="p\n\0";
		char stra[3]="a\n\0";
		char strx[3]="x\n\0";
		char stre[3]="e\n\0";
	do
	{

		fflush(stdin);
		while(j <= 0){
				close(fd);
				if((fd=open("/tmp/mypipe",O_RDONLY|O_NONBLOCK))<0)
					printf("stat open fifo failed\n");
				j = read(fd,ptr_memAccReq,sizeof(MemoryAccessRequest));
        }
        do_responsemult();
		char getc;
		int i=0;
		printf("Press P to print the page table, press A print existential, press X print secondary memory, press E to exit\n");
		//while((getc = getchar()) == '\n');
		while(TRUE)
        {
            getc=getchar();
            if(getc=='\n')
                break;
            c[i]=getc;
            i++;

        }
        c[i]='\n';

		if(strcmp(c,strp)==0){
		    do_print_info();
		}
        else if(strcmp(c,stra)==0){
		    do_print_actMem();
		}
        else if(strcmp(c,strx)==0){
		    do_print_auxMem();
		}
		else if(strcmp(c,stre)==0){
		    break;
		}
    }while((j = (read(fd,ptr_memAccReq,sizeof(MemoryAccessRequest))))>=0);
        close(fd);
		//sleep(5000);
	

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}
