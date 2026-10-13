CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -Isrc

TARGET = app
TEST_TARGET = test_runner

# App entry point
SRCS_MAIN = main.c

# Core sources (used by main app and tests)
SRCS_CORE = src/compress.c src/debug.c

# GUI sources
SRCS_GUI = src/gui.c

# Test entry point
SRCS_TEST = tests/tests.c

# Generate object file lists
OBJS_MAIN = $(SRCS_MAIN:.c=.o)
OBJS_CORE = $(SRCS_CORE:.c=.o)
OBJS_GUI  = $(SRCS_GUI:.c=.o)
OBJS_TEST = $(SRCS_TEST:.c=.o)

all: $(TARGET)

# Standard build with GUI
$(TARGET): $(OBJS_MAIN) $(OBJS_CORE) $(OBJS_GUI)
	$(CC) $(CFLAGS) -o $@ $^

# No-GUI target
no-gui: CFLAGS += -DNO_GUI
no-gui: clean-obj $(OBJS_MAIN) $(OBJS_CORE)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS_MAIN) $(OBJS_CORE)

# Target-specific flag: Map tests() to main() in tests.c during preprocessing
$(OBJS_TEST): CFLAGS += -Dtests=main

# Run test suite
tests: $(TEST_TARGET)
	@echo "Running tests..."
	./$(TEST_TARGET)

$(TEST_TARGET): $(OBJS_TEST) $(OBJS_CORE) $(OBJS_GUI)
	$(CC) $(CFLAGS) -o $@ $^

# Compile C files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean object files
clean-obj:
	rm -f $(OBJS_MAIN) $(OBJS_CORE) $(OBJS_GUI) $(OBJS_TEST)

# Clean everything
clean: clean-obj
	rm -f $(TARGET) $(TEST_TARGET)

.PHONY: all clean clean-obj no-gui tests
