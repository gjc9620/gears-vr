// Existing actual registry/identity/property helpers with synthetic memory.
// No game/process/XR, original query, Native write or alternate reader model.
#define main CoverPhysicsRolesBindingFixtureMain
#include "weapon_hand_scene_binding_test.cpp"
#undef main
#include "cover_physics_roles_live.inl"
namespace roles=cover_physics_roles_live;
#if GEARS_COVER_BODY_PROTOTYPE
static roles::Workspace workspace;
static uintptr_t asset_class{},instance_class{},body_class{},integer_kind{},array_kind{},structure_kind{},script_kind{},pointer_type{};
static uintptr_t physics_asset{},physics_instance{},body_instances[2]{},props[9]{},inner{};
static h::Binding actor{};static roles::pose::Metadata physics_expected{};
static void typed_property(uintptr_t at,unsigned index,uintptr_t owner,wchar_t const* name,uintptr_t kind,unsigned width,unsigned offset,uintptr_t target=0){
    property(at,index,owner,name,offset,target);put(at+80,kind);put(at+0x74,width);
}
static void physics_setup(){
    setup();asset_class=arena+0x72000;instance_class=arena+0x72400;body_class=arena+0x72800;
    integer_kind=arena+0x72c00;array_kind=arena+0x73000;structure_kind=arena+0x73400;script_kind=arena+0x73800;pointer_type=arena+0x73c00;
    klass(asset_class,80,L"PhysicsAsset",0);klass(instance_class,81,L"PhysicsAssetInstance",0);klass(body_class,82,L"RB_BodyInstance",0);
    klass(integer_kind,83,L"IntProperty",0);klass(array_kind,84,L"ArrayProperty",0);klass(structure_kind,85,L"StructProperty",0);klass(script_kind,86,L"ScriptStruct",0);
    object(pointer_type,87,script_kind,0,L"Pointer");put(pointer_type+0x88,8u);
    physics_asset=arena+0x48000;physics_instance=arena+0x4a000;body_instances[0]=arena+0x4c000;body_instances[1]=arena+0x4e000;
    object(physics_asset,88,asset_class,fixture_pawn,L"CurrentPhysicsAsset");object(physics_instance,89,instance_class,fixture_pawn,L"CurrentPhysicsInstance");
    for(unsigned i=0;i<2;++i)object(body_instances[i],90+i,body_class,physics_instance,i?L"Body1":L"Body0");
    wchar_t const* names[]={L"PhysicsAsset",L"PhysicsAssetInstance",L"Owner",L"Bodies",L"RootBodyIndex",L"BodyIndex",L"OwnerComponent",L"BodyData",L"BoneSpring"};
    const unsigned offsets[]={0x248,0x250,0x60,0x6c,0x7c,0x60,0x68,0x88,0x90};
    const uintptr_t kinds[]={fixture_object_property,fixture_object_property,fixture_object_property,array_kind,integer_kind,integer_kind,fixture_component_property,structure_kind,structure_kind};
    const uintptr_t targets[]={asset_class,instance_class,fixture_actor,0,0,0,fixture_mesh_class,pointer_type,pointer_type};
    for(unsigned i=0;i<9;++i){props[i]=arena+0x78000+uintptr_t(i)*0x400;
        typed_property(props[i],100+i,i<2?fixture_mesh_class:(i<5?instance_class:body_class),names[i],kinds[i],i==3?16u:(i==4||i==5?4u:8u),offsets[i],targets[i]);
        put(props[i]+0x68,i==1||i==4||i==8?uintptr_t{}:props[i]+0x400);}
    put(fixture_mesh_class+0x80,props[0]);put(instance_class+0x80,props[2]);put(body_class+0x80,props[5]);
    inner=arena+0x7a800;typed_property(inner,110,props[3],L"Bodies",fixture_object_property,8,0,body_class);put(props[3]+0xd0,inner);
    put(fixture_mesh+0x248,physics_asset);put(fixture_mesh+0x250,physics_instance);put(physics_instance+0x60,fixture_pawn);
    put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,2,2});put(physics_instance+0x7c,1);
    for(unsigned i=0;i<2;++i){put(arena+0x86000+i*8,body_instances[i]);put(body_instances[i]+0x60,int(i));put(body_instances[i]+0x68,fixture_mesh);
        put(body_instances[i]+0x88,arena+0x88000+i*0x1000);put(body_instances[i]+0x90,uintptr_t{});}
    constexpr uintptr_t physx=0x180000000;
    put(arena+0x89000,physx+0x2330a8);put(arena+0x89038,arena+0x8b000);
    put(arena+0x8b000,physx+0x22ccb8);put(arena+0x8b270,arena+0x8c000);
    put(arena+0x8c000,physx+0x23ca98);put(arena+0x8c6b0,arena+0x8b000);
    actor={};C(h::sample(&scene,actor));physics_expected={};physics_expected.physics_asset=physics_asset;physics_expected.physics_instance=physics_instance;
}
static bool guard_lock_free=true;static unsigned guards{};static roles::Code role_verdict{};
static bool guard(){++guards;if(!h::mutex.try_lock()){guard_lock_free=false;return false;}h::mutex.unlock();
    h::Binding current{};return h::current_tick_actor(actor,current)&&h::same_key(actor,current);}
static bool observe(roles::Snapshot& out){
    memcpy(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size());
    role_verdict=roles::sample(actor,physics_expected,guard,workspace,out);
    C(!memcmp(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size()));C(!native_calls&&guard_lock_free);
    return role_verdict==roles::Code::current;
}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    physics_setup();roles::Snapshot out{};C(observe(out));C(out.root==1&&out.source.body_index==1&&out.source.mesh==fixture_mesh&&out.source.thread==GetCurrentThreadId());
    C(out.bodies[0].index==0&&out.bodies[1].index==1&&out.bodies[1].main==arena+0x89000&&out.owner.address==fixture_pawn);
    const auto accepted=out;C(observe(out)&&roles::same(accepted,out));
    constexpr uintptr_t physx=0x180000000;
    const std::array<std::pair<unsigned,unsigned>,10> slots{{
        {0x2330b0,0x135b00},{0x22ccc0,0xa0300},{0x23cda0,0x171770},{0x23cda8,0x1716f0},{0x23cdc8,0x168350},
        {0x23cdd0,0x1682d0},{0x22cf40,0xa6e30},{0x22cf48,0xa6aa0},{0x22cf20,0xa6290},{0x22cf28,0xa60b0}}};
    auto source_read=[&](void* dst,void const* address,size_t n){
        for(auto const& slot:slots)if(reinterpret_cast<uintptr_t>(address)==physx+slot.first&&n==8){const auto target=physx+slot.second;memcpy(dst,&target,8);return true;}
        return safe_copy(dst,address,n);};
    roles::query_source::Snapshot raw{};
    C(roles::query_source::capture(physx,0x300000,out.source,source_read,guard,raw)==roles::query_source::Code::current);
    C(roles::matches_query_source(out,raw));
    {auto wrong=raw;wrong.members[0]=wrong.body;C(!roles::matches_query_source(out,wrong));}
    {auto wrong=raw;wrong.actor=arena+0x88000;C(!roles::matches_query_source(out,wrong));}
    {auto wrong=raw;wrong.source.body_index=0;C(!roles::matches_query_source(out,wrong));}
    {auto wrong=raw;wrong.bodies.data+=8;C(!roles::matches_query_source(out,wrong));}
    // The original reflected Owner may refer to the actual Pawn or Mesh; its
    // declared class must match, rather than guessing one universal offset/type.
    put(physics_instance+0x60,fixture_mesh);put(props[2]+0xd0,fixture_mesh_class);C(observe(out)&&out.owner.address==fixture_mesh);
    for(unsigned bad=0;bad<27;++bad){physics_setup();out={};out.root=999;const auto before=out;
        if(bad==0)put(fixture_mesh+0x248,fixture_weapon);
        if(bad==1)put(fixture_mesh+0x250,fixture_weapon);
        if(bad==2)put(physics_instance+0x60,fixture_pc);
        if(bad==3)put(physics_instance+0x7c,-1);
        if(bad==4)put(physics_instance+0x7c,2);
        if(bad==5)put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,65,65});
        if(bad==6)put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,2,1});
        if(bad==7)put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86001,2,2});
        if(bad==8)put(arena+0x86008,body_instances[0]);
        if(bad==9)put(arena+0x86008,fixture_weapon);
        if(bad==10)put(body_instances[0]+0x60,1);
        if(bad==11)put(body_instances[1]+0x68,fixture_pc);
        if(bad==12)put(body_instances[1]+0x88,uintptr_t{});
        if(bad==13)put(props[0]+0x94,0x240u);
        if(bad==14)put(props[3]+0x94,0x68u);
        if(bad==15)put(inner+0x74,4u);
        if(bad==16)put(inner+0x94,4u);
        if(bad==17)put(inner+64,instance_class);
        if(bad==18)put(props[7]+0x94,0x80u);
        if(bad==19)put(pointer_type+0x88,16u);
        if(bad==20)put(props[6]+80,fixture_object_property);
        if(bad==21)put(props[6]+0xd0,fixture_weapon_base);
        if(bad==22)put(props[2]+0xd0,fixture_weapon_base);
        if(bad==23)put(body_instances[1]+0x90,arena+0x88001);
        if(bad==24)put(props[8]+0x68,props[5]);
        if(bad==25){typed_property(arena+0x7ac00,111,body_class,L"BodyIndex",integer_kind,4,0x60);put(props[8]+0x68,arena+0x7ac00);}
        if(bad==26)denied=arena+0x86000;
        C(!observe(out)&&roles::same(before,out));
    }
    physics_setup();
    for(unsigned i=0;i<64;++i){const uintptr_t body=arena+0xa0000+i*0x100;
        object(body,150+i,body_class,physics_instance,L"BoundedBody",i);
        put(arena+0x86000+i*8,body);put(body+0x60,int(i));put(body+0x68,fixture_mesh);put(body+0x88,arena+0xb0000+i*0x100);}
    put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,64,256});put(physics_instance+0x7c,63);
    C(observe(out)&&out.source.body_index==63&&out.bodies[63].index==63);
    put(arena+0xa0000+62*0x100+0x68,fixture_pc);C(!observe(out)); // Non-root members are not skipped.
    physics_setup();
    for(unsigned i=0;i<9;++i){const auto cls=arena+0xc0000+i*0x400,body=arena+0xa0000+i*0x100;
        klass(cls,170+i,L"DerivedRB_BodyInstance",body_class);object(body,190+i,cls,physics_instance,L"DerivedBody",i);
        put(arena+0x86000+i*8,body);put(body+0x60,int(i));put(body+0x68,fixture_mesh);put(body+0x88,arena+0xb0000+i*0x100);}
    put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,9,9});put(physics_instance+0x7c,0);
    C(!observe(out)&&role_verdict==roles::Code::capacity);
    // In-place schema/data/identity changes during the first read cannot be
    // hidden by a valid final class name or an unchanged object pointer.
    for(unsigned bad=0;bad<9;++bad){physics_setup();C(observe(out));const auto before=out;
        if(bad==0)plan(body_instances[1]+0x88,1,physics_instance+0x7c,0,4);
        if(bad==1)plan(body_instances[1]+0x88,1,body_instances[0]+0x68,fixture_pc,8);
        if(bad==2)plan(body_instances[1]+0x88,1,inner+0x74,4,4);
        if(bad==3)plan(body_instances[1]+0x88,1,props[8]+0x68,props[5],8);
        if(bad==4){plan(body_instances[1]+0x88,1,0,0,0);change_generation=true;}
        if(bad==5){plan(body_instances[1]+0x88,1,0,0,0);change_thread=true;}
        if(bad==6){plan(body_instances[1]+0x88,1,0,0,0);throw_read=true;}
        if(bad==7)plan(body_instances[1]+0x88,1,arena+0x86008,body_instances[0],8);
        if(bad==8)plan(body_instances[1]+0x88,1,body_class+0x60,fixture_weapon_base,8);
        // This intentional adversarial fixture mutates memory; test only the
        // reader's role_verdict/output preservation, not observe()'s no-write check.
        role_verdict=roles::sample(actor,physics_expected,guard,workspace,out);
        C(role_verdict!=roles::Code::current&&roles::same(before,out));read_hook=nullptr;C(h::mutex.try_lock());h::mutex.unlock();
    }
    physics_setup();C(observe(out));const auto before=out;
    C(roles::sample(actor,physics_expected,[](){return false;},workspace,out)==roles::Code::guard&&roles::same(before,out));
    unsigned closing{};C(roles::sample(actor,physics_expected,[&](){return ++closing<2;},workspace,out)==roles::Code::guard&&roles::same(before,out));
    C(roles::sample(actor,physics_expected,guard,workspace,workspace.first)==roles::Code::request);
    C(roles::sample(actor,physics_expected,[]()->bool{throw 7;},workspace,out)==roles::Code::exception&&roles::same(before,out));
    C(guards>2&&guard_lock_free&&native_calls==0&&!b::enabled&&!b::layout_ready);
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("cover_physics_roles_live: %u checks, %u failures; typed source only, no query/writer\n",checks,failures);return failures?1:0;
}
#else
int main(){roles::Snapshot out{};out.root=999;roles::Workspace w{};unsigned calls{};
    C(roles::sample(h::Binding{},roles::pose::Metadata{},[&](){++calls;return true;},w,out)==roles::Code::unavailable);
    C(out.root==999&&calls==0&&reads==0&&native_calls==0);
    printf("cover_physics_roles_live OFF: %u checks, %u failures; no reads/guard/calls\n",checks,failures);return failures?1:0;
}
#endif
