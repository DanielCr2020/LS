TARGET_EXEC=ls
SOURCE=ls.c ls.h
CC=gcc
FLAGS=-Wall -Wpedantic -pedantic-errors -g -fstack-protector-all -lm

all: $(TARGET_EXEC)

ls: $(SOURCE)
	$(CC) $(SOURCE) -o $(TARGET_EXEC) $(FLAGS)

.PHONY: clean

clean:
	rm $(TARGET_EXEC)
