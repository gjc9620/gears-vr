// Real typed roles and registry reader, synthetic registered class members.
#define GEARS_PHYSICS_ROLES_TEST_MAIN PriorMemberRolesMain
#include "cover_physics_roles_live_test.cpp"
#undef GEARS_PHYSICS_ROLES_TEST_MAIN
static uintptr_t add_mesh_siblings(unsigned count){
    const auto objects=get<gears_anchor::Registry>(b::metadata.objects_header);
    const auto names=get<gears_anchor::Registry>(b::metadata.names_header);
    for(unsigned i=0;i<count;++i){const auto address=arena+0xa0000+uintptr_t(i)*0x100;
        const unsigned index=300+i;const auto entry=arena+0xd0000+uintptr_t(i)*0x100;
        gears_anchor::Identity id{};id.address=address;id.index=id.name_index=index;id.cls=fixture_object_property;id.outer=fixture_mesh_class;
        swprintf_s(id.name.data(),id.name.size(),L"MeshSibling%u",i);
        put(address+8,index);put(address+64,id.outer);put(address+72,index);put(address+76,0u);put(address+80,id.cls);
        put(address+0x68,i+1<count?address+0x100:uintptr_t{});
        put(objects.data+uintptr_t(index)*8,address);put(names.data+uintptr_t(index)*8,entry);put(entry,index);put(entry+20,id.name);
    }
    put(props[1]+0x68,count?arena+0xa0000:uintptr_t{});
    return count?arena+0xa0000+uintptr_t(count-1)*0x100:0;
}
#ifndef GEARS_MEMBER_WITNESS_TEST_MAIN
#define GEARS_MEMBER_WITNESS_TEST_MAIN main
#endif
int GEARS_MEMBER_WITNESS_TEST_MAIN(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    physics_setup();roles::Snapshot out{};const auto tail=add_mesh_siblings(192);
    C(observe(out));const auto accepted=out;
    const auto witness=roles::observation(workspace);
    C(witness.step==roles::LayoutStep::complete&&witness.pass==2&&witness.member_identities==201&&!witness.member_capacity);
    C(witness.mesh_identities<128&&witness.mesh_asset_offset==0x248&&workspace.members.current());
    // The public/shared Proof was not enlarged or given another success path.
    static h::Proof legacy;legacy={};h::Property selected{};
    C(h::property(legacy,fixture_mesh_class,L"PhysicsAsset",L"ObjectProperty",8,L"PhysicsAsset",selected));
    C(!roles::membership(legacy,selected)&&legacy.identity_count==128&&legacy.identities.size()==128);
    for(unsigned bad=0;bad<7;++bad){physics_setup();const auto last=add_mesh_siblings(192);out=accepted;
        if(bad==0)put(last+0x68,arena+0xa0000); // Cycle after the selected fields.
        if(bad==1)put(last+72,get<unsigned>(props[0]+72)); // Duplicate selected name at the tail.
        if(bad==2)put(last+64,instance_class); // Wrong declaring owner.
        if(bad==3)plan(body_instances[1]+0x88,2,last+80,fixture_component_property,8); // Closing identity.
        if(bad==4)plan(body_instances[1]+0x88,2,last+0x68,props[0],8); // Closing link.
        if(bad==5){const auto objects=get<gears_anchor::Registry>(b::metadata.objects_header);
            plan(body_instances[1]+0x88,2,objects.data+uintptr_t(491)*8,fixture_weapon,8);}
        if(bad==6)denied=last+0x68;
        if(bad>=3&&bad<=5){role_verdict=roles::sample(actor,physics_expected,guard,workspace,out);C(role_verdict!=roles::Code::current&&roles::same(out,accepted));read_hook=nullptr;}
        else C(!observe(out)&&roles::same(out,accepted));
    }
    physics_setup();add_mesh_siblings(192);C(observe(out));
    // Small instantiation exercises the same bounded hash/collision/exhaustion
    // code with actual registered nodes; production remains bounded at4096.
    cover_member_witness_live::Storage<2> small_store{};
    gears_anchor::Identity first{},second{},third{};uintptr_t next{};
    C(h::identity(props[0],first)&&h::identity(props[1],second)&&h::identity(tail,third));
    C(small_store.link(first,next)&&small_store.link(second,next)&&small_store.current());
    C(!small_store.link(third,next)&&small_store.exhausted&&small_store.count==2&&!small_store.current());
    C(!small_store.link(third,next)&&small_store.count==2);small_store.clear();
    C(small_store.count==0&&!small_store.exhausted&&small_store.link(third,next)&&small_store.current());
    put(tail+0x68,props[0]);C(!small_store.current());
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverMemberWitness after repair checks=%u failures=%u actual typed reader; synthetic members, no game\n",checks,failures);return failures?1:0;
}
