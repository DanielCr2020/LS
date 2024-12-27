TARGET=ls
CC=gcc
FLAGS=-Wall -Wpedantic -pedantic-errors -g -fstack-protector-all

all: $(TARGET)

ls: ls.c
	$(CC) ls.c -o ls $(FLAGS)
clean:
	rm ls
