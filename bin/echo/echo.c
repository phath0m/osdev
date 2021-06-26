#include <stdio.h>

__attribute__((noinline))
void
a_test_func(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++) {
        printf("%s", argv[i]);
    }
}

int
main(int argc, char *argv[])
{
    a_test_func(argc, argv); 
    puts("");

    return 0;
}

