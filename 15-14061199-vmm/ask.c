#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char** args)
{
	int pid;
	char input[80];
	FILE* fifo;
	if(argc >= 1){
		if(sscanf(args[1],"%d",&pid) == 0)
			return 1;
	}
	else{
		pid = getpid();
	}

	while(fgets(input,80,stdin)){
		sleep(2);
	if((fifo = fopen("fifo","w")) == 0){
		printf("open fifo failed\n");
		return -1;
	}
		if(input[strlen(input)-1] == '\n'){
			input[strlen(input)-1] = '\0';
		}
		fprintf(fifo,"%d %s\n",pid,input);
		fflush(fifo);
		fclose(fifo);
	}
	return 0;

}
