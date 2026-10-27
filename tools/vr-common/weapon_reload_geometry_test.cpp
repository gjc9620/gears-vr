#include "weapon_reload_geometry.h"
#include <cassert>
#include <cstdio>
using namespace gears_vr;
using namespace gears_weapon;

int main(){
    FeedPortMount port{};
    assert(port.valid());
    FeedInsertionStroke stroke;
    assert(stroke.arm(10,11,12,13));
    FeedSample sample{};sample.weapon=10;sample.item=11;sample.world=12;
    sample.space_epoch=13;sample.tracked=sample.item_held=true;
    sample.sequence=1;sample.monotonic_ns=1000000000;
    sample.left_grip.position={0,0,.07};
    assert(stroke.observe(sample,port)==FeedResult::approaching);
    sample.sequence++;sample.monotonic_ns+=20000000;sample.left_grip.position.z=.05;
    assert(stroke.observe(sample,port)==FeedResult::approaching);
    sample.sequence++;sample.monotonic_ns+=20000000;sample.left_grip.position.z=.002;
    assert(stroke.observe(sample,port)==FeedResult::inserted);
    assert(stroke.observe(sample,port)==FeedResult::rejected); // one item, one commit
    assert(stroke.arm(10,11,12,13));
    sample.sequence++;sample.monotonic_ns+=20000000;sample.left_grip.position.z=.002;
    assert(stroke.observe(sample,port)==FeedResult::rejected); // touching port alone
    sample.sequence++;sample.monotonic_ns+=20000000;sample.left_grip.position.z=.06;
    assert(stroke.observe(sample,port)==FeedResult::approaching);
    sample.sequence++;sample.monotonic_ns+=1000000;sample.left_grip.position.z=.002;
    assert(stroke.observe(sample,port)==FeedResult::rejected); // teleport
    assert(stroke.arm(10,11,12,13));
    sample.sequence++;sample.monotonic_ns+=20000000;sample.left_grip.position={.08,0,.07};
    assert(stroke.observe(sample,port)==FeedResult::rejected); // outside feed port
    sample.space_epoch=14;sample.sequence++;sample.monotonic_ns+=20000000;
    assert(stroke.observe(sample,port)==FeedResult::rejected); // recenter invalidates
    assert(stroke.arm(10,11,12,14));
    sample.left_grip.position={0,0,.07};
    sample.left_grip.orientation={0,1,0,0}; // backwards item
    sample.sequence++;sample.monotonic_ns+=20000000;
    assert(stroke.observe(sample,port)==FeedResult::rejected);
    StereoSample stereo{};stereo.session=21;stereo.sequence=7;
    stereo.space_epoch=8;stereo.predicted_time=1000;stereo.focused=1;
    ControllerInputFrame hands{};hands.session=21;hands.sequence=7;
    hands.space_epoch=8;hands.predicted_time=1000;hands.focused=1;
    hands.sync_ns=999999900;hands.received_ns=1000000000;
    hands.hands[0].grip_active=1;hands.hands[0].grip_flags=15;
    WeaponPoseSolution gun{};gun.session=21;gun.sequence=7;
    gun.space_epoch=8;gun.predicted_time=1000;
    FeedSample sourced{};
    assert(feed_sample_from_hands(hands,stereo,gun,10,11,12,1000000100,true,sourced));
    assert(sourced.sequence==7&&sourced.space_epoch==8);
    stereo.sequence=8;
    assert(!feed_sample_from_hands(hands,stereo,gun,10,11,12,1000000100,true,sourced));
    ReloadModel reload;
    ReloadPolicy policy{LoadUnit::round,8,3,true,true,100,
        MechanismAction::lever_down_return,MechanismAction::lever_down_return};
    assert(reload.begin(10,policy,7,1,1000)==ReloadEvent::started);
    assert(reload_guide(reload)==GuidePart::feed_port);
    assert(reload.grab(10)==ReloadEvent::grabbed);
    assert(reload_guide(reload)==GuidePart::feed_port);
    assert(reload.insert(10,1010)==ReloadEvent::mechanism_needed);
    assert(reload_guide(reload)==GuidePart::lever_handle);
    assert(reload.cycle(10,1020,MechanismAction::lever_down_return)==ReloadEvent::perfect);
    assert(reload_guide(reload)==GuidePart::none);
    std::puts("physical feed stroke and per-step yellow guide policy passed");
}
