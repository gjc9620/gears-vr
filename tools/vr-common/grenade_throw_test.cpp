#include "grenade_throw.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace gears_vr;
using namespace gears_weapon;

static bool near(double a,double b,double tolerance=1e-5){return std::abs(a-b)<tolerance;}

int main(){
    ControllerInputFrame hands{};StereoSample stereo{};
    hands.session=stereo.session=8;
    hands.space_epoch=stereo.space_epoch=2;
    hands.focused=stereo.focused=1;
    hands.hands[1].grip_active=1;hands.hands[1].grip_flags=15;
    hands.hands[1].grip.orientation={0,0,0,1};
    GrenadeThrowTracker tracker;
    auto frame=[&](unsigned sequence,Vector position){
        hands.sequence=stereo.sequence=sequence;
        hands.predicted_time=stereo.predicted_time=10000000000LL+10000000LL*sequence;
        hands.received_ns=2000000000LL+10000000LL*sequence;
        hands.sync_ns=hands.received_ns-1000;
        hands.hands[1].grip.position=position;
        return hands.received_ns+1000;
    };
    assert(tracker.begin(77,99,1,3));
    for(unsigned i=1;i<=12;++i){
        const double t=double(i-1)*.01;
        const auto now=frame(i,{4*t,1.5+2*t,-t});
        const auto result=tracker.advance(hands,stereo,77,99,3,now,i==12);
        if(i!=12)assert(!result);
        else{
            assert(result&&near(result->velocity_mps.x,4)&&
                   near(result->velocity_mps.y,2)&&near(result->velocity_mps.z,-1));
            assert(near(result->mass_kg,3)&&near(result->impulse_kg_mps.x,12));
            assert(near(result->release_position.x,.44));
            const auto arc=grenade_ballistic_position(*result,1);
            assert(near(arc.x,result->release_position.x+4));
            assert(near(arc.y,result->release_position.y+2-4.905));
            GameGrenadeLaunch game{};
            const Pose reference{{0,1.65,0},{0,0,0,1}};
            const Viewpoint base{{100,20,30},{0,0,0}};
            assert(grenade_world_launch(*result,reference,base,100,game));
            assert(near(game.position.x,111)&&near(game.position.y,64)&&
                   near(game.position.z,37));
            assert(near(game.velocity.x,100)&&near(game.velocity.y,400)&&
                   near(game.velocity.z,200));
            assert(near(game.impulse.y,1200)&&near(game.gravity.z,-981));
            assert(!tracker.armed());
        }
    }
    assert(tracker.begin(77,99,1,4));
    for(unsigned i=13;i<=21;++i){
        const auto now=frame(i,{0,1.5,0});
        assert(!tracker.advance(hands,stereo,77,99,4,now,i==21));
    }
    assert(!tracker.armed()); // A stationary hand does not throw.
    assert(tracker.begin(77,99,1,5));
    auto now=frame(22,{0,1.5,0});
    assert(!tracker.advance(hands,stereo,77,99,5,now,false));
    now=frame(23,{.04,1.5,0});
    assert(!tracker.advance(hands,stereo,77,99,6,now,true));
    assert(!tracker.armed()); // Recenter/reference change cancels the history.
    assert(tracker.begin(77,99,1,7));
    now=frame(24,{0,1.5,0});
    assert(!tracker.advance(hands,stereo,77,99,7,now,false));
    hands.hands[1].grip_flags=0;
    now=frame(25,{.04,1.5,0});
    assert(!tracker.advance(hands,stereo,77,99,7,now,true));
    assert(!tracker.armed()); // Lost tracking cannot release a projectile.
    hands.hands[1].grip_flags=15;
    assert(tracker.begin(77,99,1,8));
    for(unsigned i=26;i<=30;++i){
        now=frame(i,{.03*double(i-26),1.5,0});
        assert(!tracker.advance(hands,stereo,77,99,8,now,false));
    }
    now=frame(31,{2,1.5,0});
    assert(!tracker.advance(hands,stereo,77,99,8,now,true));
    assert(!tracker.armed()); // A tracking teleport resets velocity history.
    assert(tracker.begin(77,99,1,9));
    now=frame(32,{0,1.5,0});
    assert(!tracker.advance(hands,stereo,77,99,9,now,true));
    assert(!tracker.armed()); // One sample is not a throw trajectory.
    std::puts("grenade hand-history velocity, 3 kg impulse, arc and discontinuity checks passed");
}
