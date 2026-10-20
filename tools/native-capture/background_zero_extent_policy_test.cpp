#include "background_zero_extent_policy.h"
using namespace gears_background_zero_extent;
constexpr Evidence candidate(){
    Evidence e{};
    e.trial_enabled=e.outermost=e.owned_hwnd=e.windowed=true;
    e.background_verified=e.iconic=e.physical_verified=true;
    e.physical_width=2534;e.physical_height=1009;
    return e;
}
constexpr bool check(){
    auto e=candidate();auto d=resolve(0,0,e);
    if(!d.substituted||d.width!=2534||d.height!=1009)return false;
    d=resolve(1600,0,e);if(d.width!=1600||d.height!=1009)return false;
    d=resolve(0,900,e);if(d.width!=2534||d.height!=900)return false;
    if(resolve(1600,900,e).substituted)return false;
    // Explicit tiny requests retain their original semantics.
    if(resolve(8,8,e).substituted)return false;
    e.iconic=false;
    if(resolve(0,0,e).substituted)return false;
    e.client_valid=true;e.client_width=2534;e.client_height=1009;
    if(resolve(0,0,e).substituted)return false;
    e.client_width=e.client_height=0;
    if(!resolve(0,0,e).substituted)return false;
    e.client_width=e.client_height=8;
    if(!resolve(0,0,e).substituted)return false;
    e.client_width=9;if(resolve(0,0,e).substituted)return false;
    for(unsigned i=0;i<9;++i){
        e=candidate();
        switch(i){
        case 0:e.trial_enabled=false;break;
        case 1:e.outermost=false;break;
        case 2:e.owned_hwnd=false;break;
        case 3:e.windowed=false;break;
        case 4:e.background_verified=false;break;
        case 5:e.physical_verified=false;break;
        case 6:e.physical_width=8;break;
        case 7:e.physical_height=8;break;
        case 8:e.physical_width=0;break;
        }
        if(resolve(0,0,e).substituted)return false;
    }
    return true;
}
static_assert(check(),"Background zero-axis extent policy contract");
int main(){return check()?0:1;}
