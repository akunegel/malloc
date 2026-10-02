ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif
NAME     := libft_malloc_$(HOSTTYPE).so
LINK     := libft_malloc.so

CC       := cc
CFLAGS   := -Wall -Wextra -Werror
CPPFLAGS := -Iinc -Ift_printf
SHARED   := -shared -fPIC

SRC_MALLOC := \
	src/free.c \
	src/init.c \
	src/malloc.c \
	src/show_alloc_mem.c \
	src/utils.c

SRC_PRINTF := \
	ft_printf/ft_c.c \
	ft_printf/ft_d.c \
	ft_printf/ft_hexa_maj.c \
	ft_printf/ft_hexa_min.c \
	ft_printf/ft_pourcent.c \
	ft_printf/ft_printf.c \
	ft_printf/ft_putstr_fd.c \
	ft_printf/ft_s.c \
	ft_printf/ft_u.c

OBJ := $(SRC_MALLOC:%.c=obj/%.o) $(SRC_PRINTF:%.c=obj/%.o)
DEP := $(OBJ:.o=.d)

all: $(LINK)

$(LINK): $(NAME)
	ln -sf $(NAME) $(LINK)

$(NAME): $(OBJ)
	$(CC) $(SHARED) $(OBJ) -o $@

obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

clean:
	$(RM) -r obj

fclean: clean
	$(RM) $(NAME) $(LINK)

re: fclean all

.PHONY: all clean fclean re

-include $(DEP)
