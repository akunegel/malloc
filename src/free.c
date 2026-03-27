#include "../include/malloc.h"

void    free(void *ptr)
{
    if (ptr)
        write(1, "I free\n", 7);
    write(1, "I don't free\n", 13);
}
