#include "weapon_hammerburst_magazine.h"
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Assertions must remain enabled
#endif
using namespace gears_weapon;
FeedSample sample(double depth,unsigned sequence=1){
    return {1,2,3,4,sequence,1000000000+std::int64_t(sequence)*20000000,
        {{0,0,0},{0,0,0,1}},{{0,0,depth},{0,0,0,1}},true,true};
}
int main(){
    FeedPortMount m{};HammerburstMagazineExtraction e;
    assert(!e.complete());assert(e.observe(sample(0),m));
    assert(!e.complete());assert(e.observe(sample(.025,2),m));
    assert(!e.complete());assert(e.observe(sample(.05,3),m));assert(e.complete());
    for(unsigned failure=0;failure<10;++failure){
        e.reset();assert(e.observe(sample(0),m));auto s=sample(.05,2);
        switch(failure){
        case 0:++s.weapon;break;case 1:++s.item;break;case 2:++s.world;break;
        case 3:++s.space_epoch;break;case 4:s.sequence=1;break;
        case 5:s.monotonic_ns+=101000000;break;
        case 6:s.tracked=false;break;case 7:s.item_held=false;break;
        case 8:s.left_grip.position.x=.1;break;
        case 9:s.left_grip.position.z=.2;break;
        }
        assert(!e.observe(s,m));assert(!e.complete());
        assert(!e.observe(sample(.05,3),m)); // No cold outside-start proof.
    }
    e.reset();assert(!e.observe(sample(.05),m));
    assert(e.observe(sample(0,2),m));assert(e.observe(sample(.02,3),m));
    assert(e.observe(sample(0,4),m));assert(!e.complete());
    { // The witness follows the actual weapon basis, not world +Z.
      FeedPortMount rotated=m;rotated.port_in_weapon.position={.1,.2,.3};
      const double h=std::sqrt(.5);
      const gears_vr::Pose gun{{2,3,4},{0,h,0,h}};
      const auto point=[&](double depth,unsigned sequence){
          auto s=sample(depth,sequence);s.weapon_pose=gun;
          const auto local=sum(rotated.port_in_weapon.position,Vector{0,0,depth});
          s.left_grip={sum(gun.position,rotate(gun.orientation,local)),gun.orientation};return s;
      };
      e.reset();assert(e.observe(point(0,1),rotated));
      assert(e.observe(point(.025,2),rotated));assert(e.observe(point(.05,3),rotated));assert(e.complete());
    }
    std::puts("HB magazine: continuous extraction and 10 invalidation cases passed");
}
