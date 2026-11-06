#include "grenade_throw.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

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
            assert(grenade_world_launch(*result,reference,base,100,game,{50,-20,0}));
            assert(near(game.position.x,111)&&near(game.position.y,64)&&
                   near(game.position.z,37));
            assert(near(game.velocity.x,150)&&near(game.velocity.y,380)&&
                   near(game.velocity.z,200));
            assert(near(game.impulse.y,1140)&&near(game.gravity.z,-981));
            const double direction_length=std::sqrt(21.);
            assert(near(game.direction.x,1/direction_length)&&near(game.direction.y,4/direction_length)&&
                   near(game.direction.z,2/direction_length)); // Pawn base velocity is added only to velocity.
            assert(!tracker.armed());
        }
    }
    assert(tracker.begin(77,99,1,4));
    for(unsigned i=13;i<=21;++i){
        const auto now=frame(i,{0,1.5,0});
        const auto result=tracker.advance(hands,stereo,77,99,4,now,i==21);
        if(i==21)assert(result&&near(length(result->velocity_mps),0));
        else assert(!result);
    }
    assert(!tracker.armed()); // A deliberate stationary release is a drop.
    {
        GrenadeThrowSolution drop{};drop.weapon=77;drop.world=99;drop.session=8;drop.space_epoch=2;
        drop.reference_generation=4;drop.predicted_time=10000000000ll;drop.gravity_mps2=9.81;
        const double h=std::sqrt(.5);drop.release_orientation={0,h,0,h};
        GameGrenadeLaunch launch{};
        assert(grenade_world_launch(drop,{{},{0,0,0,1}},{{},{0,0,0}},100,launch,{50,0,0}));
        assert(near(launch.direction.x,0)&&near(launch.direction.y,-1)&&near(launch.direction.z,0));
        assert(near(launch.velocity.x,50)&&near(launch.velocity.y,0));
        drop.release_orientation={0,0,0,0};
        assert(!grenade_world_launch(drop,{{},{0,0,0,1}},{{},{0,0,0}},100,launch));
    }
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
    assert(tracker.begin(77,99,1,10,{.2,0,0}));
    for(unsigned i=33;i<=44;++i){
        const double angle=.1*double(i-33);
        hands.hands[1].grip.orientation={0,0,std::sin(angle/2),std::cos(angle/2)};
        now=frame(i,{0,1.5,0});
        const auto result=tracker.advance(hands,stereo,77,99,10,now,i==44);
        if(i==44){
            assert(result&&result->velocity_mps.y>1.0);
            assert(result->release_position.y>1.65);
        }else assert(!result);
    }
    assert(!tracker.armed()); // Wrist motion moves the grenade even with a fixed grip origin.
    {
        // One slow GT poll gets genuine90/100Hz samples, not eight invented
        // Stereo Frames. Release is sample8; sample9 is already moving away.
        gears_hand_continuity::MotionHistory history{};history.count=8;
        for(unsigned i=0;i<8;++i){const auto sequence=i+2;now=frame(sequence,{.02*double(sequence-1),1.5,0});
            hands.hands[1].grip.orientation={0,0,0,1};
            auto& sample=history.frames[i];sample.input=hands;sample.stream_epoch=11;
            sample.tracking_epoch[0]=21;sample.tracking_epoch[1]=22;
        }
        history.frames[7].input.hands[1].grip.position.x=.30;hands=history.frames[7].input;
        const auto original=history;
        assert(tracker.begin(77,99,1,11));
        const auto release=tracker.advance_history(history,hands,stereo,77,99,11,now,1,8);
        assert(release&&near(release->velocity_mps.x,2)&&near(release->release_position.x,.14));
        assert(release->predicted_time==history.frames[6].input.predicted_time&&!tracker.armed());
        assert(!std::memcmp(&original,&history,sizeof history));
        for(unsigned bad=0;bad<10;++bad){auto invalid=history;
            switch(bad){case 0:invalid.reserved=1;break;case 1:invalid.count=9;break;
            case 2:++invalid.frames[3].stream_epoch;break;case 3:++invalid.frames[3].tracking_epoch[1];break;
            case 4:++invalid.frames[3].input.input_epoch;break;case 5:invalid.frames[3].input.focused=0;break;
            case 6:invalid.frames[3].input.hands[1].grip_active=0;break;case 7:++invalid.frames[3].input.sequence;break;
            case 8:invalid.frames[7].input.sync_ns++;break;case 9:invalid.frames[3].input.predicted_time=invalid.frames[2].input.predicted_time;break;}
            assert(tracker.begin(77,99,1,11));
            assert(!tracker.advance_history(invalid,hands,stereo,77,99,11,now,1,8)&&!tracker.armed());
        }
        assert(tracker.begin(77,99,1,11));
        assert(!tracker.advance_history(history,hands,stereo,77,99,11,now,1,1)&&!tracker.armed());
        assert(tracker.begin(77,99,1,11));
        assert(!tracker.advance_history(history,hands,stereo,77,99,11,now,1,7)&&!tracker.armed()); // <55ms, not enough motion data.
        assert(tracker.begin(77,99,1,11));
        assert(!tracker.advance_history(history,hands,stereo,77,99,11,now,1));
        assert(!tracker.advance_history(history,hands,stereo,77,99,11,now,1,8)&&!tracker.armed()); // Replay cannot throw.
    }
    std::puts("grenade hand-history velocity, original-ring release position, impulse, arc and discontinuity checks passed");
}
