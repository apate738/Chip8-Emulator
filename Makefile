chip8: gameLoop.c chip8.c chip8.h
	g++ gameLoop.c chip8.c -o chip8 -std=c++17 -Wall -Wextra -Werror -I./SDL3-3.4.10/x86_64-w64-mingw32/include/SDL3 -L./SDL3-3.4.10/x86_64-w64-mingw32/lib -lmingw32 -lSDL3main -lSDL3
