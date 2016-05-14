#ifndef VMM_H
#define VMM_H

#ifndef DEBUG
#define DEBUG
#endif
#undef DEBUG


/* 模拟辅存的文件路径 */
#define AUXILIARY_MEMORY "vmm_auxMem"

/* 页面大小（字节）*/
#define PAGE_SIZE 4
/* 虚存空间大小（字节） */
#define VIRTUAL_MEMORY_SIZE (64 * 4)
/* 实存空间大小（字节） */ 
#define ACTUAL_MEMORY_SIZE (32 * 4)
//二级页表数目
#define PAGE_TOTAL (VIRTUAL_MEMORY_SIZE/PAGE_SIZE)	
//一级页表数目		
#define OUTER_PAGE_TOTAL (PAGE_TOTAL/PAGE_SIZE)				
/* 总物理块数 */
#define BLOCK_TOTAL (ACTUAL_MEMORY_SIZE / PAGE_SIZE)
#define PID_NUM 2

/* 可读标识位 */
#define READABLE 0x01u
/* 可写标识位 */
#define WRITABLE 0x02u
/* 可执行标识位 */
#define EXECUTABLE 0x04u
#define MAX_VALUE 0xFFu

#define FIFO "fifo"




/* 定义字节类型 */
#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long

typedef enum {
	TRUE = 1, FALSE = 0
} BOOL;




/*外层页表结构类型*/
typedef struct
{
	unsigned int page_num;										//外层页号
	unsigned int index_num;										//二级页表块号
}OuterPageTableItem,*OuterPageTablePtr;

/*二级页表结构*/
typedef struct
{
	unsigned int page_num;										//二级页号
	unsigned int block_num;										//物理内存块号
	BOOL filled;												//特征位
	BOOL changed;												//页面是否改变
	unsigned char pro_type;										//读、写、执行类型
	unsigned long virtual_address;								//虚存地址
	unsigned long count;										//页面使用计数
    unsigned int no_use;                                        //用来寻找最近最久未使用的页面
}PageTableItem,*PageTablePtr;

/* 访存请求类型 */
typedef enum
{
	READ,
	WRITE,
	EXECUTE 
}RequestType;

/* 访存请求 */
typedef struct
{
	RequestType request_type;									//访存请求类型
	unsigned long virtual_address;								//虚存地址
	unsigned char value;										//存储的值
}MemoryAccessRequest,*MemoryAccessRequestPtr;


/* 访存错误代码 */
typedef enum
{
	READ_DENY,													//该地址内容不可读
	WRITE_DENY,													//该地址内容不可写
	EXECUTE_DENY,												//该地址内容不可执行
	INVALID_REQUEST,											//非法访存请求
	OVER_BOUNDARY,												//访存地址越界
	FILE_OPEN_FAILED,											//打开文件失败
	FILE_CLOSE_FAILED,											//关闭文件失败
	FILE_SEEK_FAILED,											//文件指针定位失败
	FILE_READ_FAILED,											//读取文件失败
	FILE_WRITE_FAILED											//写入文件失败
}ErrorType;

/*类似PCB结构*/
typedef struct
{
	unsigned int pid;											//作业号
	unsigned int begin;											//起始外层页表号
	unsigned int end;											//结束外层页表号
}PCB;

/*全局变量声明*/

unsigned char actual_memory[ACTUAL_MEMORY_SIZE];				//模拟实存空间的数组
FILE *auxmem_ptr;												//指向模拟辅存空间的文件指针
PageTableItem pagetable[PAGE_TOTAL];							//二级页表数组
OuterPageTableItem outerpagetable[OUTER_PAGE_TOTAL];			//一级页表数组
BOOL block_status[BLOCK_TOTAL];									//物理块使用标识
MemoryAccessRequestPtr mem_request;								//访存请求
unsigned int Time[PAGE_TOTAL];                                  //记录页面使用时间，便于FIFO算法使用
PCB pcb[PID_NUM];                                               //记录作业对应的起始以及结束页表号
unsigned int exec_times;										//记录页面使用的次数，便于LRU算法使用
int time_n;
int work_id;

/*初始化环境*/
void init();

/*访存请求*/
void do_request(MemoryAccessRequestPtr);						//随机产生访存请求
void do_response();												//响应访存请求

/*页表处理*/
void do_page_in(PageTablePtr,unsigned int);						//将辅存内容写入实存
void do_page_out(PageTablePtr);									//将实存中被替换页面的内容写回辅存
void do_page_fault(PageTablePtr);								//缺页处理

/*页面淘汰算法*/
void do_LFU(PageTablePtr);										//最不频繁使用淘汰算法
void do_OPT(PageTablePtr);										//最优算法
void do_FIFO(PageTablePtr);										//先进先出
void do_LRU(PageTablePtr);										//最近最久未使用淘汰算法

/*打印页表*/
void print_pageinfo();

/*错误处理*/
void handle_error(ErrorType);

/*time数组处理*/
void time_change(unsigned int);

/*获取页面保护类型字符串*/
char *get_protype_str(char *str,unsigned char type);



#endif
