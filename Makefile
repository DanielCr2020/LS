TARGET_EXEC=ls
SOURCE=ls.c ls.h
CC=gcc
CFLAGS=-Wall -Wpedantic -pedantic-errors -g -fstack-protector-all
LDFLAGS=-lm

all: $(TARGET_EXEC)

ls: $(SOURCE)
	$(CC) $(SOURCE) -o $(TARGET_EXEC) $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm $(TARGET_EXEC)
