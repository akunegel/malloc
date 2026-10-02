/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <marvin@42.fr>                     +#+  +:+       +#+       */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/01 17:48:19 by elotana           #+#    #+#             */
/*   Updated: 2026/10/01 17:48:25 by elotana          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

t_zone *find_zone(void *ptr)
{
	t_zone *lists[3] = {heap.tiny, heap.small, heap.large};
	char *p = (char *)ptr;

	for (int i = 0; i < 3; i++) {
		for (t_zone *z = lists[i]; z; z = z->next) {
			if (p >= (char *)z && p < (char *)z + z->zone_size)
				return z;
		}
	}
	return NULL;
}

static void release_zone(t_zone *zone)
{
	if (zone->prev == NULL) {
		if (zone->type == ZONE_TINY) {
			heap.tiny = heap.tiny->next;
		} else if (zone->type == ZONE_SMALL) {
			heap.small = heap.small->next;
		} else if (zone->type == ZONE_LARGE) {
			heap.large = heap.large->next;
		}
	}

	if (zone->prev) {
		zone->prev->next = zone->next;
	}
	if (zone->next) {
		zone->next->prev = zone->prev;
	}

	if (munmap(zone, zone->zone_size) < 0)
		fatal();
}

void free_impl(void *ptr)
{
	if (!ptr)
		return;

	t_zone *zone = find_zone(ptr);
	if (!zone)
		return;

	t_block *b = zone->blocks;
	while (b && (char *)b + BLOCK_HDR != (char *)ptr)
		b = b->next;

	if (!b || b->is_free)
		return;

	b->is_free = 1;

	if (b->next && b->next->is_free) {
		t_block *n = b->next;
		b->size += BLOCK_HDR + n->size;
		b->next = n->next;
		if (b->next) {
			b->next->prev = b;
		}
	}
	if (b->prev && b->prev->is_free) {
		t_block *p = b->prev;
		p->size += BLOCK_HDR + b->size;
		p->next = b->next;
		if (b->next) {
			p->next->prev = p;
		}
	}

	if (!zone->blocks->next && zone->blocks->is_free) {
		release_zone(zone);
	} else {
		update_max_free_size(zone);
	}
}

void free(void *ptr)
{
	pthread_mutex_lock(&g_lock);
	free_impl(ptr);
	pthread_mutex_unlock(&g_lock);
}
