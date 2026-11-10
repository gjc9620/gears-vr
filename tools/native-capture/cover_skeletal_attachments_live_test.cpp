#define GEARS_COMPONENT_ROLES_TEST_MAIN PriorSkeletalCatalogMain
#include "cover_component_roles_live_test.cpp"
#undef GEARS_COMPONENT_ROLES_TEST_MAIN
#if GEARS_COVER_BODY_PROTOTYPE
static uintptr_t attached_mesh{},attached_leaf{},attached_body{};
static void attachment_rows(uintptr_t data,unsigned count,unsigned stride,unsigned component_offset,uintptr_t component){
    for(unsigned n=0;n<count;++n){std::array<unsigned char,catalog::attachment_stride_capacity> bytes{};
        const auto bone_offset=get<unsigned>(arena+0xc6000+0x94);const std::uint64_t bone=0x123400000056ull+n;
        std::memcpy(bytes.data()+component_offset,&component,8);std::memcpy(bytes.data()+bone_offset,&bone,8);
        const std::array<float,3> location{float(n),.5f,-.5f},scale{1,1,1};const std::array<int,3> rotation{100,200,300};
        std::memcpy(bytes.data()+16,location.data(),12);std::memcpy(bytes.data()+28,rotation.data(),12);std::memcpy(bytes.data()+40,scale.data(),12);
        bytes[stride-1]=static_cast<unsigned char>(0x30+n);std::memcpy(reinterpret_cast<void*>(data+std::size_t(n)*stride),bytes.data(),stride);}
}
static void skeletal_setup(){
    catalog_setup();attached_mesh=arena+0xd4000;attached_leaf=arena+0xd4800;attached_body=arena+0xd5000;
    object(attached_mesh,190,fixture_mesh_class,fixture_pc,L"AttachedSkeletalMesh");object(attached_leaf,191,audio_class,fixture_pawn,L"AttachedAudio");
    object(attached_body,192,body_class,attached_mesh,L"AttachedSkeletalBody");
    put(attached_mesh+0x170,fixture_pc);put(attached_leaf+0x170,fixture_pawn);put(attached_mesh+0x1a0,attached_body);put(attached_mesh+0x248,physics_asset);
    put(attached_body+0x60,-9);put(attached_body+0x68,attached_mesh);put(attached_body+0x88,arena+0xb6000);
    put(arena+0xb6000,uintptr_t(0x1802330a8));put(arena+0xb6038,arena+0x8b000);put(arena+0xb6040,arena+0xb7000);put(arena+0xb7010,arena+0xb6000);
    put(fixture_mesh+0x260,roles::pose::ArrayHeader{arena+0xd6000,4,4});attachment_rows(arena+0xd6000,4,56,0,attached_mesh);
    put(arena+0xd6000+56,uintptr_t{});put(arena+0xd6000+56*3,fixture_mesh);
    put(attached_mesh+0x260,roles::pose::ArrayHeader{arena+0xd7000,2,2});attachment_rows(arena+0xd7000,2,56,0,attached_leaf);put(arena+0xd7000+56,weapon_mesh);
    put(weapon_mesh+0x260,roles::pose::ArrayHeader{arena+0xd8000,1,1});attachment_rows(arena+0xd8000,1,56,0,attached_mesh);
}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto result=std::make_unique<catalog::Snapshot>();skeletal_setup();
    C(capture_catalog(*result)==catalog::Code::current&&result->component_count==6&&result->attachment_count==7&&result->mesh_seed.address==fixture_mesh);
    C(result->components[4].identity.address==attached_mesh&&!result->components[4].owner_roots&&result->components[4].body.main==arena+0xb6000&&
        result->components[5].identity.address==attached_leaf&&result->components[5].body_state==catalog::BodyState::nonprimitive);
    C(result->components[1].attachments.stride==56&&result->components[1].attachments.component_offset==0&&result->components[3].attachments.first_row==4&&result->components[4].attachments.first_row==5);
    C(!result->attachment_rows[1].component&&result->attachment_rows[1].bytes[8]&&result->attachment_rows[0].component==result->attachment_rows[2].component&&
        result->attachment_rows[0].bytes!=result->attachment_rows[2].bytes);
    auto accepted=std::make_unique<catalog::Snapshot>(*result);auto raw=std::make_unique<gears_cover_physx_components::Source>();
    C(catalog::input(*result,*raw)&&raw->attachment_count==7&&raw->mesh_seed==fixture_mesh&&raw->attachment_rows[4].component==attached_mesh);
    C(capture_catalog(*result)==catalog::Code::current&&catalog::same(*result,*accepted));
    // The primary Mesh is an explicit seed even when Actor arrays omit it.
    put(arena+0xd0000+16,uintptr_t{});put(arena+0xd1000,uintptr_t{});
    C(capture_catalog(*result)==catalog::Code::current&&!result->pawn_mesh_listed&&result->component_count==6&&result->components[3].identity.address==fixture_mesh&&result->attachment_count==7);
    C(catalog::input(*result,*raw)&&!raw->pawn_mesh_listed&&raw->mesh_seed==fixture_mesh);
    // Runtime stride and Component offset are reflected, not packed constants.
    skeletal_setup();put(arena+0xc5400+0x74,64u);put(arena+0xc5088,64u);put(arena+0xc5c00+0x94,8u);put(arena+0xc6000+0x94,0u);
    attachment_rows(arena+0xd6000,4,64,8,attached_mesh);put(arena+0xd6000+64+8,uintptr_t{});put(arena+0xd6000+64*3+8,fixture_mesh);
    attachment_rows(arena+0xd7000,2,64,8,attached_leaf);put(arena+0xd7000+64+8,weapon_mesh);attachment_rows(arena+0xd8000,1,64,8,attached_mesh);
    C(capture_catalog(*result)==catalog::Code::current&&result->components[1].attachments.stride==64&&result->components[1].attachments.component_offset==8&&result->attachment_rows[0].component==attached_mesh);
    C(catalog::input(*result,*raw));
    for(unsigned bad=0;bad<21;++bad){skeletal_setup();*result=*accepted;
        if(bad==0)put(arena+0xc5400+0x50,fixture_object_property);
        if(bad==1)put(arena+0xc5400+0x74,0u);
        if(bad==2)put(arena+0xc5400+0x74,129u);
        if(bad==3)put(arena+0xc5000+0x50,fixture_meta);
        if(bad==4)put(arena+0xc5000+0x40,fixture_actor);
        if(bad==5)put(arena+0xc5088,64u);
        if(bad==6)put(arena+0xc5060,arena+0xc7000);
        if(bad==7)put(arena+0xc5c00+0x50,fixture_object_property);
        if(bad==8)put(arena+0xc5c00+0xd0,primitive_class);
        if(bad==9)put(arena+0xc5c00+0x70,2u);
        if(bad==10)put(arena+0xc5c00+0x94,55u);
        if(bad==11)put(arena+0xc6000+0x94,0u);
        if(bad==12)put(arena+0xc6800+0x68,uintptr_t{});
        if(bad==13)put(arena+0xc6400+0xd0,arena+0xc7400);
        if(bad==14)put(arena+0xc7088,16u);
        if(bad==15)put(fixture_mesh+0x260,roles::pose::ArrayHeader{arena+0xd6000,65,65});
        if(bad==16)put(fixture_mesh+0x260,roles::pose::ArrayHeader{arena+0xd6000,4,3});
        if(bad==17)put(fixture_mesh+0x260,roles::pose::ArrayHeader{0,1,1});
        if(bad==18)put(arena+0xd6000,attached_mesh+1);
        if(bad==19)denied=arena+0xd6000;
        if(bad==20)put(attached_mesh+0x170,uintptr_t{});
        C(capture_catalog(*result)!=catalog::Code::current&&catalog::same(*result,*accepted));
    }
    for(unsigned change=0;change<6;++change){skeletal_setup();*result=*accepted;
        if(change==0)plan(attached_body+0x90,1,arena+0xd6000+16,0x40000000,4);
        if(change==1)plan(attached_body+0x90,1,arena+0xd6000+8,7,8);
        if(change==2)plan(attached_body+0x90,1,arena+0xd6000+55,9,1);
        if(change==3)plan(attached_body+0x90,1,arena+0xd6000,attached_leaf,8);
        if(change==4)plan(attached_body+0x90,1,arena+0xc5400+0x74,64,4);
        if(change==5)plan(attached_body+0x90,1,fixture_mesh+0x260,arena+0xda000,8);
        C(catalog::sample(actor,guard,catalog_work,*result)!=catalog::Code::current&&catalog::same(*result,*accepted));read_hook=nullptr;
    }
    skeletal_setup();object(attached_leaf,191,fixture_mesh_class,fixture_pawn,L"LeafSkeletalMesh");put(attached_leaf+0x170,fixture_pawn);
    const std::array<uintptr_t,4> meshes{fixture_mesh,weapon_mesh,attached_mesh,attached_leaf};
    for(unsigned n=0;n<4;++n){const auto data=arena+0xd6000+n*0x1000;put(meshes[n]+0x260,roles::pose::ArrayHeader{data,64,64});
        attachment_rows(data,64,56,0,n==2?attached_leaf:n==3?fixture_mesh:attached_mesh);}
    C(capture_catalog(*result)==catalog::Code::current&&result->attachment_count==256&&result->attachment_rows[255].component==fixture_mesh);
    auto full=std::make_unique<catalog::Snapshot>(*result);const auto extra=arena+0xda000;
    object(extra,199,fixture_mesh_class,fixture_pawn,L"OverflowSkeletalMesh");put(extra+0x170,fixture_pawn);
    put(extra+0x260,roles::pose::ArrayHeader{arena+0xdb000,1,1});attachment_rows(arena+0xdb000,1,56,0,fixture_mesh);put(arena+0xd6000,extra);
    C(capture_catalog(*result)==catalog::Code::capacity&&catalog::same(*result,*full));
    skeletal_setup();for(auto mesh:{fixture_mesh,weapon_mesh})put(mesh+0x260,roles::pose::ArrayHeader{});
    C(capture_catalog(*result)==catalog::Code::current&&result->component_count==4&&!result->attachment_count&&!result->attachment_rows[255].component&&!result->components[4].identity.address);
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverSkeletalAttachments checks=%u failures=%u reflected structure graph only; no complete self or world fence\n",checks,failures);return failures?1:0;
}
#else
int main(){return PriorSkeletalCatalogMain();}
#endif
