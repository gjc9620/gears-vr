#include "frame_link.h"
#include "swapchain_binding_observer.h"
#include "gpu_viewport_stats.h"
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <memory>

using namespace gears_frame_link;
static_assert(Tracker::list_capacity==gears_viewport::Tracker::list_capacity,
    "Frame association and target binding admission must cover the same lists");
static unsigned checks{};
static void check(bool condition, int line) {
    ++checks;
    if (!condition) { std::fprintf(stderr, "FAIL line %d\n", line); std::exit(1); }
}
#define CHECK(value) check((value), __LINE__)
static constexpr Identity buffer = 100, queue = 200;
static bool eye(Tracker& tracker, ListRef ref, uint64_t ticket, unsigned index, Identity resource = buffer,
                uint32_t width = 1920, uint32_t height = 1080) {
    return tracker.draw_final(ref, ticket, index, {resource, width, height}, {index * (width / 2), 0, width / 2, height});
}
static void pair(Tracker& tracker, ListRef ref, uint64_t ticket = 1) {
    CHECK(eye(tracker, ref, ticket, 0)); CHECK(eye(tracker, ref, ticket, 1));
}
static void owned_retirement_cases() {
    const Target target{buffer,1920,1080};
    auto configured=[&](){auto t=std::make_unique<Tracker>();
        CHECK(t->configure_owned_output(target,queue,9,4,true));return t;};
    auto submit=[](Tracker& t,ListRef ref){ExecutionToken token{};
        CHECK(t.prepare_execute(queue,&ref,1,token));CHECK(t.commit_execute(token));};
    // Independent proof and every identity field are mandatory; refusal leaves
    // the original snapshot and generation usable, not a partial retirement.
    for(unsigned bad=0;bad<13;++bad){
        auto t=configured();auto ref=t->reset_success(1);pair(*t,ref,10);submit(*t,ref);
        OwnedRetirement proof{};CHECK(t->owned_retirement_snapshot(proof));auto altered=proof;
        if(bad==0)++altered.world_epoch;
        if(bad==1)++altered.revision;
        if(bad==2)++altered.ticket_cutoff;
        if(bad==3)++altered.output.target.resource;
        if(bad==4)++altered.output.target.width;
        if(bad==5)++altered.output.target.height;
        if(bad==6)++altered.output.queue;
        if(bad==7)++altered.output.generation;
        if(bad==8)++altered.output.state;
        if(bad==9)altered.output.enabled=false;
        if(bad==10)altered.output.certified=false;
        if(bad==11)altered={};
        if(bad<12)CHECK(!t->owned_retirement_current(altered));
        CHECK(!t->retire_owned_output(altered,bad!=12));
        CHECK(t->owned_status().enabled&&t->owned_status().certified);
        CHECK(!t->world_handoff_ready(1,2)&&!t->advance_world_epoch(1,2));
        CHECK(t->retire_owned_output(proof,true));CHECK(!t->retire_owned_output(proof,true));
        CHECK(t->world_handoff_ready(1,2));
    }
    // Even unrelated pending submission blocks; refusal cannot cancel its token.
    {
        auto t=configured();auto ref=t->reset_success(1);OwnedRetirement proof{};
        CHECK(t->owned_retirement_snapshot(proof));ExecutionToken token{};
        CHECK(t->prepare_execute(queue,&ref,1,token));
        CHECK(!t->owned_retirement_current(proof));
        OwnedRetirement untouched=proof;CHECK(!t->owned_retirement_snapshot(untouched));
        CHECK(untouched.revision==proof.revision);CHECK(!t->retire_owned_output(proof,true));
        CHECK(!t->world_handoff_ready(1,2));CHECK(t->commit_execute(token));
        CHECK(t->owned_retirement_current(proof)); // Empty/unrelated commit changed no source evidence.
        CHECK(t->retire_owned_output(proof,true));
    }
    // No erase-to-pass: finals, unknown writes, barriers and overlays all wait
    // for actual submission. Failed metadata/capacity remain terminal.
    for(unsigned mode=0;mode<7;++mode){
        auto t=configured();auto ref=t->reset_success(1);OwnedRetirement proof{};
        CHECK(t->owned_retirement_snapshot(proof));
        if(mode==0)CHECK(eye(*t,ref,1,0));
        if(mode==1)CHECK(t->draw_unknown(ref,buffer));
        if(mode==2)CHECK(t->record_transition(ref,buffer,9,4,128));
        if(mode==3){pair(*t,ref);auto p=t->overlay_pair(ref,target);CHECK(p.linked);
            CHECK(t->draw_overlay_pair(ref,1,target,p.eyes,3));}
        if(mode==4)t->invalidate();
        if(mode==5)t->invalidate_capacity();
        if(mode==6)t->invalidate_list(1);
        OwnedRetirement untouched{};untouched.revision=123;
        CHECK(!t->owned_retirement_snapshot(untouched));CHECK(untouched.revision==123);
        CHECK(!t->retire_owned_output(proof,true));CHECK(t->owned_status().enabled);
        CHECK(!t->advance_world_epoch(1,2));
        if(mode<4){submit(*t,ref);CHECK(t->owned_retirement_snapshot(proof));CHECK(t->retire_owned_output(proof,true));}
        if(mode==5){CHECK(t->snapshot().capacity_bad);CHECK(!t->world_handoff_ready(1,2));}
    }
    // Desktop work need not stop while the owned retirement fence is pending.
    // Preserve the ORIGINAL snapshot; no new snapshot is paired with that fence.
    for(bool recreate:{false,true}){
        auto t=configured();OwnedRetirement proof{};CHECK(t->owned_retirement_snapshot(proof));
        for(unsigned n=0;n<3;++n){auto desktop=recreate?t->create_success(1,true):t->reset_success(1);
            CHECK(t->draw_unknown(desktop,buffer+1));CHECK(t->draw_unknown(desktop,buffer+1));
            CHECK(t->owned_retirement_current(proof));submit(*t,desktop);
            CHECK(t->owned_retirement_current(proof));}
        OwnedRetirement same{};CHECK(t->owned_retirement_snapshot(same));CHECK(same.revision==proof.revision);
        CHECK(t->retire_owned_output(proof,true));CHECK(!t->owned_retirement_current(proof));
    }
    // Source mutation after a retirement fence snapshot never regains that old
    // receipt even after execution, Reset, or a net-zero state transition.
    for(unsigned mode=0;mode<8;++mode){
        auto t=configured();auto source=t->reset_success(1);OwnedRetirement proof{};
        CHECK(t->owned_retirement_snapshot(proof));
        if(mode==0||mode==1||mode==4){CHECK(t->draw_unknown(source,buffer));
            if(mode==1)CHECK(t->draw_unknown(source,buffer));}
        if(mode==2||mode==3){CHECK(t->record_transition(source,buffer,9,4,128));
            CHECK(t->record_transition(source,buffer,9,128,4));}
        if(mode==5)CHECK(eye(*t,source,1,0));
        if(mode==6){pair(*t,source);auto p=t->overlay_pair(source,target);CHECK(p.linked);
            CHECK(t->draw_overlay_pair(source,1,target,p.eyes,3));}
        if(mode==7)CHECK(t->record_owned_barrier_fault(source,9));
        CHECK(!t->owned_retirement_current(proof));CHECK(!t->retire_owned_output(proof,true));
        if(mode==3||mode==4||mode==7)t->reset_success(1);else submit(*t,source);
        CHECK(!t->owned_retirement_current(proof));CHECK(!t->retire_owned_output(proof,true));
        CHECK(t->owned_status().enabled); // Refusal did not release the owner.
    }
    for(bool consume:{false,true}){
        auto t=configured();auto source=t->reset_success(1);pair(*t,source);submit(*t,source);
        OwnedRetirement proof{};CHECK(t->owned_retirement_snapshot(proof));CHECK(t->owned_retirement_current(proof));
        if(consume)CHECK(t->consume_owned_output(buffer,9,queue,1).linked);
        else CHECK(t->seal_owned_output(buffer,9,queue).linked);
        CHECK(!t->owned_retirement_current(proof));CHECK(!t->retire_owned_output(proof,true));
    }
    // Retirement retains real generations and executed markers, seals unrelated
    // unsubmitted recordings, and only a later epoch/new resource generation can
    // be born. Reused resource addresses require that same new lifetime proof.
    for(bool recreate:{false,true}){
        auto t=configured();auto done=t->reset_success(1);pair(*t,done,10);submit(*t,done);
        auto open=t->reset_success(2);CHECK(t->draw_unknown(open,buffer+1));
        auto closed=t->create_success(3,false);const auto before=t->snapshot();
        OwnedRetirement proof{};CHECK(t->owned_retirement_snapshot(proof));
        CHECK(proof.world_epoch==1&&proof.ticket_cutoff==10&&proof.output.generation==9);
        CHECK(t->retire_owned_output(proof,true));
        CHECK(!t->owned_status().enabled&&!t->owned_status().certified);
        CHECK(t->snapshot().active_lists==before.active_lists&&t->snapshot().reset_calls==before.reset_calls);
        CHECK(t->current(1).generation==done.generation&&t->current(2).generation==open.generation);
        CHECK(!t->current(3).identity);
        CHECK(!t->birth_owned_output(target,queue,10,4,true,true)); // Same epoch.
        CHECK(!t->configure_owned_output(target,queue,10,4,true));
        CHECK(!t->execute(queue,&open,1));CHECK(!t->present(buffer,queue).linked);
        CHECK(t->world_handoff_ready(1,2));CHECK(t->advance_world_epoch(1,2));
        CHECK(!t->birth_owned_output(target,queue,9,4,true,true)); // Generation ABA.
        CHECK(t->birth_owned_output(target,queue,10,4,true,true));
        CHECK(!t->retire_owned_output(proof,true)); // Previous lifetime receipt.
        auto fresh=recreate?t->create_success(2,true):t->reset_success(2);
        CHECK(fresh.generation>open.generation&&fresh.generation>closed.generation);
        pair(*t,fresh,11);submit(*t,fresh);CHECK(t->consume_owned_output(buffer,10,queue,11).linked);
        CHECK(t->owned_retirement_snapshot(proof));CHECK(t->retire_owned_output(proof,true));
        CHECK(t->advance_world_epoch(2,3));CHECK(t->birth_owned_output(target,queue,11,128,true,true));
    }
    // No Present is needed to advance, and neither an executed nor unsubmitted
    // old list acquires the new epoch merely from birth clearing associations.
    for(bool executed:{false,true}){
        auto t=configured();auto old=t->reset_success(1);CHECK(t->draw_unknown(old,buffer+1));
        if(executed)submit(*t,old);OwnedRetirement proof{};
        CHECK(t->owned_retirement_snapshot(proof));CHECK(t->retire_owned_output(proof,true));
        CHECK(t->advance_world_epoch(1,2));CHECK(t->birth_owned_output(target,queue,10,4,true,true));
        CHECK(t->current(1).generation==old.generation);ExecutionToken token{123};
        CHECK(!t->prepare_execute(queue,&old,1,token));CHECK(token.value==123);
        CHECK(!t->owned_status().certified); // Late old submission is terminal.
    }
    {
        auto t=configured();OwnedRetirement proof{};CHECK(t->owned_retirement_snapshot(proof));
        CHECK(t->retire_owned_output(proof,true));t->invalidate_capacity();
        CHECK(t->advance_world_epoch(1,2));CHECK(t->snapshot().capacity_bad);
        CHECK(!t->birth_owned_output(target,queue,10,4,true,true));
    }
    {
        auto t=std::make_unique<Tracker>();OwnedRetirement proof{};
        CHECK(!t->owned_retirement_snapshot(proof));CHECK(!t->retire_owned_output(proof,true));
    }
    // Native handoff reclaims only AFTER certified retirement, before releasing
    // canonical COM leases. Distinct lifetime lists must not accumulate across
    // worlds, while the tracker keeps monotonically increasing generations.
    auto retire_and_reclaim=[](Tracker& t,OwnedRetirement const& proof){
        if(!t.retire_owned_output(proof,true))return false;
        t.retire_present_interval();return true;
    };
    {
        auto t=configured();uint64_t last_generation{};
        constexpr size_t lists_per_world=Tracker::list_capacity/2;
        static_assert(lists_per_world*3>Tracker::list_capacity);
        for(uint64_t world=1;world<=3;++world){
            for(size_t i=0;i<lists_per_world;++i){
                const Identity id=1000+(world-1)*lists_per_world+i;
                auto ref=i%3==0?t->create_success(id,false):t->reset_success(id);
                CHECK(ref.identity==id&&ref.generation>last_generation);last_generation=ref.generation;
                if(i==1){pair(*t,ref,world);submit(*t,ref);}
                else if(i%3==2)CHECK(t->draw_unknown(ref,buffer+1)); // Unsubmitted unrelated recording.
            }
            CHECK(t->consume_owned_output(buffer,8+world,queue,world).linked);
            OwnedRetirement proof{};CHECK(t->owned_retirement_snapshot(proof));
            CHECK(retire_and_reclaim(*t,proof));
            CHECK(t->snapshot().active_lists==0&&!t->snapshot().capacity_bad);
            CHECK(!t->current(1000+(world-1)*lists_per_world+1).identity);
            CHECK(t->advance_world_epoch(world,world+1));
            if(world<3)CHECK(t->birth_owned_output(target,queue,9+world,4,true,true));
        }
        CHECK(t->snapshot().list_highwater==lists_per_world&&!t->snapshot().list_overflow);
    }
    // Reclamation cannot manufacture retirement proof for a pending batch or
    // an unfinished source recording. Failed retirement preserves admission.
    for(bool pending:{false,true}){
        auto t=configured();auto ref=t->reset_success(1);OwnedRetirement proof{};
        CHECK(t->owned_retirement_snapshot(proof));ExecutionToken token{};
        if(pending)CHECK(t->prepare_execute(queue,&ref,1,token));
        else CHECK(t->record_transition(ref,buffer,9,4,4));
        CHECK(!retire_and_reclaim(*t,proof));
        CHECK(t->current(1).generation==ref.generation&&t->snapshot().active_lists==1);
        if(pending)CHECK(t->commit_execute(token));else submit(*t,ref);
        CHECK(t->owned_retirement_snapshot(proof));CHECK(retire_and_reclaim(*t,proof));
    }
    // Both executed and unrelated unsubmitted old references stay unknown after
    // reclamation/birth, including when their identity has acquired a new Reset.
    for(bool executed:{false,true})for(bool reuse:{false,true}){
        auto t=configured();auto old=t->reset_success(1);CHECK(t->draw_unknown(old,buffer+1));
        if(executed)submit(*t,old);OwnedRetirement proof{};
        CHECK(t->owned_retirement_snapshot(proof));CHECK(retire_and_reclaim(*t,proof));
        CHECK(t->advance_world_epoch(1,2));CHECK(t->birth_owned_output(target,queue,10,4,true,true));
        if(reuse){auto fresh=t->reset_success(1);CHECK(fresh.generation>old.generation);}
        ExecutionToken token{123};CHECK(!t->prepare_execute(queue,&old,1,token));
        CHECK(token.value==123&&!t->owned_status().certified);
    }
    {
        auto t=configured();OwnedRetirement proof{};CHECK(t->owned_retirement_snapshot(proof));
        CHECK(t->retire_owned_output(proof,true));t->invalidate_capacity();
        t->retire_present_interval();CHECK(t->snapshot().capacity_bad);
        CHECK(t->advance_world_epoch(1,2));CHECK(!t->birth_owned_output(target,queue,10,4,true,true));
    }
}
int main() {
    owned_retirement_cases();
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1);
        CHECK(t.draw_unknown(a, buffer)); pair(t, a);
        CHECK(t.execute(queue, &a, 1)); auto r = t.present(buffer, queue);
        CHECK(r.linked && r.ticket == 1 && !r.contaminated);
        CHECK(r.eyes[0].width == 960 && r.eyes[1].x == 960);
        CHECK(!t.present(buffer, queue).linked);
        CHECK(!t.execute(queue, &a, 1)); CHECK(!t.present(buffer, queue).linked);
        auto b = t.reset_success(1); CHECK(b.generation != a.generation);
        pair(t, b, 2); CHECK(t.execute(queue, &b, 1)); CHECK(t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1), b = t.reset_success(2);
        CHECK(eye(t, a, 1, 0, buffer, 2560, 1440));
        CHECK(eye(t, b, 1, 1, buffer, 2560, 1440));
        ListRef members[]{a, b}; CHECK(t.execute(queue, members, 2));
        auto r = t.present(buffer, queue); CHECK(r.linked && r.eyes[1].x == 1280);
    }
    for (unsigned position : {0u, 1u}) {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1);
        CHECK(eye(t, a, 1, 0));
        if (position == 0) CHECK(t.draw_unknown(a, buffer));
        CHECK(eye(t, a, 1, 1));
        if (position == 1) CHECK(t.draw_unknown(a, buffer));
        CHECK(t.execute(queue, &a, 1)); auto r = t.present(buffer, queue);
        CHECK(!r.linked && r.contaminated);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1), b = t.reset_success(2);
        pair(t, a); CHECK(t.execute(queue, &a, 1));
        CHECK(t.draw_unknown(b, buffer)); CHECK(t.execute(queue, &b, 1));
        CHECK(t.present(buffer, queue).contaminated);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1);
        pair(t, a); CHECK(t.draw_unknown(a, buffer + 1));
        CHECK(t.execute(queue, &a, 1)); CHECK(t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1), b = t.reset_success(2);
        CHECK(eye(t, a, 1, 0)); CHECK(eye(t, b, 2, 1));
        ListRef members[]{a, b}; CHECK(t.execute(queue, members, 2));
        CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1), b = t.reset_success(2);
        CHECK(eye(t, a, 1, 0)); CHECK(eye(t, b, 1, 1));
        CHECK(t.execute(queue, &a, 1)); CHECK(t.execute(queue + 1, &b, 1));
        CHECK(!t.present(buffer, queue).linked);
    }
    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1); pair(t, a);
        switch (scenario) {
        case 0: CHECK(t.execute(queue, &a, 1)); CHECK(!t.execute(queue, &a, 1)); break;
        case 1: { ListRef members[]{a, a}; CHECK(!t.execute(queue, members, 2)); break; }
        case 2: { ListRef unknown{2, 1}; CHECK(!t.execute(queue, &unknown, 1)); break; }
        case 3: CHECK(!t.draw_unknown(a, 0)); CHECK(!t.execute(queue, &a, 1)); break;
        case 4: t.invalidate(); CHECK(!t.execute(queue, &a, 1)); break;
        case 5: t.invalidate_list(1); CHECK(!t.execute(queue, &a, 1)); break;
        case 6: CHECK(!t.execute(0, &a, 1)); break;
        case 7: CHECK(t.execute(queue, &a, 1)); CHECK(!t.present(buffer, 0).linked); break;
        case 8: CHECK(t.execute(queue, &a, 1)); CHECK(!t.present(buffer, queue + 1).linked); break;
        case 9: CHECK(t.execute(queue, &a, 1)); CHECK(!t.present(buffer + 1, queue).linked); break;
        }
        CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1); CHECK(eye(t, a, 1, 0));
        CHECK(t.execute(queue, &a, 1)); CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1); pair(t, a); CHECK(eye(t, a, 1, 1));
        CHECK(t.execute(queue, &a, 1)); CHECK(!t.present(buffer, queue).linked);
    }
    for (unsigned scenario = 0; scenario < 4; ++scenario) {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1);
        const Rect invalid[]{ {0, 0, 961, 1080}, {0, 1, 960, 1080}, {0, 0, 960, 1079}, {960, 0, 960, 1080} };
        CHECK(!t.draw_final(a, 1, 0, {buffer, 1920, 1080}, invalid[scenario]));
        CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1);
        CHECK(!eye(t, a, 1, 0, buffer, 1919));
        CHECK(!eye(t, a, 1, 0, 0));
        CHECK(!eye(t, a, 0, 0));
        CHECK(!t.draw_final(a, 1, 2, {buffer, 1920, 1080}, {}));
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto old = t.reset_success(1); CHECK(eye(t, old, 1, 0));
        auto fresh = t.reset_success(1); CHECK(eye(t, fresh, 2, 1));
        CHECK(!t.execute(queue, &old, 1)); CHECK(!t.execute(queue, &fresh, 1));
        CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto old = t.reset_success(1); pair(t, old);
        auto fresh = t.reset_success(1); pair(t, fresh, 2);
        CHECK(t.execute(queue, &fresh, 1)); CHECK(t.present(buffer, queue).linked);
        CHECK(!eye(t, old, 3, 0)); CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto old = t.reset_success(1); CHECK(eye(t, old, 1, 0));
        CHECK(t.execute(queue, &old, 1)); auto fresh = t.reset_success(1);
        CHECK(eye(t, fresh, 2, 1)); CHECK(t.execute(queue, &fresh, 1));
        CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1); pair(t, a);
        // Even a failed actual Present must call this consume boundary.
        CHECK(!t.present(buffer, queue).linked);
        CHECK(!t.execute(queue, &a, 1)); CHECK(!t.present(buffer, queue).linked);
        auto b = t.reset_success(1); CHECK(!eye(t, b, 1, 0));
        CHECK(!t.present(buffer, queue).linked);
        auto c = t.reset_success(1); pair(t, c, 2); CHECK(t.execute(queue, &c, 1));
        CHECK(t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1);
        for (size_t i = 0; i < Tracker::events_per_list; ++i) CHECK(t.draw_unknown(a, buffer+(i%2)));
        CHECK(!t.draw_unknown(a, buffer)); CHECK(!t.execute(queue, &a, 1));
        CHECK(!t.present(buffer, queue).linked);
        auto b = t.reset_success(1); pair(t, b, 2); CHECK(!t.execute(queue, &b, 1));
        CHECK(!t.present(buffer, queue).linked);
        t.invalidate_all(); CHECK(!t.present(buffer, queue).linked);
        auto c = t.reset_success(1); pair(t, c, 3); CHECK(t.execute(queue, &c, 1));
        CHECK(t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage;
        for (size_t i = 1; i <= Tracker::list_capacity; ++i) CHECK(t.reset_success(i).generation != 0);
        CHECK(t.reset_success(Tracker::list_capacity + 1).generation == 0);
        CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage=std::make_unique<Tracker>();auto& t=*storage;
        CHECK(t.configure_owned_output({buffer,1920,1080},queue,9,4,true));
        const auto pending=t.reset_success(1);
        CHECK(t.draw_unknown(pending,buffer+1)); // Unsubmitted recording survives every consume.
        uint64_t last_generation{};
        for(uint64_t frame=1;frame<=100;++frame){
            std::array<ListRef,16> submitted{};
            for(size_t i=0;i<submitted.size();++i){
                submitted[i]=t.create_success(2+(frame-1)*submitted.size()+i,true);
                CHECK(submitted[i].generation>last_generation);
                last_generation=submitted[i].generation;
            }
            pair(t,submitted[0],frame);
            ExecutionToken token{};CHECK(t.prepare_execute(queue,submitted.data(),submitted.size(),token));
            CHECK(t.commit_execute(token));
            std::array<Identity,Tracker::list_capacity> retired{};size_t count{};
            const auto consumed=t.consume_owned_output(buffer,9,queue,frame,retired.data(),retired.size(),&count);
            CHECK(consumed.linked&&consumed.state_certified&&count==submitted.size());
            for(size_t i=0;i<count;++i)CHECK(retired[i]==submitted[i].identity&&!t.current(retired[i]).identity);
            CHECK(t.current(pending.identity).generation==pending.generation);
            CHECK(t.snapshot().active_lists==1&&!t.snapshot().list_overflow);
        }
        CHECK(t.snapshot().list_highwater==17&&!t.snapshot().capacity_bad);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage;
        for (size_t i = 0; i <= Tracker::target_capacity; ++i) {
            auto a = t.reset_success(i + 1);
            CHECK(eye(t, a, i + 1, 0, buffer + i));
            CHECK(eye(t, a, i + 1, 1, buffer + i));
            const bool accepted = t.execute(queue, &a, 1);
            CHECK(accepted == (i < Tracker::target_capacity));
        }
        CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage;
        auto a = t.reset_success(1), b = t.reset_success(2);
        pair(t, a); CHECK(eye(t, b, 1, 0, buffer + 1));
        ListRef members[]{a, b}; CHECK(!t.execute(queue, members, 2));
        CHECK(!t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage;
        auto a = t.reset_success(1);
        CHECK(eye(t, a, 1, 0, buffer, 2, 1)); CHECK(eye(t, a, 1, 1, buffer, 2, 1));
        CHECK(t.execute(queue, &a, 1)); CHECK(t.present(buffer, queue).linked);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a = t.reset_success(1), b = t.reset_success(2);
        // Actual simultaneous recording; checks run only after joining.
        bool left{}, right{};
        std::thread first([&] { left = eye(t, a, 1, 0); });
        std::thread second([&] { right = eye(t, b, 1, 1); });
        first.join(); second.join(); CHECK(left && right);
        ListRef members[]{a, b}; CHECK(t.execute(queue, members, 2)); CHECK(t.present(buffer, queue).linked);
    }
    {
        gears_swapchain_binding::Observer observer;
        CHECK(!observer.bind({1,2,3,16,16,2},true,true));
        CHECK(!observer.bind({1,2,3,1920,1080,2},false,true));
        CHECK(!observer.bind({1,2,3,1920,1080,2},true,false));
        CHECK(observer.bind({1,2,3,1920,1080,2},true,true));
        CHECK(observer.buffer(1,100,0)); CHECK(observer.buffer(1,101,1));
        const uintptr_t resources[]{100,101};
        CHECK(observer.unchanged(1,1920,1080,resources,2));
        CHECK(observer.known_buffer(100) && observer.lookup(1).queue==2);
        const uintptr_t resized[]{100,102};
        CHECK(!observer.unchanged(1,1920,1080,resized,2));
        CHECK(!observer.known_buffer(100) && observer.lookup(1).queue==0);
        CHECK(observer.snapshot().changed==1);
    }
    {
        gears_swapchain_binding::Observer observer;
        CHECK(observer.bind({1,2,3,1920,1080,2},true,true));
        CHECK(!observer.bind({1,4,3,1920,1080,2},true,true));
        CHECK(observer.snapshot().conflicts==1);
    }
    {
        gears_swapchain_binding::Observer observer;
        CHECK(observer.bind({1,2,3,1920,1080,2},true,true));
        CHECK(observer.bind({4,2,3,1920,1080,2},true,true));
        CHECK(observer.buffer(1,100,0)); CHECK(!observer.buffer(4,100,0));
        CHECK(observer.snapshot().conflicts==1);
    }
    {
        gears_swapchain_binding::Observer observer;
        CHECK(observer.bind({1,2,3,1920,1080,2},true,true)); CHECK(observer.buffer(1,100,0));
        const uintptr_t resources[]{100}; CHECK(!observer.unchanged(1,1280,720,resources,1));
    }
    {
        gears_swapchain_binding::Observer observer;
        for(uintptr_t i=1;i<=8;++i)CHECK(observer.bind({i,20,30,1920,1080,2},true,true));
        CHECK(!observer.bind({9,20,30,1920,1080,2},true,true)); CHECK(observer.snapshot().overflow==1);
    }
    {
        auto storage = std::make_unique<Tracker>(); auto& t = *storage; auto a=t.reset_success(1);
        CHECK(t.current(1).generation==a.generation); pair(t,a); CHECK(t.execute(queue,&a,1));
        CHECK(!t.execute(queue,&a,1)); CHECK(t.present(buffer,queue).rejection&duplicate_execute);
    }
    {
        gears_swapchain_binding::Observer observer;
        CHECK(observer.bind({1,2,3,1280,720,2},true,true));
        CHECK(observer.buffer(1,100,0));CHECK(observer.buffer(1,101,1));
        const uintptr_t trial[]{200,201};
        CHECK(observer.anchor(1,1920,1080,trial,2));
        CHECK(observer.snapshot().anchors==1&&!observer.snapshot().rejected);
        CHECK(observer.known_buffer(200)&&!observer.known_buffer(100));
        CHECK(observer.first_comparison().expected_width==1280&&observer.first_comparison().actual_width==1920);
        CHECK(observer.first_comparison().expected[1]==101&&observer.first_comparison().actual[1]==201);
        CHECK(observer.unchanged(1,1920,1080,trial,2));
        const uintptr_t resized[]{300,301};
        CHECK(!observer.unchanged(1,1920,1080,resized,2));
        CHECK(!observer.anchor(1,1920,1080,resized,2));
        CHECK(observer.first_failure().reason&gears_swapchain_binding::buffer_identity);
        CHECK(observer.first_failure().expected[0]==200&&observer.first_failure().actual[0]==300);
        CHECK(observer.registered(1).queue==2&&observer.lookup(1).queue==0);
    }
    {
        gears_swapchain_binding::Observer observer;
        CHECK(observer.bind({1,2,3,1920,1080,2},true,true));
        CHECK(observer.bind({4,5,6,1280,720,2},true,true));
        CHECK(observer.buffer(1,100,0)&&observer.buffer(1,101,1)&&observer.buffer(4,400,0));
        const uintptr_t first[]{100,101},second[]{200,201};
        CHECK(observer.registered(1).generation==1);
        CHECK(observer.begin_resize(1));
        CHECK(observer.registered(1).generation==2&&observer.registered(1).resizing);
        CHECK(!observer.known_buffer(100)&&!observer.lookup(1).chain);
        CHECK(observer.known_buffer(400)&&observer.lookup(4).queue==5);
        CHECK(!observer.unchanged(1,1920,1080,first,2));
        CHECK(!observer.snapshot().rejected); // Pending comparison is not a hard fault.
        CHECK(observer.complete_resize(1,3840,2160,second,2,true));
        CHECK(observer.lookup(1).generation==2&&observer.lookup(1).width==3840);
        CHECK(observer.known_buffer(200)&&observer.known_buffer(201)&&!observer.known_buffer(100)&&!observer.known_buffer(101));
        CHECK(observer.known_buffer(400)&&observer.unchanged(1,3840,2160,second,2));
        CHECK(observer.begin_resize(1));
        CHECK(!observer.complete_resize(1,0,0,nullptr,0,false));
        CHECK(observer.registered(1).needs_revalidation&&!observer.lookup(1).chain&&!observer.known_buffer(200));
        CHECK(!observer.unchanged(1,1920,1080,first,2)); // Failed resize never admits replacement resources.
        CHECK(observer.unchanged(1,3840,2160,second,2));
        CHECK(observer.known_buffer(200)&&observer.lookup(1).generation==3&&!observer.snapshot().rejected);
        CHECK(observer.begin_resize(1)&&observer.complete_resize(1,3840,2160,second,2,false));
        CHECK(observer.lookup(1).generation==4&&observer.known_buffer(200));
        CHECK(observer.begin_resize(1)&&!observer.complete_resize(1,0,0,nullptr,0,false));
        CHECK(observer.begin_resize(1)&&observer.complete_resize(1,1920,1080,first,2,true)); // A failed API may itself be retried.
        CHECK(observer.lookup(1).generation==6&&observer.known_buffer(100)&&!observer.known_buffer(200));
    }
    {
        gears_swapchain_binding::Observer observer;
        CHECK(observer.bind({1,2,3,1920,1080,2},true,true));
        CHECK(observer.buffer(1,100,0)&&observer.buffer(1,101,1));
        CHECK(!observer.inventory(1).valid); // Unanchored startup state is insufficient.
        const uintptr_t first[]{100,101},second[]{200,201};
        CHECK(observer.anchor(1,1920,1080,first,2));
        const auto old=observer.inventory(1);
        CHECK(old.valid&&old.binding.chain==1&&old.binding.queue==2&&old.binding.generation==1&&old.count==2);
        CHECK(old.resources[0]==100&&old.resources[1]==101&&!old.resources[2]);
        CHECK(observer.begin_resize(1)&&!observer.inventory(1).valid);
        CHECK(observer.complete_resize(1,2560,1440,second,2,true));
        const auto current=observer.inventory(1);
        CHECK(current.valid&&current.binding.generation==2&&current.binding.width==2560&&current.binding.height==1440);
        CHECK(current.count==2&&current.resources[0]==200&&current.resources[1]==201&&current.resources!=old.resources);
        CHECK(observer.begin_resize(1)&&!observer.complete_resize(1,0,0,nullptr,0,false));
        CHECK(!observer.inventory(1).valid); // Failed resize requires revalidation.
        CHECK(observer.unchanged(1,2560,1440,second,2)&&observer.inventory(1).valid);
        observer.invalidate();CHECK(!observer.inventory(1).valid&&!observer.inventory(99).valid);
    }
    {
        // Separate chains can resize concurrently and complete in either order.
        gears_swapchain_binding::Observer observer;
        CHECK(observer.bind({1,2,3,1920,1080,2},true,true)&&observer.bind({4,5,6,1920,1080,2},true,true));
        CHECK(observer.buffer(1,100,0)&&observer.buffer(4,400,0));
        CHECK(observer.begin_resize(1)&&observer.begin_resize(4));
        const uintptr_t a[]{200},b[]{500};
        CHECK(observer.complete_resize(4,1280,720,b,1,true));
        CHECK(observer.known_buffer(500)&&!observer.known_buffer(100));
        CHECK(observer.complete_resize(1,1280,720,a,1,true));
        CHECK(observer.lookup(1).generation==2&&observer.lookup(4).generation==2);
        CHECK(!observer.begin_resize(999)&&observer.known_buffer(200));
    }
    {
        for(unsigned fault=0;fault<6;++fault){
            gears_swapchain_binding::Observer observer;
            CHECK(observer.bind({1,2,3,1920,1080,2},true,true));CHECK(observer.buffer(1,100,0));
            const uintptr_t valid[]{200},duplicate[]{200,200};
            if(fault==0){CHECK(observer.begin_resize(1));CHECK(!observer.begin_resize(1));}
            if(fault==1)CHECK(!observer.complete_resize(1,1920,1080,valid,1,true));
            if(fault==2){CHECK(observer.begin_resize(1));CHECK(!observer.complete_resize(1,1920,1080,duplicate,2,true));}
            if(fault==3){CHECK(observer.begin_resize(1));CHECK(!observer.complete_resize(1,1920,1080,valid,17,true));}
            if(fault==4){CHECK(observer.bind({4,5,6,1920,1080,2},true,true));CHECK(observer.buffer(4,200,0));
                CHECK(observer.begin_resize(1));CHECK(!observer.complete_resize(1,1920,1080,valid,1,true));}
            if(fault==5){CHECK(observer.begin_resize(1)&&observer.complete_resize(1,1920,1080,valid,1,true));
                CHECK(!observer.complete_resize(1,1920,1080,valid,1,true));}
            CHECK(observer.snapshot().rejected&&!observer.lookup(1).chain&&!observer.known_buffer(100));
            CHECK(!observer.begin_resize(1)); // Protocol, alias and capacity faults remain sticky.
        }
    }
    {
        gears_swapchain_binding::Observer observer;
        for(uintptr_t chain=1;chain<=3;++chain)CHECK(observer.bind({chain,20,30,1920,1080,2},true,true));
        std::array<uintptr_t,16> inventory{};
        for(uintptr_t chain=1;chain<=2;++chain){
            for(unsigned i=0;i<inventory.size();++i)inventory[i]=chain*100+i;
            CHECK(observer.begin_resize(chain)&&observer.complete_resize(chain,1920,1080,inventory.data(),16,true));
        }
        CHECK(observer.snapshot().buffers==32&&observer.begin_resize(3));
        const uintptr_t extra[]{999};CHECK(!observer.complete_resize(3,1920,1080,extra,1,true));
        CHECK(observer.snapshot().overflow==1&&!observer.lookup(1).chain&&!observer.known_buffer(100));
    }
    {
        gears_swapchain_binding::Observer observer;
        CHECK(observer.bind({1,2,3,1920,1080,2},true,true));CHECK(observer.buffer(1,100,0));
        const uintptr_t invalid[]{0},valid[]{200};
        CHECK(observer.begin_resize(1)&&!observer.complete_resize(1,1920,1080,invalid,1,true));
        CHECK(!observer.lookup(1).chain&&!observer.known_buffer(100));
        CHECK(!observer.unchanged(1,1920,1080,valid,1));
        CHECK(observer.begin_resize(1)&&observer.complete_resize(1,1920,1080,valid,1,true));
        CHECK(observer.known_buffer(200)&&!observer.snapshot().rejected);
        observer.invalidate();CHECK(!observer.begin_resize(1)&&!observer.known_buffer(200));
    }
    {
        auto storage=std::make_unique<Tracker>();auto& t=*storage;
        uint64_t last_generation{};
        // More lifetime identities than capacity, but each consumed window is
        // retired explicitly. This is reclamation at Present, never eviction.
        for(uintptr_t frame=0;frame<12;++frame){
            std::array<ListRef,Tracker::list_capacity> lists{};
            for(size_t i=0;i<lists.size();++i){lists[i]=t.reset_success(1+frame*lists.size()+i);
                CHECK(lists[i].generation>last_generation);last_generation=lists[i].generation;}
            pair(t,lists[0],frame+1);CHECK(t.execute(queue,lists.data(),lists.size()));
            CHECK(t.present(buffer,queue).linked);CHECK(t.snapshot().active_lists==0);
            CHECK(!t.current(lists[0].identity).generation);
        }
        CHECK(t.snapshot().list_highwater==Tracker::list_capacity&&!t.snapshot().list_overflow);
    }
    {
        auto storage=std::make_unique<Tracker>();auto& t=*storage;auto old=t.reset_success(1);pair(t,old);
        t.invalidate_all();CHECK(!t.current(1).generation);CHECK(!t.present(buffer,queue).linked);
        CHECK(!t.execute(queue,&old,1));CHECK(!t.present(buffer,queue).linked);
        auto next=t.reset_success(1);CHECK(next.generation>old.generation);pair(t,next,2);
        CHECK(t.execute(queue,&next,1));CHECK(t.present(buffer,queue).linked);
    }
    {
        auto storage=std::make_unique<gears_viewport::Tracker>();auto& t=*storage;
        t.create(10,{100,1920,1080,24,0});t.reset(1);t.bind(1,1,10);
        CHECK(t.binding(1).target.resource==100);t.clear_bindings();
        CHECK(!t.binding(1).targets&&t.lookup(10).resource==100);
        t.reset(2);t.bind(2,1,10);CHECK(t.binding(2).target.resource==100);
    }
    {
        auto targets=std::make_unique<gears_viewport::Tracker>();auto links=std::make_unique<Tracker>();
        // Legacy/non-strict recording must cover the entire shared bound,
        // including the observed cinematic296-list window and lists257..512.
        // The last list retains both RS state and its OM target.
        std::array<ListRef,gears_viewport::Tracker::list_capacity> lists{};
        const gears_viewport::Rect rectangle{0,0,1920,1080};targets->create(10,{buffer,1920,1080,24,0});
        for(size_t i=0;i<lists.size();++i){const uintptr_t id=i+1;
            // Also cover legacy OM admission without a preceding observed Reset.
            if(i%2==0)CHECK(targets->reset(id));
            CHECK(targets->bind(id,1,10));lists[i]=links->reset_success(id);
            CHECK(targets->set(id,gears_viewport::full,false,rectangle));
            CHECK(targets->set(id,gears_viewport::full,true,rectangle));
            const auto state=targets->draw_state(id);
            CHECK(state.tracked&&state.viewport==rectangle&&state.scissor==rectangle);
            CHECK(lists[i].generation&&targets->binding(id).target.resource==buffer);}
        auto target_stats=targets->snapshot();
        CHECK(target_stats.active_lists==lists.size()&&target_stats.list_highwater==lists.size()&&!target_stats.overflow&&!target_stats.first_failed_list);
        pair(*links,lists.back());CHECK(links->execute(queue,lists.data(),lists.size()));
        CHECK(links->present(buffer,queue).linked&&!links->snapshot().active_lists);
        CHECK(!targets->clear_bindings()&&!targets->snapshot().active_lists);
        CHECK(!targets->binding(lists.size()).targets&&!targets->draw_state(lists.size()).tracked&&targets->lookup(10).resource==buffer);
        CHECK(targets->snapshot().list_highwater==lists.size());
    }
    {
        auto targets=std::make_unique<gears_viewport::Tracker>();auto links=std::make_unique<Tracker>();
        constexpr uintptr_t rejected=gears_viewport::Tracker::list_capacity+1;
        for(uintptr_t i=1;i<=gears_viewport::Tracker::list_capacity;++i)CHECK(targets->reset(i));
        CHECK(!targets->reset(rejected));CHECK(!targets->bind(rejected,1,10));
        CHECK(targets->snapshot().list_highwater==gears_viewport::Tracker::list_capacity);
        CHECK(targets->snapshot().first_failed_list==rejected);
        auto list=links->reset_success(1);pair(*links,list);
        // Production checks the boolean capacity failure even though a lookup
        // would otherwise resemble explicitly unbound render targets.
        if(!targets->bind(rejected,1,10))links->invalidate();
        CHECK(!targets->binding(rejected).targets);CHECK(!links->execute(queue,&list,1));
        CHECK(!links->present(buffer,queue).linked);
        // Production's interval boundary treats target-state overflow as lost
        // association proof. Reclaimed slots do not clear that FrameLink fault.
        const bool overflowed=targets->clear_bindings();CHECK(overflowed);
        if(overflowed)links->invalidate_capacity();
        CHECK(!targets->clear_bindings()&&!targets->snapshot().active_lists);
        CHECK(targets->reset(rejected));CHECK(targets->snapshot().active_lists==1);
        CHECK(targets->retire(rejected)&&!targets->retire(rejected));
        CHECK(!targets->snapshot().active_lists&&targets->snapshot().first_failed_list==rejected);
        auto next=links->reset_success(2);pair(*links,next,2);
        CHECK(!links->execute(queue,&next,1)&&!links->present(buffer,queue).linked&&links->snapshot().capacity_bad);
    }
    {
        auto targets=std::make_unique<gears_viewport::Tracker>();auto links=std::make_unique<Tracker>();
        targets->create(32,{buffer,1920,1080,24,0});targets->bind(1,1,32);
        targets->create(32,{buffer+1,1920,1080,24,0});
        auto list=links->reset_success(1);auto actual=targets->binding(1).target.resource;
        CHECK(actual==buffer);CHECK(eye(*links,list,1,0,actual));CHECK(eye(*links,list,1,1,actual));
        CHECK(links->execute(queue,&list,1));CHECK(!links->present(buffer+1,queue).linked);
        // Even a complete candidate with restored mapping must fail closed
        // after bounded descriptor provenance is lost, across Present/anchor.
        for(uintptr_t i=1;i<=5000;++i)targets->create(i*32,{buffer,1920,1080,24,0});
        CHECK(targets->descriptor_capacity_lost());
        for(uint64_t ticket=2;ticket<=3;++ticket){
            targets->clear_bindings();targets->create(32,{buffer,1920,1080,24,0});
            list=links->reset_success(1);pair(*links,list,ticket);CHECK(links->execute(queue,&list,1));
            if(targets->descriptor_capacity_lost())links->invalidate();
            CHECK(!links->present(buffer,queue).linked);
            links->invalidate_all();CHECK(!links->present(buffer,queue).linked);
        }
    }
    for(auto counts: {std::array<unsigned,2>{0,1},{3,0},{0,0}}){
        auto links=std::make_unique<Tracker>();auto list=links->reset_success(1);
        // Use the exact guard called by both production Draw hooks. Two
        // otherwise valid final scopes with no draw work cannot form a pair.
        if(gears_viewport::has_draw_work(counts[0],counts[1]))pair(*links,list);
        CHECK(links->execute(queue,&list,1));CHECK(!links->present(buffer,queue).linked);
    }
    {
        auto links=std::make_unique<Tracker>();auto game=links->reset_success(1);pair(*links,game,1);
        CHECK(links->execute(queue,&game,1));CHECK(links->present(buffer,queue).linked);
        // A separately ordered original-Present interval may contain unknown
        // Execute and new recordings, but neither survives its closing boundary.
        ListRef unknown{99,99};CHECK(!links->execute(queue,&unknown,1));
        auto internal=links->reset_success(2);pair(*links,internal,2);CHECK(!links->execute(queue,&internal,1));
        links->retire_present_interval();CHECK(!links->current(2).generation);CHECK(!links->snapshot().active_lists);
        auto next=links->reset_success(1);CHECK(!eye(*links,next,2,0));
        CHECK(!links->present(buffer,queue).linked);
        next=links->reset_success(1);pair(*links,next,3);CHECK(links->execute(queue,&next,1));CHECK(links->present(buffer,queue).linked);
        links->invalidate_capacity();links->retire_present_interval();CHECK(links->snapshot().capacity_bad);
        next=links->reset_success(1);pair(*links,next,4);CHECK(!links->execute(queue,&next,1));
        CHECK(!links->present(buffer,queue).linked);CHECK(links->snapshot().capacity_bad);
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);Target target{buffer,1920,1080};
        CHECK(!t->recorded_pair(ref,target).linked);CHECK(eye(*t,ref,1,0));CHECK(!t->recorded_pair(ref,target).linked);
        CHECK(eye(*t,ref,1,1));auto p=t->recorded_pair(ref,target);CHECK(p.linked&&p.ticket==1);
        for(unsigned i=0;i<1000;++i)CHECK(t->draw_overlay_pair(ref,p.ticket,target,p.eyes,3));
        CHECK(t->snapshot().event_highwater==3);CHECK(t->execute(queue,&ref,1));CHECK(t->present(buffer,queue).linked);
        CHECK(t->snapshot().overlay_recorded==1000&&t->snapshot().overlay_executed==1000);
        CHECK(!t->execute(queue,&ref,1));CHECK(t->snapshot().overlay_executed==1000);
        CHECK(!t->recorded_pair(ref,target).linked);
    }
    for(unsigned failure=0;failure<8;++failure){
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);Target target{buffer,1920,1080};pair(*t,ref);
        auto p=t->recorded_pair(ref,target);CHECK(p.linked);
        CHECK(t->draw_overlay_pair(ref,1,target,p.eyes,3));
        auto badref=ref;auto badtarget=target;auto rects=p.eyes;uint64_t ticket=1;unsigned mask=3;
        if(failure==0)ticket=2;
        if(failure==1)badtarget.resource=buffer+1;
        if(failure==2)badtarget.width=3840;
        if(failure==3)++badref.generation;
        if(failure==4)mask=1;
        if(failure==5)rects[1].x=0;
        if(failure==6){CHECK(t->draw_unknown(ref,buffer));CHECK(!t->recorded_pair(ref,target).linked);}
        if(failure==7){ref=t->reset_success(1);badref=ref;CHECK(!t->recorded_pair(ref,target).linked);}
        CHECK(!t->draw_overlay_pair(badref,ticket,badtarget,rects,mask));
        t->execute(queue,&ref,1);CHECK(!t->present(buffer,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);Target target{buffer,1920,1080};pair(*t,ref);
        auto p=t->recorded_pair(ref,target);CHECK(t->draw_overlay_pair(ref,1,target,p.eyes,3));
        CHECK(t->draw_unknown(ref,buffer));CHECK(t->execute(queue,&ref,1));CHECK(!t->present(buffer,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);Target target{buffer,1920,1080};pair(*t,ref);
        auto p=t->recorded_pair(ref,target);CHECK(t->draw_overlay_pair(ref,1,target,p.eyes,3));
        CHECK(t->execute(queue+1,&ref,1));CHECK(!t->present(buffer,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);Target target{buffer,1920,1080};
        for(unsigned i=0;i<Tracker::events_per_list-2;++i)CHECK(t->draw_unknown(ref,buffer+1+(i%2)));
        pair(*t,ref);
        auto p=t->recorded_pair(ref,target);
        CHECK(!t->recorded_pair(ref,target).linked);CHECK(!t->draw_overlay_pair(ref,1,target,p.eyes,3));
        CHECK(t->snapshot().capacity_bad&&t->snapshot().event_overflow==1);
        CHECK(!t->execute(queue,&ref,1));CHECK(!t->present(buffer,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);Target target{buffer,1920,1080};
        for(unsigned i=0;i<Tracker::events_per_list-3;++i)CHECK(t->draw_unknown(ref,buffer+1+(i%2)));
        pair(*t,ref);auto p=t->recorded_pair(ref,target);CHECK(p.linked);
        CHECK(t->draw_overlay_pair(ref,1,target,p.eyes,3));
        for(unsigned i=0;i<1000;++i){CHECK(t->recorded_pair(ref,target).linked);CHECK(t->draw_overlay_pair(ref,1,target,p.eyes,3));}
        CHECK(t->snapshot().event_highwater==32&&!t->snapshot().capacity_bad&&t->snapshot().event_overflow==0);
        CHECK(t->execute(queue,&ref,1));CHECK(t->present(buffer,queue).linked);
        CHECK(t->snapshot().overlay_recorded==1001&&t->snapshot().overlay_executed==1001);
        CHECK(!t->draw_overlay_pair(ref,1,target,p.eyes,3));
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);Target target{buffer,1920,1080};pair(*t,ref);
        auto p=t->recorded_pair(ref,target);CHECK(t->draw_overlay_pair(ref,1,target,p.eyes,3));
        CHECK(t->draw_unknown(ref,buffer+1));CHECK(t->draw_overlay_pair(ref,1,target,p.eyes,3));
        CHECK(t->snapshot().event_highwater==5); // Even harmless other-target writes separate runs.
        CHECK(t->execute(queue,&ref,1));CHECK(t->present(buffer,queue).linked);
        CHECK(t->snapshot().overlay_recorded==2&&t->snapshot().overlay_executed==2);
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);Target a{buffer,1920,1080},b{buffer+1,1920,1080};
        pair(*t,ref);auto p=t->recorded_pair(ref,a);CHECK(t->draw_overlay_pair(ref,1,a,p.eyes,3));
        CHECK(eye(*t,ref,2,0,buffer+1));CHECK(eye(*t,ref,2,1,buffer+1));
        auto other=t->recorded_pair(ref,b);CHECK(other.linked);CHECK(t->draw_overlay_pair(ref,2,b,other.eyes,3));
        CHECK(t->draw_overlay_pair(ref,1,a,p.eyes,3));CHECK(t->snapshot().event_highwater==7);
        CHECK(t->execute(queue,&ref,1));CHECK(t->present(buffer,queue).linked);
        auto next=t->reset_success(1);pair(*t,next,3);auto fresh=t->recorded_pair(next,a);
        CHECK(t->draw_overlay_pair(next,3,a,fresh.eyes,3));CHECK(t->execute(queue,&next,1));
        CHECK(t->present(buffer,queue).linked);
        CHECK(t->snapshot().overlay_recorded==4&&t->snapshot().overlay_executed==4);
    }
    // Explicit executed-world continuation on a different recording list.
    {
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto world=t->reset_success(1),ui=t->reset_success(2);pair(*t,world);
        CHECK(!t->recorded_pair(ui,target).linked);CHECK(!t->overlay_pair(ui,target).linked);
        auto snap=t->snapshot();CHECK(snap.overlay_no_executed_snapshot_count==1);
        CHECK(snap.overlay_no_executed_snapshots[0].other_complete_recorded_lists==1);
        CHECK(snap.overlay_no_executed_snapshots[0].unique_recorded_ref.identity==world.identity);
        CHECK(snap.overlay_no_executed_snapshots[0].unique_recorded_ticket==1);
        CHECK(t->execute(queue,&world,1));auto p=t->overlay_pair(ui,target);
        CHECK(p.linked&&p.from_executed&&p.source_queue==queue&&p.ticket==1);
        for(unsigned i=0;i<1000;++i)CHECK(t->draw_overlay_pair(ui,p.ticket,target,p.eyes,3));
        CHECK(!t->recorded_pair(ui,target).linked); // Original API remains list-local.
        CHECK(t->execute(queue,&ui,1));snap=t->snapshot();
        CHECK(snap.overlay_from_executed_recorded==1000&&snap.overlay_from_executed_executed==1000);
        CHECK(snap.overlay_executed_queries==1&&snap.event_highwater==2);
        CHECK(t->present(buffer,queue).linked);CHECK(!t->overlay_pair(ui,target).linked);
    }
    // Never authorize a merely recorded pair or execute UI before world.
    {
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto world=t->reset_success(1),ui=t->reset_success(2);pair(*t,world);
        for(unsigned n=0;n<16;++n)CHECK(!t->overlay_pair(ui,target).linked);
        CHECK(t->snapshot().overlay_no_executed_queries==16);
        CHECK(t->snapshot().overlay_no_executed_snapshot_count==8);
        std::array<Rect,2> rects{{{0,0,960,1080},{960,0,960,1080}}};
        CHECK(!t->draw_overlay_pair(ui,1,target,rects,3));
        CHECK(!t->execute(queue,&world,1));CHECK(!t->present(buffer,queue).linked);
    }
    // Execute order, queue and intervening pollution are rechecked after recording.
    for(unsigned failure=0;failure<3;++failure){
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto world=t->reset_success(1),ui=t->reset_success(2);pair(*t,world);CHECK(t->execute(queue,&world,1));
        auto p=t->overlay_pair(ui,target);CHECK(p.linked&&t->draw_overlay_pair(ui,1,target,p.eyes,3));
        if(failure==0){CHECK(!t->execute(queue+1,&ui,1));}
        if(failure==1){auto dirty=t->reset_success(3);CHECK(t->draw_unknown(dirty,buffer));CHECK(t->execute(queue,&dirty,1));CHECK(!t->execute(queue,&ui,1));}
        if(failure==2){auto newworld=t->reset_success(3);pair(*t,newworld,2);CHECK(t->execute(queue,&newworld,1));CHECK(!t->execute(queue,&ui,1));}
        CHECK(!t->present(buffer,queue).linked);CHECK(t->snapshot().overlay_from_executed_executed==0);
    }
    for(unsigned failure=0;failure<7;++failure){
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto world=t->reset_success(1),ui=t->reset_success(2);pair(*t,world);CHECK(t->execute(queue,&world,1));
        if(failure==0)CHECK(t->draw_unknown(ui,buffer));
        if(failure==1)CHECK(t->draw_unknown(ui,buffer+1));
        if(failure==2)CHECK(eye(*t,ui,1,0)); // Mixed speculative world write.
        if(failure==3){auto dirty=t->reset_success(3);CHECK(t->draw_unknown(dirty,buffer));CHECK(t->execute(queue,&dirty,1));}
        if(failure==4){auto newer=t->reset_success(3);CHECK(eye(*t,newer,2,0,buffer+1));}
        if(failure==5)target.width=2560;
        if(failure==6)t->invalidate_capacity();
        CHECK(!t->overlay_pair(ui,target).linked);
    }
    {
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto world=t->reset_success(1),ui=t->reset_success(2);CHECK(eye(*t,world,1,0));CHECK(t->execute(queue,&world,1));
        CHECK(!t->overlay_pair(ui,target).linked);CHECK(t->snapshot().overlay_no_executed_snapshots[0].executed_mask==1);
    }
    // Old UI generations and Present/invalidation cannot reuse executed provenance.
    for(unsigned boundary=0;boundary<4;++boundary){
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto world=t->reset_success(1),ui=t->reset_success(2);pair(*t,world);CHECK(t->execute(queue,&world,1));
        CHECK(t->overlay_pair(ui,target).linked);
        if(boundary==0){auto replacement=t->reset_success(2);CHECK(replacement.generation!=ui.generation);}
        if(boundary==1)t->present(buffer,queue);
        if(boundary==2)t->invalidate_list(ui.identity);
        if(boundary==3)t->invalidate_all();
        CHECK(!t->overlay_pair(ui,target).linked);
    }
    {
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto world=t->reset_success(1),ui=t->reset_success(2);pair(*t,world);CHECK(t->execute(queue,&world,1));
        auto recycled=t->reset_success(1); // Executed GPU work survives recording Reset by existing contract.
        CHECK(recycled.generation!=world.generation);CHECK(t->overlay_pair(ui,target).linked);
        auto p=t->overlay_pair(ui,target);CHECK(t->draw_overlay_pair(ui,1,target,p.eyes,3));CHECK(t->execute(queue,&ui,1));CHECK(t->present(buffer,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto first=t->reset_success(1),second=t->reset_success(2),ui=t->reset_success(3);
        pair(*t,first,1);pair(*t,second,2);CHECK(!t->overlay_pair(ui,target).linked);
        auto d=t->snapshot().overlay_no_executed_snapshots[0];CHECK(d.other_complete_recorded_lists==2);
        CHECK(!d.unique_recorded_ref.identity&&!d.unique_recorded_ticket);
    }
    {
        auto t=std::make_unique<Tracker>();Target target{buffer,1920,1080};
        auto local=t->reset_success(1);pair(*t,local);auto p=t->overlay_pair(local,target);
        CHECK(p.linked&&!p.from_executed&&!p.source_queue);CHECK(t->snapshot().overlay_local_queries==1);
    }
    // A rejected UI frame must not exhaust metadata merely because many draws
    // repeat the same unknown write. It still cannot be published, and the next
    // fresh window must retain its ordinary recovery behavior.
    for(unsigned position=0;position<3;++position){
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);
        if(position>0)CHECK(eye(*t,ref,1,0));
        if(position==2)CHECK(eye(*t,ref,1,1));
        for(unsigned i=0;i<1000;++i)CHECK(t->draw_unknown(ref,buffer));
        if(position==0)CHECK(eye(*t,ref,1,0));
        if(position<2)CHECK(eye(*t,ref,1,1));
        CHECK(t->snapshot().unknown_coalesced==999);
        CHECK(t->snapshot().event_highwater==3&&!t->snapshot().capacity_bad);
        CHECK(t->execute(queue,&ref,1));auto p=t->present(buffer,queue);
        CHECK(p.linked==(position==0));CHECK(p.contaminated==(position>0));
        auto next=t->reset_success(1);pair(*t,next,2);
        CHECK(t->execute(queue,&next,1));CHECK(t->present(buffer,queue).linked);
        CHECK(!t->draw_unknown(ref,buffer)); // No merge into a retired generation.
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);pair(*t,ref);
        t->invalidate(); // A transient metadata failure rejects the entire window.
        for(unsigned i=0;i<1000;++i)CHECK(t->draw_unknown(ref,buffer));
        CHECK(!t->execute(queue,&ref,1));CHECK(!t->present(buffer,queue).linked);
        CHECK(!t->snapshot().capacity_bad&&t->snapshot().event_overflow==0);
        auto next=t->reset_success(1);pair(*t,next,2);
        CHECK(t->execute(queue,&next,1));CHECK(t->present(buffer,queue).linked);
    }
    // One fixed owned resource; state recording is distinct from actual queue
    // submission. These CPU tests simulate original Execute returning between
    // prepare and commit, never claim GPU completion or native state discovery.
    const auto configure=[](Tracker& t,unsigned initial=4){CHECK(t.configure_owned_output({buffer,1920,1080},queue,9,initial,true));};
    const auto submit_owned=[](Tracker& t,const ListRef* members,size_t count){ExecutionToken token{};
        if(!t.prepare_execute(queue,members,count,token))return false;
        // Caller forwards original Execute exactly once here under its order lock.
        return t.commit_execute(token);};
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);(void)ref;
        t->present(0,0);CHECK(!t->configure_owned_output({buffer,1920,1080},queue,9,4,true));
        CHECK(!t->owned_status().enabled); // Even an emptied prior tracking window is not an activation boundary.
    }
    {
        auto t=std::make_unique<Tracker>();
        CHECK(!t->configure_owned_output({buffer,1920,1080},queue,0,4,true));
        CHECK(!t->configure_owned_output({buffer,1920,1080},queue,9,0,true));
        CHECK(!t->configure_owned_output({buffer,1920,1080},queue,9,4,false));
        configure(*t,128);CHECK(!t->configure_owned_output({buffer+1,1920,1080},queue,10,4,true));
        auto b=t->reset_success(2),a=t->reset_success(1); // Record order differs from execution order.
        CHECK(eye(*t,b,1,1));CHECK(t->record_transition(b,buffer,9,4,128,UINT32_MAX));
        CHECK(t->record_transition(a,buffer,9,128,4,0));CHECK(eye(*t,a,1,0));
        CHECK(t->owned_status().state==128&&t->owned_status().certified);
        ListRef members[]={a,b};ExecutionToken token{};CHECK(t->prepare_execute(queue,members,2,token));
        CHECK(t->owned_status().state==128);CHECK(t->commit_execute(token));
        auto result=t->seal_owned_output(buffer,9,queue);
        CHECK(result.linked&&result.state_certified&&result.ticket==1&&result.source_queue==queue&&result.owned_generation==9&&result.final_state==128);
        CHECK(t->owned_status().certified&&t->owned_status().state==128);
        auto next=t->reset_success(1);CHECK(t->record_transition(next,buffer,9,128,4));pair(*t,next,2);
        CHECK(t->record_transition(next,buffer,9,4,128));CHECK(submit_owned(*t,&next,1));
        CHECK(t->seal_owned_output(buffer,9,queue).linked);CHECK(t->owned_status().state==128);
        t->retire_present_interval();CHECK(t->owned_status().certified&&t->owned_status().state==128);
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);
        CHECK(t->record_owned_barrier_fault(ref,9));CHECK(t->owned_status().certified&&t->owned_status().state==4);
        ref=t->reset_success(1); // Unsubmitted unsupported recording was discarded.
        pair(*t,ref);CHECK(submit_owned(*t,&ref,1));CHECK(t->seal_owned_output(buffer,9,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t,128);auto a=t->reset_success(1),b=t->reset_success(2);
        CHECK(t->record_transition(a,buffer,9,128,4));CHECK(eye(*t,a,1,0));CHECK(eye(*t,b,1,1));
        CHECK(t->record_transition(b,buffer,9,4,128));ListRef reversed[]={b,a};ExecutionToken token{};
        CHECK(!t->prepare_execute(queue,reversed,2,token));CHECK(!t->owned_status().certified); // Actual batch order governs state.
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);
        CHECK(t->record_transition(ref,buffer,9,4,128,0,1));CHECK(!t->recorded_pair(ref,{buffer,1920,1080}).linked);
        CHECK(t->owned_status().certified); // Recording alone never changes submitted state.
    }
    for(unsigned fault=0;fault<10;++fault){auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);
        if(fault==0)CHECK(t->record_transition(ref,buffer,9,128,4)); // Actual before must match.
        if(fault==1)CHECK(t->record_transition(ref,buffer,9,4,128,1)); // Multi-subresource unsupported.
        if(fault==2)CHECK(t->record_transition(ref,buffer,9,4,128,0,1)); // BEGIN split.
        if(fault==3)CHECK(t->record_transition(ref,buffer,9,4,128,0,2)); // END split.
        if(fault==4)CHECK(t->record_transition(ref,buffer,10,4,128));
        if(fault==5)CHECK(t->record_transition(ref,buffer+1,9,4,128));
        if(fault==6)CHECK(t->record_transition(ref,buffer,9,4,8)); // UAV unsupported.
        if(fault==7)CHECK(t->record_owned_barrier_fault(ref,9)); // Aliasing/wildcard/unknown coverage.
        if(fault==8)CHECK(t->record_transition(ref,buffer,9,4,128|64)); // Uncertified combined read flags.
        if(fault==9)CHECK(t->record_transition(ref,buffer,9,4,16)); // Depth texture state unsupported.
        CHECK(t->owned_status().certified&&t->owned_status().state==4);
        ExecutionToken untouched{123};CHECK(!t->prepare_execute(queue,&ref,1,untouched));CHECK(untouched.value==123);
        CHECK(!t->owned_status().certified&&t->owned_status().state==4);
        CHECK(!t->seal_owned_output(buffer,9,queue).linked);
        t->invalidate_all();CHECK(!t->owned_status().certified);CHECK(!t->configure_owned_output({buffer,1920,1080},queue,10,4,true));
    }
    for(unsigned fault=0;fault<8;++fault){auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);ExecutionToken token{};
        if(fault==0){CHECK(!t->prepare_execute(queue+1,&ref,1,token));}
        if(fault==1){ListRef repeated[]={ref,ref};CHECK(!t->prepare_execute(queue,repeated,2,token));}
        if(fault==2){auto stale=ref;t->reset_success(1);CHECK(!t->prepare_execute(queue,&stale,1,token));}
        if(fault==3){CHECK(t->prepare_execute(queue,&ref,1,token));CHECK(!t->commit_execute({token.value+1}));}
        if(fault==4){CHECK(t->prepare_execute(queue,&ref,1,token));t->reset_success(1);CHECK(!t->commit_execute(token));}
        if(fault==5){CHECK(t->prepare_execute(queue,&ref,1,token));CHECK(t->record_transition(ref,buffer,9,4,128));CHECK(!t->commit_execute(token));}
        if(fault==6){CHECK(t->prepare_execute(queue,&ref,1,token));ExecutionToken again{};CHECK(!t->prepare_execute(queue,&ref,1,again));CHECK(!t->commit_execute(token));}
        if(fault==7){CHECK(t->prepare_execute(queue,&ref,1,token));CHECK(t->commit_execute(token));CHECK(!t->commit_execute(token));}
        CHECK(!t->owned_status().certified);CHECK(!t->seal_owned_output(buffer,9,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t,128);auto ref=t->reset_success(1);pair(*t,ref);
        CHECK(!submit_owned(*t,&ref,1));CHECK(!t->owned_status().certified); // Final writes require RT at their event position.
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);
        CHECK(eye(*t,ref,1,0,buffer,1280,720));CHECK(eye(*t,ref,1,1,buffer,1280,720));
        CHECK(!submit_owned(*t,&ref,1));CHECK(!t->seal_owned_output(buffer,9,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);CHECK(eye(*t,ref,1,0));
        CHECK(submit_owned(*t,&ref,1));auto r=t->seal_owned_output(buffer,9,queue);CHECK(!r.linked&&!r.state_certified);
        CHECK(t->owned_status().certified); // Missing content pair does not fabricate state failure.
        ref=t->reset_success(1);pair(*t,ref,2);CHECK(submit_owned(*t,&ref,1));CHECK(t->seal_owned_output(buffer,9,queue).linked);
    }
    for(unsigned failure=0;failure<3;++failure){auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);CHECK(submit_owned(*t,&ref,1));
        CHECK(!t->seal_owned_output(failure==0?buffer+1:buffer,failure==1?10:9,failure==2?queue+1:queue).linked);CHECK(!t->owned_status().certified);}
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);
        for(size_t i=2;i<Tracker::events_per_list;++i)CHECK(t->record_transition(ref,buffer,9,4,4));
        CHECK(!t->record_transition(ref,buffer,9,4,128));CHECK(!submit_owned(*t,&ref,1));CHECK(!t->owned_status().certified);
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);CHECK(submit_owned(*t,&ref,1));
        auto ui=t->reset_success(2);auto p=t->overlay_pair(ui,{buffer,1920,1080});CHECK(p.linked&&p.from_executed);
        CHECK(t->draw_overlay_pair(ui,1,{buffer,1920,1080},p.eyes,3));CHECK(t->record_transition(ui,buffer,9,4,128));
        CHECK(submit_owned(*t,&ui,1));auto result=t->seal_owned_output(buffer,9,queue);CHECK(result.linked&&result.final_state==128);
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);CHECK(!t->execute(queue,&ref,1));
        CHECK(!t->owned_status().certified); // Owned mode cannot shortcut the original-call boundary.
    }
    // Native resource birth occurs after real legacy tracking has started.
    // These external proof flags simulate an unexposed allocation under the
    // caller's API-order lock, never pretend that the helper proves that fact.
    const auto birth=[](Tracker& t,unsigned state=4){
        return t.birth_owned_output({buffer,1920,1080},queue,9,state,true,true);};
    {
        auto t=std::make_unique<Tracker>();auto open=t->reset_success(1),done=t->reset_success(2);
        CHECK(eye(*t,done,68,0,buffer+1));CHECK(eye(*t,done,68,1,buffer+1));
        CHECK(t->execute(queue,&done,1));
        CHECK(eye(*t,open,70,0,buffer+2));CHECK(eye(*t,open,70,1,buffer+2));
        auto before=t->snapshot();CHECK(before.reset_calls==2&&before.execute_calls==1&&before.active_lists==2);
        CHECK(!t->configure_owned_output({buffer,1920,1080},queue,9,4,true));
        CHECK(birth(*t));auto after=t->snapshot();
        CHECK(after.reset_calls==before.reset_calls&&after.execute_calls==before.execute_calls&&after.active_lists==before.active_lists);
        CHECK(after.list_highwater==before.list_highwater&&after.event_highwater==before.event_highwater);
        CHECK(after.owned_births==1&&after.owned_birth_discarded_events==4&&after.owned_birth_discarded_candidates==1);
        CHECK(t->current(open.identity).generation==open.generation&&t->current(done.identity).generation==done.generation);
        CHECK(!t->recorded_pair(open,{buffer+2,1920,1080}).linked); // No old association survives.
        pair(*t,open,71);CHECK(t->record_transition(open,buffer,9,4,128));
        CHECK(submit_owned(*t,&open,1));auto result=t->seal_owned_output(buffer,9,queue);
        CHECK(result.linked&&result.ticket==71&&result.final_state==128&&result.state_certified);
        CHECK(t->snapshot().reset_calls==2); // Continued the actual open list, no fabricated Reset.
        auto next=t->reset_success(1);CHECK(next.generation>done.generation&&next.generation>open.generation);
    }
    {
        auto t=std::make_unique<Tracker>();auto done=t->reset_success(1);
        CHECK(t->execute(queue,&done,1));CHECK(birth(*t));
        ExecutionToken unchanged{123};CHECK(!t->prepare_execute(queue,&done,1,unchanged));CHECK(unchanged.value==123);
        CHECK(t->snapshot().duplicate_batches==1); // Birth must not reset the real executed marker.
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);
        CHECK(eye(*t,ref,5,0,buffer+1));CHECK(birth(*t));
        CHECK(!eye(*t,ref,5,0)); // Birth retires pre-existing recorded tickets too.
        CHECK(t->snapshot().reset_calls==1);
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);
        t->retire_present_interval();CHECK(birth(*t,128));
        CHECK(!t->current(ref.identity).identity); // Already retired lists are not resurrected.
        auto next=t->reset_success(1);CHECK(next.generation>ref.generation);
        CHECK(t->record_transition(next,buffer,9,128,4));pair(*t,next,1);
        CHECK(submit_owned(*t,&next,1));CHECK(t->seal_owned_output(buffer,9,queue).linked);
    }
    for(unsigned invalid=0;invalid<11;++invalid){
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);
        CHECK(eye(*t,ref,3,0,buffer+1));CHECK(eye(*t,ref,3,1,buffer+1));
        auto target=Target{buffer,1920,1080};auto q=queue;uint64_t generation=9;unsigned state=4;
        bool normal=true,boundary=true;
        if(invalid==0)normal=false;if(invalid==1)boundary=false;if(invalid==2)target.resource=0;
        if(invalid==3)q=0;if(invalid==4)generation=0;if(invalid==5)state=0;
        if(invalid==6)target.width=1921;if(invalid==7)target.height=0;
        if(invalid==8)target.width=8194;if(invalid==9)target.height=8193;
        if(invalid==10)target.resource=buffer+1; // Actual prior use contradicts claimed birth.
        CHECK(!t->birth_owned_output(target,q,generation,state,normal,boundary));
        CHECK(!t->owned_status().enabled&&t->snapshot().owned_births==0);
        CHECK(t->current(1).generation==ref.generation&&t->snapshot().reset_calls==1);
        CHECK(t->recorded_pair(ref,{buffer+1,1920,1080}).linked);
        CHECK(t->execute(queue,&ref,1));CHECK(t->present(buffer+1,queue).linked); // Failure left legacy work intact.
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);pair(*t,ref);
        CHECK(t->execute(queue,&ref,1));t->reset_success(1);CHECK(!birth(*t));
        CHECK(t->present(buffer,queue).linked); // Existing executed candidate is a birth contradiction.
    }
    {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);
        for(size_t i=0;i<Tracker::events_per_list;++i)CHECK(t->draw_unknown(ref,1000+i));
        CHECK(!t->draw_unknown(ref,2000));const auto before=t->snapshot();CHECK(!birth(*t));
        CHECK(t->snapshot().event_overflow==before.event_overflow&&t->snapshot().capacity_bad);
        CHECK(t->current(1).generation==ref.generation&&t->snapshot().reset_calls==1);
    }
    for(unsigned invalid=0;invalid<3;++invalid){
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);
        if(invalid==0)t->invalidate();
        if(invalid==1)t->invalidate_capacity();
        if(invalid==2)t->invalidate_list(ref.identity);
        const auto before=t->snapshot();CHECK(!birth(*t));auto after=t->snapshot();
        CHECK(!t->owned_status().enabled&&after.reset_calls==before.reset_calls&&after.capacity_bad==before.capacity_bad);
        CHECK(after.owned_births==0); // No recovery from loss of coverage at a creation boundary.
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);
        ExecutionToken token{};CHECK(t->prepare_execute(queue,&ref,1,token));
        CHECK(!t->birth_owned_output({buffer+1,1920,1080},queue,10,4,true,true));
        CHECK(t->commit_execute(token));CHECK(t->seal_owned_output(buffer,9,queue).linked); // Pending output untouched.
        CHECK(!birth(*t)); // No replacement even after successful seal.
    }
    // Continuous consumption keeps real list history and submitted resource
    // state. Synthetic API ordering here; actual GPU checks live separately.
    {
        auto t=std::make_unique<Tracker>();configure(*t,128);
        auto open=t->reset_success(2);CHECK(t->draw_unknown(open,buffer+1));
        for(uint64_t ticket=1;ticket<=100;++ticket){
            auto ref=t->reset_success(1);CHECK(t->record_transition(ref,buffer,9,128,4));
            pair(*t,ref,ticket);CHECK(t->record_transition(ref,buffer,9,4,128));
            CHECK(submit_owned(*t,&ref,1));auto before=t->snapshot();
            auto result=t->consume_owned_output(buffer,9,queue,ticket);
            CHECK(result.linked&&result.state_certified&&result.ticket==ticket&&result.final_state==128);
            CHECK(t->current(ref.identity).generation==ref.generation);
            CHECK(t->current(open.identity).generation==open.generation);
            CHECK(t->snapshot().reset_calls==before.reset_calls&&t->snapshot().active_lists==2);
            CHECK(t->owned_status().certified&&t->owned_status().state==128);
        }
        CHECK(t->snapshot().owned_consumed_frames==100);
        CHECK(submit_owned(*t,&open,1)); // Unrelated unsubmitted recording survived all 100 frames.
        auto next=t->reset_success(2);CHECK(t->record_transition(next,buffer,9,128,4));pair(*t,next,101);
        CHECK(submit_owned(*t,&next,1));CHECK(t->consume_owned_output(buffer,9,queue,101).linked);
    }
    for(unsigned fault=0;fault<13;++fault){
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);
        CHECK(submit_owned(*t,&ref,1));auto open=t->reset_success(2);
        if(fault==0)CHECK(t->record_transition(open,buffer,9,4,128)); // Recorded but not submitted.
        if(fault==1)CHECK(t->draw_unknown(open,buffer));
        if(fault==2)pair(*t,open,2); // Later ticket must never be silently discarded.
        if(fault==3){ExecutionToken token{};CHECK(t->prepare_execute(queue,&open,1,token));}
        if(fault==4)t->invalidate_capacity();
        if(fault==5)t->invalidate_list(open.identity);
        if(fault==6){CHECK(t->draw_unknown(open,buffer));CHECK(submit_owned(*t,&open,1));}
        if(fault==7){CHECK(t->record_transition(open,buffer,9,4,1024));CHECK(submit_owned(*t,&open,1));}
        const auto before=t->snapshot();
        auto result=t->consume_owned_output(fault==8?buffer+1:buffer,fault==9?10:9,fault==10?queue+1:queue,
                                            fault==11?0:fault==12?2:1);
        CHECK(!result.linked&&!result.state_certified&&!t->owned_status().certified);
        CHECK(t->snapshot().owned_consumed_frames==0&&t->snapshot().reset_calls==before.reset_calls);
        CHECK(t->current(ref.identity).generation==ref.generation); // Failure did not erase executed history.
        CHECK(!t->birth_owned_output({buffer,1920,1080},queue,10,4,true,true));
    }
    for(unsigned misuse=0;misuse<3;++misuse){
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);
        CHECK(submit_owned(*t,&ref,1));CHECK(t->consume_owned_output(buffer,9,queue,1).linked);
        if(misuse==0)CHECK(!submit_owned(*t,&ref,1)); // Executed marker must survive consumption.
        if(misuse==1)CHECK(!t->consume_owned_output(buffer,9,queue,1).linked); // No duplicate publication proof.
        if(misuse==2){auto next=t->reset_success(1);CHECK(!eye(*t,next,1,0));CHECK(!submit_owned(*t,&next,1));}
        CHECK(!t->owned_status().certified);
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);pair(*t,ref);
        CHECK(submit_owned(*t,&ref,1));auto discarded=t->reset_success(2);
        CHECK(t->record_owned_barrier_fault(discarded,9));t->reset_success(2); // Real Reset discards recording.
        CHECK(t->consume_owned_output(buffer,9,queue,1).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto open=t->reset_success(1),done=t->reset_success(2);
        CHECK(t->draw_unknown(open,buffer+1));CHECK(t->draw_unknown(done,buffer+1));CHECK(t->execute(queue,&done,1));
        auto invalid=t->reset_success(3);t->invalidate_list(3);
        const auto before=t->snapshot();CHECK(t->retire_before_owned_birth());
        CHECK(t->current(1).generation==open.generation&&t->current(2).generation==done.generation&&!t->current(3).identity);
        CHECK(t->snapshot().reset_calls==before.reset_calls&&t->snapshot().active_lists==before.active_lists);
        CHECK(birth(*t));CHECK(!t->retire_before_owned_birth());
        CHECK(eye(*t,open,1,0));CHECK(eye(*t,open,1,1));CHECK(submit_owned(*t,&open,1));
        CHECK(t->consume_owned_output(buffer,9,queue,1).linked);CHECK(!submit_owned(*t,&done,1));
        CHECK(!t->owned_status().certified); // Prior execution was retained, not converted to a fresh recording.
    }
    {
        auto t=std::make_unique<Tracker>();t->invalidate_capacity();CHECK(t->retire_before_owned_birth());
        CHECK(t->snapshot().capacity_bad&&!birth(*t));
    }
    for(unsigned failure=0;failure<3;++failure){
        auto t=std::make_unique<Tracker>();auto known=t->reset_success(1);
        ListRef batch[]{known,failure==1?known:ListRef{99,1}};
        CHECK(!t->execute(queue,batch,2)); // Original API still submits this rejected batch.
        ListRef next{};if(failure==2)next=t->reset_success(1); // Reentrant Reset must not mark the new generation.
        CHECK(t->submitted_before_owned_birth(batch,2));CHECK(!t->current(99).identity);
        CHECK(t->retire_before_owned_birth());CHECK(birth(*t));
        CHECK(!t->submitted_before_owned_birth(batch,2)); // Cannot change post-birth history this way.
        if(failure==2){pair(*t,next);CHECK(submit_owned(*t,&next,1));CHECK(t->consume_owned_output(buffer,9,queue,1).linked);}
        else {CHECK(!submit_owned(*t,&known,1));CHECK(t->snapshot().duplicate_batches>0);CHECK(!t->owned_status().certified);}
    }
    for(bool initially_open:{true,false}){
        auto t=std::make_unique<Tracker>();auto created=t->create_success(1,initially_open);
        CHECK(created.identity==1&&t->snapshot().list_creates==1&&t->snapshot().reset_calls==0);
        CHECK(bool(t->current(1).identity)==initially_open);
        CHECK(t->retire_before_owned_birth());CHECK(birth(*t));
        if(initially_open){pair(*t,created);CHECK(submit_owned(*t,&created,1));CHECK(t->consume_owned_output(buffer,9,queue,1).linked);}
        auto reset=t->reset_success(1);CHECK(reset.generation>created.generation&&t->snapshot().reset_calls==1);
        pair(*t,reset,2);CHECK(submit_owned(*t,&reset,1));CHECK(t->consume_owned_output(buffer,9,queue,2).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto closed=t->create_success(1,false);
        CHECK(!t->execute(queue,&closed,1));CHECK(!t->current(1).identity);
        t->retire_present_interval();CHECK(!t->configure_owned_output({buffer,1920,1080},queue,9,4,true));
    }
    {
        auto t=std::make_unique<Tracker>();CHECK(!t->has_unsubmitted_owned_events(buffer));configure(*t);
        auto world=t->reset_success(1);pair(*t,world);CHECK(t->has_unsubmitted_owned_events(buffer));
        CHECK(submit_owned(*t,&world,1));CHECK(!t->has_unsubmitted_owned_events(buffer));
        auto unrelated=t->reset_success(3);CHECK(t->draw_unknown(unrelated,buffer+1));
        CHECK(!t->has_unsubmitted_owned_events(buffer));CHECK(!t->has_unsubmitted_owned_events(buffer+1));
        CHECK(!t->has_unsubmitted_owned_events(0));
        auto ui=t->reset_success(2);auto p=t->overlay_pair(ui,{buffer,1920,1080});CHECK(p.linked);
        CHECK(t->draw_overlay_pair(ui,1,{buffer,1920,1080},p.eyes,3));
        const auto before=t->snapshot();const auto owned_before=t->owned_status();
        for(unsigned i=0;i<3;++i)CHECK(t->has_unsubmitted_owned_events(buffer));
        CHECK(!t->has_unsubmitted_owned_events(buffer+1));
        CHECK(t->snapshot().owned_consumed_frames==before.owned_consumed_frames);
        CHECK(t->snapshot().execute_calls==before.execute_calls&&t->snapshot().overlay_recorded==before.overlay_recorded);
        CHECK(t->owned_status().certified==owned_before.certified&&t->owned_status().state==owned_before.state);
        ExecutionToken token{};CHECK(t->prepare_execute(queue,&ui,1,token));
        CHECK(t->has_unsubmitted_owned_events(buffer));CHECK(!t->has_unsubmitted_owned_events(buffer+1));
        CHECK(t->commit_execute(token));CHECK(!t->has_unsubmitted_owned_events(buffer));
        // Pending submission itself must block consumption even when its only
        // recorded events name another resource.
        CHECK(t->prepare_execute(queue,&unrelated,1,token));CHECK(t->has_unsubmitted_owned_events(buffer));
        CHECK(t->commit_execute(token));CHECK(!t->has_unsubmitted_owned_events(buffer));
        auto result=t->consume_owned_output(buffer,9,queue,1);CHECK(result.linked&&result.state_certified);
        CHECK(!t->has_unsubmitted_owned_events(buffer));
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);auto ref=t->reset_success(1);
        CHECK(t->record_transition(ref,buffer,9,4,4));CHECK(t->has_unsubmitted_owned_events(buffer));
        t->reset_success(1);CHECK(!t->has_unsubmitted_owned_events(buffer)); // Real Reset discards recording.
    }
    for(bool recreate:{false,true})for(bool committed:{false,true}) {
        auto t=std::make_unique<Tracker>();configure(*t);
        auto world=t->reset_success(1);pair(*t,world);CHECK(submit_owned(*t,&world,1));
        auto ui=t->reset_success(2);auto p=t->overlay_pair(ui,{buffer,1920,1080});CHECK(p.linked);
        CHECK(t->draw_overlay_pair(ui,1,{buffer,1920,1080},p.eyes,3));
        if(committed)CHECK(submit_owned(*t,&ui,1));
        auto next=recreate?t->create_success(2,true):t->reset_success(2);
        CHECK(next.generation>ui.generation);
        CHECK(!t->has_unsubmitted_owned_events(buffer));
        auto result=t->consume_owned_output(buffer,9,queue,1);
        CHECK(result.linked==committed&&result.state_certified==committed);
        CHECK(t->owned_status().certified==committed);
    }
    // Discarding unrelated overlays must not lose owned certification.
    for(bool recreate:{false,true}) {
        auto t=std::make_unique<Tracker>();configure(*t);
        auto world=t->reset_success(1);pair(*t,world);CHECK(submit_owned(*t,&world,1));
        auto other=t->reset_success(2);CHECK(eye(*t,other,1,0,buffer+1));CHECK(eye(*t,other,1,1,buffer+1));
        auto p=t->overlay_pair(other,{buffer+1,1920,1080});CHECK(p.linked);
        CHECK(t->draw_overlay_pair(other,1,{buffer+1,1920,1080},p.eyes,3));
        if(recreate)t->create_success(2,true);else t->reset_success(2);
        CHECK(t->owned_status().certified);CHECK(t->consume_owned_output(buffer,9,queue,1).linked);
    }
    // Before output birth, overlays remain ordinary discardable recordings.
    for(bool recreate:{false,true}) {
        auto t=std::make_unique<Tracker>();auto ref=t->reset_success(1);pair(*t,ref);
        auto p=t->overlay_pair(ref,{buffer,1920,1080});CHECK(p.linked);
        CHECK(t->draw_overlay_pair(ref,1,{buffer,1920,1080},p.eyes,3));
        if(recreate)t->create_success(1,true);else t->reset_success(1);
        CHECK(birth(*t));CHECK(t->owned_status().certified);
    }
    // A world boundary retires association without manufacturing Reset history.
    for(bool recreate:{false,true}){
        auto t=std::make_unique<Tracker>();auto old=t->reset_success(1);pair(*t,old,10);
        CHECK(t->world_handoff_ready(1,2));CHECK(!t->world_handoff_ready(1,3));
        auto before=t->snapshot();CHECK(t->advance_world_epoch(1,2));
        CHECK(t->world_epoch()==2&&t->snapshot().world_epoch==2);
        CHECK(t->current(1).generation==old.generation);
        CHECK(t->snapshot().active_lists==before.active_lists&&t->snapshot().reset_calls==before.reset_calls);
        CHECK(!t->advance_world_epoch(1,2)&&!t->advance_world_epoch(2,2)&&!t->advance_world_epoch(2,4));
        CHECK(!t->advance_world_epoch(UINT64_MAX,0));
        CHECK(!t->overlay_pair(old,{buffer,1920,1080}).linked);
        CHECK(!t->execute(queue,&old,1)); // Recorded before switch, executed after.
        CHECK(!t->present(buffer,queue).linked);
        CHECK(!t->current(1).identity); // Present kept its existing seal semantics.
        CHECK(!t->execute(queue,&old,1)); // Erased old generation stays unknown.
        CHECK(!t->present(buffer,queue).linked);
        auto fresh=recreate?t->create_success(1,true):t->reset_success(1);
        CHECK(fresh.generation>old.generation);pair(*t,fresh,11);
        CHECK(t->execute(queue,&fresh,1));CHECK(t->present(buffer,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto old=t->reset_success(1);pair(*t,old,20);
        CHECK(t->execute(queue,&old,1));CHECK(t->advance_world_epoch(1,2));
        CHECK(!t->present(buffer,queue).linked); // Previous candidate cannot survive.
        auto fresh=t->reset_success(1);CHECK(!eye(*t,fresh,20,0));
        CHECK((t->present(buffer,queue).rejection&stale_ticket)!=0);
        auto late=t->reset_success(2);CHECK(t->advance_world_epoch(2,3));
        CHECK(!eye(*t,late,21,0)); // Old list cannot record a new-world ticket.
        CHECK(!t->draw_unknown(late,buffer));CHECK(!t->present(buffer,queue).linked);
        auto current=t->reset_success(2);pair(*t,current,22);
        CHECK(t->execute(queue,&current,1));CHECK(t->present(buffer,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto old=t->create_success(1,true);pair(*t,old,30);
        CHECK(t->advance_world_epoch(1,2));
        auto fresh=t->reset_success(1);CHECK(fresh.generation>old.generation);
        pair(*t,fresh,31);CHECK(t->execute(queue,&fresh,1));CHECK(t->present(buffer,queue).linked);
        auto closed=t->create_success(2,false);CHECK(t->advance_world_epoch(2,3));
        CHECK(!t->execute(queue,&closed,1));CHECK(!t->present(buffer,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();t->invalidate_capacity();
        CHECK(t->advance_world_epoch(1,2));CHECK(t->snapshot().capacity_bad);
        auto fresh=t->reset_success(1);pair(*t,fresh);
        CHECK(!t->execute(queue,&fresh,1));CHECK((t->present(buffer,queue).rejection&capacity)!=0);
    }
    {
        auto t=std::make_unique<Tracker>();configure(*t);
        CHECK(!t->world_handoff_ready(1,2));
        CHECK(!t->advance_world_epoch(1,2));CHECK(t->world_epoch()==1&&t->owned_status().certified);
        auto ref=t->reset_success(1);pair(*t,ref);ExecutionToken token{};
        CHECK(t->prepare_execute(queue,&ref,1,token));
        CHECK(!t->advance_world_epoch(1,2)); // Refusal must not disturb pending transaction.
        CHECK(t->commit_execute(token));CHECK(t->consume_owned_output(buffer,9,queue,1).linked);
        CHECK(!t->advance_world_epoch(1,2));
    }
    {
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);
        CHECK(!t->has_recorded_target(a,buffer));CHECK(t->draw_unknown(a,buffer+2));
        CHECK(!t->has_recorded_target(a,buffer+2));CHECK(eye(*t,a,1,0));
        CHECK(t->has_recorded_target(a,buffer));CHECK(!t->has_recorded_target(a,0));
        CHECK(eye(*t,a,1,1));CHECK(t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));
        CHECK(t->has_recorded_target(a,buffer)&&t->has_recorded_target(a,buffer+1));
        CHECK(t->execute(queue,&a,1));CHECK(!t->has_recorded_target(a,buffer));
        CHECK(t->present(buffer+1,queue).linked);
        auto b=t->reset_success(1);CHECK(!t->has_recorded_target(a,buffer));CHECK(!t->has_recorded_target(b,buffer));
        pair(*t,b,2);CHECK(t->advance_world_epoch(1,2));CHECK(!t->has_recorded_target(b,buffer));
    }
    for(unsigned retirement=0;retirement<3;++retirement){
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        CHECK(!t->has_transfer_endpoint(buffer));CHECK(!t->has_transfer_endpoint(0));
        CHECK(t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));
        CHECK(!t->has_transfer_endpoint(buffer+1));CHECK(t->execute(queue,&a,1));
        CHECK(t->has_transfer_endpoint(buffer)&&t->has_transfer_endpoint(buffer+1));
        CHECK(!t->has_transfer_endpoint(buffer+2)&&!t->has_transfer_endpoint(0));
        auto b=t->reset_success(2);CHECK(!t->has_recorded_target(b,buffer));
        if(t->has_transfer_endpoint(buffer))CHECK(t->draw_unknown(b,buffer));
        CHECK(t->execute(queue,&b,1));
        if(retirement==0)CHECK(!t->present(buffer+1,queue).linked);
        else if(retirement==1)t->invalidate_all();
        else CHECK(t->advance_world_epoch(1,2));
        CHECK(!t->has_transfer_endpoint(buffer)&&!t->has_transfer_endpoint(buffer+1));
    }
    // Explicit same-recording image transfer preserves the ticket through UI
    // before and after the move, including later submitted UI continuations.
    for(bool continuation:{false,true}){
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        Target source{buffer,1920,1080},dest{buffer+1,1920,1081};
        auto before=t->overlay_pair(a,source);CHECK(before.linked);
        CHECK(t->draw_overlay_pair(a,1,source,before.eyes,3));
        CHECK(t->draw_transfer(a,source,dest));
        CHECK(!t->overlay_pair(a,source).linked);
        auto after=t->overlay_pair(a,dest);CHECK(after.linked&&after.ticket==1&&!after.from_executed);
        CHECK(after.eyes[0].height==1081&&after.eyes[1].x==960);
        CHECK(t->draw_overlay_pair(a,1,dest,after.eyes,3));
        CHECK(t->execute(queue,&a,1));
        if(continuation){auto ui=t->reset_success(2);auto p=t->overlay_pair(ui,dest);
            CHECK(p.linked&&p.from_executed&&p.ticket==1);
            CHECK(t->draw_overlay_pair(ui,1,dest,p.eyes,3));CHECK(t->execute(queue,&ui,1));}
        auto result=t->present(dest.resource,queue);CHECK(result.linked&&result.ticket==1&&result.target.height==1081);
    }
    // Incomplete/local-generation/geometry/ownership admission failures.
    for(unsigned mode=0;mode<10;++mode){
        auto t=std::make_unique<Tracker>();if(mode==9)CHECK(t->configure_owned_output({buffer,1920,1080},queue,1,4,true));
        auto a=t->reset_success(1);CHECK(eye(*t,a,1,0));if(mode!=0)CHECK(eye(*t,a,1,1));
        Target source{buffer,1920,1080},dest{buffer+1,1920,1081};
        if(mode==1)CHECK(t->draw_unknown(a,buffer));
        if(mode==2)CHECK(t->draw_unknown(a,buffer+1));
        if(mode==3)t->reset_success(1);
        if(mode==4)CHECK(t->advance_world_epoch(1,2));
        if(mode==5)dest.width=1919;
        if(mode==6)dest.height=0;
        if(mode==7)source.height=1079;
        if(mode==8)dest.resource=source.resource;
        CHECK(!t->draw_transfer(a,source,dest));CHECK(!t->present(dest.resource,queue).linked);
    }
    // The source may not be borrowed from a different recording, even if it
    // has already executed and would qualify for an ordinary UI continuation.
    for(bool submitted:{false,true}){
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        if(submitted)CHECK(t->execute(queue,&a,1));auto b=t->reset_success(2);
        CHECK(!t->draw_transfer(b,{buffer,1920,1080},{buffer+1,1920,1081}));
        CHECK(!t->present(buffer+1,queue).linked);
    }
    for(unsigned mode=0;mode<9;++mode){
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        CHECK(t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));
        if(mode==0)CHECK(!t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));
        if(mode==1)CHECK(!t->draw_transfer(a,{buffer+1,1920,1081},{buffer+2,1920,1081}));
        if(mode==2)CHECK(!t->draw_transfer(a,{buffer,1920,1080},{buffer+2,1920,1081}));
        if(mode==3||mode==4)CHECK(t->draw_unknown(a,buffer+(mode==4)));
        if(mode==5){t->reset_success(1);CHECK(!t->execute(queue,&a,1));}
        else if(mode==6){CHECK(t->advance_world_epoch(1,2));CHECK(!t->execute(queue,&a,1));}
        else {t->execute(queue,&a,1);if(mode==7)CHECK(!t->execute(queue,&a,1));}
        CHECK(!t->present(buffer+1,mode==8?queue+1:queue).linked);
    }
    // Source writes after the transfer, including another submitted list,
    // invalidate the moved pair instead of resurrecting the old source.
    for(bool destination:{false,true}){
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        CHECK(t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));CHECK(t->execute(queue,&a,1));
        auto b=t->reset_success(2);CHECK(t->draw_unknown(b,buffer+destination));t->execute(queue,&b,1);
        CHECK(!t->present(buffer+1,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);CHECK(eye(*t,a,1,0));
        CHECK(t->draw_unknown(a,buffer));CHECK(eye(*t,a,1,1));
        CHECK(!t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));
        CHECK(!t->present(buffer+1,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto earlier=t->reset_success(2);
        CHECK(t->draw_unknown(earlier,buffer+1));CHECK(t->execute(queue,&earlier,1));
        auto a=t->reset_success(1);pair(*t,a);
        CHECK(t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));
        CHECK(!t->execute(queue,&a,1));CHECK(!t->present(buffer+1,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        CHECK(t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));CHECK(t->execute(queue,&a,1));
        CHECK(t->present(buffer+1,queue).linked);auto b=t->reset_success(1);
        CHECK(!eye(*t,b,1,0));CHECK(!t->draw_transfer(b,{buffer,1920,1080},{buffer+1,1920,1081}));
        CHECK(!t->present(buffer+1,queue).linked);
    }
    // A full event list can still merge an exact destination overlay. Its
    // prefix source query must not mistake the full suffix for source overflow.
    {
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        CHECK(t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));
        for(unsigned i=3;i<Tracker::events_per_list-1;++i)CHECK(t->draw_unknown(a,1000+i));
        auto p=t->overlay_pair(a,{buffer+1,1920,1081});CHECK(p.linked);
        CHECK(t->draw_overlay_pair(a,1,p.target,p.eyes,3));CHECK(t->overlay_pair(a,p.target).linked);
        CHECK(t->draw_overlay_pair(a,1,p.target,p.eyes,3));CHECK(t->execute(queue,&a,1));
        CHECK(t->present(buffer+1,queue).linked);
    }
    // Ordinary finals still cannot assign one ticket to two output resources.
    {
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        CHECK(t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));
        CHECK(eye(*t,a,1,0,buffer+2));CHECK(!t->execute(queue,&a,1));CHECK(!t->present(buffer+1,queue).linked);
    }
    {
        auto t=std::make_unique<Tracker>();auto a=t->reset_success(1);pair(*t,a);
        for(unsigned i=2;i<Tracker::events_per_list;++i)CHECK(t->draw_unknown(a,1000+i));
        CHECK(!t->draw_transfer(a,{buffer,1920,1080},{buffer+1,1920,1081}));CHECK(!t->present(buffer+1,queue).linked);
    }
    std::printf("frame_link: %u checks passed\n", checks);
}
