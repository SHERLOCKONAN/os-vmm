#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "vmm.h"


/* 页表 */
PageTableItem pageTable[PAGE_SUM];
MasterPageTableItem masterPageTable[MASTER_PAGE_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* FIFO */
int fd;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;



/* 初始化环境 */
void do_init()
{
	int i, j;
	srandom(time(NULL));
    for (i = 0; i < MASTER_PAGE_SUM; i++)
    {
        masterPageTable[i].pages = pageTable + i * PAGE_SUM / MASTER_PAGE_SUM;
    }
	for (i = 0; i < PAGE_SUM; i++)
	{
		pageTable[i].pageNum = i;
		pageTable[i].filled = FALSE;
		pageTable[i].edited = FALSE;
        pageTable[i].count = 0;
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
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (random() % 2 == 0)
		{
			do_page_in(&pageTable[j], j);
			pageTable[j].blockNum = j;
			pageTable[j].filled = TRUE;
			blockStatus[j] = TRUE;
            pageTable[j].count += 128;
		} else {
			blockStatus[j] = FALSE;
        }
	}
}

/* 响应请求 */
void do_response()
{
    Ptr_PageTableItem ptr_pageTabIt;
    unsigned int masterPageNum, pageNum, offAddr;
    unsigned int actAddr;
    int i;
    int count;
    while (TRUE)
    {
        if ((count = read(fd, ptr_memAccReq, sizeof(MemoryAccessRequest))) < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("本次读取结束.\n");
                return;
            } else {
                printf("read FIFO failed.\n");
                exit(1);
            }
        } else if (count < sizeof(MemoryAccessRequest)) {
            printf("本次读取结束.\n");
            return;
        }
        if (ptr_memAccReq->virAddr >= ptr_memAccReq->id * VIRTUAL_MEMORY_EACH && ptr_memAccReq->virAddr < (ptr_memAccReq->id + 1) * VIRTUAL_MEMORY_EACH)
        {
            switch(ptr_memAccReq->reqType)
            {
                case 0: //读请求
                    {
                        printf("产生请求：\nid：%d\t地址：%u\t类型：读取\n",ptr_memAccReq->id, ptr_memAccReq->virAddr);
                        break;
                    }
                case 1: //写请求
                    {
                        if (ptr_memAccReq->value >= 0 && ptr_memAccReq->value < 0xFFu)
                        {
                            printf("产生请求：\nid：%d\t地址：%u\t类型：写入\t值：%02X\n", ptr_memAccReq->id, ptr_memAccReq->virAddr, ptr_memAccReq->value);
                        } else {
                            do_error(ERROR_INVALID_REQUEST);
                        }
                        break;
                    }
                case 2:
                    {
                        printf("产生请求：\nid：%d\t地址：%u\t类型：执行\n", ptr_memAccReq->id, ptr_memAccReq->virAddr);
                        break;
                    }
                default:
                    do_error(ERROR_INVALID_REQUEST);
                    continue;
            }
        } else {
            do_error(ERROR_OVER_BOUNDARY);
            continue;
        }

        /* 计算页号和页内偏移值 */
        masterPageNum = ptr_memAccReq->virAddr / PAGE_SUM;
        pageNum = ptr_memAccReq->virAddr % PAGE_SUM / PAGE_SIZE;
        offAddr = ptr_memAccReq->virAddr % PAGE_SUM % PAGE_SIZE;
        printf("一级页号为：%u\t二级页号为：%u\t页内偏移为：%u\n", masterPageNum, pageNum, offAddr);

        /* 获取对应页表项 */
        ptr_pageTabIt = masterPageTable[masterPageNum].pages + pageNum;

        /* 根据特征位决定是否产生缺页中断 */
        if (!ptr_pageTabIt->filled)
        {
            do_page_fault(ptr_pageTabIt);
        }

        actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
        printf("实地址为：%u\n", actAddr);

        /* 检查页面访问权限并处理访存请求 */
        switch (ptr_memAccReq->reqType)
        {
            case REQUEST_READ: //读请求
                {
                    if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
                    {
                        do_error(ERROR_READ_DENY);
                        continue;
                    }
                    /* 读取实存中的内容 */
                    for (i = 0; i < PAGE_SUM; i++) 
                    {
                        pageTable[i].count /= 2;
                        if (pageTable + i == ptr_pageTabIt) 
                        {
                            pageTable[i].count += 128;
                        }
                    }
                    printf("读操作成功：值为%02X\n", actMem[actAddr]);
                    break;
                }
            case REQUEST_WRITE: //写请求
                {
                    if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
                    {
                        do_error(ERROR_WRITE_DENY);	
                        continue;
                    }
                    /* 向实存中写入请求的内容 */
                    for (i = 0; i < PAGE_SUM; i++) 
                    {
                        pageTable[i].count /= 2;
                        if (pageTable + i == ptr_pageTabIt) 
                        {
                            pageTable[i].count += 128;
                        }
                    }
                    actMem[actAddr] = ptr_memAccReq->value;
                    ptr_pageTabIt->edited = TRUE;			
                    printf("写操作成功\n");
                    break;
                }
            case REQUEST_EXECUTE: //执行请求
                {
                    if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
                    {
                        do_error(ERROR_EXECUTE_DENY);
                        continue;
                    }
                    for (i = 0; i < PAGE_SUM; i++) 
                    {
                        pageTable[i].count /= 2;
                        if (pageTable + i == ptr_pageTabIt) 
                        {
                            pageTable[i].count += 128;
                        }
                    }
                    printf("执行成功\n");
                    break;
                }
            default: //非法请求类型
                {	
                    do_error(ERROR_INVALID_REQUEST);
                    continue;
                }
        }
    }
}

/* 处理缺页中断 */
void do_page_fault(Ptr_PageTableItem ptr_pageTabIt)
{
    unsigned int i;
    printf("产生缺页中断，开始进行调页...\n");
    for (i = 0; i < PAGE_SUM; i++) 
    {
        pageTable[i].count /= 2;
        if (pageTable + i == ptr_pageTabIt) 
        {
            pageTable[i].count += 128;
        }
    }
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
			blockStatus[i] = TRUE;
            return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
    do_switch(ptr_pageTabIt);
}

void do_switch(Ptr_PageTableItem ptr_pageTabIt)
{
    unsigned int rec = 0, i;
    unsigned char min = 255;
	printf("没有空闲物理块，开始进行老化页面替换...\n");
    for (i = 0; i < PAGE_SUM; i++)
    {
        if (pageTable[i].filled && pageTable[i].count < min){
            min = pageTable[i].count;
            rec = i;
        }
    }
    printf("选择第%u页进行替换\n", rec);
    if (pageTable[rec].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(pageTable + rec);
	}
	pageTable[rec].filled = FALSE;
	pageTable[rec].count = 0;

	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable[rec].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable[rec].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
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
		printf("DEBUG: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
        printf("actMem:%d\tblockNum:%d\n", actMem, blockNum);
        printf("readNum:%d\n", readNum);
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
        case ERROR_FILE_CREATE_FAILED:
        {
            printf("系统错误：创建文件失败\n");
            break;
        }
		default:
		{
			printf("未知错误：没有这个错误代码\n");
		}
	}
}

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];
	printf("页号\t块号\t装入\t修改\t保护\t计数\t辅存\n");
	for (i = 0; i < PAGE_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\n", i, pageTable[i].blockNum, pageTable[i].filled, pageTable[i].edited, get_proType_str(str, pageTable[i].proType), pageTable[i].count, pageTable[i].auxAddr);
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

void initFile()
{
    int i;
    char *key = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char buffer[VIRTUAL_MEMORY_SIZE + 1];
    
    ptr_auxMem = fopen(AUXILIARY_MEMORY, "w");
    for (i = 0; i < VIRTUAL_MEMORY_SIZE; i++) 
    {
        buffer[i] = key[rand() % 62];
    }
    buffer[VIRTUAL_MEMORY_SIZE] = '\0';
    fwrite(buffer, sizeof(BYTE), VIRTUAL_MEMORY_SIZE, ptr_auxMem);
	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
}


int main(int argc, char* argv[])
{
	char c;
	int i;
    struct stat statbuf;

    if (access(AUXILIARY_MEMORY, F_OK) == -1)
    {
        if(creat(AUXILIARY_MEMORY, 0755) < 0)
        {
            do_error(ERROR_FILE_CREATE_FAILED);
        }
        initFile();
    }
    if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
    {
        do_error(ERROR_FILE_OPEN_FAILED);
        exit(1);
    }

    if(stat(FIFO, &statbuf)==0)
    {
        /* 如果FIFO文件存在,删掉 */
        if(remove(FIFO)<0)
        {
            printf("remove FIFO failed");
            exit(1);
        }
    }
    if(mkfifo(FIFO, 0666)<0)
    {
        printf("mkfifo failed");
        exit(1);
    }

    /* 在非阻塞模式下打开FIFO */
    if((fd = open(FIFO, O_RDONLY | O_NONBLOCK)) < 0) 
    {
        printf("open FIFO failed");
        exit(1);
    }

	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
        printf("按S读取请求，按其他键不读取请求...\n");
        if ((c = getchar()) == 's' || c == 'S')
            do_response();
        while (c != '\n')
            c = getchar();
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
	}
    close(fd);
	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}
