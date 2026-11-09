#include "weapon_hammerburst_burst.h"
#ifdef NDEBUG
#error Assertions required
#endif
#include <cassert>
#include <cstdio>
namespace hb=gears_hammerburst_burst;
hb::Source source(){
    hb::Source s{};s.cohort.known=true;
    s.cohort.key={100,101,102,103,1000,104,105,106};
    for(unsigned i=0;i<s.cohort.actors.size();++i)
        s.cohort.actors[i]={0x1000+0x100*i,0x2000,0x3000,10+i,20+i,0,1};
    s.cohort.hold=110;s.cohort.reference=111;s.cohort.session=112;
    s.cohort.space=113;s.cohort.input=114;s.cohort.profile=115;
    s.cohort.presses=12;s.cohort.releases=11;s.frame=1000;
    s.now=1000000000;s.sync=s.now-20000000;s.stereo=s.now-30000000;
    s.render=s.now-40000000;s.sampled=s.now-1000000;
    s.trigger_down=true;s.authenticated=true;return s;
}
hb::Completion armed(){hb::Completion b;assert(b.commit(source(),100,1,6,true));return b;}
int main(){
    unsigned checks{};
    // Full original burst, with real R2-up after ANY of its first five shots.
    for(unsigned release_after=1;release_after<=6;++release_after){
        auto b=armed();auto s=source();
        for(unsigned before=1;before<6;++before){
            if(before>=release_after){s.trigger_down=false;s.cohort.releases=12;}
            hb::Ticket t{};assert(b.enter(s,before,s.now-10000000,before,6,t));
            assert(b.permits(t,s));assert(!b.commit(s,200,1,6,true));
            auto foreign=t;++foreign.serial;assert(!b.leave(foreign,100+before,before+1,true,true));
            assert(b.permits(t,s));
            assert(b.leave(t,100+before,before+1,true,true));
            assert(!b.permits(t,s));assert(!b.leave(t,100+before,before+1,true,true));
            ++s.frame;++s.cohort.key.frame;checks+=8;
        }
        assert(b.state()==hb::State::idle);assert(!b.commit(source(),100,1,6,true));
        hb::Ticket seventh{};assert(!b.enter(s,7,s.now,6,6,seventh));checks+=3;
    }
    // No raw edge, state entry, stale pose or wrong original burst can arm.
    for(unsigned failure=0;failure<10;++failure){
        hb::Completion b;auto s=source();unsigned count=1,limit=6;bool owner=true;
        switch(failure){
        case 0:owner=false;break;case 1:s.trigger_down=false;break;
        case 2:s.authenticated=false;break;case 3:s.cohort.known=false;break;
        case 4:s.sync=s.now-100000001;break;case 5:s.render=s.now+1;break;
        case 6:count=0;break;case 7:count=2;break;case 8:limit=7;break;case 9:s.frame=0;break;
        }
        assert(!b.commit(s,100,count,limit,owner));assert(b.state()==hb::State::idle);checks+=2;
    }
    // Every exact ancestry field, press and release counter is a hard boundary.
    for(unsigned failure=0;failure<48;++failure){
        auto b=armed();auto s=source();bool lifecycle=true;
        if(failure<28){auto& id=s.cohort.actors[failure/7];switch(failure%7){
            case 0:++id.address;break;case 1:++id.outer;break;case 2:++id.klass;break;
            case 3:++id.registry_index;break;case 4:++id.name_index;break;
            case 5:++id.name_number;break;case 6:id.known=0;break;
        }}else switch(failure){
        case 28:++s.cohort.key.game_start;break;case 29:++s.cohort.key.native_started_qpc;break;
        case 30:++s.cohort.key.registration_cookie;break;case 31:++s.cohort.key.lifecycle;break;
        case 32:++s.cohort.key.world_generation;break;case 33:++s.cohort.key.game_pid;break;
        case 34:++s.cohort.key.thread_id;break;case 35:++s.cohort.hold;break;
        case 36:++s.cohort.reference;break;case 37:++s.cohort.session;break;
        case 38:++s.cohort.space;break;case 39:++s.cohort.input;break;case 40:++s.cohort.profile;break;
        case 41:++s.cohort.presses;break;case 42:++s.cohort.releases;break;
        case 43:s.trigger_down=false;break;
        case 44:s.trigger_down=false;s.cohort.releases+=2;break;
        case 45:s.authenticated=false;break;case 46:s.cohort.known=false;break;
        case 47:lifecycle=false;break;
        }
        assert(!b.observe(s,lifecycle));assert(b.needs_cancel());
        assert(!b.commit(source(),200,1,6,true));
        assert(!b.acknowledge_cancel(false,true));assert(!b.acknowledge_cancel(true,false));
        assert(b.acknowledge_cancel(true,true));assert(!b.commit(source(),100,1,6,true));
        assert(b.commit(source(),200,1,6,true));checks+=8;
    }
    // UP is one way. Re-press/new edge cannot revive an old burst ticket.
    {
        auto b=armed();auto s=source();s.trigger_down=false;++s.cohort.releases;
        assert(b.observe(s,true));s=source();assert(!b.observe(s,true));assert(b.needs_cancel());checks+=3;
    }
    // Freshness tests exercise each real clock independently, including the
    // existing 20ms reserve. A stale source never consumes/renews the event.
    for(unsigned clock=0;clock<4;++clock){
        auto b=armed();auto s=source();
        auto* stamp=clock==0?&s.sync:clock==1?&s.stereo:clock==2?&s.render:&s.sampled;
        *stamp=s.now-80000000;hb::Ticket t{};
        assert(b.enter(s,1,s.now-250000000,1,6,t));assert(b.permits(t,s));
        *stamp=s.now-100000000;assert(b.permits(t,s));
        --*stamp;assert(!b.permits(t,s));b.revoke();assert(b.needs_cancel());checks+=5;
        b=armed();s=source();stamp=clock==0?&s.sync:clock==1?&s.stereo:clock==2?&s.render:&s.sampled;
        *stamp=s.now-80000001;assert(!b.enter(s,1,s.now-200000000,1,6,t));
        assert(b.state()==hb::State::committed);
        s=source();s.now+=60000000;s.sync=s.stereo=s.render=s.sampled=s.now;
        assert(!b.enter(s,1,800000000,1,6,t));assert(b.needs_cancel());checks+=4;
    }
    // Partial, ambiguous or missing execution can never replay ammunition.
    for(unsigned failure=0;failure<5;++failure){
        auto b=armed();auto s=source();hb::Ticket t{};assert(b.enter(s,1,s.now,1,6,t));
        assert(!b.leave(t,failure==0?100:101,failure==1?1:failure==2?3:2,failure!=3,failure!=4));
        assert(b.needs_cancel());assert(!b.enter(s,2,s.now,1,6,t));checks+=4;
    }
    for(unsigned failure=0;failure<4;++failure){
        auto b=armed();auto s=source();hb::Ticket t{};
        assert(!b.enter(s,1,failure==0?s.now+1:failure==1?0:failure==2?s.now-250000001:s.now,
            failure==3?2:1,6,t));assert(b.needs_cancel());checks+=2;
    }
    // A completed event ID and its ticket cannot be recycled in the next shot.
    {auto b=armed();auto s=source();hb::Ticket t{};
        assert(b.enter(s,1,s.now,1,6,t));assert(b.leave(t,101,2,true,true));
        assert(!b.enter(s,1,s.now,2,6,t));assert(b.enter(s,2,s.now,2,6,t));checks+=4;}
    // Child firing roles can share a ticket only within its actual Native
    // Frame. No altered ticket can close it or become a new invocation.
    for(unsigned mutation=0;mutation<4;++mutation){
        auto b=armed();auto s=source();hb::Ticket t{};assert(b.enter(s,1,s.now,1,6,t));
        auto other=t;
        switch(mutation){case 0:++other.serial;break;case 1:++other.event;break;
            case 2:++other.frame;break;case 3:++other.before;break;}
        assert(!b.permits(other,s));assert(!b.leave(other,101,2,true,true));
        assert(b.permits(t,s));auto next=s;++next.frame;++next.cohort.key.frame;
        assert(!b.permits(t,next));assert(b.leave(t,101,2,true,true));checks+=6;
    }
    {auto b=armed();auto s=source();hb::Ticket t{};++s.frame;
        assert(!b.enter(s,1,s.now,1,6,t));assert(b.state()==hb::State::committed);checks+=2;}
    // Counter wrap or a second release never looks like the one real UP.
    {auto s=source();s.cohort.releases=0xffffffffu;hb::Completion b;
        assert(b.commit(s,100,1,6,true));s.trigger_down=false;s.cohort.releases=0;
        assert(!b.observe(s,true));assert(b.needs_cancel());checks+=3;}
    // A revoked in-flight invocation cannot be completed by its old callback.
    {auto b=armed();auto s=source();hb::Ticket t{};assert(b.enter(s,1,s.now,1,6,t));
        b.revoke();assert(!b.permits(t,s));assert(!b.leave(t,101,2,true,true));
        assert(b.needs_cancel());checks+=4;}
    for(unsigned count=1;count<=6;++count){
        auto b=armed();auto s=source();
        for(unsigned i=1;i<count;++i){hb::Ticket t{};assert(b.enter(s,i,s.now,i,6,t));assert(b.leave(t,100+i,i+1,true,true));}
        assert(!b.original_ended(count,false,true,true));assert(!b.original_ended(count,true,false,true));
        assert(!b.original_ended(count+1,true,true,true));
        if(count<6)assert(!b.original_ended(count,true,true,false));
        assert(b.original_ended(count,true,true,count<6));assert(b.state()==hb::State::idle);
        assert(!b.commit(s,100,1,6,true));assert(b.commit(s,200,1,6,true));checks+=count<6?8:7;
    }
    {auto b=armed();auto s=source();hb::Ticket t{};assert(b.enter(s,1,s.now,1,6,t));
        assert(!b.leave_original_end(t,1,false,true,true));assert(!b.leave_original_end(t,1,true,false,true));
        assert(!b.leave_original_end(t,1,true,true,false));assert(!b.leave_original_end(t,2,true,true,true));
        auto other=t;++other.serial;assert(!b.leave_original_end(other,1,true,true,true));
        assert(b.leave_original_end(t,1,true,true,true));assert(b.state()==hb::State::idle);
        assert(!b.leave(t,101,2,true,true));assert(!b.permits(t,s));checks+=10;}
    std::printf("HB finite original-burst ledger: %u CPU boundary checks passed; no runtime adapter enabled\n",checks);
}
