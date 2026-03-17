#include "display.h"
#include <stdio.h>

int display_init(display_t *display, const char *title)
{
    // Initialize SDL — VIDEO flag only, we don't need audio yet
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }

    // Create the OS window
    display->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_W,
        WINDOW_H,
        SDL_WINDOW_SHOWN);
    if (!display->window)
    {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create the renderer — -1 means "first available driver", 0 means no flags
    display->renderer = SDL_CreateRenderer(display->window, -1, 0);
    if (!display->renderer)
    {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(display->window);
        SDL_Quit();
        return -1;
    }

    // Create a texture matching the Chip-8 resolution (64x32)
    // SDL_PIXELFORMAT_RGBA8888 — 4 bytes per pixel (R, G, B, A)
    // SDL_TEXTUREACCESS_STREAMING — we'll update it every frame
    display->texture = SDL_CreateTexture(
        display->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        64, 32);
    if (!display->texture)
    {
        fprintf(stderr, "SDL_CreateTexture error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(display->renderer);
        SDL_DestroyWindow(display->window);
        SDL_Quit();
        return -1;
    }

    return 0;
}

void display_render(display_t *display, const uint8_t *pixels)
{
    // Convert the 1-bit Chip-8 pixel buffer into RGBA colors
    // Each Chip-8 pixel is 0 or 1 — map to black or white RGBA
    uint32_t rgba_buffer[64 * 32];
    for (int i = 0; i < 64 * 32; i++)
    {
        rgba_buffer[i] = pixels[i] ? 0xFFFFFFFF : 0x000000FF;
        //                           white RGBA    black RGBA
    }

    // Upload our RGBA buffer to the texture
    // pitch = number of bytes per row = 64 pixels * 4 bytes each
    SDL_UpdateTexture(display->texture, NULL, rgba_buffer, 64 * sizeof(uint32_t));

    // Clear, copy texture (SDL scales 64x32 → 640x320 automatically), present
    SDL_RenderClear(display->renderer);
    SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
    SDL_RenderPresent(display->renderer);
}

void display_destroy(display_t *display)
{
    SDL_DestroyTexture(display->texture);
    SDL_DestroyRenderer(display->renderer);
    SDL_DestroyWindow(display->window);
    SDL_Quit();
}