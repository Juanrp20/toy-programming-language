CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -Iinclude -O2

ifeq ($(OS),Windows_NT)
    # En Windows (MinGW), libasan y libubsan normalmente no vienen por defecto
    DEBUG_FLAGS = -Wall -Wextra -std=c99 -pedantic -Iinclude -g
    EXE = .exe
    RM = del /Q /F
    RUN_TEST = $(TEST_BIN)
else
    DEBUG_FLAGS = -Wall -Wextra -std=c99 -pedantic -Iinclude -g -fsanitize=address,undefined
    EXE =
    RM = rm -f
    RUN_TEST = ./$(TEST_BIN)
endif

SRC = src/token.c src/lexer.c src/arena.c src/ast.c src/parser.c
LL1_SRC = src/set.c src/ll1.c
MAIN_SRC = src/main.c
MAIN_LL1_SRC = src/main_ll1.c
TEST_SRC = tests/test_c.c

TARGET = toy_c$(EXE)
TARGET_LL1 = toy_ll1$(EXE)
TEST_BIN = test_c$(EXE)

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
	$(RUN_TEST)

clean:
	$(RM) $(TARGET) $(TARGET_LL1) $(TEST_BIN)

.PHONY: all debug test clean


