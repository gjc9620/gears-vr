#define GEARS_COVER_CONTEXT_TEST_MAIN PriorLinkContextMain
#include "cover_context_live_test.cpp"
#undef GEARS_COVER_CONTEXT_TEST_MAIN
#ifndef GEARS_COVER_LINK_PATH_PROOF
#define GEARS_COVER_LINK_PATH_PROOF 1
#endif
static void link_depth(unsigned depth){
    auto parent=link_class;
    for(unsigned n=0;n<depth;++n){const auto cls=arena+0x90000+n*0x200;
        klass(cls,200+n,L"DerivedCover",parent);parent=cls;
    }
    put(link_instance+0x50,parent);
}
int main(){
    if(PriorLinkContextMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    for(unsigned depth:{0u,4u,12u}){
        cover_setup();put(fixture_pawn+0x298,static_cast<unsigned char>(2));link_depth(depth);
        context::Observation observation{};C(observe(observation));context::CurrentKey key{};
        C(context::sample_current(observation.actor,key));const auto first=key;const auto before=reads;
        for(unsigned n=0;n<5;++n)C(context::sample_current(observation.actor,key)&&context::same(first,key));
        printf("cover_link_path depth=%u five_reads=%u\n",depth,reads-before);
    }
    // Changes present at a new call must never use a stale ready proof.
    for(unsigned bad=0;bad<9;++bad){
        cover_setup();put(fixture_pawn+0x298,static_cast<unsigned char>(2));link_depth(4);
        context::Observation observation{};C(observe(observation));context::CurrentKey key{};key.slot=999;
        if(bad==0)put(arena+0x90200+0x60,uintptr_t{});
        if(bad==1)put(arena+0x90200+0x50,fixture_object_property);
        if(bad==2)put(arena+0x90200+8,700u);
        if(bad==3)put(link_instance+0x50,fixture_actor);
        if(bad==4)put(link_class+0x60,link_class);
        if(bad==5)put(cover_properties[4]+0x74,8u);
        if(bad==6)put(inner+0x74,48u);
        if(bad==7)++committed_world_generation;
        if(bad==8)recording=false;
        C(!context::sample_current(observation.actor,key)&&key.slot==999);
    }
    // The prior full derivation was not in the final property proof. Mutate
    // its last ancestor AFTER the third derivation, at the last slot read.
    cover_setup();put(fixture_pawn+0x298,static_cast<unsigned char>(2));
    context::Observation observation{};C(observe(observation));context::CurrentKey key{};key.slot=999;
    plan(arena+0x80000,3,fixture_actor+0x60,link_class,8);
    const bool accepted=context::sample_current(observation.actor,key);read_hook=nullptr;
#if GEARS_COVER_LINK_PATH_PROOF
    C(!accepted&&key.slot==999);
#else
    C(accepted);printf("baseline_unclosed_ancestor=1\n");
#endif
    C(native_calls==0);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverLinkPathProof checks=%u failures=%u real context reader; no game/write\n",checks,failures);return failures?1:0;
}
