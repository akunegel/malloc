/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <marvin@42.fr>                     +#+  +:+       +#+       */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 16:39:38 by elotana           #+#    #+#             */
/*   Updated: 2026/10/02 16:39:43 by elotana          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

static void copy_bytes(void *dst, void *src, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	unsigned char *s = (unsigned char *)src;
	while (n--)
		*d++ = *s++;
}

static void *realloc_impl(void *ptr, size_t size)
{
	if (size == 0) {
		free_impl(ptr);
		return NULL;
	}
	if (size > SIZE_MAX - 15 - BLOCK_HDR - ZONE_HDR - get_page_size()) {
		errno = ENOMEM;
		return NULL;
	}

	size = ALIGN16(size);

	if (!ptr)
		return malloc_impl(size);

	t_zone *zone = find_zone(ptr);
	if (!zone) {
		errno = EINVAL;
		return NULL;
	}

	t_block *b = zone->blocks;
	while (b && (char *)b + BLOCK_HDR != (char *)ptr)
		b = b->next;

	if (!b || b->is_free)
		return NULL;

	if (b->size >= size) {
		return (char *)b + BLOCK_HDR;
	}

	if (b->next && b->next->is_free &&
		b->size + BLOCK_HDR + b->next->size >= size) {
		t_block *n = b->next;
		b->size += BLOCK_HDR + n->size;
		b->next = n->next;
		if (b->next) {
			b->next->prev = b;
		}
		split_block(b, size);
		update_max_free_size(zone);
		return (char *)b + BLOCK_HDR;
	} else {
		void *new = malloc_impl(size);
		if (!new)
			return NULL;
		copy_bytes(new, ptr, b->size < size ? b->size : size);
		free_impl(ptr);
		return new;
	}
}

void *realloc(void *ptr, size_t size)
{
	pthread_mutex_lock(&g_lock);
	void *ret = realloc_impl(ptr, size);
	pthread_mutex_unlock(&g_lock);
	return ret;
}
