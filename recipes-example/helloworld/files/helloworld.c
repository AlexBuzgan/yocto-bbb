#include<stdio.h>
#include<unistd.h>

int main(void)
{
    while(1)
    {
        printf("Hello World!\n");
        fflush(stdout);
        sleep(2);
    }
    
    return 0;
}