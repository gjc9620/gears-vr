// The actual new current_actor reader, on the existing synthetic registry.
#define main PriorSceneBindingTestMain
#include "weapon_hand_scene_binding_test.cpp"
#undef main
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    h::Binding owner{};setup();C(take(owner));C(h::current_actor(owner));
    // SpecialMove is the certified BYTE field, not the following three bytes.
    put(fixture_pawn+0x192,0xffffffu);C(h::current_actor(owner));
    for(unsigned mode=0;mode<8;++mode){setup();C(take(owner));
        if(mode==0)put(fixture_pawn+0x180,fixture_weapon);
        if(mode==1)put(fixture_pc+0x100,uintptr_t(0));
        if(mode==2)put(fixture_pawn+0x188,fixture_weapon);
        if(mode==3)put(fixture_mesh+76,2u);
        if(mode==4)committed_world_generation=2;
        if(mode==5)put(fixture_pawn+0x191,static_cast<unsigned char>(7));
        if(mode==6)put(fixture_pawn+0x1d0,fixture_weapon2);
        if(mode==7)put(fixture_weapon+0x218,fixture_pc);
        C(!h::current_actor(owner));
    }
    setup();C(take(owner));plan(fixture_pawn+0x180,2,fixture_pawn+0x180,fixture_weapon,8);C(!h::current_actor(owner));read_hook=nullptr;
    setup();C(take(owner));plan(fixture_pawn+0x180,1,0,0,0);throw_read=true;C(!h::current_actor(owner));read_hook=nullptr;throw_read=false;
    C(h::mutex.try_lock());h::mutex.unlock();C(native_calls==0);
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("continuous_actor: %u checks %u failures; actual reader, synthetic registry, native_calls=0\n",checks,failures);return failures?1:0;
}
