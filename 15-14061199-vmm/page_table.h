#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H
#include "global.h"
//#include <unistd.h>

#define OFFSET_BIT_NUM 4
/* 页面大小（字节）*/
#define PAGE_SIZE (1<<OFFSET_BIT_NUM)
/* 虚存空间大小（字节） */
#define VIRTUAL_MEMORY_SIZE (64 * PAGE_SIZE)
/* 实存空间大小（字节） */ 
#define ACTUAL_MEMORY_SIZE (32 * PAGE_SIZE)
#define BLOCK_SUM (ACTUAL_MEMORY_SIZE / PAGE_SIZE)

#define ACTUAL_GROUP_NUM 4
#define GROUP_ITEM_NUM (BLOCK_SUM / GROUP_ITEM_NUM)

///* 总虚页数 */
//#define PAGE_SUM (VIRTUAL_MEMORY_SIZE / PAGE_SIZE)
/* 总物理块数 */


typedef struct{
	pid_t pid;
	ushort v_page;
	uchar space[4];
	ulong64 age;
}mem_item;

typedef struct{
	mem_item items[BLOCK_SUM];
}mem_manage;

//typedef struct{
//	uchar space[2];
//}table_item_c;
//
//#define PAGE_NUM_C (PAGE_SIZE / sizeof(table_item_c))
//
//typedef struct{
//	table_item_c items[PAGE_NUM_C];
//}page_table_c;

typedef union _table_item{
	uchar content_c[2];
	ushort content_s;
}*table_item;

//#define PAGE_NUM (VIRTUAL_MEMORY_SIZE / PAGE_NUM_C / PAGE_SIZE)
#define PAGE_NUM (PAGE_SIZE / sizeof(union _table_item))

typedef struct _page_table{
	union _table_item items[PAGE_NUM];
}*page_table;

void deleteTable(page_table table);

ERROR_CODE read_from(pid_t pid, ushort address, uchar* res);
ERROR_CODE write_to(pid_t pid, ushort address, uchar data);
ERROR_CODE execute_at(pid_t pid, ushort address);

void do_page_out(table_item item);
ERROR_CODE do_page_in(pid_t pid, table_item item);


BOOL page_access_r(table_item item);
BOOL page_access_w(table_item item);
BOOL page_access_x(table_item item);
BOOL page_loaded(table_item item);
BOOL page_modified(table_item item);
#endif	//PAGE_TABLE_H
