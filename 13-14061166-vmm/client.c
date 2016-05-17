#include <stdio.h>
#include <stdlib.h>
#define FIFO "MYFIFO"

int main()
{
	FILE *fp;

	char str[1000];
	char c;
	int i=0;
	/* 打开FIFO */
	while(1){
		if((fp=fopen(FIFO,"w"))==NULL){
		perror("open");
		exit(1);
		}
		printf("开始输入数据，回车键发送\n");
		printf("输入e结束\n");
		i=-1;
		str[++i]=getchar();
		if(str[i]=='e'||str[i]=='E'){
			str[i+1]='\0';
			break;
		}
		while(str[i]!='\n'){
			str[++i]=getchar();
		}
		fputs(str,fp);
	
		fclose(fp);
	}
	fputs(str,fp);
	fclose(fp);	
	return 0;
}
