#include <stdio.h>
void error_sys(char *s){
    perror(s);
    exit(0);
}
