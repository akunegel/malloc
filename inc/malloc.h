/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: elotana <akunegel@student.42.fr>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/01 10:15:37 by elotana           #+#    #+#             */
/*   Updated: 2026/10/01 10:15:43 by elotana          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <errno.h>
# include <stddef.h>
# include <stdint.h>
# include <sys/mman.h>
# include <unistd.h>
# include <pthread.h>

# define T_MAX_SIZE ((size_t)128)
# define S_MAX_SIZE ((size_t)1024)

# define ALIGN16(x) (((x) + 15) & ~(size_t)15)
# define ZONE_HDR ALIGN16(sizeof(t_zone))
# define BLOCK_HDR ALIGN16(sizeof(t_block))

# define ROUND_PAGE(x) ((((x) + get_page_size() - 1) / get_page_size()) * get_page_size())

# define T_ZONE_SIZE ROUND_PAGE((T_MAX_SIZE + BLOCK_HDR) * 100 + ZONE_HDR)
# define S_ZONE_SIZE ROUND_PAGE((S_MAX_SIZE + BLOCK_HDR) * 100 + ZONE_HDR)

typedef enum e_zone_type
{
	ZONE_TINY,
	ZONE_SMALL,
	ZONE_LARGE
}			t_zone_type;

typedef struct s_block
{
	size_t			size;
	int				is_free;
	struct s_block	*next;
	struct s_block	*prev;
	struct s_zone	*zone;
}				t_block;

typedef struct s_zone
{
	struct s_zone	*next;
	struct s_zone	*prev;
	t_zone_type		type;
	size_t			zone_size;
	size_t			max_free_size;
	t_block			*blocks;
}				t_zone;

typedef struct s_heap
{
	t_zone	*tiny;
	t_zone	*small;
	t_zone	*large;
	size_t	page_size;
} t_heap;

extern t_heap heap;
extern pthread_mutex_t g_lock;

void	*malloc(size_t size);
void	free(void *ptr);
void	*realloc(void *ptr, size_t size);
void	show_alloc_mem(void);

t_zone	*init_zone(t_zone_type type, size_t size);
t_zone	*need_init(t_zone_type type, size_t size);
size_t	get_page_size(void);
void	update_max_free_size(t_zone *zone);
t_block	*find_free_block(t_zone *zone, size_t size);
void	split_block(t_block *block, size_t size);
void	fatal(void);
t_zone	*find_zone(void *ptr);
void	*malloc_impl(size_t size);
void	free_impl(void *ptr);

#endif
