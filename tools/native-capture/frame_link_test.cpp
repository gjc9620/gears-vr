#include "frame_link.h"
#include "swapchain_binding_observer.h"
#include "gpu_viewport_stats.h"
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <memory>

using namespace gears_frame_link;
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
int main() {
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
        for(uintptr_t i=1;i<=256;++i)CHECK(targets->reset(i));
        CHECK(!targets->reset(257));CHECK(!targets->bind(257,1,10));
        auto list=links->reset_success(1);pair(*links,list);
        // Production checks the boolean capacity failure even though a lookup
        // would otherwise resemble explicitly unbound render targets.
        if(!targets->bind(257,1,10))links->invalidate();
        CHECK(!targets->binding(257).targets);CHECK(!links->execute(queue,&list,1));
        CHECK(!links->present(buffer,queue).linked);
        CHECK(targets->clear_bindings());CHECK(!targets->clear_bindings());
        CHECK(targets->reset(257));
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
    std::printf("frame_link: %u checks passed\n", checks);
}
