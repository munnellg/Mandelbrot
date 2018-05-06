OBJS = $(wildcard src/*.c)

CC = gcc

LDFLAGS = -lm -lSDL2

CFLAGS = -Wall -fopenmp

TARGET = mandelbrot

all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS) 
