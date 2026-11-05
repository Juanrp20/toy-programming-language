CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -Iinclude -O2
DEBUG_FLAGS = -Wall -Wextra -std=c99 -pedantic -Iinclude -g -fsanitize=address,undefined

SRC = src/token.c src/lexer.c src/arena.c src/ast.c src/parser.c
MAIN_SRC = src/main.c
TEST_SRC = tests/test_c.c

TARGET = toy_c
TEST_BIN = test_c

all: $(TARGET)

$(TARGET): $(SRC) $(MAIN_SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(MAIN_SRC)

debug: $(SRC) $(MAIN_SRC)
	$(CC) $(DEBUG_FLAGS) -o $(TARGET) $(SRC) $(MAIN_SRC)

$(TEST_BIN): $(SRC) $(TEST_SRC)
	$(CC) $(DEBUG_FLAGS) -o $@ $(SRC) $(TEST_SRC)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TARGET) $(TEST_BIN)

.PHONY: all debug test clean
