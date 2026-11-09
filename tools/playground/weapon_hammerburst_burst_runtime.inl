#include "../vr-common/weapon_hammerburst_burst.h"
// HB-only completion transaction, to be called by the EXISTING event owner.
// There is deliberately no dispatcher, hook or publisher in this module.
namespace hammerburst_burst_runtime {
namespace policy=gears_hammerburst_burst;
namespace hand=gears_weapon_hand;
namespace meta=gripdamage_metadata;
hammerburst_burst_contract::Certificate certificate;
policy::Completion completion;
policy::Cohort pressed;
bool first_debt{},firing_debt{},cancel_requested{},scope_active{},scope_first{};
policy::Ticket ticket;
policy::Source entered;
unsigned enter_stage{};
policy::Source attempted_source; // Observation only; never a source permit.
struct Observed {
    policy::Source source{};std::uint64_t shot{};unsigned count{},limit{};
    bool known{},conflict{};
} observed;
policy::Cohort cohort(hand::Frame const& f) noexcept {
    policy::Cohort c{};c.key=f.key;c.actors={f.world,f.controller,f.pawn,f.weapon};
    c.hold=certificate.held.generation;c.reference=f.reference_generation;
    c.session=f.stereo.session;c.space=f.stereo.space_epoch;
    c.input=f.controllers.input_epoch;c.profile=f.controllers.profile_generation;
    c.presses=f.controllers.hands[1].pressed[gears_vr::control_trigger];
    c.releases=f.controllers.hands[1].released[gears_vr::control_trigger];
    c.known=certificate.known&&hand::same(f.world,certificate.held.world)&&
        hand::same(f.controller,certificate.held.controller)&&hand::same(f.pawn,certificate.held.pawn)&&
        hand::same(f.weapon,certificate.held.weapon)&&f.key.registration_cookie==certificate.held.native_cookie&&
        f.key.native_started_qpc==certificate.held.native_started&&f.key.world_generation==certificate.held.world_generation;
    return c;
}
policy::Source source(hand::Frame const& f,std::int64_t render,std::int64_t sampled,std::int64_t now,
                      bool tracking_only=false) noexcept {
    policy::Source s{};s.cohort=cohort(f);s.frame=f.key.frame;s.now=now;
    s.sync=f.controllers.sync_ns;s.stereo=f.stereo.received_ns;s.render=render;s.sampled=sampled;
    s.trigger_down=f.controllers.hands[1].trigger_active==1&&f.controllers.hands[1].trigger>.35f;
    // Actual source admission remains the caller's unchanged native/muzzle
    // validation. This only identifies a transaction already in progress.
    s.authenticated=s.cohort.known;s.tracking_only=tracking_only;return s;
}
void revoke() noexcept {
    if(firing_debt||first_debt||completion.state()!=policy::State::idle)cancel_requested=true;
    completion.revoke();observed.conflict=true;
}
bool raw_counts(unsigned& count,unsigned& limit) noexcept {
    if(!certificate.known)return false;
    unsigned char c{},n{};
    if(!copy(&c,certificate.held.weapon.address+certificate.count.offset,1)||
       !copy(&n,certificate.held.weapon.address+certificate.limit.offset,1)||c>6||n!=6)return false;
    count=c;limit=n;return true;
}
bool checked_counts(unsigned& count,unsigned& limit) noexcept {
    if(!certificate.known||!weaponhand::current_gt(nullptr)||
       !lancerrefire::same_hold(certificate.held,weaponhand::held_binding))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::Property c{},n{},mode{};unsigned before_count{},before_limit{},after_count{},after_limit{};
    unsigned char before_mode{},after_mode{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,certificate.held.weapon)||
       !r.property(certificate.count_property,c)||!gears_hammerburst_property_cache::same(r,c,certificate.count,
            certificate.count_descriptor,certificate.count_type)||
       !r.property(certificate.limit_property,n)||!gears_hammerburst_property_cache::same(r,n,certificate.limit,
            certificate.limit_descriptor,certificate.limit_type)||
       !r.property(certificate.mode_property,mode)||!gears_hammerburst_property_cache::same(r,mode,certificate.mode,
            certificate.mode_descriptor,certificate.mode_type)||
       !r.at(certificate.held.weapon.address,mode.offset,before_mode)||before_mode||
       !raw_counts(before_count,before_limit)||!hammerburst_burst_contract::retain(r,c)||
       !hammerburst_burst_contract::retain(r,n)||!hammerburst_burst_contract::retain(r,mode)||
       !unified_weapon_fire::retained_metadata(r)||!raw_counts(after_count,after_limit)||
       before_count!=after_count||before_limit!=after_limit||
       !r.at(certificate.held.weapon.address,mode.offset,after_mode)||before_mode!=after_mode)return false;
    count=before_count;limit=before_limit;return true;
}
bool prime(hand::Frame const& f,uptr original_timer) noexcept {
    if(scope_active||firing_debt||first_debt||cancel_requested||completion.state()!=policy::State::idle)return false;
    hammerburst_burst_contract::Certificate next{};
    if(!nativeequipment::current(nullptr,f)||
       !hammerburst_burst_contract::prepare(weaponhand::held_binding,original_timer,next)||
       !nativeequipment::current(nullptr,f))return false;
    certificate=next;pressed={};return true;
}
// Called only around the already authenticated original ButtonPress, after
// prime(). It does not arm Completion or synthesize a controller edge.
bool press(hand::Frame const& f) noexcept {
    if(!certificate.known||firing_debt||first_debt||cancel_requested||scope_active)return false;
    auto c=cohort(f);if(!c.known||f.controllers.hands[1].trigger_active!=1||f.controllers.hands[1].trigger<=.35f)return false;
    pressed=c;enter_stage=0;attempted_source={};return true;
}
bool capture_first() noexcept {
    // The adapter must call this only after its exact original Engine
    // WeaponFiring.BeginState proof and existing queue() capture succeed.
    if(!certificate.known||!pressed.known||cancel_requested||scope_active||first_debt||
       completion.state()!=policy::State::idle||(firing_debt&&completion.count()!=6))return false;
    first_debt=true;firing_debt=true;return true;
}
bool need_source(hand::Frame const& f) noexcept {
    if(!certificate.known||cancel_requested||scope_active)return false;
    auto c=cohort(f);
    if(!policy::same(c,pressed)){revoke();return false;}
    if(first_debt)return f.controllers.hands[1].trigger_active==1&&f.controllers.hands[1].trigger>.35f&&
        c.presses==pressed.presses&&c.releases==pressed.releases;
    if(completion.state()!=policy::State::committed)return false;
    // No stale pose is retained here. Freshness is checked on the actual
    // muzzle sample in enter(), then independently at every callback.
    policy::Source s{};s.cohort=c;s.frame=f.key.frame;s.authenticated=c.known;
    s.trigger_down=f.controllers.hands[1].trigger_active==1&&f.controllers.hands[1].trigger>.35f;
    if(!completion.observe(s,true)){revoke();return false;}
    return true;
}
bool enter(bool first,policy::Source actual,std::uint64_t event,std::int64_t captured,bool original_resets_counter=false) noexcept {
    enter_stage=1;attempted_source=actual;
    if(scope_active||cancel_requested||!certificate.known)return false;
    enter_stage=2;
    unsigned count{},limit{};
    if(!checked_counts(count,limit)){revoke();return false;}
    // Refresh only the decision clock after metadata work, never any source,
    // render, pose or sampling stamp supplied by the real muzzle preparation.
    actual.now=weaponhand::clock_ns(nullptr);
    attempted_source=actual;
    if(first){
        enter_stage=3;if(!first_debt)return false;
        enter_stage=4;if(!vrfire::input.owns_press())return false;
        enter_stage=5;if(!actual.trigger_down)return false;
        enter_stage=6;if(!policy::same(pressed,actual.cohort))return false;
        enter_stage=7;if(actual.cohort.presses!=pressed.presses)return false;
        enter_stage=8;if(actual.cohort.releases!=pressed.releases)return false;
        enter_stage=9;if(!event)return false;
        enter_stage=10;if(captured<=0||captured>actual.now)return false;
        enter_stage=11;if(actual.now-captured>250000000)return false;
        enter_stage=12;if(!original_resets_counter&&count)return false;
        enter_stage=13;if(limit!=6)return false;
        enter_stage=14;if(!policy::fresh(actual,true))return false;
    }else{
        enter_stage=20;
        if(!completion.enter(actual,event,captured,count,limit,ticket)){
            if(completion.needs_cancel())revoke();return false;
        }
    }
    entered=actual;observed={};scope_active=true;scope_first=first;enter_stage=0;return true;
}
bool permits(policy::Source const& actual) noexcept {
    if(!scope_active||cancel_requested||actual.frame!=entered.frame)return false;
    if(!scope_first)return completion.permits(ticket,actual);
    return first_debt&&policy::same(actual.cohort,pressed)&&
        actual.cohort.presses==pressed.presses&&actual.cohort.releases==pressed.releases&&
        actual.trigger_down&&policy::fresh(actual);
}
// Callback body: two cached byte reads and bounded POD only. No metadata walk,
// engine getter, client admission, timer, heap or output is introduced here.
void trace(policy::Source const& actual,std::uint64_t shot,bool consistent) noexcept {
    if(!scope_active)return;
    unsigned count{},limit{};
    if(!shot||!consistent||!permits(actual)||!raw_counts(count,limit)){
        observed.conflict=true;return;
    }
    if(observed.known&&(observed.shot!=shot||observed.count!=count||observed.limit!=limit||
       observed.source.frame!=actual.frame)){observed.conflict=true;return;}
    observed.source=actual;observed.shot=shot;observed.count=count;observed.limit=limit;observed.known=true;
}
bool leave(bool invoked) noexcept {
    if(!scope_active)return false;
    const auto first=scope_first;scope_active=false;
    unsigned count{},limit{};
    const bool valid=!cancel_requested&&invoked&&observed.known&&!observed.conflict&&checked_counts(count,limit)&&
        observed.count==count&&observed.limit==limit;
    bool accepted{};
    if(first){
        accepted=valid&&completion.commit(observed.source,observed.shot,count,limit,true);
        if(accepted)first_debt=false;
    }else accepted=completion.leave(ticket,observed.shot,count,invoked,valid);
    observed={};if(!accepted)revoke();return accepted;
}
bool leave_original_end(unsigned original_count,bool no_refire,bool firing_ended,bool magazine_empty) noexcept {
    if(!scope_active||scope_first||cancel_requested||observed.known||observed.conflict)return false;
    if(!completion.leave_original_end(ticket,original_count,no_refire,firing_ended,magazine_empty))return false;
    scope_active=false;firing_debt=false;observed={};return true;
}
bool cancel() noexcept {
    revoke();
    if(!cancel_requested)return true;
    if(scope_active||!certificate.known||!hammerburst_burst_cancel::complete(certificate.held))return false;
    if(completion.needs_cancel()&&!completion.acknowledge_cancel(true,true))return false;
    first_debt=false;firing_debt=false;cancel_requested=false;pressed={};certificate={};return true;
}
}
