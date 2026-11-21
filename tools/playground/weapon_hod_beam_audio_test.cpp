// Production audio selector + real HOD owner/input/shared R2 ledger.
// Synthetic typed reads and original calls; no retail/audio/runtime proof.
#define main hod_existing_input_regressions
#include "weapon_hod_input_test.cpp"
#undef main
#include "../vr-common/weapon_hod_beam_audio.h"
#include <cwchar>
struct AudioRead {
    bool playing_flags[3]{},pointer_ok[3]{true,true,true},flag_ok[3]{true,true,true};
    unsigned pointer_reads[3]{},flag_reads[3]{};bool null_components{};
    template<std::size_t N>bool pointer(uptr beam,wchar_t const(&name)[N],uptr& out,bool component) noexcept {
        if(beam!=700||!component)return false;
        unsigned i=std::wcscmp(name,L"FireLoopSound")==0?0:
            std::wcscmp(name,L"WarmupSound")==0?1:std::wcscmp(name,L"RippingEarthLoopSound")==0?2:3;
        if(i==3)return false;++pointer_reads[i];if(!pointer_ok[i])return false;
        out=null_components?0:800+i;return true;
    }
    bool playing(uptr at,bool& out) noexcept {
        if(!at){out=false;return true;}
        if(at<800||at>802)return false;const auto i=unsigned(at-800);++flag_reads[i];
        if(!flag_ok[i])return false;out=playing_flags[i];return true;
    }
};
int main(){
    if(hod_existing_input_regressions())return 1;
    {Fixture f;f.press();AudioRead read;read.playing_flags[2]=true;bool audio{};
        if(!gears_weapon_hod_beam_audio::sample(read,700,audio))return 2;
        hod_state::actual.beam_audio=audio;f.release();
        if(!api::owed()){std::fputs("FAIL: ripping-earth loop tail incorrectly cleared original effect debt\n",stderr);return 3;}
        api::service();api::service();if(releases!=1||ends!=0)return 4;
        read.playing_flags[2]=false;
        if(!gears_weapon_hod_beam_audio::sample(read,700,audio))return 5;
        hod_state::actual.beam_audio=audio;api::service();if(api::owed()||releases!=1||ends)return 6;}
    for(unsigned channel=0;channel<3;++channel){
        AudioRead read;read.playing_flags[channel]=true;bool audio{};
        if(!gears_weapon_hod_beam_audio::sample(read,700,audio)||!audio)return 7;
        for(unsigned i=0;i<3;++i)if(read.pointer_reads[i]!=1||read.flag_reads[i]!=1)return 8;
        read.pointer_ok[channel]=false;audio=true;
        if(gears_weapon_hod_beam_audio::sample(read,700,audio)||!audio)return 9;
        read.pointer_ok[channel]=true;read.flag_ok[channel]=false;
        if(gears_weapon_hod_beam_audio::sample(read,700,audio)||!audio)return 10;
    }
    {AudioRead read;read.null_components=true;bool audio=true;
        if(!gears_weapon_hod_beam_audio::sample(read,700,audio)||audio)return 11;}
    std::puts("HOD all three original beam audio components: tail debt, no repeated OFF, missing fields and typed null passed (synthetic only)");
    return 0;
}
