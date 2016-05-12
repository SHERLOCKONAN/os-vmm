#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "vmm.h"

MemoryAccessRequest req;
int fd;

int toInt(char *str) 
{
    int res = 0, i;
    for (i = 0; str[i] != '\0'; i++)
    {
        res = res * 10 + str[i] - '0';    
    }
    return res;
}

MemoryAccessRequestType getType(char *buf)
{
    if (strcmp("read", buf) == 0)
    {
        return REQUEST_READ;
    } else if (strcmp("write", buf) == 0) {
        return REQUEST_WRITE;
    } else if (strcmp("execute", buf) == 0) {
        return REQUEST_EXECUTE;
    } else {
        return -1;
    }
}

void parse()
{
    char buf[8];
    while(1)
    {
        printf("输入请求类型:(read/write/execute),输入其他退出\n");
        scanf("%s", buf);
        printf("输入虚地址:\n");
        switch(req.reqType = getType(buf))
        {
            case REQUEST_READ:
                scanf("%s", buf);
                req.virAddr = toInt(buf);
                req.value = 0;
                break;
            case REQUEST_WRITE:
                scanf("%s", buf);
                req.virAddr = toInt(buf);
                printf("输入值:\n");
                scanf("%s", buf);
                req.value = toInt(buf);
                break;
            case REQUEST_EXECUTE:
                scanf("%s", buf);
                req.virAddr = toInt(buf);
                req.value = 0;
                break;
            default:
                return;
        }
        if (write(fd, &req, sizeof(MemoryAccessRequest)) < 0)
        {
            printf("Write FIFO failed.\n");
            exit(1);
        }
    }
}

int main(int argc, char* argv[])
{
    int id;
    printf("等待接收端启动.\n");
    if ((fd = open(FIFO, O_WRONLY)) < 0)
    {
        printf("Open FIFO failed.\n");
        exit(1);
    }
    printf("设置本进程id以映射外存空间:0映射到0-255，1映射到256-511，2映射到512-767，3映射到768-1023.\n");
    scanf("%d", &id);
    req.id = id;
    parse();
    close(fd);
    return 0;
}
