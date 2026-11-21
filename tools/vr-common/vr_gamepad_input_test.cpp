#include "vr_gamepad_input.h"
#include <cassert>
#include <cstdio>
using namespace gears_vr;namespace pad=gears_vr_gamepad;namespace hc=gears_hand_continuity;
struct Fixture {
    ControllerInputFrame f{};hc::TrackedInput tracked{};hc::Publisher publisher{11,22};
    pad::Route route;std::int64_t now{1000000000};std::uint32_t packet{};
    Fixture(){f.session=f.space_epoch=f.profile_generation=f.input_epoch=1;f.sequence=10;f.predicted_time=1;
        f.sync_ns=f.received_ns=now;f.focused=1;f.display_mode=1;f.display_observed_ns=now-1;
        for(auto& h:f.hands){h.grip_active=h.squeeze_active=h.trigger_active=h.stick_active=1;h.grip_flags=15;}
        f.hands[0].buttons_active=controller_x|controller_y|controller_stick_click|controller_menu;
        f.hands[1].buttons_active=controller_a|controller_b|controller_stick_click;
        tracked.stream_epoch=tracked.tracking_epoch[0]=tracked.tracking_epoch[1]=1;
        assert(observe());assert(sample().buttons==0);
    }
    void advance(){now+=10000000;f.sync_ns=f.received_ns=now;++f.sequence;++f.predicted_time;}
    bool observe(bool physical=true,bool simulated=false){tracked.input=f;return route.observe(f,tracked,publisher,now,physical,simulated);}
    pad::State sample(bool game=true){return route.sample(now,game,packet);}
    void button(unsigned hand,std::uint32_t bit,unsigned control,bool down){advance();auto& h=f.hands[hand];
        if(down){h.buttons|=bit;++h.pressed[control];}else{h.buttons&=~bit;++h.released[control];}assert(observe());}
};
#if !defined(GEARS_VR_GAMEPAD_FIXTURE_ONLY)
int main(){
    {
        Fixture s;s.button(0,controller_x,control_x,true);
        const auto before=s.packet;
        assert(s.route.pending_buttons(s.now)==pad::x);
        assert(s.route.sample(s.now,true,s.packet,pad::x).buttons==0);
        assert(s.packet==before&&s.route.pending_buttons(s.now)==pad::x);
        s.button(0,controller_x,control_x,false); // Short release while vetoed.
        assert(s.route.sample(s.now,true,s.packet,pad::x).buttons==0);
        assert(s.route.pending_buttons(s.now)==pad::x);
        assert(s.sample().buttons==pad::x&&s.packet!=before);
        assert(s.sample().buttons==0&&s.route.pending_buttons(s.now)==0);
    }
    {
        Fixture s;s.button(1,controller_b,control_b,true);
        assert(s.route.sample(s.now,true,s.packet,pad::b).buttons==0);
        s.now+=100000001;
        assert(s.route.pending_buttons(s.now)==0&&s.sample().buttons==0);
    }
    {
        Fixture s;s.button(0,controller_menu,control_menu,true);
        s.button(1,controller_a,control_a,true);
        assert(s.route.sample(s.now,true,s.packet,pad::equipment_restore_buttons).buttons==0);
        assert((s.route.pending_buttons(s.now)&(pad::a|pad::start))==(pad::a|pad::start));
        s.button(0,controller_menu,control_menu,false);
        s.button(1,controller_a,control_a,false);
        assert(s.route.sample(s.now,true,s.packet,pad::equipment_restore_buttons).buttons==0);
        // A short pause tap is retained only within its original deadline;
        // after the owner restores/unfences, the original consumer sees it
        // once rather than pausing first and stranding the restoration tick.
        assert(s.sample().buttons==(pad::a|pad::start));
        assert(s.sample().buttons==0);
    }
    {
        Fixture s;s.button(0,controller_menu,control_menu,true);
        assert(s.route.sample(s.now,true,s.packet,pad::equipment_restore_buttons).buttons==0);
        s.now+=100000001;
        assert(s.route.pending_buttons(s.now)==0&&s.sample().buttons==0);
    }
    {
        Fixture s;s.button(0,controller_stick_click,control_stick,true);
        s.button(1,controller_stick_click,control_stick,true);
        assert(s.route.sample(s.now,true,s.packet,pad::equipment_restore_buttons).buttons==0);
        s.button(0,controller_stick_click,control_stick,false);
        s.button(1,controller_stick_click,control_stick,false);
        assert(s.route.sample(s.now,true,s.packet,pad::equipment_restore_buttons).buttons==0);
        assert(s.sample().buttons==pad::start);
        assert(s.sample().buttons==0);
    }
    for(unsigned mode=0;mode<8;++mode){
        Fixture s;s.button(1,controller_b,control_b,true);assert(s.sample().buttons==pad::b);
        s.advance();s.f.display_mode=mode;s.f.display_observed_ns=mode?s.now:0;
        assert(s.observe());assert(s.sample().buttons==pad::b);
        s.button(1,controller_b,control_b,false);assert(s.sample().buttons==0);
        s.advance();s.f.display_mode=1;s.f.display_observed_ns=s.now;
        ++s.f.hands[1].pressed[control_b];++s.f.hands[1].released[control_b];
        assert(s.observe());assert(s.sample().buttons==pad::b);assert(s.sample().buttons==0);
    }
    for(auto item:{std::array<unsigned,4>{1,controller_a,control_a,pad::a},{1,controller_b,control_b,pad::b},
         {0,controller_x,control_x,pad::x},{0,controller_y,control_y,pad::y},{0,controller_menu,control_menu,pad::start}}){
        Fixture s;s.button(item[0],item[1],item[2],true);assert(s.sample(false).buttons==item[3]);
        const auto p=s.packet;assert(s.sample().buttons==item[3]&&s.packet==p);
        s.button(item[0],item[1],item[2],false);assert(s.sample().buttons==0&&s.packet!=p);
    }
    {Fixture s;s.advance();auto& r=s.f.hands[1];++r.pressed[control_a];++r.released[control_a];
        assert(s.observe());assert(s.sample(false).buttons==pad::a);assert(s.sample(false).buttons==pad::a);
        assert(s.sample().buttons==pad::a);assert(s.sample().buttons==0);}
    {Fixture s;s.advance();auto& r=s.f.hands[1];++r.pressed[control_b];++r.released[control_b];
        assert(s.observe());s.now+=100000001;assert(s.sample().buttons==0);}
    {Fixture s;s.button(1,controller_a,control_a,true);assert(s.sample().buttons==pad::a);
        s.f.focused=0;assert(!s.observe());assert(s.sample().buttons==0);
        s.advance();s.f.focused=1;++s.f.input_epoch;assert(s.observe());assert(s.sample().buttons==0);
        s.button(1,controller_a,control_a,false);s.button(1,controller_a,control_a,true);assert(s.sample().buttons==pad::a);}
    {Fixture s;s.advance();s.f.hands[0].stick_x=.5f;s.f.hands[0].stick_y=-1;assert(s.observe());
        assert(!s.route.axes_owned()&&s.sample().lx==0);s.advance();s.f.display_mode=3;s.f.display_observed_ns=s.now;
        assert(s.observe());assert(s.sample().lx==0); // Held input across a menu boundary must rearm.
        s.advance();s.f.hands[0].stick_x=s.f.hands[0].stick_y=0;assert(s.observe());
        s.advance();s.f.hands[0].stick_x=.5f;s.f.hands[0].stick_y=-1;assert(s.observe());
        const auto p=s.sample();assert(s.route.axes_owned()&&p.lx==0&&p.ly==0&&p.buttons==pad::down);}
    for(unsigned mode:{2u,3u,4u}){Fixture s;s.advance();s.f.display_mode=mode;s.f.display_observed_ns=s.now;assert(s.observe());
        s.advance();s.f.hands[0].stick_y=-.8f;assert(s.observe());assert(s.sample().buttons==pad::down);
        s.advance();s.f.hands[0].stick_y=-.4f;assert(s.observe());assert(s.sample().buttons==pad::down);
        s.advance();s.f.hands[0].stick_y=-.3f;assert(s.observe());assert(s.sample().buttons==0);
        s.advance();s.f.hands[0].stick_y=0;s.f.hands[0].stick_x=.8f;assert(s.observe());assert(s.sample().buttons==pad::right);
        s.advance();s.f.hands[0].stick_x=0;assert(s.observe());assert(s.sample().buttons==0);}
    {Fixture s;s.advance();s.f.display_mode=4;s.f.display_observed_ns=s.now;assert(s.observe());
        s.advance();s.f.hands[0].stick_y=-.8f;assert(s.observe());assert(s.sample().buttons==pad::down);
        s.now+=1500000001;s.advance();assert(s.observe());assert(s.sample().buttons==0);
        s.advance();s.f.display_observed_ns=s.now;assert(s.observe());assert(s.sample().buttons==0);
        s.advance();s.f.hands[0].stick_y=0;assert(s.observe());
        s.advance();s.f.hands[0].stick_y=-.8f;assert(s.observe());assert(s.sample().buttons==pad::down);
        s.f.focused=0;assert(!s.observe());assert(s.sample().buttons==0);}
    {Fixture s;s.advance();s.f.hands[0].buttons=s.f.hands[1].buttons=controller_stick_click;
        assert(s.observe());assert(s.sample().buttons==pad::start);}
    for(unsigned mode=0;mode<8;++mode){Fixture s;s.advance();s.f.display_mode=mode;s.f.display_observed_ns=mode?s.now:0;assert(s.observe());
        s.advance();s.f.hands[0].squeeze=s.f.hands[1].squeeze=1;s.f.hands[0].trigger=s.f.hands[1].trigger=1;assert(s.observe());
        auto value=s.sample();assert(!value.lt&&!value.rt&&!(value.buttons&(pad::left_shoulder|pad::right_shoulder)));
        s.advance();s.f.display_mode=1;s.f.display_observed_ns=s.now;assert(s.observe());value=s.sample();
        assert(!value.lt&&!value.rt&&!(value.buttons&(pad::left_shoulder|pad::right_shoulder)));}
    {Fixture s;s.advance();s.f.hands[0].buttons_active&=~controller_menu;
        s.f.hands[0].buttons=s.f.hands[1].buttons=controller_stick_click;
        assert(s.observe());assert(s.sample().buttons==pad::start);}
    {Fixture s;assert(!s.observe(false));s.advance();s.f.source=StereoSource::MetaOperatorSimulated;
        assert(!s.observe(false,false));assert(s.observe(false,true));}
    {Fixture s;s.button(1,controller_a,control_a,true);assert(s.sample().buttons==pad::a);
        s.advance();++s.publisher.start;assert(s.observe());assert(s.sample().buttons==0);}
    {Fixture s;s.button(1,controller_a,control_a,true);assert(s.sample().buttons==pad::a);
        s.now+=100000001;assert(s.sample().buttons==0);assert(!s.observe());}
    {Fixture s;s.button(1,controller_stick_click,control_stick,true);
        s.route.consume_buttons(pad::right_thumb);
        assert(!(s.route.pending_buttons(s.now)&pad::right_thumb)&&s.sample().buttons==0);
        s.advance();assert(s.observe());assert(s.sample().buttons==0);
        s.button(1,controller_stick_click,control_stick,false);assert(s.sample().buttons==0);
        s.button(1,controller_stick_click,control_stick,true);assert(s.sample().buttons==pad::right_thumb);}
    {Fixture s;s.button(1,controller_stick_click,control_stick,true);
        s.button(1,controller_stick_click,control_stick,false);
        assert(s.route.pending_buttons(s.now)&pad::right_thumb);
        s.route.consume_buttons(pad::right_thumb);
        assert(!(s.route.pending_buttons(s.now)&pad::right_thumb)&&s.sample().buttons==0);}
    {Fixture s;s.button(1,controller_stick_click,control_stick,true);
        s.route.consume_buttons(pad::right_thumb);
        s.button(0,controller_stick_click,control_stick,true);assert(s.sample().buttons==pad::start);
        s.button(0,controller_stick_click,control_stick,false);
        // The R3 owner may already be gone. Breaking the pause chord must
        // still not redeliver that held press to the original weapon.
        assert(s.sample().buttons==0);
        s.button(1,controller_stick_click,control_stick,false);assert(s.sample().buttons==0);
        s.button(1,controller_stick_click,control_stick,true);assert(s.sample().buttons==pad::right_thumb);}
    {Fixture s;s.button(1,controller_a,control_a,true);s.button(1,controller_stick_click,control_stick,true);
        s.route.consume_buttons(pad::right_thumb);assert(s.sample().buttons==pad::a);}
    std::puts("VR original-gamepad mapping and explicit consumption: existing routing, no delayed/held R3 replay, pause chord preservation and real release/repress passed; no game/XR acceptance");
}
#endif
