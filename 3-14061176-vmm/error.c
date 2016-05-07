#include <stdio.h>
#include <stdlib.h>
void error_sys(char *s){
    perror(s);
    exit(0);
}
