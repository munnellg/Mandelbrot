OBJS = $(wildcard src/*.c)

CC = clang

CFLAGS = -Wall

LDFLAGS =

TARGET = mandelbrot

all : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
