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
void index_acquisition_cases(){
    const auto press=[](Host& h,unsigned slot=2){h.advance();auto& r=h.frame.controllers.hands[1];
        r.trigger_active=1;r.trigger=1;++r.pressed[vr::control_trigger];h.publish();
        h.event(slot);h.events.back().kind=gw::EquipmentIntentKind::index_acquire_slot;};
    for(unsigned slot=2;slot<4;++slot){
        ++cases;Host h;h.index_enabled=true;h.baseline();press(h,slot);const auto before=h.frame;
        const auto result=h.tick();C(result.status==er::Status::switch_requested&&result.press_counter==1);
        C(h.switch_calls==1&&h.index_begins==1&&h.runtime.state().consumed_press==0&&h.runtime.state().consumed_index_press==1);
        C(!std::memcmp(&before,&h.frame,sizeof(before)));h.selected(slot);C(h.tick().status==er::Status::held);
        h.advance();++h.frame.controllers.hands[1].released[vr::control_trigger];
        h.frame.controllers.hands[1].trigger=0;h.publish();h.tick();
        press(h,slot);C(h.tick().status==er::Status::invalid_event&&h.model.held()&&h.stow_calls==0&&h.switch_calls==1);
    }
    for(unsigned bad=0;bad<7;++bad){
        ++cases;Host h;h.index_enabled=bad!=0;h.index_allowed=bad!=1;h.baseline();press(h,bad==2?0:2);
        auto& r=h.frame.controllers.hands[1];
        if(bad==3){r.trigger=0;++r.released[vr::control_trigger];}
        if(bad==4)--h.events.back().input_sequence;
        if(bad==5)++h.events.back().input_sequence;
        if(bad==6)r.grip_flags=3;
        h.publish();h.tick();C(!h.model.held()&&h.switch_calls==0&&h.hold_calls==0&&!h.runtime.state().future_known);
    }
    ++cases;Host empty;empty.index_enabled=true;empty.game.weapon_in_slot[2]=0;empty.baseline();press(empty);
    C(empty.tick().status==er::Status::empty_slot&&empty.sound_calls==1&&empty.switch_calls==0&&!empty.model.held());
    empty.event();empty.events.back().kind=gw::EquipmentIntentKind::index_acquire_slot;
    C(empty.tick().status==er::Status::duplicate_press&&empty.sound_calls==1);
}

void baseline_cases(){
    ++cases;Host h;h.press();h.event();h.baseline_blocked=true;
    C(h.tick().status==er::Status::baseline_waiting);C(h.runtime.state().cursor==UINT64_MAX);
    h.press();h.event();C(h.tick().status==er::Status::baseline_waiting);C(h.switch_calls==0);
    h.baseline_blocked=false;C(h.tick().status==er::Status::baseline);
    C(h.runtime.state().consumed_press==2&&h.runtime.state().cursor==2);C(h.tick().status==er::Status::idle);
    h.press();h.event();auto out=h.tick();C(out.status==er::Status::switch_requested&&out.event_serial==3&&out.press_counter==3);
    C(h.switch_calls==1&&h.hold_calls==0);

    ++cases;Host stale;stale.invalidate_at=Callback::next;
    C(stale.tick().status==er::Status::unavailable);C(!stale.runtime.state().baseline_ready&&stale.runtime.state().cursor==UINT64_MAX);
    stale.current_ok=true;stale.advance();stale.baseline();
}
void press_cases(){
    ++cases;Host h;h.baseline();h.press(3);h.event();h.event();
    C(h.tick().status==er::Status::switch_requested);C(h.runtime.state().consumed_press==3);
    C(h.tick().status==er::Status::duplicate_press);C(h.switch_calls==1&&h.sound_calls==0&&h.hold_calls==0);
    h.press();h.event(3);C(h.tick().status==er::Status::busy);C(h.switch_calls==1&&h.runtime.state().consumed_press==4);

    ++cases;Host old;old.baseline();old.press();old.event();old.press();old.event(3);
    C(old.tick().status==er::Status::invalid_event);C(old.switch_calls==0);
    C(old.tick().status==er::Status::switch_requested);C(old.last_slot==3&&old.runtime.state().consumed_press==2);

    ++cases;Host outside;outside.baseline();outside.press(5);C(outside.tick().status==er::Status::idle);
    outside.press();outside.event();C(outside.tick().status==er::Status::switch_requested);C(outside.runtime.state().consumed_press==6);
}
void future_cases(){
    ++cases;Host h;h.baseline();h.press();h.event();h.events.back().input_sequence+=2;
    const auto serial=h.events.back().serial,created=h.events.back().created_ms;
    C(h.tick().status==er::Status::future_event);auto reads=h.next_calls;
    h.event(3);C(h.tick().status==er::Status::future_event);C(h.next_calls==reads);
    C(h.runtime.state().future.serial==serial&&h.runtime.state().future.created_ms==created);
    h.advance();C(h.tick().status==er::Status::future_event);C(h.next_calls==reads);
    h.advance();C(h.tick().status==er::Status::switch_requested);C(h.switch_calls==1&&h.last_slot==2&&!h.runtime.state().future_known);
    C(h.tick().status==er::Status::duplicate_press);C(h.switch_calls==1);

    ++cases;Host expired;expired.baseline();expired.press();expired.event();expired.events.back().input_sequence+=10;
    C(expired.tick().status==er::Status::future_event);reads=expired.next_calls;
    expired.advance(251);C(expired.tick().status==er::Status::expired_event);C(expired.next_calls==reads&&expired.switch_calls==0);

    ++cases;Host boundary;boundary.baseline();boundary.press();boundary.event();boundary.events.back().input_sequence+=10;
    C(boundary.tick().status==er::Status::future_event);++boundary.frame.controllers.input_epoch;boundary.publish();
    C(boundary.tick().status==er::Status::baseline);C(!boundary.runtime.state().future_known&&boundary.switch_calls==0);
}
void event_rejections(){
    // The live producer validates these too; this consumer keeps its own exact
    // tuple/age proof even when a controlled backend returns malformed data.
    for(unsigned which=0;which<15;++which){
        ++cases;Host h;h.baseline();h.press();h.event();auto& e=h.events.back();
        switch(which){
        case 0:++e.controller;break;case 1:++e.pawn;break;case 2:++e.weapon;break;
        case 3:++e.input_session;break;case 4:++e.space_epoch;break;case 5:++e.input_epoch;break;
        case 6:++e.world_generation;break;case 7:e.inventory_slot=4;break;
        case 8:e.input_sequence=0;break;case 9:e.created_ms=h.ms-251;break;
        case 10:e.created_ms=h.ms+1;break;
        case 11:h.frame.controllers.hands[1].last_squeeze_press_sequence=0;h.live=h.frame;break;
        case 12:h.frame.controllers.hands[1].last_squeeze_press_flags=3;h.live=h.frame;break;
        case 13:h.frame.controllers.hands[1].last_squeeze_press_grip.orientation.w=0;h.live=h.frame;break;
        default:e.kind=gw::EquipmentIntentKind::grab_ammo;e.inventory_slot=4;break;
        }
        const auto out=h.tick();C(out.status!=er::Status::switch_requested&&out.status!=er::Status::held);
        C(h.switch_calls==0&&h.sound_calls==0&&h.hold_calls==0);
    }
    ++cases;Host reload;reload.baseline();reload.press();reload.event();reload.events.back().kind=gw::EquipmentIntentKind::reload;
    reload.events.back().inventory_slot=4;C(reload.tick().status==er::Status::event_ignored);C(reload.switch_calls==0&&reload.sound_calls==0);

    ++cases;Host edge;edge.baseline();edge.press();edge.event();edge.events.back().created_ms=edge.ms-250;
    C(edge.tick().status==er::Status::switch_requested);

    ++cases;Host late;late.baseline();late.press();late.event();late.events.back().created_ms=late.ms-240;
    late.age_at=Callback::identify;late.delay_ms=11;
    C(late.tick().status==er::Status::expired_event);C(late.switch_calls==0&&late.hold_calls==0);
}
void source_cases(){
    for(unsigned which=0;which<5;++which){
        ++cases;Host h;h.baseline();h.press();h.event();
        switch(which){
        case 0:h.ns+=100000001;break;case 1:h.frame.controllers.focused=0;h.live=h.frame;break;
        case 2:h.frame.controllers.hands[1].grip_active=0;h.live=h.frame;break;
        case 3:h.current_ok=false;break;default:++h.live.key.frame;break;
        }
        const auto resets=h.reset_calls;C(h.tick().status==er::Status::unavailable);C(h.reset_calls>resets);
        C(h.runtime.state().cursor==UINT64_MAX&&!h.runtime.state().future_known&&h.switch_calls==0);
        h.current_ok=true;h.frame.controllers.focused=1;h.frame.controllers.hands[1].grip_active=1;h.advance();
        C(h.tick().status==er::Status::baseline);C(h.switch_calls==0);h.press();h.event();C(h.tick().status==er::Status::switch_requested);
    }
    for(auto stage:{Callback::inventory,Callback::identify,Callback::attached}){
        ++cases;Host h;h.baseline();h.press();h.event(stage==Callback::attached?0u:2u);
        h.age_at=stage;h.delay_ms=101;C(h.tick().status==er::Status::unavailable);
        C(h.switch_calls==0&&h.hold_calls==0&&!h.runtime.state().baseline_ready);
    }
}
void boundary_cases(){
    for(unsigned which=0;which<5;++which){
        ++cases;Host h;h.start();const auto deadline=h.runtime.state().deadline_ms;
        h.advance();switch(which){
        case 0:++h.frame.key.lifecycle;break;
        case 1:++h.frame.controllers.input_epoch;break;
        case 2:++h.frame.controllers.profile_generation;break;
        case 3:++h.frame.controllers.space_epoch;h.frame.stereo.space_epoch=h.frame.controllers.space_epoch;break;
        default:++h.frame.controllers.session;h.frame.stereo.session=h.frame.controllers.session;break;
        }
        h.live=h.frame;C(h.tick().status==er::Status::baseline);C(h.model.switch_pending()&&h.runtime.state().pending);
        C(h.runtime.state().deadline_ms==deadline&&h.switch_calls==1);
        h.selected();C(h.tick().status==er::Status::held);C(h.hold_calls==1&&h.switch_calls==1);
    }
    for(unsigned which=0;which<12;++which){
        ++cases;Host h;h.start();h.advance();
        switch(which){
        case 0:++h.frame.key.registration_cookie;break;case 1:++h.frame.key.native_started_qpc;break;
        case 2:++h.frame.key.game_start;break;case 3:++h.frame.key.game_pid;break;
        case 4:++h.frame.key.world_generation;break;case 5:++h.frame.key.thread_id;break;
        case 6:++h.frame.world.name_number;break;case 7:++h.frame.controller.registry_index;break;
        case 8:++h.frame.pawn.name_index;break;case 9:h.frame.mesh.outer+=8;break;
        case 10:h.frame.pawn.klass+=8;break;default:h.frame.mesh.address+=8;break;
        }
        h.live=h.frame;C(h.tick().status==er::Status::baseline);C(!h.model.switch_pending()&&!h.runtime.state().pending&&!h.model.held());
        C(h.switch_calls==1&&h.hold_calls==0);
    }
    ++cases;Host loss;loss.start();const auto deadline=loss.runtime.state().deadline_ms;
    loss.runtime.suspend(loss.backend());C(loss.runtime.state().pending&&loss.model.switch_pending());
    C(loss.runtime.state().deadline_ms==deadline&&!loss.runtime.state().baseline_ready);
    loss.selected();C(loss.tick().status==er::Status::held);C(loss.switch_calls==1&&loss.hold_calls==1);
}
void decision_cases(){
    for(bool animation:{false,true}){
        ++cases;Host h;h.baseline();h.game.animation_busy=animation;h.game.can_switch=animation;
        h.press();h.event();C(h.tick().status==er::Status::busy);C(h.switch_calls==0&&h.sound_calls==0&&h.hold_calls==0);
        h.game.animation_busy=false;h.game.can_switch=true;h.event();C(h.tick().status==er::Status::duplicate_press);C(h.switch_calls==0);
    }
    for(unsigned which=0;which<4;++which){
        ++cases;Host h;h.baseline();unsigned slot=2;
        switch(which){case 0:h.game.weapon_in_slot[2]=0;break;case 1:h.game.ammo_available[2]=false;break;
        case 2:slot=1;h.game.throwable_count=0;break;default:slot=1;h.game.throwable_count_known=false;break;}
        h.press();h.event(slot);C(h.tick().status==er::Status::empty_slot);C(h.sound_calls==1&&h.switch_calls==0);
        h.event(slot);C(h.tick().status==er::Status::duplicate_press);C(h.sound_calls==1);
    }
    ++cases;Host sound;sound.baseline();sound.game.weapon_in_slot[2]=0;sound.sound_ok=false;sound.press();sound.event();
    C(sound.tick().status==er::Status::empty_sound_failed);C(sound.sound_calls==1);

    ++cases;Host dry;dry.baseline();dry.game.ammo_available[2]=false;dry.game.can_select_without_ammo[2]=true;
    dry.press();dry.event();C(dry.tick().status==er::Status::switch_requested);C(dry.sound_calls==0);

    ++cases;Host h;h.baseline();h.press();h.event(0);C(h.tick().status==er::Status::held);
    C(h.model.held()&&h.hold_calls==1&&h.switch_calls==0&&h.published_hold&&h.hold_saw_model);
    const auto generation=h.model.hold_generation();h.press();h.event(0);
    C(h.tick().status==er::Status::unsupported_stow);C(h.model.held()&&h.model.hold_generation()==generation&&h.published_hold);
    C(h.hold_calls==1&&h.switch_calls==0&&h.sound_calls==0);

    ++cases;Host unknown;unknown.baseline();unknown.attached_ok=false;unknown.press();unknown.event(0);
    C(unknown.tick().status==er::Status::attachment_unavailable);C(!unknown.model.held()&&unknown.hold_calls==0);
    unknown.attached_ok=true;unknown.event(0);C(unknown.tick().status==er::Status::duplicate_press);C(unknown.hold_calls==0);

    ++cases;Host fail;fail.baseline();fail.hold_ok=false;fail.press();fail.event(0);
    C(fail.tick().status==er::Status::hold_failed);C(!fail.model.held()&&!fail.published_hold&&fail.hold_calls==1&&fail.hold_saw_model);
}
void observe_cases(){
    ++cases;Host h;h.start();auto reads=h.inventory_calls;
    h.advance(20);C(h.tick().status==er::Status::switch_pending);C(h.inventory_calls==reads+1&&h.switch_calls==1);
    h.selected(2,true);C(h.tick().status==er::Status::switch_pending);C(h.hold_calls==0);
    h.game.animation_busy=false;h.attached_ok=false;h.advance();C(h.tick().status==er::Status::attachment_unavailable);
    C(h.model.switch_pending()&&h.runtime.state().pending&&h.hold_calls==0);
    h.attached_ok=true;h.advance();C(h.tick().status==er::Status::held);
    C(h.model.held_for(h.game.world,h.game.pawn,h.game.current_weapon)&&h.hold_saw_model&&h.published_hold);
    C(h.switch_calls==1&&h.hold_calls==1&&!h.runtime.state().pending);
    h.advance();C(h.tick().status==er::Status::idle);C(h.hold_calls==1);

    for(unsigned which=0;which<3;++which){
        ++cases;Host changed;changed.start();changed.advance();
        if(which==0)changed.game.weapon_in_slot[2]=0x7800;
        else if(which==1)++changed.objects[2].name_number;
        else changed.selected(3);
        C(changed.tick().status==er::Status::switch_interrupted);
        C(!changed.model.switch_pending()&&!changed.runtime.state().pending&&changed.switch_calls==1&&changed.hold_calls==0);
    }
    ++cases;Host timed;timed.start();timed.advance(5000);
    C(timed.tick().status==er::Status::switch_timed_out);C(!timed.model.switch_pending()&&timed.switch_calls==1&&timed.hold_calls==0);
    timed.selected();C(timed.tick().status==er::Status::idle);C(timed.hold_calls==0);

    ++cases;Host delayed;delayed.start();delayed.advance(4950);delayed.selected();
    delayed.age_at=Callback::attached;delayed.delay_ms=41;
    C(delayed.tick().status==er::Status::switch_timed_out);C(delayed.hold_calls==0&&!delayed.model.held());

    ++cases;Host hold_late;hold_late.start();hold_late.advance(4950);hold_late.selected();
    hold_late.age_at=Callback::hold;hold_late.delay_ms=41;
    C(hold_late.tick().status==er::Status::switch_timed_out);
    C(hold_late.hold_calls==1&&hold_late.switch_calls==1&&!hold_late.model.held()&&!hold_late.published_hold);

    for(auto callback:{Callback::identify,Callback::attached,Callback::hold}){
        ++cases;Host lost;lost.start();lost.selected();lost.invalidate_at=callback;
        C(lost.tick().status==er::Status::unavailable);C(!lost.model.held()&&!lost.published_hold);
        C(lost.switch_calls==1&&lost.hold_calls==(callback==Callback::hold?1u:0u));
        if(callback!=Callback::hold)C(lost.runtime.state().pending&&lost.model.switch_pending());
    }
    ++cases;Host reject;reject.start();reject.selected();reject.hold_ok=false;
    C(reject.tick().status==er::Status::hold_failed);C(!reject.model.held()&&!reject.runtime.state().pending&&!reject.published_hold);

    ++cases;Host temporary;temporary.start();temporary.inventory_ok=false;temporary.advance();
    C(temporary.tick().status==er::Status::inventory_unavailable);C(temporary.runtime.state().pending);
    temporary.inventory_ok=true;temporary.selected();C(temporary.tick().status==er::Status::held);C(temporary.switch_calls==1);
}
void final_checks(){
    ++cases;Host failed;failed.baseline();failed.press();failed.event();failed.switch_ok=false;
    C(failed.tick().status==er::Status::switch_failed);C(!failed.model.switch_pending()&&!failed.runtime.state().pending);
    failed.event();C(failed.tick().status==er::Status::duplicate_press);C(failed.switch_calls==1);

    for(bool stop:{false,true}){
        ++cases;Host h;h.baseline();h.press();h.event();
        if(stop)h.suspend_in_forward=true;else h.invalidate_at=Callback::forward;
        C(h.tick().status==er::Status::unavailable);C(h.runtime.state().pending&&h.model.switch_pending()&&h.switch_calls==1);
        h.current_ok=true;h.selected();C(h.tick().status==er::Status::held);C(h.switch_calls==1&&h.hold_calls==1);
    }
    ++cases;Host recurse;recurse.baseline();recurse.press();recurse.event();recurse.reenter=true;
    C(recurse.tick().status==er::Status::switch_requested);C(recurse.reentrant_status==er::Status::reentrant&&recurse.switch_calls==1);

    ++cases;Host regressed;regressed.start();--regressed.ms;C(regressed.tick().status==er::Status::clock_invalid);
    C(!regressed.model.switch_pending()&&!regressed.runtime.state().pending&&regressed.switch_calls==1);
    regressed.runtime.reset_after_drain(regressed.backend(),regressed.model);regressed.advance();regressed.baseline();

    ++cases;Host overflow;overflow.ms=UINT64_MAX-4000;overflow.baseline();overflow.press();overflow.event();
    C(overflow.tick().status==er::Status::clock_invalid);C(overflow.switch_calls==0&&!overflow.model.switch_pending());

    ++cases;Host invalid;auto b=invalid.backend();b.hold=nullptr;
    C(invalid.runtime.tick(invalid.frame,b,invalid.model).status==er::Status::invalid_backend);C(invalid.switch_calls==0);

    ++cases;Host held;held.baseline();held.press();held.event(0);C(held.tick().status==er::Status::held);
    held.runtime.suspend(held.backend());C(held.model.held()&&!held.published_hold);
    held.advance();C(held.tick().status==er::Status::baseline);C(held.model.held()&&held.published_hold&&held.hold_calls==2);
    ++held.objects[0].registry_index;held.advance();C(held.tick().status==er::Status::identity_mismatch);C(!held.model.held()&&!held.published_hold);

    ++cases;Host animation;animation.baseline();animation.press();animation.event(0);
    C(animation.tick().status==er::Status::held&&animation.published_hold);
    // The independent original-animation owner has validated its current hold.
    // Suspending only inventory input must drop a queued shoulder edge without
    // clearing/reissuing that existing hold or admitting any action itself.
    animation.press();animation.event(2);
    const auto animation_holds=animation.hold_calls,animation_switches=animation.switch_calls,
        animation_resets=animation.reset_calls,animation_clears=animation.clear_calls;
    animation.runtime.suspend_input(animation.backend());
    C(animation.published_hold&&animation.runtime.state().hold_published&&animation.model.held());
    C(!animation.runtime.state().input_bound&&!animation.runtime.state().baseline_ready&&
        !animation.runtime.state().future_known&&animation.runtime.state().cursor==UINT64_MAX);
    C(animation.reset_calls==animation_resets+1&&animation.clear_calls==animation_clears);
    animation.advance();C(animation.tick().status==er::Status::baseline);
    animation.advance();C(animation.tick().status==er::Status::idle);
    C(animation.hold_calls==animation_holds&&animation.switch_calls==animation_switches&&animation.published_hold);
    // This narrow API cannot make a stale/unowned Frame actionable. Ordinary
    // currentness failure still revokes the publication through suspend().
    animation.current_ok=false;animation.advance();C(animation.tick().status==er::Status::unavailable);
    C(!animation.published_hold&&!animation.runtime.state().hold_published);

    ++cases;Host idle;idle.baseline();idle.press();idle.event(0);C(idle.tick().status==er::Status::held);
    C(idle.held_inventory_calls==0);idle.advance();C(idle.tick().status==er::Status::idle);
    C(idle.held_inventory_calls==1&&idle.model.held());
    idle.advance();idle.press();idle.event(0);(void)idle.tick();C(idle.held_inventory_calls==1);

    ++cases;Host reset;reset.start();reset.runtime.reset_after_drain(reset.backend(),reset.model);
    C(!reset.runtime.state().actor_bound&&!reset.runtime.state().pending&&!reset.model.switch_pending());
    C(reset.runtime.state().cursor==UINT64_MAX&&reset.switch_calls==1);reset.advance();reset.baseline();
}
}
void integrated_stow_cases(){
    const auto held=[](Host& h){h.integrated_stow=true;h.baseline();h.press();h.event(0);C(h.tick().status==er::Status::held);};
    ++cases;Host parked;held(parked);parked.prepare_stow_changed=true;parked.invalidate_on_prepare=true;
    parked.press();const auto request=parked.event(0);
    C(parked.tick().status==er::Status::stow_pending);
    C(parked.stow_calls==0&&parked.runtime.state().pending_stow&&parked.model.held());
    C(parked.runtime.state().deadline_ms==request.created_ms+250);
    parked.current_ok=true;parked.advance();
    C(parked.tick().status==er::Status::stowed);
    C(parked.stow_calls==1&&parked.prepare_stow_calls==1&&!parked.model.held());
    C(!parked.runtime.state().pending);
    ++cases;Host stale;held(stale);stale.prepare_stow_changed=true;stale.press();stale.event(0);
    C(stale.tick().status==er::Status::stow_pending);stale.advance(250);
    C(stale.tick().status==er::Status::switch_timed_out&&stale.stow_calls==0);
    ++cases;Host foreign;held(foreign);foreign.prepare_stow_changed=true;foreign.press();foreign.event(0);
    C(foreign.tick().status==er::Status::stow_pending);foreign.advance();foreign.game.weapon_in_slot[0]=0x7770;
    C(foreign.tick().status==er::Status::switch_interrupted&&foreign.stow_calls==0);
    ++cases;Host refused;held(refused);refused.prepare_stow_ok=false;refused.press();refused.event(0);
    C(refused.tick().status==er::Status::stow_failed&&refused.stow_calls==0&&refused.model.held());
    ++cases;Host h;h.integrated_stow=true;h.baseline();C(h.idle_unheld_calls==1);
    h.press();h.event(0);C(h.tick().status==er::Status::held);
    C(h.model.held()&&h.published_hold&&h.unholster_calls==1);
    h.press();h.event(0);C(h.tick().status==er::Status::stowed);
    C(h.stow_calls==1&&!h.model.held()&&!h.published_hold&&!h.runtime.state().pending);
    h.advance();C(h.tick().status==er::Status::idle);C(h.idle_unheld_calls==2);
    h.unholster_changed=true;h.press();h.event(0);const auto holds=h.hold_calls;
    C(h.tick().status==er::Status::unholster_pending);
    C(h.runtime.state().pending_unholster&&!h.model.held()&&!h.published_hold&&h.hold_calls==holds);
    C(h.tick().status==er::Status::unholster_pending);C(h.hold_calls==holds);
    h.advance();C(h.tick().status==er::Status::held);
    C(h.model.held()&&h.published_hold&&!h.runtime.state().pending&&h.hold_calls==holds+1);
    C(h.stow_calls==1&&h.unholster_calls==2&&h.switch_calls==0);

    ++cases;Host failed;failed.integrated_stow=true;failed.baseline();failed.press();failed.event(0);
    C(failed.tick().status==er::Status::held);failed.stow_ok=false;failed.press();failed.event(0);
    C(failed.tick().status==er::Status::stow_failed);C(failed.model.held()&&failed.published_hold);

    ++cases;Host unholster;unholster.integrated_stow=true;unholster.unholster_ok=false;
    unholster.baseline();unholster.press();unholster.event(0);
    C(unholster.tick().status==er::Status::unholster_failed);
    C(!unholster.model.held()&&!unholster.published_hold&&!unholster.runtime.state().pending&&unholster.hold_calls==0);

    ++cases;Host timeout;timeout.integrated_stow=true;timeout.unholster_changed=true;
    timeout.baseline();timeout.press();timeout.event(0);C(timeout.tick().status==er::Status::unholster_pending);
    timeout.advance(5000);C(timeout.tick().status==er::Status::switch_timed_out);
    C(!timeout.model.held()&&!timeout.runtime.state().pending&&timeout.hold_calls==0&&timeout.unholster_calls==1);

    ++cases;Host replaced;replaced.integrated_stow=true;replaced.unholster_changed=true;
    replaced.baseline();replaced.press();replaced.event(0);C(replaced.tick().status==er::Status::unholster_pending);
    replaced.advance();replaced.game.weapon_in_slot[0]=0x7770;
    C(replaced.tick().status==er::Status::switch_interrupted);
    C(!replaced.model.held()&&!replaced.runtime.state().pending&&replaced.hold_calls==0);

    ++cases;Host boundary;boundary.integrated_stow=true;boundary.unholster_changed=true;
    boundary.baseline();boundary.press();boundary.event(0);C(boundary.tick().status==er::Status::unholster_pending);
    const auto deadline=boundary.runtime.state().deadline_ms;boundary.runtime.suspend(boundary.backend());
    C(boundary.runtime.state().pending_unholster&&boundary.runtime.state().deadline_ms==deadline);
    boundary.advance();C(boundary.tick().status==er::Status::held);C(boundary.hold_calls==1&&boundary.unholster_calls==1);

    ++cases;Host switched;switched.integrated_stow=true;switched.unholster_changed=true;
    switched.baseline();switched.press();switched.event(2);
    C(switched.tick().status==er::Status::switch_pending);
    C(switched.runtime.state().pending_dispatch&&!switched.model.switch_pending()&&switched.switch_calls==0);
    C(switched.tick().status==er::Status::switch_pending);C(switched.switch_calls==0);
    switched.advance();C(switched.tick().status==er::Status::switch_requested);
    C(!switched.runtime.state().pending_dispatch&&switched.model.switch_pending()&&switched.switch_calls==1);
    switched.selected();C(switched.tick().status==er::Status::held);
    C(switched.switch_calls==1&&switched.unholster_calls==1&&switched.hold_calls==1);

    ++cases;Host expired;expired.integrated_stow=true;expired.unholster_changed=true;
    expired.baseline();expired.press();const auto old=expired.event(2);
    C(expired.tick().status==er::Status::switch_pending);
    C(expired.runtime.state().deadline_ms==old.created_ms+250);
    expired.advance(250);C(expired.tick().status==er::Status::switch_timed_out);
    C(!expired.runtime.state().pending&&expired.switch_calls==0&&expired.hold_calls==0);
}
void blocked_input_cases(){
    const auto held=[](Host& h){h.start();h.selected();C(h.tick().status==er::Status::held);C(h.model.held());};
    ++cases;Host unbound;C(!unbound.runtime.observe_input_while_blocked(unbound.frame,unbound.backend()));
    C(unbound.next_calls==0&&unbound.switch_calls==0);
    ++cases;Host h;held(h);h.game.can_switch=false;
    const auto switches=h.switch_calls,holds=h.hold_calls,sounds=h.sound_calls;
    // A long original motor hold does not pause the sole input observer or
    // permit any equipment action. Fresh source frames retain original ages.
    for(unsigned n=0;n<120;++n){h.advance(90);C(h.runtime.observe_input_while_blocked(h.frame,h.backend()));}
    C(h.switch_calls==switches&&h.hold_calls==holds&&h.sound_calls==sounds&&h.model.held());
    h.press();const auto original=h.event(3);
    C(h.runtime.observe_input_while_blocked(h.frame,h.backend()));
    C(h.runtime.state().future_known&&h.runtime.state().future.created_ms==original.created_ms&&h.switch_calls==switches);
    const auto reads=h.next_calls;
    h.advance(100);C(h.runtime.observe_input_while_blocked(h.frame,h.backend()));
    C(h.next_calls==reads&&h.runtime.state().future.created_ms==original.created_ms);
    h.game.can_switch=true;C(h.tick().status==er::Status::switch_requested&&h.switch_calls==switches+1&&h.last_slot==3);
    C(!h.runtime.state().future_known);

    ++cases;Host expired;held(expired);expired.press();expired.event(3);
    C(expired.runtime.observe_input_while_blocked(expired.frame,expired.backend()));
    expired.advance(251);C(expired.runtime.observe_input_while_blocked(expired.frame,expired.backend()));
    C(!expired.runtime.state().future_known);(void)expired.tick();C(expired.switch_calls==1);
    ++cases;Host boundary;held(boundary);boundary.press();boundary.event(3);
    C(boundary.runtime.observe_input_while_blocked(boundary.frame,boundary.backend()));
    ++boundary.frame.controllers.input_epoch;boundary.publish();
    C(!boundary.runtime.observe_input_while_blocked(boundary.frame,boundary.backend()));
    C(!boundary.runtime.state().future_known&&!boundary.runtime.state().baseline_ready&&boundary.switch_calls==1);
    ++cases;Host weapon;held(weapon);weapon.press();weapon.event(3);
    C(weapon.runtime.observe_input_while_blocked(weapon.frame,weapon.backend()));
    weapon.frame.weapon.registry_index++;weapon.publish();
    C(!weapon.runtime.observe_input_while_blocked(weapon.frame,weapon.backend()));
    C(!weapon.runtime.state().future_known&&weapon.switch_calls==1);
    ++cases;Host stale;held(stale);stale.ns+=100000001;
    C(!stale.runtime.observe_input_while_blocked(stale.frame,stale.backend()));
    C(!stale.runtime.state().baseline_ready&&stale.switch_calls==1);
    ++cases;Host lost;held(lost);lost.press();lost.event(3);lost.invalidate_at=Callback::next;
    C(!lost.runtime.observe_input_while_blocked(lost.frame,lost.backend()));
    C(!lost.runtime.state().future_known&&lost.switch_calls==1);
    ++cases;Host bad;held(bad);bad.press();bad.event(3);bad.events.back().pawn+=8;
    C(!bad.runtime.observe_input_while_blocked(bad.frame,bad.backend()));
    C(!bad.runtime.state().future_known&&bad.switch_calls==1);

    // 6864: the blocked input observer expired and erased the existing hold
    // before the backward-OFF transition returned to ordinary equipment.
    // A separate live owner can retain only that hold, never the stale Frame
    // or deferred input. Its check runs AFTER the failing backend operation.
    ++cases;Host pinned;held(pinned);pinned.press();pinned.event(3);
    pinned.age_at=Callback::next;pinned.delay_ms=101;
    unsigned pin_checks{};const auto clears=pinned.clear_calls,old_switches=pinned.switch_calls;
    C(!pinned.runtime.observe_input_while_blocked(pinned.frame,pinned.backend(),[&]() noexcept {
        ++pin_checks;C(pinned.age_at==Callback::none);return true;
    }));
    C(pin_checks==1&&pinned.published_hold&&pinned.clear_calls==clears&&pinned.model.held());
    C(!pinned.runtime.state().future_known&&!pinned.runtime.state().baseline_ready&&pinned.switch_calls==old_switches);
    pinned.advance();C(pinned.tick().status==er::Status::baseline);
    C(pinned.published_hold&&pinned.switch_calls==old_switches);

    ++cases;Host released;held(released);released.ns+=100000001;
    const auto released_clears=released.clear_calls;
    C(!released.runtime.observe_input_while_blocked(released.frame,released.backend(),[]() noexcept {return false;}));
    C(!released.published_hold&&released.clear_calls==released_clears+1);

    ++cases;Host changed;held(changed);++changed.frame.controllers.input_epoch;changed.publish();
    unsigned changed_checks{};
    C(!changed.runtime.observe_input_while_blocked(changed.frame,changed.backend(),[&]() noexcept {++changed_checks;return true;}));
    C(changed_checks==0&&!changed.published_hold);

    ++cases;Host malformed;held(malformed);malformed.press();malformed.event(3);malformed.events.back().pawn+=8;
    unsigned malformed_checks{};
    C(!malformed.runtime.observe_input_while_blocked(malformed.frame,malformed.backend(),[&]() noexcept {++malformed_checks;return true;}));
    C(malformed_checks==0&&!malformed.published_hold&&!malformed.runtime.state().future_known);
}
void unheld_input_order_cases(){
    static std::uint64_t expected_input_ms{};
    for(unsigned variant=0;variant<8;++variant){
        ++cases;Host h;h.baseline();h.press();h.event(0);
        const auto original_created=h.events.back().created_ms;
        const auto full_reads=h.inventory_calls;
        expected_input_ms=h.ms;
        auto backend=h.backend();
        backend.next=[](void* p,wh::Frame const& f,std::uint64_t& cursor,gw::EquipmentIntentEvent& event) noexcept {
            C(Host::host(p).ms==expected_input_ms);
            return Host::next(p,f,cursor,event);
        };
        h.age_at=Callback::inventory;
        h.delay_ms=variant==0?0:variant==1?20:variant==2?60:variant==3?101:10;
        if(variant==4)h.inventory_ok=false;
        if(variant==5)h.game.pawn+=8;
        if(variant==6)h.invalidate_at=Callback::inventory;
        if(variant==7)h.identify_ok=false;
        const auto out=h.runtime.tick(h.frame,backend,h.model);
        C(h.inventory_calls==full_reads+1&&h.held_inventory_calls==0);
        C(h.events.back().created_ms==original_created&&!h.runtime.state().future_known);
        if(variant<3)C(out.status==er::Status::held&&h.model.held()&&h.hold_calls==1);
        else {C(!h.model.held()&&!h.published_hold&&h.hold_calls==0&&h.switch_calls==0);
            h.age_at=h.invalidate_at=Callback::none;h.current_ok=h.inventory_ok=h.identify_ok=true;
            h.game.pawn=h.frame.pawn.address;h.advance();h.tick();
            C(!h.model.held()&&!h.published_hold); // No retained local event replay.
        }
    }
}

void delayed_pickup_log_semantics(){
 ++cases;Host h;h.integrated_stow=true;h.unholster_changed=true;h.baseline();h.press();h.event(0);
 const auto pending=h.tick();C(pending.status==er::Status::unholster_pending);
 const auto deadline=h.runtime.state().deadline_ms;
 h.runtime.suspend(h.backend());h.advance();const auto held=h.tick();
 C(held.status==er::Status::held&&held.event_serial==pending.event_serial&&held.press_counter==pending.press_counter);
 C(h.model.held()&&h.published_hold&&h.unholster_calls==1&&!h.runtime.state().pending);
 C(h.ms<deadline);
 // A subsequent publication gap resets the input observer, not the model.
 h.runtime.suspend(h.backend());C(h.model.held()&&!h.published_hold);
 h.advance();const auto baseline=h.tick();
 C(baseline.status==er::Status::baseline&&baseline.slot==4&&!baseline.target&&!baseline.event_serial);
 C(h.model.held()&&h.published_hold&&h.unholster_calls==1&&h.switch_calls==0);
 h.advance();C(h.tick().status==er::Status::idle&&h.model.held()&&h.published_hold);
 std::puts("Actual Runtime delayed original pickup + held baseline outcome: same original event completes once; baseline slot4 target0 coexists with held model and freshly republished binding");
}

int main(){
    delayed_pickup_log_semantics();
    unheld_input_order_cases();
    baseline_cases();press_cases();future_cases();event_rejections();source_cases();boundary_cases();
    decision_cases();observe_cases();final_checks();integrated_stow_cases();
    blocked_input_cases();index_acquisition_cases();
    std::printf("WeaponEquipmentRuntimeTest: %u cases / %u checks passed; actual Runtime + model, controlled synchronous PG backend seam; no Game/XR/Native callback or engine acceptance\n",cases,checks);
}
