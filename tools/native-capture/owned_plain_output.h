#pragma once
#include <array>
#include <cstdint>

namespace gears_owned_plain_output {
// The ordinary Output fallback also runs with DLSS disabled. Admit only its
// unchanged identity mapping into the active owned renderer's runtime extent.
// Controlled/upscaled quads continue to require their separate output proof.
inline bool eligible(bool code_attested,bool owned,bool current,bool controlled,
    uintptr_t caller,const std::array<float,8>& args,const std::array<unsigned,4>& sizes){
    const auto width=sizes[0],height=sizes[1];
    if(!code_attested||!owned||!current||controlled||
       !width||width%2||!height||width>8192||height>8192)return false;
    // The ordinary Output pass can write an intermediate. Its guarded copy
    // helper then samples the same pixel rectangle from a larger pooled texture
    // into the owned target. Source pixels keep their identity mapping.
    const bool direct=caller==0xd83cbf&&sizes[2]==width&&sizes[3]==height;
    // MP_WarMachine also reaches e27709. The version-attested caller forwards
    // the same eye pixel rectangle from a larger pooled source into this
    // renderer's owned output. Shader semantics are not inferred from its RVA.
    const bool copied=(caller==0x2b11a0||caller==0xe27709)&&sizes[2]>=width&&sizes[3]>=height&&
        sizes[2]<=8192&&sizes[3]<=8192;
    if(!direct&&!copied)return false;
    const auto half=float(width/2),h=float(height);
    return args==std::array<float,8>{0,0,half,h,0,0,half,h}||
           args==std::array<float,8>{half,0,half,h,half,0,half,h};
}
}
