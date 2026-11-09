// Adapter to lancerrefire's ONE existing DeferredEvent. No timer, hook, input
// ledger, VM frame or independent dispatcher is introduced for HB.
#include "weapon_hammerburst_burst_runtime.h"
namespace hammerburst_burst_event {
namespace tx=hammerburst_burst_runtime;
namespace common=lancerrefire;
namespace meta=gripdamage_metadata;
namespace hand=gears_weapon_hand;
common::Cohort original_press;
struct Pending {
    std::uint64_t serial{};std::int64_t captured{};
    hand::ObjectIdentity function{};common::policy::EventKind kind{};
} event;
std::uint64_t event_serial{};
bool validated{};
unsigned service_stage{},delivery_stage{};
void report_first_wait(wchar_t const* phase) noexcept {
    static unsigned rows{};
    if(!log_file||rows>=12)return;
    ++rows;auto const& source=tx::attempted_source;const auto now=weaponhand::clock_ns(nullptr);
    fwprintf(log_file,L"native_hammerburst_first_wait phase=%ls service=%u delivery=%u enter=%u validated=%u first_debt=%u cancel=%u source_frame=%llu decision_ns=%lld sync_ns=%lld stereo_ns=%lld render_ns=%lld sampled_ns=%lld event_captured_ns=%lld observed_ns=%lld\n",
        phase,service_stage,delivery_stage,tx::enter_stage,unsigned(validated),unsigned(tx::first_debt),
        unsigned(tx::cancel_requested),source.frame,source.now,source.sync,source.stereo,source.render,source.sampled,event.captured,now);
    fflush(log_file);
}
bool targets(std::uint64_t weapon) noexcept {
    return tx::certificate.known&&weapon==tx::certificate.held.weapon.address&&
        common::same_hold(tx::certificate.held,weaponhand::held_binding);
}
bool outstanding() noexcept {
    return tx::first_debt||tx::firing_debt||tx::cancel_requested||
        tx::completion.state()!=tx::policy::State::idle;
}
bool cleanup_pending() noexcept {return outstanding();}
bool owns_pending() noexcept {
    auto const& queued_event=common::pending_fire;
    return event.serial&&!queued_event.event.empty()&&event.kind==queued_event.event.kind()&&
        hand::same(event.function,queued_event.function)&&targets(queued_event.cohort.held.weapon.address)&&
        common::same_hold(queued_event.cohort.held,tx::certificate.held);
}
bool requires_release() noexcept {
    return outstanding()&&(tx::cancel_requested||tx::completion.needs_cancel()||
        (owns_pending()&&common::pending_fire.event.release_required(weaponhand::clock_ns(nullptr))));
}
void clear_pending() noexcept {
    if(owns_pending()){common::pending_fire.event.reset();common::pending_validated=false;}
    event={};validated=false;
}
bool prepare_press(hand::Frame const& f) noexcept {
    if(outstanding()&&!release_completed(nullptr))return false;
    const auto state=playground::field(f.weapon.klass,L"WeaponFiring",L"State");
    const auto timer=state?playground::field(state,L"RefireCheckTimer",L"Function"):0;
    const bool ready=timer&&tx::prime(f,timer);
    static unsigned preflight_logs{};
    if(!ready&&log_file&&preflight_logs++<4){fwprintf(log_file,L"native_hammerburst_press_not_ready stage=%u loop=%u frame=%llu sync_age_ns=%lld\n",
        hammerburst_burst_contract::stage,hammerburst_burst_contract::loop_reason,f.key.frame,
        weaponhand::clock_ns(nullptr)-f.controllers.sync_ns);fflush(log_file);}
    return ready;
}
bool owned_press(hand::Frame const& f) noexcept {
    if(!tx::press(f))return false;
    original_press=common::cohort(f);return original_press.known;
}
void press_failed() noexcept {tx::revoke();validated=false;}
bool right_tracked(hand::Frame const& f) noexcept {
    auto const& h=f.controllers.hands[1];return gears_vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active);
}
bool need_source(hand::Frame const& f) noexcept {
    if(!outstanding())return false;
    if(!right_tracked(f)){tx::revoke();return false;}
    return tx::need_source(f);
}
bool source_allowed(hand::Frame const& f,std::int64_t render,std::int64_t sampled,std::int64_t now) noexcept {
    return right_tracked(f)&&tx::permits(tx::source(f,render,sampled,now));
}
void trace(hand::Frame const& f,std::uint64_t shot,std::int64_t render,std::int64_t sampled,
           std::int64_t now,bool consistent) noexcept {
    tx::trace(tx::source(f,render,sampled,now),shot,consistent&&right_tracked(f));
}
bool queue(meta::Reader& r,common::policy::EventKind kind,uptr function,void const* parameters,unsigned bytes,unsigned script_bound=256) noexcept {
    if(!original_press.known)return false;
    const bool empty=common::pending_fire.event.empty();
    if(!empty&&!owns_pending())return false;
    const auto captured=weaponhand::clock_ns(nullptr);
    if(empty&&event_serial==(std::numeric_limits<std::uint64_t>::max)()){tx::revoke();return false;}
    if(!common::queue(r,kind,function,parameters,bytes,tx::certificate.timer.address,&original_press,script_bound))return false;
    if(empty){event={++event_serial,captured,common::pending_fire.function,kind};}
    return true;
}
// Parent body proof also validates the original final super target used by
// HB's whole outer entry. A VM super call need not cross ProcessEvent.
bool first_proof(meta::Reader& r,uptr function,unsigned& bytes) noexcept {
    auto const& c=tx::certificate;meta::Structure structure{};std::uint16_t count{},result{};
    if(!weaponhand::object(r,c.first)||function!=c.first.address||!weaponhand::object(r,c.engine_timer)||
       !weaponhand::object(r,c.ammunition)||!weaponhand::object(r,c.super_ammunition)||!weaponhand::object(r,c.timing)||
       !r.structure(function,structure)||!r.at(function,0x106,count)||(count!=0&&count!=8)||
       !r.at(function,0x108,result)||result!=0xffff||
       !common::hammerburst_refire::vm_shape(r,function,count,0xffff))return false;
    meta::Property previous{},mode{},burst{};
    const auto parameter=playground::field(function,L"PreviousStateName");
    if(!parameter||!r.property(parameter,previous)||!r.named(previous.identity,L"PreviousStateName")||
       r.result.identities[previous.identity].outer!=function||!weaponhand::core_type(r,previous.type,L"NameProperty")||
       previous.dimension!=1||previous.element_bytes!=8||previous.offset||!meta::property_fits(previous,structure.extent)||
       (count==8&&(previous.flags&0x580)!=0x80)||(count==0&&(previous.flags&0x580))||
       structure.children!=parameter||previous.next||
       !r.property(c.mode_property,mode)||!gears_hammerburst_property_cache::same(r,mode,c.mode,c.mode_descriptor,c.mode_type)||
       !r.named(mode.identity,L"CurrentFireMode")||
       !weaponhand::core_type(r,mode.type,L"ByteProperty")||mode.dimension!=1||mode.element_bytes!=1||
       !r.property(c.count_property,burst)||!gears_hammerburst_property_cache::same(r,burst,c.count,c.count_descriptor,c.count_type))return false;
    common::Script body{},ammunition{};unsigned reason{};
    if(!common::original_loop(r,c.held.weapon.address,c.engine_timer.address,reason)||
       !common::hammerburst_refire::vm_shape(r,c.ammunition.address,0,0xffff)||
       !common::script(r,function,body)||!common::script(r,c.ammunition.address,ammunition)||
       !gears_hammerburst_refire::ammunition(ammunition.bytes.data(),std::size_t(ammunition.array.count),
            c.count_property,c.super_ammunition.address)||
       !common::policy::first_shot(body.bytes.data(),std::size_t(body.array.count),
            {c.ammunition.name_index,c.ammunition.name_number},{c.timing.name_index,c.timing.name_number},c.mode_property)||
       !hammerburst_burst_contract::retain(r,previous)||!hammerburst_burst_contract::retain(r,mode)||
       !hammerburst_burst_contract::retain(r,burst)||!unified_weapon_fire::retained_metadata(r)||
       !common::unchanged(r,function,body)||!common::unchanged(r,c.ammunition.address,ammunition))return false;
    bytes=count;return true;
}
bool capture_initial(std::uint64_t object,std::uint64_t function,void const* parameters) noexcept {
    const bool outer=function==tx::certificate.outer_first.address;
    if(!targets(object)||(!outer&&function!=tx::certificate.first.address)||common::delivering||
       !weaponhand::current_gt(nullptr))return false;
    const bool owned=common::press_dispatch||vrfire::input.owns_press();
    if(!owned&&!outstanding())return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};unsigned bytes{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,tx::certificate.held.weapon)){
        tx::revoke();return false; // Never consume an unknown/recycled receiver.
    }
    common::Script outer_body{};
    const bool proved=outer?
        (weaponhand::object(r,tx::certificate.outer_first)&&
         hammerburst_first_contract::prove(r,object,function,outer_body)&&first_proof(r,tx::certificate.first.address,bytes)):
        first_proof(r,function,bytes);
    if(!owned||vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||
       !enabled.load(std::memory_order_acquire)||!nativeequipment::domain_ready()||
       !unified_weapon_fire::manual_profile()||!proved){
        // Cleanup debt grants no shot. The original HB outer entry has already
        // entered Firing, even though this inherited first body was not run.
        tx::first_debt=true;tx::firing_debt=true;
        static unsigned first_rejections{};
        if(log_file&&first_rejections++<4){fwprintf(log_file,L"native_hammerburst_first_proof_rejected outer=%u stage=%u bytes=%d parent_parameters=%u path_node=%u path_gate=%u extent=%u parameters=%u result=%u\n",
            unsigned(outer),hammerburst_first_contract::stage,outer_body.array.count,bytes,
            hammerburst_first_contract::path_node,hammerburst_first_contract::path_gate,
            hammerburst_first_contract::observed_extent,unsigned(hammerburst_first_contract::observed_parameters),
            unsigned(hammerburst_first_contract::observed_result));fflush(log_file);}
        tx::revoke();return true; // Authenticated owned first target cannot consume ammo on failed proof.
    }
    // A completed prior six-shot event may still await its normal timer's
    // state handoff; only this new authentic first entry can start another.
    if(owns_pending()&&common::pending_fire.event.kind()==common::policy::EventKind::refire)clear_pending();
    const bool first_was_owed=tx::first_debt;
    if(!first_was_owed&&!tx::capture_first()){tx::revoke();return true;}
    // The authenticated outer BeginState has one original eight-byte Name
    // parameter. Preserve that actual caller payload in the existing queue;
    // the original VM still calls its inherited BeginState itself.
    if(!queue(r,common::policy::EventKind::first,function,parameters,outer?8:bytes,outer?512:256))tx::revoke();
    static unsigned first_captures{};
    if(log_file&&first_captures++<6){fwprintf(log_file,L"native_hammerburst_first_captured outer=%u event=%llu bytes=%d retained=%u\n",
        unsigned(outer),event.serial,outer?outer_body.array.count:common::pending_fire.body.array.count,unsigned(owns_pending()));fflush(log_file);}
    return true;
}
bool release_completed(hand::Frame const* f) noexcept {
    if(!outstanding())return true;
    validated=false;common::press.known=false;common::certified=false;common::pending_validated=false;
    // Only a current real UP can preserve a committed finite burst. A raw
    // release callback from source loss/stop carries no such continuation.
    vrfire::lr::Game game{};
    if(f&&vrfire::mode.load(std::memory_order_acquire)==vrfire::running&&
       tx::completion.state()==tx::policy::State::committed&&
       f->controllers.hands[1].trigger_active==1&&f->controllers.hands[1].trigger<=.35f&&
       hand::frame_active(*f,weaponhand::clock_ns(nullptr))&&right_tracked(*f)&&
       vrfire::read_game(vrfire::input.actors(),f,game)&&game.known&&game.r2_known&&!game.r2_held&&
       !game.busy&&!game.animation&&!lancerreload_runtime::blocks_fire(f->weapon)&&nativeequipment::current(nullptr,*f)&&
       tx::need_source(*f))return true;
    tx::revoke();
    if(!tx::cancel())return false;
    // complete() invalidates the certificate, so clear the event using the
    // captured original owner rather than re-evaluating targets afterwards.
    if(event.serial&&!common::pending_fire.event.empty()&&hand::same(event.function,common::pending_fire.function)&&
       common::same_hold(original_press.held,common::pending_fire.cohort.held))common::pending_fire.event.reset();
    event={};original_press={};return true;
}
void after_tick(hand::Frame const* f) noexcept {
    if(!outstanding())return;
    if(!f||vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||tx::cancel_requested||
       !targets(f->weapon.address)||!right_tracked(*f)||!nativeequipment::current(nullptr,*f)){
        tx::revoke();release_completed(nullptr);return;
    }
    if(tx::completion.state()==tx::policy::State::committed&&!tx::need_source(*f))release_completed(nullptr);
}
bool context(meta::Reader& r) noexcept {
    auto const& held=tx::certificate.held;vrfire::lr::Game game{};
    weaponhand::BoolField putting_down{};
    return targets(held.weapon.address)&&enabled.load(std::memory_order_acquire)&&
        weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready()&&unified_weapon_fire::manual_profile()&&
        weaponhand::client.registration_alive(held.native_cookie,held.native_started)&&
        weaponhand::object(r,held.world)&&weaponhand::object(r,held.controller)&&
        weaponhand::object(r,held.pawn)&&weaponhand::object(r,held.weapon)&&
        vrfire::read_game(vrfire::input.actors(),nullptr,game)&&game.known&&game.r2_known&&game.lancer&&
        hand::same(game.actors.world,held.world)&&hand::same(game.actors.controller,held.controller)&&
        hand::same(game.actors.pawn,held.pawn)&&hand::same(game.actors.weapon,held.weapon)&&
        !game.busy&&!game.animation&&!lancerreload_runtime::blocks_fire(held.weapon)&&
        held.generation==vrequip::model.hold_generation()&&!vrequip::model.animation_owns_hands()&&!vrequip::model.switch_pending()&&
        weaponhand::boolean(r,held.weapon.address,L"bWeaponPutDown",putting_down)&&!(putting_down.bits&putting_down.mask);
}
bool state_firing(meta::Reader& r) noexcept {
    Name current{};meta::NameText text{};
    return common::state_name(r,tx::certificate.held.weapon.address,current)&&
        meta::fresh_name(r.result.names,current.index,text)&&meta::exact_text(text,L"WeaponFiring");
}
bool terminal_proof(unsigned original_count,bool require_empty) noexcept {
    auto const& c=tx::certificate;
    hammerburst_burst_contract::Certificate fresh_certificate{};
    if(!hammerburst_burst_contract::prepare(c.held,c.timer.address,fresh_certificate)||
       !hand::same(c.should,fresh_certificate.should)||
       !hand::same(c.count_descriptor,fresh_certificate.count_descriptor)||!hand::same(c.count_type,fresh_certificate.count_type)||
       !hand::same(c.limit_descriptor,fresh_certificate.limit_descriptor)||!hand::same(c.limit_type,fresh_certificate.limit_type)||
       !gears_hammerburst_property_cache::layout(c.count,fresh_certificate.count)||
       !gears_hammerburst_property_cache::layout(c.limit,fresh_certificate.limit))return false;
    lancerreload_inventory::Counts before{},after{};
    if(require_empty&&(!lancerreload_inventory::query(c.held.weapon.address,before)||before.used!=before.capacity))return false;
    playground::Args query(c.held.weapon.address,L"ShouldRefire");
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::Property result{};unsigned mask{},again{},count{},limit{};
    const auto returned=query.prop(L"ReturnValue");
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,c.should)||query.r.function!=c.should.address||
       !query.valid||query.r.size!=4||!returned||!r.property(returned,result)||
       r.result.identities[result.identity].outer!=c.should.address||!weaponhand::core_type(r,result.type,L"BoolProperty")||
       result.dimension!=1||result.element_bytes!=4||result.offset||(result.flags&0x580)!=0x580||
       !r.at(returned,0xd0,mask)||!mask||(mask&(mask-1))||!hammerburst_burst_contract::retain(r,result)||
       !query.run()||(query.output<unsigned>()&mask)||!r.at(returned,0xd0,again)||again!=mask||
       !unified_weapon_fire::retained_metadata(r)||!tx::checked_counts(count,limit)||count!=original_count||limit!=6)return false;
    return !require_empty||(lancerreload_inventory::query(c.held.weapon.address,after)&&before==after&&after.used==after.capacity);
}
bool ended(meta::Reader& r) noexcept {
    Name current{};meta::NameText text{};
    return common::state_name(r,tx::certificate.held.weapon.address,current)&&
        meta::fresh_name(r.result.names,current.index,text)&&meta::exact_text(text,L"BurstDelaying");
}
bool defer(std::uint64_t object,std::uint64_t function) noexcept {
    if(!targets(object)||function!=tx::certificate.timer.address||!outstanding()||common::delivering||
       !weaponhand::current_gt(nullptr))return false;
    struct Recursion {bool before{inside};Recursion(){inside=true;}~Recursion(){inside=before;}} recursion;
    weapon_metadata_parse::TickScope parsing;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};unsigned reason{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,tx::certificate.held.weapon)||
       !weaponhand::object(r,tx::certificate.timer)){tx::revoke();return false;}
    if(vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||tx::cancel_requested){tx::revoke();return true;}
    if(!context(r)||!state_firing(r)||
       !common::hammerburst_refire::original_loop(r,object,function,reason)){tx::revoke();return true;}
    if(owns_pending()||tx::first_debt)return true; // Coalesce, never renew the old event.
    unsigned count{},limit{};
    if(!tx::checked_counts(count,limit)||count!=tx::completion.count()||limit!=6){tx::revoke();return true;}
    if(count==6&&tx::completion.state()==tx::policy::State::idle){
        // This ACTUAL stock timer cannot fire another shot. Complete its
        // original bWasTargeting/BurstDelaying branch without a pose permit.
        if(!terminal_proof(count,false)){tx::revoke();return true;}
        const bool invoked=common::invoke_original(object,function,0,nullptr);
        unsigned after_count{},after_limit{};
        if(!invoked||!ended(r)||!tx::checked_counts(after_count,after_limit)||after_count!=count||after_limit!=limit||
           !tx::completion.original_ended(count,true,true,false)){tx::revoke();return true;}
        tx::firing_debt=false;return true;
    }
    if(tx::completion.state()!=tx::policy::State::committed||
       !queue(r,common::policy::EventKind::refire,function,nullptr,0))tx::revoke();
    return true;
}
bool service(std::uint64_t controller) noexcept {
    struct Report {bool first{tx::first_debt};~Report(){if(first)report_first_wait(L"service");}} report;
    service_stage=1;
    if(!outstanding()&&!owns_pending())return false;
    service_stage=2;
    if(!tx::certificate.known||controller!=tx::certificate.held.controller.address||
       !weaponhand::current_gt(nullptr)||common::delivering)return false;
    validated=false;
    service_stage=3;
    if(tx::cancel_requested||vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||
       !weaponhand::client.armed()||(owns_pending()&&common::pending_fire.event.release_required(weaponhand::clock_ns(nullptr)))){
        tx::revoke();release_completed(nullptr);return true;
    }
    service_stage=4;if(!owns_pending())return true;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};unsigned reason{};
    service_stage=5;if(!meta::tables(base,schema.objects,schema.names)){tx::revoke();return true;}
    service_stage=6;if(!common::pending_identity(r)){tx::revoke();return true;}
    service_stage=7;if(!context(r)){tx::revoke();return true;}
    service_stage=8;if(!state_firing(r)){tx::revoke();return true;}
    service_stage=9;
    if(!common::hammerburst_refire::original_loop(r,tx::certificate.held.weapon.address,tx::certificate.timer.address,reason)){
        tx::revoke();return true;
    }
    validated=true;service_stage=0;return true;
}
void deliver(hand::Frame const& f) noexcept {
    struct Report {bool first{tx::first_debt};~Report(){if(first)report_first_wait(L"deliver");}} report;
    delivery_stage=1;if(!validated)return;
    delivery_stage=2;if(!owns_pending())return;
    delivery_stage=3;if(tx::cancel_requested)return;
    delivery_stage=4;if(common::delivering)return;
    delivery_stage=5;if(vrfire::mode.load(std::memory_order_acquire)!=vrfire::running)return;
    delivery_stage=6;if(!right_tracked(f))return;
    delivery_stage=7;if(!nativeequipment::current(nullptr,f))return;
    delivery_stage=8;if(!unified_weapon_fire::ready_for(f,20000000))return;
    auto const& sample=unified_weapon_fire::muzzle_sample;
    delivery_stage=9;if(!hand::same_frame(f,sample.source))return;
    const auto actual=tx::source(sample.source,sample.render_acquisition_ns,sample.sampled_ns,weaponhand::clock_ns(nullptr));
    const auto first=event.kind==common::policy::EventKind::first;
    const bool resets_counter=first&&event.function.address==tx::certificate.outer_first.address;
    delivery_stage=10;if(!tx::enter(first,actual,event.serial,event.captured,resets_counter))return;
    const auto function=common::pending_fire.function.address;
    const auto parameters=common::pending_fire.parameters;const auto bytes=common::pending_fire.parameter_bytes;
    delivery_stage=11;if(!common::pending_fire.event.claim(weaponhand::clock_ns(nullptr))){tx::leave(false);return;}
    event={};validated=false;common::pending_validated=false;
    delivery_stage=12;const bool invoked=common::invoke_original(f.weapon.address,function,bytes,parameters.data());
    static unsigned delivery_logs{};
    if(log_file&&delivery_logs++<20){fwprintf(log_file,L"native_hammerburst_original_delivered first=%u outer=%u invoked=%u frame=%llu r2_source_down=%u input_owned=%u observed_shot=%llu original_count=%u\n",
        unsigned(first),unsigned(resets_counter),unsigned(invoked),f.key.frame,unsigned(actual.trigger_down),
        unsigned(vrfire::input.owns_press()),tx::observed.shot,tx::observed.count);fflush(log_file);}
    // Real ammo exhaustion can finish before six. The original timer itself
    // then ends Firing WITHOUT a shot; do not manufacture a missing trajectory.
    if(invoked&&!first&&!tx::observed.known&&!tx::observed.conflict){
        unsigned count{},limit{};
        meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
        if(meta::tables(base,schema.objects,schema.names)&&ended(r)&&tx::checked_counts(count,limit)&&
           count==tx::completion.count()&&limit==6&&terminal_proof(count,true)&&
           tx::leave_original_end(count,true,true,true))return;
    }
    if(!tx::leave(invoked)){
        // Keep the existing normal-GT input/cleanup owner RUNNING long enough
        // to release R2 and discharge this original firing-state debt. A global
        // fault here would make vrfire::disable return before that cleanup.
        common::certified=false;static unsigned failure_logs{};
        if(log_file&&failure_logs++<4){
            fwprintf(log_file,L"native_hammerburst_burst_failed original_invoked=%u original_first=%u frame=%llu cancellation_owed=1\n",
                unsigned(invoked),unsigned(first),f.key.frame);fflush(log_file);
        }
    }
}
}
