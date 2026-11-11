#define GEARS_PHYSICS_ROLES_TEST_MAIN PriorColliderPhysicsRolesMain
#include "cover_physics_roles_live_test.cpp"
#undef GEARS_PHYSICS_ROLES_TEST_MAIN
#include "cover_collider_roles_live.inl"
#include <thread>
namespace collider=cover_collider_roles_live;
#ifndef GEARS_COLLIDER_ROLES_TEST_MAIN
#define GEARS_COLLIDER_ROLES_TEST_MAIN main
#endif
#if GEARS_COVER_BODY_PROTOTYPE
static collider::Workspace collider_work;
static uintptr_t primitive_class{},cylinder_class{},cylinder{},single_body{},cylinder_prop{},collision_prop{},owner_prop{},instance_prop{};
static void collider_setup(){
    physics_setup();primitive_class=arena+0xa0000;cylinder_class=arena+0xa0400;
    klass(primitive_class,140,L"PrimitiveComponent",0);klass(cylinder_class,141,L"CylinderComponent",primitive_class);
    put(fixture_mesh_class+0x60,primitive_class);
    cylinder=arena+0xa1000;single_body=arena+0xa2000;
    object(cylinder,142,cylinder_class,fixture_pawn,L"SelectedCylinder",4);
    object(single_body,143,body_class,cylinder,L"SingleBody");
    cylinder_prop=arena+0xa3000;collision_prop=arena+0xa3400;owner_prop=arena+0xa3800;instance_prop=arena+0xa3c00;
    typed_property(cylinder_prop,144,fixture_pawn_class,L"CylinderComponent",fixture_component_property,8,0x1f0,cylinder_class);
    typed_property(collision_prop,145,fixture_pawn_class,L"CollisionComponent",fixture_component_property,8,0x1f8,primitive_class);
    put(special_property+0x68,cylinder_prop);put(cylinder_prop+0x68,collision_prop);
    typed_property(owner_prop,146,primitive_class,L"Owner",fixture_object_property,8,0x170,fixture_actor);
    typed_property(instance_prop,147,primitive_class,L"BodyInstance",fixture_object_property,8,0x1a0,body_class);
    put(primitive_class+0x80,owner_prop);put(owner_prop+0x68,instance_prop);
    put(fixture_pawn+0x1f0,cylinder);put(fixture_pawn+0x1f8,cylinder);
    put(cylinder+0x170,fixture_pawn);put(cylinder+0x1a0,single_body);
    put(single_body+0x60,-1);put(single_body+0x68,cylinder);put(single_body+0x88,arena+0xb0000);put(single_body+0x90,arena+0xb1000);
    put(arena+0xb0000,uintptr_t(0x1802330a8));put(arena+0xb0038,arena+0x8b000);put(arena+0xb0040,arena+0xb3000);put(arena+0xb3010,arena+0xb0000);
    put(props[6]+0xd0,primitive_class); // Real RB_BodyInstance owner role is a PrimitiveComponent.
    // physics_setup already sampled its smaller synthetic schema. Construct
    // the extended fixture before obtaining this test's actual binding.
    h::base_proof={};h::instigator_proof={};
    C(h::sample(&scene,actor));
}
static collider::Code capture(collider::Snapshot& result){
    memcpy(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size());
    const auto code=collider::sample(actor,guard,collider_work,result);
    C(!memcmp(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size()));C(native_calls==0&&guard_lock_free);
    return code;
}
int GEARS_COLLIDER_ROLES_TEST_MAIN(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    collider_setup();collider::Snapshot result{};
    C(capture(result)==collider::Code::current&&result.cylinder.address==cylinder&&result.body.address==single_body);
    C(result.presence==collider::Presence::body_data_nonnull&&result.body_index==-1&&result.body_data==arena+0xb0000&&result.bone_spring==arena+0xb1000);
    const auto initial=result;C(capture(result)==collider::Code::current&&collider::same(result,initial));
    {namespace cx=gears_cover_physx_collider;cx::Snapshot raw{};raw.fields=collider::input(result);raw.presence=cx::Presence::actor_observed;
        raw.query_source.actor_table=raw.actor_table=0x1802330a8;raw.query_source.scene=raw.scene=arena+0x8b000;raw.internal=arena+0xb3000;
        C(collider::matches(result,raw));
        for(unsigned bad=0;bad<6;++bad){auto wrong=raw;if(bad==0)wrong.fields.component+=8;if(bad==1)wrong.fields.body_index++;
            if(bad==2)wrong.fields.body_instance_offset+=8;if(bad==3)wrong.actor_table+=8;if(bad==4)wrong.scene+=8;if(bad==5)wrong.internal=0;
            C(!collider::matches(result,wrong));}}
    put(cylinder+0x1a0,uintptr_t{});C(capture(result)==collider::Code::current&&result.presence==collider::Presence::body_instance_field_null&&!result.body.address&&!result.body_data&&!result.bone_spring);
    put(cylinder+0x1a0,single_body);put(single_body+0x88,uintptr_t{});
    C(capture(result)==collider::Code::current&&result.presence==collider::Presence::body_data_field_null&&result.body.address==single_body&&result.bone_spring==arena+0xb1000);
    collider_setup();put(instance_prop+0x94,0x1b0u);put(cylinder+0x1b0,single_body);put(cylinder+0x1a0,uintptr_t{});
    C(capture(result)==collider::Code::current&&result.body.address==single_body); // Reflected, not a guessed component offset.
    for(unsigned bad=0;bad<24;++bad){collider_setup();result=initial;
        if(bad==0)put(fixture_pawn+0x1f0,uintptr_t{});
        if(bad==1)put(fixture_pawn+0x1f8,fixture_mesh);
        if(bad==2)put(cylinder+0x40,fixture_world);
        if(bad==3)put(cylinder+0x170,fixture_pc);
        if(bad==4)put(cylinder+0x50,fixture_mesh_class);
        if(bad==5)put(cylinder+0x1a0,fixture_weapon);
        if(bad==6)put(single_body+0x68,fixture_mesh);
        if(bad==7)put(single_body+0x88,arena+0xb0001);
        if(bad==8)put(single_body+0x90,arena+0xb1001);
        if(bad==9)put(cylinder_prop+0x74,4u);
        if(bad==10)put(cylinder_prop+0x50,fixture_object_property);
        if(bad==11)put(collision_prop+0xd0,fixture_mesh_class);
        if(bad==12)put(instance_prop+0xd0,fixture_weapon_base);
        if(bad==13)put(owner_prop+0xd0,fixture_weapon_base);
        if(bad==14)put(props[6]+0xd0,fixture_mesh_class);
        if(bad==15)put(props[7]+0x94,0x80u);
        if(bad==16)put(pointer_type+0x88,16u);
        if(bad==17)put(collision_prop+0x68,cylinder_prop);
        if(bad==18)put(cylinder_class+0x60,fixture_actor);
        if(bad==19)denied=single_body+0x88;
        if(bad==20)denied=arena+0x1000+142*8;
        if(bad==21)object(single_body,143,body_class,cylinder,L"Default__RB_BodyInstance");
        if(bad==22)object(cylinder,142,cylinder_class,fixture_pawn,L"Default__CylinderComponent");
        if(bad==23)put(instance_prop+0x40,fixture_mesh_class);
        C(capture(result)!=collider::Code::current&&collider::same(result,initial));
    }
    for(unsigned change=0;change<7;++change){collider_setup();result=initial;
        if(change==0)plan(single_body+0x88,1,cylinder+0x1a0,0,8);
        // Mutate on BoneSpring's own second observation. This remains a
        // between-read change with scalar or paired source-field readers.
        if(change==1)plan(single_body+0x90,2,single_body+0x90,arena+0xb2000,8);
        if(change==2)plan(single_body+0x88,1,instance_prop+0x94,0x1a8,4);
        if(change==3){plan(single_body+0x88,1,0,0,0);change_generation=true;}
        if(change==4){plan(single_body+0x88,1,0,0,0);throw_read=true;}
        if(change==5)plan(single_body+0x88,1,single_body+0x68,fixture_mesh,8);
        if(change==6)plan(single_body+0x88,1,cylinder+0x170,fixture_pc,8);
        C(collider::sample(actor,guard,collider_work,result)!=collider::Code::current&&collider::same(result,initial));read_hook=nullptr;
        C(h::mutex.try_lock());h::mutex.unlock();
    }
    collider_setup();result=initial;
    C(collider::sample(actor,[](){return false;},collider_work,result)==collider::Code::guard&&collider::same(result,initial));
    unsigned boundaries{};C(collider::sample(actor,[&](){return ++boundaries<2;},collider_work,result)==collider::Code::guard&&collider::same(result,initial));
    C(collider::sample(actor,guard,collider_work,collider_work.first)==collider::Code::request);
    auto invalid=actor;invalid.context_valid=false;const auto old_reads=reads;unsigned invalid_guards{};
    C(collider::sample(invalid,[&](){++invalid_guards;return true;},collider_work,result)==collider::Code::request&&!invalid_guards&&reads==old_reads&&collider::same(result,initial));
    h::mutex.lock();C(collider::sample(actor,[](){return true;},collider_work,result)==collider::Code::metadata);h::mutex.unlock();
    collider::Code other{};std::thread worker([&](){other=collider::sample(actor,[](){return true;},collider_work,result);});worker.join();
    C(other==collider::Code::metadata&&collider::same(result,initial));
    auto mutable_actor=actor;unsigned last{};
    C(collider::sample(mutable_actor,[&](){if(++last==2)mutable_actor.scene_invocation++;return true;},collider_work,result)==collider::Code::changed&&collider::same(result,initial));
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverColliderRoles checks=%u failures=%u synthetic reflected fields only; no NxActor/complete-self/geometry/movement proof\n",checks,failures);return failures?1:0;
}
#else
int GEARS_COLLIDER_ROLES_TEST_MAIN(){collider::Snapshot result{};result.body_data=99;collider::Workspace work{};unsigned calls{};
    C(collider::sample(h::Binding{},[&](){++calls;return true;},work,result)==collider::Code::unavailable&&result.body_data==99&&!calls&&!reads&&!native_calls);
    printf("CoverColliderRoles OFF checks=%u failures=%u no reads/guard/calls\n",checks,failures);return failures?1:0;}
#endif
