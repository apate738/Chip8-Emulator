#include "chip8.h"

void chip8_init(chip8_t *chip)
{
    memset(chip, 0, sizeof(chip8_t));
    chip->pc = ROM_START;
    memcpy(&chip->memory[FONT_START], FONT_SET, sizeof(FONT_SET));
}

int chip8_load_rom(chip8_t *chip, const char *path)
{
    FILE *rom = fopen(path, "rb");
    if (!rom)
    {
        fprintf(stderr, "Error: could not open ROM '%s'\n", path);
        return -1;
    }

    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    long max_size = MEMORY_SIZE - ROM_START;
    if (rom_size > max_size)
    {
        fprintf(stderr, "Error: ROM too large (%ld bytes, max %ld)\n", rom_size, max_size);
        fclose(rom);
        return -1;
    }

    size_t bytes_read = fread(&chip->memory[ROM_START], 1, rom_size, rom);
    if (bytes_read != (size_t)rom_size)
    {
        fprintf(stderr, "Error: failed to read ROM (got %zu of %ld bytes)\n", bytes_read, rom_size);
        fclose(rom);
        return -1;
    }

    fclose(rom);
    printf("Loaded ROM '%s' (%ld bytes) at 0x%03X\n", path, rom_size, ROM_START);
    return 0;
}

void chip8_debug_memory(const chip8_t *chip, uint16_t start, uint16_t end)
{
    printf("\n=== Memory Dump [0x%03X - 0x%03X] ===\n", start, end);
    for (uint16_t i = start; i <= end; i++)
    {
        if ((i - start) % 16 == 0)
            printf("\n0x%03X: ", i);
        printf("%02X ", chip->memory[i]);
    }
    printf("\n\n");
}

uint16_t chip8_fetch(chip8_t *chip)
{
    uint16_t opcode = ((uint16_t)chip->memory[chip->pc] << 8) | chip->memory[chip->pc + 1];
    chip->pc += 2;
    return opcode;
}

void chip8_execute(chip8_t *chip, uint16_t opcode)
{
    // Pre-extract all possible fields — not every opcode uses all of them
    uint8_t x = (opcode & 0x0F00) >> 8; // register index (0-15)
    uint8_t y = (opcode & 0x00F0) >> 4; // register index (0-15)
    uint8_t kk = (opcode & 0x00FF);     // 8-bit immediate value
    uint16_t nnn = (opcode & 0x0FFF);   // 12-bit address
    // uint8_t n = (opcode & 0x000F);        // 4-bit nibble — not needed today

    switch (opcode & 0xF000)
    {

    case 0x0000:
        switch (opcode)
        {
        case 0x00E0:
            // Clear display — zero out every pixel
            memset(chip->display, 0, sizeof(chip->display));
            chip->draw_flag = 1;
            break;
        case 0x00EE:
            // Return from subroutine — implement Day 4
            printf("00EE not yet implemented\n");
            break;
        default:
            printf("Unknown 0x0000 opcode: 0x%04X\n", opcode);
            break;
        }
        break;

    case 0x1000:
        // 1NNN — Jump to address NNN
        chip->pc = nnn;
        break;

    case 0x2000:
        // 2NNN — Call subroutine — implement Day 4
        printf("2NNN not yet implemented\n");
        break;

    case 0x3000:
        // 3XKK — Skip next instruction if V[X] == KK
        if (chip->V[x] == kk)
            chip->pc += 2;
        break;

    case 0x4000:
        // 4XKK — Skip next instruction if V[X] != KK
        if (chip->V[x] != kk)
            chip->pc += 2;
        break;

    case 0x5000:
        // 5XY0 — Skip next instruction if V[X] == V[Y]
        if (chip->V[x] == chip->V[y])
            chip->pc += 2;
        break;

    case 0x6000:
        // 6XKK — Set V[X] = KK
        chip->V[x] = kk;
        break;

    case 0x7000:
        // 7XKK — Set V[X] = V[X] + KK (no carry flag)
        chip->V[x] += kk;
        break;

    case 0x9000:
        // 9XY0 — Skip next instruction if V[X] != V[Y]
        if (chip->V[x] != chip->V[y])
            chip->pc += 2;
        break;

    case 0xA000:
        // ANNN — Set I = NNN
        chip->I = nnn;
        break;

    case 0xB000:
        // BNNN — Jump to address NNN + V0
        chip->pc = nnn + chip->V[0];
        break;

    case 0xD000:
    {
        uint8_t x_pos = chip->V[x] % 64; // wrap around screen edges
        uint8_t y_pos = chip->V[y] % 32;
        uint8_t height = opcode & 0x000F; // n = number of rows to draw

        chip->V[0xF] = 0; // reset collision flag before drawing

        for (int row = 0; row < height; row++)
        {
            // fetch the sprite byte for this row from memory at I + row
            uint8_t sprite_byte = chip->memory[chip->I + row];

            for (int col = 0; col < 8; col++)
            {
                // check each bit from left to right (bit 7 down to bit 0)
                // if the bit is 1, this pixel is active in the sprite
                if (sprite_byte & (0x80 >> col))
                {
                    int px = (x_pos + col) % 64; // wrap horizontally
                    int py = (y_pos + row) % 32; // wrap vertically

                    int index = py * 64 + px;

                    // collision — if screen pixel is already on, set VF
                    if (chip->display[index] == 1)
                        chip->V[0xF] = 1;

                    // XOR the pixel — flips it on if off, off if on
                    chip->display[index] ^= 1;
                }
            }
        }

        chip->draw_flag = 1;
        break;
    }

    default:
        printf("Unknown opcode: 0x%04X at PC: 0x%03X\n", opcode, chip->pc - 2);
        break;
    }
}

void chip8_cycle(chip8_t *chip)
{
    uint16_t opcode = chip8_fetch(chip);
    chip8_execute(chip, opcode);

    if (chip->delay_timer > 0)
        chip->delay_timer--;
    if (chip->sound_timer > 0)
        chip->sound_timer--;
}