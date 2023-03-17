#include <err.h>
#include <time.h>
#include <stdio.h>


extern void do_stuff(void);


int main(int argc, char *argv[])
{
    struct timespec begin, end;

    printf("Calling do_stuff()!\n");

    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin) != 0)
    {
        err(-1, "Failed to get start time");
    }

    do_stuff();

    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) != 0)
    {
        err(-1, "Failed to get end time");
    }

    printf("Back in main() again.\n");

    return 0;
}