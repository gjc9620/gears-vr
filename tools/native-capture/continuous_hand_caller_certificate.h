#pragma once
#include <array>
#include <cstdint>
namespace continuous_hand_caller_certificate {
// Historical evidence identifiers only. Startup must hash the CURRENT bytes.
struct Chunk {std::uintptr_t rva;unsigned size;char const* sha256;};
inline constexpr std::array<Chunk,2> chunks{{
    {0x12e330,798,"0bee3d77e2a1af4f96a0a7c8af7afb36991787e8e6918052aefa656d3393d43a"},
    {0x8a9e74,26,"594984067d046f94112ec0009466b82b4b1f21a26d794a8bb2bb572c2ee00964"},
}};
inline constexpr std::array<unsigned char,16> callsite{{0x49,0x8b,0x00,0x49,0x8b,0xc8,0xc5,0xf8,0x28,0xce,0xff,0x90,0x60,0x02,0x00,0x00}};
} // namespace continuous_hand_caller_certificate
