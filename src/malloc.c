/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <akunegel@student.42.fr>           +#+  +:+       +#+       */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/01 10:15:55 by elotana           #+#    #+#             */
/*   Updated: 2026/10/01 10:15:57 by elotana          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

t_heap heap = {NULL, NULL, NULL, 0};
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static void *allocate(t_zone_type type, size_t size)
{
	t_zone *zone;
	t_block *block;

	zone = init_zone(type, size);
	if (!zone)
		return NULL;
	block = find_free_block(zone, size);
	if (!block) {
		errno = ENOMEM;
		return NULL;
	}
	split_block(block, size);
	block->is_free = 0;
	update_max_free_size(zone);
	return (char *)block + BLOCK_HDR;
}

static void *large_allocation(size_t size)
{
	t_zone *zone = need_init(ZONE_LARGE, size);
	if (!zone) {
		return NULL;
	}
	return (char *)zone->blocks + BLOCK_HDR;
}

void *malloc_impl(size_t size)
{
	if (size == 0) {
		size = 1;
	}

	if (size > SIZE_MAX - 15 - BLOCK_HDR - ZONE_HDR - get_page_size()) {
		errno = ENOMEM;
		return NULL;
	}

	size = ALIGN16(size);

	if (size <= T_MAX_SIZE) {
		return allocate(ZONE_TINY, size);
	} else if (size <= S_MAX_SIZE) {
		return allocate(ZONE_SMALL, size);
	} else {
		return large_allocation(size);
	}
}

void	*malloc(size_t size)
{
	pthread_mutex_lock(&g_lock);
	void *ret = malloc_impl(size);
	pthread_mutex_unlock(&g_lock);
	return ret;
}