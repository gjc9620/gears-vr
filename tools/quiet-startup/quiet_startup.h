#pragma once
#include <Windows.h>
#include <cstdint>

namespace gears_quiet {
constexpr DWORD abi = 1;
enum Call : unsigned { show, foreground, active, focus, position, bring, create_w, create_a, count };
struct Status {
    DWORD version{abi};
    DWORD installed{};
    std::uint64_t suppressed[count]{};
};
using Install = DWORD (WINAPI*)(void*);
using Read = DWORD (WINAPI*)(Status*);
}
