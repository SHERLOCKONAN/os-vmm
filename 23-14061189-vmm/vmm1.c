#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#include "vmm.h"



/* 页表 */
PageTableItem pageTable[4][SecondPAGE_SUM][PAGE_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
FILE *fp;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;



/* 初始化环境 */
void do_init()
{
	unsigned int i, j, k,n;
	srand(time(NULL));
	for(n=0;n<4;n++){
	for (k = 0; k<SecondPAGE_SUM;k++){
		for (i = 0; i < PAGE_SUM; i++)
		{
			pageTable[n][k][i].pageNum = i;
			pageTable[n][k][i].filled = FALSE;
			pageTable[n][k][i].edited = FALSE;
			pageTable[n][k][i].count = 0;
			for(j=0;j<8;j++)
			{
				pageTable[n][k][i].LRUcount[j]=0;
			}
			pageTable[n][k][i].processNum=n;
			pageTable[n][k][i].visited=0;

			/* 使用随机数设置该页的保护类型 */
			switch (rand() % 7)
			{
				case 0:
				{
					pageTable[n][k][i].proType = READABLE;
					break;
				}
				case 1:
				{
					pageTable[n][k][i].proType = WRITABLE;
					break;
				}
				case 2:
				{
					pageTable[n][k][i].proType = EXECUTABLE;
					break;
				}
				case 3:
				{
					pageTable[n][k][i].proType = READABLE | WRITABLE;
					break;
				}
				case 4:
				{
					pageTable[n][k][i].proType = READABLE | EXECUTABLE;
					break;
				}
				case 5:
				{
					pageTable[n][k][i].proType = WRITABLE | EXECUTABLE;
					break;
				}
				case 6:
				{
					pageTable[n][k][i].proType = READABLE | WRITABLE | EXECUTABLE;
					break;
				}
				default:
					break;
			}
			/* 设置该页对应的辅存地址 */
			pageTable[n][k][i].auxAddr =(k*8+ i) * PAGE_SIZE * 2;

		}
	}
	}
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (rand() % 2 == 0)
		{
			do_page_in(&pageTable[j%4][j/8][j%8], j);
			pageTable[j%4][j/8][j%8].blockNum = j;
			pageTable[j%4][j/8][j%8].filled = TRUE;
			blockStatus[j] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
}


/* 响应请求 */
void do_response(int processNum)//响应请求的时候标记他最近被访问过
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum1, pageNum2,offAddr;
	unsigned int actAddr;
	
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* 计算页号和页内偏移值 */
	pageNum1 = (ptr_memAccReq->virAddr / PAGE_SIZE)/SecondPAGE_SUM;//页表目录号
	pageNum2 = (ptr_memAccReq->virAddr / PAGE_SIZE)%SecondPAGE_SUM;//页表项
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("进程号：%d\t页目录号为：%u\t页号为：%u\t页内偏移为：%u\n", processNum+1,pageNum1, pageNum2,offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = &pageTable[processNum][pageNum1][pageNum2];
	
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt,processNum);
	}
	
	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);
	
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->visited=1;
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%02x\n", actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->visited=1;
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);	
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;			
			printf("写操作成功,写入值为%02x\n",ptr_memAccReq->value);
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->visited=1;
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
void do_page_fault(Ptr_PageTableItem ptr_pageTabIt,int processNum)
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
	do_LRU(ptr_pageTabIt,processNum);
}

/* 根据LRU算法进行页面替换 */
void do_LRU(Ptr_PageTableItem ptr_pageTabIt,int processNum)
{
	unsigned int i, j,k,min, page1,page2;
	unsigned int Temp=0;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for (k = 0; k < SecondPAGE_SUM; k++){
		for (i = 0, min = 0xFFFFFFFF, page1 = 0,page2=0; i < PAGE_SUM; i++)
		{
			for(j=0;j<8;j++){
				Temp=10*Temp+pageTable[processNum][k][i].LRUcount[j];
			}
			if (Temp < min)
			{
				min = Temp;
				page1 = k;
				page2 = i;
			}
		}
	}
	printf("选择第%u页进行替换\n", page1*SecondPAGE_SUM+i);
	if (pageTable[processNum][page1][page2].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[processNum][page1][page2]);
	}
	pageTable[processNum][page1][page2].filled = FALSE;
	for(j=0;j<8;j++)
	{
		pageTable[processNum][page1][page2].LRUcount[j]=0;
	}



	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[processNum][page1][page2].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[processNum][page1][page2].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	for(j=0;j<8;j++)
	{
		ptr_pageTabIt->LRUcount[j]=0;
	}
	printf("页面替换成功\n");
}

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
	printf("调页成功：进程号%d 辅存地址%u-->>物理块%u\n", ptr_pageTabIt->processNum,ptr_pageTabIt->auxAddr, blockNum);
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
	printf("写回成功：物理块%u-->>进程号%d 辅存地址%03X\n", ptr_pageTabIt->auxAddr,ptr_pageTabIt->processNum, ptr_pageTabIt->blockNum);
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
void do_print_info(int processNum)
{
	unsigned int i, k;
	char str[4];
	printf("页号\t块号\t装入\t修改\t保护\t计数\t辅存\n");
	for(k = 0; k< SecondPAGE_SUM;k++){
	for (i = 0; i < PAGE_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\n", k*8+i, pageTable[processNum][k][i].blockNum, pageTable[processNum][k][i].filled, 
			pageTable[processNum][k][i].edited, get_proType_str(str, pageTable[processNum][k][i].proType), 
			pageTable[processNum][k][i].count, pageTable[processNum][k][i].auxAddr);
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
	char Request[20]={'\0'};
	char buffer[5]={'\0'};
	char CurrentRequest[20]={'\0'};
	int i,j,k,n,underlinecount;
	int Requestcount=0;
	int virAddr,reqType;
	int value;
	int processNum=1;
	int RequestCount=0;//用来每处理一定的请求就根据页表的脏位来修改LRUcount;
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY1, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}

	do_init();
	do_print_info(1);

	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		virAddr=0;
		reqType=0;
		value=0;
		processNum=0;
		Requestcount++;

		if(!(fp = fopen("MYFIFO","r"))){
			do_error(ERROR_FILE_OPEN_FAILED);
			exit(1);
		}
		for(n=1;n<=Requestcount;n++){
			if(fgets(CurrentRequest,256,fp)==NULL){
				printf("系统错误：读入文件失败\n");
				break;
			}
		}
		fclose(fp);
		printf("请求读取完成:%s\n",CurrentRequest);
		underlinecount=0;
		for(j=0;j<strlen(CurrentRequest)-1;j++){
			if(CurrentRequest[j]=='_'){
				underlinecount++;
			}
			else{
				if(underlinecount==0){
					virAddr=10*virAddr+CurrentRequest[j]-'0';
				}
				else if(underlinecount==1){
					reqType=10*reqType+CurrentRequest[j]-'0';
				}
				else if(underlinecount==2){
					value=10*value+CurrentRequest[j]-'0';
				}
				else{
					processNum=10*processNum+CurrentRequest[j]-'0';
				}
			}
		}
		printf("%d %d %d %d",virAddr,reqType,value,processNum);//模拟手动添加请求
		ptr_memAccReq->virAddr=virAddr;
		ptr_memAccReq->reqType=reqType;
		ptr_memAccReq->value=value;
		ptr_memAccReq->processNum=processNum;
		if(processNum==1){
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY1, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
		else if(processNum==2){
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY2, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
		else if(processNum==3){			
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY3, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
		else if(processNum=4){
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY4, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
		do_response(processNum);
		RequestCount++;
		printf("按Y打印页表，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info(processNum);
		while (c != '\n')
			c = getchar();
		printf("按E打印实存，按其他键不打印...\n");
		if ((c = getchar()) == 'E' || c == 'e')
		{
			for(i=0;i<ACTUAL_MEMORY_SIZE;i=i+4){
				printf("物理块号：%d:%c%c%c%c\n",i/4,actMem[i],actMem[i+1],actMem[i+2],actMem[i+3]);
			}
		}
		while (c != '\n')
			c = getchar();
		printf("按R打印辅存，按其他键不打印...\n");
		if ((c = getchar()) == 'R' || c == 'r')
		{
			for(i=0;i<VIRTUAL_MEMORY_SIZE;i=i+4){
				fread(buffer,sizeof(BYTE),4,ptr_auxMem);
				buffer[4]='\0';
				printf("辅存块号：%d:%s\n",i/4,buffer);
			}
		}
		while (c != '\n')
			c = getchar();
		printf("按X退出程序，按其他键继续...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		while (c != '\n')
			c = getchar();
		if(RequestCount==100){
			RequestCount=0;
			for(n=0;n<4;n++){
			for(k=0;k<SecondPAGE_SUM;k++){
				for(i=0;i<PAGE_SUM;i++){
					for(j=7;j>0;j++){
						pageTable[n][k][i].LRUcount[j]=pageTable[n][k][i].LRUcount[j-1];
						}
					pageTable[n][k][i].LRUcount[0]=pageTable[n][k][i].visited;
					}
				}
			}
		}
		}

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}