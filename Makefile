# Makefile

CC = gcc
CFLAGS = -Wall -g
OBJS = main.o shell.o path.o builtins.o execute.o

all: shell

shell: $(OBJS)
	$(CC) $(CFLAGS) -o shell $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o shell
