#define main original_asset_read_regression
#include "weapon_lancer_asset_read_test.cpp"
#undef main
namespace {
namespace unified_weapon_fire {
namespace metadata=gripdamage_metadata;using U32=metadata::U32;
#include "weapon_gnasher_route_closure_test.inl"
}
namespace route {
namespace hand=gears_weapon_hand;
#include "weapon_gnasher_route_asset.inl"
}
bool old_route(AssetMemory& m){
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField mesh{},asset{};
    return meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,m.frame.weapon)&&
        weaponhand::object(r,m.identity(m.component))&&weaponhand::object(r,m.identity(m.asset))&&
        lancermain_asset::mesh_pointer(r,m.frame.weapon.address,mesh)&&mesh.value==m.component&&
        weaponhand::pointer(r,mesh.value,L"SkeletalMesh",m.asset,asset)&&unified_weapon_fire::retained_metadata(r);
}
unsigned route_cases{};
void check(bool value){if(!value)std::fprintf(stderr,"GN route check %u failed\n",route_cases+1);assert(value);++route_cases;}
}
int main(){
    assert(original_asset_read_regression()==0);
    {
        auto m=std::make_unique<AssetMemory>();std::size_t old_reads{},new_reads{};
        read_observer=[&](uptr,std::size_t){++old_reads;};check(old_route(*m));
        read_observer=[&](uptr,std::size_t){++new_reads;};
        check(route::route_asset_metadata(m->frame,m->identity(m->component),m->identity(m->asset)));
        read_observer={};check(new_reads<old_reads);
        std::printf("GN route owned-memory reads old=%zu new=%zu (not game timings)\n",old_reads,new_reads);
    }
    for(unsigned reason=0;reason<12;++reason){
        auto m=std::make_unique<AssetMemory>();const auto component=m->identity(m->component),asset=m->identity(m->asset);
        // Mutate immediately after the real discovery reads the asset link.
        // The old discovery identities must never pass the closing reread.
        mutate_read=m->component+0x120;
        mutation=[&,reason]{switch(reason){
            case 0:Memory::put(m->weapon,8,999999u);break;
            case 1:Memory::put(m->component,0x40,uptr{});break;
            case 2:Memory::put(m->asset,0x48,m->text(L"ChangedAsset"));break;
            case 3:Memory::put(m->weapon_mesh_property,0x94,0x120u);break;
            case 4:Memory::put(m->asset_property,0x74,4u);break;
            case 5:Memory::put(m->asset_property,0x40,m->weapon_class);break;
            case 6:Memory::put(m->mesh_class,0x88,1u);break;
            case 7:fail_read=m->asset+0x48;break;
            case 8:Memory::put(m->weapon_mesh_property,0x48,m->text(L"ChangedMeshField"));break;
            case 9:Memory::put(m->asset,0x50,m->mesh_class);break;
            case 10:Memory::put(m->weapon,0x118,uptr{});break;
            case 11:Memory::put(m->component,0x120,m->component);break;
        }};
        check(!route::route_asset_metadata(m->frame,component,asset));
    }
    for(unsigned reason=0;reason<4;++reason){
        auto m=std::make_unique<AssetMemory>();const auto component=m->identity(m->component),asset=m->identity(m->asset);
        switch(reason){
            case 0:Memory::put(m->weapon,0x118,uptr{});break;
            case 1:Memory::put(m->component,0x120,m->component);break;
            case 2:Memory::put(m->weapon_mesh_property,0x74,4u);break;
            case 3:fail_read=m->component+0x120;break;
        }
        check(!route::route_asset_metadata(m->frame,component,asset));
    }
    {
        auto m=std::make_unique<AssetMemory>();const auto component=m->identity(m->component),asset=m->identity(m->asset);
        check(route::route_asset_metadata(m->frame,component,asset));
        Memory::put(m->asset,0x48,m->text(L"ChangedBetweenQueries"));
        check(!route::route_asset_metadata(m->frame,component,asset));
    }
    // Reproduce the prior route's concrete closing omissions. These are
    // expected old failures, never new-route acceptance cases.
    for(unsigned reason=0;reason<2;++reason){
        auto m=std::make_unique<AssetMemory>();mutate_read=m->component+0x120;
        mutation=[&,reason]{if(reason)Memory::put(m->weapon,0x118,uptr{});
            else Memory::put(m->weapon_mesh_property,0x94,0x120u);};
        check(old_route(*m));
    }
    std::printf("WeaponGnasherRouteAssetTest: %u focused checks passed; fresh closing mutations rejected\n",route_cases);return 0;
}
