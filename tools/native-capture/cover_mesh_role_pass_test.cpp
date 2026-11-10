// Actual typed reader, existing memory fixture and closing proof. No game.
#define GEARS_PHYSICS_ROLES_TEST_MAIN CoverMeshRoleExistingFixtureMain
#include "cover_physics_roles_live_test.cpp"
#undef GEARS_PHYSICS_ROLES_TEST_MAIN
#ifndef GEARS_MESH_ROLE_PASS_TEST
#define GEARS_MESH_ROLE_PASS_TEST 1
#endif

int main(){
    if(CoverMeshRoleExistingFixtureMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    for(unsigned bodies:{2u,16u,64u}){
        physics_setup();
        for(unsigned i=0;i<bodies;++i){const auto body=arena+0xa0000+i*0x100;
            object(body,150+i,body_class,physics_instance,L"PassBody",i);
            put(arena+0x86000+i*8,body);put(body+0x60,int(i));put(body+0x68,fixture_mesh);put(body+0x88,arena+0xb0000+i*0x100);}
        put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,int(bodies),int(bodies)});put(physics_instance+0x7c,int(bodies-1));
        roles::Snapshot out{};const auto begin=reads;C(observe(out));const auto used=reads-begin;
        C(workspace.layout_count==1&&out.bodies[bodies-1].index==int(bodies-1));
        printf("mesh_role_pass bodies=%u reads=%u\n",bodies,used);
#if GEARS_MESH_ROLE_PASS_TEST
        C(workspace.layouts[0].mesh_role_pass==2&&workspace.layouts[0].mesh_role_class==fixture_mesh_class);
#endif
    }
    // Mutations after the first body's certification must still be rejected
    // by fresh body reads, pass2 or the unchanged full closing proof.
    for(unsigned pass:{1u,2u})for(unsigned bad=0;bad<6;++bad){
        physics_setup();roles::Snapshot out{};C(observe(out));const auto before=out;
        if(bad==0)plan(body_instances[0]+0x88,pass,props[6]+0xd0,fixture_weapon_base,8);
        if(bad==1)plan(body_instances[0]+0x88,pass,fixture_mesh_class+0x60,fixture_weapon_base,8);
        if(bad==2)plan(body_instances[0]+0x88,pass,fixture_mesh+80,fixture_weapon_base,8);
        if(bad==3)plan(body_instances[0]+0x88,pass,body_instances[1]+0x68,fixture_pc,8);
        if(bad==4)plan(body_instances[0]+0x88,pass,props[8]+0x68,props[5],8);
        if(bad==5){plan(body_instances[0]+0x88,pass,0,0,0);change_generation=true;}
        const auto code=roles::sample(actor,physics_expected,guard,workspace,out);
        C(code!=roles::Code::current&&roles::same(before,out));read_hook=nullptr;
    }
#if GEARS_MESH_ROLE_PASS_TEST
    physics_setup();roles::Snapshot out{};C(observe(out));auto& layout=workspace.layouts[0];
    auto begin=reads;C(roles::body_mesh_role(layout,fixture_mesh_class,2)&&reads==begin);
    begin=reads;C(roles::body_mesh_role(layout,fixture_mesh_class,1)&&reads>begin); // New pass recertifies.
    C(!roles::body_mesh_role(layout,fixture_weapon_base,1)); // Changed class cannot use key.
    const auto reference=layout.fields[1].reference.address;layout.fields[1].reference.address=fixture_weapon_base;
    C(!roles::body_mesh_role(layout,fixture_mesh_class,1));layout.fields[1].reference.address=reference;
    begin=reads;C(roles::body_mesh_role(layout,fixture_mesh_class,0)&&reads>begin);
    begin=reads;C(roles::body_mesh_role(layout,fixture_mesh_class,0)&&reads>begin); // Unset pass never caches.
    put(props[6]+0xd0,fixture_weapon_base);C(!roles::layouts_current(workspace));
#endif
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("cover_mesh_role_pass: %u checks, %u failures; includes original typed-reader checks\n",checks,failures);
    return failures?1:0;
}
