#include "page_table.h"
#include "vmm.h"
#include <string.h>
#include "proc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

char input_buffer[80];

ERROR_CODE proc_read(pid_t pid,ushort address,uchar* opnum)
{
	return read_from(pid,address,opnum);
}
ERROR_CODE proc_write(pid_t pid,ushort address, uchar data)
{
	return write_to(pid,address,data);
}
ERROR_CODE proc_exe(pid_t pid,ushort address)
{
	return execute_at(pid,address);
}

int process_input()
{
	static char opt[10];
	pid_t pid;
	ushort address;
	ushort opnum;
	int n;
	ERROR_CODE err = ERROR_OK;
	n = sscanf(input_buffer, "%d %s %hu %hu", &pid, opt,&address,&opnum);
	if(n < 2){
		printf("缺少参数\n");
		fflush(stdout);
		return 0;
	}
	if(strcmp(opt,"READ") == 0){		//
		uchar res;
		if(n != 3){
			printf("格式不正确：\npid READ address\n");
			fflush(stdout);
			return 0;
		}
		err = proc_read(pid,address,&res);
		if(!err){
			printf("read res = %x\n",res);
			fflush(stdout);
		}
	}
	else if(strcmp(opt,"WRITE") == 0){
		if(n != 4){
			printf("格式不正确：\npid WRITE address data\n");
			fflush(stdout);
			return 0;
		}
		err = proc_write(pid,address,(uchar)opnum);
		if(!err){
			printf("write %d succeeded\n",(uchar)opnum);
			fflush(stdout);
		}
	}
	else if(strcmp(opt,"EXECUTE") == 0){
		if(n != 3){
			printf("格式不正确：\npid EXECUTE address\n");
			fflush(stdout);
			return 0;
		}
		else{
			err = proc_exe(pid,address);
		}
		
	}
	else if(strcmp(opt,"END") == 0){
		if(n != 2){
			printf("格式不正确：\npid END\n");
			fflush(stdout);
			return 0;
		}
		if(pid == 0)
			return 1;
		rmProc(pid);
	}
	else if(strcmp(opt,"CREATE") == 0){
		int i;
		if(n != 2){
			printf("格式不正确：\npid END\n");
			fflush(stdout);
			return 0;
		}
		i = addProc(pid);
		if(-1 == i){
			printf("创建进程%d 失败",pid);
			fflush(stdout);
			return 0;
		}
		else if(1 == i){
			printf("进程%d 已存在",pid);
			fflush(stdout);
			return 0;
		}
		else{
		}
	}
	else if(strcmp(opt,"PRINT") == 0){
		if(n != 2){
			printf("格式不正确：\npid PRINT\n");
			fflush(stdout);
			return 0;
		}
		proc_print(pid);
	}
	else{
		printf("指令不正确：\nREAD|WRITE|EXECUTE|END|CREATE\n");
		fflush(stdout);
		return 0;
	}
	if(err)
	 do_error(err);
	input_buffer[strlen(input_buffer)-1]='\0';
	printf("-------------------%s---------------------\n",input_buffer);
	fflush(stdout);
	return 0;
}
void close_z()
{
}
void init()
{
	do_init_mm();
}
int main(){
	init();
	fd_set rdst;
	
	int fdout;
	mkfifo("fifo",0777);
	mkfifo("fifo_out",0777);
	fdout = open("fifo_out",O_WRONLY);
	dup2(fdout,1);
	FD_ZERO(&rdst);
	do{
 		FILE* fi = fopen("fifo","r");
		FD_SET(fileno(fi),&rdst);
		FD_SET(fileno(stdin),&rdst);
		select(fileno(fi)+1,&rdst,NULL,NULL,NULL);
		if(FD_ISSET(fileno(stdin),&rdst)){
			fgets(input_buffer, 80,stdin);
				if(process_input()!=0)
					goto B;
		}
		if(FD_ISSET(fileno(fi),&rdst)){
			while((fgets(input_buffer, 80,fi))){
				if(process_input()!=0)
					goto B;
			}
		}
		fclose(fi);
	}while(1);
B:	close_z();
}
