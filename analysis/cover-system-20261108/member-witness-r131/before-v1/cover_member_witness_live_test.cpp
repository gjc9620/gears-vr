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
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    physics_setup();roles::Snapshot out{};out.root=999;const auto before=out;
    add_mesh_siblings(192);
    C(!observe(out)&&roles::same(out,before));
    const auto witness=roles::observation(workspace);
    C(role_verdict==roles::Code::layout&&witness.step==roles::LayoutStep::mesh_asset_property&&witness.mesh_identities==128);
    C(witness.mesh_asset_offset==0x248&&witness.pass==1);
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverMemberWitness before repair checks=%u failures=%u actual typed reader; synthetic members, no game\n",checks,failures);return failures?1:0;
}
