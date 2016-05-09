#ifndef VM_H
#define VM_H

/* 模拟辅存的文件路径 */
#define AUXILIARY_MEMORY "vmm_auxMem"

#define REQFIFO "/tmp/vmm_requ"
#define RESFIFO "/tmp/vmm_resp"

#define PAGE_LEN 2u

#define DISK_LEN 12u

#define VIRTUAL_MEMORY_LEN 10u

#define ACTUAL_MEMORY_LEN 8u

#define IDX2_LEN 5u

#define IDX1_LEN 8u

/* 页面大小（字节）*/
#define PAGE_SIZE (1u<<PAGE_LEN)
/* disk空间大小（字节） */
#define DISK_SIZE (1u<<DISK_LEN)
/* 虚存空间大小（字节） */
#define VIRTUAL_MEMORY_SIZE (1u<<VIRTUAL_MEMORY_LEN)
/* 实存空间大小（字节） */ 
#define ACTUAL_MEMORY_SIZE (1u<<ACTUAL_MEMORY_LEN)

#define IDX2_SIZE (1u<<IDX2_LEN)

#define IDX1_SIZE (1u<<IDX1_LEN)
/* 总虚页数 */
#define PAGE_SUM (1u<<(VIRTUAL_MEMORY_LEN - PAGE_LEN))

#define ITEM_SUM (1u<<(IDX2_LEN-PAGE_LEN))

#define IDX2_SUM (1u<<(IDX1_LEN-IDX2_LEN))

#define IDX1_SUM (1u<<(VIRTUAL_MEMORY_LEN-IDX1_LEN))

/* 总物理块数 */
#define BLOCK_SUM (1u<<(ACTUAL_MEMORY_LEN - PAGE_LEN))

#define PROC_MAXN (1u<<(DISK_LEN - VIRTUAL_MEMORY_LEN))

/* 可读标识位 */
#define READABLE 0x01u
/* 可写标识位 */
#define WRITABLE 0x02u
/* 可执行标识位 */
#define EXECUTABLE 0x04u


/* 定义字节类型 */
#define BYTE unsigned char

typedef enum {
	TRUE = 1, FALSE = 0
} BOOL;

/* 访存请求类型 */
typedef enum { 
	REQUEST_READ, 
	REQUEST_WRITE, 
	REQUEST_EXECUTE 
} MemoryAccessRequestType;

/* 访存请求 */
typedef struct
{
	int pid;
	MemoryAccessRequestType reqType; //访存请求类型
	unsigned int virAddr; //虚地址
	BYTE value; //写请求的值
} MemoryAccessRequest, *Ptr_MemoryAccessRequest;


/* 访存错误代码 */
typedef enum {
	ERROR_READ_DENY, //该页不可读
	ERROR_WRITE_DENY, //该页不可写
	ERROR_EXECUTE_DENY, //该页不可执行
	ERROR_INVALID_REQUEST, //非法请求类型
	ERROR_OVER_BOUNDARY, //地址越界
	ERROR_FILE_OPEN_FAILED, //文件打开失败
	ERROR_FILE_CLOSE_FAILED, //文件关闭失败
	ERROR_FILE_SEEK_FAILED, //文件指针定位失败
	ERROR_FILE_READ_FAILED, //文件读取失败
	ERROR_FILE_WRITE_FAILED //文件写入失败
} ERROR_CODE;

/* 错误处理 */
void do_error(ERROR_CODE);

#endif
