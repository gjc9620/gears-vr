// Exercise real readers with existing synthetic-memory fixtures, never game.
#define GEARS_COMPONENT_ROLES_TEST_MAIN RootRepeatPriorMain
#include "cover_component_roles_live_test.cpp"
#undef GEARS_COMPONENT_ROLES_TEST_MAIN
int main(){
    if(RootRepeatPriorMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto result=std::make_unique<catalog::Snapshot>();
    for(unsigned root_count:{2u,8u,16u}){
        catalog_setup();const auto array=arena+0xda000;
        for(unsigned n=2;n<root_count;++n){const auto at=arena+0xe0000+n*0x800;
            object(at,1000+n,fixture_actor,fixture_world,L"AttachedActor",n);
            put(at+0x188,fixture_world);put(array+(n-2)*8,at);}
        if(root_count>2)put(fixture_pawn+0x530,roles::pose::ArrayHeader{array,int(root_count-2),int(root_count-2)});
        const auto before=reads;const auto observed_code=capture_catalog(*result);
        C(observed_code==catalog::Code::current&&result->root_count==root_count);
        printf("root_repeat roots=%u reads=%u\n",root_count,reads-before);
    }
    for(unsigned pass:{1u,2u})for(unsigned bad=0;bad<8;++bad){
        catalog_setup();C(capture_catalog(*result)==catalog::Code::current);const auto initial=std::make_unique<catalog::Snapshot>(*result);
        const auto occurrence=pass==1?1u:3u; // source_field performs two real reads per dynamic pass.
        if(bad==0)plan(fixture_pawn+0x500,occurrence,array_props[0]+0x94,0x540u,4);
        if(bad==1)plan(fixture_pawn+0x500,occurrence,array_props[1]+80,fixture_object_property,8);
        if(bad==2)plan(fixture_pawn+0x500,occurrence,array_inners[0]+0xd0,fixture_actor,8);
        if(bad==3)plan(fixture_pawn+0x500,occurrence,fixture_pawn_class+0x60,fixture_weapon_base,8);
        if(bad==4)plan(fixture_pawn+0x500,occurrence,array_props[0]+0x68,uintptr_t{},8);
        if(bad==5)plan(fixture_pawn+0x500,occurrence,array_inners[1]+0x74,4u,4);
        if(bad==6){plan(fixture_pawn+0x500,occurrence,0,0,0);change_generation=true;}
        if(bad==7)plan(fixture_pawn+0x500,occurrence,fixture_pawn+0x188,fixture_weapon,8);
        C(catalog::sample(actor,guard,catalog_work,*result)!=catalog::Code::current&&catalog::same(*result,*initial));read_hook=nullptr;
    }
    // Attached graph growth between dynamic passes remains a refused change.
    catalog_setup();C(capture_catalog(*result)==catalog::Code::current);const auto initial=std::make_unique<catalog::Snapshot>(*result);
    const auto attached=arena+0xe4000;object(attached,780,fixture_actor,fixture_world,L"LateActor");put(attached+0x188,fixture_world);
    put(arena+0xda000,attached);put(fixture_pawn+0x530,roles::pose::ArrayHeader{arena+0xda000,0,1});
    plan(fixture_pawn+0x500,3,fixture_pawn+0x538,1u,4);
    const auto late_code=catalog::sample(actor,guard,catalog_work,*result);
    C(late_code!=catalog::Code::current&&catalog::same(*result,*initial));read_hook=nullptr;
    C(native_calls==0&&guard_lock_free);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverComponentRootRepeat checks=%u failures=%u includes prior component fixture; no game\n",checks,failures);
    return failures?1:0;
}
