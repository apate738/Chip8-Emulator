all:
	g++ gameLoop.c -o chip8.exe -std=c++17 -Wall -Wextra -Werror -I./SDL3-3.4.10/x86_64-w64-mingw32/include -L./SDL3-3.4.10/x86_64-w64-mingw32/lib -lSDL3
	copy /Y .\SDL3-3.4.10\x86_64-w64-mingw32\bin\SDL3.dll .

debug:
	g++ gameLoop.c -o chip8.exe -std=c++17 -Wall -Wextra -Werror -DDEBUG -I./SDL3-3.4.10/x86_64-w64-mingw32/include -L./SDL3-3.4.10/x86_64-w64-mingw32/lib -lSDL3
	copy /Y .\SDL3-3.4.10\x86_64-w64-mingw32\bin\SDL3.dll .
