#include "reference_space_epoch.h"
#include <cstdio>
#include <stdexcept>

static unsigned checks{};
static void require(bool value,const char* label){++checks;if(!value)throw std::runtime_error(label);}
int main(){try{
    using gears_xr::ReferenceSpaceEpoch;
    ReferenceSpaceEpoch future;
    require(future.epoch()==1&&future.last_change_time()==0&&future.pending()==0,"initial epoch");
    require(future.enqueue(100)&&future.epoch()==1,"enqueue never applies early");
    require(!future.advance(99)&&!future.advance_rejected()&&future.epoch()==1&&future.pending()==1,"future retained");
    require(future.advance(100)&&future.epoch()==2&&future.last_change_time()==100&&future.pending()==0,"boundary applies");
    require(!future.advance(100)&&!future.advance_rejected(),"same predicted time accepted unchanged");
    require(future.enqueue(100)&&future.enqueue(50)&&future.pending()==0,"already applied events ignored");
    require(!future.advance(99)&&future.advance_rejected()&&future.last_predicted_time()==100,"predicted regression rejected");
    require(!future.enqueue(0)&&!future.enqueue(-1),"invalid event times");
    require(!future.advance(0)&&future.advance_rejected(),"zero predicted time");
    require(!future.advance(-1)&&future.advance_rejected(),"negative predicted time");

    ReferenceSpaceEpoch sorted;
    require(sorted.enqueue(300)&&sorted.enqueue(100)&&sorted.enqueue(200)&&sorted.enqueue(200)&&sorted.pending()==3,"sort and deduplicate");
    require(sorted.advance(200)&&sorted.epoch()==3&&sorted.last_change_time()==200&&sorted.pending()==1,"two due events applied");
    require(!sorted.advance(250)&&!sorted.advance_rejected()&&sorted.pending()==1,"later event not early");
    require(sorted.advance(300)&&sorted.epoch()==4&&sorted.last_change_time()==300,"last queued event");
    require(sorted.enqueue(200)&&sorted.pending()==0&&!sorted.advance(301)&&sorted.epoch()==4,"late duplicate cannot bump epoch");

    ReferenceSpaceEpoch full;
    for(int i=16;i>=1;--i)require(full.enqueue(i*10),"fill out of order");
    require(full.pending()==16&&!full.enqueue(170)&&full.pending()==16,"capacity bounded");
    require(full.enqueue(80)&&full.pending()==16,"duplicate accepted when full");
    require(full.advance(10)&&full.epoch()==2&&full.pending()==15,"release one queue slot");
    require(full.enqueue(170)&&full.pending()==16,"reuse capacity");
    require(full.advance(170)&&full.epoch()==18&&full.pending()==0&&full.last_change_time()==170,"apply all remaining events");

    ReferenceSpaceEpoch past;
    require(!past.advance(500)&&past.enqueue(400)&&past.epoch()==1,"late delivery not applied by enqueue");
    require(!past.advance(499)&&past.advance_rejected()&&past.pending()==1,"late event does not permit time regression");
    require(past.advance(500)&&past.epoch()==2&&past.last_change_time()==400,"late delivery applies at next monotonic advance");

    const auto maximum=(std::numeric_limits<std::uint64_t>::max)();
    ReferenceSpaceEpoch overflow(maximum-1);
    require(overflow.enqueue(10)&&overflow.enqueue(20),"overflow setup");
    require(!overflow.advance(20)&&overflow.advance_rejected()&&overflow.epoch()==maximum-1&&overflow.pending()==2&&
        overflow.last_change_time()==0&&overflow.last_predicted_time()==0,"overflow rejects atomically");
    require(overflow.advance(10)&&overflow.epoch()==maximum&&overflow.pending()==1,"last representable epoch");
    require(!overflow.advance(20)&&overflow.advance_rejected()&&overflow.last_change_time()==10&&overflow.pending()==1,"no epoch wrap");
    require(overflow.enqueue(10)&&overflow.pending()==1,"old duplicate at maximum ignored");
    std::printf("reference space epoch: %u checks passed\n",checks);return 0;
}catch(std::exception const& error){std::fprintf(stderr,"reference epoch: %s\n",error.what());return 1;}}
