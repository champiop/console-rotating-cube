CC = gcc
LD = gcc

CFLAGS = -Wall -Wextra -std=c99 -O0
LDFLAGS = -lm

SRC_FILES=$(wildcard src/*.c)
OBJ_FILES=$(patsubst src/%.c,obj/%.o,$(SRC_FILES))

GEOMETRY = cube

all: cube

cube: obj $(OBJ_FILES)
	$(LD) $(OBJ_FILES) $(LDFLAGS) -o $@

obj:
	mkdir obj -p

obj/%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -rf cube obj

build: cube

run: cube
	./cube geometry/$(GEOMETRY).txt