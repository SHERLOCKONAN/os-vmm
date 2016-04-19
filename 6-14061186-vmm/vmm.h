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
} PageTableItem, *Ptr_PageTableItem;

/* 页表目录项 */
typedef struct
{
	unsigned int pageIndex;
	unsigned int offset; //位移
} PageIndexItem, *Ptr_PageIndexItem;

typedef struct
{
	PageTableItem paget[PAGE_SUM];
	int pid;
} ProcessItem, *Ptr_ProcessItem;

/* 产生访存请求 */
void do_request();

/* 响应访存请求 */
void do_response();

/* 处理缺页中断 */
void do_page_fault(Ptr_PageTableItem, Ptr_PageTableItem pageTable);

/* LFU页面替换 */
void do_LFU(Ptr_PageTableItem, Ptr_PageTableItem pageTable);

/* 装入页面 */
void do_page_in(Ptr_PageTableItem, unsigned in);

/* 写出页面 */
void do_page_out(Ptr_PageTableItem);

/* 打印页表相关信息 */
void do_print_info();

/* 获取页面保护类型字符串 */
char *get_proType_str(char *, BYTE);


#endif
