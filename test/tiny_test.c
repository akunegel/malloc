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
** limits: smallest and largest tiny sizes, the tiny + 1 boundary,
** zero and unrepresentable sizes.
*/
static void	test_limits(void)
{
	void	*p;
	size_t	usable;

	ft_printf("-- limits --\n");
	p = malloc(0);
	check(p != NULL, "malloc(0) returns a non-NULL pointer");
	if (p)
	{
		check(zone_of(p) && zone_of(p)->type == ZONE_TINY, "malloc(0) lives in TINY");
		*(char *)p = 42;
		check(*(char *)p == 42, "malloc(0) is writable");
		free(p);
	}
	p = malloc(1);
	check(p != NULL, "malloc(1) returns a non-NULL pointer");
	if (p)
	{
		check(zone_of(p) && zone_of(p)->type == ZONE_TINY, "malloc(1) lives in TINY");
		usable = block_of(p)->size;
		check(usable >= 1, "malloc(1) usable size >= 1");
		free(p);
	}
	p = malloc(T_MAX_SIZE);
	check(p != NULL, "malloc(128) returns a non-NULL pointer");
	if (p)
	{
		check(zone_of(p) && zone_of(p)->type == ZONE_TINY, "malloc(128) lives in TINY (max)");
		usable = block_of(p)->size;
		check(usable >= T_MAX_SIZE, "malloc(128) usable size >= 128");
		free(p);
	}
	p = malloc(T_MAX_SIZE + 1);
	check(p != NULL, "malloc(129) returns a non-NULL pointer");
	if (p)
	{
		check(zone_of(p) && zone_of(p)->type == ZONE_SMALL, "malloc(129) lives in SMALL (tiny + 1)");
		free(p);
	}
	p = malloc(SIZE_MAX);
	check(p == NULL, "malloc(SIZE_MAX) returns NULL");
	p = malloc(SIZE_MAX - 4096);
	check(p == NULL, "malloc(SIZE_MAX - 4096) returns NULL");
}

/*
** Fill the current tiny zone until a second one has to be mapped.
*/
static void	test_new_zone(void)
{
	void	*ptrs[1024];
	int		n;
	int		before;

	ft_printf("-- new zone when full --\n");
	n = 0;
	ptrs[n] = malloc(T_MAX_SIZE);
	if (ptrs[n])
		n++;
	before = zone_count(heap.tiny);
	while (n < 1024 && zone_count(heap.tiny) == before)
	{
		ptrs[n] = malloc(T_MAX_SIZE);
		if (!ptrs[n])
			break ;
		n++;
	}
	check(zone_count(heap.tiny) == before + 1,
		"a new TINY zone is mapped when the first one is full");
	check(n > 1 && zone_of(ptrs[n - 1]) != zone_of(ptrs[0]),
		"the overflowing allocation went to the new zone");
	while (n > 0)
	{
		n--;
		free(ptrs[n]);
	}
	check(zone_count(heap.tiny) == 0,
		"all TINY zones are released once emptied");
}

/*
** Two zones, one kept alive: freeing every block of the other must
** release it even though it is not the last one.
*/
static void	test_release_middle_zone(void)
{
	void	*ptrs[1024];
	void	*keep;
	t_zone	*second;
	int		n;
	int		z0;
	int		i;

	ft_printf("-- release an emptied, non-last zone --\n");
	z0 = zone_count(heap.tiny);
	n = 0;
	while (n < 1024 && zone_count(heap.tiny) < z0 + 2)
	{
		ptrs[n] = malloc(T_MAX_SIZE);
		if (!ptrs[n])
			break ;
		n++;
	}
	check(zone_count(heap.tiny) == z0 + 2, "two TINY zones are mapped");
	if (zone_count(heap.tiny) != z0 + 2)
		return ;
	keep = ptrs[n - 1];
	second = zone_of(keep);
	i = 0;
	while (i < n - 1)
	{
		if (zone_of(ptrs[i]) != second)
			free(ptrs[i]);
		i++;
	}
	check(zone_count(heap.tiny) == z0 + 1,
		"the emptied TINY zone is released while another remains");
	*(char *)keep = 7;
	check(*(char *)keep == 7, "the surviving zone stays usable");
	free(keep);
	check(zone_count(heap.tiny) == z0, "the last TINY zone is released");
}

/*
** Free neighbours must coalesce, and a fully freed zone must go away.
*/
static void	test_defrag(void)
{
	void	*a;
	void	*b;
	void	*c;
	void	*d;

	ft_printf("-- defragmentation --\n");
	a = malloc(16);
	b = malloc(16);
	c = malloc(16);
	check(a && b && c, "three consecutive TINY allocations succeed");
	if (a && b && c)
	{
		check(zone_of(a) == zone_of(b) && zone_of(b) == zone_of(c),
			"the three blocks share one zone");
		check((char *)b == (char *)a + BLOCK_HDR + block_of(a)->size,
			"blocks are physically consecutive");
		free(a);
		free(b);
		d = malloc(32);
		check(d == a, "freeing a+b lets malloc(32) reuse the merged block");
		check(d && block_of(d)->size >= 32, "the reused block is big enough");
		free(d);
		free(c);
	}
	check(zone_count(heap.tiny) == 0, "a fully freed TINY zone is released");
}

/*
** The usable size must never be smaller than the request, and the block
** header must sit exactly at data + size (no offset bug).
*/
static void	test_usable_size(void)
{
	size_t	sizes[] = {1, 8, 15, 16, 17, 63, 64, 100, 127, 128};
	void	*p[16];
	t_block	*b;
	t_block	*next;
	size_t	before_size;
	int		before_free;
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
		j = 0;
		while (j < sizes[i])
		{
			((unsigned char *)p[i])[j] = (unsigned char)(i + 1);
			j++;
		}
		i++;
	}
	check(ok, "each tiny request has a usable size >= request, 16-aligned");
	ok = 1;
	i = 0;
	while (i < n)
	{
		j = 0;
		while (j < sizes[i])
		{
			if (((unsigned char *)p[i])[j] != (unsigned char)(i + 1))
				ok = 0;
			j++;
		}
		i++;
	}
	check(ok, "no overlap: every block keeps its own pattern");
	ok = 1;
	i = 0;
	while (i < n - 1)
	{
		b = block_of(p[i]);
		next = (t_block *)((char *)p[i] + b->size);
		before_size = next->size;
		before_free = next->is_free;
		memset(p[i], 0x5A, b->size);
		if (next->size != before_size || next->is_free != before_free)
			ok = 0;
		i++;
	}
	check(ok, "writing the whole usable block leaves the next header intact");
	i = 0;
	while (i < n)
	{
		free(p[i]);
		i++;
	}
	check(zone_count(heap.tiny) == 0, "all TINY memory is released");
}

static void	test_edges(void)
{
	void	*ptrs[200];
	void	*p;
	void	*q;
	int		i;
	int		ok;

	ft_printf("-- edges --\n");
	p = malloc(64);
	free(p);
	q = malloc(64);
	check(q == p, "malloc reuses the most recently freed TINY block");
	check(((uintptr_t)q & 15) == 0, "returned pointer is 16-byte aligned");
	free(q);
	p = malloc(100);
	check(p != NULL, "malloc(100) returns a non-NULL pointer");
	if (p)
	{
		check(block_of(p)->zone == zone_of(p), "block back-pointer matches its zone");
		free(p);
	}
	i = 0;
	while (i < 200)
	{
		ptrs[i] = malloc((i % 100) + 1);
		i++;
	}
	ok = 1;
	i = 0;
	while (i < 200)
	{
		if (!ptrs[i])
			ok = 0;
		i++;
	}
	check(ok, "200 consecutive TINY allocations succeed");
	i = 200;
	while (i > 0)
	{
		i--;
		free(ptrs[i]);
	}
	check(zone_count(heap.tiny) == 0, "all TINY zones are released after churn");
}

int	main(void)
{
	ft_printf("== TINY TESTS ==\n");
	test_limits();
	test_new_zone();
	test_release_middle_zone();
	test_defrag();
	test_usable_size();
	test_edges();
	ft_printf("== RESULT: %d passed, %d failed ==\n", g_ok, g_ko);
	return (g_ko != 0);
}
