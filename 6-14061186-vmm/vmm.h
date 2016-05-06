#include <time.h>
#include "vm.h"
#ifndef VMM_H
#define VMM_H

#ifndef DEBUG
#define DEBUG
#endif
#undef DEBUG


/* 页表项 */
typedef struct
{
	unsigned int pageNum;
	unsigned int blockNum; //物理块号
	BOOL filled; //页面装入特征位
	BYTE proType; //页面保护类型
	BOOL edited; //页面修改标识
	unsigned int auxAddr; //外存地址
	unsigned int count; //页面使用计数器
	clock_t time_now;	//最后访问时间
} PageTableItem, *Ptr_PageTableItem;

/* 页表目录项 */
typedef struct
{
	PageTableItem paget[ITEM_SUM];
} Index2Item, *Ptr_Index2Item;

typedef struct
{
	Ptr_Index2Item paget[IDX2_SUM];
} Index1Item, *Ptr_Index1Item;

typedef struct
{
	//Ptr_PageTableItem paget[PAGE_SUM];
	//singlePage
	Ptr_Index1Item paget[IDX1_SUM];
	//multiplyPage
	int pid;
} ProcessItem, *Ptr_ProcessItem;

/* 产生访存请求 */
void do_request();

/* 响应访存请求 */
void do_response();

/* 处理缺页中断 */
void do_page_fault(Ptr_PageTableItem, Ptr_MemoryAccessRequest ptr);

/* LFU页面替换 */
void do_LFU(Ptr_PageTableItem, Ptr_MemoryAccessRequest req);

/* LRU页面替换 */
void do_LRU(Ptr_PageTableItem, Ptr_MemoryAccessRequest req);

/* 装入页面 */
void do_page_in(Ptr_PageTableItem, unsigned in);

/* 写出页面 */
void do_page_out(Ptr_PageTableItem);

/* 打印页表相关信息 */
void do_print_info();

/* 获取页面保护类型字符串 */
char *get_proType_str(char *, BYTE);

void free_proc(int pid);


#endif
