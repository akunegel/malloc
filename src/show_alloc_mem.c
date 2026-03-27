/*
 * Output should be
 * TINY : 0xA0000
 * 0xA0020 - 0xA004A : 42 bytes
 * 0xA006A - 0xA00BE : 84 bytes
 * tiny : 0xAD000
 * 0xAD020 - 0xADEAD : 3725 bytes
 * LARGE : 0xB0000
 * 0xB0020 - 0xBBEEF : 48847 bytes
 * Total : 52698 bytes
 */

#include <stdio.h>
#include "../include/malloc.h"

static size_t print_zone(const char *zone_name, t_block *block)
{
    size_t total_zone = 0;

    if (block == NULL)
        return (0);

    printf("%s : %p\n", zone_name, (void *)block);

    while (block != NULL)
    {
        if (block->free == 0)
        {
            void *start_data = (void *)((char *)block + sizeof(t_block));
            void *end_data = (void *)((char *)start_data + block->size);

            printf("%p - %p : %zu bytes\n", start_data, end_data, block->size);
            
            total_zone += block->size;
        }
        block = block->next;
    }
    return (total_zone);
}

void show_alloc_mem(void)
{
    size_t total = 0;

    total += print_zone("tiny", zones_blocks.tiny);
    total += print_zone("small", zones_blocks.small);
    total += print_zone("LARGE", zones_blocks.large);

    printf("Total : %zu bytes\n", total);
}
