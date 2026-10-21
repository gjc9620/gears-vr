#include "ngx_trial_launch_policy.h"
#include <cassert>
#include <cstdio>
using namespace gears_ngx_trial_launch;
int main() {
    Flags background{true,false,false,true,false,true,true,true};
    assert(mode(background)==Mode::background);
    for(unsigned bit=0;bit<4;++bit) {
        auto f=background;
        if(bit==0)f.attested=false;
        if(bit==1)f.windows_behind=false;
        if(bit==2)f.muted=false;
        if(bit==3)f.observe_only=false;
        assert(mode(f)==Mode::rejected);
    }
    Flags manual{true,true,true,true,false,false,false,false};
    assert(mode(manual)==Mode::manual_owned);
    for(unsigned bit=0;bit<7;++bit) {
        auto f=manual;
        if(bit==0)f.attested=false;
        if(bit==1)f.manual_compiled=false;
        if(bit==2)f.manual_requested=false;
        if(bit==3)f.owned_scene=false;
        if(bit==4)f.final_xr=true;
        if(bit==5)f.windows_behind=true;
        if(bit==6)f.muted=true;
        assert(mode(f)==Mode::rejected);
    }
    auto mixed=background;mixed.manual_requested=true;mixed.manual_compiled=true;
    assert(mode(mixed)==Mode::rejected);
    manual.observe_only=true;assert(mode(manual)==Mode::manual_owned);
    std::puts("NGX trial launch policy passed: unchanged background guards, explicit compiled manual owned route, no mute/behind mixing or final-XR upgrade");
}
