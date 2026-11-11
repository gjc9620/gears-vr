#define GEARS_COMPONENT_ROLES_TEST_MAIN PriorComponentRecheckMain
#include "cover_component_roles_live_test.cpp"
#undef GEARS_COMPONENT_ROLES_TEST_MAIN
int main(){
    if(PriorComponentRecheckMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto baseline=std::make_unique<catalog::Snapshot>();auto observed=std::make_unique<catalog::Snapshot>();
    catalog_setup();C(catalog::sample(actor,guard,catalog_work,*baseline)==catalog::Code::current);
    const auto recheck_begin=reads;
    for(unsigned n=0;n<5;++n)C(catalog::recheck(actor,guard,catalog_work,*observed)==catalog::Code::current&&catalog::same(*observed,*baseline));
    const auto recheck_reads=reads-recheck_begin;
    const auto sample_begin=reads;
    for(unsigned n=0;n<5;++n)C(catalog::sample(actor,guard,catalog_work,*observed)==catalog::Code::current&&catalog::same(*observed,*baseline));
    printf("component_recheck five_sample_reads=%u five_recheck_reads=%u\n",reads-sample_begin,recheck_reads);
    // Every accepted initial capture belongs to this exact operation. Mutated
    // values/schema/identity and unreadable data cannot publish partial output.
    for(unsigned bad=0;bad<11;++bad){
        catalog_setup();C(catalog::sample(actor,guard,catalog_work,*baseline)==catalog::Code::current);
        *observed=*baseline;observed->component_count=99;const auto untouched=std::make_unique<catalog::Snapshot>(*observed);
        if(bad==0)put(fixture_pawn+0x508,3);
        if(bad==1)put(arena+0xd0000,audio);
        if(bad==2)put(single_body+0x88,arena+0xb8000);
        if(bad==3)put(array_props[0]+0x94,0x520u);
        if(bad==4)++committed_world_generation;
        if(bad==5)++actor.scene_invocation;
        if(bad==6)++actor.special_move;
        if(bad==7)put(array_inners[0]+0x74,4u);
        if(bad==8)denied=weapon_body+0x88;
        if(bad==9)put(fixture_weapon+0x188,fixture_pc);
        if(bad==10)put(fixture_pawn+0x500,roles::pose::ArrayHeader{arena+0xd1000,4,4});
        C(catalog::recheck(actor,guard,catalog_work,*observed)!=catalog::Code::current);
        C(catalog::same(*observed,*untouched)&&!catalog_work.recheck_ready);
    }
    // Both Owner boundaries must consume readiness on rejection; restoring
    // permission alone cannot revive the failed operation's schema grant.
    for(unsigned boundary=1;boundary<=2;++boundary){
        catalog_setup();C(catalog::sample(actor,guard,catalog_work,*baseline)==catalog::Code::current);
        unsigned owner_calls=0;*observed=*baseline;
        C(catalog::recheck(actor,[&](){return ++owner_calls!=boundary&&guard();},catalog_work,*observed)==catalog::Code::guard);
        C(!catalog_work.recheck_ready&&catalog::same(*observed,*baseline));
        C(catalog::recheck(actor,guard,catalog_work,*observed)!=catalog::Code::current);
        C(catalog::sample(actor,guard,catalog_work,*baseline)==catalog::Code::current);
        C(catalog::recheck(actor,guard,catalog_work,*observed)==catalog::Code::current);
    }
    // Changes while the actual reader is running must fail its closing checks.
    for(unsigned during=0;during<3;++during){
        catalog_setup();C(catalog::sample(actor,guard,catalog_work,*baseline)==catalog::Code::current);*observed=*baseline;
        if(during==0)plan(single_body+0x88,1,arena+0xd0000,audio,8);
        if(during==1){plan(single_body+0x88,1,0,0,0);change_generation=true;}
        if(during==2){plan(single_body+0x88,1,0,0,0);throw_read=true;}
        C(catalog::recheck(actor,guard,catalog_work,*observed)!=catalog::Code::current);
        C(!catalog_work.recheck_ready&&catalog::same(*observed,*baseline));read_hook=nullptr;
        C(h::mutex.try_lock());h::mutex.unlock();
    }
    C(native_calls==0);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("ComponentRecheck checks=%u failures=%u actual production reader; no game\n",checks,failures);return failures?1:0;
}
