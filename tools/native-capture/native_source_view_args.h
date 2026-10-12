#pragma once
#include <array>
#include <cstdint>
#include <cstring>

// Candidate ABI mapping from the observed 22-argument source constructor.
// This header does NOT select a native address, allocate, hook or call the game.
// The snapshot contains borrowed pointers and must not outlive its source frame.
namespace gears_source_args {
using Constructor=void*(__fastcall*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,
    float,float,float,float,const void*,const void*,const void*,const void*,const void*,const void*,float,unsigned);
struct Snapshot {
    alignas(8) std::array<unsigned char,0x430> bytes{};
    template<class T> T value(size_t offset) const {T out{};memcpy(&out,bytes.data()+offset,sizeof(out));return out;}
    void* pointer(size_t offset) const {return reinterpret_cast<void*>(value<uintptr_t>(offset));}
    // Caller supplies a validated constructor and a live, compatible owner/state.
    // Constructor input pointers reference this owned snapshot for call duration.
    void* invoke(Constructor fn,void* destination,void* owner,void* state) const {
        return fn(destination,owner,state,pointer(0x10),pointer(0x18),pointer(0x20),pointer(0x28),
            pointer(0x30),pointer(0x38),pointer(0x40),value<float>(0x48),value<float>(0x4c),
            value<float>(0x50),value<float>(0x54),bytes.data()+0x70,bytes.data()+0xb0,
            bytes.data()+0xf0,bytes.data()+0x100,bytes.data()+0x110,bytes.data()+0x140,
            value<float>(0x3e0),value<unsigned>(0x3e4));
    }
};
}
