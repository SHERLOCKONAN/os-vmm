#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/ipc.h>
#include <signal.h>
#include "vmm.h"
#define MANUAL
/* 页表 */
PageTableItem pageTable[PAGE1_SUM][PAGE2_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;
FILE *fd;
/*初始化辅存文件*/
void initFile(){
    int i;
    char* key="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char buffer[VIRTUAL_MEMORY_SIZE*2+1];
    //errno_t err;
    
    ptr_auxMem = fopen(AUXILIARY_MEMORY,"w+");
    for(i=0;i<VIRTUAL_MEMORY_SIZE*2-3;i++)
    {
        buffer[i]=key[random()%62];
        
    }
    buffer[VIRTUAL_MEMORY_SIZE*2-3]='y';
    buffer[VIRTUAL_MEMORY_SIZE*2-2]='m';
    buffer[VIRTUAL_MEMORY_SIZE*2-1]='c';
    buffer[VIRTUAL_MEMORY_SIZE*2]='\0';
    //随机生成512位字符串
    fwrite(buffer,sizeof(BYTE),VIRTUAL_MEMORY_SIZE*2,ptr_auxMem);
    printf("系统提示：初始化赋存模拟文件完成\n");
    fclose(ptr_auxMem);
    
}
/* 初始化环境 */
void do_init()
{
	int i, j;
        Ttime=0;
        time_n=0;
	srandom(time(NULL));
	for (i = 0; i < PAGE1_SUM; i++)
	{
		for (j = 0; j < PAGE2_SUM; j++){
			pageTable[i][j].pageNum = i*PAGE1_SUM+j;
			pageTable[i][j].filled = FALSE;
			pageTable[i][j].edited = FALSE;
			pageTable[i][j].count = 0;
			/* 使用随机数设置该页的保护类型和保护进程号 */

			switch (random() % 7)
			{
				case 0:
				{
					pageTable[i][j].proType = READABLE;
					pageTable[i][j].pidProType =0;
					break;
				}
				case 1:
				{
					pageTable[i][j].proType = WRITABLE;
					pageTable[i][j].pidProType =1;
					break;
				}
				case 2:
				{
					pageTable[i][j].proType = EXECUTABLE;
					pageTable[i][j].pidProType =2;
					break;
				}
				case 3:
				{
					pageTable[i][j].proType = READABLE | WRITABLE;
					pageTable[i][j].pidProType =3;
					break;
				}
				case 4:
				{
					pageTable[i][j].proType = READABLE | EXECUTABLE;
					pageTable[i][j].pidProType =4;
					break;
				}
				case 5:
				{
					pageTable[i][j].proType = WRITABLE | EXECUTABLE;
					pageTable[i][j].pidProType =5;
					break;
				}
				case 6:
				{
					pageTable[i][j].proType = READABLE | WRITABLE | EXECUTABLE;
					pageTable[i][j].pidProType =6;
					break;
				}
				default:
					break;
			}

		/* 设置该页对应的辅存地址 */
		pageTable[i][j].auxAddr = (i*PAGE1_SUM+j) * PAGE_SIZE;
            }
	}
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (random() % 2 == 0)
		{
			do_page_in(&pageTable[j/PAGE1_SUM][j%PAGE1_SUM], j);
			pageTable[j/PAGE1_SUM][j%PAGE1_SUM].blockNum = j;
			pageTable[j/PAGE1_SUM][j%PAGE1_SUM].filled = TRUE;
			blockStatus[j] = TRUE;
                        Time[time_n++]=pageTable[j/PAGE1_SUM][j%PAGE1_SUM].pageNum;
		}
		else
			blockStatus[j] = FALSE;
	}
}
/*
Ptr_MemoryAccessRequest restore(char ch[]){
        Ptr_MemoryAccessRequest p;
        switch (ch[3])
	{
		case 'r':
		{
			p->reqType=REQUEST_READ;
			break;
		}
		case 'w':
		{
			p->reqType=REQUEST_WRITE;
                        p->value=ch[4];
			break;
		}
		case 'e':
		{
			p->reqType=REQUEST_EXECUTE;
			break;
		}
        }
               

        if(ch[1]=='q'){
              p->virAddr=(unsigned long)(ch[0]-'0');
        }
        else if(ch[2]=='q'){
              p->virAddr=(unsigned long)((ch[0]-'0')*10+(ch[1]-'0'));
        }
        else{
              p->virAddr=(unsigned long)(((ch[0]-'0')*10+(ch[1]-'0'))*10+(ch[2]-'0'));
        }
        return p;
}
*/
/*测试进程与内存是否一致*/
BOOL test(int pid,int pidProType){
    if(pid==pidProType){
        return TRUE;
    }
    else{
        return FALSE;
    }
}
/* 响应请求 */

void do_response()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum, offAddr;
	unsigned int actAddr;	
	/* 检查地址是否越界 */

	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr > VIRTUAL_MEMORY_SIZE-1)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* 计算页号和页内偏移值 */

	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("页号为：%u\t页内偏移为：%u\n", pageNum, offAddr);

	/* 获取对应页表项 */

	ptr_pageTabIt = &pageTable[pageNum/PAGE1_SUM][pageNum%PAGE1_SUM];
	
	/* 根据特征位决定是否产生缺页中断 */

	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}
	
	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);
	ptr_pageTabIt->usetime=Ttime;
	Ttime++;
	/* 检查页面访问权限并处理访存请求 */
        if(test(ptr_memAccReq->pid,pageTable[pageNum/PAGE1_SUM][pageNum%PAGE1_SUM].pidProType)==FALSE){
            do_error(ERROR_PID_WRONG);//页面与进程不匹配
            return;
        }
        else{
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

			printf("读操作成功：值为%02X\n", actMem[actAddr]);
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
			printf("写操作成功\n");
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
	unsigned int i,j, min, page;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for (i = 0, min = 0xFFFFFFFF, page = 0; i < PAGE1_SUM; i++)
	{
		for(j = 0;j<PAGE2_SUM; j++){
			if (pageTable[i][j].count < min)
			{
				min = pageTable[i][j].count;
				page = i*PAGE1_SUM+j;
			}
		}
	}
	printf("选择第%u页进行替换\n", page);
	if (pageTable[page/PAGE1_SUM][page%PAGE1_SUM].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[page/PAGE1_SUM][page%PAGE1_SUM]);
	}
	pageTable[page/PAGE1_SUM][page%PAGE1_SUM].filled = FALSE;
	pageTable[page/PAGE1_SUM][page%PAGE1_SUM].count = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[page/PAGE1_SUM][page%PAGE1_SUM].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page/PAGE1_SUM][page%PAGE1_SUM].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
}
void do_LRU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i,j, early_use, page;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for (i = 0, early_use = 0xFFFFFFFF, page = 0; i < PAGE1_SUM; i++)
	{
		for(j=0;j<PAGE2_SUM;j++){
			if ((pageTable[i][j].usetime < early_use)  &&  pageTable[i][j].filled)
			{
				early_use = pageTable[i][j].usetime;
				page = i*PAGE1_SUM+j;
			}
		}
	}
	printf("选择第%u页进行替换\n",pageTable[page/PAGE1_SUM][page%PAGE1_SUM].pageNum);
	if (pageTable[page/PAGE1_SUM][page%PAGE1_SUM].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[page/PAGE1_SUM][page%PAGE1_SUM]);
	}
	pageTable[page/PAGE1_SUM][page%PAGE1_SUM].filled = FALSE;
	pageTable[page/PAGE1_SUM][page%PAGE1_SUM].count = 0;


	/* 读辅存内容，写入到实存*/
	do_page_in(ptr_pageTabIt, pageTable[page/PAGE1_SUM][page%PAGE1_SUM].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[page/PAGE1_SUM][page%PAGE1_SUM].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
}
void do_FIFO(Ptr_PageTableItem ptr_pageTabIt)
{
    unsigned int firstcome;
    int i;
    firstcome=Time[0];			//Time[PAGE_TOTAL]里面存着现在的主存里的页面存储状况，每一项是页号，第一项是先进来的页面
    printf("没有空闲物理块，开始进行FIFO页面替换...\n ");
    printf("选择第%u页进行替换\n",firstcome);
    if(pageTable[firstcome/PAGE1_SUM][firstcome%PAGE1_SUM].edited)
    {
          printf("该页内容有修改，写回至辅存\n");
          do_page_out(&pageTable[firstcome/PAGE1_SUM][firstcome%PAGE1_SUM]);
    }
    pageTable[firstcome/PAGE1_SUM][firstcome%PAGE1_SUM].edited=FALSE;
    pageTable[firstcome/PAGE1_SUM][firstcome%PAGE1_SUM].count=0;
    pageTable[firstcome/PAGE1_SUM][firstcome%PAGE1_SUM].filled=FALSE;

    do_page_in(ptr_pageTabIt,pageTable[firstcome/PAGE1_SUM][firstcome%PAGE1_SUM].blockNum);


    ptr_pageTabIt->blockNum = pageTable[firstcome/PAGE1_SUM][firstcome%PAGE1_SUM].blockNum;
    ptr_pageTabIt->filled = TRUE;
    ptr_pageTabIt->edited = FALSE;
    ptr_pageTabIt->count = 0;
    printf("页面替换成功\n");
    for(i=0;i<time_n-1;++i)
		Time[i]=Time[i+1];
	Time[time_n]=ptr_pageTabIt->pageNum;
}

/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)//移动失败，将ptr_auxMem以SEEK_SET为基准移动ptr_pageTabIt个字节的位置
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
                case ERROR_PID_WRONG:
		{
			printf("访存失败：该页面与进程不匹配\n");
			break;
		}		
		default:
		{
			printf("未知错误：没有这个错误代码\n");
		}
	}
}
/*将整形转化为字符串*/
void itoa(unsigned long i,char*string)
      {
      int power,j;
      j=i;
      for(power=1;j>=10;j/=10)
      power*=10;
      for(;power>0;power/=10)
       {
           *string++='0'+i/power;
           i%=power;
       }
      *string='\0';
     }

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];
	printf("页号\t块号\t装入\t修改\t保护\t计数\t辅存\t进程\n");
	for(i=0;i<PAGE1_SUM;i++){
		for(j=0;j<PAGE2_SUM;j++){

			printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\t%d\n", i*PAGE1_SUM+j, pageTable[i][j].blockNum, pageTable[i][j].filled, 
			pageTable[i][j].edited, get_proType_str(str, pageTable[i][j].proType), 
			pageTable[i][j].count, pageTable[i][j].auxAddr,pageTable[i][j].pidProType);
		}
	}
}
/* 打印辅存 */
void do_print_virtual()
{
	char ch;
	int i=1;
        ptr_auxMem=fopen(AUXILIARY_MEMORY,"r");
        printf("辅存内容如下\n");
        while(fread(&ch,sizeof(BYTE),1,ptr_auxMem)!=0){
 		printf("%c\t",ch);
		if(i%10==0){
			printf("\n");
		}
		i++;
        }
	printf("\n");
}
/* 打印实存 */
void do_print_practical()
{
	int i=0;
     	for(i=0;i<ACTUAL_MEMORY_SIZE;i++){
        	if(i%10==0){
			printf("\n");
		}
		printf("%x\t",actMem[i]);
	}
   	printf("\n");
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
	int i,readNum;
        struct stat statbuf;
        initFile();
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	
	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
        umask(0);
        mkfifo(FIFO,S_IFIFO|0666);
        if((fd=fopen(FIFO,"r"))<0)
              printf("open fifo failed"); 
	while (TRUE)
	{
                        readNum=fread(ptr_memAccReq,sizeof(MemoryAccessRequest),1,fd);
		        do_response();
		        printf("按Y打印页表，按其他键不打印...\n");
                        c = getchar();
		        if (c == 'y' || c == 'Y')
			     do_print_info();
		        while (c != '\n')
			     c = getchar();
 			printf("按F打印辅存，按其他键不打印...\n");
                        c = getchar();
		        if (c == 'f' || c == 'F')
			     do_print_virtual();
		        while (c != '\n')
			     c = getchar();
     			printf("按S打印实存，按其他键不打印...\n");
                 	c = getchar();
		        if (c == 's' || c == 'S')
			     do_print_practical();
		        while (c != '\n')
			     c = getchar();
		        printf("按X退出程序，按其他键继续...\n");
                        c = getchar();
		        if (c == 'x' || c == 'X')
			    break;
		        while (c != '\n')
			    c = getchar();
	}
        fclose(fd);
	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}
