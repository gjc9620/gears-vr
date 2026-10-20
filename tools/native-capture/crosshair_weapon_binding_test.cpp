#include "crosshair_weapon_binding.h"
#include "crosshair_assignment_test_fixture.h"
#include <functional>

static OwnedConfig startup_config(Fixture const& f){OwnedConfig result;
    result.identities=f.identities;result.assignment=f.c;result.assignment.identities=&result.identities;return result;}
struct WeaponFixture {
    Fixture f;OwnedConfig startup{startup_config(f)};
    unsigned pistol_class{},pistol{};
    WeaponFixture(){pistol_class=f.add(L"Weap_Pistol",0);pistol=f.add(L"Pistol",pistol_class);
        f.m.put(f.address(pistol_class)+f.identities.field_super,f.address(f.c.war_weapon));
        f.m.put(f.address(pistol)+f.c.weapon_instigator.offset,f.address(f.c.pawn));
        // Dynamic actor numbers are legitimate; Class names must remain zero.
        f.m.put(f.address(pistol)+f.identities.object_name+4,unsigned(7));select(pistol);}
    void select(unsigned index){f.m.put(f.address(f.c.pawn)+f.c.pawn_weapon.offset,f.address(index));}
};
static bool unchanged(OwnedConfig const& out,OwnedConfig const& before){
    // Compare owning arrays by fields, without depending on struct padding.
    if(!(out.pid==before.pid&&out.assignment.identities==&out.identities&&
        out.identities.identity_count==before.identities.identity_count&&out.identities.super_count==before.identities.super_count&&
        out.assignment.script==before.assignment.script&&out.assignment.weapon==before.assignment.weapon))return false;
    for(unsigned i=0;i<out.identities.identities.size();++i)
        if(!gears_anchor::equal_identity(out.identities.identities[i],before.identities.identities[i]))return false;
    for(unsigned i=0;i<out.identities.supers.size();++i)
        if(out.identities.supers[i].identity!=before.identities.supers[i].identity||out.identities.supers[i].parent!=before.identities.supers[i].parent)return false;
    return true;
}
static void reject(WeaponFixture& x){OwnedConfig out=x.startup;out.pid=777;const OwnedConfig before=out;
    assert(!rebind_weapon(x.startup,x.f.read(),out)&&unchanged(out,before));}
int main(){
    // The startup certificate can outlive an equipment change. Activation uses
    // the current same-Pawn weapon and still requires the unhooked entry bytes.
    {WeaponFixture y;OwnedConfig out;ValidationFailure failure{};
        assert(!validate_bindings(y.startup.assignment,y.f.read(),true,&failure)&&failure==ValidationFailure::schema);
        assert(prepare_activation(y.startup,y.f.read(),out,&failure)&&failure==ValidationFailure::none);
        assert(out.identities.identities[out.assignment.weapon].address==y.f.address(y.pistol));
        y.f.m.put(y.f.identities.base+0x1d7ad0,std::uint8_t(0xe9));
        const OwnedConfig before=out;assert(!prepare_activation(y.startup,y.f.read(),out,&failure)&&unchanged(out,before));
    }
    {WeaponFixture y;y.f.m.blocks.erase(y.f.address(y.f.c.weapon));OwnedConfig out;
        assert(prepare_activation(y.startup,y.f.read(),out));}
    {Fixture y;auto startup=startup_config(y);OwnedConfig out;
        assert(prepare_activation(startup,y.read(),out));
        assert(out.assignment.identities==&out.identities&&out.identities.identity_count==startup.identities.identity_count);}
    WeaponFixture x;OwnedConfig output;
    assert(rebind_weapon(x.startup,x.f.read(),output));
    const auto count=x.startup.identities.identity_count;
    assert(output.assignment.weapon==x.startup.assignment.weapon&&output.assignment.pawn==x.startup.assignment.pawn);
    assert(output.identities.identities[output.assignment.weapon].address==x.f.address(x.pistol));
    assert(output.identities.identities[output.assignment.weapon].number==7);
    assert(output.identities.identity_count==count+1&&output.assignment.identities==&output.identities);
    assert(validate_bindings(output.assignment,x.f.read(),false));
    // Each switch starts from startup, without accumulating prior class imports.
    x.select(x.f.c.weapon);assert(rebind_weapon(x.startup,x.f.read(),output));
    assert(output.identities.identity_count==count&&output.identities.identities[output.assignment.weapon].address==x.f.address(x.f.c.weapon));
    x.select(x.pistol);assert(rebind_weapon(x.startup,x.f.read(),output));assert(output.identities.identity_count==count+1);
    assert(x.startup.identities.identity_count==count&&x.startup.assignment.identities==&x.startup.identities);
    // A freed/holstered startup actor is not an admission prerequisite.
    {WeaponFixture y;y.f.m.blocks.erase(y.f.address(y.f.c.weapon));OwnedConfig candidate;
        assert(rebind_weapon(y.startup,y.f.read(),candidate));}
    using Mutation=std::function<void(WeaponFixture&)>;
    const Mutation faults[]{
        [](auto& y){y.f.m.put(y.f.address(y.pistol)+y.f.c.weapon_instigator.offset,y.f.address(y.f.c.controller));},
        [](auto& y){y.f.m.put(y.f.address(y.f.c.controller)+y.f.c.controller_pawn.offset,y.f.address(y.pistol));},
        [](auto& y){y.f.m.put(y.f.address(y.f.c.pawn)+y.f.c.pawn_controller.offset,y.f.address(y.pistol));},
        [](auto& y){y.f.m.put(y.f.address(y.pistol_class)+y.f.identities.field_super,y.f.address(y.f.c.war_pawn));},
        [](auto& y){y.f.m.put(y.f.address(y.pistol_class)+y.f.identities.object_class,y.f.address(y.f.c.war_weapon));},
        [](auto& y){y.f.m.put(y.f.address(y.pistol_class)+y.f.identities.object_name+4,unsigned(1));},
        [](auto& y){y.f.m.put(y.f.address(y.pistol_class)+y.f.identities.field_super,y.f.address(y.pistol_class));},
        [](auto& y){y.f.m.put(0x10000+y.pistol*8,y.f.address(y.f.c.weapon));},
        [](auto& y){y.f.m.put(0x10000+y.pistol_class*8,y.f.address(y.f.c.war_pc));},
        [](auto& y){y.f.m.put(0xc0000+y.pistol*0x200,unsigned(999));},
        [](auto& y){std::array<wchar_t,64> name{};name.fill(L'X');y.f.m.put(0xc0000+y.pistol*0x200+8,name);},
        [](auto& y){std::array<wchar_t,64> name{};std::copy_n(L"Default__Pistol",16,name.begin());y.f.m.put(0xc0000+y.pistol*0x200+8,name);},
        [](auto& y){y.f.m.put(y.f.address(y.pistol)+y.f.identities.object_internal_index,y.f.identities.identities[y.f.c.pawn].index);},
        [](auto& y){y.f.m.put(y.f.address(y.pistol)+y.f.identities.object_class,y.f.address(y.f.c.pawn));},
        [](auto& y){y.f.m.put(y.f.address(y.f.c.pawn)+y.f.c.pawn_weapon.offset,std::uintptr_t(0));},
        [](auto& y){y.f.m.put(y.f.c.script+0x359,std::uint8_t(0));},
        [](auto& y){y.f.m.put(y.f.address(y.f.c.pawn_weapon.property)+0x94,unsigned(0x110));},
        [](auto& y){y.startup.identities.super_count=1;y.startup.identities.supers[0]={y.f.c.war_weapon,y.f.address(y.f.c.war_pawn)};},
    };
    for(auto const& mutate:faults){WeaponFixture y;mutate(y);reject(y);
        OwnedConfig out=y.startup;out.pid=777;const OwnedConfig before=out;
        assert(!prepare_activation(y.startup,y.f.read(),out)&&unchanged(out,before));}
    // Two unknown classes can be imported, but a cycle cannot be certified.
    {WeaponFixture y;auto parent=y.f.add(L"PistolBase",0);
        y.f.m.put(y.f.address(y.pistol_class)+0x60,y.f.address(parent));
        y.f.m.put(y.f.address(parent)+0x60,y.f.address(y.f.c.war_weapon));
        OwnedConfig candidate;assert(rebind_weapon(y.startup,y.f.read(),candidate));
        assert(candidate.identities.identity_count==y.startup.identities.identity_count+2);
        y.f.m.put(y.f.address(parent)+0x60,y.f.address(y.pistol_class));reject(y);}
    {WeaponFixture y;auto previous=y.pistol_class;
        for(unsigned i=0;i<39;++i){auto next=y.f.add(L"PistolAncestor",0);
            y.f.m.put(y.f.address(previous)+0x60,y.f.address(next));previous=next;}
        y.f.m.put(y.f.address(previous)+0x60,y.f.address(y.f.c.war_weapon));reject(y); // 41 classes.
    }
    // Fill the immutable certificate's identity storage. Replacing its actor
    // slot cannot manufacture room for an unknown dynamic Class.
    {WeaponFixture y;auto& ids=y.startup.identities;
        while(ids.identity_count<ids.identities.size()){
            auto& id=ids.identities[ids.identity_count++];id.address=0x200000+ids.identity_count*0x1000;
            id.index=id.name_index=100+ids.identity_count;id.cls=ids.identities[0].address;
            std::copy_n(L"StaticExtra",12,id.name.begin());
            y.f.m.block(id.address,0x200);y.f.m.put(id.address+0x10,id.index);y.f.m.put(id.address+0x18,id.name_index);
            y.f.m.put(id.address+0x20,id.cls);y.f.m.put(0x10000+id.index*8,id.address);
            const auto entry=std::uintptr_t(0x400000+id.index*0x200);
            y.f.m.put(0x18000+id.name_index*8,entry);y.f.m.block(entry,0x180);
            y.f.m.put(entry,id.name_index);y.f.m.put(entry+8,id.name);
        }
        y.select(y.f.c.weapon);assert(validate_bindings(y.startup.assignment,y.f.read(),false));
        y.select(y.pistol);reject(y);
    }
    // Invalid startup roles/layout/counts are rejected before indexed reads.
    for(unsigned fault=0;fault<6;++fault){WeaponFixture y;
        if(fault==0)y.startup.assignment.weapon=96;if(fault==1)y.startup.assignment.pawn=96;
        if(fault==2)y.startup.identities.object_name=93;if(fault==3)y.startup.identities.identity_count=97;
        if(fault==4)y.startup.identities.super_count=97;if(fault==5)y.startup.assignment.pawn_weapon.offset=65536;
        reject(y);}
    // A read-induced registry or selected-weapon change never publishes a
    // partially rebound certificate, including late changes after validation.
    for(unsigned fault=0;fault<3;++fault){WeaponFixture y;OwnedConfig out=y.startup;out.pid=777;const OwnedConfig before=out;
        auto read=y.f.read();unsigned hits=0;
        auto changing=[&](void* dest,void const* ptr,std::size_t size){const auto address=reinterpret_cast<std::uintptr_t>(ptr);
            const bool ok=read(dest,ptr,size);
            if(address==y.f.address(y.pistol)&&size==96&&++hits==2){
                if(fault==0)y.f.m.put(0x8000,gears_ui::Registry{0x10000,2000,2001});
                if(fault==1)y.select(y.f.c.weapon);
                if(fault==2)y.f.m.put(y.f.address(y.pistol_class)+0x60,y.f.address(y.f.c.war_pawn));
            }return ok;};
        assert(!rebind_weapon(y.startup,changing,out)&&unchanged(out,before));
    }
    {WeaponFixture y;OwnedConfig out=y.startup;out.pid=777;const OwnedConfig before=out;
        auto read=y.f.read();unsigned checks=0;
        auto late_script_change=[&](void* dest,void const* ptr,std::size_t size){
            const bool ok=read(dest,ptr,size);
            if(reinterpret_cast<std::uintptr_t>(ptr)==y.f.c.script+0x359&&size==38&&++checks==1)
                y.f.m.put(y.f.c.script+0x363,std::uint8_t(0));
            return ok;
        };
        assert(!rebind_weapon(y.startup,late_script_change,out)&&unchanged(out,before)&&checks==2);
    }
    std::cout<<"Same-Pawn weapon rebind startup ownership/class closure/churn/rejection tests passed\n";
}
