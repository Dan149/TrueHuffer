CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -Isrc

TARGET = app
SRCS = main.c src/gui.c src/compile.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
