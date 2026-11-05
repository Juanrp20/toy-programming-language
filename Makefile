CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -Iinclude -O2
DEBUG_FLAGS = -Wall -Wextra -std=c99 -pedantic -Iinclude -g -fsanitize=address,undefined

SRC = src/token.c src/lexer.c src/arena.c src/ast.c src/parser.c
LL1_SRC = src/set.c src/ll1.c
MAIN_SRC = src/main.c
MAIN_LL1_SRC = src/main_ll1.c
TEST_SRC = tests/test_c.c

TARGET = toy_c
TARGET_LL1 = toy_ll1
TEST_BIN = test_c

all: $(TARGET) $(TARGET_LL1)

$(TARGET): $(SRC) $(MAIN_SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(MAIN_SRC)

$(TARGET_LL1): $(LL1_SRC) $(MAIN_LL1_SRC)
	$(CC) $(CFLAGS) -o $@ $(LL1_SRC) $(MAIN_LL1_SRC)

debug: $(SRC) $(LL1_SRC) $(MAIN_SRC) $(MAIN_LL1_SRC)
	$(CC) $(DEBUG_FLAGS) -o $(TARGET) $(SRC) $(MAIN_SRC)
	$(CC) $(DEBUG_FLAGS) -o $(TARGET_LL1) $(LL1_SRC) $(MAIN_LL1_SRC)

$(TEST_BIN): $(SRC) $(LL1_SRC) $(TEST_SRC)
	$(CC) $(DEBUG_FLAGS) -o $@ $(SRC) $(LL1_SRC) $(TEST_SRC)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TARGET) $(TARGET_LL1) $(TEST_BIN)

.PHONY: all debug test clean

