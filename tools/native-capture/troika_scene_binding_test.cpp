// Reuse the existing sparse-memory/config loader, without executing its suite.
#define main mounted_fixture_unused_main
#include "mounted_observer_test.cpp"
#undef main
#undef CHECK
#define GEARS_COVER_BODY_PROTOTYPE 1
#include <atomic>
#include <mutex>
std::atomic<uint64_t> committed_world_generation{1};std::atomic<bool> recording{true};
gears_anchor::Config anchor_config;Fixture* fixture{};
void (*read_change)(uintptr_t,size_t){};uintptr_t denied{};
unsigned memory_reads{};
bool safe_copy(void* out,void const* from,size_t size){
    ++memory_reads;
    auto address=reinterpret_cast<uintptr_t>(from);if(read_change)read_change(address,size);
    return address!=denied&&fixture&&fixture->read(out,from,size);
}
#include "weapon_hand_scene_binding.inl"
#include "troika_scene_binding.inl"
namespace t=troika_scene_binding;namespace o=weapon_hand_scene_binding;
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::printf("failed %d: %s field=%ls parts=%u/%u ready=%u weapon=%u/%u\n",__LINE__,#x,o::rejected_field,t::parts_proof.identity_count,t::parts_proof.source_count,unsigned(t::parts_proof.ready),t::weapon_proof.identity_count,t::weapon_proof.source_count);return 1;}}while(0)
gears_anchor::Receipt fixture_scene;
bool race_done{};
void race(uintptr_t address,size_t){
    if(!race_done&&address==0x80800){race_done=true;fixture->put(0x20810,uintptr_t(0));}
}
int troika_scene_binding_tests(){
    Fixture f;fixture=&f;CHECK(f.load(L"troika"));anchor_config=f.anchor;
    gears_anchor::Anchor anchor{};uint32_t stamp{};
    CHECK(gears_anchor::guard(anchor_config,safe_copy,anchor,stamp));
    fixture_scene.anchor=anchor;fixture_scene.invocation=17;fixture_scene.thread=GetCurrentThreadId();
    fixture_scene.scene_qpc=1;fixture_scene.family_qpc=2;fixture_scene.renderer_qpc=3;
    t::Binding value{};const bool initial=t::sample(&fixture_scene,value);
    if(!initial){for(unsigned i=0;i<t::parts_proof.source_count;++i)std::printf("source %u %llx size%u\n",i,static_cast<unsigned long long>(t::parts_proof.sources[i].address),t::parts_proof.sources[i].size);
        uintptr_t child{};o::field(child,0x3000,0x80);for(unsigned i=0;i<8&&child;++i){gears_anchor::Identity id{};const bool ok=o::identity(child,id,false,true);std::printf("child %llx identity%u outer%llx name%ls\n",static_cast<unsigned long long>(child),unsigned(ok),static_cast<unsigned long long>(id.outer),id.name.data());o::field(child,child,0x68);}}
    CHECK(initial);
    CHECK(value.turret.address==0x80000&&value.driver.address==0x20000);
    CHECK(value.turret_mesh.address!=value.driver_mesh.address&&value.weapon.address);
    CHECK(value.world_generation==1&&value.scene_invocation==17&&value.scene_qpc==1);
    o::Binding ordinary{};CHECK(!o::sample(&fixture_scene,ordinary)); // WarPawn guard stays strict.
    const auto memory=f.memory;const auto source=fixture_scene;const auto expected=value;
    for(unsigned fault=0;fault<12;++fault){
        f.memory=memory;fixture_scene=source;recording=true;committed_world_generation=1;denied=0;
        if(fault==0)f.put(0x20810,uintptr_t(0));
        if(fault==1)f.put(0x20250,uintptr_t(0x10000));
        if(fault==2)f.put(0x80800,uintptr_t(0));
        if(fault==3)f.put(0x10224,uintptr_t(0x20000));
        if(fault==4)f.put(0x80850,expected.driver_mesh.address);
        if(fault==5)f.put(expected.weapon.address+0x220,uintptr_t(0x20000));
        if(fault==6)f.put(expected.driver_mesh.address+8,999u);
        if(fault==7)fixture_scene.thread++;
        if(fault==8)fixture_scene.invocation=0;
        if(fault==9)recording=false;
        if(fault==10)denied=anchor_config.objects_header;
        if(fault==11)fixture_scene.renderer_qpc=0;
        value=expected;CHECK(!t::sample(&fixture_scene,value));CHECK(t::same(value,expected));
    }
    f.memory=memory;fixture_scene=source;recording=true;denied=0;read_change=race;race_done=false;
    CHECK(!t::sample(&fixture_scene,value));CHECK(race_done);read_change=nullptr;
    f.memory=memory;fixture_scene=source;CHECK(t::sample(&fixture_scene,value));
    CHECK(o::game_thread.load()==0); // Mounted observation never forges normal admission.
    std::printf("Troika separate mounted Scene binding: %u checks passed; synthetic only.\n",checks);
    return 0;
}
#ifndef TROIKA_SCENE_BINDING_FIXTURE_ONLY
int main(){return troika_scene_binding_tests();}
#endif
