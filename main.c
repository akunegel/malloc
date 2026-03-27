#include "include/malloc.h"
#include <stdio.h>

int main()
{
	char *str;
	int i = 0;
	while (i < 20) {
		if (i == 5)
    		str = (char *)malloc(sizeof(char) * i * 100);
		i++;
	}
	while (i < 40) {
		if (i == 25)
			str = (char *)malloc(sizeof(char) * i * 10000);
		i++;
	}
	str = (char *)malloc(sizeof(char) * i * 100000);
	show_alloc_mem();
    return 0;
}
