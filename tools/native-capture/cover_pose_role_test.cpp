#include "cover_pose_role.h"
#include <cstdio>
int main(){unsigned checks=0,failed=0;
    struct Case {const char* name;bool selected,legacy,input,restore,expected;};
    const Case cases[]={
        {"unchanged cover-disabled profile",false,false,false,false,true},
        {"legacy writer still rejected",false,true,false,false,false},
        {"unqualified input still rejected",false,false,true,false,false},
        {"old simultaneous writers still rejected",false,true,true,false,false},
        {"sealed C0 reader pair admitted",true,true,true,false,true},
        {"partial reader setup rejected",true,true,false,false,false},
        {"missing reader rejected",true,false,true,false,false},
        {"engine restore conflicts with C0",true,true,true,true,false},
        {"engine restore conflicts with ordinary profile",false,false,false,true,false},
        {"disabled cover remains data-only",true,false,false,false,true}};
    for(const auto& c:cases){
        gears_cover_pose_role::c0_exclusive=c.selected;
        const bool actual=gears_cover_pose_role::no_competing_writer(c.legacy,c.input,c.restore);
        ++checks;if(actual!=c.expected){++failed;std::printf("failed %s\n",c.name);}
    }
    gears_cover_pose_role::c0_exclusive=false;
    std::printf("CoverPoseRoleTest checks=%u failures=%u\n",checks,failed);return failed?1:0;
}
