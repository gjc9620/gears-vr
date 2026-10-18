#include "owned_attachment_observer.h"
#include <cassert>
int main(){
    using namespace gears_owned_attachments;
    gears_viewport::Target target{100,5120,2800,26,0};
    auto read=[](unsigned i,uintptr_t& out){out=32+uintptr_t(i)*8;return i!=2;};
    auto lookup=[&](uintptr_t handle){return handle==32||handle==40?target:gears_viewport::Target{};};
    auto a=capture(2,true,8,false,read,lookup,true,true,88);
    assert(a.complete()&&a.read_mask==3&&a.known_mask==3&&a.handles[1]==40&&a.depth_handle==88);
    Tracker<2> tracker;tracker.bind(1,a);const auto first=tracker.binding(1);
    target.width=3840;assert(tracker.binding(1).targets[0].width==5120); // bind-time value
    tracker.bind(1,a);assert(same(first,tracker.binding(1))&&first.serial<tracker.binding(1).serial);
    auto none=capture(0,false,0,false,read,lookup,false,false,0);
    assert(none.complete()&&!none.depth_present&&!same(none,Snapshot{}));
    tracker.bind(1,none);assert(tracker.binding(1).count==0&&tracker.binding(1).observed);
    tracker.retire(1);assert(!tracker.binding(1).observed); // Reset and submission retirement
    auto unread=capture(3,false,0,false,read,lookup,true,false,99);
    assert(!unread.complete()&&unread.read_mask==3&&unread.depth_present&&!unread.depth_read&&!unread.depth_handle);
    auto unknown=capture(3,true,8,false,read,lookup,false,false,0);
    assert(!unknown.complete()&&unknown.read_mask==7&&unknown.known_mask==3);
    auto bad_stride=capture(2,true,0,false,read,lookup,false,false,0);
    assert(bad_stride.read_mask==1&&!bad_stride.complete());
    auto unread_first=capture(2,true,8,false,[](unsigned,uintptr_t&){return false;},lookup,false,false,0);
    assert(!unread_first.read_mask&&!unread_first.complete());
    auto overflow_handle=capture(2,true,8,false,[](unsigned,uintptr_t& out){out=UINTPTR_MAX-4;return true;},lookup,false,false,0);
    assert(overflow_handle.read_mask==1&&!overflow_handle.complete());
    auto bad_count=capture(9,true,8,false,read,lookup,false,false,0);assert(!bad_count.count_valid&&!bad_count.complete());
    auto lost=capture(2,true,8,true,read,lookup,false,false,0);assert(!lost.complete());
    tracker.bind(1,a);tracker.bind(2,a);tracker.bind(3,a);assert(tracker.overflow()==1&&!tracker.binding(3).observed);
    tracker.retire(1);tracker.bind(3,a);assert(tracker.binding(3).observed);
    tracker.clear();assert(!tracker.binding(2).observed&&!tracker.binding(3).observed);
}
