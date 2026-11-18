#pragma once
#include <cstdint>

namespace gears_weapon_hod_beam_audio {
// Caller retains the exact beam/manager/pawn identity and typed two-pass
// schema checks. An unreadable component or playing flag is never silence.
template<class Read> bool sample(Read& read,std::uintptr_t beam,bool& out) noexcept {
    std::uintptr_t fire{},warm{},ripping{};bool fire_playing{},warm_playing{},ripping_playing{};
    if(!read.pointer(beam,L"FireLoopSound",fire,true)||!read.playing(fire,fire_playing)||
       !read.pointer(beam,L"WarmupSound",warm,true)||!read.playing(warm,warm_playing)||
       !read.pointer(beam,L"RippingEarthLoopSound",ripping,true)||!read.playing(ripping,ripping_playing))return false;
    // Original DoHODCooldownEffects/GoDormant fade the ripping-earth loop
    // separately from the fire loop. Preserve debt until its actual readback.
    out=fire_playing||warm_playing||ripping_playing;return true;
}
}
