// Isolated retail-game verification publisher. Never included in the launcher.
#include "../vr-common/controller_input_bridge.h"
#include "../vr-common/pose_bridge.h"
#include "../vr-common/stereo_bridge.h"
#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc,char** argv){
    if(argc!=3)return 2;
    const auto pid=static_cast<DWORD>(std::strtoul(argv[1],nullptr,10));
    if(!pid)return 2;
    HANDLE game=OpenProcess(SYNCHRONIZE,FALSE,pid);
    if(!game)return 3;
    gears_vr::StereoBridge stereo;
    gears_vr::ControllerInputBridge hands;
    if(!stereo.open(pid,true)||!hands.open(pid,true)){
        CloseHandle(game);return 4;
    }
    gears_vr::ControllerEdgeCounter edges;
    std::uint64_t sequence=0;
    unsigned consecutive_misses=0,total_misses=0;
    std::puts("vr-fire-test-publisher-ready");std::fflush(stdout);
    while(WaitForSingleObject(game,0)==WAIT_TIMEOUT){
        std::string action;
        HANDLE control=CreateFileA(argv[2],GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,
            OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(control!=INVALID_HANDLE_VALUE){
            char bytes[32]{};DWORD count{};
            if(ReadFile(control,bytes,sizeof(bytes)-1,&count,nullptr))
                action.assign(bytes,count);
            CloseHandle(control);
        }
        if(action=="stop")break;
        if(action=="pause"){Sleep(10);continue;}
        const auto now=gears_vr::monotonic_ns();
        gears_vr::StereoSample sample{};
        sample.source=gears_vr::StereoSource::MetaOperatorSimulated;
        sample.session=std::uint64_t(pid)+100000;sample.sequence=++sequence;
        sample.space_epoch=1;sample.predicted_time=now+10000000;
        sample.received_ns=now;sample.focused=1;
        gears_vr::ControllerInputFrame frame{};
        frame.source=sample.source;frame.session=sample.session;
        frame.sequence=sample.sequence;frame.space_epoch=sample.space_epoch;
        frame.predicted_time=sample.predicted_time;frame.focused=1;
        frame.sync_ns=now-1000;frame.received_ns=now;
        auto& left=frame.hands[0];
        left.grip_active=1;left.grip_flags=15;left.grip.orientation.w=1;
        left.trigger_active=1;left.trigger=(action=="aim"||action=="both")?.9f:0.f;
        auto& right=frame.hands[1];
        right.grip_active=1;right.grip_flags=15;right.grip.orientation.w=1;
        right.trigger_active=1;right.trigger=(action=="press"||action=="both")?.9f:0.f;
        edges.update(frame);
        if(!hands.publish(frame)||!stereo.publish(sample)){
            ++total_misses;
            if(++consecutive_misses>20){
                std::fprintf(stderr,"publish unavailable at %llu misses=%u\n",
                    static_cast<unsigned long long>(sequence),total_misses);
                CloseHandle(game);return 5;
            }
        }else{
            consecutive_misses=0;
        }
        Sleep(10);
    }
    std::printf("publisher_exit frames=%llu transient_misses=%u\n",
        static_cast<unsigned long long>(sequence),total_misses);
    CloseHandle(game);return 0;
}
