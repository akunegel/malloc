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

static int	pattern_ok(void *ptr, size_t n, unsigned char v)
{
	unsigned char	*c;
	size_t			i;

	c = ptr;
	i = 0;
	while (i < n)
	{
		if (c[i] != v)
			return (0);
		i++;
	}
	return (1);
}

/*
** limits: small + 1, page sized, huge, zero and unrepresentable sizes.
*/
static void	test_limits(void)
{
	void	*p;

	ft_printf("-- limits --\n");
	p = malloc(S_MAX_SIZE + 1);
	check(p != NULL, "malloc(1025) returns a non-NULL pointer");
	if (p)
	{
		check(zone_of(p) && zone_of(p)->type == ZONE_LARGE,
			"malloc(1025) lives in LARGE (small + 1)");
		free(p);
	}
	p = malloc(4096);
	check(p != NULL && zone_of(p) && zone_of(p)->type == ZONE_LARGE,
		"malloc(4096) lives in LARGE");
	free(p);
	p = malloc(1 << 20);
	check(p != NULL && zone_of(p) && zone_of(p)->type == ZONE_LARGE,
		"malloc(1MB) lives in LARGE");
	free(p);
	p = malloc(0);
	check(p != NULL, "malloc(0) returns a non-NULL pointer");
	if (p)
	{
		check(zone_of(p) && zone_of(p)->type == ZONE_TINY,
			"malloc(0) still lives in TINY");
		free(p);
	}
	p = malloc(SIZE_MAX);
	check(p == NULL, "malloc(SIZE_MAX) returns NULL");
	p = malloc(SIZE_MAX - 4096);
	check(p == NULL, "malloc(SIZE_MAX - 4096) returns NULL");
}

/*
** Every large request gets its own, single-block zone.
*/
static void	test_isolated_zone(void)
{
	void	*p;
	t_zone	*z;
	int		before;

	ft_printf("-- one zone per large allocation --\n");
	before = zone_count(heap.large);
	p = malloc(2000);
	check(p != NULL, "malloc(2000) returns a non-NULL pointer");
	if (p)
	{
		z = zone_of(p);
		check(z && z->type == ZONE_LARGE, "malloc(2000) lives in LARGE");
		check(zone_count(heap.large) == before + 1,
			"exactly one new LARGE zone is mapped");
		check(z && z->blocks->next == NULL && z->blocks->prev == NULL,
			"the LARGE zone holds a single block");
		free(p);
	}
	check(zone_count(heap.large) == before,
		"freeing a LARGE block releases its zone");
}

/*
** The usable size must cover the request; the mapping is page rounded.
*/
static void	test_usable_size(void)
{
	size_t	sizes[] = {1025, 2000, 4096, 65536};
	void	*p[8];
	t_block	*b;
	int		n;
	int		i;
	int		ok;
	size_t	j;

	ft_printf("-- usable size vs requested --\n");
	n = (int)(sizeof(sizes) / sizeof(sizes[0]));
	i = 0;
	while (i < n)
	{
		p[i] = malloc(sizes[i]);
		i++;
	}
	ok = 1;
	i = 0;
	while (i < n)
	{
		b = block_of(p[i]);
		ok &= (p[i] != NULL);
		ok &= (b->size >= sizes[i]);
		ok &= ((b->size & 15) == 0);
		ok &= (b->is_free == 0);
		j = 0;
		while (j < sizes[i])
		{
			((unsigned char *)p[i])[j] = (unsigned char)(i + 1);
			j++;
		}
		i++;
	}
	check(ok, "each large request is fully writable and marked used");
	ok = 1;
	i = 0;
	while (i < n)
	{
		if (!pattern_ok(p[i], sizes[i], (unsigned char)(i + 1)))
			ok = 0;
		i++;
	}
	check(ok, "no overlap between large blocks");
	i = 0;
	while (i < n)
	{
		free(p[i]);
		i++;
	}
	check(zone_count(heap.large) == 0, "all LARGE zones are released");
}

static void	test_free_orders(void)
{
	void	*ptrs[40];
	int		n;
	int		i;
	int		ok;

	ft_printf("-- free in mixed order --\n");
	n = 40;
	i = 0;
	while (i < n)
	{
		ptrs[i] = malloc(1500 + i * 137);
		if (ptrs[i])
			memset(ptrs[i], (unsigned char)i, 1500);
		i++;
	}
	ok = 1;
	i = 0;
	while (i < n)
	{
		if (!ptrs[i] || !pattern_ok(ptrs[i], 1500, (unsigned char)i))
			ok = 0;
		i++;
	}
	check(ok, "40 large allocations keep their contents");
	i = 0;
	while (i < n)
	{
		free(ptrs[i]);
		i += 2;
	}
	i = 1;
	while (i < n)
	{
		free(ptrs[i]);
		i += 2;
	}
	check(zone_count(heap.large) == 0, "all LARGE zones are released after mixed frees");
}

static void	test_edges(void)
{
	void	*a;
	void	*b;
	uintptr_t	ia;
	uintptr_t	ib;

	ft_printf("-- edges --\n");
	a = malloc(5000);
	b = malloc(5000);
	check(a && b && a != b, "two large allocations return distinct pointers");
	if (a && b)
	{
		ia = (uintptr_t)a;
		ib = (uintptr_t)b;
		check((ia & 15) == 0 && (ib & 15) == 0, "large pointers are 16-byte aligned");
		check(ia + 5000 <= ib || ib + 5000 <= ia, "large blocks do not overlap");
		memset(a, 0x11, 5000);
		memset(b, 0x22, 5000);
		check(pattern_ok(a, 5000, 0x11) && pattern_ok(b, 5000, 0x22),
			"large blocks stay independent");
	}
	free(a);
	free(b);
	check(zone_count(heap.large) == 0, "large zones released");
}

int	main(void)
{
	ft_printf("== LARGE TESTS ==\n");
	test_limits();
	test_isolated_zone();
	test_usable_size();
	test_free_orders();
	test_edges();
	ft_printf("== RESULT: %d passed, %d failed ==\n", g_ok, g_ko);
	return (g_ko != 0);
}
