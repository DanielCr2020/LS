CC=gcc
CFLAGS=-Wall -Wpedantic -pedantic-errors -g
LDFLAGS=-lm

TARGET=ls
SRCS=$(wildcard *.c)
HDRS=$(wildcard *.h)
OBJS=$(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS) $(HDRS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJS) $(TARGET)

.PHONY: all clean
