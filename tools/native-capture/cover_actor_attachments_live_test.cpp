#define GEARS_COMPONENT_ROLES_TEST_MAIN PriorActorGraphCatalogMain
#include "cover_component_roles_live_test.cpp"
#undef GEARS_COMPONENT_ROLES_TEST_MAIN
#if GEARS_COVER_BODY_PROTOTYPE
static uintptr_t child_actor{},grandchild_actor{},child_mesh{},child_body{};
static void actor_graph_setup(){
    catalog_setup();child_actor=arena+0xd4000;grandchild_actor=arena+0xd4600;child_mesh=arena+0xd5000;child_body=arena+0xd6000;
    object(child_actor,190,fixture_actor,fixture_pawn,L"AttachedActor");object(grandchild_actor,191,fixture_actor,fixture_pc,L"GrandchildActor");
    object(child_mesh,192,fixture_mesh_class,grandchild_actor,L"AttachedMesh");object(child_body,193,body_class,child_mesh,L"AttachedBody");
    for(auto at:{child_actor,grandchild_actor})put(at+0x188,fixture_world);
    put(child_actor+0x520,fixture_pawn);put(grandchild_actor+0x520,fixture_pc);
    const std::array<std::array<uintptr_t,4>,4> attached{{{{child_actor,0,child_actor,fixture_weapon}},{{child_actor,0,0,0}},{{grandchild_actor,fixture_pawn,0,0}},{{child_actor,0,0,0}}}};
    const std::array<uintptr_t,4> owners{fixture_pawn,fixture_weapon,child_actor,grandchild_actor};
    for(unsigned n=0;n<4;++n){const auto data=arena+0xd7000+n*0x400;put(owners[n]+0x530,roles::pose::ArrayHeader{data,n==0?4:n==2?2:1,4});
        for(unsigned k=0;k<4;++k)put(data+k*8,attached[n][k]);}
    put(child_actor+0x500,roles::pose::ArrayHeader{arena+0xd8000,1,1});put(arena+0xd8000,child_mesh);
    put(grandchild_actor+0x510,roles::pose::ArrayHeader{arena+0xd8400,1,1});put(arena+0xd8400,child_mesh);
    put(child_mesh+0x170,grandchild_actor);put(child_mesh+0x1a0,child_body);put(child_mesh+0x248,physics_asset);put(child_mesh+0x250,physics_instance);
    put(child_body+0x60,-11);put(child_body+0x68,child_mesh);put(child_body+0x88,arena+0xb6000);
    put(arena+0xb6000,uintptr_t(0x1802330a8));put(arena+0xb6038,arena+0x8b000);put(arena+0xb6040,arena+0xb7000);put(arena+0xb7010,arena+0xb6000);
}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto result=std::make_unique<catalog::Snapshot>();actor_graph_setup();
    C(capture_catalog(*result)==catalog::Code::current&&result->seed_count==2&&result->root_count==4&&result->component_count==5&&result->asset_body_count==4);
    C(result->roots[2].actor.address==child_actor&&result->roots[3].actor.address==grandchild_actor&&
        result->roots[2].base.address==fixture_pawn&&result->roots[3].base.address==fixture_pc);
    C(result->roots[0].attached_members[1]==0&&result->roots[0].attached_members[0]==result->roots[0].attached_members[2]&&
        result->roots[2].attached_members[1]==fixture_pawn&&result->roots[3].attached_members[0]==child_actor);
    C(result->components[4].identity.address==child_mesh&&result->components[4].owner_roots==8&&result->components[4].body_owner_roots==8&&
        result->components[4].body.index==-11&&result->components[4].body.main==arena+0xb6000);
    auto accepted=std::make_unique<catalog::Snapshot>(*result);auto raw=std::make_unique<gears_cover_physx_components::Source>();
    C(catalog::input(*result,*raw)&&raw->seed_count==2&&raw->root_count==4&&raw->roots[3].base==fixture_pc&&raw->components[4].owner_roots==8);
    C(capture_catalog(*result)==catalog::Code::current&&catalog::same(*result,*accepted));
    for(unsigned bad=0;bad<13;++bad){actor_graph_setup();*result=*accepted;
        if(bad==0)put(child_actor+0x188,fixture_pc);
        if(bad==1)put(arena+0xc4c00+0xd0,primitive_class);
        if(bad==2)put(arena+0xc4c00+0x50,fixture_component_property);
        if(bad==3)put(child_actor+0x520,child_body);
        if(bad==4)put(child_actor+0x530,roles::pose::ArrayHeader{arena+0xd7801,2,4});
        if(bad==5)put(child_actor+0x530,roles::pose::ArrayHeader{arena+0xd7800,65,65});
        if(bad==6)put(child_actor+0x530,roles::pose::ArrayHeader{0,1,1});
        if(bad==7)put(arena+0xc4800+0x94,0x520u);
        if(bad==8)put(arena+0xc4400+0x94,0x500u);
        if(bad==9)denied=arena+0xd7800;
        if(bad==10)object(child_actor,190,fixture_actor,fixture_pawn,L"Default__AttachedActor");
        if(bad==11)put(arena+0xd7800,child_body);
        if(bad==12)put(arena+0xc4c00+0x74,4u);
        C(capture_catalog(*result)!=catalog::Code::current&&catalog::same(*result,*accepted));
    }
    for(unsigned change=0;change<5;++change){actor_graph_setup();*result=*accepted;
        if(change==0)plan(child_body+0x90,1,arena+0xd7800,fixture_pc,8);
        if(change==1)plan(child_body+0x90,1,child_actor+0x520,fixture_weapon,8);
        if(change==2)plan(child_body+0x90,1,grandchild_actor+0x188,fixture_pc,8);
        if(change==3)plan(child_body+0x90,1,child_actor+0x538,1,4);
        if(change==4)plan(child_body+0x90,1,arena+0xd8400,0,8);
        C(catalog::sample(actor,guard,catalog_work,*result)!=catalog::Code::current&&catalog::same(*result,*accepted));read_hook=nullptr;
    }
    actor_graph_setup();put(fixture_pawn+0x1d0,uintptr_t{});C(h::sample(&scene,actor));
    C(capture_catalog(*result)==catalog::Code::current&&result->seed_count==1&&result->root_count==4&&
        result->roots[1].actor.address==child_actor&&result->roots[2].actor.address==fixture_weapon);
    // All sixteen actor nodes close, including the final node. Another node
    // rejects rather than silently losing its component or physical sources.
    catalog_setup();
    for(unsigned n=0;n<15;++n){const auto at=arena+0xe0000+n*0x600;object(at,800+n,fixture_actor,fixture_pawn,L"BoundedAttachedActor",n);
        put(at+0x188,fixture_world);put(at+0x520,fixture_pawn);put(arena+0xd7000+n*8,at);}
    put(fixture_pawn+0x530,roles::pose::ArrayHeader{arena+0xd7000,14,15});
    C(capture_catalog(*result)==catalog::Code::current&&result->root_count==16&&result->roots[15].actor.address==arena+0xe4e00);
    auto full=std::make_unique<catalog::Snapshot>(*result);put(fixture_pawn+0x538,15);
    C(capture_catalog(*result)==catalog::Code::capacity&&catalog::same(*result,*full));
    put(fixture_pawn+0x530,roles::pose::ArrayHeader{});
    C(capture_catalog(*result)==catalog::Code::current&&result->root_count==2&&result->component_count==4&&
        !result->roots[2].actor.address&&!result->roots[15].base.address&&!result->roots[15].attached_members[0]);
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverActorAttachments checks=%u failures=%u declared recursive Actor edges only; no complete self or world fence\n",checks,failures);return failures?1:0;
}
#else
int main(){return PriorActorGraphCatalogMain();}
#endif
