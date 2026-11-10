// Actual typed readers and memory-backed fixtures; no game or alternative model.
#define GEARS_COLLIDER_ROLES_TEST_MAIN CoverSchemaPriorColliderMain
#include "cover_collider_roles_live_test.cpp"
#undef GEARS_COLLIDER_ROLES_TEST_MAIN

int main(){
    if(PriorColliderPhysicsRolesMain()||CoverSchemaPriorColliderMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    for(unsigned bodies:{2u,16u,64u}){
        physics_setup();
        for(unsigned i=0;i<bodies;++i){const auto body=arena+0xc0000+i*0x100;
            object(body,150+i,body_class,physics_instance,L"SchemaBody",i);
            put(arena+0x86000+i*8,body);put(body+0x60,int(i));put(body+0x68,fixture_mesh);put(body+0x88,arena+0xd0000+i*0x100);}
        put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,int(bodies),int(bodies)});put(physics_instance+0x7c,int(bodies-1));
        roles::Snapshot out{};const auto begin=reads;C(observe(out));C(out.bodies[bodies-1].index==int(bodies-1));
        printf("schema_repeat physics bodies=%u reads=%u\n",bodies,reads-begin);
    }
    for(unsigned presence=0;presence<3;++presence){
        collider_setup();if(presence==0)put(cylinder+0x1a0,uintptr_t{});if(presence==1)put(single_body+0x88,uintptr_t{});
        collider::Snapshot result{};const auto begin=reads;C(capture(result)==collider::Code::current);
        C(result.presence==(presence==0?collider::Presence::body_instance_field_null:presence==1?collider::Presence::body_data_field_null:collider::Presence::body_data_nonnull));
        printf("schema_repeat collider presence=%u reads=%u\n",presence,reads-begin);
    }
    for(unsigned pass:{1u,2u})for(unsigned bad=0;bad<7;++bad){
        physics_setup();roles::Snapshot out{};C(observe(out));const auto initial=out;
        if(bad==0)plan(body_instances[0]+0x88,pass,props[0]+0x94,0x240u,4);
        if(bad==1)plan(body_instances[0]+0x88,pass,props[3]+80,fixture_object_property,8);
        if(bad==2)plan(body_instances[0]+0x88,pass,props[6]+0xd0,fixture_weapon_base,8);
        if(bad==3)plan(body_instances[0]+0x88,pass,fixture_mesh_class+0x60,fixture_weapon_base,8);
        if(bad==4)plan(body_instances[0]+0x88,pass,props[8]+0x68,props[5],8);
        if(bad==5)plan(body_instances[0]+0x88,pass,inner+0x74,4,4);
        if(bad==6){plan(body_instances[0]+0x88,pass,0,0,0);change_generation=true;}
        C(roles::sample(actor,physics_expected,guard,workspace,out)!=roles::Code::current&&roles::same(initial,out));read_hook=nullptr;
    }
    for(unsigned pass:{1u,2u})for(unsigned bad=0;bad<7;++bad){
        collider_setup();collider::Snapshot out{};C(capture(out)==collider::Code::current);const auto initial=out;
        if(bad==0)plan(single_body+0x88,pass,cylinder_prop+0x94,0x1f8u,4);
        if(bad==1)plan(single_body+0x88,pass,instance_prop+0xd0,fixture_weapon_base,8);
        if(bad==2)plan(single_body+0x88,pass,cylinder_class+0x60,fixture_weapon_base,8);
        if(bad==3)plan(single_body+0x88,pass,props[8]+0x68,props[5],8);
        if(bad==4)plan(single_body+0x88,pass,props[7]+0x94,0x80u,4);
        if(bad==5)plan(single_body+0x88,pass,owner_prop+80,fixture_component_property,8);
        if(bad==6){plan(single_body+0x88,pass,0,0,0);change_generation=true;}
        C(collider::sample(actor,guard,collider_work,out)!=collider::Code::current&&collider::same(initial,out));read_hook=nullptr;
    }
    // Both null/non-null transitions remain changes. A null first pass never
    // authorizes consumption of unbuilt body offsets during the second pass.
    for(bool initially_null:{false,true}){
        collider_setup();if(initially_null)put(cylinder+0x1a0,uintptr_t{});
        collider::Snapshot out{};C(capture(out)==collider::Code::current);const auto initial=out;
        plan(cylinder+0x1a0,2,cylinder+0x1a0,initially_null?single_body:uintptr_t{},8);
        C(collider::sample(actor,guard,collider_work,out)!=collider::Code::current&&collider::same(initial,out));read_hook=nullptr;
    }
    C(native_calls==0&&guard_lock_free);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("cover_schema_repeat: %u checks, %u failures; includes existing physics/collider fixtures\n",checks,failures);
    return failures?1:0;
}
