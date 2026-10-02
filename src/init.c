/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: elotana <akunegel@student.42.fr>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/01 10:16:02 by elotana           #+#    #+#             */
/*   Updated: 2026/10/01 10:16:03 by elotana          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

static t_block *create_first_block(size_t size, t_zone_type type, t_zone *zone)
{
	t_block *block;

	block = (t_block *)((char *)zone + ZONE_HDR);
	zone->prev = NULL;
	zone->next = NULL;
	zone->zone_size = size;
	zone->type = type;
	zone->blocks = block;
	block->is_free = 1;
	block->next = NULL;
	block->prev = NULL;
	block->zone = zone;
	block->size = size - ZONE_HDR - BLOCK_HDR;
	zone->max_free_size = block->size;

	return block;
}

static void add_zone(t_zone *zone, t_zone_type type, size_t size)
{
	t_zone **head;
	t_zone *tmp;

	if (type == ZONE_TINY) {
		head = &heap.tiny;
	} else if (type == ZONE_SMALL) {
		head = &heap.small;
	} else {
		head = &heap.large;
	}

	zone->blocks = create_first_block(size, type, zone);
	tmp = *head;

	if (!tmp || zone < tmp) {
		zone->next = tmp;
		if (tmp)
			tmp->prev = zone;
		*head = zone;
		return;
	}

	while (tmp->next && tmp->next < zone)
		tmp = tmp->next;
	zone->next = tmp->next;
	zone->prev = tmp;
	if (tmp->next)
		tmp->next->prev = zone;
	tmp->next = zone;
}

t_zone *need_init(t_zone_type type, size_t size)
{
	t_zone *zone;
	size_t req;

	req = size;
	if (type == ZONE_TINY) {
		size = T_ZONE_SIZE;
	} else if (type == ZONE_SMALL) {
		size = S_ZONE_SIZE;
	} else if (type == ZONE_LARGE) {
		size = ROUND_PAGE(size + BLOCK_HDR + ZONE_HDR);
	}

	zone =
		mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (zone == MAP_FAILED) {
		errno = ENOMEM;
		return NULL;
	}
	add_zone(zone, type, size);

	if (type == ZONE_LARGE) {
		zone->blocks->size = req;
		zone->blocks->is_free = 0;
		zone->max_free_size = 0;
	}

	return zone;
}

t_zone *init_zone(t_zone_type type, size_t size)
{
	t_zone *zone = NULL;
	if (type == ZONE_TINY) {
		zone = heap.tiny;
	} else if (type == ZONE_SMALL) {
		zone = heap.small;
	}

	if (zone) {
		while (zone->next != NULL) {
			if (zone->max_free_size >= size)
				break;
			zone = zone->next;
		}
		if (zone->max_free_size < size)
			zone = NULL;
	}

	if (zone == NULL) {
		zone = need_init(type, size);
		if (!zone) {
			return NULL;
		}
	}

	return zone;
}
