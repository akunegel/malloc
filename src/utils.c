/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <akunegel@student.42.fr>           +#+  +:+       +#+       */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/01 10:16:08 by elotana           #+#    #+#             */
/*   Updated: 2026/10/01 10:16:09 by elotana          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

size_t get_page_size(void)
{
	long ret;

	if (!heap.page_size) {
		ret = sysconf(_SC_PAGESIZE);
		heap.page_size = (ret > 0) ? (size_t)ret : 4096;
	}
	return heap.page_size;
}

void update_max_free_size(t_zone *zone)
{
	t_block *block;

	zone->max_free_size = 0;
	block = zone->blocks;
	while (block) {
		if (block->is_free && block->size > zone->max_free_size)
			zone->max_free_size = block->size;
		block = block->next;
	}
}

t_block *find_free_block(t_zone *zone, size_t size)
{
	t_block *block;

	block = zone->blocks;
	while (block) {
		if (block->is_free && block->size >= size)
			return block;
		block = block->next;
	}
	return NULL;
}

void split_block(t_block *block, size_t size)
{
	t_block *rest;

	if (block->size < size + BLOCK_HDR + 16)
		return;
	rest = (t_block *)((char *)block + BLOCK_HDR + size);
	rest->size = block->size - size - BLOCK_HDR;
	rest->is_free = 1;
	rest->next = block->next;
	rest->prev = block;
	rest->zone = block->zone;
	if (block->next)
		block->next->prev = rest;
	block->next = rest;
	block->size = size;
}

void	fatal(void)
{
	write(2, "malloc: fatal error\n", 20);
	__builtin_trap();
}
