#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL3/SDL.h>

//SDL Container
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_t;

// Emulator config object
typedef struct {
    uint32_t window_width;
    uint32_t window_height;
    uint32_t fg_color;     //Foreground color
    uint32_t bg_color;     //background color
    uint32_t scale_factor; //
} config_t;

//Emulator states
typedef enum {
    QUIT,
    RUNNING,
    PAUSED,
} emulator_state_t;


//chip8 Machine object
typedef struct{
    emulator_state_t state;
}chip8_t;

//Initalize SDL
bool init_sdl(sdl_t *sdl, const config_t *config) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow(
        "Chip8 Emulator",
        (int)config->window_width * config->scale_factor,
        (int)config->window_height * config->scale_factor,
        0
    );

    if (!sdl->window) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return false;
    }

    sdl->renderer = SDL_CreateRenderer(sdl->window, NULL);
    if (!sdl->renderer) {
        SDL_Log("Could not create renderer: %s", SDL_GetError());
        return false;
    }

    return true;
}

//Set up the initalize emulator configuration, taking in arguments from user
static bool set_config_from_args(config_t *config, int argc, char **argv) {

    //Set defaults
    *config = (config_t){
        .window_width = 64,
        .window_height = 32,
        .fg_color = 0xFFFFFFFF, //white
        .bg_color = 0xFFFF00FF, //yellow
        .scale_factor = 20, //Default res is 1280 X 640;
    };

    //Override defaults with user arguments
    for (int i = 1; i < argc; i++) {
        (void)argv[i]; //Temp to prevent compiler error
    }

    return true; //Success
}

bool init_chip8(chip8_t * chip8){
    chip8->state = RUNNING; //Default machine state
    return true;
}

//Final cleanup
static void final_cleanup(const sdl_t *sdl) {
    if (sdl->renderer) {
        SDL_DestroyRenderer(sdl->renderer);
    }
    if (sdl->window) {
        SDL_DestroyWindow(sdl->window);
    }
    SDL_Quit(); //Shut down SDL subsystem
}

// Clear screen / SDL Window to background color
static void clear_screen(const sdl_t *sdl, const config_t *config) {
    const uint8_t r = (config->bg_color >> 24) & 0xFF; //Shift to get rbg values
    const uint8_t g = (config->bg_color >> 16) & 0xFF;
    const uint8_t b = (config->bg_color >> 8) & 0xFF;
    const uint8_t a = (config->bg_color >> 0) & 0xFF;

    SDL_SetRenderDrawColor(sdl->renderer, r, g, b, a);
    SDL_RenderClear(sdl->renderer);
}

//Updates the window with changes
static void update_screen(const sdl_t *sdl) {
    SDL_RenderPresent(sdl->renderer);
}

void handle_input(chip8_t *chip8) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                //Exit window
                chip8->state = QUIT;
                return;
            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key){
                    case SDLK_ESCAPE:
                        //Exit window and end program
                            chip8->state = QUIT;
                            return;
                    
                    default:
                        break;
                }
                break;
            case SDL_EVENT_KEY_UP:
                break;
            
            default:
                break;
        }
    }

    return;
}

int main(int argc, char **argv) {

    //Initalize emulator config
    config_t config{};
    if (!set_config_from_args(&config, argc, argv)) {
        return EXIT_FAILURE;
    }

    //Initalize SDL
    sdl_t sdl{};
    if (!init_sdl(&sdl, &config)) {
        return EXIT_FAILURE;
    }

    //Initalize chip8 machine
    chip8_t chip8{};
    if(!init_chip8(&chip8)) exit(EXIT_FAILURE);

    //Inital screen clear to background color
    clear_screen(&sdl, &config);

    //Main emulator loop
    while (chip8.state != QUIT) {
        //Handle user input 
        handle_input(&chip8);
        //Get time();
        //Emulate CHIP8 Instrucitons
        //Get_time();

        clear_screen(&sdl, &config);

        //Delay 60fps (approx)
        //SDL_Delay(16 - actual time passed);
        SDL_Delay(16);
        // Update window with changes
        update_screen(&sdl);
    }

    //Final cleanup
    final_cleanup(&sdl);

    exit(EXIT_SUCCESS);
}
