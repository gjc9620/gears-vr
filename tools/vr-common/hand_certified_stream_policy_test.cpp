#include "hand_stream_ticket_policy.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <type_traits>
#include <utility>

using namespace gears_hand_stream;
using gears_hand_continuity::Certificate;
static_assert(noexcept(std::declval<Policy&>().enable_certified_calibration(1,0)));
static_assert(noexcept(std::declval<Policy&>().observe_certified(Context{},nullptr,0,0)));
static_assert(noexcept(std::declval<Policy const&>().authorize_certified(Plan{},Context{},0,0)));
static_assert(noexcept(std::declval<Policy const&>().certified_failed()));
static_assert(std::is_trivially_copyable<Policy>::value&&std::is_standard_layout<Policy>::value);
static_assert(observe_reason_count==15);
namespace {
constexpr std::uint64_t native=700,publisher=81,publisher_start=91;
constexpr std::uint32_t source=1;
Context context(std::uint64_t now=1000){return {{11,12,13,14,0},7,1,now,true,true};}
Ticket ticket(Context const& ctx,std::uint64_t serial,std::uint64_t sequence,std::uint64_t now,std::int64_t ns){
    Ticket result{{1,2,3,ctx.expected_native_world_generation},serial,sequence,now,
        ctx.game.controller,ctx.game.pawn,ctx.game.weapon,100.,both_hands};
    result.certificate={native,publisher,publisher_start,1,1,1,{1,1},1,ns,ns,source};
    return result;
}
struct Live {
    Policy policy;Context ctx=context();Ticket receipt{};Plan plan{};
    std::uint64_t now=1000;std::int64_t ns=10000000000,stereo_lag{},input_lag{};
    explicit Live(bool commit=true,std::uint64_t generation=1,std::int64_t stereo_age=0,std::int64_t input_age=0)
        :stereo_lag(stereo_age),input_lag(input_age){
        assert(policy.enable_certified_calibration(native,source));
        receipt=ticket(ctx,1,1,now,ns);receipt.certificate.reference_generation=generation;times();
        plan=policy.observe_certified(ctx,&receipt,now,ns);
        assert(plan.baseline_mask==3&&plan.accept_mask==0&&plan.bind_mask==0&&plan.restore_mask==3);
        assert(!policy.commit_binding(plan,3));
        next();assert(plan.accept_mask==3&&plan.bind_mask==3&&!policy.certified_failed());
        if(commit){assert(policy.commit_binding(plan,3));assert(authorized()==3);}
    }
    void times(){receipt.created_ms=now;receipt.certificate.stereo_received_ns=ns-stereo_lag;receipt.certificate.input_received_ns=ns-input_lag;}
    void advance(std::uint64_t gap=1){
        now+=gap;ns+=static_cast<std::int64_t>(gap)*1000000;ctx.observed_ms=now;
        ++receipt.serial;++receipt.sequence;times();
    }
    void observe(){plan=policy.observe_certified(ctx,&receipt,now,ns);}
    void next(std::uint64_t gap=1){advance(gap);observe();}
    unsigned authorized() const {return policy.authorize_certified(plan,ctx,now,ns);}
};
void permanent_failure(Live& live,ObserveReason expected=ObserveReason::count){
    const auto old=live.plan;live.observe();
    assert(live.policy.certified_failed()&&live.plan.accept_mask==0&&live.plan.bind_mask==0);
    assert(live.plan.reset_binding_mask==3&&live.plan.restore_mask==3&&live.policy.bound_mask()==0);
    if(expected!=ObserveReason::count)assert(live.policy.last_observe().mask(expected)==3);
    assert(!live.policy.commit_binding(old,1)&&live.authorized()==0);
    // Neither waiting for a rejected future stamp nor a newer corrected packet
    // can reopen this deliberately bounded permanent fence.
    live.now+=200;live.ns+=200000000;live.ctx.observed_ms=live.now;
    live.observe();assert(live.authorized()==0&&live.policy.certified_failed());
    live.receipt=ticket(live.ctx,100,100,live.now,live.ns);live.observe();
    assert(live.plan.accept_mask==0&&live.policy.bound_mask()==0&&live.policy.certified_failed());
    assert(!live.policy.enable_certified_calibration(native,source));
}
void age_only_keeps_mount_but_not_an_old_plan(){
    Live live;const auto old=live.plan;const auto before=live.policy.continuity_snapshot(live.now);
    live.next(101);
    assert(live.plan.accept_mask==3&&live.plan.bind_mask==0&&live.plan.reset_binding_mask==0&&live.authorized()==3);
    assert(live.policy.bound_mask()==3&&live.policy.last_observe().mask(ObserveReason::binding_age)==3);
    const auto after=live.policy.continuity_snapshot(live.now);
    assert(after.source_stamp[0]==live.receipt.created_ms&&after.source_stamp[0]>before.source_stamp[0]);
    assert(live.policy.last_observe().previous_context_age.ms==101);
    assert(live.policy.authorize_certified(old,old.context,old.observed_now,old.ticket.certificate.input_received_ns)==0);
    assert(!live.policy.commit_binding(old,1));
    Live uncommitted(false);const auto pending=uncommitted.plan;
    auto yielded=uncommitted.policy.observe_certified(uncommitted.ctx,nullptr,uncommitted.now,uncommitted.ns);
    assert(yielded.restore_mask==3&&!uncommitted.policy.commit_binding(pending,3));
}
void no_receipt_and_aged_context_suspend_without_renewal(){
    Live live;const auto stamp=live.receipt.created_ms;
    live.now+=250;live.ns+=250000000; // Preserve the old observed context as well.
    auto yielded=live.policy.observe_certified(live.ctx,nullptr,live.now,live.ns);
    assert(yielded.accept_mask==0&&yielded.bind_mask==0&&yielded.reset_binding_mask==0&&yielded.restore_mask==3);
    assert(live.policy.bound_mask()==3&&!live.policy.certified_failed());
    auto snapshot=live.policy.continuity_snapshot(live.now);
    assert(snapshot.source_stamp[0]==stamp&&snapshot.old_ticket_age[0].ms==250&&!snapshot.ready);
    live.next();assert(live.plan.accept_mask==3&&live.plan.bind_mask==0&&live.authorized()==3);
    Live aged;const auto old_stamp=aged.receipt.created_ms;aged.advance(101);aged.ctx.observed_ms=aged.now-101;aged.observe();
    assert(aged.plan.accept_mask==0&&aged.plan.reset_binding_mask==0&&aged.plan.restore_mask==3);
    assert(aged.policy.bound_mask()==3&&aged.policy.continuity_snapshot(aged.now).source_stamp[0]==old_stamp);
    aged.next();assert(aged.plan.accept_mask==3&&aged.plan.bind_mask==0&&aged.authorized()==3);
    Live replay;replay.advance(101);replay.ctx.observed_ms=replay.now-101;replay.observe();
    replay.ctx.observed_ms=replay.now;permanent_failure(replay,ObserveReason::not_newer); // The suspended receipt was consumed.
}
void hidden_single_hand_loss_requires_only_that_hand_to_rebind(){
    for(unsigned hand=0;hand<2;++hand){
        Live live;live.advance(101);++live.receipt.certificate.tracking_epoch[hand];live.observe();
        const unsigned lost=1u<<hand,other=3u&~lost;
        assert(live.plan.reset_binding_mask==lost&&live.plan.baseline_mask==lost&&live.plan.accept_mask==other);
        assert(live.plan.bind_mask==0&&live.policy.bound_mask()==other&&live.authorized()==other);
        assert(live.policy.last_observe().mask(ObserveReason::tracking_loss)==lost);
        live.next();assert(live.plan.accept_mask==3&&live.plan.bind_mask==lost);
        assert(live.policy.commit_binding(live.plan,lost)&&live.authorized()==3);
    }
    Live explicit_loss;explicit_loss.advance();explicit_loss.receipt.valid_mask=left_hand;
    ++explicit_loss.receipt.certificate.tracking_epoch[1];explicit_loss.observe();
    assert(explicit_loss.plan.baseline_mask==right_hand&&explicit_loss.authorized()==left_hand);
    explicit_loss.advance();explicit_loss.receipt.valid_mask=3;explicit_loss.observe();
    assert(explicit_loss.plan.bind_mask==right_hand&&explicit_loss.policy.commit_binding(explicit_loss.plan,right_hand));
}
void global_boundaries_require_baseline_and_new_receipt(){
    for(unsigned field=0;field<8;++field){
        Live live(true,9);live.advance();
        switch(field){
        case 0:++live.receipt.certificate.stream_epoch;break;
        case 1:++live.receipt.certificate.reference_epoch;live.receipt.certificate.reference_generation=1;break;
        case 2:++live.receipt.stream.session;break;
        case 3:++live.receipt.stream.space_epoch;break;
        case 4:++live.receipt.stream.input_epoch;break;
        case 5:++live.receipt.certificate.profile_generation;break;
        case 6:live.receipt.units_per_metre=std::nextafter(100.,101.);break;
        case 7:++live.ctx.expected_native_world_generation;++live.receipt.stream.world_generation;break;
        }
        live.observe();assert(!live.policy.certified_failed());
        assert(live.plan.baseline_mask==3&&live.plan.reset_binding_mask==3&&live.plan.accept_mask==0&&live.policy.bound_mask()==0);
        assert(!live.policy.commit_binding(live.plan,3)&&live.authorized()==0);
        live.next();assert(live.plan.bind_mask==3&&live.policy.commit_binding(live.plan,3)&&live.authorized()==3);
    }
}
void identity_action_and_readiness_boundaries_are_not_age_only(){
    for(unsigned field=0;field<8;++field){
        Live live;
        switch(field){
        case 0:++live.ctx.game.world;break;case 1:++live.ctx.game.controller;break;
        case 2:++live.ctx.game.pawn;break;case 3:++live.ctx.game.mesh;break;
        case 4:++live.ctx.game.weapon;break;case 5:++live.ctx.action_epoch;break;
        case 6:live.ctx.idle_ready=false;break;case 7:live.ctx.clean=false;break;
        }
        const auto yielded=live.policy.observe_certified(live.ctx,nullptr,live.now,live.ns);
        assert(yielded.reset_binding_mask==3&&yielded.restore_mask==3&&live.policy.bound_mask()==0&&!live.policy.certified_failed());
        live.ctx.idle_ready=live.ctx.clean=true;live.advance();
        live.receipt.controller=live.ctx.game.controller;live.receipt.pawn=live.ctx.game.pawn;live.receipt.weapon=live.ctx.game.weapon;
        live.observe();assert(live.plan.baseline_mask==3&&live.plan.accept_mask==0);
        live.next();assert(live.plan.bind_mask==3&&live.policy.commit_binding(live.plan,3)&&live.authorized()==3);
    }
    Live future_context;future_context.ctx.observed_ms=future_context.now+1;
    auto yielded=future_context.policy.observe_certified(future_context.ctx,nullptr,future_context.now,future_context.ns);
    assert(yielded.reset_binding_mask==3&&!future_context.policy.certified_failed());
}
void raw_sequence_restart_requires_new_stream_and_session_but_serial_never_resets(){
    Live live;live.advance();++live.receipt.stream.session;++live.receipt.certificate.stream_epoch;live.receipt.sequence=1;
    live.observe();assert(live.plan.baseline_mask==3&&!live.policy.certified_failed());
    live.next();assert(live.receipt.sequence==2&&live.plan.bind_mask==3&&live.policy.commit_binding(live.plan,3));
    auto snap=live.policy.continuity_snapshot(live.now);assert(snap.serial==4&&snap.sequence==2);
    for(unsigned kind=0;kind<4;++kind){
        Live bad;bad.advance();
        switch(kind){
        case 0:++bad.receipt.stream.session;bad.receipt.sequence=1;break;
        case 1:++bad.receipt.certificate.stream_epoch;bad.receipt.sequence=1;break;
        case 2:++bad.receipt.certificate.stream_epoch;++bad.receipt.stream.session;bad.receipt.sequence=1;bad.receipt.serial=2;break;
        case 3:bad.receipt.sequence=2;break;
        }
        permanent_failure(bad,ObserveReason::not_newer);
    }
}
void epochs_generation_and_source_times_cannot_regress(){
    for(unsigned field=0;field<8;++field){
        Live live;live.advance();
        // First establish non-minimal monotonic epochs, with a legal reference reset.
        live.receipt.certificate.stream_epoch=5;live.receipt.certificate.reference_epoch=5;
        live.receipt.certificate.tracking_epoch[0]=live.receipt.certificate.tracking_epoch[1]=5;
        live.receipt.certificate.reference_generation=9;live.observe();live.next();
        assert(live.policy.commit_binding(live.plan,3));live.advance();
        switch(field){
        case 0:--live.receipt.certificate.stream_epoch;break;
        case 1:--live.receipt.certificate.reference_epoch;break;
        case 2:--live.receipt.certificate.tracking_epoch[0];break;
        case 3:--live.receipt.certificate.tracking_epoch[1];break;
        case 4:--live.receipt.certificate.reference_generation;break;
        case 5:++live.receipt.certificate.reference_generation;break;
        case 6:live.receipt.certificate.stereo_received_ns=live.ns-2000000;break;
        case 7:live.receipt.certificate.input_received_ns=live.ns-2000000;break;
        }
        permanent_failure(live,ObserveReason::not_newer);
    }
}
void malformed_wrong_publisher_or_native_receipts_are_terminal(){
    for(unsigned field=0;field<12;++field){
        Live live;live.advance();
        switch(field){
        case 0:++live.receipt.certificate.native_started_qpc;break;
        case 1:++live.receipt.certificate.xr_publisher_pid;break;
        case 2:++live.receipt.certificate.xr_publisher_start;break;
        case 3:live.receipt.certificate.source=0;break;
        case 4:live.receipt.certificate.tracking_epoch[0]=0;break;
        case 5:live.receipt.certificate.profile_generation=0;break;
        case 6:live.receipt.valid_mask=4;break;
        case 7:live.receipt.units_per_metre=0;break;
        case 8:++live.receipt.pawn;break;
        case 9:++live.receipt.stream.world_generation;break;
        case 10:live.receipt.serial=0;break;
        case 11:live.receipt.stream.input_epoch=0;break;
        }
        permanent_failure(live);
    }
    Policy first;auto ctx=context();ctx.idle_ready=false;auto receipt=ticket(ctx,1,1,1000,10000000000);
    assert(first.enable_certified_calibration(native,source));
    first.observe_certified(ctx,&receipt,1000,10000000000);assert(!first.certified_failed());
    ++receipt.serial;++receipt.sequence;++receipt.certificate.xr_publisher_start;ctx.idle_ready=true;
    const auto plan=first.observe_certified(ctx,&receipt,1000,10000000000);
    assert(first.certified_failed()&&plan.accept_mask==0); // First legal source froze the publisher before binding.
}
void future_and_aged_new_receipts_cannot_wait_until_valid(){
    for(unsigned field=0;field<6;++field){
        Live live(true,1,field==4?100000000:0,field==5?100000000:0);
        live.advance(field==1?202:field>=4?101:1);
        switch(field){
        case 0:live.receipt.created_ms=live.now+1;break;
        case 1:live.receipt.created_ms=live.now-101;break;
        case 2:live.receipt.certificate.stereo_received_ns=live.ns+1;break;
        case 3:live.receipt.certificate.input_received_ns=live.ns+1;break;
        case 4:live.receipt.certificate.stereo_received_ns=live.ns-100000001;break;
        case 5:live.receipt.certificate.input_received_ns=live.ns-100000001;break;
        }
        permanent_failure(live,ObserveReason::ticket_age);
    }
}
void both_original_source_ages_are_rechecked_at_authorization(){
    for(unsigned hand=0;hand<2;++hand){
        Live live(true,1,hand==0?100000000:0,hand==1?100000000:0);
        assert(live.authorized()==3);
        // Context and Native publication remain fresh; only one original NS is now too old.
        assert(live.policy.authorize_certified(live.plan,live.ctx,live.now,live.ns+1)==0);
        assert(live.policy.authorize_certified(live.plan,live.ctx,live.now,live.ns-1)==0);
    }
    Live live;auto ctx=live.ctx;++ctx.action_epoch;
    assert(live.policy.authorize_certified(live.plan,ctx,live.now,live.ns)==0);
    ctx=live.ctx;ctx.clean=false;assert(live.policy.authorize_certified(live.plan,ctx,live.now,live.ns)==0);
    ctx=live.ctx;ctx.observed_ms=live.now+1;assert(live.policy.authorize_certified(live.plan,ctx,live.now,live.ns)==0);
}
void alter_certificate(Certificate& c,unsigned field){
    switch(field){
    case 0:++c.native_started_qpc;break;case 1:++c.xr_publisher_pid;break;case 2:++c.xr_publisher_start;break;
    case 3:++c.stream_epoch;break;case 4:++c.reference_epoch;break;case 5:++c.reference_generation;break;
    case 6:++c.tracking_epoch[0];break;case 7:++c.tracking_epoch[1];break;case 8:++c.profile_generation;break;
    case 9:++c.stereo_received_ns;break;case 10:++c.input_received_ns;break;case 11:c.source^=1;break;
    }
}
void plans_seal_every_certificate_field(){
    Live live(false);
    for(unsigned field=0;field<12;++field){
        auto forged=live.plan;alter_certificate(forged.ticket.certificate,field);
        assert(!live.policy.commit_binding(forged,3)&&live.policy.bound_mask()==0);
    }
    assert(live.policy.commit_binding(live.plan,3)&&live.authorized()==3);
    for(unsigned field=0;field<12;++field){
        auto forged=live.plan;alter_certificate(forged.ticket.certificate,field);
        assert(live.policy.authorize_certified(forged,live.ctx,live.now,live.ns)==0);
    }
}
void modes_cannot_bypass_the_source_clock(){
    Policy disabled;auto ctx=context();auto receipt=ticket(ctx,1,1,1000,10000000000);
    auto plan=disabled.observe_certified(ctx,&receipt,1000,10000000000);
    assert(plan.accept_mask==0&&disabled.authorize_certified(plan,ctx,1000,10000000000)==0);
    assert(!disabled.enable_certified_calibration(native,source));
    Policy observed;observed.observe(ctx,nullptr,1000);assert(!observed.enable_certified_calibration(native,source));
    Policy transported;transported.refresh_bound_transport(receipt,ctx.action_epoch,1000);
    assert(!transported.enable_certified_calibration(native,source));
    Policy fresh;fresh.continuity_snapshot(1000);
    assert(!fresh.enable_certified_calibration(0,source)&&!fresh.enable_certified_calibration(native,2));
    assert(fresh.enable_certified_calibration(native,0)&&!fresh.enable_certified_calibration(native,0));
    receipt.certificate.source=0;plan=fresh.observe_certified(ctx,&receipt,1000,10000000000);
    assert(plan.baseline_mask==3&&!fresh.certified_failed());
    ++receipt.serial;++receipt.sequence;plan=fresh.observe_certified(ctx,&receipt,1000,10000000000);
    assert(fresh.commit_binding(plan,3)&&fresh.authorize_certified(plan,ctx,1000,10000000000)==3);
    for(unsigned method=0;method<2;++method){
        Live live;const auto old=live.plan;
        assert(live.policy.authorize(old,live.ctx,live.now)==0&&live.authorized()==3);
        live.advance();
        if(method==0){plan=live.policy.observe(live.ctx,&live.receipt,live.now);assert(plan.accept_mask==0&&plan.restore_mask==3);}
        else {auto result=live.policy.refresh_bound_transport(live.receipt,live.ctx.action_epoch,live.now);
            assert(result.refreshed_mask==0&&result.reset_binding_mask==3);}
        assert(live.policy.bound_mask()==0&&live.policy.authorize_certified(old,old.context,old.observed_now,live.ns)==0);
        live.observe();assert(live.plan.baseline_mask==3&&live.plan.accept_mask==0);
        live.next();assert(live.plan.bind_mask==3&&live.policy.commit_binding(live.plan,3));
    }
}
}
int main(){
    age_only_keeps_mount_but_not_an_old_plan();no_receipt_and_aged_context_suspend_without_renewal();
    hidden_single_hand_loss_requires_only_that_hand_to_rebind();global_boundaries_require_baseline_and_new_receipt();
    identity_action_and_readiness_boundaries_are_not_age_only();
    raw_sequence_restart_requires_new_stream_and_session_but_serial_never_resets();
    epochs_generation_and_source_times_cannot_regress();malformed_wrong_publisher_or_native_receipts_are_terminal();
    future_and_aged_new_receipts_cannot_wait_until_valid();both_original_source_ages_are_rechecked_at_authorization();
    plans_seal_every_certificate_field();modes_cannot_bypass_the_source_clock();
    std::puts("certified hand policy: 12 age, continuity, authentication, baseline and permission-isolation scenarios passed");
}
