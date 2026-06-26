#include <stdio.h>
#include "chip8.h"
#include "display.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom_file>\n", argv[0]);
        return 1;
    }

    chip8_t chip;
    chip8_init(&chip);
    if (chip8_load_rom(&chip, argv[1]) != 0)
        return 1;

    display_t display;
    if (display_init(&display, "CHIP-8") != 0)
        return 1;

    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
        }

        chip8_cycle(&chip);

        if (chip.draw_flag) {
            display_render(&display, chip.display);
            chip.draw_flag = 0;
        }

        SDL_Delay(2);
    }

    display_destroy(&display);
    return 0;
}
