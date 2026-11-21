#include "weapon_boomshot_grasp_pose.h"
#include <cstdio>

int main(){
    namespace p=gears_boomshot_grasp_pose;
    p::api::Joints reference{};reference.fill({0,0,0,1});
    const auto geometry=gears_hand_authored_mount::marcus115_signature;
    gears_weapon_hand::Frame source{};p::api::Request main{},carrier{};
    if(!p::request(p::Part::main_grip,geometry,reference,source,{},false,main)||!p::api::structural(main))return 1;
    if(!p::request(p::Part::carrier,geometry,reference,source,{{0,0,0},{0,0,0,1}},false,carrier)||!p::api::structural(carrier))return 2;
    for(unsigned i=0;i<15;++i)if(main.closed[i].x!=p::main_captured[i].x||main.closed[i].y!=p::main_captured[i].y||
        main.closed[i].z!=p::main_captured[i].z||main.closed[i].w!=p::main_captured[i].w||
        main.trigger_closed[i].x!=main.closed[i].x||main.trigger_closed[i].y!=main.closed[i].y||
        main.trigger_closed[i].z!=main.closed[i].z||main.trigger_closed[i].w!=main.closed[i].w)return 3;
    for(unsigned i=0;i<15;++i)if(carrier.closed[i].x!=p::carrier_captured[i].x||carrier.closed[i].y!=p::carrier_captured[i].y||
        carrier.closed[i].z!=p::carrier_captured[i].z||carrier.closed[i].w!=p::carrier_captured[i].w)return 4;
    std::puts("Boomshot captured finger structure: main=15 captured trigger=no-op carrier=15");
    return 0;
}
