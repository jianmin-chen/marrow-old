C_FLAGS=-lm -lcurl -Wall -Wextra -pedantic
SRC=$(wildcard status/*.c) $(wildcard libs/*.c) $(wildcard window/*.c) $(wildcard highlight/*.c) $(wildcard keyboard/*.c) $(wildcard plugins/*.c) $(wildcard tab/*.c) $(wildcard tree/*.c) marrow.c

marrow: $(SRC)
	$(CC) -o $@ $^ $(C_FLAGS)
