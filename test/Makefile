# =========================================================================== #
#                              malloc test suite                              #
# =========================================================================== #

ROOT := ..

CC       := cc
CFLAGS   := -Wall -Wextra -Werror -g
CPPFLAGS := -I$(ROOT)/inc -I$(ROOT)/ft_printf

# --- library sources (malloc module) --------------------------------------- #
LIB_MALLOC := \
	$(ROOT)/src/free.c \
	$(ROOT)/src/init.c \
	$(ROOT)/src/malloc.c \
	$(ROOT)/src/show_alloc_mem.c \
	$(ROOT)/src/utils.c

# --- library sources (printf module) --------------------------------------- #
LIB_PRINTF := \
	$(ROOT)/ft_printf/ft_c.c \
	$(ROOT)/ft_printf/ft_d.c \
	$(ROOT)/ft_printf/ft_hexa_maj.c \
	$(ROOT)/ft_printf/ft_hexa_min.c \
	$(ROOT)/ft_printf/ft_pourcent.c \
	$(ROOT)/ft_printf/ft_printf.c \
	$(ROOT)/ft_printf/ft_putstr_fd.c \
	$(ROOT)/ft_printf/ft_s.c \
	$(ROOT)/ft_printf/ft_u.c

LIB_OBJ := $(LIB_MALLOC:$(ROOT)/%.c=obj/%.o) $(LIB_PRINTF:$(ROOT)/%.c=obj/%.o)
LIB_DEP := $(LIB_OBJ:.o=.d)

TESTS := tiny_test small_test large_test g_test

# --- Rules ----------------------------------------------------------------- #
all: $(TESTS)

obj/%.o: $(ROOT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

libtest.a: $(LIB_OBJ)
	$(AR) rcs $@ $(LIB_OBJ)

$(TESTS): %: %.c libtest.a
	$(CC) $(CFLAGS) $(CPPFLAGS) $< libtest.a -o $@

clean:
	$(RM) -r obj libtest.a

fclean: clean
	$(RM) $(TESTS)
	$(RM) -r $(TESTS:=.dSYM)

re: fclean all

.PHONY: all clean fclean re

-include $(LIB_DEP)
