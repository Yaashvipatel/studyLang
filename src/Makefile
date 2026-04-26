# Makefile for StudyLang Compiler
# Works on macOS (no -lfl needed with modern flex) and Linux

CC      = gcc
CFLAGS  = -Wall -Wextra -g

SRC     = lex.yy.c parser.tab.c semantic.c icg.c main.c
TARGET  = studylang

.PHONY: all clean

all: $(TARGET)

# Generate C from Flex spec
lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

# Generate C+H from Bison spec
parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) 2>/dev/null || \
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -lfl 2>/dev/null || \
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -ll

clean:
	rm -f lex.yy.c parser.tab.c parser.tab.h $(TARGET) input.txt
