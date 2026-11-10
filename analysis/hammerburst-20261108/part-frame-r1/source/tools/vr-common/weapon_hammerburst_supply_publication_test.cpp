#include "weapon_hammerburst_supply_publication.h"
#include "weapon_hand_client.h"
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Assertions required
#endif
namespace hand=gears_weapon_hand;
using gears_hammerburst_supply_publication::Snapshot;
hand::Frame frame(){
    hand::Frame f{};f.key={11,22,33,1,710,1,44,55};f.controllers.sequence=5166;
    f.controllers.sync_ns=1000000000;f.controllers.input_epoch=5;f.controllers.profile_generation=6;
    f.reference_generation=7;f.press_token=2;f.left_grip_known=f.left_grip_down=f.left_tracked=1;return f;
}
hand::Decision decision(hand::Frame const& f){
    hand::Decision d{};d.key=f.key;d.proposal_nonce=3;d.press_token=f.press_token;
    d.lease={};d.lease.kind=hand::LeaseKind::ammo;d.lease.id=f.press_token;d.lease.lifecycle=f.key.lifecycle;
    d.status=hand::DecisionStatus::granted;d.new_grant=1;d.release_required=0;return d;
}
int main(){
    unsigned checks{};auto f=frame();auto d=decision(f);Snapshot s{};
    assert(s.remember(f,d));assert(s.same_acquisition_source(f,d.lease,2));checks+=2;
    hand::GripGameState committed{};committed.request_ammo=true;committed.known=true;
    committed.main_hand_held=true;committed.kind=hand::WeaponKind::firearm;committed.mount_known=true;
    auto changed=committed;changed.request_ammo=false;changed.object_incumbents[0]=d.lease;
    // The exact actual45736 difference5 must remain a strict rejection unless
    // the HB contributor itself keeps its original acquisition observation.
    assert(hand::game_difference(changed,committed)==5);
    assert(!hand::same_pose_consumer_observation(f,d,changed,committed));checks+=2;
    for(unsigned mutation=0;mutation<14;++mutation){
        auto other=f;auto held=d.lease;std::uint64_t item=2;
        switch(mutation){
        case 0:++other.key.frame;break;case 1:++other.key.registration_cookie;break;
        case 2:++other.key.lifecycle;break;case 3:++other.key.world_generation;break;
        case 4:++other.controllers.sequence;break;case 5:++other.controllers.sync_ns;break;
        case 6:++other.controllers.input_epoch;break;case 7:++other.controllers.profile_generation;break;
        case 8:++other.reference_generation;break;case 9:++other.press_token;break;
        case 10:other.left_grip_down=0;break;case 11:other.left_tracked=0;break;
        case 12:++held.id;break;case 13:++item;break;
        }
        assert(!s.same_acquisition_source(other,held,item));++checks;
    }
    for(unsigned mutation=0;mutation<9;++mutation){
        auto bad=d;auto source=f;
        switch(mutation){
        case 0:bad.new_grant=0;break;case 1:bad.status=hand::DecisionStatus::retained;break;
        case 2:bad.release_required=1;break;case 3:bad.proposal_nonce=0;break;
        case 4:bad.lease.kind=hand::LeaseKind::magazine;break;case 5:++bad.lease.lifecycle;break;
        case 6:++bad.press_token;break;case 7:source.left_grip_known=0;break;
        case 8:source.controllers.sync_ns=0;break;
        }
        assert(!s.remember(source,bad));assert(!s.same_acquisition_source(f,d.lease,2));checks+=2;
    }
    assert(s.remember(f,d));s.reset();assert(!s.same_acquisition_source(f,d.lease,2));checks+=2;
    {auto latch=d;latch.lease.kind=hand::LeaseKind::magazine;
        assert(!s.remember(f,latch)); // Default supply cannot adopt a latch.
        assert(s.remember(f,latch,hand::LeaseKind::magazine));
        assert(s.same_acquisition_source(f,latch.lease,2));
        auto next=f;++next.key.frame;assert(!s.same_acquisition_source(next,latch.lease,2));
        assert(!s.same_acquisition_source(f,d.lease,2));
        assert(!s.remember(f,latch,hand::LeaseKind::support));
        assert(!s.same_acquisition_source(f,latch.lease,2));checks+=7;
    }
    std::printf("HB supply publication: %u exact-source/first-grant rejection checks; no grant, input renewal or game execution\n",checks);
}
