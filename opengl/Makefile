OBJS = $(wildcard src/*.c)

CC = clang

CFLAGS = -Wall

LDFLAGS = -lSDL2 -lGL -lGLU -lGLEW

TARGET = mandelbrot

all : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
