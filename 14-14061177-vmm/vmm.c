#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vmm.h"
/* 页目录 */
PageIndexItem pageIndex[Process_SUM][INDEX_SUM];
/* 页表 */
//PageTableItem pageTable[PAGE_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq,head=NULL;



/* 初始化环境 */
void do_init()
{
	int i, j,k,r,q,processCount,c;
	srandom(time(NULL));
	for(processCount=0;processCount<Process_SUM;processCount++)
	for (k = 0; k < INDEX_SUM; k++)
	for(i=0;i<INDEX_PAGE;i++)
	{
		pageIndex[processCount][k].processNum=processCount;
		pageIndex[processCount][k].indexNum=k;
		pageIndex[processCount][k].index[i].pageNum = i;
		pageIndex[processCount][k].index[i].filled = FALSE;
		pageIndex[processCount][k].index[i].edited = FALSE;
		pageIndex[processCount][k].index[i].visited = 0;
		for(c = 0;c < 8;c++){
            pageIndex[processCount][k].index[i].count[c] = '0';
		}
		/* 使用随机数设置该页的保护类型 */
		switch (random() % 7)
		{
			case 0:
			{
				pageIndex[processCount][k].index[i].proType = READABLE;
				//pageTable[i].proType = READABLE;
				break;
			}
			case 1:
			{
				pageIndex[processCount][k].index[i].proType = WRITABLE;
				break;
			}
			case 2:
			{
				pageIndex[processCount][k].index[i].proType =  EXECUTABLE;
				break;
			}
			case 3:
			{
				pageIndex[processCount][k].index[i].proType= READABLE | WRITABLE;
				break;
			}
			case 4:
			{
				pageIndex[processCount][k].index[i].proType= READABLE | EXECUTABLE;
				break;
			}
			case 5:
			{
				pageIndex[processCount][k].index[i].proType= WRITABLE | EXECUTABLE;
				break;
			}
			case 6:
			{
				pageIndex[processCount][k].index[i].proType= READABLE | WRITABLE | EXECUTABLE;
				break;
			}
			default:
				break;
		}
		/* 设置该页对应的辅存地址 */
		pageIndex[processCount][k].index[i].auxAddr = processCount*256+(k*INDEX_PAGE+i)*4;
		//pageTable[i].auxAddr = i * PAGE_SIZE ;
	}
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (random() % 2 == 0)
		{
			q = j/INDEX_PAGE;
			r = j%INDEX_PAGE;
			do_page_in(&pageIndex[0][q].index[r], j);
			pageIndex[0][q].index[r].blockNum = j;
			pageIndex[0][q].index[r].filled = TRUE;
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
	unsigned int pageNum, offAddr,indexNum,ProcessNum;
	unsigned int actAddr;
	Ptr_MemoryAccessRequest aptr_memAccReq;
	int processCount, k, i, c;

	if(head==NULL){
	printf("hi\n");
		return;}
	aptr_memAccReq = head;
	head = head->next;
	ProcessNum =  aptr_memAccReq->ProcessNum;
	/* 检查地址是否越界 */
	if (aptr_memAccReq->virAddr < aptr_memAccReq->ProcessNum*256 || aptr_memAccReq->virAddr >= (aptr_memAccReq->ProcessNum+1)*256)
	{
		printf("viraddr=%d\t,processnum=%d\n",aptr_memAccReq->virAddr,ProcessNum);
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}

	/* 计算页号和页内偏移值 */

	pageNum = (aptr_memAccReq->virAddr-ProcessNum*256) / PAGE_SIZE % INDEX_PAGE ;
	indexNum = (aptr_memAccReq->virAddr-ProcessNum*256) / PAGE_SIZE / INDEX_PAGE;
	offAddr = (aptr_memAccReq->virAddr-ProcessNum*256) % PAGE_SIZE;

	printf("进程号为：%u\t页目录为：%u\t页号为：%u\t页内偏移为：%u\n",ProcessNum,indexNum, pageNum, offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = &pageIndex[ProcessNum][indexNum].index[pageNum];
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}

	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);

    for(processCount=0;processCount<Process_SUM;processCount++)
        for (k = 0; k < INDEX_SUM; k++)
            for(i=0;i<INDEX_PAGE;i++)
            {
                pageIndex[processCount][k].index[i].visited = 0;
            }

	/* 检查页面访问权限并处理访存请求 */
	switch (aptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->visited = 1;
			for(processCount=0;processCount<Process_SUM;processCount++)
                for (k = 0; k < INDEX_SUM; k++)
                    for(i=0;i<INDEX_PAGE;i++)
                    {
                        for(c = 7;c > 0;c--){
                            pageIndex[processCount][k].index[i].count[c] = pageIndex[processCount][k].index[i].count[c - 1];
                        }
                        pageIndex[processCount][k].index[i].count[0] = pageIndex[processCount][k].index[i].visited + '0';
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
			ptr_pageTabIt->visited = 1;
			for(processCount=0;processCount<Process_SUM;processCount++)
                for (k = 0; k < INDEX_SUM; k++)
                    for(i=0;i<INDEX_PAGE;i++)
                    {
                        for(c = 7;c > 0;c--){
                            pageIndex[processCount][k].index[i].count[c] = pageIndex[processCount][k].index[i].count[c - 1];
                        }
                        pageIndex[processCount][k].index[i].count[0] = pageIndex[processCount][k].index[i].visited + '0';
                    }
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = aptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;
			printf("写操作成功\n");
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->visited = 1;
			for(processCount=0;processCount<Process_SUM;processCount++)
                for (k = 0; k < INDEX_SUM; k++)
                    for(i=0;i<INDEX_PAGE;i++)
                    {
                        for(c = 7;c > 0;c--){
                            pageIndex[processCount][k].index[i].count[c] = pageIndex[processCount][k].index[i].count[c - 1];
                        }
                        pageIndex[processCount][k].index[i].count[0] = pageIndex[processCount][k].index[i].visited + '0';
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
	unsigned int i, c;
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
			ptr_pageTabIt->visited = 0;
			for(c = 0;c < 8;c++){
				ptr_pageTabIt->count[c] = '0';
			}

			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_PA(ptr_pageTabIt);
}
/* 根据页面老化算法进行页面替换 */
void do_PA(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i,k,s, min, page,index,processNum, c, temp_count;
	printf("没有空闲物理块，开始进行页面老化算法页面替换...\n");
	for(s=0,min=0xFFFFFFFF,page=0,index=0,processNum=0;s<Process_SUM;s++)
        for(k=0;k<INDEX_SUM;k++)
            for (i = 0; i < INDEX_PAGE; i++)
            {
                for(c = 0, temp_count = 0;c < 8;c++){
                    temp_count = temp_count * 10 + pageIndex[s][k].index[i].count[c] - '0';
                }
                if (temp_count < min && pageIndex[s][k].index[i].filled==TRUE)
                {
                    min = temp_count;
                    processNum=s;
                    page = i;
                    index=k;
                }
            }
    printf("选择第%u个目录第%u页进行替换\n",index,page);
	if (pageIndex[processNum][index].index[page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageIndex[processNum][index].index[page]);
	}
	pageIndex[processNum][index].index[page].filled = FALSE;
	pageIndex[processNum][index].index[page].visited = 0;
	for(c = 0;c < 8;c++){
		pageIndex[processNum][index].index[page].count[c] = '0';
	}

	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt,pageIndex[processNum][index].index[page].blockNum);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum =pageIndex[processNum][index].index[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->visited = 0;
	for(c = 0;c < 8;c++){
		ptr_pageTabIt->count[c] = '0';
	}
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
		default:
		{
			printf("未知错误：没有这个错误代码\n");
		}
	}
}

/* 产生访存请求 */
void do_request(cmd acmd)
{
	Ptr_MemoryAccessRequest p;
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	ptr_memAccReq->virAddr = acmd.virAddr;
	ptr_memAccReq->ProcessNum = acmd.ProcessNum;
	printf("dorequest addr:=%ld\n",acmd.virAddr);
	ptr_memAccReq->reqType = acmd.reqType;
	if(acmd.reqType==REQUEST_WRITE)
		ptr_memAccReq->value = acmd.value;
	ptr_memAccReq->next = NULL;
	if(head==NULL)
		head = ptr_memAccReq;
	else{
		for(p=head;p->next!=NULL;p=p->next)
			;
		p->next = ptr_memAccReq;
	}
}

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k,s,p,c;
	char str[4];
	printf("进程号\t目录号\t页号\t块号\t装入\t修改\t保护\t计数\t\t辅存\n");
	for(p=0;p<Process_SUM;p++)
	for(s=0;s<INDEX_SUM;s++)
	for (i = 0; i <INDEX_PAGE; i++)
	{
		printf("%u\t%u\t%u\t%u\t%u\t%u\t%s\t", p,s,i, pageIndex[p][s].index[i].blockNum, pageIndex[p][s].index[i].filled,
			pageIndex[p][s].index[i].edited, get_proType_str(str, pageIndex[p][s].index[i].proType));
        for(c = 0;c < 8;c++){
			printf("%c",pageIndex[p][s].index[i].count[c]);
		}
		printf("\t%u\n", pageIndex[p][s].index[i].auxAddr);
	}
}
/*打印实存*/
void do_print_act()
{
	int i;
	printf("print actual memeory\n");
	for(i = 0;i < ACTUAL_MEMORY_SIZE;i++){
		printf("%d\t%c\n",i,actMem[i]);
	}
}
/*打印辅存*/
void do_print_vir()
{
	int i;
	char temp_byte;
	FILE* p = fopen("vmm_auxMem","r");
	printf("print virtual memory\n");
	for(i = 0;i < VIRTUAL_MEMORY_SIZE;i++){
        printf("%c",fgetc(p));
	}
	/*while((temp_byte = fgetc(p)) != EOF){
		printf("%c",temp_byte);
	}*/

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
	int i;
	int fifo2;
	int count;
	cmd req;
	struct stat statbuf;
	req.RT=NOR;
	if(stat("/tmp/doreq",&statbuf)==0){
		/* 如果FIFO文件存在,删掉 */
		if(remove("/tmp/doreq")<0)
			printf("remove failed\n");
	}

	if(mkfifo("/tmp/doreq",0666)<0)
		printf("mkfifo failed\n");
	/* 在非阻塞模式下打开FIFO */
	if((fifo2=open("/tmp/doreq",O_RDONLY|O_NONBLOCK))<0)
		printf("open fifo failed\n");

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
		if((count=read(fifo2,&req,CMDLEN))<0)
			printf("read fifo failed\n");
		switch(req.RT){
			case REQUEST: printf("addr=%ld\n",req.virAddr);
			do_request(req);req.RT=NOR;
			break;
			//case RESPONSE:printf("aaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n");
			//do_response();req.RT=NOR;
			//break;
			default:
			break;
		}
		do_response();
		printf("按Y打印页表，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info();
		while (c != '\n')
			c = getchar();
        printf("按A打印实存，按其他键不打印...\n");
		if ((c = getchar()) == 'a' || c == 'A')
			do_print_act();
		while (c != '\n')
			c = getchar();
        printf("按V打印辅存，按其他键不打印...\n");
		if ((c = getchar()) == 'v' || c == 'V')
			do_print_vir();
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
	close(fifo2);
	return (0);
}
