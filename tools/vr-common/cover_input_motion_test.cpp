// Actual controller adapter -> sole left-hand arbiter -> interaction -> ledger.
// Only the body's completion backend is an explicit owned-memory Host seam.
#define main ExistingInputTransactionMain
#include "cover_input_transaction_test.cpp"
#undef main
namespace motion=gears_cover_motion;
struct BodyHost final:motion::Backend {
    CoverInputOwner* input{};
    motion::Binding binding{{1,1,1,1,1,1,1},71,72};
    motion::Commit published{};
    motion::Offset body{};
    motion::Observation observation{motion::Observation::committed};
    motion::Admission admission{motion::Admission::current};
    unsigned observations{},admissions{},body_writes{};
    BodyHost(){published.binding=binding;published.event=10;published.version=1;
        published.body_publication=100;published.occupancy=101;}
    motion::Observation observe(std::uint64_t after,motion::Commit& out) noexcept override {
        ++observations;if(observation!=motion::Observation::committed)return observation;
        if(after==published.event)return motion::Observation::unchanged;
        out=published;return motion::Observation::committed;
    }
    motion::Admission admit(motion::Intent const& i,std::int64_t) noexcept override {
        ++admissions;if(admission!=motion::Admission::current)return admission;
        const auto actual=input->motion_candidate();
        return actual.available&&motion::detail::same(actual.binding,i.binding)&&motion::detail::same(actual.hand,i.hand)&&
            motion::detail::same(actual.requested,i.requested)&&motion::detail::same(body,i.from)?
            motion::Admission::current:motion::Admission::rejected;
    }
    bool consume(motion::Intent const& i,motion::Offset actual,motion::Completion completion=motion::Completion::accepted){
        if(!input->motion_current(i)||i.from_event!=published.event||i.from_version!=published.version||
           !motion::detail::same(i.from,body))return false;
        motion::Commit next{};next.binding=i.binding;next.event=published.event+1;next.previous_event=i.from_event;
        next.version=i.from_version+1;next.from_version=i.from_version;next.intent=i.id;
        next.body_publication=published.body_publication+1;next.occupancy=published.occupancy+1;
        next.completion=completion;next.from=i.from;next.requested=i.requested;next.position=actual;next.hand=i.hand;
        body=actual;++body_writes;published=next;return true;
    }
};
struct Deferred {
    BodyHost backend;
    Fixture f{&backend};
    CoverInputOwnerResult last{};
    Deferred(){backend.input=&f.kernel;f.frame.motion_binding=backend.binding;}
    CoverInputOwnerResult run(){last=f.run();return last;}
    CoverInputOwnerResult next(float squeeze){last=f.next(squeeze);return last;}
    void grasp(){run();C(next(.8f).interaction.owns_left_hand);}
    motion::Intent move(double up){
        f.hands.hands[0].grip.position.y=1.2-up;
        next(.8f);motion::Intent intent{};C(f.kernel.pending_motion(intent));return intent;
    }
};
static void waiting_release_and_actual_commit(){
    Deferred before;before.grasp();const auto pending=before.move(.3);
    C(close(before.last.interaction.offset.up,0)&&close(before.last.view.base_eye_world.z,-40));
    C(before.last.interaction.owns_left_hand&&before.backend.body_writes==0);
    C(!before.next(0).interaction.owns_left_hand);
    C(!before.backend.consume(pending,{0,.3})&&close(before.backend.body.up,0));
    C(close(before.last.interaction.offset.up,0));
    Deferred immediate;immediate.grasp();const auto old=immediate.move(.3);
    immediate.f.advance(0);const auto release=immediate.f.begin();
    C(release.accepted&&!release.left_grip_down&&!immediate.f.kernel.motion_current(old));
    C(!immediate.backend.consume(old,old.requested));
    C(immediate.f.kernel.update_with_decoded(release.ticket,absent(),immediate.f.owner).accepted);
    C(close(immediate.f.kernel.complete(release.ticket,true,immediate.f.owner).interaction.offset.up,0));

    Deferred after;after.grasp();const auto executed=after.move(.3);
    C(after.backend.consume(executed,executed.requested));
    C(after.f.kernel.motion_current(executed)); // cancellation state is not a second write permit
    C(!after.backend.consume(executed,executed.requested)); // real submitter's from-version already advanced
    C(close(after.last.interaction.offset.up,0));
    auto released=after.next(0);
    C(!released.interaction.owns_left_hand&&close(released.interaction.offset.up,.3));
    C(released.view_valid&&close(released.view.base_eye_world.z,-10));
    C(after.f.kernel.motion_committed().intent==executed.id);
    after.f.hands.hands[0].grip.position.y=4;released=after.next(0);
    C(close(released.interaction.offset.up,.3));
    C(after.next(.8f).interaction.owns_left_hand);
    after.f.hands.hands[0].grip.position.y+=.1;after.next(.8f);
    motion::Intent retract{};C(after.f.kernel.pending_motion(retract)&&close(retract.requested.up,.2));
}
static void waiting_keeps_stable_source_without_renewing_it(){
    Deferred f;f.grasp();const auto original=f.move(.3);
    for(unsigned i=0;i<3;++i){
        const auto r=f.next(.8f);motion::Intent waiting{};C(f.f.kernel.pending_motion(waiting));
        C(r.interaction.owns_left_hand&&close(r.interaction.offset.up,0));
        C(waiting.id==original.id&&waiting.hand.sample.observed_ns==original.hand.sample.observed_ns);
    }
    C(f.backend.consume(original,original.requested));
    C(close(f.next(.8f).interaction.offset.up,.3));
    Deferred expiry;expiry.grasp();const auto old=expiry.move(.3);
    for(unsigned i=0;i<12;++i)expiry.next(.8f);
    motion::Intent renewed{};C(expiry.f.kernel.pending_motion(renewed));
    C(renewed.id>old.id&&renewed.hand.sample.observed_ns>old.hand.sample.observed_ns);
    C(!expiry.f.kernel.motion_current(old)&&!expiry.backend.consume(old,{0,.3}));
}
static void rebase_only_completed_clips(){
    Deferred range;range.grasp();const auto limited=range.move(2.4);
    C(limited.hand.limit_clipped&&close(limited.requested.up,.7));
    C(close(range.last.interaction.offset.up,0));
    C(range.backend.consume(limited,limited.requested)); // exact execution of an input-limited request
    range.f.hands.hands[0].grip.position.y+=.01;range.next(.8f);
    motion::Intent reverse{};C(range.f.kernel.pending_motion(reverse));
    C(close(range.last.interaction.offset.up,.7)&&close(reverse.requested.up,.69));
    C(range.backend.consume(reverse,reverse.requested));
    C(close(range.next(0).interaction.offset.up,.69));

    Deferred collision;collision.grasp();const auto requested=collision.move(.3);
    C(collision.backend.consume(requested,{0,.2},motion::Completion::clipped));
    collision.f.hands.hands[0].grip.position.y+=.01;collision.next(.8f);
    C(collision.f.kernel.pending_motion(reverse));
    C(close(collision.last.interaction.offset.up,.2)&&close(reverse.requested.up,.19));
    Deferred at_limit;at_limit.grasp();const auto first=at_limit.move(2.4);
    C(at_limit.backend.consume(first,first.requested));at_limit.next(.8f);
    at_limit.f.hands.hands[0].grip.position.y-=.3;at_limit.next(.8f);
    C(!at_limit.f.kernel.pending_motion(reverse)&&at_limit.backend.body_writes==1);
    const auto version=at_limit.f.kernel.motion_committed().version;
    at_limit.f.hands.hands[0].grip.position.y+=.01;at_limit.next(.8f);
    C(at_limit.f.kernel.pending_motion(reverse)&&close(reverse.requested.up,.69));
    C(at_limit.f.kernel.motion_committed().version==version&&at_limit.backend.body_writes==1);
}
static void callback_completion_and_failed_weapon_transaction(){
    for(bool clipped:{false,true}){
        Deferred late;late.grasp();const auto first=late.move(.3);
        late.f.advance(.8f);const auto decoded=late.f.begin();
        C(late.f.kernel.update_with_decoded(decoded.ticket,absent(),late.f.owner).accepted);
        C(late.backend.consume(first,clipped?motion::Offset{0,.2}:first.requested,clipped?motion::Completion::clipped:motion::Completion::accepted));
        const auto r=late.f.kernel.complete(decoded.ticket,true,late.f.owner);
        C(r.interaction.owns_left_hand&&close(r.interaction.offset.up,clipped?.2:.3));
        motion::Intent next{};C(!late.f.kernel.pending_motion(next));
        late.f.hands.hands[0].grip.position.y-=.05;late.next(.8f);C(late.f.kernel.pending_motion(next));
        C(close(next.requested.up,clipped?.25:.35));
    }
    Deferred changed;changed.grasp();const auto stale=changed.move(.3);
    changed.f.hands.hands[0].grip.position.y-=.05;changed.f.advance(.8f);const auto replacement=changed.f.begin();
    C(!changed.f.kernel.motion_current(stale)&&!changed.backend.consume(stale,stale.requested));
    C(changed.f.kernel.update_with_decoded(replacement.ticket,absent(),changed.f.owner).accepted);
    C(changed.f.kernel.complete(replacement.ticket,true,changed.f.owner).interaction.owns_left_hand);
    motion::Intent next{};C(changed.f.kernel.pending_motion(next)&&next.id!=stale.id&&close(next.requested.up,.35));
    for(bool executed:{false,true}){
        Deferred failed;failed.grasp();const auto first=failed.move(.3);
        failed.f.advance(.8f);const auto decoded=failed.f.begin();
        C(failed.f.kernel.update_with_decoded(decoded.ticket,absent(),failed.f.owner).accepted);
        if(executed)C(failed.backend.consume(first,first.requested));
        const auto r=failed.f.kernel.complete(decoded.ticket,false,failed.f.owner);
        C(!r.accepted&&!r.interaction.owns_left_hand&&close(r.interaction.offset.up,executed?.3:0));
        C(!failed.f.kernel.motion_current(first));
        if(!executed)C(!failed.backend.consume(first,{0,.3}));
    }
}
static void outage_context_and_weapon_scope(){
    Deferred interrupted;interrupted.grasp();auto old=interrupted.move(.3);
    interrupted.backend.observation=motion::Observation::unavailable;auto r=interrupted.next(.8f);
    C(!r.view_valid&&!r.interaction.owns_left_hand&&!interrupted.f.kernel.motion_current(old));
    C(interrupted.f.kernel.motion_status()==motion::Code::recovery_required);
    Deferred epoch;epoch.grasp();old=epoch.move(.3);++epoch.f.frame.cover.epochs.reference;
    r=epoch.next(.8f);C(!r.view_valid&&!r.interaction.owns_left_hand&&!epoch.f.kernel.motion_current(old));
    Deferred exit;exit.grasp();old=exit.move(.3);exit.f.frame.cover.cover_valid=false;
    r=exit.next(0);C(!r.view_valid&&!exit.f.kernel.motion_current(old));
    // A retired domain is deliberately not revived by reusing its old numbers.
    // An authenticated Native basis handoff is still required for a new one.
    exit.f.frame.cover.cover_valid=true;r=exit.next(.8f);C(!r.view_valid&&!r.interaction.owns_left_hand);
    motion::UnavailableBackend unavailable;Fixture weapon(&unavailable);auto contribution=ready();contribution.requests.support=true;
    weapon.run(contribution);r=weapon.next(.8f,contribution);
    C(r.accepted&&r.left_hand.lease.kind==LeftHandInteraction::support&&!r.interaction.owns_left_hand);
    C(!r.view_valid); // unavailable cover motion does not abort a valid weapon grant
}
static void baseline_ranges_head_motion_and_final_outage(){
    Deferred baseline;baseline.backend.body={0,.25};baseline.backend.published.from=baseline.backend.published.requested=
        baseline.backend.published.position=baseline.backend.body;
    auto r=baseline.run();C(close(r.interaction.offset.up,.25)&&r.view_valid&&close(r.view.base_eye_world.z,-15));
    C(baseline.next(.8f).interaction.owns_left_hand);
    baseline.f.hands.hands[0].grip.position.y-=.1;r=baseline.next(.8f);
    motion::Intent i{};C(baseline.f.kernel.pending_motion(i)&&close(i.from.up,.25)&&close(i.requested.up,.35));
    C(baseline.backend.consume(i,i.requested));baseline.next(0);
    baseline.f.frame.cover.range.max_up=.1;r=baseline.next(0);
    C(!r.view_valid&&!r.interaction.offset_certified&&close(r.interaction.offset.up,.35));

    Deferred head;head.grasp();const auto waiting=head.move(.3);
    head_delta(head.f,.1,.1);r=head.next(.8f);motion::Intent unchanged{};
    C(r.view_valid&&close(r.interaction.offset.up,0)&&close(r.requested_up_metres,.1));
    C(head.f.kernel.pending_motion(unchanged)&&unchanged.id==waiting.id);
    C(unchanged.hand.sample.observed_ns==waiting.hand.sample.observed_ns);

    Deferred final;final.grasp();const auto pending=final.move(.3);
    final.f.advance(.8f);const auto decoded=final.f.begin();
    C(final.f.kernel.update_with_decoded(decoded.ticket,absent(),final.f.owner).accepted);
    final.backend.observation=motion::Observation::unavailable;
    r=final.f.kernel.complete(decoded.ticket,true,final.f.owner);
    C(!r.view_valid&&!r.interaction.owns_left_hand&&!r.interaction.offset_certified&&!final.f.kernel.motion_current(pending));
    C(final.f.kernel.motion_status()==motion::Code::recovery_required);
    C(final.f.kernel.update_with_decoded(decoded.ticket,absent(),final.f.owner).left_hand.lease.kind==LeftHandInteraction::none);

    Deferred rejected;rejected.grasp();rejected.backend.admission=motion::Admission::rejected;
    rejected.f.hands.hands[0].grip.position.y-=.3;r=rejected.next(.8f);
    C(!r.interaction.owns_left_hand&&!r.view_valid&&!rejected.f.kernel.pending_motion(i));
    C(rejected.f.kernel.motion_status()==motion::Code::rejected);

    BodyHost outside_backend;Fixture outside(&outside_backend);outside_backend.input=&outside.kernel;
    outside.frame.cover.cover_valid=false;auto support=ready();support.requests.support=true;
    outside.run(support);r=outside.next(.8f,support);
    C(r.left_hand.lease.kind==LeftHandInteraction::support&&outside_backend.observations==0&&outside_backend.admissions==0);
    Deferred mutated;mutated.grasp();const auto old=mutated.move(.3);
    mutated.f.advance(.8f);const auto first=mutated.f.begin();
    ++mutated.f.frame.motion_binding.basis;
    C(!mutated.f.kernel.begin(mutated.f.frame,mutated.f.owner).accepted);
    C(!mutated.f.kernel.motion_current(old)&&!mutated.backend.consume(old,old.requested));
    C(!mutated.f.kernel.complete(first.ticket,true,mutated.f.owner).accepted);
}
int main(){
    if(ExistingInputTransactionMain())return 1;const auto inherited=checks;
    waiting_release_and_actual_commit();waiting_keeps_stable_source_without_renewing_it();rebase_only_completed_clips();
    callback_completion_and_failed_weapon_transaction();outage_context_and_weapon_scope();
    baseline_ranges_head_motion_and_final_outage();
    std::printf("cover_input_motion: %u checks (%u inherited), %u failures; actual input/owner/kernel/ledger, owned Host body only; Game=0\n",checks,inherited,failures);
    return failures?1:0;
}
