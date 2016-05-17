#ifndef VMM_H
#define VMM_H

#include "global.h"
#include <stdio.h>

/* 页表项 */
typedef struct
{
	unsigned int pageNum;
	unsigned int blockNum; //物理块号
	BOOL filled; //页面装入特征位
	BYTE proType; //页面保护类型
	BOOL edited; //页面修改标识
	unsigned long auxAddr; //外存地址
	unsigned long count; //页面使用计数器
} PageTableItem, *Ptr_PageTableItem;

/* 访存请求类型 */
typedef enum { 
	REQUEST_READ, 
	REQUEST_WRITE, 
	REQUEST_EXECUTE 
} MemoryAccessRequestType;

/* 访存请求 */
typedef struct
{
	MemoryAccessRequestType reqType; //访存请求类型
	unsigned long virAddr; //虚地址
	BYTE value; //写请求的值
} MemoryAccessRequest, *Ptr_MemoryAccessRequest;


/* 随即产生访存请求 */
void do_request_rand();
int do_request();

/* 响应访存请求 */
void do_response();

/* 处理缺页中断 */
void do_page_fault(Ptr_PageTableItem);

/* LFU页面替换 */
void do_LFU(Ptr_PageTableItem);

/* 装入页面 */
//void do_page_in(Ptr_PageTableItem, unsigned in);
//
///* 写出页面 */
//void do_page_out(Ptr_PageTableItem);

/* 错误处理 */
void do_error(ERROR_CODE);

/* 打印页表相关信息 */
void do_print_info();

/* 获取页面保护类型字符串 */
char *get_proType_str(char *, BYTE);

void load_page(ushort block_num, FILE* file, ulong offset);
void store_page(ushort block_num, FILE* file ,ulong offset);

pid_t get_pid_by_block_num(ushort block_num);
ushort get_vpage(ushort block_num);
void age(ushort block_num);
ulong64 get_age(ushort block_num);
/* 设置块所属pid，指向虚页号，同时age清零 */
void set_mm_ctrl(ushort block_num, pid_t pid, ushort vpage);
void do_init_mm();
#endif
