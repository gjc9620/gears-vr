#include "weapon_equipment_input.h"
#include "weapon_equipment_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#ifdef NDEBUG
#error NativeEquipmentInputTest requires active checks
#endif
namespace ei=gears_equipment_input;
namespace er=gears_equipment_runtime;
namespace wh=gears_weapon_hand;
namespace vr=gears_vr;
namespace gw=gears_weapon;
namespace {
unsigned cases{},checks{};
void check(bool value,char const* expression,int line){
    ++checks;if(!value){std::fprintf(stderr,"FAIL line %d: %s\n",line,expression);std::exit(1);}
}
#define C(x) check(bool(x),#x,__LINE__)
struct Sample {
    wh::Frame f{};gw::ArmspanCalibration calibration{1,1.7,-.15};
    std::int64_t ns{1000000000};std::uint64_t ms{1000};
    static wh::ObjectIdentity id(std::uint64_t address){return {address,0x8000,0x9000,unsigned(address/8),7,0,1};}
    Sample(){
        f.key={11,22,33,1,1,1,44,55};f.flags=wh::scene_owned|wh::input_matched|wh::gameplay_interactable;
        f.world=id(0x1000);f.controller=id(0x2000);f.pawn=id(0x3000);f.mesh=id(0x4000);f.weapon=id(0x5000);
        f.reference_generation=1;f.stereo_status=unsigned(vr::StereoStatus::accepted);
        auto& s=f.stereo;s.session=1;s.sequence=100;s.predicted_time=123;s.head_flags=s.view_flags=15;
        s.focused=s.visible=s.should_render=1;s.head={{0,1.6,0},{0,0,0,1}};
        s.eyes[0].pose.position={-.032,1.6,0};s.eyes[1].pose.position={.032,1.6,0};
        for(auto& eye:s.eyes){eye.pose.orientation={0,0,0,1};eye.fov={-.8,.8,.8,-.8};}
        auto& c=f.controllers;c.session=s.session;c.sequence=s.sequence;c.space_epoch=s.space_epoch;
        c.predicted_time=s.predicted_time;c.focused=1;
        for(auto& h:c.hands){h.grip_active=1;h.grip_flags=15;h.squeeze_active=1;}
        stamp();C(wh::frame_active(f,ns));
    }
    void stamp(){f.now_ns=ns;f.stereo.received_ns=f.controllers.received_ns=ns-1000000;f.controllers.sync_ns=ns-2000000;}
    void advance(unsigned delta=10){ms+=delta;ns+=std::int64_t(delta)*1000000;++f.key.frame;
        ++f.stereo.sequence;f.controllers.sequence=f.stereo.sequence;stamp();}
    vr::Vector point(unsigned slot) const {
        const double lateral=calibration.armspan_metres*.135;
        const double y=calibration.hands_below_head_metres+.035-(slot<2?.55:0);
        const auto local=vr::Vector{slot==0||slot==2?-lateral:lateral,y,slot<2?.02:.05};
        const auto offset=vr::transform(vr::rotation_matrix(f.reference.orientation),local);
        return {f.stereo.head.position.x+offset.x,f.stereo.head.position.y+offset.y,f.stereo.head.position.z+offset.z};
    }
    void press(unsigned slot=2,unsigned count=1){
        advance();auto& r=f.controllers.hands[1];r.pressed[vr::control_squeeze]+=count;
        r.released[vr::control_squeeze]=r.pressed[vr::control_squeeze]; // Already released is still one recorded press.
        r.squeeze=0;r.grip.position=slot<4?point(slot):vr::Vector{3,3,3};
        r.last_squeeze_press_grip=r.grip;r.last_squeeze_press_flags=15;r.last_squeeze_press_sequence=f.controllers.sequence;
    }
    void simulated(){f.flags|=wh::allow_sim;f.controllers.source=f.stereo.source=vr::StereoSource::MetaOperatorSimulated;}
};
struct Direct {
    Sample sample;ei::Input input;std::uint64_t cursor{UINT64_MAX};ei::Event out{};
    bool next(){return input.next(sample.f,sample.calibration,sample.ns,sample.ms,cursor,out);}
    void baseline(){C(!next());C(cursor!=UINT64_MAX&&input.evidence().status==ei::Status::baseline);}
    void unchanged_failure(ei::Status status){const auto before=out;C(!next());C(cursor==UINT64_MAX);
        C(input.evidence().status==status);C(!std::memcmp(&before,&out,sizeof(out)));}
};
void routes(){
    for(unsigned simulated=0;simulated<2;++simulated)for(unsigned slot=0;slot<4;++slot){
        ++cases;Direct h;if(simulated)h.sample.simulated();
        h.out.serial=999;h.baseline();C(h.out.serial==999);const auto previous=h.sample.f.controllers.sync_ns;
        h.sample.press(slot);h.sample.f.controllers.hands[1].grip.position={3,3,3}; // Last-press pose, not current grip.
        C(h.next());C(h.out.kind==gw::EquipmentIntentKind::slot&&h.out.inventory_slot==slot);
        C(h.out.serial==1&&h.cursor==1&&h.out.input_sequence==h.sample.f.controllers.hands[1].last_squeeze_press_sequence);
        C(h.out.controller==h.sample.f.controller.address&&h.out.pawn==h.sample.f.pawn.address&&h.out.weapon==h.sample.f.weapon.address);
        C(h.out.input_session==h.sample.f.controllers.session&&h.out.input_epoch==h.sample.f.controllers.input_epoch);
        C(h.out.created_ms==998&&h.out.created_ms<h.sample.ms);
        C(h.input.evidence().previous_sync_ns==previous&&h.input.evidence().sync_ns==h.sample.f.controllers.sync_ns);
        C(h.input.evidence().received_ns==h.sample.f.controllers.received_ns&&h.input.evidence().press_counter==1);
        const auto before=h.out;C(!h.next());C(!std::memcmp(&before,&h.out,sizeof(before)));
        h.sample.ns+=5000000;h.sample.ms+=5;C(!h.next());C(h.out.created_ms==998);
        h.sample.advance();C(!h.next());C(h.cursor==1);
    }
    ++cases;Direct rotated;rotated.sample.f.reference.orientation={0,.7071067811865475,0,.7071067811865475};
    rotated.sample.f.stereo.head.orientation={0,-.7071067811865475,0,.7071067811865475};
    rotated.sample.f.stereo.head.position={.3,1.7,-.4};
    for(auto& eye:rotated.sample.f.stereo.eyes){eye.pose.position.x+=.3;eye.pose.position.y+=.1;eye.pose.position.z-=.4;}
    rotated.baseline();rotated.sample.press(3);C(rotated.next());C(rotated.out.inventory_slot==3);

    ++cases;Direct air;air.baseline();air.sample.press(4);C(!air.next());C(air.cursor==0);
    air.sample.press(4);C(!air.next());air.sample.press(2);C(air.next());C(air.input.evidence().press_counter==3);
    ++cases;Direct jump;jump.baseline();jump.sample.press(2,3);C(jump.next());C(jump.input.evidence().press_counter==3);
    C(!jump.next());jump.sample.press(3);C(jump.next());C(jump.out.serial==2&&jump.out.inventory_slot==3);
    ++cases;Direct releases;releases.sample.press(4);
    releases.sample.f.controllers.hands[1].released[vr::control_squeeze]=0;releases.baseline();
    releases.sample.press(2);C(releases.next());C(releases.input.evidence().press_counter==2);
    C(releases.sample.f.controllers.hands[1].released[vr::control_squeeze]==2);

    ++cases;Direct other;other.baseline();other.sample.advance();
    auto& left=other.sample.f.controllers.hands[0];left.pressed[vr::control_squeeze]=left.released[vr::control_squeeze]=3;
    left.last_squeeze_press_sequence=other.sample.f.controllers.sequence;left.last_squeeze_press_flags=15;
    left.last_squeeze_press_grip.position=other.sample.point(2);
    auto& right=other.sample.f.controllers.hands[1];right.pressed[vr::control_trigger]=right.released[vr::control_trigger]=3;
    right.pressed[vr::control_stick]=right.released[vr::control_stick]=2;
    C(!other.next());other.sample.press(2);C(other.next());C(other.out.kind==gw::EquipmentIntentKind::slot);

    ++cases;Direct old;old.sample.press(2);old.baseline();old.sample.advance();C(!old.next());
    old.sample.press(3);C(old.next());C(old.input.evidence().press_counter==2);
}
void boundaries(){
    for(unsigned which=0;which<19;++which){
        ++cases;Direct h;h.baseline();h.sample.press();auto& f=h.sample.f;
        switch(which){
            case 0:++f.key.registration_cookie;break;case 1:++f.key.native_started_qpc;break;
            case 2:++f.key.lifecycle;break;case 3:++f.key.game_start;break;case 4:++f.key.game_pid;break;
            case 5:++f.key.thread_id;break;case 6:++f.key.world_generation;break;
            case 7:++f.world.registry_index;break;case 8:++f.controller.registry_index;break;
            case 9:++f.pawn.registry_index;break;case 10:++f.mesh.registry_index;break;
            case 11:++f.weapon.registry_index;break;case 12:++f.controllers.profile_generation;break;
            case 13:++f.controllers.input_epoch;break;case 14:++f.controllers.space_epoch;++f.stereo.space_epoch;break;
            case 15:++f.controllers.session;++f.stereo.session;break;case 16:++f.reference_generation;break;
            case 17:h.sample.calibration.armspan_metres=1.8;break;case 18:h.sample.simulated();break;
        }
        h.unchanged_failure(ei::Status::cohort_changed);h.baseline();C(!h.next());
        h.sample.press(2);C(h.next());C(h.input.evidence().press_counter==2);
    }
    ++cases;Direct reset;reset.baseline();reset.sample.press();C(reset.next());reset.input.reset();reset.cursor=UINT64_MAX;
    reset.baseline();C(reset.cursor==1);reset.sample.press(3);C(reset.next());C(reset.out.serial==2);
    ++cases;Direct wrong;wrong.baseline();wrong.cursor=5;wrong.sample.advance();wrong.unchanged_failure(ei::Status::cursor_mismatch);
}
void invalids(){
    for(unsigned which=0;which<14;++which){
        ++cases;Direct h;h.baseline();h.sample.press();auto& f=h.sample.f;auto expected=ei::Status::invalid_frame;
        switch(which){
            case 0:h.sample.calibration.armspan_metres=0;expected=ei::Status::invalid_calibration;break;
            case 1:h.sample.calibration.hands_below_head_metres=std::numeric_limits<double>::quiet_NaN();expected=ei::Status::invalid_calibration;break;
            case 2:h.sample.calibration.version=2;expected=ei::Status::invalid_calibration;break;
            case 3:f.controllers.hands[1].squeeze_active=0;expected=ei::Status::invalid_grip;break;
            case 4:f.controllers.hands[1].grip_active=0;expected=ei::Status::invalid_grip;break;
            case 5:f.controllers.hands[1].grip_flags=3;expected=ei::Status::invalid_grip;break;
            case 6:f.controllers.hands[1].last_squeeze_press_flags=3;expected=ei::Status::invalid_press;break;
            case 7:f.controllers.hands[1].last_squeeze_press_sequence=100;expected=ei::Status::invalid_press;break;
            case 8:f.controllers.hands[1].last_squeeze_press_sequence=f.controllers.sequence+1;break;
            case 9:f.reference.orientation={0,0,0,0};break;
            case 10:f.flags&=~wh::gameplay_interactable;break;
            case 11:h.sample.simulated();f.flags&=~wh::allow_sim;break;
            case 12:f.controllers.received_ns=h.sample.ns-100000001;f.controllers.sync_ns=f.controllers.received_ns-1;break;
            case 13:f.controllers.sync_ns=h.sample.ns-100000001;expected=ei::Status::source_gap;break;
        }
        h.unchanged_failure(expected);
    }
    ++cases;Direct none;none.sample.f.weapon={};none.baseline();none.sample.press();C(!none.next());C(none.cursor==0);
    ++cases;Direct badclock;badclock.baseline();badclock.sample.press();badclock.sample.ms=999;badclock.unchanged_failure(ei::Status::invalid_clock);
    ++cases;Direct underflow;underflow.sample.ms=0;underflow.baseline();underflow.sample.press();underflow.unchanged_failure(ei::Status::invalid_clock);
}
void times(){
    ++cases;Direct gap;gap.baseline();gap.sample.advance(101);gap.sample.press();gap.unchanged_failure(ei::Status::source_gap);
    gap.baseline();C(!gap.next());gap.sample.press(3);C(gap.next());C(gap.input.evidence().press_counter==2);
    ++cases;Direct edge;edge.baseline();edge.sample.advance(88);edge.sample.press();C(edge.next());
    C(edge.sample.ns-edge.input.evidence().previous_sync_ns==100000000&&edge.out.created_ms==998);
    ++cases;Direct past;past.baseline();past.sample.advance(88);past.sample.press();++past.sample.ns;
    past.unchanged_failure(ei::Status::source_gap);
    ++cases;Direct stale;stale.baseline();const auto sync=stale.sample.f.controllers.sync_ns;
    stale.sample.advance(101);stale.sample.f.controllers.sync_ns=sync;stale.unchanged_failure(ei::Status::source_gap);
    ++cases;Direct repeat;repeat.baseline();repeat.sample.f.controllers.hands[1].pressed[vr::control_squeeze]=1;
    repeat.unchanged_failure(ei::Status::source_regressed);
    ++cases;Direct backwards;backwards.baseline();backwards.sample.advance();backwards.sample.f.controllers.sync_ns=1;
    backwards.unchanged_failure(ei::Status::source_gap);
    ++cases;Direct counter;counter.sample.press();counter.baseline();counter.sample.advance();
    counter.sample.f.controllers.hands[1].pressed[vr::control_squeeze]=0;
    counter.unchanged_failure(ei::Status::counter_regressed);
}

// Actual Input -> actual Runtime -> actual model. Only game/attachment/current
// callbacks are a synchronous owned-data seam; no mapping or event injection.
struct Integrated {
    Sample sample;ei::Input input;er::Runtime runtime;gw::WeaponEquipmentModel model;gw::EquipmentSnapshot game{};
    unsigned switches{},holds{},sounds{},resets{};bool attached{true},current_ok{true};
    Integrated(){game.world=sample.f.world.address;game.pawn=sample.f.pawn.address;game.current_weapon=sample.f.weapon.address;
        game.weapon_in_slot={0x5000,0x6000,0x7000,0x7800};game.can_switch=true;game.ammo_known.fill(true);
        game.ammo_available.fill(true);game.throwable_count_known=true;game.throwable_count=2;}
    static Integrated& h(void* p){return *static_cast<Integrated*>(p);}
    static std::uint64_t clock_ms(void* p) noexcept {return h(p).sample.ms;}
    static std::int64_t clock_ns(void* p) noexcept {return h(p).sample.ns;}
    static bool current(void* p,wh::Frame const& f) noexcept {return h(p).current_ok&&!std::memcmp(&f,&h(p).sample.f,sizeof(f));}
    static bool next(void* p,wh::Frame const& f,std::uint64_t& cursor,ei::Event& out) noexcept {
        auto& x=h(p);return x.input.next(f,x.sample.calibration,x.sample.ns,x.sample.ms,cursor,out);}
    static bool inventory(void* p,wh::Frame const&,gw::EquipmentSnapshot& out) noexcept {out=h(p).game;return true;}
    static bool is_attached(void* p,wh::Frame const&,gw::EquipmentSnapshot const&) noexcept {return h(p).attached;}
    static bool identify(void*,std::uint64_t address,wh::ObjectIdentity& out) noexcept {out=Sample::id(address);return true;}
    static bool forward(void* p,wh::Frame const&,gw::EquipmentSnapshot const&,unsigned) noexcept {++h(p).switches;return true;}
    static bool sound(void* p,wh::Frame const&,gw::EquipmentSnapshot const&,unsigned) noexcept {++h(p).sounds;return true;}
    static bool hold(void* p,wh::Frame const&,gw::EquipmentSnapshot const&) noexcept {++h(p).holds;return h(p).model.held();}
    static void clear_hold(void*) noexcept {}
    static void reset_input(void* p) noexcept {++h(p).resets;h(p).input.reset();}
    er::Backend backend(){return {this,clock_ms,clock_ns,current,next,inventory,is_attached,identify,forward,sound,hold,clear_hold,reset_input};}
    er::Outcome tick(){return runtime.tick(sample.f,backend(),model);}
    void baseline(){C(tick().status==er::Status::baseline);C(runtime.state().baseline_ready&&runtime.state().cursor==0);}
};
void integration(){
    ++cases;Integrated h;h.sample.simulated();h.baseline();
    h.sample.press(4);C(h.tick().status==er::Status::idle);C(h.runtime.state().consumed_press==0);
    h.sample.press(4);C(h.tick().status==er::Status::idle);
    h.sample.press(2);C(h.tick().status==er::Status::switch_requested);C(h.switches==1&&h.holds==0);
    C(h.runtime.state().consumed_press==3&&h.model.switch_pending());C(h.tick().status==er::Status::switch_pending);C(h.switches==1);
    h.sample.advance();h.game.current_weapon=0x7000;h.sample.f.weapon=Sample::id(0x7000);h.attached=false;
    C(h.tick().status==er::Status::attachment_unavailable);C(h.holds==0&&h.switches==1);
    h.sample.advance();h.attached=true;C(h.tick().status==er::Status::held);C(h.model.held()&&h.holds==1&&h.switches==1);

    ++cases;Integrated gap;gap.baseline();gap.sample.advance(101);gap.sample.press(2);
    C(gap.tick().status==er::Status::baseline_waiting);C(!gap.runtime.state().baseline_ready&&gap.switches==0);
    C(gap.tick().status==er::Status::baseline);C(gap.runtime.state().consumed_press==1);
    gap.sample.press(2);C(gap.tick().status==er::Status::switch_requested);C(gap.switches==1);
    gap.sample.advance(101);C(gap.tick().status==er::Status::baseline_waiting);
    C(gap.runtime.state().pending&&gap.model.switch_pending()&&gap.switches==1);
    C(gap.tick().status==er::Status::baseline);C(gap.runtime.state().pending&&gap.switches==1);

    ++cases;Integrated lost;lost.baseline();lost.sample.press(2);lost.current_ok=false;
    C(lost.tick().status==er::Status::unavailable);C(lost.switches==0&&!lost.runtime.state().baseline_ready);
    lost.current_ok=true;C(lost.tick().status==er::Status::baseline);lost.sample.advance();C(lost.tick().status==er::Status::idle);
    lost.sample.press(3);C(lost.tick().status==er::Status::switch_requested);C(lost.switches==1);

    ++cases;Integrated missing;missing.sample.calibration={};C(missing.tick().status==er::Status::baseline_waiting);
    missing.sample.press();C(missing.tick().status==er::Status::baseline_waiting);C(missing.switches==0);
    missing.sample.calibration={1,1.7,-.15};C(missing.tick().status==er::Status::baseline);
    missing.sample.press(0);C(missing.tick().status==er::Status::held);C(missing.holds==1&&missing.switches==0);
}
}
int main(){routes();boundaries();invalids();times();integration();
    std::printf("NativeEquipmentInputTest: %u cases / %u checks passed; actual Native Frame input/router + Runtime/model, controlled backend seam; no Game/XR/mapping/engine acceptance\n",cases,checks);
}
