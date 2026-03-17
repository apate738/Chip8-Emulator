CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LIBS   = $(shell sdl2-config --cflags --libs)

SRC = src/main.c src/chip8.c src/display.c
OUT = chip8

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)