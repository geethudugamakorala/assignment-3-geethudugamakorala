# Makefile for writer application

# Compiler flags for better code quality
CFLAGS := -Wall -Werror

# Default target
all: writer

writer: writer.c
ifdef CROSS_COMPILE
	$(CROSS_COMPILE)gcc $(CFLAGS) -o writer writer.c
else
	gcc $(CFLAGS) -o writer writer.c
endif

clean:
	rm -f writer *.o

.PHONY: all clean