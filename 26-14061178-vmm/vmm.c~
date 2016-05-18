#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vmm.h"

#define LV1_PAGE_SUM 8
#define LV2_PAGE_SUM 8
#define VIRTUAL_PROGRESSES 2

/* 页表 */
PageTableItem pageTableface[VIRTUAL_PROGRESSES][LV1_PAGE_SUM];
PageTableItem pageTable[VIRTUAL_PROGRESSES][LV1_PAGE_SUM][LV2_PAGE_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/*物理页计数器*/
int actmemcount[VIRTUAL_PROGRESSES][PAGE_SUM][8];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

int whichprogress[BLOCK_SUM];
void do_LRU(Ptr_PageTableItem ptr_pageTabIt);
/* 初始化环境 */
void do_init()
{
	int i, j,k;
	srand(time(NULL));
	for(i=0;i<BLOCK_SUM;i++)
	{
	    whichprogress[i]=rand()%VIRTUAL_PROGRESSES;
	}
	for(j=0;j<VIRTUAL_PROGRESSES;j++)
	for (i = 0; i < LV1_PAGE_SUM; i++)
	{
		pageTableface[j][i].pageNum = i;                                             
		pageTableface[j][i].filled = FALSE;
		pageTableface[j][i].edited = FALSE;
		pageTableface[j][i].count = 0;
		/* 使用随机数设置该页的保护类型 */
		/*switch (rand() % 7)
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
		}*/
		/* 设置该页对应的辅存地址 */
		//pageTable[i].auxAddr = i * PAGE_SIZE * 2;
		//pageTable[i].progressNum = rand() % VIRTUAL_PROGRESSES;
	}
	for(k=0;k<VIRTUAL_PROGRESSES;k++)
	for (i = 0; i < LV1_PAGE_SUM; i++){
		for (j = 0; j < LV2_PAGE_SUM; j++){
			pageTable[k][i][j].pageNum = i * LV1_PAGE_SUM + j;
			pageTable[k][i][j].filled = FALSE;
			pageTable[k][i][j].edited = FALSE;
			pageTable[k][i][j].count = 0;
			switch (rand() % 7)
			{
				case 0:
				{
					pageTable[k][i][j].proType = READABLE;
					break;
				}
				case 1:
				{
					pageTable[k][i][j].proType = WRITABLE;
					break;
				}
				case 2:
				{
					pageTable[k][i][j].proType = EXECUTABLE;
					break;
				}
				case 3:
				{
					pageTable[k][i][j].proType = READABLE | WRITABLE;
					break;
				}
				case 4:
				{
					pageTable[k][i][j].proType = READABLE | EXECUTABLE;
					break;
				}
				case 5:
				{
					pageTable[k][i][j].proType = WRITABLE | EXECUTABLE;
					break;
				}
				case 6:
				{
					pageTable[k][i][j].proType = READABLE | WRITABLE | EXECUTABLE;
					break;
				}
				default:
					break;
			}
			pageTable[k][i][j].auxAddr = (i * LV1_PAGE_SIZE + j * LV2_PAGE_SIZE) ;
			//pageTable[k][i][j].progressNum = rand() % VIRTUAL_PROGRESSES;
		}
	}

	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (rand() % 2 == 0)
		{
			do_page_in(&pageTable[whichprogress[j]][j / LV1_PAGE_SUM][j % LV1_PAGE_SUM], j);                                   
			pageTable[whichprogress[j]][j / LV1_PAGE_SUM][j % LV1_PAGE_SUM].blockNum = j;
			pageTable[whichprogress[j]][j / LV1_PAGE_SUM][j % LV1_PAGE_SUM].filled = TRUE;
			blockStatus[j] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
	for(i=0;i<VIRTUAL_PROGRESSES;i++)
	for (j = 0; j < PAGE_SUM; j++)
	{
	    for(k=0;k<8;k++)
	    {
	        actmemcount[i][j][k]=0;
	    }
	}
}


/* 响应请求 */
void do_response()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum1, pageNum2, offAddr;
	unsigned int actAddr,i;

	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}

	/* 计算页号和页内偏移值 */
	pageNum1 = ptr_memAccReq->virAddr / LV1_PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % LV1_PAGE_SIZE;
	pageNum2 = offAddr / LV2_PAGE_SIZE;
	offAddr = offAddr % LV2_PAGE_SIZE;
	printf("页号为：%u\t页内偏移为：%u\n", pageNum1 * LV1_PAGE_SUM + pageNum2, offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = &pageTable[ptr_memAccReq->FromProgress][pageNum1][pageNum2];                                

	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
	    for(i=0;i<8;i++)
	    {
	        actmemcount[ptr_memAccReq->FromProgress][pageNum1 * LV1_PAGE_SUM + pageNum2][i]=0;
	    }
		do_page_fault(ptr_pageTabIt);
	}
  //  if(ptr_memAccReq->FromProgress != ptr_pageTabIt->progressNum)
 //   {
 //       for(i=0;i<8;i++)
//	    {
//	        actmemcount[pageNum1 * LV1_PAGE_SUM + pageNum2][i]=0;
//	    }
//		do_page_fault(ptr_pageTabIt);
//    }
	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			actmemcount[ptr_memAccReq->FromProgress][pageNum1 * LV1_PAGE_SUM + pageNum2][0]=1;
			/* 检查页面访问权限并处理访存请求 */
			/*if(ptr_memAccReq->FromProgress != ptr_pageTabIt->progressNum){
                printf("%d %d",ptr_memAccReq->FromProgress,ptr_pageTabIt->progressNum);
                do_error(ERROR_UNMATCHED_PROGRESS);
                return;
            }*/
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
			actmemcount[ptr_memAccReq->FromProgress][pageNum1 * LV1_PAGE_SUM + pageNum2][0]=1;
			/* 检查页面访问权限并处理访存请求 */
			/*if(ptr_memAccReq->FromProgress != ptr_pageTabIt->progressNum){
                printf("%d %d",ptr_memAccReq->FromProgress,ptr_pageTabIt->progressNum);
                do_error(ERROR_UNMATCHED_PROGRESS);
                return;
            }*/
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
			actmemcount[ptr_memAccReq->FromProgress][pageNum1 * LV1_PAGE_SUM + pageNum2][0]=1;
			/* 检查页面访问权限并处理访存请求 */
			/*if(ptr_memAccReq->FromProgress != ptr_pageTabIt->progressNum){
                printf("%d %d",ptr_memAccReq->FromProgress,ptr_pageTabIt->progressNum);
                do_error(ERROR_UNMATCHED_PROGRESS);
                return;
            }*/
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
		//	ptr_pageTabIt->progressNum=ptr_memAccReq->FromProgress;

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
	unsigned int i, j, k,min, firstpage, page;
	int whichprogress=0;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for(k=0;k<VIRTUAL_PROGRESSES;k++)
	for (i = 0, min = 0xFFFFFFFF, firstpage = 0, page = 0; i < LV1_PAGE_SUM; i++)
	{
		for (j = 0; j < LV2_PAGE_SUM; j++)
		{
			if (pageTable[k][i][j].count < min)                                                
			{
			    whichprogress=k;
				min = pageTable[k][i][j].count;
				firstpage = i;
				page = j;
			}
		}
	}
	printf("选择第%u页进行替换\n", firstpage * LV1_PAGE_SUM + page);
	if (pageTable[whichprogress][firstpage][page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[whichprogress][firstpage][page]);
	}
	pageTable[whichprogress][firstpage][page].filled = FALSE;
	pageTable[whichprogress][firstpage][page].count = 0;
    for(i=0;i<8;i++)
    {
        actmemcount[whichprogress][firstpage * LV1_PAGE_SUM + page][i]=0;
    }


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[whichprogress][firstpage][page].blockNum);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[whichprogress][firstpage][page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
}
void do_LRU(Ptr_PageTableItem ptr_pageTabIt)
{
    unsigned int i,j,k,min,page,firstpage;
    int flag=0;
    int count[VIRTUAL_PROGRESSES][PAGE_SUM]={0};
    int Whichprogress=0;
    for(k=0;k<VIRTUAL_PROGRESSES;k++)
    for(i=0;i<PAGE_SUM;i++)
    {
        for(j=0;j<8;j++)
        count[k][i]=count[k][i]*2+actmemcount[k][i][j];
        //printf("%d\n",count[i]);
    }
    printf("没有空闲物理块，开始进行LRU页面替换...\n");
    for(k=0;k<VIRTUAL_PROGRESSES;k++)
    for (i = 0, firstpage = 0, page = 0; i < LV1_PAGE_SUM; i++)
	{
		for (j = 0; j < LV2_PAGE_SUM; j++)
		{
			if (pageTable[k][i][j].filled)                                                  
			{
			    Whichprogress=k;
				firstpage = i;
				page = j;
				flag=1;
			}
			break;
		}
		if(flag==1)
        {
            break;
        }
	}
	for(k=0;k<VIRTUAL_PROGRESSES;k++)
    for (i = 0, min = 0xFFFFFFFF; i < LV1_PAGE_SUM; i++)
	{
		for (j = 0; j < LV2_PAGE_SUM; j++)
		{
			if (count[k][i*LV1_PAGE_SUM+j] < min
            &&pageTable[k][i][j].filled)                                                   
			{
			    Whichprogress=k;
				min = count[k][i*LV1_PAGE_SUM+j];
				firstpage = i;
				page = j;
			}
		}
	}
    printf("选择第%u个进程页表的第%u页进行替换\n", k, firstpage * LV1_PAGE_SUM + page);
    if (pageTable[Whichprogress][firstpage][page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[Whichprogress][firstpage][page]);
	}
	for(i=0;i<8;i++)
	{
	    actmemcount[Whichprogress][firstpage * LV1_PAGE_SUM + page][i]=0;
	}
	whichprogress[pageTable[Whichprogress][firstpage][page].blockNum]=ptr_memAccReq->FromProgress;
	pageTable[Whichprogress][firstpage][page].filled = FALSE;
	pageTable[Whichprogress][firstpage][page].count = 0;
	pageTable[Whichprogress][firstpage][page].blockNum = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[Whichprogress][firstpage][page].blockNum);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[Whichprogress][firstpage][page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	//ptr_pageTabIt->progressNum=ptr_memAccReq->FromProgress;
	printf("页面替换成功\n");
}
/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
//#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
//#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	//printf("auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
	if ((readNum = fread(actMem + blockNum * PAGE_SIZE,
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
//#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
		printf("JIBA: sizeof(BYTE)*PAGE_SIZE=%d\n", sizeof(BYTE)*PAGE_SIZE);
//#endif
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
		case ERROR_UNMATCHED_PROGRESS:
		{
			printf("访存失败：访存进程与页面所属于进程不匹配\n");
			break;
		}
		case ERROR_ILLEGAL_INPUT:
		{
			printf("请求读入错误：非法的请求的输入");
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
	int i,j,k;
	ptr_memAccReq->virAddr = rand() % VIRTUAL_MEMORY_SIZE;
	ptr_memAccReq->FromProgress = rand() % VIRTUAL_PROGRESSES;
	/* 随机产生请求类型 */
	switch (rand() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n地址：%u\t类型：读取\t 所属进程： %u\n", ptr_memAccReq->virAddr, ptr_memAccReq->FromProgress);
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = rand() % 0xFFu;
			printf("产生请求：\n地址：%u\t类型：写入\t值：%02X\t 所属进程： %u\n", ptr_memAccReq->virAddr, ptr_memAccReq->value, ptr_memAccReq->FromProgress);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n地址：%u\t类型：执行\t 所属进程： %u\n", ptr_memAccReq->virAddr, ptr_memAccReq->FromProgress);
			break;
		}
		default:
			break;
	}
	for(k=0;k<VIRTUAL_PROGRESSES;k++)
	for (i= 0; i < PAGE_SUM; i++)
	{
	    for(j=7;j>0;j--)
	    {
	        actmemcount[k][i][j]=actmemcount[k][i][j-1];
	    }
	    actmemcount[k][i][0]=0;
	}
}

int IsNum(char c){
	if(c >= '0' && c <= '9'){
		return 1;
	}
	return 0;
}

int IsAlpha(char c){
	if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')){
		return 1;
	}
	return 0;
}

int IsNumorAlpha(char c){
	if(IsAlpha(c) || IsNum(c)){
		return 1;
	}
	return 0;
}

/*int DealWithInput(char *str){
	int pointer, state, pointert;
	int Virtual_Address = -1;
	int Progress = -1;
	char TypeOfRequest;
	unsigned char WriteData;
	char temp[100];
	for(pointer = 0, state = 0, pointert = 0; str[pointer] != '\n'; pointer++){
	
	}
}*/

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k,l;
	char str[4];

	for(l=0;l<VIRTUAL_PROGRESSES;l++)
	{
	    printf("第%d号进程\n",l);
	    printf("页号\t块号\t装入\t修改\t保护\t计数\t辅存\t老化计数器\n");
	    for (i = 0; i < LV1_PAGE_SUM; i++)
        {
            for(j = 0; j < LV2_PAGE_SUM; j++)
            {
                printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\t", i * LV1_PAGE_SUM + j, pageTable[l][i][j].blockNum, pageTable[l][i][j].filled,                           //replace5
                pageTable[l][i][j].edited, get_proType_str(str, pageTable[l][i][j].proType),
                pageTable[l][i][j].count, pageTable[l][i][j].auxAddr);
                for(k=0;k<8;k++)
                    printf("%d",actmemcount[l][i*LV1_PAGE_SUM+j][k]);
                putchar('\n');
            }
        }
	}

}

int do_input_request(){
	int Virtual_Address;
	int Progress;
	char Type;
	unsigned char content;
	printf("请输入合法要求 格式为 “地址 进程号 访问类型 写入数据（仅对写操作有效）\n格式的合法范围为0-255\n进程号的合法范围为0-1\n访问类型为：r(读)、w(写)、x(执行)\n写入数据仅在写时有效");
	scanf("%d %d %c %c", &Virtual_Address, &Progress, &Type, &content);
	if(Virtual_Address < 0 || Virtual_Address > VIRTUAL_MEMORY_SIZE){
		do_error(ERROR_ILLEGAL_INPUT);
		return 0;
	}
	if(Progress < 0 || Progress > VIRTUAL_PROGRESSES){
		do_error(ERROR_ILLEGAL_INPUT);
		return 0;
	}
	if(Type != 'w' && Type != 'r' && Type != 'x'){
		do_error(ERROR_ILLEGAL_INPUT);
		return 0;
	}
	switch(Type){
		case 'w':
			ptr_memAccReq->FromProgress = Progress;
			ptr_memAccReq->virAddr = Virtual_Address;
			ptr_memAccReq->reqType = REQUEST_WRITE;
			printf("读入请求：\n地址：%u\t类型：写入\t值：%02X\t 所属进程： %u\n", ptr_memAccReq->virAddr, ptr_memAccReq->value, ptr_memAccReq->FromProgress);
			ptr_memAccReq->value = content;
			break;
		case 'r':
			ptr_memAccReq->FromProgress = Progress;
			ptr_memAccReq->virAddr = Virtual_Address;
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("读入请求：\n地址：%u\t类型：读取\t 所属进程： %u\n", ptr_memAccReq->virAddr, ptr_memAccReq->FromProgress);
			break;
		case 'x':
			ptr_memAccReq->FromProgress = Progress;
			ptr_memAccReq->virAddr = Virtual_Address;
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("读入请求：\n地址：%u\t类型：执行\t 所属进程： %u\n", ptr_memAccReq->virAddr, ptr_memAccReq->FromProgress);
			break;
		default:
			do_error(ERROR_ILLEGAL_INPUT);
			break;
	}
	return 1;
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
	int i;
	char c;
	int flag = 0;
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
		do_request();
loop:	do_response();
		printf("按Y打印页表，按其他键不打印...\n");
loop1:	if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info();
		while (c != '\n')
			c = getchar();
		if(flag == 1){
			flag = 0;
			goto loop1;
		}
		printf("按X退出程序，按I手动输入请求，按其他键继续...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		if (c == 'i' || c == 'I'){
			flag = do_input_request();
			if(flag == 1)
				goto loop;
			printf("1111\n");
		}
		while (c != '\n'){
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
