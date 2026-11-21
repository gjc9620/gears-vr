#include "authored_grasp_owner_policy.h"
#include <cassert>
#include <cstdio>
int main(){
    namespace p=gears_authored_grasp_owner;namespace a=gears_hand_authored_grasp;
    a::Request q{};q.source.key.world_generation=12;
    q.source.world.address=16;q.source.pawn.address=32;q.source.mesh.address=48;q.source.weapon.address=64;
    p::Actor actor{q.source.world,q.source.pawn,q.source.mesh,q.source.weapon,12,p::Family::lancer,true,true};
    q.mode=a::Mode::left_part;
    for(auto kind:{a::object::Kind::magazine,a::object::Kind::support,a::object::Kind::loading,a::object::Kind::starter}){
        q.kind=kind;const auto expected=kind==a::object::Kind::loading?p::Need::lancer_loading:
            kind==a::object::Kind::starter?p::Need::lancer_starter:p::Need::none;
        assert(p::required(q,actor)==expected);
        auto unknown=actor;unknown.family=p::Family::unknown;assert(p::required(q,unknown)==p::Need::reject);
        auto other=actor;other.family=p::Family::other;assert(p::required(q,other)==p::Need::none);
        auto wrong=actor;++wrong.weapon.name_number;assert(p::required(q,wrong)==p::Need::reject);
        wrong=actor;++wrong.world_generation;assert(p::required(q,wrong)==p::Need::reject);
        wrong=actor;wrong.context_valid=false;assert(p::required(q,wrong)==p::Need::reject);
        wrong=actor;wrong.weapon_known=false;assert(p::required(q,wrong)==p::Need::reject);
        wrong=actor;wrong.mesh.address+=8;assert(p::required(q,wrong)==p::Need::reject);
        wrong=actor;wrong.pawn.klass+=8;assert(p::required(q,wrong)==p::Need::reject);
        wrong=actor;wrong.world.registry_index++;assert(p::required(q,wrong)==p::Need::reject);
    }
    q.mode=a::Mode::right_main_fingers;q.kind=a::object::Kind::none;
    assert(p::required(q,actor)==p::Need::none);
    actor.family=p::Family::unknown;assert(p::required(q,actor)==p::Need::reject);
    actor.family=static_cast<p::Family>(99);assert(p::required(q,actor)==p::Need::reject);
    actor.family=p::Family::other;actor.weapon.address=0;assert(p::required(q,actor)==p::Need::reject);
    std::puts("Authored owner policy44 checks: full source/class association and original Lancer C/E needs; value-only, no runtime proof");
}
