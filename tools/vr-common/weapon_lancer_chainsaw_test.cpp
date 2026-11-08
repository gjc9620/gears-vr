#include "weapon_lancer_chainsaw.h"
#include <cassert>
#include <cstdio>

namespace {
gears_weapon::LancerSawSample sample() {
    gears_weapon::LancerSawSample s{};
    s.world=1;s.pawn=2;s.weapon=3;s.session=4;s.space_epoch=5;
    s.input_epoch=6;s.world_generation=7;s.sequence=1;s.created_ms=1000;
    s.lancer_active=s.right_owns_weapon=s.right_tracked=s.left_tracked=true;
    s.game_can_melee=true;
    return s;
}
void advance(gears_weapon::LancerSawSample& s,std::uint64_t ms=80) {
    ++s.sequence;s.created_ms+=ms;
}
void pull(gears_weapon::LancerChainsawInteraction& interaction,
          gears_weapon::LancerSawSample& s) {
    using namespace gears_weapon;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::pull_began);
    for(double x:{.05,.10,.15,.19}){
        advance(s);s.left_palm_m.x=x;
        const auto decision=interaction.observe(s);
        assert(std::abs(interaction.pull_metres()-x)<1e-8);
        if(x<.19)assert(decision.event==LancerSawEvent::none);
        else {
            assert(decision.event==LancerSawEvent::request_original_hold);
            assert(decision.press_original_melee);
            assert(interaction.suppress_original_auto_target());
        }
    }
}
gears_weapon::LancerSawInputContinuity witness(gears_weapon::LancerSawSample const& s,std::uint64_t after) {
    gears_weapon::LancerSawInputContinuity w{};
    w.world=s.world;w.pawn=s.pawn;w.weapon=s.weapon;w.session=s.session;
    w.space_epoch=s.space_epoch;w.input_epoch=s.input_epoch;w.world_generation=s.world_generation;
    w.producer_stream=s.producer_stream;w.tracking_epoch[0]=s.tracking_epoch[0];w.tracking_epoch[1]=s.tracking_epoch[1];
    w.publisher_pid=12;w.publisher_start=13;w.profile=14;w.sequence=s.sequence+100+after;
    w.created_ms=s.created_ms+after;w.observed_ms=w.created_ms+20;w.presses=1;
    w.focused=w.left_tracked=w.right_tracked=w.left_grip_down=true;return w;
}
}
int main() {
    {
        // Grip release locks a partial real slider. Regrasp starts there and
        // continues the remaining physical/mechanical stroke, without reset.
        using namespace gears_weapon;
        LancerChainsawInteraction model;auto s=sample();
        s.original_handle=s.original_slide=s.local_rail=s.require_full_slide=true;
        s.starter_travel_m=.08;s.starter_actual_angle=1.5707963267948966;s.pull_axis_m={0,1,0};
        s.starter_actual_pull_m=.04;model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began&&std::abs(model.pull_metres()-.04)<1e-9);
        advance(s);assert(!model.observe(s).press_original_melee);
        advance(s);s.rail_hand_m.y=s.left_palm_m.y=.04;
        assert(!model.observe(s).press_original_melee); // still no independent mechanical progress
        advance(s);s.starter_actual_pull_m=.08;
        assert(model.observe(s).press_original_melee);
    }
    {
        // Regrasp at the locked full-forward endpoint is NOT a new start.
        // Neither waiting nor axial overtravel can manufacture part motion.
        using namespace gears_weapon;
        LancerChainsawInteraction model;auto s=sample();
        s.original_handle=s.original_slide=s.local_rail=s.require_full_slide=true;
        s.starter_travel_m=.08;s.starter_actual_angle=1.5707963267948966;s.pull_axis_m={0,1,0};
        s.starter_actual_pull_m=.08;model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began);
        for(unsigned i=0;i<4;++i){advance(s);s.rail_hand_m.y=s.left_palm_m.y=.02*i;
            assert(!model.observe(s).press_original_melee);}
        advance(s);s.rail_hand_m.y=s.left_palm_m.y=-.06;s.starter_actual_pull_m=.02;
        assert(!model.observe(s).press_original_melee);
        advance(s);s.rail_hand_m.y=s.left_palm_m.y=0;s.starter_actual_pull_m=.08;
        assert(model.observe(s).press_original_melee);
    }
    {
        using namespace gears_weapon;
        LancerChainsawInteraction model;auto s=sample();
        s.original_handle=s.original_slide=s.local_rail=s.require_full_slide=true;
        s.starter_travel_m=.08;s.starter_actual_angle=1.5707963267948966;s.pull_axis_m={0,1,0};
        model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began);
        advance(s);s.rail_hand_m.y=s.left_palm_m.y=.05;s.starter_actual_pull_m=.05;
        assert(!model.observe(s).press_original_melee); // old5cm gate is no longer enough
        advance(s);s.rail_hand_m.y=s.left_palm_m.y=.08;s.starter_actual_pull_m=.05;
        assert(!model.observe(s).press_original_melee); // hand endpoint alone is not mechanical proof
        advance(s);s.starter_actual_pull_m=.08;
        assert(model.observe(s).press_original_melee);
    }
    {
        using namespace gears_weapon;
        LancerChainsawInteraction model;auto s=sample();
        s.original_handle=s.original_slide=s.local_rail=true;
        s.starter_travel_m=.08;s.starter_actual_angle=1.5707963267948966;s.pull_axis_m={0,1,0};
        model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began);
        bool requested=false;
        for(unsigned i=1;i<=20;++i){
            advance(s);s.rail_hand_m.y=s.left_palm_m.y=.05*i;
            s.starter_actual_pull_m=std::min(.08,.05*i);
            const auto event=model.observe(s);
            requested=requested||event.press_original_melee;
            assert(event.event!=LancerSawEvent::interrupted&&model.pull_metres()<=.08);
            if(requested)s.original_hold=true;
        }
        assert(requested&&model.phase()==LancerSawPhase::running);
        // A radial departure is different from the full metre of axial
        // overtravel above. Actual original input ownership must be released.
        advance(s);s.rail_hand_m.x=s.left_palm_m.x=.181;
        const auto departed=model.observe(s);
        assert(departed.event==LancerSawEvent::interrupted&&departed.release_original_melee);
    }
    {
        using namespace gears_weapon;
        LancerChainsawInteraction m;auto s=sample();
        s.original_handle=s.original_slide=s.local_rail=true;s.starter_travel_m=.08;
        s.pull_axis_m={0,1,0};m.observe(s);advance(s);s.left_grip_down=true;
        assert(m.observe(s).event==LancerSawEvent::pull_began);
        // A single natural forward push begins DURING cosmetic deployment.
        // Keep its physical origin instead of demanding another push later.
        advance(s);s.rail_hand_m=s.left_palm_m={.06,.07,0};m.observe(s);
        advance(s);s.starter_actual_angle=1.5707963267948966;m.observe(s);
        advance(s);assert(!m.observe(s).press_original_melee); // part not moved
        advance(s);s.starter_actual_pull_m=.07;
        assert(m.observe(s).press_original_melee);
        advance(s);s.original_hold=true;assert(m.observe(s).event==LancerSawEvent::hold_started);
        // Curved arm movement and12cm real overshoot do not release C.
        for(unsigned i=1;i<=6;++i){advance(s);s.rail_hand_m=s.left_palm_m={.06+.01*i,.07+.02*i,0};
            s.starter_actual_pull_m=.08;assert(m.observe(s).event!=LancerSawEvent::interrupted);}
        advance(s);s.rail_hand_m=s.left_palm_m={.12,-.06,0};s.starter_actual_pull_m=0;
        assert(m.observe(s).release_original_melee);
        s.original_hold=false;advance(s);s.left_grip_down=false;
        assert(m.observe(s).event==LancerSawEvent::interrupted);
    }
    using namespace gears_weapon;
    {
        // Owner's final control: forward on, backward off, repeat while the
        // SAME actual grip remains held. There is no second fabricated edge.
        LancerChainsawInteraction model;auto s=sample();s.original_handle=s.original_slide=s.local_rail=true;
        s.starter_travel_m=.08;s.starter_actual_angle=1.5707963267948966;s.pull_axis_m={0,1,0};
        model.observe(s);advance(s);s.left_grip_down=true;model.observe(s);
        unsigned starts{},stops{};
        for(unsigned cycle=0;cycle<2;++cycle){
            for(unsigned i=1;i<=4;++i){advance(s);s.rail_hand_m.y=s.left_palm_m.y=.02*i;s.starter_actual_pull_m=.02*i;
                starts+=model.observe(s).press_original_melee?1u:0u;}
            advance(s);s.original_hold=true;assert(model.observe(s).event==LancerSawEvent::hold_started);
            for(unsigned i=0;i<4;++i){advance(s);s.rail_hand_m.y=s.left_palm_m.y=.06-.02*i;s.starter_actual_pull_m=.06-.02*i;
                const auto d=model.observe(s);stops+=d.release_original_melee?1u:0u;}
            s.original_hold=false;assert(model.phase()==LancerSawPhase::rail_off&&s.left_grip_down);
            for(unsigned i=0;i<24;++i){advance(s);assert(!model.observe(s).press_original_melee);}
        }
        assert(starts==2&&stops==2);
    }
    {
        LancerChainsawInteraction model;auto s=sample();s.original_handle=s.original_slide=true;
        s.starter_travel_m=.08;s.starter_anchor_m={.08,0,0};s.pull_axis_m={0,-1,0};
        model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began&&model.phase()==LancerSawPhase::deploying);
        advance(s);s.starter_actual_angle=1.5707963267948966;s.starter_grasp_m={.08,0,0};
        assert(!model.observe(s).press_original_melee&&model.phase()==LancerSawPhase::pulling);
        // A real grasp stays attached during cosmetic unfolding. The player
        // need not chase the newly deployed part with a second contact.
        for(unsigned i=0;i<8;++i){advance(s);assert(!model.observe(s).press_original_melee);}
        bool started{};
        for(unsigned i=1;i<=5;++i){advance(s);s.left_palm_m.y=-.01*i;s.starter_actual_pull_m=.01*i;
            started=started||model.observe(s).press_original_melee;}
        assert(started); // Unfolding alone supplied neither travel nor start.
    }
    // Rearward C-rail pull: actual hand AND independently evaluated part must
    // move. No display-only movement, rotation, release or gun-only drag.
    for(unsigned mode=0;mode<6;++mode){
        LancerChainsawInteraction model;auto s=sample();s.original_handle=s.original_slide=true;
        s.starter_actual_angle=1.5707963267948966;s.starter_travel_m=.08;s.pull_axis_m={0,-1,0};
        model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began);bool started{};
        for(unsigned i=1;i<=8;++i){
            advance(s);s.left_palm_m={0,-.01*i,0};s.starter_actual_pull_m=std::max(0.,.01*(i-1));
            if(mode==1)s.starter_actual_pull_m=0;
            if(mode==2)s.left_palm_m={-.01*i,0,0};
            if(mode==3&&i>=3)s.left_grip_down=false;
            if(mode==4){s.starter_anchor_m={0,.01*i,0};s.left_palm_m={};}
            if(mode==5)s.left_palm_m={};
            const auto result=model.observe(s);started=started||result.press_original_melee;
        }
        assert(started==(mode==0));
        if(mode==0){advance(s);s.original_hold=true;s.left_grip_down=false;
            assert(model.observe(s).event==LancerSawEvent::interrupted);}
    }
    // Original hinge: physical arc and actual evaluated bone must agree.
    // A display command alone, a release, or a straight pull cannot start B.
    for(unsigned mode=0;mode<4;++mode){
        LancerChainsawInteraction model;auto s=sample();s.original_handle=true;
        s.starter_anchor_m={0,.08,0};s.left_palm_m=s.starter_anchor_m;s.pull_axis_m={0,0,1};
        model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began);
        bool started{};
        for(unsigned i=1;i<=14;++i){
            const double a=i*.1;advance(s);s.left_palm_m={-.08*std::sin(a),.08*std::cos(a),0};
            s.starter_actual_angle=mode==1?0:(i-1)*.1;
            if(mode==2&&i==6)s.left_grip_down=false;
            if(mode==3)s.left_palm_m={-.01*i,.08,0};
            const auto result=model.observe(s);started=started||result.press_original_melee;
            if(mode==0&&i==14)assert(result.event==LancerSawEvent::request_original_hold);
        }
        assert(started==(mode==0));
    }
    // Reproduce42800's101ms consumer interval. Only a real, unchanged XR
    // producer companion may distinguish it from tracking loss. No witness,
    // changed source/hand epoch, or a long consumer stall must still cancel.
    for(unsigned mode=0;mode<6;++mode){
        LancerChainsawInteraction model;auto s=sample();
        if(mode){s.producer_stream=9;s.tracking_epoch[0]=10;s.tracking_epoch[1]=11;}
        model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began);
        advance(s,mode==5?251:101);
        if(mode==2)++s.producer_stream;
        if(mode==3)++s.tracking_epoch[0];
        if(mode==4)s.tracking_epoch[1]=0;
        assert(model.observe(s).event==(mode==1?LancerSawEvent::none:LancerSawEvent::interrupted));
    }
    // Reproduce59276: actual geometry at13cm, missing two render samples,
    // then a NEW actual19cm sample311ms later. Only a chain of independently
    // current same-publisher/tracking/grip inputs may preserve the old grasp.
    for(unsigned mode=0;mode<24;++mode){
        LancerChainsawInteraction model;auto s=sample();
        s.producer_stream=9;s.tracking_epoch[0]=10;s.tracking_epoch[1]=11;
        model.observe(s);advance(s);s.left_grip_down=true;
        assert(model.observe(s).event==LancerSawEvent::pull_began);
        advance(s,100);s.left_palm_m.x=.13;assert(model.observe(s).event==LancerSawEvent::none);
        auto w=witness(s,30);assert(model.observe_input(w).event==LancerSawEvent::none);
        assert(model.pull_metres()==.13&&model.bridged_samples()==0); // no geometry progress
        w=witness(s,130);
        switch(mode){
        case 2:w.publisher_pid++;break;case 3:w.publisher_start++;break;case 4:w.profile++;break;
        case 5:w.producer_stream++;break;case 6:w.tracking_epoch[0]++;break;case 7:w.tracking_epoch[1]++;break;
        case 8:w.input_epoch++;break;case 9:w.space_epoch++;break;case 10:w.session++;break;
        case 11:w.world++;break;case 12:w.pawn++;break;case 13:w.weapon++;break;
        case 14:w.world_generation++;break;case 15:w.left_grip_down=false;break;
        case 16:w.presses=2;w.releases=1;break; // release/regrip hidden between geometry samples
        case 17:w.observed_ms=w.created_ms+101;break;
        case 18:w.sequence=1;break;case 19:w.created_ms=s.created_ms+29;w.observed_ms=w.created_ms+20;break;
        case 20:w.focused=false;break;case 21:w.left_tracked=false;break;case 22:w.right_tracked=false;break;
        case 23:w.created_ms=s.created_ms+281;w.observed_ms=w.created_ms+20;break; // input gap251ms
        default:break;
        }
        assert(model.observe_input(w).event==(mode<2?LancerSawEvent::none:LancerSawEvent::interrupted));
        if(mode>=2){assert(model.phase()==LancerSawPhase::idle&&!model.suppress_original_auto_target());continue;}
        if(mode==0){w=witness(s,230);assert(model.observe_input(w).event==LancerSawEvent::none);}
        else {w=witness(s,330);assert(model.observe_input(w).event==LancerSawEvent::none);}
        assert(model.pull_metres()==.13); // witness never starts a motor or moves a handle
        advance(s,311);s.left_palm_m.x=.19;
        auto decision=model.observe(s);
        assert(decision.event==LancerSawEvent::request_original_hold&&decision.press_original_melee);
        assert(model.bridged_samples()==1);
        // It is still the original once-only handoff, not a new grasp or B clock.
        advance(s,40);assert(!model.observe(s).press_original_melee);
    }
    {
        LancerChainsawInteraction model;auto s=sample();s.producer_stream=9;s.tracking_epoch[0]=10;s.tracking_epoch[1]=11;
        auto w=witness(s,1);assert(model.observe_input(w).event==LancerSawEvent::none&&model.phase()==LancerSawPhase::idle);
        model.observe(s);advance(s);s.left_grip_down=true;model.observe(s);
        // A single current input after the long gap cannot retroactively fill it.
        w=witness(s,311);assert(model.observe_input(w).event==LancerSawEvent::interrupted);
    }
    for(unsigned mode=0;mode<4;++mode){
        LancerChainsawInteraction model;auto s=sample();s.producer_stream=9;s.tracking_epoch[0]=10;s.tracking_epoch[1]=11;
        model.observe(s);advance(s);s.left_grip_down=true;model.observe(s);
        auto w=witness(s,30);model.observe_input(w);
        if(mode==0){w.observed_ms=w.created_ms+101;assert(model.observe_input(w).event==LancerSawEvent::interrupted);}
        if(mode==1){w.created_ms++;assert(model.observe_input(w).event==LancerSawEvent::interrupted);}
        if(mode==2){
            for(unsigned i=0;i<5;++i)assert(model.observe_input(w).event==LancerSawEvent::none);
            advance(s,311);s.left_palm_m.x=.19;
            assert(model.observe(s).event==LancerSawEvent::interrupted); // duplicate input did not renew a clock
        }
        if(mode==3){
            for(auto after:{230u,430u,630u,830u,1030u,1230u,1430u}){
                w=witness(s,after);assert(model.observe_input(w).event==LancerSawEvent::none);
                assert(model.phase()==LancerSawPhase::pulling&&!model.suppress_original_auto_target());
            }
            w=witness(s,1530);assert(model.observe_input(w).event==LancerSawEvent::interrupted); // original1500ms pull bound
        }
    }
    //13108 actual observer gap: old input746ms -> new1079ms, latest age47ms,
    // same XR stream/tracking/edges. Only an independently authenticated
    // existing producer hold can bridge that GAME-consumer gap. The witness
    // alone never moves the hand/part or starts the original motor.
    for(unsigned mode=0;mode<14;++mode){
        LancerChainsawInteraction model;auto s=sample();
        s.producer_stream=9;s.tracking_epoch[0]=10;s.tracking_epoch[1]=11;
        s.original_handle=s.original_slide=s.local_rail=true;s.starter_actual_angle=1.5707963267948966;
        s.starter_travel_m=.08;s.pull_axis_m={0,1,0};
        model.observe(s);advance(s);s.left_grip_down=true;model.observe(s);
        auto w=witness(s,30);assert(model.observe_input(w).event==LancerSawEvent::none);
        w=witness(s,363);w.observed_ms=w.created_ms+47;
        switch(mode){
        case 2:++w.producer_stream;break;case 3:++w.tracking_epoch[0];break;
        case 4:++w.tracking_epoch[1];break;case 5:++w.publisher_pid;break;
        case 6:++w.publisher_start;break;case 7:++w.profile;break;
        case 8:++w.weapon;break;case 9:w.presses=2;w.releases=1;break;
        case 10:w.left_grip_down=false;break;case 11:w.focused=false;break;
        case 12:w.observed_ms=w.created_ms+101;break;case 13:w.right_tracked=false;break;
        default:break;
        }
        const auto d=model.observe_input(w,mode!=0);
        assert(d.event==(mode==1?LancerSawEvent::none:LancerSawEvent::interrupted));
        assert(!d.press_original_melee&&model.pull_metres()==0.);
        if(mode!=1)continue;
        assert(model.phase()==LancerSawPhase::pulling&&model.bridged_input_polls()==1);
        advance(s,363);s.left_palm_m.y=.08;s.rail_hand_m.y=.08;
        // Real hand motion without the actual part still cannot start.
        assert(!model.observe(s).press_original_melee);
        advance(s);s.starter_actual_pull_m=.08;
        assert(model.observe(s).press_original_melee);
    }
    std::puts("Lancer chainsaw input-only continuity:28 prior and14 certified consumer-gap/epoch/release/freshness cases passed");
    LancerSawContact contact{};
    contact.victim=44;
    contact.original={true,true,true,true,true,true,true,true,true,true,true};
    std::array<float,16> gun_matrix{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    assert(!lancer_chain_outline_from_mesh(gun_matrix,false,contact.blade));
    assert(!lancer_saw_blade_contact(contact)); // no authenticated source mesh
    assert(lancer_chain_outline_from_mesh(gun_matrix,true,contact.blade));
    assert(contact.blade.world_raw[0].y==28);
    contact.target_a_raw={0,60,-20};contact.target_b_raw={0,60,20};
    contact.target_radius_raw=2;contact.blade_radius_raw=1.5;
    assert(lancer_saw_blade_contact(contact)); // chain edge reaches target
    contact.target_a_raw={0,50,1};contact.target_b_raw={0,50,3};
    assert(!lancer_saw_blade_contact(contact)); // housing centre is not the edge
    contact.target_a_raw={0,60,-20};contact.target_b_raw={0,60,20};
    contact.target_a_raw.x=30;contact.target_b_raw.x=30;
    assert(!lancer_saw_blade_contact(contact)); // view-facing is not contact
    contact.target_a_raw.x=0;contact.target_b_raw.x=0;
    contact.original.blade_path_unobstructed=false;
    assert(lancer_saw_blade_geometry(contact)); // geometric contact alone grants no attack
    assert(!lancer_saw_blade_contact(contact)); // wall blocks blade
    contact.original.blade_path_unobstructed=true;
    contact.original.weapon_target_valid=false;
    assert(!lancer_saw_blade_contact(contact)); // original rules still apply
    contact.original.weapon_target_valid=true;
    contact.original.target_controller_valid=false;
    assert(!lancer_saw_blade_contact(contact)); // controllerless dummy is ineligible
    contact.original.target_controller_valid=true;
    Vector closest_a{},closest_b{};
    assert(std::abs(closest_segment_points({0,0,0},{1,0,0},{.5,-1,0},{.5,1,0},closest_a,closest_b))<1e-10);
    assert(std::abs(closest_a.x-.5)<1e-10&&std::abs(closest_b.x-.5)<1e-10);
    assert(std::abs(closest_segment_points({0,0,0},{0,0,0},{1,0,0},{1,0,0},closest_a,closest_b)-1)<1e-10);
    auto invalid=gun_matrix;invalid[0]=-1;
    assert(!lancer_chain_outline_from_mesh(invalid,true,contact.blade));
    invalid=gun_matrix;invalid[4]=.5f;
    assert(!lancer_chain_outline_from_mesh(invalid,true,contact.blade));
    invalid=gun_matrix;invalid[12]=100;
    LancerBladeOutline moved{};
    assert(lancer_chain_outline_from_mesh(invalid,true,moved));
    assert(moved.world_raw[0].x==100&&moved.world_raw[0].y==28);
    std::array<float,16> turned{0,1,0,0, -1,0,0,0, 0,0,1,0,
                                100,200,300,1};
    assert(lancer_chain_outline_from_mesh(turned,true,moved));
    assert(moved.world_raw[0].x==72&&moved.world_raw[0].y==200&&
           moved.world_raw[0].z==305);

    LancerChainsawInteraction interaction;
    auto s=sample();
    pull(interaction,s);
    advance(s);s.original_hold=true;s.original_button_held=true;
    assert(interaction.observe(s).event==LancerSawEvent::hold_started);
    assert(interaction.phase()==LancerSawPhase::running);
    s.game_can_melee=false; // original entry gate can close after B is held
    advance(s);s.contact=contact;
    s.contact.target_a_raw.x=30;s.contact.target_b_raw.x=30;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.contact.target_a_raw.x=0;s.contact.target_b_raw.x=0;
    auto d=interaction.observe(s);
    assert(d.event==LancerSawEvent::request_original_acquire&&d.victim==44);
    assert(interaction.suppress_original_auto_target());
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.original_hold=false;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.original_execution=true;s.original_victim=44;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::animation_handoff);
    assert(!d.release_original_melee&&d.animation_owns_hands);
    assert(!d.unexpected_original_target);
    assert(!interaction.suppress_original_auto_target());
    advance(s);++s.space_epoch;s.right_tracked=false;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.original_execution=false;s.original_hold=false;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.original_ready=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::request_original_release);
    assert(d.release_original_melee&&d.animation_owns_hands&&!d.restore_6dof);
    advance(s);
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::none&&!d.release_original_melee&&
           d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.original_button_held=false;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&!d.restore_6dof);
    advance(s);s.right_tracked=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::restored&&d.restore_6dof);
    assert(interaction.phase()==LancerSawPhase::idle);

    // If the original attack has ended after the gun was stowed, clear the
    // animation lock without restoring a gun that the player no longer owns.
    interaction.reset();s=sample();pull(interaction,s);
    advance(s);s.original_button_held=true;s.original_execution=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::animation_handoff);
    advance(s);s.original_execution=false;s.original_ready=true;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::request_original_release);
    advance(s);s.original_button_held=false;s.right_owns_weapon=false;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::interrupted&&!d.restore_6dof&&
           !d.animation_owns_hands&&interaction.phase()==LancerSawPhase::idle);

    // A grip already held on the first frame is a baseline, not a fresh tug.
    s=sample();s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.left_grip_down=false;interaction.observe(s);
    advance(s);s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::pull_began);
    advance(s);s.left_palm_m.x=.2;s.starter_anchor_m.x=-.11;
    assert(interaction.observe(s).event==LancerSawEvent::interrupted);

    interaction.reset();s=sample();interaction.observe(s);
    advance(s);s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::pull_began);
    advance(s);s.left_palm_m.x=.19;
    assert(interaction.observe(s).event==LancerSawEvent::interrupted);

    // Moving only the gun towards a stationary hand cannot pull its cord.
    interaction.reset();s=sample();interaction.observe(s);
    advance(s);s.left_grip_down=true;
    assert(interaction.observe(s).event==LancerSawEvent::pull_began);
    advance(s);s.starter_anchor_m.x=-.08;
    assert(interaction.observe(s).event==LancerSawEvent::none);
    advance(s);s.left_palm_m.y=.10;
    assert(interaction.observe(s).event==LancerSawEvent::interrupted);

    // A recenter or tracking gap drops ownership and releases the original B.
    interaction.reset();s=sample();pull(interaction,s);
    advance(s);++s.space_epoch;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::interrupted&&d.release_original_melee);
    assert(!interaction.suppress_original_auto_target());

    interaction.reset();s=sample();pull(interaction,s);
    advance(s,120);
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::interrupted&&d.release_original_melee);

    // If an original attack somehow starts before the verified contact, hand
    // ownership still yields to its animation and records the unsafe path.
    interaction.reset();s=sample();pull(interaction,s);
    advance(s);s.original_hold=true;interaction.observe(s);
    advance(s);s.original_execution=true;s.original_victim=99;
    d=interaction.observe(s);
    assert(d.animation_owns_hands&&d.unexpected_original_target);
    assert(!d.release_original_melee);

    // Attack entry and a lost XR frame in the same tick still belongs to the
    // original paired animation; releasing B here would cut it short.
    interaction.reset();s=sample();pull(interaction,s);
    advance(s,120);s.original_execution=true;s.original_victim=99;
    ++s.space_epoch;s.right_tracked=false;
    d=interaction.observe(s);
    assert(d.event==LancerSawEvent::animation_handoff&&
           d.animation_owns_hands&&!d.release_original_melee);
    std::puts("Lancer chainsaw VR gesture, contact and handoff passed");
}
