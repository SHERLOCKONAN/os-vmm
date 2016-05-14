#ifndef GLOBAL_H
#define GLOBAL_H
#include <unistd.h>


/* 模拟辅存的文件路径 */
#define AUXILIARY_MEMORY "vmm_auxMem"


/* 可读标识位 */
#define READABLE 0x01u
/* 可写标识位 */
#define WRITABLE 0x02u
/* 可执行标识位 */
#define EXECUTABLE 0x04u

/* 访存错误代码 */
typedef enum {
	ERROR_OK = 0,
	ERROR_PROC_INVALID,
	ERROR_TABLE_INVALID,
	ERROR_PARAM_INVALID,
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




/* 定义字节类型 */
#define BYTE unsigned char

//typedef long pid_t;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned long long ulong64;
typedef enum {
	TRUE = 1, FALSE = 0
} BOOL;

typedef unsigned char uchar;
#endif	//GLOBAL_H
