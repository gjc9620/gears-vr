#include "hand_world_continuity_bridge.h"
#include "stereo_bridge.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <thread>

namespace hc=gears_hand_continuity;
constexpr std::uint64_t native_activation=1234567;
static hc::WorldFrame frame(std::int64_t now) {
    hc::WorldFrame f{};auto& p=f.pose;
    p.session=7;p.sequence=1;p.space_epoch=2;p.input_epoch=3;p.world_generation=4;
    p.controller=100;p.pawn=200;p.weapon=300;p.units_per_metre=100;p.valid_mask=3;
    auto& c=f.certificate;
    c.native_started_qpc=native_activation;c.xr_publisher_pid=GetCurrentProcessId();
    c.xr_publisher_start=gears_vr::controller_process_start(GetCurrentProcess());
    c.stream_epoch=1;c.reference_epoch=2;c.reference_generation=1;
    c.tracking_epoch[0]=c.tracking_epoch[1]=1;c.profile_generation=5;
    c.stereo_received_ns=now-1;c.input_received_ns=now;
    return f;
}
struct Mapping {
    HANDLE mapping{},mutex{};hc::WorldPacket* data{};
    Mapping() {
        wchar_t name[128]{};const auto start=gears_vr::controller_process_start(GetCurrentProcess());
        swprintf_s(name,L"Local\\GearsVR.WorldHandContinuity.v1.%lu.%llu",GetCurrentProcessId(),start);
        mapping=OpenFileMappingW(FILE_MAP_WRITE,FALSE,name);assert(mapping);
        data=static_cast<hc::WorldPacket*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(hc::WorldPacket)));assert(data);
        swprintf_s(name,L"Local\\GearsVR.WorldHandContinuityLock.v1.%lu.%llu",GetCurrentProcessId(),start);
        mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,name);assert(mutex);
    }
    template<class F> void change(F edit) {
        assert(WaitForSingleObject(mutex,0)==WAIT_OBJECT_0);edit(*data);assert(ReleaseMutex(mutex));
    }
    ~Mapping(){if(data)UnmapViewOfFile(data);if(mapping)CloseHandle(mapping);if(mutex)CloseHandle(mutex);}
};
struct Pair {
    hc::WorldBridge producer,consumer;
    std::uint64_t cursor{UINT64_MAX};
    Pair(){
        assert(producer.open(true,native_activation));assert(consumer.open(false,native_activation));
        hc::WorldFrame out{};assert(consumer.read(cursor,out)==hc::WorldRead::baseline);
        assert(cursor==0);
    }
};
int main() {
    {
        const auto now=gears_vr::monotonic_ns();auto f=frame(now);
        assert(hc::world_frame_shape_valid(f)&&hc::certificate_recent(f.certificate,now));
        assert(hc::certificate_recent(f.certificate,now+99999999));
        assert(!hc::certificate_recent(f.certificate,now+100000000)); // Earlier stereo timestamp.
        f.certificate.stereo_received_ns=now;assert(hc::certificate_recent(f.certificate,now+100000000));
        f.certificate.input_received_ns=now-1;assert(!hc::certificate_recent(f.certificate,now+100000000));
        f.certificate.input_received_ns=now+1;assert(!hc::certificate_recent(f.certificate,now));
        f=frame(now);auto copy=f.certificate;assert(hc::same_certificate(copy,f.certificate));
        ++copy.tracking_epoch[0];assert(!hc::same_certificate(copy,f.certificate));
    }
    {
        hc::WorldBridge unopened;hc::WorldFrame out=frame(gears_vr::monotonic_ns());std::uint64_t cursor=0;
        assert(unopened.read(cursor,out)==hc::WorldRead::rejected);
        assert(!out.pose.serial&&!out.certificate.native_started_qpc);
        assert(!unopened.open(true,0)&&!unopened.open(false,0));
        Pair p;hc::WorldBridge duplicate,wrong;
        assert(!duplicate.open(true,native_activation));assert(!wrong.open(false,native_activation+1));
        // Both protocols coexist in the same process and maintain separate serials.
        gears_weapon::EquipmentIntentBridge legacy;assert(legacy.open(true));
        const auto now=gears_vr::monotonic_ns();auto source=frame(now);
        source.pose.serial=999;source.pose.created_ms=1;
        assert(p.producer.publish(source));assert(source.pose.serial==999&&source.pose.created_ms==1);
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::received);
        assert(out.pose.serial==1&&out.pose.created_ms>1&&hc::same_certificate(out.certificate,source.certificate));
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::none&&!out.pose.serial);
        assert(legacy.publish_pose(source.pose));
        hc::WorldBridge late;assert(late.open(false,native_activation));std::uint64_t late_cursor=UINT64_MAX;
        assert(late.read(late_cursor,out)==hc::WorldRead::baseline&&late_cursor==1);
    }
    {
        Pair p;const auto now=gears_vr::monotonic_ns();auto f=frame(now);hc::WorldFrame out{};
        assert(p.producer.publish(f));assert(p.consumer.read(p.cursor,out)==hc::WorldRead::received);
        assert(!p.producer.publish(f));assert(p.consumer.read(p.cursor,out)==hc::WorldRead::none);
        // StereoGate may reuse an identical eye ticket and even recenter it.
        // Neither case manufactures a new source receipt for the strict writer.
        ++f.certificate.reference_epoch;++f.certificate.reference_generation;
        assert(!p.producer.publish(f));assert(p.consumer.read(p.cursor,out)==hc::WorldRead::none);
        ++f.pose.sequence;
        assert(p.producer.publish(f));assert(p.consumer.read(p.cursor,out)==hc::WorldRead::received);
        assert(out.pose.serial==2&&out.pose.sequence==2&&out.certificate.reference_generation==2);
        --f.pose.sequence;assert(!p.producer.publish(f));
    }
    for(unsigned invalid=0;invalid<6;++invalid){
        Pair p;const auto now=gears_vr::monotonic_ns();auto f=frame(now);
        switch(invalid){
        case 0:f.certificate.native_started_qpc++;break;
        case 1:f.certificate.xr_publisher_start=0;break;
        case 2:f.pose.valid_mask=4;break;
        case 3:f.pose.grip_down_mask=4;break;
        case 4:f.pose.left.position.x=std::numeric_limits<double>::quiet_NaN();break;
        case 5:f.certificate.input_received_ns=now+1000000000;break;
        }
        assert(!p.producer.publish(f));hc::WorldFrame out{};
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::none&&p.cursor==0);
    }
    // New consumed packets rejected for source age/future or payload corruption
    // cannot reappear after time catches up or the same payload is repaired.
    for(unsigned invalid=0;invalid<5;++invalid){
        Pair p;const auto now=gears_vr::monotonic_ns();auto f=frame(now);assert(p.producer.publish(f));
        Mapping map;hc::WorldFrame out{};
        if(invalid==0)map.change([&](auto& x){x.frame.certificate.input_received_ns=now+1000000000;});
        if(invalid==1)map.change([&](auto& x){x.frame.certificate.stereo_received_ns=now-100000001;});
        if(invalid==2)map.change([](auto& x){x.frame.pose.valid_mask=4;});
        if(invalid==3)map.change([](auto& x){x.frame.pose.created_ms=GetTickCount64()+1000;});
        if(invalid==4)map.change([](auto& x){x.frame.certificate.native_started_qpc++;});
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::rejected&&p.cursor==1);
        assert(!out.pose.serial&&!out.certificate.native_started_qpc);
        map.change([&](auto& x){f.pose.serial=1;f.pose.created_ms=GetTickCount64();x.frame=f;});
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::none);
    }
    for(unsigned invalid=0;invalid<6;++invalid){
        Pair p;const auto now=gears_vr::monotonic_ns();assert(p.producer.publish(frame(now)));
        hc::WorldFrame out{};assert(p.consumer.read(p.cursor,out)==hc::WorldRead::received);
        Mapping map;
        map.change([&](auto& x){
            switch(invalid){
            case 0:++x.native_started_qpc;break;
            case 1:++x.game_start;break;
            case 2:++x.qpc_frequency;break;
            case 3:--x.size;break;
            case 4:x.latest_serial=0;break;
            case 5:x.latest_serial=UINT64_MAX;break;
            }
        });
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::rejected);
    }
    {
        Pair p;const auto now=gears_vr::monotonic_ns();auto f=frame(now);
        f.pose.valid_mask=f.pose.grip_down_mask=0;f.certificate.tracking_epoch[0]=2;
        assert(p.producer.publish(f));hc::WorldFrame out{};
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::received);
        assert(out.pose.valid_mask==0&&out.certificate.tracking_epoch[0]==2&&out.certificate.tracking_epoch[1]==1);
        Mapping map;map.change([](auto& x){x.latest_serial=UINT64_MAX-1;});
        assert(!p.producer.publish(f));
        map.change([](auto& x){assert(x.latest_serial==UINT64_MAX-1);});
    }
    {
        Pair p;Mapping map;HANDLE acquired=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        HANDLE release=CreateEventW(nullptr,TRUE,FALSE,nullptr);assert(acquired&&release);
        std::thread holder([&]{assert(WaitForSingleObject(map.mutex,0)==WAIT_OBJECT_0);
            SetEvent(acquired);WaitForSingleObject(release,INFINITE);assert(ReleaseMutex(map.mutex));});
        assert(WaitForSingleObject(acquired,5000)==WAIT_OBJECT_0);hc::WorldFrame out{};
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::none);
        SetEvent(release);holder.join();CloseHandle(acquired);CloseHandle(release);
        std::thread abandoned([&]{assert(WaitForSingleObject(map.mutex,0)==WAIT_OBJECT_0);});abandoned.join();
        assert(p.consumer.read(p.cursor,out)==hc::WorldRead::rejected);
    }
    std::puts("world-hand continuity: fixed incarnation, separate serials, strict source clocks, consumed rejection and IPC checks passed");
}
