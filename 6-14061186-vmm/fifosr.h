#ifndef FIFOSR_H
#define FIFOSR_H
#endif


int receive(char *addr, int len, int fd);

int receiveUntilChar(char *addr,char endSign, int fd);

int connect(char *fifo, char op, int *fd);

int arrival(char *fifo, int *fd, int spid);

void disconnect(int fd);
