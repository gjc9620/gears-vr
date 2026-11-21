#include "weapon_longshot_history_evidence.h"
#include <cassert>
#include <cstring>
#include <cwchar>
#include <vector>
namespace e=gears_longshot_history_evidence;
namespace c=gears_hand_continuity;
namespace r=gears_controller_relative_motion;
c::TrackedInput input(unsigned sequence){
    c::TrackedInput value{};value.stream_epoch=2;value.tracking_epoch[0]=value.tracking_epoch[1]=1;
    auto& p=value.input;p.session=1;p.sequence=sequence;p.space_epoch=p.input_epoch=p.profile_generation=1;
    p.sync_ns=1000000000+std::int64_t(sequence-1)*22448260;p.received_ns=p.sync_ns+1000;
    p.predicted_time=p.sync_ns+10000000;p.focused=1;
    for(auto& h:p.hands){h.grip_active=h.aim_active=h.squeeze_active=h.trigger_active=1;h.grip_flags=h.aim_flags=15;}
    p.hands[0].squeeze=1;p.hands[0].pressed[gears_vr::control_squeeze]=1;
    p.hands[0].grip.position.z=-.002*(sequence-1);return value;
}
bool replay(e::Record const& value){
    r::Segment out{};out.count=99;
    const bool accepted=r::build(value.previous,value.current,value.window,value.control,value.fixed_part_in_right_m,value.scale,out);
    assert(accepted?out.count==6:out.count==99);return accepted;
}
int main(int argc,char** argv){
    unsigned cases{};
    for(unsigned scenario=0;scenario<5;++scenario){
        e::Record source{};source.game_pid=37448;source.game_start=134387277995045930;
        source.serial=source.token=1;source.frame=746;source.control=gears_vr::control_squeeze;source.scale=1;
        source.previous=input(1);source.current=input(6);source.window.count=5;
        for(unsigned i=0;i<5;++i)source.window.frames[i]=input(i+2);
        // Keep the actual observed endpoint gap (112.2413ms), while varying
        // facts absent from37448 logs. These are fixtures, not its raw ring.
        if(scenario==1){source.window.count=1;source.window.frames[0]=source.current;}
        if(scenario==2)source.window.frames[2].tracking_epoch[0]=2;
        if(scenario==3)source.window.frames[2].input.predicted_time=source.window.frames[1].input.predicted_time;
        if(scenario==4){source.window.frames[2].input.hands[0].pressed[source.control]=2;source.window.frames[2].input.hands[0].released[source.control]=1;}
        const bool before=replay(source);assert(before==(scenario==0));
        if(argc==2&&scenario==1){FILE* artifact{};assert(fopen_s(&artifact,argv[1],"w, ccs=UTF-8")==0&&artifact);
            assert(e::write(artifact,source));assert(fwprintf(artifact,L"longshot_bolt_history_raw_end serial=1 frame=746 complete=1 motion_only=1 action_already_decided=1\n")>0);fclose(artifact);}
        const auto unchanged=source;FILE* log{};assert(tmpfile_s(&log)==0&&log);
        assert(e::write(log,source));assert(std::memcmp(&source,&unchanged,sizeof(source))==0);rewind(log);
        e::Record decoded{};std::vector<unsigned char> raw(sizeof(decoded));std::size_t filled{};
        wchar_t line[512]{};
        while(fgetws(line,512,log)){
            unsigned long long serial{},frame{};std::size_t bytes{},offset{};wchar_t hex[129]{};
            assert(swscanf_s(line,L"longshot_bolt_history_raw_v1 serial=%llu frame=%llu bytes=%zu offset=%zu hex=%128ls",
                &serial,&frame,&bytes,&offset,hex,unsigned(std::size(hex)))==5);
            const auto count=wcslen(hex);assert(serial==1&&frame==746&&bytes==sizeof(decoded)&&offset==filled&&count%2==0);
            auto digit=[](wchar_t ch){assert((ch>=L'0'&&ch<=L'9')||(ch>=L'a'&&ch<=L'f'));return ch<=L'9'?ch-L'0':ch-L'a'+10;};
            for(std::size_t i=0;i<count;i+=2){assert(filled<raw.size());raw[filled++]=static_cast<unsigned char>(digit(hex[i])*16+digit(hex[i+1]));}
        }
        fclose(log);assert(filled==sizeof(decoded));std::memcpy(&decoded,raw.data(),sizeof(decoded));
        assert(e::compatible(decoded)&&replay(decoded)==before);
        assert(c::same_tracking(decoded.previous,source.previous)&&c::same_tracking(decoded.current,source.current));
        assert(decoded.window.count==source.window.count);
        for(unsigned i=0;i<source.window.count;++i)assert(c::same_tracking(decoded.window.frames[i],source.window.frames[i]));
        ++decoded.tracked_bytes;assert(!e::compatible(decoded));++cases;
    }
    assert(!e::write(nullptr,{}));
    std::printf("LongshotHistoryEvidenceTest passed:%u scenarios; exact raw roundtrip, unchanged production replay and inputs, truncated ring preserved, ABI mismatch rejected; fixtures only\n",cases);
}
