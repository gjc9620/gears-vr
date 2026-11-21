#include "weapon_snub_part_contact.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
    using namespace gears_weapon;
    using P=gears_vr::Pose;using V=gears_vr::Vector;
    const V socket{7,2,11};V result{};
    const P original{{12000,7000,130},{0,0,0,1}};
    unsigned cases{},stale_misses{};
    for(double angle:{0.,.5,1.5707963267948966,3.141592653589793})
        for(V shift:{V{0,0,0},V{20,-10,8},V{-35,24,-5}}){
            P gun{sum(original.position,shift),{0,0,std::sin(angle/2),std::cos(angle/2)}};
            const auto world=sum(gun.position,rotate(gun.orientation,socket));
            assert(snub_part_contact_local(gun,world,result));
            assert(length(difference(result,socket))<1e-8);
            if(angle!=0||length(shift)>0){
                // Reproduce the old mixed-frame conversion: a legal contact
                // can move outside the unchanged 6.5cm acquisition radius.
                const auto stale=rotate(gears_vr::inverse(original.orientation),difference(world,original.position));
                assert(length(difference(stale,socket))>1.);
                if(length(difference(stale,socket))>6.5)++stale_misses;
            }
            ++cases;
        }
    assert(stale_misses>0); // Old conversion rejects an actual on-part hand at 100 units/m.
    P invalid=original;invalid.orientation={0,0,0,0};
    assert(!snub_part_contact_local(invalid,socket,result));
    assert(!snub_part_contact_local(original,{std::numeric_limits<double>::quiet_NaN(),0,0},result));
    assert(length(result)==0);
    std::printf("Snub current-gun contact: %u translation/rotation cases + invalid input passed (CPU only).\n",cases);
}
