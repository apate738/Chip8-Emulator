#ifndef CHIP8_DISPLAY_H
#define CHIP8_DISPLAY_H

#include <SDL2/SDL.h>
#include <stdint.h>

#define SCALE    10
#define WINDOW_W (64 * SCALE)
#define WINDOW_H (32 * SCALE)

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
} display_t;

int  display_init(display_t *display, const char *title);
void display_render(display_t *display, const uint8_t *pixels);
void display_destroy(display_t *display);

#endif /* CHIP8_DISPLAY_H */
