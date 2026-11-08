// Existing reflected memory/Scene fixture; no process, XR or engine call.
#define GEARS_RUN_CAMERA_TEST_MAIN CoverContextExistingFixtureMain
#include "run_camera_live_test.cpp"
#include "cover_context_live.inl"
namespace context=cover_context_live;
#if GEARS_COVER_BODY_PROTOTYPE
uintptr_t link_class{},link_instance{},slot_type{},inner{},array_kind{};
std::array<uintptr_t,7> cover_properties{};
void cover_setup(bool wrong_last_action_name=false){
    live_setup();context::pawn_proof={};context::link_proof={};context::metadata_generation=0;
    link_class=arena+0x72000;link_instance=arena+0x48000;slot_type=arena+0x72400;
    array_kind=arena+0x72800;klass(link_class,80,L"CoverLink",fixture_actor);klass(array_kind,81,L"ArrayProperty",0);
    object(slot_type,82,script_struct,0,L"CoverSlot");put(slot_type+0x88,212u);
    object(link_instance,83,link_class,arena+0x49000,L"SyntheticCover");
    const wchar_t* names[]={L"CurrentLink",L"CurrentSlotIdx",L"CoverAction",
        wrong_last_action_name?L"LastCoverAction":L"LastCoverActionAnim",L"Slots"};
    const unsigned offsets[]={0x300,0x308,0x30c,0x30d,0x340};
    const uintptr_t kinds[]={fixture_object_property,int_class,fixture_byte_property,fixture_byte_property,array_kind};
    const unsigned widths[]={8,4,1,1,16};
    for(unsigned i=0;i<5;++i){auto addr=arena+0x74000+uintptr_t(i)*0x400;cover_properties[i]=addr;
        pfield(addr,84+i,i==4?link_class:fixture_pawn_class,names[i],kinds[i],widths[i],offsets[i],i==0?link_class:0);
        put(addr+0x68,i<3?addr+0x400:uintptr_t(0));}
    put(new_properties[7]+0x68,cover_properties[0]);put(link_class+0x80,cover_properties[4]);
    inner=arena+0x75800;property(inner,89,cover_properties[4],L"Slots",0,slot_type);
    put(inner+80,struct_class);put(inner+0x74,212u);put(cover_properties[4]+0xd0,inner);
    put(fixture_pawn+0x300,link_instance);put(fixture_pawn+0x308,0);put(fixture_pawn+0x30c,static_cast<unsigned char>(0));
    put(fixture_pawn+0x30d,static_cast<unsigned char>(0));put(link_instance+0x188,fixture_world);
    put(link_instance+0x340,context::Array{arena+0x80000,2,2});
    memset(reinterpret_cast<void*>(arena+0x80000),0x5a,424);
}
bool observe(context::Observation& out){
    memcpy(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size());
    const auto result=context::sample(&scene,out);
    C(!memcmp(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size()));
    C(!b::enabled&&!b::layout_ready&&native_calls==0);return result;
}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    cover_setup();context::Observation out{};C(observe(out));C(!out.cover_type&&!out.link.address&&out.slot==-1);
    // Independent of legacy CoverBody, even though its gates are deliberately bad.
    for(unsigned type:{1u,2u}){cover_setup();put(fixture_pawn+0x298,static_cast<unsigned char>(type));C(observe(out));
        C(out.cover_type==type&&out.link.address==link_instance&&out.slot==0&&out.scene==scene.invocation);
        auto again=out;C(observe(again)&&context::same(out,again));
        put(fixture_pawn+0x30c,static_cast<unsigned char>(7));put(fixture_pawn+0x294,0x84u);C(observe(again));
        C(again.cover_action==7&&again.transitioning&&!context::same(out,again));
        put(fixture_pawn+0x30d,static_cast<unsigned char>(3));C(observe(again)&&again.last_action==3);}
    // The shipped animation state is LastCoverActionAnim. A similarly named
    // synthetic property must not make the fixture validate the wrong reader.
    cover_setup(true);out={};out.scene=999;C(!observe(out)&&out.scene==999);
    for(unsigned bad=0;bad<12;++bad){cover_setup();put(fixture_pawn+0x298,static_cast<unsigned char>(2));out={};out.scene=999;
        if(bad==0)put(fixture_pawn+0x308,-1);
        if(bad==1)put(fixture_pawn+0x308,2);
        if(bad==2)put(link_instance+0x188,fixture_pawn);
        if(bad==3)put(link_instance+0x340,context::Array{arena+0x80000,2,1});
        if(bad==4)put(link_instance+0x340,context::Array{arena+0x80001,2,2});
        if(bad==5)put(inner+0x74,48u);
        if(bad==6)put(inner+0x94,4u);
        if(bad==7)put(slot_type+0x88,48u);
        if(bad==8)put(new_properties[6]+0xd0,3u);
        if(bad==9)scene.thread++;
        if(bad==10)recording=false;
        if(bad==11)denied=arena+0x80000;
        C(!observe(out)&&out.scene==999);}
    // Mutations after initial values, including an otherwise identical slot,
    // cannot escape the closing bracket or modify caller output on failure.
    for(unsigned bad=0;bad<6;++bad){cover_setup();put(fixture_pawn+0x298,static_cast<unsigned char>(2));C(observe(out));
        if(bad==0)plan(fixture_pawn+0x308,2,fixture_pawn+0x308,1,4);
        if(bad==1)plan(fixture_pawn+0x308,2,arena+0x80000,0,4);
        if(bad==2)plan(fixture_pawn+0x308,2,inner+0x94,4,4);
        if(bad==3){plan(fixture_pawn+0x308,2,0,0,0);change_scene=true;}
        if(bad==4){plan(fixture_pawn+0x308,2,0,0,0);change_generation=true;}
        if(bad==5)plan(fixture_pawn+0x308,2,fixture_pawn+0x30d,1,1);
        const auto before=out;C(!context::sample(&scene,out)&&!memcmp(&before,&out,sizeof out));read_hook=nullptr;
        C(h::mutex.try_lock());h::mutex.unlock();}
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("cover_context_live: %u checks, %u failures; legacy body disabled, no hooks/writes\n",checks,failures);return failures?1:0;
}
#else
int main(){context::Observation out{};out.scene=999;C(!context::sample(nullptr,out)&&out.scene==999);return failures?1:0;}
#endif
