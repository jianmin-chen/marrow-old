C_FLAGS=-Wall -Wextra -pedantic
SRC=$(wildcard error/*.c) $(wildcard buffer/*.c) $(wildcard keyboard/*.c) $(wildcard tab/*.c) marrow.c

marrow: $(SRC)
	$(CC) -o $@ $^ $(C_FLAGS)
