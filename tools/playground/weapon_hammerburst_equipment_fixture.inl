// Owned Host seam extracted from the last tested frozen equipment runtime test.
// Production Runtime and WeaponEquipmentModel are included, not copied.
#include "weapon_equipment_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef NDEBUG
#error WeaponEquipmentRuntimeTest requires active checks
#endif
namespace er=gears_equipment_runtime;
namespace wh=gears_weapon_hand;
namespace vr=gears_vr;
namespace gw=gears_weapon;
namespace {
unsigned checks{},cases{};
void check(bool value,char const* expression,int line){
    ++checks;if(!value){std::fprintf(stderr,"FAIL line %d: %s\n",line,expression);std::exit(1);}
}
#define C(x) check(bool(x),#x,__LINE__)
enum class Callback {none,next,inventory,identify,attached,forward,hold};

// Controlled synchronous PG backend seam. This exercises the actual Runtime,
// Frame validators, GVE6 event type and WeaponEquipmentModel, not game memory,
// engine callbacks, module pinning or a second implementation of orchestration.
struct Host {
    er::Runtime runtime;gw::WeaponEquipmentModel model;
    wh::Frame frame{},live{};gw::EquipmentSnapshot game{};
    std::uint64_t ms{1000},latest{};std::int64_t ns{1000000000};
    std::vector<gw::EquipmentIntentEvent> events;
    std::vector<wh::ObjectIdentity> objects;
    bool current_ok{true},inventory_ok{true},attached_ok{true},identify_ok{true},switch_ok{true},sound_ok{true},hold_ok{true};
    bool baseline_blocked{},published_hold{},hold_saw_model{},reenter{},suspend_in_forward{};
    bool integrated_stow{},stow_ok{true},unholster_ok{true},unholster_changed{};
    bool prepare_stow_changed{},prepare_stow_ok{true},invalidate_on_prepare{};
    bool index_enabled{},index_allowed{true};unsigned index_begins{};
    unsigned prepare_stow_calls{};
    unsigned stow_calls{},unholster_calls{},idle_unheld_calls{};
    unsigned next_calls{},inventory_calls{},identify_calls{},attached_calls{},switch_calls{},sound_calls{},hold_calls{},clear_calls{},reset_calls{};
    unsigned last_slot{4};er::Status reentrant_status{er::Status::idle};
    Callback invalidate_at{Callback::none},age_at{Callback::none};std::uint64_t delay_ms{};
    static wh::ObjectIdentity object(std::uint64_t address){return {address,0x8000,0x9000,unsigned(address/8),7,0,1};}
    Host(){
        frame.key={11,22,33,1,1,1,44,55};frame.flags=wh::scene_owned|wh::input_matched|wh::gameplay_interactable;
        frame.world=object(0x1000);frame.controller=object(0x2000);frame.pawn=object(0x3000);
        frame.mesh=object(0x4000);frame.weapon=object(0x5000);frame.reference_generation=1;
        frame.stereo_status=unsigned(vr::StereoStatus::accepted);
        auto& s=frame.stereo;s.session=1;s.sequence=100;s.predicted_time=123;s.head_flags=s.view_flags=15;
        s.focused=s.visible=s.should_render=1;s.head={{0,1.6,0},{0,0,0,1}};
        s.eyes[0].pose.position={-.032,1.6,0};s.eyes[1].pose.position={.032,1.6,0};
        for(auto& eye:s.eyes){eye.pose.orientation={0,0,0,1};eye.fov={-.8,.8,.8,-.8};}
        auto& c=frame.controllers;c.session=s.session;c.sequence=s.sequence;c.space_epoch=s.space_epoch;
        c.predicted_time=s.predicted_time;c.focused=1;
        for(auto& hand:c.hands){hand.grip_active=1;hand.grip_flags=15;hand.squeeze_active=1;}
        game.world=frame.world.address;game.pawn=frame.pawn.address;game.current_weapon=frame.weapon.address;
        game.weapon_in_slot={0x5000,0x6000,0x7000,0x7800};game.can_switch=true;
        game.ammo_known.fill(true);game.ammo_available.fill(true);game.throwable_count_known=true;game.throwable_count=2;
        for(auto address:game.weapon_in_slot)objects.push_back(object(address));
        publish();C(wh::frame_active(frame,ns));
    }
    void publish(){
        frame.now_ns=ns;frame.stereo.received_ns=ns-1000000;
        frame.controllers.received_ns=frame.stereo.received_ns;frame.controllers.sync_ns=frame.controllers.received_ns-1;
        live=frame;
    }
    void advance(std::uint64_t delta=10){
        ms+=delta;ns+=std::int64_t(delta)*1000000;++frame.key.frame;++frame.stereo.sequence;
        frame.controllers.sequence=frame.stereo.sequence;publish();
    }
    void press(unsigned skipped=1){
        advance();auto& r=frame.controllers.hands[1];r.pressed[vr::control_squeeze]+=skipped;
        r.released[vr::control_squeeze]=r.pressed[vr::control_squeeze];
        r.last_squeeze_press_sequence=frame.controllers.sequence;r.last_squeeze_press_flags=15;
        r.last_squeeze_press_grip=r.grip;live=frame;
    }
    gw::EquipmentIntentEvent event(unsigned slot=2){
        gw::EquipmentIntentEvent e{};e.serial=++latest;e.created_ms=ms;e.input_session=frame.controllers.session;
        e.input_sequence=frame.controllers.sequence;e.space_epoch=frame.controllers.space_epoch;
        e.input_epoch=frame.controllers.input_epoch;e.world_generation=frame.key.world_generation;
        e.controller=frame.controller.address;e.pawn=frame.pawn.address;e.weapon=frame.weapon.address;
        e.inventory_slot=std::uint8_t(slot);events.push_back(e);return e;
    }
    void selected(unsigned slot=2,bool busy=false){
        advance();game.current_weapon=game.weapon_in_slot[slot];game.animation_busy=busy;
        for(auto const& o:objects)if(o.address==game.current_weapon)frame.weapon=o;
        live=frame;
    }
    void fault(Callback which){
        if(invalidate_at==which){current_ok=false;invalidate_at=Callback::none;}
        if(age_at==which){ms+=delay_ms;ns+=std::int64_t(delay_ms)*1000000;age_at=Callback::none;}
    }
    static Host& host(void* p){return *static_cast<Host*>(p);}
    static std::uint64_t clock_ms(void* p) noexcept {return host(p).ms;}
    static std::int64_t clock_ns(void* p) noexcept {return host(p).ns;}
    static bool current(void* p,wh::Frame const& f) noexcept {
        auto& h=host(p);return h.current_ok&&!std::memcmp(&f,&h.live,sizeof(f));
    }
    static bool next(void* p,wh::Frame const& f,std::uint64_t& cursor,gw::EquipmentIntentEvent& out) noexcept {
        C(!std::memcmp(&f,&host(p).frame,sizeof(f)));
        auto& h=host(p);++h.next_calls;h.fault(Callback::next);
        if(cursor==UINT64_MAX){if(!h.baseline_blocked)cursor=h.latest;return false;}
        for(auto const& e:h.events)if(e.serial==cursor+1){cursor=e.serial;out=e;return true;}
        return false;
    }
    static bool inventory(void* p,wh::Frame const&,gw::EquipmentSnapshot& out) noexcept {
        auto& h=host(p);++h.inventory_calls;h.fault(Callback::inventory);if(!h.inventory_ok)return false;out=h.game;return true;
    }
    unsigned held_inventory_calls{};
    static bool held_inventory(void* p,wh::Frame const& f,gw::EquipmentSnapshot& out) noexcept {
        ++host(p).held_inventory_calls;return inventory(p,f,out);
    }    static bool attached(void* p,wh::Frame const&,gw::EquipmentSnapshot const&) noexcept {
        auto& h=host(p);++h.attached_calls;h.fault(Callback::attached);return h.attached_ok;
    }
    static bool identify(void* p,std::uint64_t address,wh::ObjectIdentity& out) noexcept {
        auto& h=host(p);++h.identify_calls;h.fault(Callback::identify);if(!h.identify_ok)return false;
        for(auto const& o:h.objects)if(o.address==address){out=o;return true;}return false;
    }
    static bool forward(void* p,wh::Frame const&,gw::EquipmentSnapshot const&,unsigned slot) noexcept {
        auto& h=host(p);++h.switch_calls;h.last_slot=slot;h.fault(Callback::forward);
        if(h.reenter){h.reenter=false;h.reentrant_status=h.runtime.tick(h.frame,h.backend(),h.model).status;}
        if(h.suspend_in_forward){h.suspend_in_forward=false;h.runtime.suspend(h.backend());}
        return h.switch_ok;
    }
    static bool empty_sound(void* p,wh::Frame const&,gw::EquipmentSnapshot const&,unsigned slot) noexcept {
        auto& h=host(p);++h.sound_calls;h.last_slot=slot;return h.sound_ok;
    }
    static bool hold(void* p,wh::Frame const&,gw::EquipmentSnapshot const& game) noexcept {
        auto& h=host(p);++h.hold_calls;h.hold_saw_model=h.model.held_for(game.world,game.pawn,game.current_weapon);
        h.published_hold=h.hold_ok;h.fault(Callback::hold);return h.hold_ok;
    }
    static void clear_hold(void* p) noexcept {auto& h=host(p);++h.clear_calls;h.published_hold=false;}
    static void reset_input(void* p) noexcept {++host(p).reset_calls;}
    static bool stow(void* p,wh::Frame const&,gw::EquipmentSnapshot const&) noexcept {
        auto& h=host(p);++h.stow_calls;if(h.stow_ok)h.attached_ok=false;return h.stow_ok;
    }
    static bool unholster(void* p,wh::Frame const&,gw::EquipmentSnapshot const&,bool& changed) noexcept {
        auto& h=host(p);++h.unholster_calls;changed=h.unholster_changed;
        if(h.unholster_ok&&changed)h.attached_ok=true;
        return h.unholster_ok;
    }
    static void idle_unheld(void* p,wh::Frame const&,gw::EquipmentSnapshot const&) noexcept {++host(p).idle_unheld_calls;}
    static bool prepare_stow(void* p,wh::Frame const&,bool& changed) noexcept {
        auto& h=host(p);++h.prepare_stow_calls;changed=h.prepare_stow_changed;
        if(h.invalidate_on_prepare)h.current_ok=false;
        return h.prepare_stow_ok;
    }
    static bool begin_index(void* p,wh::Frame const&,gw::EquipmentSnapshot const&,unsigned) noexcept {
        auto& h=host(p);++h.index_begins;return h.index_allowed;
    }
    er::Backend backend(){return {this,clock_ms,clock_ns,current,next,inventory,attached,identify,forward,empty_sound,hold,clear_hold,reset_input,held_inventory,
        integrated_stow?stow:nullptr,integrated_stow?unholster:nullptr,integrated_stow?idle_unheld:nullptr,
        integrated_stow?prepare_stow:nullptr,index_enabled?begin_index:nullptr};}
    er::Outcome tick(){return runtime.tick(frame,backend(),model);}
    void baseline(){
        const auto switches=switch_calls,sounds=sound_calls,holds=hold_calls;
        C(tick().status==er::Status::baseline);C(runtime.state().baseline_ready);
        C(switch_calls==switches&&sound_calls==sounds&&hold_calls==holds);
    }
    void start(unsigned slot=2){baseline();press();event(slot);C(tick().status==er::Status::switch_requested);C(switch_calls==1&&model.switch_pending()&&!model.held()&&!published_hold);}
};

}
