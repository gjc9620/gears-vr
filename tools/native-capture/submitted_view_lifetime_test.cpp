#include "submitted_view_lifetime.h"
#include <atomic>
#include <thread>
#include <cstdio>
#include <memory>
int main() {
    // Independent 64-frame stores exceed the default Windows stack as
    // frame certificates grow. Production owns its store outside this stack.
    auto stores=std::make_unique<gears_submitted::Lifetime[]>(12);
    auto& life=stores[0];
    if(life.begin(0) || !life.begin(1) || life.begin(1) || !life.begin(2))return 1;
    if(life.close() || life.begin(3) || life.retire(99) || life.retire(1))return 2;
    if(!life.retire(2) || life.close() || life.retire(2) || life.pending())return 3;
    auto& pool=stores[1];
    for(unsigned i=1;i<=64;++i)if(!pool.begin(i))return 4;
    if(pool.begin(65) || pool.retire(1) || !pool.begin(65))return 5;
    if(pool.close())return 6;
    std::atomic<unsigned> claims{};
    auto retire=[&](unsigned first,unsigned last){for(unsigned i=first;i<=last;++i)if(pool.retire(i))++claims;};
    std::thread a(retire,2,33),b(retire,34,65);a.join();b.join();
    if(claims!=1 || pool.pending() || pool.close())return 7;
    auto& reused=stores[2];
    gears_projection::Metadata metadata{};metadata.enabled=true;metadata.phase=gears_projection::Phase::b;metadata.pair_qpc=111;
    gears_projection::Metadata read{};
    if(!reused.begin(42,metadata) || !reused.projection(42,read) || read.pair_qpc!=111 || read.phase!=metadata.phase)return 9;
    if(reused.retire(42) || reused.projection(42,read) || read.enabled)return 10;
    metadata.phase=gears_projection::Phase::c;metadata.pair_qpc=222;
    if(!reused.begin(42,metadata) || !reused.projection(42,read) || read.pair_qpc!=222 || read.phase!=metadata.phase || reused.retire(42))return 11;
    if(!reused.begin(42) || reused.retire(42) || !reused.begin(42) || reused.close() || !reused.retire(42))return 8;
    auto& frames=stores[3];gears_submitted::FrameTicket ticket{},copied{};
    ticket.enabled=true;ticket.id=1;ticket.world_generation=3;ticket.sample.session=7;ticket.sample.sequence=111;ticket.expected.view[1][12]=3.2f;
    ticket.ngx_constructor_source={1,3,0x100,0x200,{2560,1440,3840,2160,2,67,0}};
    const auto frozen_create=ticket.ngx_constructor_source;
    if(!frames.begin(42,{},ticket))return 12;
    ticket.world_generation=4;ticket.sample.sequence=222;ticket.expected.view[1][12]=9;
    ++ticket.ngx_constructor_source.revision;++ticket.ngx_constructor_source.world;
    ticket.ngx_constructor_source.fields[0]=1920;
    if(!frames.frame(42,copied) || copied.sample.sequence!=111 || copied.expected.view[1][12]!=3.2f)return 13;
    if(copied.id!=1||copied.world_generation!=3)return 17;
    if(!copied.ngx_constructor_source.matches(frozen_create)||copied.ngx_constructor_source.matches(ticket.ngx_constructor_source))return 41;
    copied.ngx_constructor_source={};if(!frames.frame(42,copied)||!copied.ngx_constructor_source.matches(frozen_create))return 42;
    copied.sample.sequence=999;if(!frames.frame(42,copied) || copied.sample.sequence!=111)return 14;
    if(frames.retire(42) || frames.frame(42,copied) || copied.enabled||copied.ngx_constructor_source.revision)return 15;
    if(!frames.begin(42,{},ticket) || !frames.frame(42,copied) || copied.sample.sequence!=222 ||
       copied.world_generation!=4||copied.expected.view[1][12]!=9 || frames.close() || !frames.retire(42) || frames.frame(42,copied))return 16;
    auto& same_pose=stores[4];ticket.id=10;
    if(!same_pose.begin(1,{},ticket))return 18;
    ticket.id=11;if(!same_pose.begin(2,{},ticket) || !same_pose.frame(1,copied) || copied.id!=10 ||
       copied.sample.sequence!=ticket.sample.sequence || !same_pose.frame(2,copied) || copied.id!=11)return 19;
    auto& idle_stop=stores[5];idle_stop.request_stop();
    if(!idle_stop.stopping()||idle_stop.is_closed()||idle_stop.begin(1)||!idle_stop.close()||idle_stop.close())return 20;
    auto& pending_stop=stores[6];
    if(!pending_stop.begin(1)||!pending_stop.begin(2))return 21;
    pending_stop.request_stop();pending_stop.request_stop();
    if(pending_stop.is_closed()||pending_stop.begin(3)||pending_stop.retire(1)||pending_stop.close()||!pending_stop.retire(2)||pending_stop.close())return 22;
    // Admission remains closed after request_stop returns, even under producers.
    auto& racing=stores[7];
    std::atomic<bool> stopping_done{},bad_admission{};
    auto produce=[&](unsigned root){for(unsigned i=0;i<10000;++i){
        bool after=stopping_done.load();bool accepted=racing.begin(root);
        if(after&&accepted)bad_admission=true;
        if(accepted&&racing.retire(root))bad_admission=true;
    }};
    std::thread c(produce,1),d(produce,2);
    racing.request_stop();stopping_done=true;c.join();d.join();
    if(bad_admission||racing.pending()||racing.is_closed()||!racing.close()||racing.close())return 23;
    auto& recover=stores[8];
    if(!recover.begin(10))return 24;
    auto q=recover.quiesce_close(1);
    if(!q.accepted||q.release_claim||recover.begin(11)||recover.confirm_destroyed(q.token).owner||recover.reopen({&recover,1}))return 25;
    auto repeated=recover.quiesce_close(1);
    if(!repeated.accepted||repeated.release_claim||repeated.token.generation!=q.token.generation)return 37;
    if(!recover.retire(10)||recover.retire(10))return 26;
    if(recover.quiesce_close(1).release_claim||recover.reopen({&recover,1}))return 38;
    auto snap=recover.snapshot();
    if(snap.generation!=1||snap.pending||!snap.claimed||!snap.quiescing||snap.destruction_confirmed)return 27;
    auto done=recover.confirm_destroyed(q.token);
    if(!done.owner||recover.confirm_destroyed(q.token).owner||!recover.reopen(done)||recover.reopen(done))return 28;
    if(recover.quiesce_close(1).accepted||recover.confirm_destroyed(q.token).owner||!recover.begin(10))return 29;
    auto q2=recover.quiesce_close(2);
    if(!q2.accepted||q2.release_claim||!recover.retire(10)||recover.reopen(done))return 30;
    auto done2=recover.confirm_destroyed(q2.token);
    if(!done2.owner||!recover.reopen(done2)||recover.snapshot().generation!=3)return 31;
    auto& terminal=stores[9];auto tq=terminal.quiesce_close(1);
    if(!tq.accepted||!tq.release_claim)return 32;
    auto td=terminal.confirm_destroyed(tq.token);terminal.request_stop();
    if(terminal.reopen(td)||terminal.quiesce_close(1).accepted)return 33;
    auto& permanent=stores[10];auto pq=permanent.quiesce_close(1);
    auto pd=permanent.confirm_destroyed(pq.token);
    if(permanent.close()||permanent.reopen(pd)||permanent.quiesce_close(1).accepted)return 34;
    auto& closed=stores[11];if(!closed.close()||closed.quiesce_close(1).accepted||closed.reopen({&closed,1}))return 35;
    if(recover.confirm_destroyed(tq.token).owner||recover.reopen(td))return 36;
    printf("Submitted lifetime: bounded pool, concurrent retirement, permanent stop and generation-bound recovery passed\n");
    return 0;
}
