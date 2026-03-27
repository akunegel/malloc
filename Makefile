ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME		= libft_malloc_$(HOSTTYPE).so
SYMLINK		= libft_malloc.so

CC			= gcc
CFLAGS		= -Wall -Wextra -Werror -fPIC 
LDFLAGS		= -shared

SRCS		= $(wildcard src/*.c)
OBJS		= $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	ln -sf $@ $(SYMLINK)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) 

fclean: clean
	rm -f $(NAME) $(SYMLINK)

re: fclean all

.PHONY: all clean fclean re bonus
