#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
** Real malloc/realloc/free test suite.
**
** These tests deliberately contain no knowledge of the allocator internals.
** They only use the standard C interface (malloc/realloc/free) and, when
** available, the project helper show_alloc_mem() (looked up with dlsym so
** that this file never has to include the project header).
**
** The test binary is linked against ../libft_malloc.so and is also run with
** LD_PRELOAD (see the Makefile) so that the C library's allocations inside
** this process are served by the allocator under test.
*/

/* ------------------------------------------------------------------ */
/* Minimal test framework                                             */
/* ------------------------------------------------------------------ */

static int g_pass;
static int g_fail;
static const char *g_test = "?";

static void check_impl(int ok, const char *expr, int line)
{
	if (ok) {
		g_pass++;
	} else {
		g_fail++;
		fprintf(stderr, "  [FAIL] %s:%d: %s\n", g_test, line, expr);
	}
}

#define CHECK(cond) check_impl((cond) != 0, #cond, __LINE__)

typedef void (*t_test_fn)(void);

struct test_case {
	const char *name;
	t_test_fn fn;
};

typedef void (*t_show_fn)(void);
static t_show_fn g_show;

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

struct memblk {
	void   *p;
	size_t  n;
	unsigned seed;
};

/* The allocator must return memory suitably aligned for any type. */
static int is_aligned(const void *p)
{
	return ((uintptr_t)p % _Alignof(max_align_t)) == 0;
}

/*
** Position dependent byte pattern. Two blocks filled with the same seed and
** length produce identical bytes, but a shifted / truncated / overlapping
** block is detected. This catches "content moved" and "blocks overlap" bugs.
*/
static unsigned char pat(size_t i, unsigned seed)
{
	unsigned x = (unsigned)i + seed * 2654435761u;

	x ^= x >> 15;
	x *= 0x2c1b3c6du;
	x ^= x >> 12;
	x *= 0x85ebca6bu;
	x ^= x >> 16;
	return (unsigned char)(x & 0xffu);
}

static void fill(void *p, size_t n, unsigned seed)
{
	unsigned char *b = (unsigned char *)p;

	for (size_t i = 0; i < n; i++)
		b[i] = pat(i, seed);
}

static int verify(const void *p, size_t n, unsigned seed)
{
	const unsigned char *b = (const unsigned char *)p;

	for (size_t i = 0; i < n; i++) {
		if (b[i] != pat(i, seed))
			return 0;
	}
	return 1;
}

static unsigned rnd_next(unsigned *state)
{
	unsigned x = *state;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
}

/* Size distribution: mostly small, some medium, few large. */
static size_t pick_size(unsigned *state)
{
	unsigned r = rnd_next(state);
	unsigned bucket = r % 100u;

	if (bucket < 65u)
		return (size_t)(1u + (r >> 8) % 128u);
	if (bucket < 90u)
		return (size_t)(129u + (r >> 8) % (4096u - 128u));
	if (bucket < 99u)
		return (size_t)(4097u + (r >> 8) % (262144u - 4096u));
	return (size_t)(1u + (r >> 8) % (1024u * 1024u));
}

/*
** Sizes that straddle the usual tiny/small/large zone limits. Sequential
** tests walk them one by one; concurrent tests keep them all alive at once.
*/
static const size_t g_boundaries[] = {
	1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33, 48, 63, 64, 65,
	100, 127, 128, 129, 200, 255, 256, 257, 511, 512, 513,
	1000, 1023, 1024, 1025, 2047, 2048, 2049, 4000, 4095, 4096, 4097,
	8192, 16384, 32768, 65535, 65536, 65537, 131072, 262144, 524288,
	1048576
};
#define NB (sizeof(g_boundaries) / sizeof(g_boundaries[0]))

/* ------------------------------------------------------------------ */
/* Tests                                                              */
/* ------------------------------------------------------------------ */

/* Every requested size must be writable and readable in full. */
static void test_malloc_basics(void)
{
	for (size_t i = 0; i < NB; i++) {
		size_t n = g_boundaries[i];
		void *p = malloc(n);

		CHECK(p != NULL);
		if (p == NULL)
			continue;
		CHECK(is_aligned(p));
		fill(p, n, (unsigned)n);
		CHECK(verify(p, n, (unsigned)n));
		free(p);
	}
}

/* malloc(0) may return NULL or a unique freeable pointer. */
static void test_malloc_zero(void)
{
	void *a = malloc(0);
	void *b = malloc(0);

	if (a != NULL) {
		CHECK(is_aligned(a));
		free(a);
	} else {
		CHECK(1);
	}
	if (b != NULL) {
		CHECK(is_aligned(b));
		free(b);
	} else {
		CHECK(1);
	}
	/* Two live zero-size allocations must not alias. */
	CHECK(a == NULL || b == NULL || a != b);
}

/* free(NULL) is a no-op and must not corrupt anything. */
static void test_free_null(void)
{
	void *p = malloc(64);

	free(NULL);
	CHECK(1);
	CHECK(p != NULL);
	if (p != NULL) {
		fill(p, 64, 1);
		free(p);
	}
	free(NULL);
	CHECK(1);
}

/* Alignment for a dense range of small sizes, including reallocs. */
static void test_alignment(void)
{
	for (size_t n = 1; n <= 1024; n++) {
		void *p = malloc(n);

		CHECK(p != NULL);
		if (p == NULL)
			continue;
		CHECK(is_aligned(p));
		void *q = realloc(p, n + 1);
		CHECK(q != NULL);
		if (q != NULL) {
			CHECK(is_aligned(q));
			free(q);
		} else {
			free(p);
		}
	}
}

/* All boundary sizes alive simultaneously. */
static void test_boundaries_concurrent(void)
{
	void *p[NB];

	for (size_t i = 0; i < NB; i++) {
		p[i] = malloc(g_boundaries[i]);
		CHECK(p[i] != NULL);
		if (p[i] != NULL)
			fill(p[i], g_boundaries[i], (unsigned)i);
	}
	for (size_t i = 0; i < NB; i++) {
		if (p[i] != NULL)
			CHECK(verify(p[i], g_boundaries[i], (unsigned)i));
	}
	for (size_t i = 0; i < NB; i++)
		free(p[i]);
}

/* Live allocations must occupy disjoint address ranges. */
static void test_no_overlap(void)
{
	enum { N = 128 };
	struct memblk b[N];

	for (int i = 0; i < N; i++) {
		b[i].n = (size_t)(1 + (i * 37) % 3000);
		b[i].seed = (unsigned)i;
		b[i].p = malloc(b[i].n);
		CHECK(b[i].p != NULL);
		if (b[i].p != NULL)
			fill(b[i].p, b[i].n, b[i].seed);
	}

	/* Insertion sort by address (avoids qsort possibly allocating). */
	for (int i = 1; i < N; i++) {
		struct memblk key = b[i];
		int j = i - 1;

		while (j >= 0 && (uintptr_t)b[j].p > (uintptr_t)key.p) {
			b[j + 1] = b[j];
			j--;
		}
		b[j + 1] = key;
	}

	for (int i = 1; i < N; i++) {
		if (b[i - 1].p == NULL || b[i].p == NULL)
			continue;
		CHECK((char *)b[i - 1].p + b[i - 1].n <= (char *)b[i].p);
	}

	for (int i = 0; i < N; i++) {
		if (b[i].p != NULL) {
			CHECK(verify(b[i].p, b[i].n, b[i].seed));
			free(b[i].p);
		}
	}
}

/* Freeing a block should make that memory available again (defrag/reuse). */
static void test_free_reuse(void)
{
	void *p = malloc(128);
	void *q;

	CHECK(p != NULL);
	if (p != NULL) {
		fill(p, 128, 3);
		free(p);
	}
	q = malloc(128);
	CHECK(q != NULL);
	CHECK(q == p);
	free(q);

	/* A hole punched between two live blocks must be reusable. */
	void *a = malloc(96);
	void *b = malloc(96);
	void *c = malloc(96);
	void *b2;

	CHECK(a != NULL && b != NULL && c != NULL);
	if (b != NULL)
		fill(b, 96, 4);
	free(b);
	b2 = malloc(96);
	CHECK(b2 != NULL);
	CHECK(b2 == b);
	free(a);
	free(c);
	free(b2);
}

/*
** Repeated same-size alloc/free cycles must reuse a small set of addresses,
** not keep growing the heap. This is the observable meaning of "defragmented
** free lists" from the outside.
*/
static void test_reuse_bounded(void)
{
	enum { N = 2048, TRACK = 64 };
	static uintptr_t seen[TRACK];
	int distinct = 0;

	for (int i = 0; i < N; i++) {
		void *p = malloc(64);
		uintptr_t a;
		int found = 0;

		CHECK(p != NULL);
		if (p == NULL)
			continue;
		a = (uintptr_t)p;
		for (int j = 0; j < distinct; j++) {
			if (seen[j] == a) {
				found = 1;
				break;
			}
		}
		if (!found && distinct < TRACK)
			seen[distinct++] = a;
		free(p);
	}
	CHECK(distinct <= 8);
	if (distinct > 8)
		fprintf(stderr, "  (info) %d distinct addresses for %d cycles\n",
			distinct, N);
}

/* Growing a block must preserve its original bytes. */
static void test_realloc_grow(void)
{
	void *p = malloc(64);
	void *q;

	CHECK(p != NULL);
	if (p == NULL)
		return;
	fill(p, 64, 111);
	q = realloc(p, 4096);
	CHECK(q != NULL);
	if (q == NULL) {
		free(p);
		return;
	}
	CHECK(verify(q, 64, 111));
	fill(q, 4096, 111);
	CHECK(verify(q, 4096, 111));
	free(q);
}

/* Shrinking a block must preserve the kept prefix. */
static void test_realloc_shrink(void)
{
	void *p = malloc(8192);
	void *q;

	CHECK(p != NULL);
	if (p == NULL)
		return;
	fill(p, 8192, 222);
	q = realloc(p, 100);
	CHECK(q != NULL);
	if (q == NULL) {
		free(p);
		return;
	}
	CHECK(verify(q, 100, 222));
	free(q);
}

/* Reallocating to the same size keeps the contents. */
static void test_realloc_same(void)
{
	void *p = malloc(1000);
	void *q;

	CHECK(p != NULL);
	if (p == NULL)
		return;
	fill(p, 1000, 333);
	q = realloc(p, 1000);
	CHECK(q != NULL);
	if (q == NULL) {
		free(p);
		return;
	}
	CHECK(verify(q, 1000, 333));
	free(q);
}

/* realloc(NULL, n) == malloc(n); realloc(p, 0) is implementation defined. */
static void test_realloc_special(void)
{
	void *p = realloc(NULL, 256);
	void *q;

	CHECK(p != NULL);
	if (p != NULL) {
		fill(p, 256, 444);
		CHECK(verify(p, 256, 444));
		free(p);
	}

	p = malloc(128);
	CHECK(p != NULL);
	if (p != NULL) {
		q = realloc(p, 0);
		if (q != NULL) {
			CHECK(is_aligned(q));
			free(q);
		}
		/* If q == NULL, realloc already released p. */
		CHECK(1);
	}
}

/* Grow and shrink blocks across the zone classes, preserving contents. */
static void test_realloc_across_zones(void)
{
	void *p = malloc(32);
	void *q;
	void *r;
	void *s;

	CHECK(p != NULL);
	if (p == NULL)
		return;
	fill(p, 32, 555);

	q = realloc(p, 300000);			/* tiny -> large */
	CHECK(q != NULL);
	if (q == NULL) {
		free(p);
		return;
	}
	CHECK(verify(q, 32, 555));
	fill(q, 300000, 555);

	r = realloc(q, 24);			/* large -> tiny */
	CHECK(r != NULL);
	if (r == NULL) {
		free(q);
		return;
	}
	CHECK(verify(r, 24, 555));

	s = realloc(r, 2 * 1024 * 1024);	/* tiny -> huge */
	CHECK(s != NULL);
	if (s == NULL) {
		free(r);
		return;
	}
	CHECK(verify(s, 24, 555));
	free(s);
}

/* Several big allocations alive at once, written and checked in full. */
static void test_large_blocks(void)
{
	static const size_t sizes[] = { 100000, 1024 * 1024, 4 * 1024 * 1024 };

	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
		size_t n = sizes[i];
		void *p = malloc(n);

		CHECK(p != NULL);
		if (p == NULL)
			continue;
		CHECK(is_aligned(p));
		fill(p, n, (unsigned)n);
		CHECK(verify(p, n, (unsigned)n));
		free(p);
	}
}

/* Thousands of small live blocks force many zones to be created. */
static void test_many_blocks(void)
{
	enum { N = 4096 };
	static struct memblk b[N];

	for (int i = 0; i < N; i++) {
		b[i].n = (size_t)(1 + (i * 53) % 1024);
		b[i].seed = (unsigned)i;
		b[i].p = malloc(b[i].n);
		CHECK(b[i].p != NULL);
		if (b[i].p != NULL)
			fill(b[i].p, b[i].n, b[i].seed);
	}
	for (int i = 0; i < N; i++) {
		if (b[i].p != NULL)
			CHECK(verify(b[i].p, b[i].n, b[i].seed));
	}
	for (int i = 0; i < N; i++)
		free(b[i].p);
}

/*
** Mix tiny, small and large allocations so all zone classes coexist, then
** release some and confirm the untouched blocks are uncorrupted.
*/
static void test_zones_grow(void)
{
	enum { TINY_N = 2048, SMALL_N = 256, LARGE_N = 8 };
	static void *tiny[TINY_N];
	static void *small[SMALL_N];
	static void *large[LARGE_N];

	for (int i = 0; i < TINY_N; i++) {
		tiny[i] = malloc(48);
		CHECK(tiny[i] != NULL);
		if (tiny[i] != NULL)
			fill(tiny[i], 48, (unsigned)i);
	}
	for (int i = 0; i < SMALL_N; i++) {
		small[i] = malloc(2048);
		CHECK(small[i] != NULL);
		if (small[i] != NULL)
			fill(small[i], 2048, (unsigned)i);
	}
	for (int i = 0; i < LARGE_N; i++) {
		size_t n = 300000 + (size_t)i * 100000;
		large[i] = malloc(n);
		CHECK(large[i] != NULL);
		if (large[i] != NULL)
			fill(large[i], n, (unsigned)i);
	}

	for (int i = 0; i < TINY_N; i++) {
		if (tiny[i] != NULL)
			CHECK(verify(tiny[i], 48, (unsigned)i));
	}
	for (int i = 0; i < SMALL_N; i++) {
		if (small[i] != NULL)
			CHECK(verify(small[i], 2048, (unsigned)i));
	}
	for (int i = 0; i < LARGE_N; i++) {
		if (large[i] != NULL)
			CHECK(verify(large[i], 300000 + (size_t)i * 100000, (unsigned)i));
	}

	/* Punch holes of each class, then confirm survivors are intact. */
	for (int i = 0; i < TINY_N; i += 2) {
		free(tiny[i]);
		tiny[i] = NULL;
	}
	for (int i = 0; i < LARGE_N; i++) {
		free(large[i]);
		large[i] = NULL;
	}
	for (int i = 1; i < TINY_N; i += 2) {
		if (tiny[i] != NULL)
			CHECK(verify(tiny[i], 48, (unsigned)i));
	}
	for (int i = 0; i < SMALL_N; i++) {
		if (small[i] != NULL)
			CHECK(verify(small[i], 2048, (unsigned)i));
	}
	for (int i = 1; i < TINY_N; i += 2)
		free(tiny[i]);
	for (int i = 0; i < SMALL_N; i++)
		free(small[i]);
}

/* Alternating free/alloc of many blocks; survivors must stay intact. */
static void test_fragmentation_defrag(void)
{
	enum { N = 256 };
	static struct memblk b[N];

	for (int i = 0; i < N; i++) {
		b[i].n = (size_t)(32 + (i % 8) * 32);
		b[i].seed = (unsigned)(i + 1000);
		b[i].p = malloc(b[i].n);
		CHECK(b[i].p != NULL);
		if (b[i].p != NULL)
			fill(b[i].p, b[i].n, b[i].seed);
	}
	for (int i = 0; i < N; i += 2) {
		free(b[i].p);
		b[i].p = NULL;
	}
	for (int i = 1; i < N; i += 2) {
		if (b[i].p != NULL)
			CHECK(verify(b[i].p, b[i].n, b[i].seed));
	}
	/* Refill the holes (possibly coalesced/split) and re-check. */
	for (int i = 0; i < N; i += 2) {
		b[i].p = malloc(b[i].n);
		CHECK(b[i].p != NULL);
		if (b[i].p != NULL) {
			fill(b[i].p, b[i].n, b[i].seed);
			CHECK(verify(b[i].p, b[i].n, b[i].seed));
		}
	}
	for (int i = 1; i < N; i += 2) {
		if (b[i].p != NULL)
			CHECK(verify(b[i].p, b[i].n, b[i].seed));
	}
	for (int i = 0; i < N; i++)
		free(b[i].p);
}

/* Free order must not matter: FIFO, LIFO and shuffled. */
static void test_free_order(void)
{
	enum { N = 512 };
	static void *p[N];

	for (int i = 0; i < N; i++) {
		p[i] = malloc(64);
		CHECK(p[i] != NULL);
		if (p[i] != NULL)
			fill(p[i], 64, (unsigned)i);
	}
	for (int i = 0; i < N; i++)
		free(p[i]);

	for (int i = 0; i < N; i++) {
		p[i] = malloc(96);
		CHECK(p[i] != NULL);
		if (p[i] != NULL)
			fill(p[i], 96, (unsigned)i);
	}
	for (int i = N - 1; i >= 0; i--)
		free(p[i]);

	for (int i = 0; i < N; i++) {
		p[i] = malloc(80);
		CHECK(p[i] != NULL);
		if (p[i] != NULL)
			fill(p[i], 80, (unsigned)i);
	}
	unsigned s = 12345u;
	for (int i = 0; i < N; i++) {
		int j = (int)(rnd_next(&s) % (unsigned)N);
		void *t = p[i];

		p[i] = p[j];
		p[j] = t;
	}
	for (int i = 0; i < N; i++)
		free(p[i]);
	CHECK(1);
}

/*
** Randomized model test: a shadow array records the size and pattern seed of
** every live allocation. Random malloc / realloc / free / verify operations
** are performed and contents are checked against the model.
*/
static void test_stress_random(void)
{
	enum { SLOTS = 512, OPS = 30000 };
	static struct memblk s[SLOTS];
	unsigned state = 0xC0FFEEu;

	for (int it = 0; it < OPS; it++) {
		struct memblk *e;
		unsigned act;
		int idx;

		rnd_next(&state);
		idx = (int)(rnd_next(&state) % SLOTS);
		act = rnd_next(&state) % 100u;
		e = &s[idx];

		if (e->p == NULL) {
			size_t n = pick_size(&state);

			e->p = malloc(n);
			CHECK(e->p != NULL);
			if (e->p == NULL)
				continue;
			e->n = n;
			e->seed = state;
			fill(e->p, n, e->seed);
		} else if (act < 40u) {
			free(e->p);
			e->p = NULL;
		} else if (act < 80u) {
			size_t n = pick_size(&state);
			size_t keep;
			void *np = realloc(e->p, n);

			CHECK(np != NULL);
			if (np == NULL)
				continue;
			keep = e->n < n ? e->n : n;
			CHECK(verify(np, keep, e->seed));
			e->p = np;
			e->n = n;
			fill(np, n, e->seed);
		} else {
			CHECK(verify(e->p, e->n, e->seed));
		}
	}
	for (int i = 0; i < SLOTS; i++) {
		if (s[i].p != NULL) {
			CHECK(verify(s[i].p, s[i].n, s[i].seed));
			free(s[i].p);
		}
	}
}

/* show_alloc_mem must be callable with live and freed blocks. */
static void test_show_alloc_mem(void)
{
	void *a = malloc(64);
	void *b = malloc(2048);
	void *c = malloc(100000);

	if (g_show != NULL) {
		printf("--- show_alloc_mem(output) ---\n");
		g_show();
		printf("--- end show_alloc_mem---\n");
		fflush(stdout);
		CHECK(1);
	} else {
		CHECK(0);
	}
	CHECK(b != NULL && c != NULL);
	free(a != NULL ? a : NULL);
	free(b);
	free(c);
	if (g_show != NULL) {
		g_show();
		fflush(stdout);
	}
}

/* ------------------------------------------------------------------ */
/* Runner                                                             */
/* ------------------------------------------------------------------ */

static struct test_case g_tests[] = {
	{ "malloc_basics",           test_malloc_basics },
	{ "malloc_zero",             test_malloc_zero },
	{ "free_null",               test_free_null },
	{ "alignment",               test_alignment },
	{ "boundaries_concurrent",   test_boundaries_concurrent },
	{ "no_overlap",              test_no_overlap },
	{ "free_reuse",              test_free_reuse },
	{ "reuse_bounded",           test_reuse_bounded },
	{ "realloc_grow",            test_realloc_grow },
	{ "realloc_shrink",          test_realloc_shrink },
	{ "realloc_same",            test_realloc_same },
	{ "realloc_special",         test_realloc_special },
	{ "realloc_across_zones",    test_realloc_across_zones },
	{ "large_blocks",            test_large_blocks },
	{ "many_blocks",             test_many_blocks },
	{ "zones_grow",              test_zones_grow },
	{ "fragmentation_defrag",    test_fragmentation_defrag },
	{ "free_order",              test_free_order },
	{ "stress_random",           test_stress_random },
	{ "show_alloc_mem",          test_show_alloc_mem },
};

/* Confirm the resolved malloc really comes from the ft_malloc library. */
static int allocator_is_ours(void)
{
	void *m = dlsym(RTLD_DEFAULT, "malloc");
	Dl_info info;

	if (m != NULL && dladdr(m, &info) && info.dli_fname != NULL
		&& strstr(info.dli_fname, "ft_malloc") != NULL)
		return 1;
	/* Fallback: show_alloc_mem only exists in the ft_malloc library. */
	return dlsym(RTLD_DEFAULT, "show_alloc_mem") != NULL;
}

int main(void)
{
	int n = (int)(sizeof(g_tests) / sizeof(g_tests[0]));
	void *sym = dlsym(RTLD_DEFAULT, "show_alloc_mem");

	g_show = (t_show_fn)sym;

	printf("=== ft_malloc test suite ===\n");
	if (allocator_is_ours()) {
		printf("allocator: ft_malloc detected\n");
	} else {
		fprintf(stderr,
			"[WARN] resolved malloc does not come from ft_malloc.\n");
		if (getenv("ALLOW_LIBC") == NULL) {
			fprintf(stderr,
				"Link/run against the library (or set ALLOW_LIBC=1 "
				"to test the system allocator).\n");
			return 2;
		}
	}
	if (g_show == NULL)
		printf("note: show_alloc_mem not exported/found\n");
	printf("\n");

	for (int i = 0; i < n; i++) {
		int p0 = g_pass;
		int f0 = g_fail;
		int dp;
		int df;

		g_test = g_tests[i].name;
		printf("[ RUN  ] %s\n", g_test);
		fflush(stdout);
		g_tests[i].fn();
		dp = g_pass - p0;
		df = g_fail - f0;
		if (df == 0)
			printf("[  OK  ] %s (%d checks)\n", g_test, dp);
		else
			printf("[ FAIL ] %s (%d passed, %d failed)\n",
				g_test, dp, df);
		fflush(stdout);
	}

	printf("\n==== %d passed, %d failed ====\n", g_pass, g_fail);
	return g_fail == 0 ? 0 : 1;
}
