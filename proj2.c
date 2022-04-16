#include <stdio.h>
#include <stdlib.h>

int parseInt(char *src, long *dest){
    char *ptr;
    *dest = strtol(src,&ptr,10);
    printf("%ld \n",*dest);
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
    if(parseInt(argv[1], &no)){
        return 1;
    }
    if (parseInt(argv[2], &nh)){
        return 1;
    }
    if (parseInt(argv[3], &ti) || !isValidTime(ti))
    {
        return 1;
    }
    if (parseInt(argv[4], &tb) || !isValidTime(tb))
    {
        return 1;
    }
}