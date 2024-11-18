CFLAGS = -ggdb -Wall -Wextra -Werror

.PHONY: build
build:
	@cc $(CFLAGS) main.c
