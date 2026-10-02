#include "../ft_printf/ft_printf.h"
#include "../inc/malloc.h"

static size_t print_zone(t_zone *zone)
{
	t_block	*block;
	size_t	total;

	total = 0;
	block = zone->blocks;
	while (block)
	{
		if (!block->is_free)
		{
			ft_printf("%p - %p : %u bytes\n", (char *)block + BLOCK_HDR,
					  (char *)block + BLOCK_HDR + block->size,
					  (unsigned int)block->size);
			total += block->size;
		}
		block = block->next;
	}
	return (total);
}

static size_t print_type(t_zone *zone, char *name)
{
	size_t	total;

	total = 0;
	if (!zone)
	{
		ft_printf("%s : 0x0\n", name);
		return (0);
	}
	while (zone)
	{
		ft_printf("%s : %p\n", name, (void *)zone);
		total += print_zone(zone);
		zone = zone->next;
	}
	return (total);
}

void show_alloc_mem(void)
{
	size_t	total;

	total = 0;
	total += print_type(heap.tiny, "TINY");
	total += print_type(heap.small, "SMALL");
	total += print_type(heap.large, "LARGE");
	ft_printf("Total : %u bytes\n", (unsigned int)total);
}
