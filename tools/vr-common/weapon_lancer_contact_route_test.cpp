#include "weapon_lancer_contact_route.h"
#include <cassert>
#include <cstdio>

namespace h=gears_weapon_hand;
using P=gears_weapon::LancerContact;
using Route=gears_weapon::LancerContactRoute;
h::Frame frame(){
    h::Frame f{};f.key={1,2,3,4,5,6,7,8};f.press_token=9;
    f.left_tracked=f.left_grip_known=f.left_grip_down=1;
    f.weapon={0x1000,0x2000,0x3000,10,11,0,1};
    f.controllers.sequence=12;f.controllers.sync_ns=130000000;
    f.controllers.input_epoch=14;f.stereo.session=15;f.stereo.space_epoch=16;
    f.reference_generation=17;return f;
}
h::Decision grant(h::Frame const& f){
    h::Decision d{};d.key=f.key;d.press_token=f.press_token;d.new_grant=1;
    d.release_required=0;d.status=h::DecisionStatus::granted;
    d.lease={h::LeaseKind::magazine,0,f.key.lifecycle,f.press_token};return d;
}
int main(){
    //52580: an auto-fitted C with a temporarily unavailable proposal must not
    // acquire the overlapping magazine on the same held Grip. Exercise the
    // production selector and exact proposal/grant dispatch together.
    {
        Route route;auto f=frame();auto d=grant(f);
        auto deferred=gears_weapon::resolve_lancer_starter_proposal(unsigned(P::magazine),true,false,.08,.12);
        assert(deferred.observation_pending&&deferred.candidates==0);
        assert(route.propose(f,deferred.candidates)==P::none);
        assert(!route.grants(f,d,P::magazine)&&!route.grants(f,d,P::starter));
        ++f.key.frame;++f.controllers.sequence;++f.controllers.sync_ns;
        auto current=gears_weapon::resolve_lancer_starter_proposal(unsigned(P::magazine),true,true,.08,.12);
        assert(!current.observation_pending&&current.candidates==unsigned(P::starter));
        assert(route.propose(f,current.candidates)==P::starter);
        d=grant(f);assert(route.grants(f,d,P::starter)&&!route.grants(f,d,P::magazine));
        auto ordinary=gears_weapon::resolve_lancer_starter_proposal(unsigned(P::magazine),false,true,.08,.12);
        assert(ordinary.candidates==unsigned(P::magazine)&&!ordinary.observation_pending);
        ordinary=gears_weapon::resolve_lancer_starter_proposal(unsigned(P::magazine),false,false,.08,.12);
        assert(ordinary.candidates==unsigned(P::magazine)&&!ordinary.observation_pending);
        auto ambiguous=gears_weapon::resolve_lancer_starter_proposal(unsigned(P::charging),true,true,.08,.12);
        ++f.key.frame;++f.controllers.sequence;
        assert(route.propose(f,ambiguous.candidates)==P::conflict);
    }
    unsigned handoff_cases{};
    for(auto purpose:{P::magazine,P::charging,P::starter}){
        Route route;auto f=frame();auto d=grant(f);
        assert(!route.preserves_granted_proposal(f,d.lease,purpose));
        assert(!route.remember_grant(f,d,purpose));
        assert(route.propose(f,unsigned(purpose))==purpose);
        assert(!route.preserves_granted_proposal(f,d.lease,purpose));
        auto rejected=d;rejected.release_required=1;
        assert(!route.remember_grant(f,rejected,purpose));
        assert(route.remember_grant(f,d,purpose));
        assert(route.preserves_granted_proposal(f,d.lease,purpose));++handoff_cases;
        for(unsigned change=0;change<12;++change){
            auto changed=f;auto held=d.lease;
            switch(change){
            case 0:++changed.key.frame;break;
            case 1:++changed.key.lifecycle;break;
            case 2:++changed.weapon.registry_index;break;
            case 3:++changed.controllers.sequence;break;
            case 4:++changed.controllers.sync_ns;break;
            case 5:++changed.controllers.input_epoch;break;
            case 6:++changed.stereo.space_epoch;break;
            case 7:++changed.reference_generation;break;
            case 8:++changed.press_token;break;
            case 9:changed.left_grip_down=0;break;
            case 10:changed.left_tracked=0;break;
            case 11:++held.id;break;
            }
            assert(!route.preserves_granted_proposal(changed,held,purpose));++handoff_cases;
        }
        // A later real observation is still a later observation, not a replay.
        auto newer=f;++newer.key.frame;
        assert(route.propose(newer,0)==P::none);
        assert(!route.preserves_granted_proposal(f,d.lease,purpose));
        route.reset();assert(!route.preserves_granted_proposal(f,d.lease,purpose));
    }
    for(auto purpose:{P::magazine,P::charging,P::starter}){
        Route route;auto f=frame();auto d=grant(f);
        assert(!route.grants(f,d,purpose));
        assert(route.propose(f,unsigned(purpose))==purpose);
        assert(route.grants(f,d,purpose));
        for(auto other:{P::magazine,P::charging,P::starter,P::none,P::conflict})
            assert(route.grants(f,d,other)==(purpose==other));
        // Re-observing the same immutable proposal never creates a new purpose.
        assert(route.propose(f,unsigned(purpose))==purpose);
        auto retained=d;retained.new_grant=0;retained.status=h::DecisionStatus::retained;
        assert(!route.grants(f,retained,purpose));
        auto bad=d;bad.press_token++;assert(!route.grants(f,bad,purpose));
        bad=d;bad.lease.id++;assert(!route.grants(f,bad,purpose));
        bad=d;bad.key.registration_cookie++;assert(!route.grants(f,bad,purpose));
        bad=d;bad.release_required=1;assert(!route.grants(f,bad,purpose));
        auto newer=f;++newer.key.frame;
        assert(!route.grants(newer,grant(newer),purpose));
        assert(route.propose(newer,unsigned(purpose))==purpose);
        assert(route.propose(f,unsigned(purpose))==P::conflict);
        assert(!route.grants(f,d,purpose));
        assert(route.grants(newer,grant(newer),purpose));
        // Same-frame geometry/contact disagreement requires a release/retry.
        assert(route.propose(newer,0)==P::conflict);
        assert(route.propose(newer,unsigned(purpose))==P::conflict);
        assert(!route.grants(newer,grant(newer),purpose));
    }
    for(unsigned mask:{3u,5u,6u,7u,8u,9u,~0u}){
        Route route;auto f=frame();assert(route.propose(f,mask)==P::conflict);
        for(auto p:{P::magazine,P::charging,P::starter})assert(!route.grants(f,grant(f),p));
    }
    for(unsigned change=0;change<9;++change){
        Route route;auto f=frame();assert(route.propose(f,1)==P::magazine);
        auto other=f;
        switch(change){
        case 0:other.weapon.registry_index++;break;
        case 1:other.controllers.sequence++;break;
        case 2:other.controllers.sync_ns++;break;
        case 3:other.controllers.input_epoch++;break;
        case 4:other.stereo.session++;break;
        case 5:other.stereo.space_epoch++;break;
        case 6:other.reference_generation++;break;
        case 7:other.press_token++;break;
        case 8:other.left_grip_down=0;break;
        }
        assert(!route.grants(other,grant(other),P::magazine));
        assert(route.propose(other,1)==P::conflict);
        assert(!route.grants(f,grant(f),P::magazine));
    }
    std::puts("Lancer contact purpose routing: exclusive grant, replay, overlap and identity checks passed");
    std::printf("Committed-proposal observation handoff: %u cases; exact consumed source only, no new grant\n",handoff_cases);
}
