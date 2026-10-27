#pragma once
#include "../vr-common/hand_personal_calibration.h"
#include <cstdio>
#include <cwchar>

// Startup configuration data only. Loading a record grants no tracking, model,
// thread, or pose-write permission. The worker commits it before activation;
// active hand plans only read this immutable per-session value.
namespace hand_personal_calibration_live {
using Measurement=gears_hand_personal::Measurement;
Measurement saved{};
bool loaded{};

enum class ConfigStatus { okay, partial, inactive, invalid_number, invalid_measurement };
inline ConfigStatus parse(wchar_t const* span,wchar_t const* height,bool stereo,bool has_consumer,
    Measurement& output) noexcept {
    if(!span||!height)return ConfigStatus::invalid_number;
    const bool have_span=*span!=0,have_height=*height!=0;
    if(have_span!=have_height)return ConfigStatus::partial;
    if(!have_span){output={};return ConfigStatus::okay;}
    if(!stereo||!has_consumer)return ConfigStatus::inactive;
    wchar_t* span_end{};wchar_t* height_end{};
    Measurement candidate{1,std::wcstod(span,&span_end),std::wcstod(height,&height_end)};
    if(span_end==span||height_end==height||*span_end||*height_end)return ConfigStatus::invalid_number;
    if(!candidate.valid())return ConfigStatus::invalid_measurement;
    output=candidate;return ConfigStatus::okay;
}

inline bool configure(wchar_t const* ini,bool stereo,bool has_consumer,FILE* status){
    wchar_t span[64]{},height[64]{};
    const auto span_size=GetPrivateProfileStringW(L"Capture",L"weapon_armspan_metres",L"",span,64,ini);
    const auto height_size=GetPrivateProfileStringW(L"Capture",L"weapon_hands_below_head_metres",L"",height,64,ini);
    if(span_size>=63||height_size>=63){
        if(status)std::fprintf(status,"rejected: stored armspan calibration exceeds its numeric field size\n");
        return false;
    }
    Measurement candidate{};
    const auto result=parse(span,height,stereo,has_consumer,candidate);
    if(result!=ConfigStatus::okay){
        if(status)std::fprintf(status,"rejected: invalid or inactive stored armspan calibration (status=%u)\n",static_cast<unsigned>(result));
        return false;
    }
    saved=candidate;loaded=candidate.valid();
    if(status)std::fprintf(status,
        "hand_personal_calibration_loaded=%u armspan_metres=%.17g hands_below_head_metres=%.17g method=controller_span_model_estimate physical_verification=0 writer=0\n",
        unsigned(loaded),loaded?saved.armspan_metres:0.,loaded?saved.hands_below_head_metres:0.);
    return true;
}
} // namespace hand_personal_calibration_live
