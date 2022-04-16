#include <stdio.h>
#include <stdlib.h>

int parseInt(char *src, long *dest){
    char *ptr;
    *dest = strtol(src,&ptr,10);

    if (ptr[0] != '\0'){
        fprintf(stderr, "Unexpected character: %s\n", ptr);
        return 1;
    }
    return 0;
}

int isValidTime(int time) {
    if (time >=0 && time <= 1000){
        return 1;
    }
    fprintf(stderr, "Invalid time, expected: 0-1000, got: %d.\n", time);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "Invalid count of arguments, expected 4 got: %d\n", argc-1);
        return 1;
    }

    long no, nh, ti, tb;
    int errorOccurred = 0;
    
    errorOccurred += parseInt(argv[1], &no);
    errorOccurred += parseInt(argv[2], &nh);
    errorOccurred += parseInt(argv[3], &ti);
    errorOccurred += parseInt(argv[4], &tb);
    errorOccurred += !isValidTime(ti);
    errorOccurred += !isValidTime(tb);
    if (errorOccurred){
        return 1;
    }
}