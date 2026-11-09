// Actual HB event adapter + actual finite ledger/transaction/cancel adapter.
// The common dispatcher, full retail certificate, metadata and engine are
// bounded fakes here. No claim about native ABI, game output or physical input.
#include "../vr-common/weapon_support_grip.h"
#define HB_RUNTIME_FIXTURE_ONLY
#include "weapon_hammerburst_burst_runtime_test.cpp"
#include "../vr-common/weapon_hammerburst_refire.h"
#include "weapon_hammerburst_burst_runtime.h"
#include <atomic>
FILE* log_file{};
bool inside{};
std::atomic<bool> enabled{true};
namespace event_fixture {
bool domain{true},loop{true},vm{true},body{true},unchanged{true},ready{true},identity{true},original_success{true},outer_proof{true};
unsigned invocations{};std::uint64_t shot{99};
std::array<unsigned char,32> first_body{};
std::array<unsigned char,24> ammo_body{};
}
namespace nativeequipment {bool domain_ready(){return event_fixture::domain;}}
namespace vrfire {enum :unsigned {off=0,running=1,stopping=2,stopped=3,fault=4};std::atomic<unsigned> mode{running};}
namespace vrequip {struct Model {
    std::uint64_t hold_generation(){return weaponhand::held_binding.generation;}
    bool animation_owns_hands(){return false;}bool switch_pending(){return false;}
}model;}
namespace lancerreload_runtime {bool blocks_fire(gears_weapon_hand::ObjectIdentity const&){return false;}}
namespace lancerreload_inventory {
struct Counts {int capacity{},used{};bool operator==(Counts const& b)const{return capacity==b.capacity&&used==b.used;}};
bool query(uptr,Counts& out){out={int(fixture::model.capacity),int(fixture::model.ammo_used)};return true;}
}
namespace weapon_metadata_parse {struct TickScope {TickScope(){}~TickScope(){}};}
namespace unified_weapon_fire {
struct Muzzle {gears_weapon_hand::Frame source{};std::int64_t render_acquisition_ns{},sampled_ns{};}muzzle_sample;
bool manual_profile(){return true;}
bool ready_for(gears_weapon_hand::Frame const&,std::int64_t){return event_fixture::ready;}
}
namespace lancerrefire {
namespace policy=gears_lancer_refire_loop;
struct Cohort {weaponhand::HoldBinding held{};bool known{};};
Cohort press;bool press_dispatch{},delivering{},pending_validated{},certified{};
Cohort cohort(gears_weapon_hand::Frame const&){return {weaponhand::held_binding,true};}
struct Script {Array array{};std::array<unsigned char,512> bytes{};unsigned bound{256};};
struct Deferred {
    policy::DeferredEvent event{};Cohort cohort{};gears_weapon_hand::ObjectIdentity function{};
    Script body{};std::array<unsigned char,8> parameters{};unsigned parameter_bytes{};uptr refire{};
}pending_fire;
gears_weapon_hand::ObjectIdentity identity(gripdamage_metadata::Reader&,unsigned slot){return {fixture::address(slot),0,0,slot,slot,0,1};}
bool script(gripdamage_metadata::Reader&,uptr fn,Script& out,unsigned bound=256){
    using namespace fixture;if(!event_fixture::body)return false;
    out={};out.array.data=0x30000;
    if(fn==address(first_fn)){out.array.count=32;std::memcpy(out.bytes.data(),event_fixture::first_body.data(),32);}
    else if(fn==address(hb_ammo)){out.array.count=24;std::memcpy(out.bytes.data(),event_fixture::ammo_body.data(),24);}
    else if(fn==address(hb_timer)){out.array.count=1;out.bytes[0]=0x53;}
    else if(fn==address(outer_fn)){out.array.count=271;}
    else return false;out.bound=bound;out.array.capacity=out.array.count;return unsigned(out.array.count)<=bound;
}
bool unchanged(gripdamage_metadata::Reader&,uptr,Script const&){return event_fixture::unchanged;}
bool original_loop(gripdamage_metadata::Reader&,uptr,uptr,unsigned&){return event_fixture::loop;}
namespace hammerburst_refire {
bool original_loop(gripdamage_metadata::Reader&,uptr,uptr,unsigned&){return event_fixture::loop;}
bool vm_shape(gripdamage_metadata::Reader&,uptr,unsigned,unsigned){return event_fixture::vm;}
}
bool queue(gripdamage_metadata::Reader& r,policy::EventKind kind,uptr fn,void const* params,unsigned bytes,
           uptr refire,Cohort const* intent,unsigned bound=256){
    if(!intent||!intent->known||bytes>8||(bytes&&!params))return false;
    if(!pending_fire.event.empty())return pending_fire.function.address==fn&&pending_fire.event.capture(kind,fixture::model.now);
    unsigned slot{};if(!r.path(fn,slot)||!script(r,fn,pending_fire.body,bound)||!pending_fire.event.capture(kind,fixture::model.now))return false;
    pending_fire.function=identity(r,slot);pending_fire.cohort=*intent;pending_fire.refire=refire;pending_fire.parameter_bytes=bytes;
    if(bytes)std::memcpy(pending_fire.parameters.data(),params,bytes);certified=true;return true;
}
bool pending_identity(gripdamage_metadata::Reader&){return event_fixture::identity;}
bool invoke_original(uptr,uptr function,unsigned,unsigned char const*){
    using namespace fixture;auto& m=model;++event_fixture::invocations;
    if(function==address(hb_timer)&&(m.burst_count==6||m.ammo_used==m.capacity))m.state={burst,0};
    else{
        if(function==address(outer_fn))m.burst_count=0;
        ++m.burst_count;++m.ammo_used;
        auto const& sample=unified_weapon_fire::muzzle_sample;
        assert(hammerburst_burst_event::source_allowed(sample.source,sample.render_acquisition_ns,sample.sampled_ns,m.now));
        hammerburst_burst_event::trace(sample.source,++event_fixture::shot,sample.render_acquisition_ns,sample.sampled_ns,m.now,true);
    }
    return event_fixture::original_success;
}
}
namespace hammerburst_first_contract {
unsigned stage{};
bool prove(gripdamage_metadata::Reader& r,uptr,uptr fn,lancerrefire::Script& body){
    return event_fixture::outer_proof&&lancerrefire::script(r,fn,body,512);}
}
#include "weapon_hammerburst_burst_event.inl"
namespace adapter=hammerburst_burst_event;
void reset_event(){
    reset_bridge();using namespace fixture;
    event_fixture::domain=event_fixture::loop=event_fixture::vm=event_fixture::body=event_fixture::unchanged=
        event_fixture::ready=event_fixture::identity=event_fixture::original_success=true;
    event_fixture::outer_proof=true;
    event_fixture::invocations=0;event_fixture::shot=99;enabled.store(true);vrfire::mode.store(vrfire::running);
    lancerrefire::pending_fire={};lancerrefire::press={};lancerrefire::press_dispatch=lancerrefire::delivering=false;
    lancerrefire::certified=lancerrefire::pending_validated=false;
    adapter::event={};adapter::event_serial=0;adapter::original_press={};adapter::validated=false;
    // Independent synthetic runtime first/ammunition expressions: names and
    // object operands are fake authenticated identities, all tokens are real.
    auto& first=event_fixture::first_body;first={};std::size_t at{};
    first[at++]=0x1b;Name ammo_name{hb_ammo,0};std::memcpy(first.data()+at,&ammo_name,8);at+=8;
    first[at++]=0x16;first[at++]=0x1b;Name timing_name{timing_fn,0};std::memcpy(first.data()+at,&timing_name,8);at+=8;
    first[at++]=0x01;const auto mode_property=address(hb_mode);std::memcpy(first.data()+at,&mode_property,8);at+=8;
    first[at++]=0x16;first[at++]=0x04;first[at++]=0x0b;first[at++]=0x53;assert(at==first.size());
    auto& ammo=event_fixture::ammo_body;ammo={};ammo[0]=0x8b;ammo[1]=1;
    const auto count_property=address(hb_count),super_function=address(super_ammo);
    std::memcpy(ammo.data()+2,&count_property,8);ammo[10]=0x16;ammo[11]=0x1c;
    std::memcpy(ammo.data()+12,&super_function,8);ammo[20]=0x16;ammo[21]=4;ammo[22]=0x0b;ammo[23]=0x53;
    for(auto id:{&weaponhand::held_binding.world,&weaponhand::held_binding.controller,&weaponhand::held_binding.pawn,&weaponhand::held_binding.weapon}){
        id->known=1;if(!id->klass)id->klass=0x9000;
    }
}
gears_weapon_hand::Frame active_frame(){
    auto f=frame();f.flags=7;f.now_ns=fixture::model.now-10000000;f.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
    f.mesh={0xaa000,0x8000,0x9000,4,4,0,1};
    auto& s=f.stereo;s.sequence=f.key.frame;s.predicted_time=100000;s.head_flags=s.view_flags=15;
    s.focused=s.should_render=s.visible=1;s.head={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.7,.7,.7,-.7};}
    auto& input=f.controllers;input.session=s.session;input.sequence=s.sequence;input.space_epoch=s.space_epoch;
    input.predicted_time=s.predicted_time;input.received_ns=fixture::model.now-9000000;input.focused=1;
    for(auto& h:input.hands){h.grip_active=h.aim_active=1;h.grip_flags=h.aim_flags=15;}
    assert(gears_weapon_hand::frame_active(f,fixture::model.now));return f;
}
void muzzle(gears_weapon_hand::Frame const& f){
    unified_weapon_fire::muzzle_sample={f,fixture::model.now-10000000,fixture::model.now-1000000};
}
void captured_first(gears_weapon_hand::Frame const& f){
    assert(adapter::prepare_press(f));assert(adapter::owned_press(f));
    fixture::model.r2=true;fixture::model.bytes[0]=1;
    lancerrefire::press_dispatch=true;Name previous{};
    assert(adapter::capture_initial(f.weapon.address,fixture::address(fixture::first_fn),&previous));
    lancerrefire::press_dispatch=false;
}
void actual_first(gears_weapon_hand::Frame const& f){
    captured_first(f);assert(adapter::service(f.controller.address));muzzle(f);adapter::deliver(f);
    assert(fixture::model.burst_count==1&&bridge::completion.count()==1);
}
int main(){
    using namespace fixture;unsigned checks{};
    // One inherited original first event, never HB's outer script or an
    // unscoped InstantFire, then five real-UP continuation events.
    {reset_event();auto f=active_frame();assert(adapter::prepare_press(f));
        assert(!adapter::source_allowed(f,model.now,model.now,model.now));
        assert(!adapter::capture_initial(f.weapon.address,address(hb_ammo),nullptr));checks+=3;
        actual_first(f);model.owned_press=false;model.r2=false;model.bytes[0]=0;f.controllers.hands[1].trigger=0;
        ++f.controllers.hands[1].released[gears_vr::control_trigger];
        assert(adapter::release_completed(&f));assert(bridge::firing_debt);checks+=2;
        for(unsigned before=1;before<6;++before){
            model.now+=60000000;f.now_ns=f.stereo.received_ns=f.controllers.sync_ns=model.now-10000000;
            f.controllers.received_ns=model.now-9000000;++f.key.frame;++f.stereo.sequence;++f.controllers.sequence;
            assert(gears_weapon_hand::frame_active(f,model.now));
            assert(adapter::defer(f.weapon.address,address(hb_timer)));const auto serial=adapter::event.serial;
            assert(adapter::defer(f.weapon.address,address(hb_timer)));assert(adapter::event.serial==serial);
            assert(adapter::service(f.controller.address));muzzle(f);adapter::deliver(f);adapter::deliver(f);
            assert(model.burst_count==before+1);assert(event_fixture::invocations==before+1);assert(!model.owned_press);checks+=8;
        }
        assert(adapter::defer(f.weapon.address,address(hb_timer)));
        assert(model.burst_count==6&&model.state.index==burst&&model.ammo_used==6);
        assert(!adapter::cleanup_pending());assert(event_fixture::invocations==7);checks+=4;
    }
    // An original terminal event after ammo exhaustion consumes NO extra ray
    // or inventory, even when its valid burst ended before six.
    {reset_event();auto f=active_frame();actual_first(f);model.ammo_used=model.capacity;
        assert(adapter::defer(f.weapon.address,address(hb_timer)));assert(adapter::service(f.controller.address));
        muzzle(f);adapter::deliver(f);assert(model.burst_count==1&&model.ammo_used==model.capacity);
        assert(!adapter::cleanup_pending());assert(model.state.index==burst);checks+=5;}
    // Stale pending first debt cancels, never installs a late firing loop or
    // invokes FireAmmunition on a later fresh source after normal release.
    {reset_event();auto f=active_frame();captured_first(f);model.now+=250000001;
        assert(adapter::requires_release());assert(adapter::service(f.controller.address));
        assert(adapter::cleanup_pending());model.r2=false;model.owned_press=false;model.bytes[0]=0;
        assert(adapter::release_completed(nullptr));assert(!adapter::cleanup_pending());
        assert(lancerrefire::pending_fire.event.empty());assert(!event_fixture::invocations);checks+=7;}
    for(unsigned mutation=0;mutation<5;++mutation){
        reset_event();auto f=active_frame();assert(adapter::prepare_press(f));assert(adapter::owned_press(f));
        if(mutation==0)event_fixture::first_body[0]^=1;
        if(mutation==1)event_fixture::ammo_body[0]^=1;
        if(mutation==2)event_fixture::vm=false;
        if(mutation==3)event_fixture::unchanged=false;
        if(mutation==4)gripdamage_metadata::Reader::properties[previous].flags=0;
        lancerrefire::press_dispatch=true;Name p{};
        assert(adapter::capture_initial(f.weapon.address,address(first_fn),&p));lancerrefire::press_dispatch=false;
        assert(adapter::requires_release());assert(!event_fixture::invocations);
        model.r2=false;model.owned_press=false;model.bytes[0]=0;assert(adapter::release_completed(nullptr));checks+=6;
    }
    // Normal stop/lost source must retain this same existing owner until R2
    // and every PendingFire entry are UP, then perform original state cleanup.
    {reset_event();auto f=active_frame();actual_first(f);model.r2=true;model.bytes[0]=1;
        adapter::after_tick(nullptr);assert(adapter::requires_release());assert(adapter::cleanup_pending());
        assert(!model.go_calls);model.r2=false;model.owned_press=false;assert(!adapter::release_completed(nullptr));
        model.bytes[0]=0;assert(adapter::release_completed(nullptr));assert(model.go_calls==1);checks+=6;}
    {reset_event();auto f=active_frame();actual_first(f);event_fixture::original_success=false;
        assert(adapter::defer(f.weapon.address,address(hb_timer)));assert(adapter::service(f.controller.address));
        muzzle(f);adapter::deliver(f);assert(adapter::requires_release());
        assert(vrfire::mode.load()==vrfire::running); // Existing GT cleanup owner stays alive.
        model.r2=false;model.owned_press=false;model.bytes[0]=0;assert(adapter::release_completed(nullptr));
        assert(!adapter::cleanup_pending());checks+=6;}
    {reset_event();auto f=active_frame();assert(adapter::prepare_press(f));assert(adapter::owned_press(f));
        model.objects=false;Name p{};assert(!adapter::capture_initial(f.weapon.address,address(first_fn),&p));
        assert(!event_fixture::invocations);checks+=4;}
    {reset_event();model.burst_count=6;auto f=active_frame();assert(adapter::prepare_press(f));assert(adapter::owned_press(f));
        model.r2=true;model.bytes[0]=1;lancerrefire::press_dispatch=true;
        assert(adapter::capture_initial(f.weapon.address,address(outer_fn),nullptr));lancerrefire::press_dispatch=false;
        assert(lancerrefire::pending_fire.body.array.count==271&&lancerrefire::pending_fire.parameter_bytes==0);
        assert(adapter::service(f.controller.address));muzzle(f);adapter::deliver(f);
        assert(model.burst_count==1&&bridge::completion.count()==1&&event_fixture::invocations==1);
        model.r2=false;model.bytes[0]=0;assert(adapter::release_completed(nullptr));checks+=7;}
    {reset_event();auto f=active_frame();actual_first(f);f.controllers.hands[1].grip_active=0;
        assert(gears_weapon_hand::frame_active(f,model.now)); // Envelope can still be active.
        adapter::after_tick(&f);assert(adapter::requires_release());
        assert(!adapter::source_allowed(f,model.now,model.now,model.now));
        model.r2=false;model.bytes[0]=0;model.owned_press=false;assert(adapter::release_completed(nullptr));checks+=4;}
    std::printf("HB actual event adapter: %u original-event/scope/UP/expiry/cleanup fake checks passed; common queue, engine and certificate are fakes\n",checks);
}
