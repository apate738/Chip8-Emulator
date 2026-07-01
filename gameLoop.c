#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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
    uint32_t scale_factor; //Amount to scale the CHIP8 Pixel by
    bool pixel_outlines;    // Draw pixel outlines
} config_t;

//Emulator states
typedef enum {
    QUIT,
    RUNNING,
    PAUSED,
} emulator_state_t;

//Chip8 Instruction format
typedef struct{
    uint16_t opcode;
    uint16_t NNN; //12 bit address
    uint8_t NN;   //8 bit constant  
    uint8_t N;    //4 bit constant
    uint8_t X;    //4 bit register identifier
    uint8_t Y;    //4 bit register identifier
} instruction_t;


//chip8 Machine object
typedef struct{
    emulator_state_t state;
    uint8_t ram[4096];
    bool display[64*32];    //Emulate original chip8 res pixels
    uint16_t stack[12];     //Subroutine stack
    uint16_t *stack_ptr;
    uint8_t V[16];          //Virtual data registers V0-Vf
    uint16_t I;             //Index register
    uint16_t PC;            //Program Counter
    uint8_t delay_timer;    //Decrements at 60hz if >0
    uint8_t sound_timer;    //Decrements at 60 hz + plays tone if >0
    bool keypad[16];        //Hexadecimal keypad 0x0 - 0xF
    char *rom_name;         //Currently running ROM
    instruction_t inst;     //Currently executing instruction

} chip8_t;

//Initalize SDL
bool init_sdl(sdl_t *sdl, const config_t *config) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Unable to initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow(
        "Chip8 Emulator",
        (int)config->window_width * config->scale_factor,
        (int)config->window_height * config->scale_factor,
        0
    );

    if (!sdl->window) {
        SDL_Log("Could not create window: %s\n", SDL_GetError());
        return false;
    }

    sdl->renderer = SDL_CreateRenderer(sdl->window, NULL);
    if (!sdl->renderer) {
        SDL_Log("Could not create renderer: %s\n", SDL_GetError());
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
        .bg_color = 0x000000FF, //black
        .scale_factor = 20, //Default res is 1280 X 640;
        .pixel_outlines = true //Draw pixel outlines by default
    };

    //Override defaults with user arguments
    for (int i = 1; i < argc; i++) {
        (void)argv[i]; //Temp to prevent compiler error
    }

    return true; //Success
}

bool init_chip8(chip8_t * chip8, char rom_name[]){
    const uint32_t entry_point = 0x200; //CHIP8 roms are loaded at 0x200
    const uint8_t font[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
      };
    //Load font
    memcpy(&chip8->ram[0], font, sizeof(font));

    //Load ROM to chip memory
      FILE *rom = fopen(rom_name, "rb");
      if(!rom){
        SDL_Log("Rom file %s is invalid or doesn't exist\n", rom_name);
        return false;
      }

      //Get and check rom size
      fseek(rom, 0, SEEK_END);
      const size_t rom_size = ftell(rom);
      const size_t max_size = sizeof chip8->ram - entry_point;

      if (rom_size > max_size){
        SDL_Log("Rom file %s is too big! Rom size %lu, Max size allowed allowed %lu\n"
                                                    , rom_name, (unsigned long)rom_size, (unsigned long)max_size);
        return false;
      }
      fseek(rom, 0, SEEK_SET);
      if (fread(&chip8->ram[entry_point], rom_size, 1, rom) != 1 ){
        SDL_Log("Could not read Rom file %s into memory\n", rom_name);
        return false;
      }
      fclose(rom);

    //Set defaults
    chip8->state = RUNNING; //Default machine state
    chip8->PC = entry_point; //Start program counter at ROM entry point
    chip8->rom_name = rom_name;
    chip8->stack_ptr = &chip8->stack[0];

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
static void update_screen(const sdl_t *sdl, const config_t *config, const chip8_t *chip8) {
    SDL_FRect rect = {0, 0, (float)config->scale_factor, (float)config->scale_factor};
    
    for(uint32_t i = 0; i < config->window_width * config->window_height; i++){

        //Grab color values to draw
        const uint8_t bg_r = (config->bg_color >> 24) & 0xFF; //Shift to get rbg values
        const uint8_t bg_g = (config->bg_color >> 16) & 0xFF;
        const uint8_t bg_b = (config->bg_color >> 8) & 0xFF;
        const uint8_t bg_a = (config->bg_color >> 0) & 0xFF;

        const uint8_t fg_r = (config->fg_color >> 24) & 0xFF; //Shift to get rbg values
        const uint8_t fg_g = (config->fg_color >> 16) & 0xFF;
        const uint8_t fg_b = (config->fg_color >> 8) & 0xFF;
        const uint8_t fg_a = (config->fg_color >> 0) & 0xFF;

        //Translate 1D index i value to 2D x,y coords
        // X = i % window width
        // Y = i / window width
        rect.x = ((i % config->window_width) * config->scale_factor);
        rect.y = ((i / config->window_width) * config->scale_factor);

        if(chip8->display[i]){
            //If pixel is on draw foreground color
            SDL_SetRenderDrawColor(sdl->renderer, fg_r, fg_g, fg_b, fg_a);
            SDL_RenderFillRect(sdl->renderer, &rect);

            //If user requested drawing pixel lines, draw those here
            if(config->pixel_outlines){
            SDL_SetRenderDrawColor(sdl->renderer, bg_r, bg_g, bg_b, bg_a);
            SDL_RenderRect(sdl->renderer, &rect);
            }
            

        } else {
            //Pixel is off draw background color
            SDL_SetRenderDrawColor(sdl->renderer, bg_r, bg_g, bg_b, bg_a);
            SDL_RenderFillRect(sdl->renderer, &rect);
        }
    }
    
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
                    case SDLK_SPACE:
                        //Space bar
                        if(chip8->state  == RUNNING){
                            chip8->state = PAUSED; //Pause
                        }else{
                            chip8->state = RUNNING; //Resume
                            puts("===== PAUSED =====");
                        }
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

#ifdef DEBUG
void print_debug_info(const chip8_t *chip8){
    printf("Address: 0x%04X, Opcode: 0x%04X Desc: ",
         chip8->PC-2, chip8->inst.opcode);

    switch ((chip8->inst.opcode >> 12) & 0x0F) {
        case 0x0:
            if (chip8->inst.NN == 0xE0) {
                //0x00E0: Clears the screen
                printf("Clear screen\n");
            } else if (chip8->inst.NN == 0xEE) { 
                //Return from subroutine
                //Grab last address from subroutine stack
                printf("Return from subroutine to address 0x%04X\n.", *(chip8->stack_ptr - 1));
            } else { 
                printf("Unimplemented Opcode.\n");
            }
            break;
        
        case 0x01:
            //0x1NNN: Jump to address NNN
            printf("Jump to address NNN (0x%04X).\n", chip8->inst.NNN);
            break;

        case 0x02:
            // 0x2NNN: Call subroutine at NNN
            printf("Call subroutine at NNN (0x%04X).\n", chip8->inst.NNN);
            break;
        
        case 0x03:
            // 0x3NNN Check if VX == NN, if so, skip the next instruction
            printf("Check if V%X (0x%02X) == NN (0x%02X), skip next instruction if true\n",
            chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.NN);
            break;
        
        case 0x04:
            // 0x4NNN: Check if VX != NN, if so, skip the next instruction
            printf("Check if V%X (0x%02X) != NN (0x%02X), skip next instruction if true\n",
            chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.NN);
            break;
        
        case 0x05:
            // 0x5XY0: Check if VX == VY, if so, skip the next instruction
            printf("Check if V%X (0x%02X) == V%X (0x%02X), skip next instruction if true\n",
                chip8->inst.X, chip8->V[chip8->inst.X],
                chip8->inst.Y, chip8->V[chip8->inst.Y]);
            break;
        

        case 0x06:
            //0x06XNN: Set VX to NN
            printf("Set register V%X to NN(0x%02X).\n",
                   chip8->inst.X, chip8->inst.NN);
            break;
        
        case 0x07:
            //0x07XNN: Set register VX += NN
            printf("Set register V%X (0x%02X) += NN (0x%02X). Result: 0x%02X\n",
                   chip8->inst.X,
                   chip8->V[chip8->inst.X],
                   chip8->inst.NN,
                   (uint8_t)(chip8->V[chip8->inst.X] + chip8->inst.NN));
            break;
        
            case 0x08:
            switch(chip8->inst.N) {
                case 0:
                    // 0x8XY0: Set register VX = VY
                    printf("Set register V%X = V%X (0x%02X)\n",
                           chip8->inst.X, chip8->inst.Y, chip8->V[chip8->inst.Y]);
                    break;

                case 1:
                    // 0x8XY1: Set register VX |= VY
                    printf("Set register V%X (0x%02X) |= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] | chip8->V[chip8->inst.Y]);
                    break;

                case 2:
                    // 0x8XY2: Set register VX &= VY
                    printf("Set register V%X (0x%02X) &= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] & chip8->V[chip8->inst.Y]);
                    break;

                case 3:
                    // 0x8XY3: Set register VX ^= VY
                    printf("Set register V%X (0x%02X) ^= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] ^ chip8->V[chip8->inst.Y]);
                    break;

                case 4:
                    // 0x8XY4: Set register VX += VY, set VF to 1 if carry
                    printf("Set register V%X (0x%02X) += V%X (0x%02X), VF = 1 if carry; Result: 0x%02X, VF = %X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y],
                           ((uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255));
                    break;

                case 5:
                    // 0x8XY5: Set register VX -= VY, set VF to 1 if there is not a borrow (result is positive/0)
                    printf("Set register V%X (0x%02X) -= V%X (0x%02X), VF = 1 if no borrow; Result: 0x%02X, VF = %X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] - chip8->V[chip8->inst.Y],
                           (chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]));
                    break;

                case 6:
                    // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
                    printf("Set register V%X (0x%02X) >>= 1, VF = shifted off bit (%X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->V[chip8->inst.X] & 1,
                           chip8->V[chip8->inst.X] >> 1);
                    break;

                case 7:
                    // 0x8XY7: Set register VX = VY - VX, set VF to 1 if there is not a borrow (result is positive/0)
                    printf("Set register V%X = V%X (0x%02X) - V%X (0x%02X), VF = 1 if no borrow; Result: 0x%02X, VF = %X\n",
                           chip8->inst.X, chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X],
                           (chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]));
                    break;

                case 0xE:
                    // 0x8XYE: Set register VX <<= 1, store shifted off bit in VF
                    printf("Set register V%X (0x%02X) <<= 1, VF = shifted off bit (%X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           (chip8->V[chip8->inst.X] & 0x80) >> 7,
                           chip8->V[chip8->inst.X] << 1);
                    break;

                default:
                    break;  //Wrong opcode
            }   

            break;
        
        case 0x09:
            // 0x5XY0: Check if VX == VY, if so, skip the next instruction
            printf("Check if V%X (0x%02X) != V%X (0x%02X), skip next instruction if true\n",
                chip8->inst.X, chip8->V[chip8->inst.X],
                chip8->inst.Y, chip8->V[chip8->inst.Y]);
            break;
        
        case 0x0A:
            // 0xANNN: Set index reg I to nNN
            printf("Set I to NNN (0x%04X)\n", chip8->inst.NNN);
            break;
        
        case 0x0B: 
            // 0xBNNN: Jump to V0 + NNN:
            printf("Set PC to V0 (0x%02X) + NNN (0x%04X); Result PC = 0x%04X\n",
                    chip8->V[0], chip8->inst.NNN, chip8->V[0] + chip8->inst.NNN);
            break;

        case 0x0D:
            //0xDXYN: Draw N height sprite at coords X, Y: Read from mem location I;
            // Screen pixels are XOR'd with sprite bits, 
            // VF (carry flag) is set if any screen pixels are set off
            printf("Draw N (%u) height sprite at cords V%X (0x%02X), V%X (0x%02X)\n "
                   "from memory location (0x%04X). Set VF = 1 if any pixels are turned off.\n",
                   chip8->inst.N, chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y,
                   chip8->V[chip8->inst.Y], chip8->I);
            break; 

        default:
            printf("Unimplemented Opcode.\n");
            break; //Unimplememnted / invalid
    }
}
#endif

void emulate_instruction(chip8_t *chip8, const config_t *config){
    // Get next opcode from ram
    chip8->inst.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC + 1];
    chip8->PC += 2; //Pre-increment program counter for next opcode

    // Fill out instruction format
    chip8->inst.NNN = chip8->inst.opcode & 0x0FFF;
    chip8->inst.NN = chip8->inst.opcode & 0x00FF;
    chip8->inst.N = chip8->inst.opcode & 0x0F;
    chip8->inst.X = (chip8->inst.opcode >> 8) & 0x0F;
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0F;

#ifdef DEBUG
    print_debug_info(chip8);
#endif



    //Emulate opcode
    switch ((chip8->inst.opcode >> 12) & 0x0F) {
        case 0x0:
            if (chip8->inst.NN == 0xE0) {
                //0x00E0: Clears the screen
                memset(&chip8->display[0], false, sizeof(chip8->display));
            } else if (chip8->inst.NN == 0xEE) { 
                //Return from subroutine
                //Grab last address from subroutine stack
                chip8->PC = *--chip8->stack_ptr;
            } else{
                //Unimplemented opcode
            }
            break;
        
        case 0x01:
            //0x1NNN: Jump to address NNN
            chip8->PC = chip8->inst.NNN;    //Set program counter so next address is from NNN
            break;

        case 0x02:
            // 0x2NNN: Call subroutine at NNN
            *chip8->stack_ptr++ = chip8->PC;     //Store current address to return to on subroutine stack
            chip8->PC = chip8->inst.NNN;    //Set PC to next address to get the next opcode
            break;
        
        case 0x03:
            // 0x3NNN: Check if VX == NN, if so, skip the next instruction
            if(chip8->V[chip8->inst.X] == chip8->inst.NN)
                chip8->PC += 2; //Skip next opcode/instruction
            break;
        
        case 0x04:
            // 0x4NNN: Check if VX != NN, if so, skip the next instruction
            if(chip8->V[chip8->inst.X] != chip8->inst.NN)
                chip8->PC += 2;     //Skip next opcode / instruction
            break;
        
        case 0x05:
            // 0x5XY0: Check if VX == VY, if so, skip the next instruction
            if(chip8->inst.N != 0) break; //Wrong opcode
            if(chip8->V[chip8->inst.X] == chip8->V[chip8->inst.Y])
                chip8->PC += 2;     //Skip next opcode / instruction
            break;
        
        case 0x06:
            //0x06XNN: Set VX to NN
            chip8->V[chip8->inst.X] = chip8->inst.NN;
            break;
        
        case 0x07:
            //0x07XNN: Set register VX += NN
            chip8->V[chip8->inst.X] += chip8->inst.NN;
            break;
        
        case 0x08:
            switch(chip8->inst.N){
                case 0:
                    //0x08XY0: Set reg VX = VY
                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y];
                    break;
                case 1:
                    //0x08XY1: Set reg VX |= VY
                    chip8->V[chip8->inst.X] |= chip8->V[chip8->inst.Y];
                    break;
                case 2:
                    //0x08XY2: Set reg VX &= VY
                    chip8->V[chip8->inst.X] &= chip8->V[chip8->inst.Y];
                    break;

                case 3:
                    //0x08XY3:Set reg VX ^= VY
                    chip8->V[chip8->inst.X] ^= chip8->V[chip8->inst.Y];
                    break;

                case 4:
                    //0x08XY4:Set reg VX += VY, set VF to 1 if carry
                    if((uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255){
                        chip8->V[0xF] = 1;
                    } else {
                        chip8->V[0xF] = 0;
                    }
                    chip8->V[chip8->inst.X] += chip8->V[chip8->inst.Y];

                    break;

                case 5:
                    //0x08XY5:Set reg VX -= VY, set VF to 1 if there is no carry (result is positive)
                    if(chip8->V[chip8->inst.X] >= chip8->V[chip8->inst.Y]){
                        chip8->V[0xF] = 1;
                    } else {
                        chip8->V[0xF] = 0;
                    }
                    chip8->V[chip8->inst.X] -= chip8->V[chip8->inst.Y];
                    break;
                
                case 6:
                    // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
                    chip8->V[0xF] = (chip8->V[chip8->inst.X] & 1);
                    chip8->V[chip8->inst.X] >>= 1;
                    break;                  
                
                case 7:
                    //0x08XY7: Sets VX to VY minus VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX).
                    if(chip8->V[chip8->inst.Y] >= chip8->V[chip8->inst.X]){
                        chip8->V[0xF] = 1;
                    } else {
                        chip8->V[0xF] = 0;
                    }
                    chip8->V[chip8->inst.X] = (chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X]);
                    break;
                
                case 0xE:
                    //0x08XYE: Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset
                    chip8->V[0xF] = (chip8->V[chip8->inst.X] >> 7);
                    chip8->V[chip8->inst.X] <<= 1;
                    break;

                default:
                    break;  //Wrong opcode
            }

            break;
        
        case 0x09:
            // 0x9XY0: Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block).
                if(chip8->inst.N != 0) break; //Wrong opcode
                if(chip8->V[chip8->inst.X] != chip8->V[chip8->inst.Y]){
                    chip8->PC += 2;
                }
            break;
            

        case 0x0A:
            // 0xANNN: Set index reg I to nNN
            chip8->I = chip8->inst.NNN;
            break;
        
        case 0x0B:
            // 0xBNNN: Jumps to the address NNN plus V0.
            chip8->PC = chip8->inst.NNN + chip8->V[0];
            break;
        
        case 0x0D: {
            //0xDXYN: Draw N height sprite at coords X, Y: Read from mem location I;
            // Screen pixels are XOR'd with sprite bits, 
            // VF (carry flag) is set if any screen pixels are set off
            uint8_t X_coord = chip8->V[chip8->inst.X] % config->window_width;
            uint8_t Y_coord = chip8->V[chip8->inst.Y] % config->window_height;
            const uint8_t orig_X = X_coord;
            
            chip8->V[0xF] = 0; //Initalize carry flag to 0

            for( uint8_t i = 0; i < chip8->inst.N; i++){
                //Get next bite/row of sprite data
                const uint8_t sprite_data = chip8->ram[chip8->I + i];
                X_coord = orig_X;

                for( int j = 7; j >= 0; j--){
                    //If sprite bit and pixel bit is on, set carry flag
                    bool *pixel = &chip8->display[Y_coord * config->window_width + X_coord];
                    const bool sprite_bit = sprite_data & (1 << j);
                    
                    if(sprite_bit && *pixel ){
                            chip8->V[0xF] = 1; 
                       }
                    
                       //XOR display with prite pixel/bit to set it on or off
                       *pixel ^= (sprite_bit);

                       //Stop drawing if hit right edge of the screen
                       if(++X_coord  >= config->window_width) break;
                }
                //Stop drawing if it hits the bottom edge of the screen
                if(++Y_coord >= config->window_height) break;
            }
            break;
        }
       
        default:
            break; //Unimplememnted / invalid
    }
}

int main(int argc, char **argv) {
    //Default Usage message for args
    if(argc < 2){
        fprintf(stderr, "Usage: %s <rom_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

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
    if (argc < 2) {
        SDL_Log("Usage: %s <rom_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    if(!init_chip8(&chip8, argv[1])) exit(EXIT_FAILURE);

    //Inital screen clear to background color
    clear_screen(&sdl, &config);

    //Main emulator loop
    while (chip8.state != QUIT) {
        //Handle user input 
        handle_input(&chip8);
        //Get time();
        //Emulate CHIP8 Instrucitons
        emulate_instruction(&chip8, &config);
        //Get_time();

        clear_screen(&sdl, &config);

        //Delay 60fps (approx)
        //SDL_Delay(16 - actual time passed);
        SDL_Delay(16);
        // Update window with changes
        update_screen(&sdl, &config, &chip8);
    }

    //Final cleanup
    final_cleanup(&sdl);

    exit(EXIT_SUCCESS);
}
