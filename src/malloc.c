#include "../include/malloc.h"
#include <stdio.h>

t_heap zones_blocks;

void	*tiny_allocation(size_t size)
{
	t_block	*block = zones_blocks.tiny;

	while (block != NULL)
	{
		if (block->free == 1 && block->size >= size)
			break;
		block = block->next;
	}

	if (block == NULL)
	{
		printf("No more space\n");
		return NULL;
	}

	if (block->size >= size + sizeof(t_block) + 16)
	{
		t_block *new_block = (t_block *)((char *)block + sizeof(t_block) + size);

		new_block->size = block->size - size - sizeof(t_block);
		new_block->free = 1;
		new_block->next = block->next;

		block->size = size;
		block->free = 0;
		block->next = new_block;
	}
	else
	{
		block->free = 0;
	}

	return (void *)((char *)block + sizeof(t_block));
}

void	*small_allocation(size_t size)
{
	t_block	*block = zones_blocks.small;

	while (block != NULL)
	{
		if (block->free == 1 && block->size >= size)
			break;
		block = block->next;
	}

	if (block == NULL)
		return NULL;

	if (block->size >= size + sizeof(t_block) + 16)
	{
		t_block *new_block = (t_block *)((char *)block + sizeof(t_block) + size);

		new_block->size = block->size - size - sizeof(t_block);
		new_block->free = 1;
		new_block->next = block->next;

		block->size = size;
		block->free = 0;
		block->next = new_block;
	}
	else
	{
		block->free = 0;
	}

	return (void *)((char *)block + sizeof(t_block));
}

void	*large_allocation(size_t size)
{
	t_block	*block = zones_blocks.large;

	while (block != NULL)
	{
		if (block->free == 1 && block->size >= size)
			break;
		block = block->next;
	}

	if (block == NULL)
	{
		size_t mmap_size = align_block_size(size);
		t_block *new_zone = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		
		if (new_zone == MAP_FAILED)
			return NULL;
		
		new_zone->size = mmap_size - sizeof(t_block);
		new_zone->free = 1;
		new_zone->next = zones_blocks.large;
		zones_blocks.large = new_zone;
		
		block = new_zone;
	}

	if (block->size >= size + sizeof(t_block) + 16)
	{
		t_block *new_block = (t_block *)((char *)block + sizeof(t_block) + size);

		new_block->size = block->size - size - sizeof(t_block);
		new_block->free = 1;
		new_block->next = block->next;

		block->size = size;
		block->free = 0;
		block->next = new_block;
	}
	else
	{
		block->free = 0;
	}

	return (void *)((char *)block + sizeof(t_block));
	
}

void    *malloc(size_t size)
{
    if (size == 0)
        return NULL;

    static int initialized = 0;
    if (!initialized)
    {
        if (!init_zones())
            return NULL;
        initialized = 1;
    }

    size_t needed_size = align_block_size(size);
	printf("%zu\n", needed_size);
    if (needed_size <= MAX_TINY_BLOCK) {
        return tiny_allocation(needed_size); 
    } else if (needed_size <= MAX_SMALL_BLOCK) {
        return small_allocation(needed_size);
    } else {
        return large_allocation(needed_size);
    }
	return NULL;
}
