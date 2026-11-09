#define GEARS_COLLIDER_ROLES_TEST_MAIN PriorComponentColliderMain
#include "cover_collider_roles_live_test.cpp"
#undef GEARS_COLLIDER_ROLES_TEST_MAIN
#include "cover_component_roles_live.inl"
#include <memory>
namespace catalog=cover_component_roles_live;
#ifndef GEARS_COMPONENT_ROLES_TEST_MAIN
#define GEARS_COMPONENT_ROLES_TEST_MAIN main
#endif
#if GEARS_COVER_BODY_PROTOTYPE
static catalog::Workspace catalog_work;
static uintptr_t component_base{},audio_class{},audio{},weapon_mesh{},weapon_body{},array_props[2]{},array_inners[2]{};
static void catalog_setup(){
    collider_setup();component_base=arena+0xc0000;audio_class=arena+0xc0400;
    klass(component_base,170,L"ActorComponent",0);klass(audio_class,171,L"AudioComponent",component_base);put(primitive_class+0x60,component_base);
    audio=arena+0xc1000;weapon_mesh=arena+0xc2000;weapon_body=arena+0xc2800;
    object(audio,172,audio_class,fixture_pawn,L"Audio");object(weapon_mesh,173,fixture_mesh_class,fixture_weapon,L"WeaponMesh");
    object(weapon_body,174,body_class,weapon_mesh,L"WeaponBody");
    typed_property(arena+0xc3000,175,component_base,L"Owner",fixture_object_property,8,0x170,fixture_actor);put(component_base+0x80,arena+0xc3000);
    put(audio+0x170,fixture_pc);put(weapon_mesh+0x170,fixture_weapon);put(fixture_mesh+0x170,fixture_pawn);
    put(weapon_mesh+0x1a0,weapon_body);put(weapon_body+0x68,weapon_mesh);put(weapon_body+0x60,-1);put(weapon_body+0x90,arena+0xb5000);
    for(unsigned n=0;n<2;++n){array_props[n]=arena+0xc3400+n*0x400;array_inners[n]=arena+0xc3c00+n*0x400;
        typed_property(array_props[n],176+n,fixture_actor,n?L"AllComponents":L"Components",array_kind,16,0x500+n*16);
        typed_property(array_inners[n],178+n,array_props[n],n?L"AllComponents":L"Components",fixture_component_property,8,0,component_base);
        put(array_props[n]+0xd0,array_inners[n]);}
    put(instigator_property+0x68,array_props[0]);put(array_props[0]+0x68,array_props[1]);
    typed_property(arena+0xc4400,180,fixture_actor,L"Base",fixture_object_property,8,0x520,fixture_actor);
    typed_property(arena+0xc4800,181,fixture_actor,L"Attached",array_kind,16,0x530);
    typed_property(arena+0xc4c00,182,arena+0xc4800,L"Attached",fixture_object_property,8,0,fixture_actor);
    put(array_props[1]+0x68,arena+0xc4400);put(arena+0xc4400+0x68,arena+0xc4800);put(arena+0xc4800+0xd0,arena+0xc4c00);
    put(fixture_weapon+0x188,fixture_world);
    const std::array<std::array<uintptr_t,4>,4> lists{{{{cylinder,0,fixture_mesh,cylinder}},{{fixture_mesh,audio,cylinder,weapon_mesh}},{{weapon_mesh,0,0,0}},{{weapon_mesh,audio,0,0}}}};
    for(unsigned n=0;n<4;++n){const auto at=arena+0xd0000+n*0x1000;const auto owner=n<2?fixture_pawn:fixture_weapon;
        put(owner+0x500+(n%2)*16,roles::pose::ArrayHeader{at,n==2?1:n==3?2:4,4});for(unsigned k=0;k<4;++k)put(at+k*8,lists[n][k]);}
    h::base_proof={};h::instigator_proof={};C(h::sample(&scene,actor));
}
static catalog::Code capture_catalog(catalog::Snapshot& result){
    memcpy(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size());
    const auto code=catalog::sample(actor,guard,catalog_work,result);
    C(!memcmp(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size())&&native_calls==0&&guard_lock_free);return code;
}
int GEARS_COMPONENT_ROLES_TEST_MAIN(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto result=std::make_unique<catalog::Snapshot>();catalog_setup();
    C(capture_catalog(*result)==catalog::Code::current&&result->root_count==2&&result->component_count==4&&result->pawn_mesh_listed);
    C(result->roots[0].members[0][1]==0&&result->roots[0].members[0][0]==result->roots[0].members[0][3]);
    C(result->components[0].body_state==catalog::BodyState::body_data_nonnull&&result->components[0].owner_roots==1&&result->components[0].body_owner_roots==1);
    C(result->components[1].skeletal&&result->components[1].body_state==catalog::BodyState::body_instance_field_null);
    C(result->components[2].body_state==catalog::BodyState::nonprimitive&&!result->components[2].owner_roots&&result->components[2].owner.address==fixture_pc);
    C(result->components[3].body_state==catalog::BodyState::body_data_field_null&&result->components[3].owner_roots==2&&result->components[3].body.auxiliary==arena+0xb5000);
    const auto initial=std::make_unique<catalog::Snapshot>(*result);
    C(capture_catalog(*result)==catalog::Code::current&&catalog::same(*result,*initial));
    // Shared RB body ownership is observed rather than rewritten to the array
    // owner; this is still not authorization to filter that actor as self.
    put(cylinder+0x1a0,weapon_body);C(capture_catalog(*result)==catalog::Code::current&&result->components[0].owner_roots==1&&
        result->components[0].body_owner_roots==2&&result->components[0].body_owner.address==weapon_mesh);
    put(instance_prop+0x94,0x1b0u);put(cylinder+0x1b0,single_body);put(weapon_mesh+0x1b0,weapon_body);
    put(cylinder+0x1a0,uintptr_t{});put(weapon_mesh+0x1a0,uintptr_t{});
    C(capture_catalog(*result)==catalog::Code::current&&result->components[0].body_instance_offset==0x1b0&&result->components[0].body.identity.address==single_body);
    for(auto root:{fixture_pawn,fixture_weapon})for(unsigned n=0;n<2;++n)put(root+0x500+n*16,roles::pose::ArrayHeader{});
    put(fixture_pawn+0x500,roles::pose::ArrayHeader{arena+0xd0000,0,4});
    C(capture_catalog(*result)==catalog::Code::current&&!result->component_count&&!result->pawn_mesh_listed&&
        !result->components[0].identity.address&&result->components[0].body_state==catalog::BodyState::not_sampled);
    catalog_setup();put(fixture_pawn+0x1d0,uintptr_t{});C(h::sample(&scene,actor));
    C(capture_catalog(*result)==catalog::Code::current&&result->root_count==1&&result->components[3].owner_roots==0&&!result->roots[1].actor.address);
    for(unsigned bad=0;bad<21;++bad){catalog_setup();*result=*initial;
        if(bad==0)put(fixture_weapon+0x188,fixture_pc);
        if(bad==1)put(array_inners[0]+0x50,fixture_object_property);
        if(bad==2)put(array_inners[0]+0x74,4u);
        if(bad==3)put(array_inners[0]+0xd0,primitive_class);
        if(bad==4)put(array_inners[0]+0x40,fixture_actor);
        if(bad==5)put(fixture_pawn+0x500,roles::pose::ArrayHeader{arena+0xd0000,65,65});
        if(bad==6)put(fixture_pawn+0x500,roles::pose::ArrayHeader{0,1,1});
        if(bad==7)put(fixture_pawn+0x500,roles::pose::ArrayHeader{arena+0xd0001,4,4});
        if(bad==8)put(fixture_pawn+0x510,roles::pose::ArrayHeader{arena+0xd1000,4,3});
        if(bad==9)put(arena+0xd0000,fixture_weapon);
        if(bad==10)put(audio+0x170,uintptr_t{});
        if(bad==11)put(weapon_body+0x68,fixture_weapon);
        if(bad==12)put(single_body+0x88,arena+0xb0001);
        if(bad==13)denied=arena+0xd0000;
        if(bad==14)denied=weapon_body+0x88;
        if(bad==15)put(array_props[1]+0x68,array_props[0]);
        if(bad==16)object(audio,172,audio_class,fixture_pawn,L"Default__AudioComponent");
        if(bad==17)put(array_props[0]+0x74,8u);
        if(bad==18)put(array_props[1]+0x94,0x500u);
        if(bad==19)put(instance_prop+0x94,0x170u);
        if(bad==20)put(props[5]+0x94,0x88u);
        C(capture_catalog(*result)!=catalog::Code::current&&catalog::same(*result,*initial));
    }
    for(unsigned change=0;change<5;++change){catalog_setup();*result=*initial;
        if(change==0)plan(single_body+0x88,1,fixture_pawn+0x508,3,4);
        if(change==1)plan(single_body+0x88,1,arena+0xd0000,audio,8);
        if(change==2)plan(single_body+0x88,2,audio+0x170,fixture_pawn,8);
        if(change==3){plan(single_body+0x88,1,0,0,0);change_generation=true;}
        if(change==4){plan(single_body+0x88,1,0,0,0);throw_read=true;}
        C(catalog::sample(actor,guard,catalog_work,*result)!=catalog::Code::current&&catalog::same(*result,*initial));read_hook=nullptr;
        C(h::mutex.try_lock());h::mutex.unlock();
    }
    catalog_setup();
    for(unsigned n=0;n<128;++n){const auto at=arena+0xe0000+n*0x200;object(at,800+n,audio_class,fixture_pawn,L"BoundedComponent",n);put(at+0x170,fixture_pawn);
        put(arena+0xd0000+(n/64)*0x1000+(n%64)*8,at);}
    for(unsigned n=0;n<2;++n)put(fixture_pawn+0x500+n*16,roles::pose::ArrayHeader{arena+0xd0000+n*0x1000,64,256});
    put(fixture_weapon+0x500,roles::pose::ArrayHeader{});put(fixture_weapon+0x510,roles::pose::ArrayHeader{});
    C(capture_catalog(*result)==catalog::Code::current&&result->component_count==128&&!result->pawn_mesh_listed&&result->components[127].identity.address==arena+0xefe00);
    const auto full=std::make_unique<catalog::Snapshot>(*result);
    object(arena+0xf0000,928,audio_class,fixture_weapon,L"OverflowComponent");put(arena+0xf0170,fixture_weapon);put(arena+0xd2000,arena+0xf0000);
    put(fixture_weapon+0x500,roles::pose::ArrayHeader{arena+0xd2000,1,1});
    C(capture_catalog(*result)==catalog::Code::capacity&&catalog::same(*result,*full));
    catalog_setup();*result=*initial;
    for(unsigned n=0;n<9;++n){const auto cls=arena+0xf2000+n*0x400,at=arena+0xe0000+n*0x200;
        klass(cls,600+n,L"DerivedActorComponent",component_base);object(at,800+n,cls,fixture_pawn,L"DerivedComponent",n);put(at+0x170,fixture_pawn);put(arena+0xd0000+n*8,at);}
    put(fixture_pawn+0x500,roles::pose::ArrayHeader{arena+0xd0000,9,9});put(fixture_pawn+0x510,roles::pose::ArrayHeader{});
    put(fixture_weapon+0x500,roles::pose::ArrayHeader{});put(fixture_weapon+0x510,roles::pose::ArrayHeader{});
    C(capture_catalog(*result)==catalog::Code::capacity&&catalog::same(*result,*initial));
    catalog_setup();*result=*initial;
    C(catalog::sample(actor,[](){return false;},catalog_work,*result)==catalog::Code::guard&&catalog::same(*result,*initial));
    unsigned last{};C(catalog::sample(actor,[&](){return ++last<2;},catalog_work,*result)==catalog::Code::guard&&catalog::same(*result,*initial));
    C(catalog::sample(actor,guard,catalog_work,catalog_work.first)==catalog::Code::request);
    auto invalid=actor;invalid.weapon_known=false;unsigned calls{};const auto before=reads;
    C(catalog::sample(invalid,[&](){++calls;return true;},catalog_work,*result)==catalog::Code::request&&!calls&&reads==before);
    h::mutex.lock();C(catalog::sample(actor,[](){return true;},catalog_work,*result)==catalog::Code::metadata);h::mutex.unlock();
    catalog::Code other{};std::thread worker([&](){other=catalog::sample(actor,[](){return true;},catalog_work,*result);});worker.join();
    C(other==catalog::Code::metadata&&catalog::same(*result,*initial));
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverComponentRoles checks=%u failures=%u declared Pawn/current Weapon arrays only; no complete self or actor/geometry proof\n",checks,failures);return failures?1:0;
}
#else
int GEARS_COMPONENT_ROLES_TEST_MAIN(){catalog::Snapshot result{};result.component_count=99;catalog::Workspace work{};unsigned calls{};
    C(catalog::sample(h::Binding{},[&](){++calls;return true;},work,result)==catalog::Code::unavailable&&result.component_count==99&&!calls&&!reads&&!native_calls);
    printf("CoverComponentRoles OFF checks=%u failures=%u no reads/guard/calls\n",checks,failures);return failures?1:0;}
#endif
