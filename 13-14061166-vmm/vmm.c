#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/stat.h>
#include "vmm.h"

/* 页表 */
PageTableItem pageTable[PAGE_SUM2][PAGE_SUM1];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;




/* 初始化环境 */
void do_init()
{
	int i, j ,k;
	srand(time(NULL));
	for(k=0;k<PAGE_SUM2;k++){
        for (i = 0; i < PAGE_SUM1; i++)
        {
            pageTable[k][i].pageNum1 = i;
            pageTable[k][i].pageNum2 = k;
            pageTable[k][i].filled = FALSE;
            pageTable[k][i].edited = FALSE;
            pageTable[k][i].count = 0;
            /* 使用随机数设置该页的保护类型 */
            switch (rand() % 7)
            {
                    case 0:
                {
                    pageTable[k][i].proType = READABLE;
                    break;
                }
                case 1:
                {
                    pageTable[k][i].proType = WRITABLE;
                    break;
                }
                case 2:
                {
                    pageTable[k][i].proType = EXECUTABLE;
                    break;
                }
                case 3:
                {
                    pageTable[k][i].proType = READABLE | WRITABLE;
                    break;
                }
                case 4:
                {
                    pageTable[k][i].proType = READABLE | EXECUTABLE;
                    break;
                }
                case 5:
                {
                    pageTable[k][i].proType = WRITABLE | EXECUTABLE;
                    break;
                }
                case 6:
                {
                    pageTable[k][i].proType = READABLE | WRITABLE | EXECUTABLE;
                    break;
                }
                default:
                    break;
            }
		/* 设置该页对应的辅存地址 */
            pageTable[k][i].processNum = rand() % 3;
            pageTable[k][i].auxAddr = (i+k*8)* PAGE_SIZE ;                                     //修改一：将*2去掉
            pageTable[k][i].blockNum = 500;                                               //修改三：将未被标记的改为500
        }
	}
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (rand() % 2 == 0)
		{
			do_page_in(&pageTable[j/8][j%8], j);
			pageTable[j/8][j%8].blockNum = j;
			pageTable[j/8][j%8].filled = TRUE;
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
	unsigned int pageNum1, offAddr ,pageNum2;
	unsigned int actAddr;
    int i,k;

	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}

	/* 计算页号和页内偏移值 */
	pageNum2 = ptr_memAccReq->virAddr / (PAGE_SIZE*PAGE_SUM1);
	pageNum1 = (ptr_memAccReq->virAddr - pageNum2*PAGE_SIZE*PAGE_SUM1 )/ PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("2级页号为：%u\t1级页号为: %u\t页内偏移为：%u\n", pageNum2 , pageNum1 , offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = &pageTable[pageNum2][pageNum1];
	printf("请求的进程号为：%u\t页表的进程号为:%u\n",ptr_memAccReq->reqProcess,ptr_pageTabIt->processNum);
    if(ptr_pageTabIt->processNum!=ptr_memAccReq->reqProcess){
  //      printf("%u %u\n",ptr_pageTabIt->processNum,ptr_memAccReq->reqProcess);
        do_error(ERROR_VISIT_FAILED);
    }
    else{
	/* 根据特征位决定是否产生缺页中断 */
        if (!ptr_pageTabIt->filled)
        {
            do_page_fault(ptr_pageTabIt);
        }

        actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;                         //虚实地址转换
        printf("实地址为：%u\n", actAddr);

        /* 检查页面访问权限并处理访存请求 */
        switch (ptr_memAccReq->reqType)
        {
            case REQUEST_READ: //读请求
            {
                for(k=0;k<PAGE_SUM2;k++)
                    for(i=0;i<PAGE_SUM1;i++)
                        pageTable[k][i].count=pageTable[k][i].count/10;
                ptr_pageTabIt->count=ptr_pageTabIt->count+1000000;
            //	ptr_pageTabIt->count++;
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
               for(k=0;k<PAGE_SUM2;k++)
                    for(i=0;i<PAGE_SUM1;i++)
                        pageTable[k][i].count=pageTable[k][i].count/10;
                ptr_pageTabIt->count=ptr_pageTabIt->count+1000000;
            //	ptr_pageTabIt->count++;
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
                for(k=0;k<PAGE_SUM2;k++)
                    for(i=0;i<PAGE_SUM1;i++)
                        pageTable[k][i].count=pageTable[k][i].count/10;
                ptr_pageTabIt->count=ptr_pageTabIt->count+1000000;
            //	ptr_pageTabIt->count++;
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
	do_LFU(ptr_pageTabIt);
}

/* 根据LFU算法进行页面替换 */
void do_LFU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i,k, min, page2 ,page1;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for (k = 0, min = 0xFFFFFFFF, page1 = 0,page2 = 0; k < PAGE_SUM2; k++)
	{
	    for(i = 0;i< PAGE_SUM1; i++){
            if (pageTable[k][i].count < min&&pageTable[k][i].filled)                                        //修改4：加上filled的判断
            {
                min = pageTable[k][i].count;
                page1 = i;
                page2 = k;
            }
	    }
	}
	printf("选择第%u页中的第%u页进行替换\n", page2 , page1 );
	if (pageTable[page2][page1].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[page2][page1]);
	}
	pageTable[page2][page1].filled = FALSE;
	pageTable[page2][page1].count = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[page2][page1].blockNum);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page2][page1].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
}

/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
	//	printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(actMem + blockNum * PAGE_SIZE,
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
	//	printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
	//	printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
	//	printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
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
//		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(actMem + ptr_pageTabIt->blockNum * PAGE_SIZE,
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
//		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
//		printf("DEBUG: writeNum=%u\n", writeNum);
//		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	printf("写回成功：物理块%u-->>辅存地址%lx\n", ptr_pageTabIt->blockNum, ptr_pageTabIt->auxAddr);          //修改2：物理块和辅存位置调换
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
		case ERROR_VISIT_FAILED:
        {
            printf("系统错误：无权访问\n");
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
	ptr_memAccReq->virAddr = rand() % VIRTUAL_MEMORY_SIZE;
	ptr_memAccReq->reqProcess = rand() %PROCESS_SUM ;
	/* 随机产生请求类型 */
	switch (rand() % 3)
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
			ptr_memAccReq->value = rand() % 0xFFu;
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
void deal_request(char m,unsigned long a,int b)
{
    ptr_memAccReq->virAddr = a % VIRTUAL_MEMORY_SIZE;
	ptr_memAccReq->reqProcess=b% PROCESS_SUM;
    switch (m)
	{
		case 'r': //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n地址：%u\t类型：读取\n", ptr_memAccReq->virAddr);
			break;
		}
		case 'w': //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = rand() % 0xFFu;
			printf("产生请求：\n地址：%u\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		}
		case 'x':
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n地址：%u\t类型：执行\n", ptr_memAccReq->virAddr);
			break;
		}
		default:
			break;
	}
}
/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];
	printf("进程号\t页号2\t页号1\t块号\t装入\t修改\t保护\t计数\t辅存\n");
	for(k = 0; k < PAGE_SUM2; k++){
        for (i = 0; i < PAGE_SUM1; i++)
        {
            printf("%u\t%u\t%u\t%u\t%u\t%u\t%s\t%u\t%u\n", pageTable[k][i].processNum, k, i, pageTable[k][i].blockNum, pageTable[k][i].filled,
                pageTable[k][i].edited, get_proType_str(str, pageTable[k][i].proType),
                pageTable[k][i].count, pageTable[k][i].auxAddr);
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
void initfile()
{
    int i;
    char *key="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char buffer[VIRTUAL_MEMORY_SIZE+1];
    FILE *file=NULL;
    file=fopen(AUXILIARY_MEMORY,"w+");
    for(i=0;i<VIRTUAL_MEMORY_SIZE-3;i++){
        buffer[i]=key[rand()%62];
    }
    buffer[VIRTUAL_MEMORY_SIZE-3]='y';
    buffer[VIRTUAL_MEMORY_SIZE-2]='j';
    buffer[VIRTUAL_MEMORY_SIZE-1]='c';
    buffer[VIRTUAL_MEMORY_SIZE]='0';
    //随机生成256位字符串
    fwrite(buffer,sizeof(BYTE),VIRTUAL_MEMORY_SIZE,file);
    printf("系统提示：初始化辅存模拟文件完成\n");
    fclose(file);
}




int main(int argc, char* argv[])
{
	char c,d;
	unsigned long k;
	int i,j;
	FILE *fp;
	char buf[BUFLEN];
	umask(0);
	if(mkfifo(FIFO,S_IFIFO|0660)<0)
		return -1;
	initfile();
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}

	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */

	
	while(TRUE)
	{
		printf("请输入请求\n");
		fp=fopen(FIFO,"r");
		fgets(buf,sizeof(buf),fp);
		printf("Request is: %s\n",buf);
		fclose(fp);
		i=0;
		k=0;
		j=0;
		if(buf[i]=='r'||buf[i]=='w'||buf[i]=='x'){
			c='a';
            		d=buf[i];
            		k=0;
           		while(buf[i]<'0'||buf[i]>'9'){i++;}
			while(buf[i]>='0'&&buf[i]<='9'){
                		k=k*10+buf[i]-'0';
                		i++;
            		}
			while(buf[i]<'0'||buf[i]>'9'){i++;}
			while(buf[i]>='0'&&buf[i]<='9'){
                		j=j*10+buf[i]-'0';
                		i++;
            		}
            		deal_request(d,k,j);
            		do_response();	
	    	}
	    	else{
            		printf("无法识别命令");
	    	}
		printf("按Y打印页表，按其他键不打印...\n");
		fp=fopen(FIFO,"r");
		fgets(buf,sizeof(buf),fp);
		fclose(fp);
		if (buf[0] == 'y' || buf[0] == 'Y')
			do_print_info();
		printf("按X退出程序，按其他键继续...\n");
		fp=fopen(FIFO,"r");
		fgets(buf,sizeof(buf),fp);
		fclose(fp);
		if (buf[0] == 'e' || buf[0] == 'E')
			break;
	}


/*	while (TRUE)
	{
	do_request();
        do_response();
		printf("按Y打印页表，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info();
		while (c != '\n')
			c = getchar();
		printf("按X退出程序，按其他键继续...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		while (c != '\n')
			c = getchar();
		//sleep(5000);
	}*/

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}
