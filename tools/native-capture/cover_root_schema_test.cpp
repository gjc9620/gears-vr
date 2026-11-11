// Run the actual component reader, then compare the root schema selection and
// full closing witness. No game, engine query or alternate body writer.
#define GEARS_COMPONENT_ROLES_TEST_MAIN PriorRootComponentMain
#include "cover_component_roles_live_test.cpp"
#undef GEARS_COMPONENT_ROLES_TEST_MAIN
#ifndef GEARS_ROOT_BATCH
#define GEARS_ROOT_BATCH 1
#endif
static bool select_root(catalog::Workspace& w,catalog::RootLayout& l,uintptr_t cls){
#if GEARS_ROOT_BATCH
    return catalog::root_schema(w,l,cls);
#else
    auto& p=l.proof;
    return roles::role_property(p,cls,L"WorldInfo",L"ObjectProperty",8,L"WorldInfo",l.fields[0],&w.members)&&
        roles::role_property(p,cls,L"Components",L"ArrayProperty",16,nullptr,l.fields[1],&w.members)&&
        roles::role_property(p,cls,L"AllComponents",L"ArrayProperty",16,nullptr,l.fields[2],&w.members)&&
        roles::role_property(p,cls,L"Base",L"ObjectProperty",8,L"Actor",l.fields[3],&w.members)&&
        roles::role_property(p,cls,L"Attached",L"ArrayProperty",16,nullptr,l.fields[4],&w.members)&&
        catalog::actor_array_schema(p,l.fields[4]);
#endif
}
static void reset_catalog_work(){auto clean=std::make_unique<catalog::Workspace>();catalog_work=*clean;}
int main(){
    if(PriorRootComponentMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    catalog_setup();reset_catalog_work();auto& l=catalog_work.roots[0];
    const auto before=reads;
    C(select_root(catalog_work,l,actor.pawn.cls));
    C(h::current(l.proof)&&catalog_work.members.current());
    printf("root_schema reads=%llu\n",static_cast<unsigned long long>(reads-before));
    C(l.fields[0].offset==0x188&&l.fields[1].offset==0x500&&l.fields[2].offset==0x510&&l.fields[3].offset==0x520&&l.fields[4].offset==0x530);
    // A second actor's schema may share the full Actor chain; descriptors,
    // current head, parent and extent still belong to its own closing proof.
    C(select_root(catalog_work,catalog_work.roots[1],actor.weapon.cls));
    C(h::current(catalog_work.roots[1].proof)&&catalog_work.members.current());
    for(unsigned bad=0;bad<9;++bad){
        catalog_setup();reset_catalog_work();auto& first=catalog_work.roots[0];
        C(select_root(catalog_work,first,actor.pawn.cls));
        if(bad==0)put(array_props[0]+0x94,0x540u);
        if(bad==1)put(array_props[0]+0x68,uintptr_t{});
        if(bad==2)put(fixture_actor+0x80,array_props[0]);
        if(bad==3)put(array_props[0]+0x74,8u);
        if(bad==4)put(array_props[0]+0x40,fixture_pawn);
        if(bad==5)put(fixture_actor+0x88,0x100u);
        if(bad==6)put(actor.pawn.cls+h::metadata.field_super,uintptr_t{});
        if(bad==7)put(array_props[1]+0x50,fixture_object_property);
        if(bad==8)put(instigator_property+72,get<unsigned>(array_props[0]+72));
        C(!h::current(first.proof)||!catalog_work.members.current());
    }
    // Duplicates, malformed metadata and owner cycles must refuse selection.
    for(unsigned bad=0;bad<6;++bad){
        catalog_setup();reset_catalog_work();
        if(bad==0)put(array_props[0]+0x68,array_props[0]);
        if(bad==1)put(array_props[0]+0x70,2u);
        if(bad==2)put(array_props[0]+0x94,0x10000u);
        if(bad==3)put(array_props[0]+0x50,fixture_object_property);
        if(bad==4)put(array_props[0]+0x40,fixture_pawn);
        if(bad==5){
            typed_property(arena+0xf0000,1000,fixture_actor,L"Components",array_kind,16,0x540);
            put(arena+0xc4800+0x68,arena+0xf0000);
        }
        C(!select_root(catalog_work,catalog_work.roots[0],actor.pawn.cls));
    }
    // Later nonmatching siblings must also close. Exercise a long real chain,
    // then rename its last member after selection to create a late duplicate.
    catalog_setup();reset_catalog_work();
    for(unsigned n=0;n<128;++n){
        const auto at=arena+0xe0000+n*0x200;
        object(at,1850+n,integer_kind,fixture_actor,L"UnrelatedRootMember",n);
        put(at+0x68,n==127?uintptr_t{}:at+0x200);
    }
    put(arena+0xc4800+0x68,arena+0xe0000);
    const auto long_before=reads;
    C(select_root(catalog_work,catalog_work.roots[0],actor.pawn.cls));
    C(h::current(catalog_work.roots[0].proof)&&catalog_work.members.current());
    printf("root_schema long_reads=%llu\n",static_cast<unsigned long long>(reads-long_before));
    put(arena+0xe0000+127*0x200+72,get<unsigned>(array_props[0]+72));
    C(!catalog_work.members.current());
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverRootSchema checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
