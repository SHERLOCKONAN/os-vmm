#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#include "vmm.h"
void do_error(ERROR_CODE code)
{
	switch (code)
	{
		case ERROR_READ_DENY:
		{
			printf("�ô�ʧ�ܣ��õ�ַ���ݲ��ɶ�\n");
			break;
		}
		case ERROR_WRITE_DENY:
		{
			printf("�ô�ʧ�ܣ��õ�ַ���ݲ���д\n");
			break;
		}
		case ERROR_EXECUTE_DENY:
		{
			printf("�ô�ʧ�ܣ��õ�ַ���ݲ���ִ��\n");
			break;
		}		
		case ERROR_INVALID_REQUEST:
		{
			printf("�ô�ʧ�ܣ��Ƿ��ô�����\n");
			break;
		}
		case ERROR_OVER_BOUNDARY:
		{
			printf("�ô�ʧ�ܣ���ַԽ��\n");
			break;
		}
		case ERROR_FILE_OPEN_FAILED:
		{
			printf("ϵͳ���󣺴��ļ�ʧ��\n");
			break;
		}
		case ERROR_FILE_CLOSE_FAILED:
		{
			printf("ϵͳ���󣺹ر��ļ�ʧ��\n");
			break;
		}
		case ERROR_FILE_SEEK_FAILED:
		{
			printf("ϵͳ�����ļ�ָ�붨λʧ��\n");
			break;
		}
		case ERROR_FILE_READ_FAILED:
		{
			printf("ϵͳ���󣺶�ȡ�ļ�ʧ��\n");
			break;
		}
		case ERROR_FILE_WRITE_FAILED:
		{
			printf("ϵͳ����д���ļ�ʧ��\n");
			break;
		}
		default:
		{
			printf("δ֪����û������������\n");
		}
	}
}
int main(int argc, char* argv[])
{
	char c;
	char Request[20]={'\0'};
	char CurrentRequest[20]={'\0'};
	FILE *fp;
	while (TRUE)
	{
		if(!(fp = fopen("MYFIFO","a+"))){
			do_error(ERROR_FILE_OPEN_FAILED);
			exit(1);
		}
		scanf("%s",Request);
		scanf("%s", Request);
		if(fwrite(Request,sizeof(BYTE),strlen(Request),fp)<strlen(Request)){
			printf("ϵͳ����д���ļ�ʧ��\n");
			break;
		}
		fputc('\n',fp);
		fclose(fp);
		}
	return (0);
}