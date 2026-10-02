#include "../inc/malloc.h"
#include "../ft_printf/ft_printf.h"
#include <string.h>

static int	g_ok;
static int	g_ko;

static void	check(int cond, char *name)
{
	if (cond)
	{
		ft_printf("  [OK] %s\n", name);
		g_ok++;
	}
	else
	{
		ft_printf("  [KO] %s\n", name);
		g_ko++;
	}
}

static int	zone_count(t_zone *z)
{
	int	n;

	n = 0;
	while (z)
	{
		n++;
		z = z->next;
	}
	return (n);
}

static t_zone	*zone_of(void *ptr)
{
	t_zone		*lists[3];
	t_zone		*z;
	uintptr_t	p;
	uintptr_t	start;
	int			i;

	lists[0] = heap.tiny;
	lists[1] = heap.small;
	lists[2] = heap.large;
	p = (uintptr_t)ptr;
	i = 0;
	while (i < 3)
	{
		z = lists[i];
		while (z)
		{
			start = (uintptr_t)z;
			if (p >= start && p < start + z->zone_size)
				return (z);
			z = z->next;
		}
		i++;
	}
	return (NULL);
}

static t_block	*block_of(void *ptr)
{
	return ((t_block *)((char *)ptr - BLOCK_HDR));
}

/*
** One allocation of each flavour: correct zone and writable.
*/
static void	test_three_zones(void)
{
	void	*t;
	void	*s;
	void	*l;

	ft_printf("-- basic zones --\n");
	t = malloc(32);
	s = malloc(500);
	l = malloc(5000);
	check(t != NULL && zone_of(t) && zone_of(t)->type == ZONE_TINY,
		"malloc(32) lives in TINY");
	check(s != NULL && zone_of(s) && zone_of(s)->type == ZONE_SMALL,
		"malloc(500) lives in SMALL");
	check(l != NULL && zone_of(l) && zone_of(l)->type == ZONE_LARGE,
		"malloc(5000) lives in LARGE");
	if (t && s && l)
	{
		memset(t, 0xAA, 32);
		memset(s, 0xBB, 500);
		memset(l, 0xCC, 5000);
		check(*(char *)t == (char)0xAA && *(char *)s == (char)0xBB
			&& *(char *)l == (char)0xCC, "each zone is writable");
	}
	free(t);
	free(s);
	free(l);
	check(zone_count(heap.tiny) == 0 && zone_count(heap.small) == 0
		&& zone_count(heap.large) == 0, "freeing returns every zone");
}

static void	test_bad_frees(void)
{
	void				*p;
	volatile uintptr_t	stack_addr;
	int					x;

	ft_printf("-- bad free arguments --\n");
	free(NULL);
	check(1, "free(NULL) does not crash");

	p = malloc(64);
	free((char *)p + 1);
	check(p && block_of(p)->is_free == 0, "a misaligned free is ignored");
	free((char *)p + 8);
	check(p && block_of(p)->is_free == 0, "an interior free is ignored");
	free(p);
	check(zone_count(heap.tiny) == 0, "the real free still releases the zone");

	free((void *)(uintptr_t)0x1000);
	check(1, "freeing an unmapped address does not crash");
	x = 0;
	stack_addr = (uintptr_t)&x;
	free((void *)stack_addr);
	check(1, "freeing a stack address does not crash");
}

static void	test_double_free(void)
{
	void	*p;
	void	*q;

	ft_printf("-- double free --\n");
	p = malloc(32);
	free(p);
	free(p);
	q = malloc(32);
	check(q != NULL, "the allocator still works after a double free");
	if (q)
	{
		*(char *)q = 1;
		check(*(char *)q == 1, "memory obtained after a double free is usable");
		free(q);
	}
	p = malloc(16);
	q = malloc(16);
	free(p);
	free(p);
	free(q);
	check(zone_count(heap.tiny) == 0, "double free does not leak or corrupt the zone");
}

static void	test_free_after_release(void)
{
	void	*p;

	ft_printf("-- free after the zone was unmapped --\n");
	p = malloc(16);
	free(p);
	free(p);
	check(1, "freeing a pointer whose zone is gone does not crash");
	p = malloc(16);
	check(p != NULL, "the allocator keeps working afterwards");
	free(p);
	check(zone_count(heap.tiny) == 0, "state stays consistent");
}

static void	test_show_alloc_mem(void)
{
	void	*t;
	void	*s;
	void	*l;

	ft_printf("-- show_alloc_mem --\n");
	t = malloc(32);
	s = malloc(500);
	l = malloc(5000);
	show_alloc_mem();
	check(t && s && l, "show_alloc_mem runs on a populated heap");
	free(t);
	free(s);
	free(l);
	check(zone_count(heap.tiny) == 0 && zone_count(heap.small) == 0
		&& zone_count(heap.large) == 0, "heap is clean after show_alloc_mem");
}

int	main(void)
{
	ft_printf("== GENERAL TESTS ==\n");
	test_three_zones();
	test_bad_frees();
	test_double_free();
	test_free_after_release();
	test_show_alloc_mem();
	ft_printf("== RESULT: %d passed, %d failed ==\n", g_ok, g_ko);
	return (g_ko != 0);
}
