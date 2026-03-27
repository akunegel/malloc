#include "../include/malloc.h"

size_t align_block_size(size_t size)
{
    return (size + 15) & ~15;
}

int init_zones(void)
{
    zones_blocks.tiny = mmap(NULL, TINY_ZONE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (zones_blocks.tiny == MAP_FAILED)
        return (0);
    zones_blocks.tiny->size = TINY_ZONE_SIZE - sizeof(t_block);
    zones_blocks.tiny->free = 1;
    zones_blocks.tiny->next = NULL;

    zones_blocks.small = mmap(NULL, SMALL_ZONE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (zones_blocks.small == MAP_FAILED)
        return (0);
    zones_blocks.small->size = TINY_ZONE_SIZE - sizeof(t_block);
    zones_blocks.small->free = 1;
    zones_blocks.small->next = NULL;

    zones_blocks.large = NULL;

    return (1);
}
