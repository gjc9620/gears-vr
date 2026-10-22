#include "hand_ik_xr_stream_request.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_hand_xr_trial;
namespace {
Start request(){
    Start value{};value.schema=1;value.size=sizeof(value);value.game_pid=77;
    value.game_creation_time=88;value.expected_native_world_generation=1;
    value.native_started_qpc=100;value.baseline_status_qpc=200;
    value.arm_deadline_ms=1500;value.baseline_world_time=4.5f;
    for(unsigned i=0;i<object_count;++i)value.objects[i]={0x1000u+i*64,0x2000,0x3000,5+i,10+i,0,0};
    return value;
}
bool accepts(Start const& value,std::uint64_t ms=1000,std::uint64_t qpc=250,std::uint64_t frequency=1000){
    return shape(value,77,88,ms,qpc,frequency);
}
void launch_window_and_external_epoch(){
    const auto good=request();assert(accepts(good));
    assert(accepts(good,1500)&&!accepts(good,1501));
    assert(accepts(good,500)&&!accepts(good,499));
    assert(accepts(good,1000,1200)&&!accepts(good,1000,1201));
    assert(!accepts(good,1000,199)&&!accepts(good,1000,250,0));
    for(auto epoch:{0ull,2ull,(std::numeric_limits<std::uint64_t>::max)()}){
        auto value=good;value.expected_native_world_generation=epoch;assert(!accepts(value));
    }
    auto value=good;value.native_started_qpc=0;assert(!accepts(value));
    value=good;value.native_started_qpc=201;assert(!accepts(value));
    assert(!shape(good,78,88,1000,250,1000)&&!shape(good,77,89,1000,250,1000));
}
void malformed_requests(){
    const auto good=request();
    for(unsigned field=0;field<6;++field){
        auto value=good;
        switch(field){
            case 0:++value.schema;break;case 1:--value.size;break;
            case 2:value.reserved=1;break;case 3:value.reserved2=1;break;
            case 4:value.game_pid=0;break;case 5:value.game_creation_time=0;break;
        }
        assert(!accepts(value));
    }
    for(float invalid:{-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){
        auto value=good;value.baseline_world_time=invalid;assert(!accepts(value));
    }
    for(unsigned role=0;role<object_count;++role)for(unsigned field=0;field<4;++field){
        auto value=good;auto& key=value.objects[role];
        switch(field){case 0:key.address=0;break;case 1:key.object_class=0;break;
            case 2:key.outer=0;break;case 3:key.reserved=1;break;}
        assert(!accepts(value));
    }
    // Header indices/names are runtime identity checks, not guessed bounds.
    auto value=good;value.objects[pawn].name_number=3;assert(accepts(value));
}
}
int main(){
    launch_window_and_external_epoch();malformed_requests();
    std::puts("Private XR stream arming identity, external epoch and freshness limits passed");
}
