// Actual admission can occur after the earlier log observation. These cases
// preserve every original Input guard and never generate runtime input.
#include "weapon_lancer_r2.h"
#define main weapon_hammerburst_input_clock_fixture
#include "weapon_rig_binding_test.cpp"
#undef main

int main(){
    namespace lr=gears_lancer_r2;
    using Result=gears_weapon::OriginalR2Status;
    unsigned cases{};
    for(unsigned source=0;source<2;++source){
        for(unsigned failure=0;failure<6;++failure){
            Fixture fixture(source);auto& frame=fixture.hand;
            lr::Input input;lr::Game game{};
            game.actors={frame.world,frame.controller,frame.pawn,frame.mesh,frame.weapon};
            game.known=game.lancer=game.main_hand_held=game.r2_known=true;
            unsigned calls{};
            auto execute=[&](gears_weapon::OriginalR2Action,std::uint64_t){++calls;return true;};
            CHECK(input.advance(&frame,true,game,now,execute)==Result::baseline);
            ++frame.key.frame;++frame.stereo.sequence;frame.controllers.sequence=frame.stereo.sequence;
            frame.now_ns=now+10000000;
            frame.controllers.sync_ns=now+10000000;
            frame.controllers.received_ns=now+11000000;
            frame.stereo.received_ns=now+11000000;
            auto& right=frame.controllers.hands[1];right.trigger=1;++right.pressed[gears_vr::control_trigger];
            const auto earlier_observation=now+108000000;
            auto admission=earlier_observation;
            lr::SourceStatus expected=lr::SourceStatus::inactive;
            if(failure==0){
                CHECK(wh::frame_active(frame,earlier_observation));
                CHECK(earlier_observation-frame.controllers.sync_ns==98000000);
                admission+=4000000;
                CHECK(!wh::frame_active(frame,admission));
            }
            if(failure==1)frame.flags&=~wh::gameplay_interactable;
            if(failure==2)frame.stereo.received_ns=earlier_observation-100000001;
            if(failure==3){
                frame.controllers.received_ns=earlier_observation-100000001;
                frame.controllers.sync_ns=frame.controllers.received_ns;
            }
            if(failure==4){
                frame.controllers.sync_ns=earlier_observation-100000001;
                CHECK(wh::frame_active(frame,admission));expected=lr::SourceStatus::sync_invalid;
            }
            if(failure==5){
                frame.now_ns=earlier_observation-100000001;
                CHECK(wh::frame_active(frame,admission));expected=lr::SourceStatus::stale;
            }
            input.advance(&frame,true,game,admission,execute);
            CHECK(input.source_status()==expected);
            CHECK(calls==0&&!input.owns_press());
            ++cases;
        }
    }
    std::printf("HammerburstInputClockTest: %u cases, %u checks; existing guards unchanged\n",cases,checks);
    return 0;
}
