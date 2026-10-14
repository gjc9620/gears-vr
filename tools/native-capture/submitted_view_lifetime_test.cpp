#include "submitted_view_lifetime.h"
#include <atomic>
#include <thread>
#include <cstdio>
int main() {
    gears_submitted::Lifetime life;
    if(life.begin(0) || !life.begin(1) || life.begin(1) || !life.begin(2))return 1;
    if(life.close() || life.begin(3) || life.retire(99) || life.retire(1))return 2;
    if(!life.retire(2) || life.close() || life.retire(2) || life.pending())return 3;
    gears_submitted::Lifetime pool;
    for(unsigned i=1;i<=64;++i)if(!pool.begin(i))return 4;
    if(pool.begin(65) || pool.retire(1) || !pool.begin(65))return 5;
    if(pool.close())return 6;
    std::atomic<unsigned> claims{};
    auto retire=[&](unsigned first,unsigned last){for(unsigned i=first;i<=last;++i)if(pool.retire(i))++claims;};
    std::thread a(retire,2,33),b(retire,34,65);a.join();b.join();
    if(claims!=1 || pool.pending() || pool.close())return 7;
    gears_submitted::Lifetime reused;
    gears_projection::Metadata metadata{};metadata.enabled=true;metadata.phase=gears_projection::Phase::b;metadata.pair_qpc=111;
    gears_projection::Metadata read{};
    if(!reused.begin(42,metadata) || !reused.projection(42,read) || read.pair_qpc!=111 || read.phase!=metadata.phase)return 9;
    if(reused.retire(42) || reused.projection(42,read) || read.enabled)return 10;
    metadata.phase=gears_projection::Phase::c;metadata.pair_qpc=222;
    if(!reused.begin(42,metadata) || !reused.projection(42,read) || read.pair_qpc!=222 || read.phase!=metadata.phase || reused.retire(42))return 11;
    if(!reused.begin(42) || reused.retire(42) || !reused.begin(42) || reused.close() || !reused.retire(42))return 8;
    gears_submitted::Lifetime frames;gears_submitted::FrameTicket ticket{},copied{};
    ticket.enabled=true;ticket.id=1;ticket.sample.session=7;ticket.sample.sequence=111;ticket.expected.view[1][12]=3.2f;
    if(!frames.begin(42,{},ticket))return 12;
    ticket.sample.sequence=222;ticket.expected.view[1][12]=9;
    if(!frames.frame(42,copied) || copied.sample.sequence!=111 || copied.expected.view[1][12]!=3.2f)return 13;
    if(copied.id!=1)return 17;
    copied.sample.sequence=999;if(!frames.frame(42,copied) || copied.sample.sequence!=111)return 14;
    if(frames.retire(42) || frames.frame(42,copied) || copied.enabled)return 15;
    if(!frames.begin(42,{},ticket) || !frames.frame(42,copied) || copied.sample.sequence!=222 ||
       copied.expected.view[1][12]!=9 || frames.close() || !frames.retire(42) || frames.frame(42,copied))return 16;
    gears_submitted::Lifetime same_pose;ticket.id=10;
    if(!same_pose.begin(1,{},ticket))return 18;
    ticket.id=11;if(!same_pose.begin(2,{},ticket) || !same_pose.frame(1,copied) || copied.id!=10 ||
       copied.sample.sequence!=ticket.sample.sequence || !same_pose.frame(2,copied) || copied.id!=11)return 19;
    gears_submitted::Lifetime idle_stop;idle_stop.request_stop();
    if(!idle_stop.stopping()||idle_stop.is_closed()||idle_stop.begin(1)||!idle_stop.close()||idle_stop.close())return 20;
    gears_submitted::Lifetime pending_stop;
    if(!pending_stop.begin(1)||!pending_stop.begin(2))return 21;
    pending_stop.request_stop();pending_stop.request_stop();
    if(pending_stop.is_closed()||pending_stop.begin(3)||pending_stop.retire(1)||pending_stop.close()||!pending_stop.retire(2)||pending_stop.close())return 22;
    // Admission remains closed after request_stop returns, even under producers.
    gears_submitted::Lifetime racing;
    std::atomic<bool> stopping_done{},bad_admission{};
    auto produce=[&](unsigned root){for(unsigned i=0;i<10000;++i){
        bool after=stopping_done.load();bool accepted=racing.begin(root);
        if(after&&accepted)bad_admission=true;
        if(accepted&&racing.retire(root))bad_admission=true;
    }};
    std::thread c(produce,1),d(produce,2);
    racing.request_stop();stopping_done=true;c.join();d.join();
    if(bad_admission||racing.pending()||racing.is_closed()||!racing.close()||racing.close())return 23;
    printf("Submitted lifetime: bounded pool, concurrent retire, stop admission barrier and native-only release claim passed\n");
    return 0;
}
