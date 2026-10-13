CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -Isrc

TARGET = truehuffer

# Separate core sources from GUI sources
SRCS_CORE = main.c src/compress.c src/dict.c
SRCS_GUI = src/gui.c

OBJS_CORE = $(SRCS_CORE:.c=.o)
OBJS_GUI = $(SRCS_GUI:.c=.o)

all: $(TARGET)

# Default target includes GUI objects
$(TARGET): $(OBJS_CORE) $(OBJS_GUI)
	$(CC) $(CFLAGS) -o $@ $^

# no-gui target builds the app using only the core objects
# We also pass -DNO_GUI for the C code macro to ignore #include "gui.h"
no-gui: CFLAGS += -DNO_GUI
no-gui: $(OBJS_CORE)
	$(CC) $(CFLAGS) -o $(TARGET) $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_CORE) $(OBJS_GUI) $(TARGET)

.PHONY: all clean no-gui
