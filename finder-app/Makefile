CC ?= gcc
CFLAGS := -Wall -Werror

# Default target
all: writer

writer: writer.c
$(CC) $(CFLAGS) -o writer writer.c

clean:
rm -f writer *.o

.PHONY: all clean
