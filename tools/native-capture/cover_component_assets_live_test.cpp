// Exercise the actual catalog's optional skeletal PAA reader. The registry and
// memory are synthetic; no original engine/PhysX method or body write runs.
#define GEARS_COMPONENT_ROLES_TEST_MAIN PriorAssetCatalogMain
#include "cover_component_roles_live_test.cpp"
#undef GEARS_COMPONENT_ROLES_TEST_MAIN
#if GEARS_COVER_BODY_PROTOTYPE
static uintptr_t secondary_instance{},secondary_body{},secondary_array{};
static void secondary_setup(){
    catalog_setup();secondary_instance=arena+0xd4000;secondary_body=arena+0xd4800;secondary_array=arena+0xd6000;
    object(secondary_instance,190,instance_class,fixture_weapon,L"SecondaryInstance");
    object(secondary_body,191,body_class,secondary_instance,L"SecondaryBody");
    put(weapon_mesh+0x248,physics_asset);put(weapon_mesh+0x250,secondary_instance);
    put(secondary_instance+0x60,fixture_weapon);put(secondary_instance+0x6c,roles::pose::ArrayHeader{secondary_array,3,4});
    put(secondary_instance+0x7c,-1);put(secondary_array,secondary_body);put(secondary_array+8,uintptr_t{});put(secondary_array+16,secondary_body);
    put(secondary_body+0x60,-7);put(secondary_body+0x68,fixture_mesh);put(secondary_body+0x90,arena+0xb5000);
}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto result=std::make_unique<catalog::Snapshot>();secondary_setup();
    C(capture_catalog(*result)==catalog::Code::current&&result->asset_body_count==5);
    auto const& main_asset=result->components[1].physics;auto const& extra=result->components[3].physics;
    C(main_asset.state==catalog::AssetState::instance_present&&main_asset.instance.address==physics_instance&&main_asset.array.count==2);
    C(extra.state==catalog::AssetState::instance_present&&extra.asset.address==physics_asset&&extra.instance.address==secondary_instance&&
        extra.owner.address==fixture_weapon&&extra.root==-1&&extra.first_body==2&&extra.array.count==3);
    C(result->components[0].physics.state==catalog::AssetState::nonskeletal&&result->components[2].physics.state==catalog::AssetState::nonskeletal);
    C(result->asset_bodies[2].body.identity.address==secondary_body&&result->asset_bodies[2].body.index==-7&&
        result->asset_bodies[2].owner.address==fixture_mesh&&result->asset_bodies[2].owner_actor.address==fixture_pawn&&result->asset_bodies[2].owner_roots==1&&
        !result->asset_bodies[2].body.main&&result->asset_bodies[2].body.auxiliary==arena+0xb5000);
    C(catalog::same(result->asset_bodies[3],catalog::AssetBody{})&&catalog::same(result->asset_bodies[2],result->asset_bodies[4]));
    auto accepted=std::make_unique<catalog::Snapshot>(*result);
    C(capture_catalog(*result)==catalog::Code::current&&catalog::same(*result,*accepted));
    // Shared PAA and body references retain their actual Owner and row order.
    put(weapon_mesh+0x250,physics_instance);put(weapon_mesh+0x248,uintptr_t{});
    C(capture_catalog(*result)==catalog::Code::current&&result->asset_body_count==4&&
        !result->components[3].physics.asset.address&&result->components[3].physics.owner.address==fixture_pawn&&
        catalog::same(result->asset_bodies[0],result->asset_bodies[2]));
    secondary_setup();put(weapon_mesh+0x250,uintptr_t{});
    C(capture_catalog(*result)==catalog::Code::current&&result->asset_body_count==2&&
        result->components[3].physics.state==catalog::AssetState::instance_field_null&&result->components[3].physics.asset.address==physics_asset&&
        !result->components[3].physics.instance.address&&!result->components[3].physics.array.data&&
        catalog::same(result->asset_bodies[2],catalog::AssetBody{}));
    secondary_setup();put(secondary_instance+0x60,uintptr_t{});put(secondary_instance+0x6c,roles::pose::ArrayHeader{});
    C(capture_catalog(*result)==catalog::Code::current&&result->asset_body_count==2&&
        !result->components[3].physics.owner.address&&!result->components[3].physics.array.count&&result->components[3].physics.root==-1);
    put(secondary_instance+0x6c,roles::pose::ArrayHeader{secondary_array,0,4});
    C(capture_catalog(*result)==catalog::Code::current&&result->components[3].physics.array.capacity==4);
    for(unsigned bad=0;bad<18;++bad){secondary_setup();*result=*accepted;
        if(bad==0)put(props[0]+0x94,0x1a0u);
        if(bad==1)put(weapon_mesh+0x248,secondary_body);
        if(bad==2)put(weapon_mesh+0x250,physics_asset);
        if(bad==3)put(props[2]+0x94,0x70u);
        if(bad==4)put(inner+0x74,4u);
        if(bad==5)put(inner+0xd0,fixture_actor);
        if(bad==6)put(secondary_instance+0x6c,roles::pose::ArrayHeader{secondary_array,65,65});
        if(bad==7)put(secondary_instance+0x6c,roles::pose::ArrayHeader{0,1,1});
        if(bad==8)put(secondary_instance+0x6c,roles::pose::ArrayHeader{secondary_array+1,3,4});
        if(bad==9)put(secondary_instance+0x7c,-2);
        if(bad==10)put(secondary_instance+0x7c,3);
        if(bad==11)put(secondary_body+0x68,fixture_weapon);
        if(bad==12)put(secondary_body+0x88,arena+0xb0001);
        if(bad==13)denied=secondary_array;
        if(bad==14)denied=secondary_body+0x90;
        if(bad==15)put(secondary_instance+0x60,secondary_body);
        if(bad==16)put(props[4]+0x94,0x74u);
        if(bad==17)put(secondary_instance+0x6c,roles::pose::ArrayHeader{secondary_array,3,2});
        C(capture_catalog(*result)!=catalog::Code::current&&catalog::same(*result,*accepted));
    }
    for(unsigned change=0;change<6;++change){secondary_setup();*result=*accepted;
        // Changes after the first complete secondary row must invalidate the
        // complete catalog, including fields not carried by the raw actor set.
        if(change==0)plan(secondary_body+0x90,2,secondary_array+8,secondary_body,8);
        if(change==1)plan(secondary_body+0x90,2,secondary_instance+0x60,fixture_pawn,8);
        if(change==2)plan(secondary_body+0x90,2,secondary_body+0x60,8,4);
        if(change==3)plan(secondary_body+0x90,2,secondary_instance+0x7c,0,4);
        if(change==4)plan(secondary_body+0x90,2,weapon_mesh+0x248,0,8);
        if(change==5)plan(secondary_body+0x90,2,secondary_instance+0x6c,secondary_array+0x100,8);
        C(catalog::sample(actor,guard,catalog_work,*result)!=catalog::Code::current&&catalog::same(*result,*accepted));read_hook=nullptr;
    }
    // A full pool is valid; the next body slot rejects, even with shared PAA.
    catalog_setup();
    for(unsigned n=0;n<128;++n){const auto at=n?arena+0xd8000+n*0x300:fixture_mesh;
        if(n){object(at,800+n,fixture_mesh_class,fixture_pawn,L"SharedSkeletal",n);put(at+0x170,fixture_pawn);}
        put(at+0x248,physics_asset);put(at+0x250,physics_instance);put(arena+0xd0000+(n/64)*0x1000+(n%64)*8,at);}
    for(unsigned n=0;n<2;++n)put(fixture_pawn+0x500+n*16,roles::pose::ArrayHeader{arena+0xd0000+n*0x1000,64,64});
    put(fixture_weapon+0x500,roles::pose::ArrayHeader{});put(fixture_weapon+0x510,roles::pose::ArrayHeader{});
    C(capture_catalog(*result)==catalog::Code::current&&result->asset_body_count==256&&result->components[127].physics.first_body==254&&
        result->asset_bodies[255].body.identity.address==body_instances[1]);
    auto full=std::make_unique<catalog::Snapshot>(*result);put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,3,3});
    C(capture_catalog(*result)==catalog::Code::capacity&&catalog::same(*result,*full));
    // Returning to an empty catalog clears every previous scope and pool row.
    for(auto root:{fixture_pawn,fixture_weapon})for(unsigned n=0;n<2;++n)put(root+0x500+n*16,roles::pose::ArrayHeader{});
    put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,2,2});
    C(capture_catalog(*result)==catalog::Code::current&&result->asset_body_count==2&&result->component_count==1&&!result->pawn_mesh_listed&&
        catalog::same(result->asset_bodies[255],catalog::AssetBody{})&&catalog::same(result->components[127].physics,catalog::AssetScope{}));
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverComponentAssets checks=%u failures=%u optional PAA sources only; no original calls or complete self proof\n",checks,failures);return failures?1:0;
}
#else
int main(){return PriorAssetCatalogMain();}
#endif
