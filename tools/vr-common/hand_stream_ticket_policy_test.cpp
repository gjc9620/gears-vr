#include "hand_stream_ticket_policy.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include <utility>
using namespace gears_hand_stream;
static_assert(std::is_trivially_copyable<GameKey>::value&&std::is_standard_layout<GameKey>::value);
static_assert(std::is_trivially_copyable<Ticket>::value&&std::is_standard_layout<Ticket>::value);
static_assert(std::is_trivially_copyable<Plan>::value&&std::is_standard_layout<Plan>::value);
static_assert(std::is_trivially_copyable<Policy>::value&&std::is_standard_layout<Policy>::value);
static_assert(std::is_trivially_copyable<ObserveDiagnostic>::value&&std::is_standard_layout<ObserveDiagnostic>::value);
static_assert(std::is_same<decltype(std::declval<Policy const&>().last_observe()),ObserveDiagnostic const&>::value);
static_assert(std::is_trivially_copyable<ContinuitySnapshot>::value&&std::is_standard_layout<ContinuitySnapshot>::value);
static_assert(std::is_same<decltype(std::declval<Policy const&>().continuity_snapshot(0)),ContinuitySnapshot>::value);
static_assert(noexcept(std::declval<Policy const&>().continuity_snapshot(0)));
static_assert(std::is_trivially_copyable<TransportResult>::value&&std::is_standard_layout<TransportResult>::value);
static_assert(std::is_same<decltype(std::declval<Policy&>().refresh_bound_transport(Ticket{},0,0)),TransportResult>::value);
static_assert(noexcept(std::declval<Policy&>().refresh_bound_transport(Ticket{},0,0)));
static_assert(transport_stream_capacity==8&&observe_reason_count==15);
namespace {
Context context(std::uint64_t now=1000){return {{11,12,13,14,0},7,1,now,true,true};}
Ticket ticket(Context const& c,std::uint64_t serial,std::uint64_t sequence,std::uint64_t now){
    return {{1,2,3,c.expected_native_world_generation},serial,sequence,now,
        c.game.controller,c.game.pawn,c.game.weapon,100.,both_hands};
}
struct Live {
    Policy policy;Context ctx=context();Ticket receipt{};Plan plan{};std::uint64_t now=1000;
    Live(){
        receipt=ticket(ctx,1,1,now);plan=policy.observe(ctx,&receipt,now);
        assert(plan.baseline_mask==3&&plan.accept_mask==0&&plan.bind_mask==0);
        assert(!policy.commit_binding(plan,3)&&policy.authorize(plan,ctx,now)==0);
        feed(2,2);assert(plan.accept_mask==3&&plan.bind_mask==3);
        assert(policy.commit_binding(plan,3)&&policy.authorize(plan,ctx,now)==3);
    }
    void feed(std::uint64_t serial,std::uint64_t sequence,unsigned mask=3){
        ctx.observed_ms=++now;receipt=ticket(ctx,serial,sequence,now);receipt.valid_mask=mask;
        plan=policy.observe(ctx,&receipt,now);
    }
};
void baseline_and_per_hand_commit(){
    Policy policy;auto c=context();auto t=ticket(c,1,1,1000);
    auto p=policy.observe(c,&t,1000);
    assert(p.accept_mask==0&&p.baseline_mask==3&&p.restore_mask==3);
    const auto first=policy.last_observe();
    assert(first.mask(ObserveReason::context_identity)==3&&first.mask(ObserveReason::context_readiness)==3);
    assert(first.mask(ObserveReason::stream_changed)==3&&!first.previous_context_age.known);
    assert(first.context_age.known&&first.context_age.ms==0&&!first.context_age.future);
    assert(first.new_ticket_age.known&&first.new_ticket_age.ms==0&&!first.old_ticket_age[0].known);
    assert(!policy.commit_binding(p,1)&&policy.bound_mask()==0);
    t.serial=t.sequence=2;p=policy.observe(c,&t,1000);
    assert(p.accept_mask==3&&p.bind_mask==3&&policy.authorize(p,c,1000)==0);
    for(auto mask:policy.last_observe().reason_masks)assert(mask==0); // Reasons reset each observe.
    assert(!policy.commit_binding(p,4)&&!policy.commit_binding(p,0));
    assert(policy.commit_binding(p,right_hand)&&policy.authorize(p,c,1000)==right_hand);
    assert(!policy.commit_binding(p,right_hand));
    assert(policy.commit_binding(p,left_hand)&&policy.authorize(p,c,1000)==3);
    // weapon==0 is a valid identity, not a missing owner.
    assert(c.game.weapon==0&&p.ticket.weapon==0);
}
void no_receipt_never_reuses_pose_and_old_plans_die(){
    Live live;const auto old=live.plan;
    auto p=live.policy.observe(live.ctx,nullptr,live.now);
    assert(!p.has_ticket&&p.accept_mask==0&&p.bind_mask==0&&p.restore_mask==3&&p.reset_binding_mask==0);
    assert(live.policy.bound_mask()==3&&live.policy.authorize(p,live.ctx,live.now)==0);
    assert(live.policy.authorize(old,live.ctx,live.now)==0&&!live.policy.commit_binding(old,1));
    assert(live.policy.last_observe().mask(ObserveReason::no_receipt)==3);
    assert(!live.policy.last_observe().new_ticket_age.known&&live.policy.last_observe().bound_after==3);
    live.feed(3,3);assert(live.plan.accept_mask==3&&live.plan.bind_mask==0);
    assert(live.policy.authorize(live.plan,live.ctx,live.now)==3);
}
void plans_cannot_be_modified_or_used_after_new_observation(){
    Live live;auto altered=live.plan;altered.ticket.serial++;
    assert(live.policy.authorize(altered,live.ctx,live.now)==0);
    altered=live.plan;altered.accept_mask=1;
    assert(live.policy.authorize(altered,live.ctx,live.now)==0);
    altered=live.plan;altered.context.action_epoch++;
    assert(live.policy.authorize(altered,live.ctx,live.now)==0);
    auto current=live.ctx;current.observed_ms=live.now+1;
    assert(live.policy.authorize(live.plan,current,live.now+1)==3);
    current.action_epoch++;
    assert(live.policy.authorize(live.plan,current,live.now+1)==0);
    current=live.ctx;current.game.mesh++;
    assert(live.policy.authorize(live.plan,current,live.now)==0);
    current=live.ctx;current.clean=false;
    assert(live.policy.authorize(live.plan,current,live.now)==0);
    current=live.ctx;current.idle_ready=false;
    assert(live.policy.authorize(live.plan,current,live.now)==0);
    current=live.ctx;current.observed_ms--;
    assert(live.policy.authorize(live.plan,current,live.now)==0);
    assert(live.policy.authorize(live.plan,live.ctx,live.now-1)==0);
    const auto previous=live.plan;live.feed(3,3);
    assert(live.policy.authorize(previous,live.ctx,live.now)==0);
}
void serial_and_sequence_each_strictly_advance(){
    Live live;
    live.feed(3,2);assert(live.plan.accept_mask==0&&live.policy.bound_mask()==3);
    assert(live.policy.last_observe().mask(ObserveReason::not_newer)==3);
    assert(live.policy.last_observe().mask(ObserveReason::ticket_shape)==0);
    live.feed(3,3);assert(live.plan.accept_mask==0); // Rejected serial 3 was consumed.
    live.feed(4,3);assert(live.plan.accept_mask==0); // Rejected sequence 3 was consumed.
    live.feed(5,4);assert(live.plan.accept_mask==3);
    live.feed(4,5);assert(live.plan.accept_mask==0);
    live.feed(6,5);assert(live.plan.accept_mask==0);
    live.feed(7,6);assert(live.plan.accept_mask==3);
    live.feed(0,7);assert(live.plan.accept_mask==0&&live.plan.restore_mask==3);
    assert(live.policy.last_observe().mask(ObserveReason::ticket_shape)==3);
    assert(live.policy.last_observe().mask(ObserveReason::not_newer)==0); // Shape returns first.
    live.feed(8,0);assert(live.plan.accept_mask==0);
}
void single_hand_loss_and_fresh_recovery(){
    Live live;live.feed(3,3,left_hand);
    assert(live.plan.accept_mask==left_hand&&live.plan.bind_mask==0);
    assert(live.plan.reset_binding_mask==right_hand&&live.plan.restore_mask==right_hand);
    assert(live.policy.bound_mask()==left_hand&&live.policy.authorize(live.plan,live.ctx,live.now)==left_hand);
    assert(live.policy.last_observe().mask(ObserveReason::tracking_loss)==right_hand);
    assert(live.policy.last_observe().bound_before==3&&live.policy.last_observe().bound_after==left_hand);
    live.receipt.valid_mask=both_hands;
    auto stale=live.policy.observe(live.ctx,&live.receipt,live.now);
    assert(stale.accept_mask==0&&stale.bind_mask==0&&live.policy.bound_mask()==left_hand);
    live.feed(4,4,both_hands);
    assert(live.plan.accept_mask==3&&live.plan.bind_mask==right_hand);
    assert(live.policy.authorize(live.plan,live.ctx,live.now)==left_hand);
    assert(!live.policy.commit_binding(live.plan,3)); // Already-bound left cannot be re-committed.
    assert(live.policy.commit_binding(live.plan,right_hand));
    assert(live.policy.authorize(live.plan,live.ctx,live.now)==3);
    live.feed(5,5,0);
    assert(live.plan.accept_mask==0&&live.plan.restore_mask==3&&live.policy.bound_mask()==0);
    assert(live.policy.last_observe().mask(ObserveReason::tracking_loss)==3);
    live.feed(6,6,3);
    assert(live.plan.accept_mask==3&&live.plan.bind_mask==3&&live.policy.commit_binding(live.plan,3));
}
void missing_baseline_and_readiness_boundaries(){
    Policy policy;auto c=context();auto p=policy.observe(c,nullptr,1000);
    assert(p.accept_mask==0&&p.reset_binding_mask==3);
    auto t=ticket(c,1,1,1000);p=policy.observe(c,&t,1000);
    assert(p.accept_mask==0&&p.baseline_mask==3);
    t.serial=t.sequence=2;p=policy.observe(c,&t,1000);
    assert(p.bind_mask==3&&policy.commit_binding(p,3));
    c.idle_ready=false;t.serial=t.sequence=3;p=policy.observe(c,&t,1000);
    assert(p.accept_mask==0&&p.restore_mask==3&&policy.bound_mask()==0);
    assert(policy.last_observe().mask(ObserveReason::context_readiness)==3);
    assert(policy.last_observe().mask(ObserveReason::context_identity)==0);
    assert(policy.last_observe().mask(ObserveReason::not_ready)==3);
    c.idle_ready=true;p=policy.observe(c,nullptr,1000);
    assert(p.accept_mask==0&&p.restore_mask==3);
    p=policy.observe(c,&t,1000);assert(p.accept_mask==0&&p.baseline_mask==0); // During-action receipt cannot be reused.
    t.serial=t.sequence=4;p=policy.observe(c,&t,1000);
    assert(p.accept_mask==0&&p.baseline_mask==3);
    t.serial=t.sequence=5;p=policy.observe(c,&t,1000);
    assert(p.bind_mask==3&&policy.commit_binding(p,3)&&policy.authorize(p,c,1000)==3);
    c.clean=false;t.serial=t.sequence=6;p=policy.observe(c,&t,1000);
    assert(p.accept_mask==0&&p.restore_mask==3&&policy.bound_mask()==0);
    c.clean=true;t.serial=t.sequence=7;p=policy.observe(c,&t,1000);
    assert(p.accept_mask==0&&p.baseline_mask==3);
    t.serial=t.sequence=8;p=policy.observe(c,&t,1000);
    assert(p.bind_mask==3&&policy.commit_binding(p,3));
}
void game_and_action_changes_seal_the_current_receipt(){
    for(unsigned field=0;field<7;++field){
        Live live;
        switch(field){
        case 0:++live.ctx.game.world;break;case 1:++live.ctx.game.controller;break;
        case 2:++live.ctx.game.pawn;break;case 3:++live.ctx.game.mesh;break;
        case 4:++live.ctx.game.weapon;break;case 5:++live.ctx.action_epoch;break;
        case 6:++live.ctx.expected_native_world_generation;break;
        }
        live.feed(3,3);
        assert(live.plan.accept_mask==0&&live.plan.baseline_mask==3&&live.plan.restore_mask==3);
        assert(live.policy.bound_mask()==0&&!live.policy.commit_binding(live.plan,3));
        assert(live.policy.last_observe().mask(ObserveReason::context_identity)==3);
        live.feed(4,4);assert(live.plan.bind_mask==3&&live.policy.commit_binding(live.plan,3));
    }
    Live live;++live.ctx.action_epoch;
    auto p=live.policy.observe(live.ctx,nullptr,live.now);
    assert(p.restore_mask==3&&p.accept_mask==0);
    live.feed(3,3);assert(live.plan.accept_mask==0&&live.plan.baseline_mask==3);
    live.feed(4,4);assert(live.plan.bind_mask==3);
}
void each_stream_key_change_allows_new_counters_but_only_after_baseline(){
    for(unsigned field=0;field<4;++field){
        Live live;auto next=live.receipt;next.serial=next.sequence=1;
        switch(field){
        case 0:++next.stream.session;break;case 1:++next.stream.space_epoch;break;
        case 2:++next.stream.input_epoch;break;
        case 3:++next.stream.world_generation;live.ctx.expected_native_world_generation=next.stream.world_generation;break;
        }
        auto p=live.policy.observe(live.ctx,&next,live.now);
        assert(p.accept_mask==0&&p.baseline_mask==3&&p.restore_mask==3);
        assert(live.policy.last_observe().mask(ObserveReason::stream_changed)==3);
        next.serial=next.sequence=2;p=live.policy.observe(live.ctx,&next,live.now);
        assert(p.accept_mask==3&&p.bind_mask==3&&live.policy.commit_binding(p,3));
    }
    Live live;auto wrong=live.receipt;wrong.serial=wrong.sequence=3;++wrong.stream.world_generation;
    auto p=live.policy.observe(live.ctx,&wrong,live.now);
    assert(p.accept_mask==0&&p.restore_mask==3&&live.policy.bound_mask()==0);
    assert(live.policy.last_observe().mask(ObserveReason::ticket_world)==3);
    Live invalid;auto bad=invalid.receipt;bad.serial=bad.sequence=3;bad.stream.session=0;
    p=invalid.policy.observe(invalid.ctx,&bad,invalid.now);
    assert(p.accept_mask==0&&p.restore_mask==3);
    assert(invalid.policy.last_observe().mask(ObserveReason::stream_invalid)==3);
    assert(invalid.policy.last_observe().mask(ObserveReason::ticket_shape)==0); // Earlier stream branch.
}
void rejected_receipts_never_become_fresh_later(){
    Live live;auto future=live.receipt;future.serial=future.sequence=3;future.created_ms=live.now+10;
    auto p=live.policy.observe(live.ctx,&future,live.now);
    assert(p.accept_mask==0&&p.restore_mask==3);
    assert(live.policy.last_observe().mask(ObserveReason::ticket_age)==3);
    assert(live.policy.last_observe().new_ticket_age.future&&live.policy.last_observe().new_ticket_age.ms==10);
    live.now=future.created_ms;live.ctx.observed_ms=live.now;
    p=live.policy.observe(live.ctx,&future,live.now);assert(p.accept_mask==0&&p.baseline_mask==0);
    assert(live.policy.last_observe().mask(ObserveReason::not_newer)==3);
    assert(live.policy.last_observe().mask(ObserveReason::ticket_age)==0);
    live.feed(4,4);assert(live.plan.accept_mask==0&&live.plan.baseline_mask==3);
    live.feed(5,5);assert(live.plan.bind_mask==3&&live.policy.commit_binding(live.plan,3));
    auto wrong=live.receipt;wrong.serial=wrong.sequence=6;++wrong.controller;
    p=live.policy.observe(live.ctx,&wrong,live.now);assert(p.accept_mask==0&&p.restore_mask==3);
    assert(live.policy.last_observe().mask(ObserveReason::ticket_owner)==3);
    wrong.controller=live.ctx.game.controller;
    p=live.policy.observe(live.ctx,&wrong,live.now);assert(p.accept_mask==0&&p.baseline_mask==0);
    // The first receipt of a different stream is consumed even when rejected.
    ++wrong.stream.session;wrong.serial=wrong.sequence=1;wrong.units_per_metre=0;
    p=live.policy.observe(live.ctx,&wrong,live.now);assert(p.accept_mask==0);
    wrong.units_per_metre=100.;p=live.policy.observe(live.ctx,&wrong,live.now);
    assert(p.accept_mask==0&&p.baseline_mask==0);
    wrong.serial=wrong.sequence=2;p=live.policy.observe(live.ctx,&wrong,live.now);
    assert(p.accept_mask==0&&p.baseline_mask==3);
}
void age_boundaries_and_long_gap_cannot_teleport_old_binding(){
    Live live;const auto acquired=live.now;
    auto current=live.ctx;current.observed_ms=acquired+100;
    assert(live.policy.authorize(live.plan,current,acquired+100)==3);
    current.observed_ms=acquired+101;
    assert(live.policy.authorize(live.plan,current,acquired+101)==0);
    live.ctx.observed_ms=acquired+100;
    auto p=live.policy.observe(live.ctx,nullptr,acquired+100);
    assert(p.accept_mask==0&&p.restore_mask==3&&p.reset_binding_mask==0&&live.policy.bound_mask()==3);
    assert(live.policy.last_observe().old_ticket_age[0].known&&live.policy.last_observe().old_ticket_age[0].ms==100);
    assert(live.policy.last_observe().mask(ObserveReason::binding_age)==0);
    live.ctx.observed_ms=acquired+101;
    auto fresh=ticket(live.ctx,3,3,acquired+101);
    p=live.policy.observe(live.ctx,&fresh,acquired+101);
    assert(p.accept_mask==0&&p.baseline_mask==3&&p.restore_mask==3&&live.policy.bound_mask()==0);
    const auto expired=live.policy.last_observe();
    assert(expired.mask(ObserveReason::binding_age)==3&&expired.bound_before==3&&expired.bound_after==0);
    assert(expired.old_ticket_age[0].ms==101&&expired.old_ticket_age[1].ms==101);
    assert(!expired.old_ticket_age[0].future&&expired.new_ticket_age.ms==0&&expired.context_age.ms==0);
    fresh.serial=fresh.sequence=4;p=live.policy.observe(live.ctx,&fresh,acquired+101);
    assert(p.bind_mask==3&&live.policy.commit_binding(p,3));
    // An arriving fresh receipt does not bypass expiry when there was no
    // intermediate observe(nullptr) call during the long gap.
    Live silent;silent.now+=101;silent.ctx.observed_ms=silent.now;
    fresh=ticket(silent.ctx,3,3,silent.now);p=silent.policy.observe(silent.ctx,&fresh,silent.now);
    assert(p.accept_mask==0&&p.baseline_mask==3&&p.restore_mask==3);
    assert(silent.policy.last_observe().mask(ObserveReason::binding_age)==3);
    assert(silent.policy.last_observe().previous_context_age.ms==101);
    Live one;one.feed(3,3,left_hand);one.now+=101;one.ctx.observed_ms=one.now;
    fresh=ticket(one.ctx,4,4,one.now);fresh.valid_mask=left_hand;
    p=one.policy.observe(one.ctx,&fresh,one.now);
    assert(p.accept_mask==0&&one.policy.last_observe().mask(ObserveReason::binding_age)==left_hand);
    assert(one.policy.last_observe().old_ticket_age[0].known&&!one.policy.last_observe().old_ticket_age[1].known);
    // A receipt accepted at age100 retains created_ms, not its receive time:
    // the next fresh arrival one millisecond later must baseline the old binding.
    Live stamped;fresh=ticket(stamped.ctx,3,3,stamped.now-100);
    p=stamped.policy.observe(stamped.ctx,&fresh,stamped.now);assert(p.accept_mask==3);
    ++stamped.now;stamped.ctx.observed_ms=stamped.now;fresh=ticket(stamped.ctx,4,4,stamped.now);
    p=stamped.policy.observe(stamped.ctx,&fresh,stamped.now);
    assert(p.accept_mask==0&&p.baseline_mask==3&&stamped.policy.last_observe().old_ticket_age[0].ms==101);
    assert(stamped.policy.last_observe().mask(ObserveReason::binding_age)==3&&stamped.policy.last_observe().new_ticket_age.ms==0);
    // Clock reversal is observable without unsigned age underflow or any
    // change to the existing recent()/baseline behavior.
    Live backwards;--backwards.now;backwards.ctx.observed_ms=backwards.now;
    fresh=ticket(backwards.ctx,3,3,backwards.now);
    p=backwards.policy.observe(backwards.ctx,&fresh,backwards.now);
    assert(p.accept_mask==0&&p.baseline_mask==3&&p.restore_mask==3);
    assert(backwards.policy.last_observe().mask(ObserveReason::binding_age)==3);
    assert(backwards.policy.last_observe().old_ticket_age[0].future&&backwards.policy.last_observe().old_ticket_age[0].ms==1);
    assert(backwards.policy.last_observe().previous_context_age.future);
}
void game_age_pose_age_and_future_context_are_independent(){
    Live live;auto t=live.receipt;t.serial=t.sequence=3;
    t.created_ms=live.now-100;auto p=live.policy.observe(live.ctx,&t,live.now);
    assert(p.accept_mask==3&&live.policy.authorize(p,live.ctx,live.now)==3);
    assert(live.policy.last_observe().new_ticket_age.ms==100&&live.policy.last_observe().mask(ObserveReason::ticket_age)==0);
    t.serial=t.sequence=4;t.created_ms=live.now-101;
    p=live.policy.observe(live.ctx,&t,live.now);assert(p.accept_mask==0&&p.restore_mask==3);
    assert(live.policy.last_observe().new_ticket_age.ms==101&&live.policy.last_observe().mask(ObserveReason::ticket_age)==3);
    Live stale_game;stale_game.now+=100;
    t=ticket(stale_game.ctx,3,3,stale_game.now);
    p=stale_game.policy.observe(stale_game.ctx,&t,stale_game.now);
    assert(p.accept_mask==3); // Exactly 100 ms old game facts still qualify.
    assert(stale_game.policy.last_observe().context_age.ms==100&&!stale_game.policy.last_observe().context_age.future);
    ++stale_game.now;t=ticket(stale_game.ctx,4,4,stale_game.now);
    p=stale_game.policy.observe(stale_game.ctx,&t,stale_game.now);
    assert(p.accept_mask==0&&p.restore_mask==3); // Fresh pose cannot rescue old game facts.
    assert(stale_game.policy.last_observe().context_age.ms==101&&stale_game.policy.last_observe().new_ticket_age.ms==0);
    assert(stale_game.policy.last_observe().mask(ObserveReason::not_ready)==3);
    Live future_game;future_game.ctx.observed_ms=future_game.now+1;
    t=ticket(future_game.ctx,3,3,future_game.now);
    p=future_game.policy.observe(future_game.ctx,&t,future_game.now);
    assert(p.accept_mask==0&&p.restore_mask==3);
    assert(future_game.policy.last_observe().context_age.future&&future_game.policy.last_observe().context_age.ms==1);
    assert(future_game.policy.last_observe().mask(ObserveReason::not_ready)==3);
}
void scale_changes_and_invalid_flags_fail_closed(){
    Live live;auto t=live.receipt;t.serial=t.sequence=3;
    t.units_per_metre=std::nextafter(100.,101.);
    auto p=live.policy.observe(live.ctx,&t,live.now);
    assert(p.accept_mask==0&&p.baseline_mask==3&&p.restore_mask==3);
    assert(live.policy.last_observe().mask(ObserveReason::scale_changed)==3);
    t.serial=t.sequence=4;p=live.policy.observe(live.ctx,&t,live.now);
    assert(p.bind_mask==3&&live.policy.commit_binding(p,3));
    for(double scale:{0.,-1.,std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::quiet_NaN()}){
        Live invalid;auto bad=invalid.receipt;bad.serial=bad.sequence=3;bad.units_per_metre=scale;
        p=invalid.policy.observe(invalid.ctx,&bad,invalid.now);
        assert(p.accept_mask==0&&p.restore_mask==3&&invalid.policy.bound_mask()==0);
        assert(invalid.policy.last_observe().mask(ObserveReason::ticket_shape)==3);
        assert(invalid.policy.last_observe().mask(ObserveReason::scale_changed)==0);
    }
    for(unsigned mask:{4u,7u,~0u}){
        Live invalid;invalid.feed(3,3,mask);
        assert(invalid.plan.accept_mask==0&&invalid.plan.restore_mask==3);
        assert(invalid.policy.last_observe().mask(ObserveReason::ticket_shape)==3);
    }
    Live invalid;invalid.ctx.game.mesh=0;invalid.feed(3,3);
    assert(invalid.plan.accept_mask==0&&invalid.plan.restore_mask==3);
}
void continuity_is_historical_and_returned_by_value(){
    Policy empty;const auto unknown=empty.continuity_snapshot(1000);
    assert(!unknown.context_known&&!unknown.stream_known&&!unknown.ready&&unknown.bound_mask==0);
    assert(unknown.serial==0&&unknown.sequence==0&&unknown.units_per_metre==0.);
    assert(!unknown.context_age.known&&!unknown.old_ticket_age[0].known&&!unknown.old_ticket_age[1].known);
    assert(unknown.source_stamp[0]==0&&unknown.source_stamp[1]==0);
    Live live;const Policy& policy=live.policy;
    assert(policy.authorize(live.plan,live.ctx,live.now)==3);
    auto snapshot=policy.continuity_snapshot(live.now+101);
    assert(snapshot.context_known&&snapshot.stream_known&&snapshot.ready&&snapshot.bound_mask==3);
    assert(same(snapshot.context.game,live.ctx.game)&&snapshot.context.observed_ms==live.now);
    assert(snapshot.context.action_epoch==live.ctx.action_epoch&&snapshot.context.clean&&snapshot.context.idle_ready);
    assert(snapshot.context.expected_native_world_generation==live.ctx.expected_native_world_generation);
    assert(same(snapshot.stream,live.receipt.stream)&&snapshot.serial==2&&snapshot.sequence==2);
    assert(snapshot.units_per_metre==100.&&snapshot.context_age.ms==101);
    // Mutating the returned value cannot alter stored identities, cursors,
    // readiness, binding or the still-current plan's authorization.
    snapshot.context.game.mesh=0;snapshot.context.observed_ms=0;
    snapshot.context.clean=false;snapshot.stream.session=0;
    snapshot.serial=snapshot.sequence=999;snapshot.units_per_metre=1.;
    snapshot.bound_mask=0;snapshot.ready=snapshot.context_known=snapshot.stream_known=false;
    snapshot.old_ticket_age[0].ms=0;snapshot.context_age.ms=0;
    const auto intact=policy.continuity_snapshot(live.now);
    assert(intact.context_known&&intact.stream_known&&intact.ready&&intact.bound_mask==3);
    assert(same(intact.context.game,live.ctx.game)&&intact.context.clean&&intact.context.observed_ms==live.now);
    assert(same(intact.stream,live.receipt.stream)&&intact.serial==2&&intact.sequence==2&&intact.units_per_metre==100.);
    assert(intact.old_ticket_age[0].ms==0&&intact.context_age.ms==0);
    assert(intact.source_stamp[0]==live.now&&intact.source_stamp[1]==live.now);
    assert(policy.authorize(live.plan,live.ctx,live.now)==3);
    live.ctx.clean=false;live.feed(3,3);
    const auto blocked=policy.continuity_snapshot(live.now);
    assert(blocked.context_known&&blocked.stream_known&&!blocked.ready&&!blocked.context.clean&&blocked.bound_mask==0);
    assert(!blocked.old_ticket_age[0].known&&!blocked.old_ticket_age[1].known);
}
void continuity_queries_do_not_renew_age_or_revoke_plans(){
    Live live;const auto acquired=live.now;
    const auto observed=live.policy.last_observe();
    auto current=live.ctx;current.observed_ms=acquired+100;
    assert(live.policy.authorize(live.plan,current,acquired+100)==3);
    const auto boundary=live.policy.continuity_snapshot(acquired+100);
    assert(boundary.context_age.known&&boundary.context_age.ms==100&&!boundary.context_age.future);
    for(const auto& age:boundary.old_ticket_age)assert(age.known&&age.ms==100&&!age.future);
    assert(live.policy.authorize(live.plan,current,acquired+100)==3);
    const auto expired=live.policy.continuity_snapshot(acquired+101);
    assert(expired.ready&&expired.bound_mask==3&&expired.context.observed_ms==acquired); // Historical only.
    assert(expired.context_age.ms==101&&!expired.context_age.future);
    for(const auto& age:expired.old_ticket_age)assert(age.known&&age.ms==101&&!age.future);
    current.observed_ms=acquired+101;
    assert(live.policy.authorize(live.plan,current,acquired+101)==0);
    assert(live.policy.authorize(live.plan,live.ctx,acquired)==3); // The query did not revoke it.
    const auto future=live.policy.continuity_snapshot(acquired-1);
    assert(future.context_age.known&&future.context_age.future&&future.context_age.ms==1);
    for(const auto& age:future.old_ticket_age)assert(age.known&&age.future&&age.ms==1);
    assert(live.policy.last_observe().observed_now==observed.observed_now);
    assert(live.policy.last_observe().old_ticket_age[0].ms==observed.old_ticket_age[0].ms);
    assert(live.policy.last_observe().old_ticket_age[0].known==observed.old_ticket_age[0].known);
    // None of the queries renewed last_present_ms_ or consumed the arriving
    // receipt: the original age101 binding must still take a fresh baseline.
    live.ctx.observed_ms=acquired+101;auto fresh=ticket(live.ctx,3,3,acquired+101);
    const auto plan=live.policy.observe(live.ctx,&fresh,acquired+101);
    assert(plan.accept_mask==0&&plan.baseline_mask==3&&plan.restore_mask==3);
    assert(live.policy.last_observe().mask(ObserveReason::binding_age)==3);
    assert(live.policy.last_observe().old_ticket_age[0].ms==101&&live.policy.last_observe().old_ticket_age[1].ms==101);
    fresh.serial=fresh.sequence=4;
    const auto next=live.policy.observe(live.ctx,&fresh,acquired+101);
    assert(next.accept_mask==3&&next.bind_mask==3&&live.policy.commit_binding(next,3));
}
void continuity_tracks_each_bound_hand_and_consumed_cursors(){
    Live live;live.feed(3,3,left_hand);
    const auto one=live.policy.continuity_snapshot(live.now+17);
    assert(one.bound_mask==left_hand&&one.old_ticket_age[0].known&&one.old_ticket_age[0].ms==17);
    assert(!one.old_ticket_age[0].future&&!one.old_ticket_age[1].known&&one.context_age.ms==17);
    assert(live.policy.authorize(live.plan,live.ctx,live.now)==left_hand);
    const auto future=live.policy.continuity_snapshot(live.now-1);
    assert(future.old_ticket_age[0].known&&future.old_ticket_age[0].future&&future.old_ticket_age[0].ms==1);
    assert(!future.old_ticket_age[1].known&&future.context_age.future);
    live.feed(4,4);const auto uncommitted=live.policy.continuity_snapshot(live.now);
    assert(uncommitted.bound_mask==left_hand&&!uncommitted.old_ticket_age[1].known);
    assert(live.policy.commit_binding(live.plan,right_hand)); // Query preserved the current bind plan.
    const auto both=live.policy.continuity_snapshot(live.now);
    assert(both.bound_mask==3&&both.old_ticket_age[1].known&&both.old_ticket_age[1].ms==0);
    live.feed(5,4);assert(live.plan.accept_mask==0);
    const auto consumed=live.policy.continuity_snapshot(live.now);
    assert(consumed.serial==5&&consumed.sequence==4&&consumed.bound_mask==3);
    live.feed(5,5);assert(live.plan.accept_mask==0); // Rejected serial5 remains consumed.
    live.feed(6,6);assert(live.plan.accept_mask==3&&live.plan.bind_mask==0);
}
void transport_revokes_old_plans_without_granting_binding_or_writes(){
    Live live;live.feed(3,3,left_hand);live.feed(4,4);
    const auto old=live.plan;const auto before=live.policy.continuity_snapshot(live.now);
    assert(old.bind_mask==right_hand&&live.policy.authorize(old,live.ctx,live.now)==left_hand);
    auto fresh=ticket(live.ctx,5,5,live.now+1);
    const auto result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now+1);
    assert(result.refreshed_mask==left_hand&&result.reset_binding_mask==0&&result.reason==ObserveReason::count);
    assert(!result.capacity_exhausted&&live.policy.bound_mask()==left_hand);
    assert(live.policy.authorize(old,live.ctx,live.now)==0&&!live.policy.commit_binding(old,right_hand));
    const auto after=live.policy.continuity_snapshot(live.now+1);
    assert(after.serial==before.serial&&after.sequence==before.sequence&&same(after.stream,before.stream));
    assert(after.context.observed_ms==before.context.observed_ms&&after.ready==before.ready);
    assert(after.units_per_metre==before.units_per_metre&&after.context.action_epoch==before.context.action_epoch);
    assert(after.source_stamp[0]==fresh.created_ms&&after.old_ticket_age[0].ms==0);
    assert(after.source_stamp[1]==0&&!after.old_ticket_age[1].known);
    assert(live.policy.last_observe().observed_now==live.now); // Transport is not an observe.
    // A naturally refreshed Context time with the same identity/readiness
    // must preserve the exception through the original Tick's elapsed time.
    live.ctx.observed_ms=live.now+60;
    auto plan=live.policy.observe(live.ctx,&fresh,live.now+60);
    assert(plan.accept_mask==3&&plan.bind_mask==right_hand&&live.policy.commit_binding(plan,right_hand));
    assert(live.policy.authorize(plan,live.ctx,live.now+60)==3);
    plan=live.policy.observe(live.ctx,&fresh,live.now+60);
    assert(plan.accept_mask==0&&live.policy.last_observe().mask(ObserveReason::not_newer)==3);
}
void transport_old_and_new_age_limits_are_independent(){
    for(const std::uint64_t age:{100u,101u}){
        Live live;const auto now=live.now+age;auto fresh=ticket(live.ctx,3,3,now);
        const auto result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,now);
        if(age==100){
            assert(result.refreshed_mask==3&&result.reset_binding_mask==0&&result.reason==ObserveReason::count);
            const auto sampled=live.policy.continuity_snapshot(now);
            assert(sampled.source_stamp[0]==now&&sampled.old_ticket_age[0].ms==0);
        }else{
            assert(result.refreshed_mask==0&&result.reset_binding_mask==3&&result.reason==ObserveReason::binding_age);
            assert(live.policy.bound_mask()==0);
            live.ctx.observed_ms=now;auto plan=live.policy.observe(live.ctx,&fresh,now);
            assert(plan.accept_mask==0&&plan.baseline_mask==3&&!live.policy.commit_binding(plan,3));
            fresh.serial=fresh.sequence=4;plan=live.policy.observe(live.ctx,&fresh,now);
            assert(plan.bind_mask==3&&live.policy.commit_binding(plan,3));
        }
    }
    Live stamped;const auto original=stamped.now;auto fresh=ticket(stamped.ctx,3,3,original);
    auto result=stamped.policy.refresh_bound_transport(fresh,stamped.ctx.action_epoch,original+100);
    assert(result.refreshed_mask==3&&stamped.policy.continuity_snapshot(original+100).source_stamp[0]==original);
    fresh=ticket(stamped.ctx,4,4,original+101);
    result=stamped.policy.refresh_bound_transport(fresh,stamped.ctx.action_epoch,original+101);
    assert(result.refreshed_mask==0&&result.reset_binding_mask==3&&result.reason==ObserveReason::binding_age);
    Live old_receipt;fresh=ticket(old_receipt.ctx,3,3,old_receipt.now-1);
    result=old_receipt.policy.refresh_bound_transport(fresh,old_receipt.ctx.action_epoch,old_receipt.now+100);
    assert(result.refreshed_mask==0&&result.reset_binding_mask==3&&result.reason==ObserveReason::ticket_age);
    Live late;fresh=ticket(late.ctx,3,3,late.now);
    assert(late.policy.refresh_bound_transport(fresh,late.ctx.action_epoch,late.now).refreshed_mask==3);
    late.ctx.observed_ms=late.now+101;
    const auto plan=late.policy.observe(late.ctx,&fresh,late.now+101);
    assert(plan.accept_mask==0&&late.policy.last_observe().mask(ObserveReason::ticket_age)==3);
}
void transport_future_and_backward_stamps_cannot_cross_into_writer(){
    Live future;auto fresh=ticket(future.ctx,3,3,future.now+1);
    auto result=future.policy.refresh_bound_transport(fresh,future.ctx.action_epoch,future.now);
    assert(result.refreshed_mask==0&&result.reset_binding_mask==3&&result.reason==ObserveReason::ticket_age);
    ++future.now;future.ctx.observed_ms=future.now;
    auto plan=future.policy.observe(future.ctx,&fresh,future.now);
    assert(plan.accept_mask==0&&plan.baseline_mask==0&&future.policy.last_observe().mask(ObserveReason::not_newer)==3);
    result=future.policy.refresh_bound_transport(fresh,future.ctx.action_epoch,future.now);
    assert(result.refreshed_mask==0&&result.reason==ObserveReason::not_newer);
    fresh.serial=fresh.sequence=4;plan=future.policy.observe(future.ctx,&fresh,future.now);
    assert(plan.baseline_mask==3&&plan.accept_mask==0);
    fresh.serial=fresh.sequence=5;plan=future.policy.observe(future.ctx,&fresh,future.now);
    assert(plan.bind_mask==3&&future.policy.commit_binding(plan,3));
    Live backwards;fresh=ticket(backwards.ctx,3,3,backwards.now-1);
    result=backwards.policy.refresh_bound_transport(fresh,backwards.ctx.action_epoch,backwards.now-1);
    assert(result.refreshed_mask==0&&result.reset_binding_mask==3&&result.reason==ObserveReason::binding_age);
    Live rewind;fresh=ticket(rewind.ctx,3,3,rewind.now+5);
    assert(rewind.policy.refresh_bound_transport(fresh,rewind.ctx.action_epoch,rewind.now+5).refreshed_mask==3);
    rewind.ctx.observed_ms=rewind.now+4;
    plan=rewind.policy.observe(rewind.ctx,&fresh,rewind.now+4);
    assert(plan.accept_mask==0&&rewind.policy.last_observe().mask(ObserveReason::ticket_age)==3);
    rewind.ctx.observed_ms=rewind.now+5;plan=rewind.policy.observe(rewind.ctx,&fresh,rewind.now+5);
    assert(plan.accept_mask==0&&rewind.policy.last_observe().mask(ObserveReason::not_newer)==3);
    Live regressing;const auto start=regressing.now;fresh=ticket(regressing.ctx,3,3,start+5);
    result=regressing.policy.refresh_bound_transport(fresh,regressing.ctx.action_epoch,start+10);
    assert(result.refreshed_mask==3&&regressing.policy.continuity_snapshot(start+10).source_stamp[0]==start+5);
    fresh=ticket(regressing.ctx,4,4,start+4);
    result=regressing.policy.refresh_bound_transport(fresh,regressing.ctx.action_epoch,start+11);
    assert(result.refreshed_mask==0&&result.reset_binding_mask==3&&result.reason==ObserveReason::ticket_age);
    fresh.created_ms=start+11;regressing.ctx.observed_ms=start+11;
    plan=regressing.policy.observe(regressing.ctx,&fresh,start+11);
    assert(plan.accept_mask==0&&plan.baseline_mask==0); // Correcting the stamp does not resurrect cursor4.
    Live writer;fresh=ticket(writer.ctx,3,3,writer.now+5);
    assert(writer.policy.refresh_bound_transport(fresh,writer.ctx.action_epoch,writer.now+5).refreshed_mask==3);
    writer.ctx.observed_ms=writer.now+6;fresh=ticket(writer.ctx,4,4,writer.now+4);
    plan=writer.policy.observe(writer.ctx,&fresh,writer.now+6);
    assert(plan.accept_mask==0&&writer.policy.last_observe().mask(ObserveReason::ticket_age)==3);
}
void transport_duplicates_and_partial_cursors_do_not_renew(){
    Live live;const auto start=live.now;auto fresh=ticket(live.ctx,3,3,start+10);
    assert(live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,start+10).refreshed_mask==3);
    auto result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,start+11);
    assert(result.refreshed_mask==0&&result.reset_binding_mask==0&&result.reason==ObserveReason::not_newer);
    assert(live.policy.continuity_snapshot(start+11).source_stamp[0]==start+10);
    fresh.valid_mask=0;
    result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,start+11);
    assert(result.refreshed_mask==0&&result.reset_binding_mask==0&&result.reason==ObserveReason::not_newer);
    assert(live.policy.bound_mask()==3); // A non-newer loss payload has no tracking authority.
    fresh=ticket(live.ctx,4,3,start+12);
    assert(live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,start+12).reason==ObserveReason::not_newer);
    fresh=ticket(live.ctx,4,4,start+13);
    assert(live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,start+13).reason==ObserveReason::not_newer);
    fresh=ticket(live.ctx,5,4,start+14);
    assert(live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,start+14).reason==ObserveReason::not_newer);
    assert(live.policy.continuity_snapshot(start+14).source_stamp[0]==start+10);
    fresh=ticket(live.ctx,6,5,start+15);
    result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,start+15);
    assert(result.refreshed_mask==3&&result.reason==ObserveReason::count);
    const auto state=live.policy.continuity_snapshot(start+15);
    assert(state.serial==2&&state.sequence==2&&state.source_stamp[0]==start+15);
}
void transport_and_writer_cursors_are_separate_and_one_shot(){
    Live live;live.feed(3,3);auto fresh=live.receipt;
    auto result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now);
    assert(result.refreshed_mask==0&&result.reason==ObserveReason::not_newer);
    fresh=ticket(live.ctx,4,4,live.now+1);
    assert(live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now+1).refreshed_mask==3);
    assert(live.policy.continuity_snapshot(live.now+1).serial==3);
    // Re-reading a legal transport frame preserves its single writer exception.
    result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now+1);
    assert(result.reason==ObserveReason::not_newer&&result.reset_binding_mask==0);
    live.ctx.observed_ms=live.now+1;auto plan=live.policy.observe(live.ctx,&fresh,live.now+1);
    assert(plan.accept_mask==3&&plan.bind_mask==0&&live.policy.authorize(plan,live.ctx,live.now+1)==3);
    plan=live.policy.observe(live.ctx,&fresh,live.now+1);assert(plan.accept_mask==0);
    result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now+1);
    assert(result.refreshed_mask==0&&result.reason==ObserveReason::not_newer);
    fresh=ticket(live.ctx,5,5,live.now+2);live.ctx.observed_ms=live.now+2;
    plan=live.policy.observe(live.ctx,&fresh,live.now+2);assert(plan.accept_mask==3);
    result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now+3);
    assert(result.refreshed_mask==0&&result.reason==ObserveReason::not_newer);
    assert(live.policy.continuity_snapshot(live.now+3).source_stamp[0]==fresh.created_ms);
}
void transport_tracking_loss_needs_fresh_writer_baseline_and_binding(){
    Live live;const auto now=live.now;auto loss=ticket(live.ctx,5,5,now+1);loss.valid_mask=left_hand;
    auto result=live.policy.refresh_bound_transport(loss,live.ctx.action_epoch,now+1);
    assert(result.refreshed_mask==left_hand&&result.reset_binding_mask==right_hand&&result.reason==ObserveReason::tracking_loss);
    assert(live.policy.bound_mask()==left_hand);
    const auto one=live.policy.continuity_snapshot(now+1);
    assert(one.source_stamp[0]==now+1&&one.source_stamp[1]==0&&!one.old_ticket_age[1].known);
    result=live.policy.refresh_bound_transport(loss,live.ctx.action_epoch,now+2);
    assert(result.refreshed_mask==0&&result.reset_binding_mask==0&&result.reason==ObserveReason::not_newer);
    // An older writer frame with both hands cannot overwrite the newer loss.
    auto old=ticket(live.ctx,4,4,now+1);live.ctx.observed_ms=now+2;
    auto plan=live.policy.observe(live.ctx,&old,now+2);
    assert(plan.accept_mask==0&&plan.baseline_mask==0&&live.policy.bound_mask()==left_hand);
    auto returned=ticket(live.ctx,6,6,now+3);
    result=live.policy.refresh_bound_transport(returned,live.ctx.action_epoch,now+3);
    assert(result.refreshed_mask==left_hand&&result.reset_binding_mask==0&&live.policy.bound_mask()==left_hand);
    live.ctx.observed_ms=now+3;plan=live.policy.observe(live.ctx,&returned,now+3);
    assert(plan.baseline_mask==right_hand&&plan.accept_mask==left_hand&&plan.bind_mask==0);
    returned.serial=returned.sequence=7;plan=live.policy.observe(live.ctx,&returned,now+3);
    assert(plan.accept_mask==3&&plan.bind_mask==right_hand&&live.policy.commit_binding(plan,right_hand));
    // When the writer itself sees the loss, transport must not alter last_valid_
    // or consume that baseline before the writer seals it.
    Live observed;loss=ticket(observed.ctx,3,3,observed.now);loss.valid_mask=left_hand;
    assert(observed.policy.refresh_bound_transport(loss,observed.ctx.action_epoch,observed.now).reset_binding_mask==right_hand);
    plan=observed.policy.observe(observed.ctx,&loss,observed.now);
    assert(plan.baseline_mask==right_hand&&plan.accept_mask==left_hand);
    returned=ticket(observed.ctx,4,4,observed.now);
    plan=observed.policy.observe(observed.ctx,&returned,observed.now);
    assert(plan.bind_mask==right_hand&&observed.policy.commit_binding(plan,right_hand));
}
void transport_global_mismatches_revoke_and_rejections_are_sticky(){
    for(unsigned field=0;field<9;++field){
        Live live;auto bad=ticket(live.ctx,3,3,live.now);auto action=live.ctx.action_epoch;
        ObserveReason expected=ObserveReason::ticket_owner;
        switch(field){
        case 0:++bad.controller;break;case 1:++bad.pawn;break;case 2:++bad.weapon;break;
        case 3:bad.units_per_metre=std::nextafter(100.,101.);expected=ObserveReason::scale_changed;break;
        case 4:bad.valid_mask=4;expected=ObserveReason::ticket_shape;break;
        case 5:++action;expected=ObserveReason::context_identity;break;
        case 6:++bad.stream.session;expected=ObserveReason::stream_changed;break;
        case 7:++bad.stream.world_generation;expected=ObserveReason::stream_changed;break;
        case 8:bad.stream.input_epoch=0;expected=ObserveReason::stream_invalid;break;
        }
        auto result=live.policy.refresh_bound_transport(bad,action,live.now);
        assert(result.refreshed_mask==0&&result.reset_binding_mask==3&&result.reason==expected&&live.policy.bound_mask()==0);
        if(field<6){
            auto corrected=ticket(live.ctx,3,3,live.now);
            result=live.policy.refresh_bound_transport(corrected,live.ctx.action_epoch,live.now);
            assert(result.refreshed_mask==0&&result.reason==ObserveReason::not_newer);
            const auto plan=live.policy.observe(live.ctx,&corrected,live.now);
            assert(plan.accept_mask==0&&plan.baseline_mask==0&&live.policy.last_observe().mask(ObserveReason::not_newer)==3);
        }
    }
    Live world;++world.ctx.expected_native_world_generation;
    world.policy.observe(world.ctx,nullptr,world.now);
    auto bad=ticket(world.ctx,3,3,world.now);bad.stream=world.receipt.stream;
    assert(world.policy.refresh_bound_transport(bad,world.ctx.action_epoch,world.now).reason==ObserveReason::ticket_world);
    Policy unknown;auto c=context();bad=ticket(c,1,1,1000);
    assert(unknown.refresh_bound_transport(bad,c.action_epoch,1000).reason==ObserveReason::context_identity);
}
void transport_context_transitions_clear_exception_but_not_fences(){
    for(unsigned change=0;change<4;++change){
        Live live;auto fresh=ticket(live.ctx,3,3,live.now);
        assert(live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now).refreshed_mask==3);
        switch(change){
        case 0:++live.ctx.action_epoch;break;case 1:++live.ctx.game.mesh;break;
        case 2:live.ctx.clean=false;break;case 3:live.ctx.idle_ready=false;break;
        }
        live.policy.observe(live.ctx,nullptr,live.now);
        if(change>=2){
            auto rejected=ticket(live.ctx,4,4,live.now);
            const auto result=live.policy.refresh_bound_transport(rejected,live.ctx.action_epoch,live.now);
            assert(result.refreshed_mask==0&&result.reason==ObserveReason::not_ready);
            live.ctx.clean=live.ctx.idle_ready=true;
        }
        const auto plan=live.policy.observe(live.ctx,&fresh,live.now);
        assert(plan.accept_mask==0&&plan.baseline_mask==0&&live.policy.last_observe().mask(ObserveReason::not_newer)==3);
    }
}
void transport_stream_fences_survive_regular_adoption_and_return(){
    Live live;const auto original=live.receipt.stream;auto next=ticket(live.ctx,7,7,live.now);++next.stream.session;
    auto result=live.policy.refresh_bound_transport(next,live.ctx.action_epoch,live.now);
    assert(result.refreshed_mask==0&&result.reason==ObserveReason::stream_changed&&result.reset_binding_mask==3);
    auto snapshot=live.policy.continuity_snapshot(live.now);
    assert(same(snapshot.stream,original)&&snapshot.serial==2&&snapshot.sequence==2);
    auto plan=live.policy.observe(live.ctx,&next,live.now);
    assert(plan.accept_mask==0&&plan.baseline_mask==0&&live.policy.last_observe().mask(ObserveReason::not_newer)==3);
    next.serial=next.sequence=8;plan=live.policy.observe(live.ctx,&next,live.now);
    assert(plan.baseline_mask==3&&plan.accept_mask==0);
    next.serial=next.sequence=9;plan=live.policy.observe(live.ctx,&next,live.now);
    assert(plan.bind_mask==3&&live.policy.commit_binding(plan,3));
    assert(live.policy.refresh_bound_transport(next,live.ctx.action_epoch,live.now).reason==ObserveReason::not_newer);
    auto old=ticket(live.ctx,2,2,live.now);old.stream=original;
    plan=live.policy.observe(live.ctx,&old,live.now);
    assert(plan.accept_mask==0&&plan.baseline_mask==0); // Writer cursor2 was consumed before leaving this stream.
    assert(live.policy.refresh_bound_transport(old,live.ctx.action_epoch,live.now).reason==ObserveReason::not_newer);
    old.serial=old.sequence=3;plan=live.policy.observe(live.ctx,&old,live.now);
    assert(plan.baseline_mask==3&&plan.accept_mask==0);
    old.serial=old.sequence=4;plan=live.policy.observe(live.ctx,&old,live.now);
    assert(plan.bind_mask==3&&live.policy.commit_binding(plan,3));
}
void transport_capacity_never_evicts_or_recovers_permission(){
    Live live;auto fresh=ticket(live.ctx,3,3,live.now);
    assert(live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now).refreshed_mask==3);
    for(unsigned index=1;index<transport_stream_capacity;++index){
        fresh.stream.session=1+index;fresh.serial=fresh.sequence=3+index;
        const auto result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now);
        assert(!result.capacity_exhausted&&result.reason==ObserveReason::stream_changed);
    }
    fresh.stream.session=1+transport_stream_capacity;fresh.serial=fresh.sequence=100;
    auto result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now);
    assert(result.capacity_exhausted&&result.refreshed_mask==0&&result.reset_binding_mask==3&&result.reason==ObserveReason::not_newer);
    fresh=ticket(live.ctx,101,101,live.now);
    auto plan=live.policy.observe(live.ctx,&fresh,live.now);
    assert(plan.accept_mask==0&&plan.bind_mask==0&&!live.policy.commit_binding(plan,3));
    assert(live.policy.authorize(plan,live.ctx,live.now)==0&&live.policy.last_observe().mask(ObserveReason::not_newer)==3);
    result=live.policy.refresh_bound_transport(fresh,live.ctx.action_epoch,live.now);
    assert(result.capacity_exhausted&&result.refreshed_mask==0&&live.policy.bound_mask()==0);
}
} // namespace
int main(){
    baseline_and_per_hand_commit();no_receipt_never_reuses_pose_and_old_plans_die();
    plans_cannot_be_modified_or_used_after_new_observation();serial_and_sequence_each_strictly_advance();
    single_hand_loss_and_fresh_recovery();missing_baseline_and_readiness_boundaries();
    game_and_action_changes_seal_the_current_receipt();
    each_stream_key_change_allows_new_counters_but_only_after_baseline();
    rejected_receipts_never_become_fresh_later();age_boundaries_and_long_gap_cannot_teleport_old_binding();
    game_age_pose_age_and_future_context_are_independent();scale_changes_and_invalid_flags_fail_closed();
    continuity_is_historical_and_returned_by_value();continuity_queries_do_not_renew_age_or_revoke_plans();
    continuity_tracks_each_bound_hand_and_consumed_cursors();
    transport_revokes_old_plans_without_granting_binding_or_writes();
    transport_old_and_new_age_limits_are_independent();transport_future_and_backward_stamps_cannot_cross_into_writer();
    transport_duplicates_and_partial_cursors_do_not_renew();transport_and_writer_cursors_are_separate_and_one_shot();
    transport_tracking_loss_needs_fresh_writer_baseline_and_binding();
    transport_global_mismatches_revoke_and_rejections_are_sticky();transport_context_transitions_clear_exception_but_not_fences();
    transport_stream_fences_survive_regular_adoption_and_return();transport_capacity_never_evicts_or_recovers_permission();
    std::puts("hand stream ticket policy: 25 boundary scenarios passed");
}
