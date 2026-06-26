#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string_view>
#include <array>

namespace chip8 {

inline constexpr std::size_t  MEMORY_SIZE  = 4096;
inline constexpr std::uint16_t ROM_START   = 0x200;
inline constexpr std::uint16_t FONT_START  = 0x000;

inline constexpr std::size_t  DISPLAY_W    = 64;
inline constexpr std::size_t  DISPLAY_H    = 32;
inline constexpr std::size_t  STACK_DEPTH  = 16;
inline constexpr std::size_t  NUM_REGS     = 16;
inline constexpr std::size_t  NUM_KEYS     = 16;

inline constexpr std::array<std::uint8_t, 80> FONT_SET = {{
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
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
}};

struct State {
    std::array<std::uint8_t,  MEMORY_SIZE>       memory;
    std::array<std::uint8_t,  NUM_REGS>          V;
    std::uint16_t                                I;
    std::uint16_t                                pc;
    std::array<std::uint16_t, STACK_DEPTH>       stack;
    std::uint8_t                                 sp;
    std::uint8_t                                 delay_timer;
    std::uint8_t                                 sound_timer;
    std::array<std::uint8_t,  DISPLAY_W * DISPLAY_H> display;
    std::array<std::uint8_t,  NUM_KEYS>          key;
    bool                                         draw_flag;
};

void            init        (State &chip);
bool            load_rom    (State &chip, std::string_view path);
void            cycle       (State &chip);
void            debug_memory(const State &chip, std::uint16_t start, std::uint16_t end);
std::uint16_t   fetch       (State &chip);
void            execute     (State &chip, std::uint16_t opcode);

} // namespace chip8