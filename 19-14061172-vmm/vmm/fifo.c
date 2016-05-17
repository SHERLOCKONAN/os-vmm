#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "vmm.h"

int main()
{
	FILE *fp;
	int write_num,i=0,n=0;
	char c;
	printf("Please select:'1' for manually enter instructions and '2' for automatically generated\n");
	if((c=getchar())=='1')
	{
		MemoryAccessRequestPtr mem_request;
		mem_request=(MemoryAccessRequestPtr)malloc(sizeof(MemoryAccessRequest));
		int type_num;
		int value;
		if((fp=fopen(FIFO,"a+"))==NULL)
			{
				//handle_error(FILE_OPEN_FAILED);
				exit(1);
			}
		printf("please input Address:\n");
		scanf("%lu",&mem_request->virtual_address);
		printf("please input Type...'0' for 'READ'  '1' for 'WRITE'  '3' for 'EXECUTE'\n");
		scanf("%d",&type_num);
		switch (type_num){
			case 0:
				mem_request->request_type=READ;
				printf("Input request:\nAddress:%lu\tType:read\n",mem_request->virtual_address);
				break;
			case 1:
				mem_request->request_type=WRITE;
				mem_request->value=rand()%MAX_VALUE;
				printf("Input request:\nAddress:%lu\tType:write\tvalue:%02X\n",mem_request->virtual_address,mem_request->value);
				break;
			case 2:
				mem_request->request_type=EXECUTE;
				printf("Input request:\nAddress:%lu\tType:execute\n",mem_request->virtual_address);
				break;
			default:
				break;
		}
		write_num=fwrite(mem_request,sizeof(MemoryAccessRequest),1,fp);
		fclose(fp);
		return 0;
	}
	if((c=getchar())=='2')
	{
		printf("How many requests do you need?");
		scanf("%d",&n);
		srand(time(NULL));
		MemoryAccessRequestPtr mem_request0;
		mem_request0=(MemoryAccessRequestPtr)malloc(sizeof(MemoryAccessRequest));
		//mkfifo(FIFO,S_IFIFO|0666,0);
		while(i<n)
		{
			if((fp=fopen(FIFO,"a+"))==NULL)
			{
				//handle_error(FILE_OPEN_FAILED);
				exit(1);
			}
			do_request(mem_request0);
			printf("Success request : %d\n",++i);
			write_num=fwrite(mem_request0,sizeof(MemoryAccessRequest),1,fp);
		}
		fclose(fp);
		return 0;
	}
}
void do_request(MemoryAccessRequestPtr mem_request)
{
	
	mem_request->virtual_address=rand()%VIRTUAL_MEMORY_SIZE;

	switch (rand()%3)
	{
		case 0:			/*读请求*/
			mem_request->request_type=READ;
			printf("Produce request:\nAddress:%lu\tType:read\n",mem_request->virtual_address);
			break;
		case 1:			/*写请求*/
			mem_request->request_type=WRITE;
			mem_request->value=rand()%MAX_VALUE;
			printf("Produce request:\nAddress:%lu\tType:write\tvalue:%02X\n",mem_request->virtual_address,mem_request->value);
			break;
		case 2:			/*执行请求*/
			mem_request->request_type=EXECUTE;
			printf("Produce request:\nAddress:%lu\tType:execute\n",mem_request->virtual_address);
			break;
		default:
			break;
	}	
}
