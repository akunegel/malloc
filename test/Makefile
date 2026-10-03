# ft_malloc test suite
#
#   make          build the allocator (parent dir) and the test binary
#   make run      build then execute the suite with the allocator preloaded
#   make clean    remove test objects
#   make fclean   remove test objects and the binary
#   make re       rebuild from scratch
#
# The binary is linked against ../libft_malloc.so so that malloc/free/realloc
# resolve to the allocator under test instead of libc, and it is also run
# with LD_PRELOAD so that allocations made *inside* libc (printf, dlsym, ...)
# go through the allocator too. main.c independently verifies at runtime that
# the resolved malloc really lives in the ft_malloc library.

UNAME_M  := $(shell uname -m)
UNAME_S  := $(shell uname -s)
HOSTTYPE ?= $(UNAME_M)_$(UNAME_S)

ROOT     := ..
LIB      := $(ROOT)/libft_malloc.so

CC       := gcc
CFLAGS   := -Wall -Wextra -Werror -g -O0
CPPFLAGS :=
LDFLAGS  := -L$(ROOT) -lft_malloc
LDLIBS   := -ldl

ifeq ($(UNAME_S),Darwin)
RPATH    := -Wl,-rpath,@loader_path/..
# On Darwin the binary is already linked against the allocator, so no
# DYLD_INSERT_LIBRARIES is needed (and flat-namespace tricks are avoided).
RUN_ENV  :=
else
RPATH    := -Wl,-rpath,'$$ORIGIN/..'
RUN_ENV  := LD_PRELOAD=$(abspath $(LIB))
endif

TARGET   := malloc_test
SRC      := main.c
OBJ      := $(SRC:.c=.o)

all: $(TARGET)

# Always invoke the parent build: it is incremental and cheap.
.PHONY: lib
lib:
	$(MAKE) -C $(ROOT)

$(TARGET): $(OBJ) lib
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS) $(RPATH) $(LDLIBS)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

run: $(TARGET)
	$(RUN_ENV) ./$(TARGET)

test: run

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(TARGET)

re: fclean all

.PHONY: all lib run test clean fclean re
