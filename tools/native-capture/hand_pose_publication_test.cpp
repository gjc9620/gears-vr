#include "hand_pose_publication.h"
#include "crosshair_assignment_test_fixture.h"
#include <cstdio>
#include <functional>

struct HandFixture {
    Fixture f;
    gears_anchor::Anchor anchor{};
    HandFixture(){
        const auto level=f.add(L"Level_0",0);
        auto& pawn=f.identities.identities[f.c.pawn];pawn.outer=f.address(level);
        f.m.put(pawn.address+f.identities.object_outer,pawn.outer);
        anchor.pawn=pawn.address;anchor.pawn_identity=pawn;anchor.controller=f.address(f.c.controller);
        anchor.level=pawn.outer;
        for(auto field:{f.c.controller_pawn,f.c.pawn_controller,f.c.pawn_weapon,f.c.weapon_instigator})
            f.property(field.property,field.offset,8,0);
    }
    void select(std::uintptr_t weapon){f.m.put(anchor.pawn+f.c.pawn_weapon.offset,weapon);}
    bool sample(std::uintptr_t& weapon){return gears_hand_pose::current_weapon(f.c,anchor,4,4,f.read(),weapon);}
};
int main(){
    {HandFixture x;std::uintptr_t weapon{};assert(x.sample(weapon)&&weapon==x.f.address(x.f.c.weapon));
        x.anchor.cinematic=true;assert(x.sample(weapon)); // A fixed camera/animation is not a pose-source gate.
        x.select(0);x.f.m.blocks.erase(x.f.address(x.f.c.weapon));
        assert(x.sample(weapon)&&weapon==0); // No obsolete weapon read is needed to prove unarmed.
        x.f.c.weapon=999;assert(x.sample(weapon)&&weapon==0);
    }
    {HandFixture x;const auto cls=x.f.add(L"NewWeaponClass",0);
        x.f.m.put(x.f.address(cls)+x.f.identities.field_super,x.f.address(x.f.c.war_weapon));
        const auto instance=x.f.add(L"NewWeapon_0",cls);
        x.f.m.put(x.f.address(instance)+x.f.c.weapon_instigator.offset,x.anchor.pawn);
        x.select(x.f.address(instance));x.f.m.blocks.erase(x.f.address(x.f.c.weapon));
        std::uintptr_t weapon{};assert(x.sample(weapon)&&weapon==x.f.address(instance));
        x.f.m.put(x.f.address(cls)+x.f.identities.field_super,x.f.address(cls));
        assert(!x.sample(weapon));
    }
    using Fault=std::function<void(HandFixture&)>;
    const Fault faults[]{
        [](auto& x){x.anchor.controller++;},[](auto& x){x.anchor.pawn++;},
        [](auto& x){x.anchor.level++;},[](auto& x){x.anchor.pawn_identity.number++;},
        [](auto& x){x.anchor.unpossessed=true;},
        [](auto& x){x.f.m.put(x.anchor.controller+x.f.c.controller_pawn.offset,std::uintptr_t(0));},
        [](auto& x){x.f.m.put(x.anchor.pawn+x.f.c.pawn_controller.offset,std::uintptr_t(0));},
        [](auto& x){x.f.m.put(x.f.address(x.f.c.weapon)+x.f.c.weapon_instigator.offset,std::uintptr_t(0));},
        [](auto& x){x.f.c.pawn_weapon.offset++;},
        [](auto& x){x.f.m.put(x.f.address(x.f.c.pawn_weapon.property)+0x74,unsigned(4));},
        [](auto& x){x.f.m.put(x.f.address(x.f.c.pawn_weapon.property)+0x70,unsigned(2));},
        [](auto& x){x.f.m.put(x.f.address(x.f.c.pawn_weapon.property)+0x94,unsigned(0));},
        [](auto& x){x.f.m.put(x.f.address(x.f.c.weapon)+x.f.identities.object_class,x.f.address(x.f.c.war_pawn));},
        [](auto& x){x.f.m.put(0x10000+x.f.c.pawn*8,std::uintptr_t(0));},
        [](auto& x){x.f.m.denied=x.anchor.pawn+x.f.c.pawn_weapon.offset;}
    };
    for(unsigned i=0;i<sizeof(faults)/sizeof(faults[0]);++i){HandFixture x;faults[i](x);std::uintptr_t weapon=0xdead;
        assert(!x.sample(weapon)&&weapon==0xdead);
        x.select(0); // A null field cannot bypass any player/property fault.
        if(i!=7&&i!=12)assert(!x.sample(weapon)&&weapon==0xdead); // Weapon-only faults disappear when unarmed.
    }
    {HandFixture x;std::uintptr_t weapon=0xdead;
        for(auto world:{std::uint64_t(0),std::uint64_t(5)})
            assert(!gears_hand_pose::current_weapon(x.f.c,x.anchor,4,world,x.f.read(),weapon)&&weapon==0xdead);
    }
    // Mid-read world/possession/weapon races must not publish a partial result.
    for(unsigned race=0;race<3;++race){HandFixture x;std::uintptr_t weapon=0xdead;unsigned reads{};
        auto read=x.f.read();auto racing=[&](void* out,const void* ptr,std::size_t size){
            const auto address=reinterpret_cast<std::uintptr_t>(ptr);
            const bool ok=read(out,ptr,size);
            if(address==x.anchor.pawn+x.f.c.pawn_weapon.offset&&++reads==2){
                if(race==0)x.select(0);
                if(race==1)x.f.m.put(x.anchor.pawn+x.f.c.pawn_controller.offset,std::uintptr_t(0));
                if(race==2)x.f.m.put(x.f.identities.objects_header,gears_ui::Registry{0x10000,2001,2001});
            }
            return ok;
        };
        assert(!gears_hand_pose::current_weapon(x.f.c,x.anchor,4,4,racing,weapon)&&weapon==0xdead);
    }
    std::puts("Read-only hand pose binding passed (equipped, unarmed, animation and rejection gates)");
}
