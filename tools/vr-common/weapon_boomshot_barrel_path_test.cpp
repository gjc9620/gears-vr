#include "weapon_boomshot_barrel_path.h"
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Boomshot barrel checks require assertions
#endif
namespace b=gears_weapon::boomshot_barrel;
b::Sample sample(unsigned n,double q=0) {
    b::Sample s{};s.stamp={{1,2,3,4},5,n,6,100+n,1000000000+40000000LL*n};
    s.frame=n;s.lease=7;s.press=8;s.operation=9;s.hand_m={0,-q*b::travel_raw/100.,0};
    s.actual_progress=q;s.grip_held=s.grant_proven=s.bone_measured=s.left_tracked=s.right_tracked=s.lease_current=true;
    return s;
}
int main(){
    double q{},key{};
    assert(b::progress(b::closed_raw,q)&&q==0);
    auto open=b::closed_raw;open.y-=b::travel_raw;
    assert(b::progress(open,q)&&std::abs(q-1)<1e-12);
    open.x=.01;assert(!b::progress(open,q));
    assert(b::original_key(0,b::Direction::opening,key)&&key==66);
    assert(b::original_key(1,b::Direction::opening,key)&&key==68);
    assert(b::original_key(1,b::Direction::closing,key)&&key==72);
    assert(b::original_key(0,b::Direction::closing,key)&&key==75);
    assert(!b::original_key(1.01,b::Direction::closing,key));
    {b::Stroke stroke;auto s=sample(1);assert(stroke.begin(s,b::Direction::opening,100,s.stamp.monotonic_ns));
        s=sample(2,.5);assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::moved);
        s=sample(3,1);assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::moved);
        assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::waiting);
        s=sample(4,1);s.bone_measured=false;assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::waiting);
        s=sample(5,1);assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::complete);
        b::Completion done{};assert(stroke.take_completion(done)&&done.valid()&&done.direction()==b::Direction::opening);
        assert(done.began().frame==1&&done.began().stamp.sequence==1&&done.began().stamp.monotonic_ns==sample(1).stamp.monotonic_ns);
        assert(done.ended().frame==5&&done.ended().stamp.sequence==5&&done.ended().stamp.monotonic_ns==s.stamp.monotonic_ns);
        assert(done.began().lease==7&&done.ended().lease==7&&done.ended().press==8&&done.ended().operation==9);
        assert(!stroke.take_completion(done)&&!done.valid());}
    {b::Stroke stroke;auto s=sample(1,1);assert(stroke.begin(s,b::Direction::closing,100,s.stamp.monotonic_ns));
        s=sample(2,.5);assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::moved);
        s=sample(3,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::moved);
        s=sample(4,0);assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::complete);
        b::Completion done{};assert(stroke.take_completion(done)&&done.valid()&&done.direction()==b::Direction::closing&&done.began().actual_progress==1&&done.ended().actual_progress==0);}
    for(unsigned bad=0;bad<12;++bad){
        b::Stroke stroke;auto s=sample(1);assert(stroke.begin(s,b::Direction::opening,100,s.stamp.monotonic_ns));
        s=sample(2,.1);
        switch(bad){
            case 0:s.stamp.identity.weapon++;break;case 1:s.stamp.binding++;break;
            case 2:s.stamp.motion_epoch++;break;case 3:s.lease++;break;case 4:s.press++;break;
            case 5:s.operation++;break;case 6:s.grip_held=false;break;case 7:s.left_tracked=false;break;
            case 8:s.right_tracked=false;break;case 9:s.lease_current=false;break;
            case 10:s.hand_m.x=.019;break;case 11:s.stamp.monotonic_ns+=100000001;break;
        }
        assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::interrupted&&!stroke.active());
        b::Completion done{};assert(!stroke.take_completion(done)&&!done.valid());
    }
    {b::Stroke stroke;auto s=sample(1);s.grant_proven=false;assert(!stroke.begin(s,b::Direction::opening,100,s.stamp.monotonic_ns));}
    // Merely grabbing a mechanism already at its endpoint cannot mint travel.
    for(auto direction:{b::Direction::opening,b::Direction::closing}){
        const double endpoint=direction==b::Direction::opening?1.:0.;b::Stroke stroke;auto s=sample(1,endpoint);
        assert(stroke.begin(s,direction,100,s.stamp.monotonic_ns));s=sample(2,endpoint);
        assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::waiting);b::Completion done{};
        assert(!stroke.take_completion(done)&&!done.valid());stroke.release();assert(!stroke.take_completion(done));
    }
    {b::Stroke stroke;auto s=sample(1);assert(!stroke.begin(s,static_cast<b::Direction>(99),100,s.stamp.monotonic_ns));}
    // Releasing after an otherwise complete stroke withdraws an untaken proof.
    {b::Stroke stroke;auto s=sample(1);assert(stroke.begin(s,b::Direction::opening,100,s.stamp.monotonic_ns));
        for(unsigned n=2;n<=3;++n){s=sample(n,(n-1)*.5);assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::moved);}
        s=sample(4,1);assert(stroke.observe(s,s.stamp.monotonic_ns)==b::Result::complete);stroke.release();
        b::Completion done{};assert(!stroke.take_completion(done)&&!done.valid());}
    std::puts("Boomshot barrel: original translation, physical travel, later bone ACK, once-only original-sample completion, stationary-endpoint rejection and interruption checks passed (synthetic only)");
}
