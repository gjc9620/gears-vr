#include "hand_stream_session.h"
#include <cassert>
#include <cstdio>

int main(){
    using namespace gears_hand_session;
    Session session;
    assert(!session.running(100,false,false));
    session.begin(100);
    assert(session.running(100,false,false));
    for(unsigned i=0;i<recording_leases;++i)assert(session.record_lease(100+i));
    assert(!session.record_lease(500));
    assert(session.running(15101,false,false));
    assert(session.running(100000000,false,false));
    assert(!session.running(100000000,true,false));
    assert(!session.running(100000000,false,true));
    assert(!session.running(99,false,false));
    session.end();
    assert(!session.running(100000001,false,false));

    Session window;
    window.begin(100);
    assert(window.recording(15100));
    assert(!window.recording(15101));
    assert(!window.recording(15100)); // A backward clock cannot reopen detail.
    assert(window.running(15101,false,false));
    Session events;
    events.begin(100);
    for(unsigned i=0;i<event_limit;++i)assert(events.diagnostic_event(101));
    assert(!events.diagnostic_event(101));
    assert(events.record_lease(102)); // Event exhaustion cannot stop a writer.
    assert(events.diagnostic_events()==event_limit);

    std::uint64_t until{};
    assert(write_until(20000,20003,20005,until)&&until==20100);
    assert(write_until(20005,20000,20003,until)&&until==20100);
    assert(write_until(20005,20003,20000,until)&&until==20100);
    constexpr auto maximum=std::numeric_limits<std::uint64_t>::max();
    assert(write_until(maximum-100,maximum-100,maximum-100,until)&&until==maximum);
    assert(!write_until(maximum-99,20,20,until)&&until==0);
    assert(!write_until(20,maximum-99,20,until)&&until==0);
    assert(!write_until(20,20,maximum-99,until)&&until==0);
    unsigned count=std::numeric_limits<unsigned>::max()-1;
    assert(saturating_add(count)&&count==std::numeric_limits<unsigned>::max());
    assert(!saturating_add(count)&&count==std::numeric_limits<unsigned>::max());
    std::uint64_t sum=maximum-3;
    assert(!saturating_add(sum,std::uint64_t{4})&&sum==maximum);
    assert(status_leases(65535)==65535&&status_leases(65536)==65535);
    assert(status_leases(std::numeric_limits<unsigned>::max())==65535);
    std::puts("Continuous hand session: lifetime, bounded detail, checked Plan cutoff, saturation and status passed");
}
