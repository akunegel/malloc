#ifndef MALLOC_H
# define MALLOC_H

# define MAX_TINY_BLOCK  (size_t)(4 * sysconf(_SC_PAGESIZE))
# define MAX_SMALL_BLOCK (size_t)(32 * sysconf(_SC_PAGESIZE))

#define TINY_ZONE_SIZE  (size_t)(MAX_TINY_BLOCK * 100)
#define SMALL_ZONE_SIZE (size_t)(MAX_SMALL_BLOCK * 100)

# include <stddef.h>
# include <unistd.h>
# include <sys/mman.h>

typedef struct	s_block {
    size_t			size;
    int				free;
    struct s_block	*next;
} t_block;

typedef struct	s_heap {
	t_block	*tiny;
	t_block	*small;
	t_block	*large;
} t_heap;

extern t_heap	zones_blocks;

void	free(void *ptr);
void	*malloc(size_t size);
void	*realloc(void *ptr, size_t size);

int		init_zones(void);
size_t	align_block_size(size_t size);

void	show_alloc_mem(void);

#endif

// n = 4 * PAGESIZE
// m = 32 * PAGESIZE
