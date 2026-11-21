#define GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL 1
#include "../vr-common/weapon_main_mount_provider.h"
#include "weapon_hand_offline_fixture.h"
#include <limits>
namespace {
#include "weapon_lancer_asset_read.inl"
#include "weapon_torque_bow_arrow_source.inl"
#include "weapon_torque_bow_guide_source.inl"
struct ProgressMemory:Memory {
    uptr bow{},float_type{},count_property{},max_property{};
    ProgressMemory(){
        bow=cls(L"Weap_Bow_base",warfare,weapon_class);put(weapon,0x50,bow);float_type=cls(L"FloatProperty",core);
        count_property=property(bow,L"ChargeTimeCount",float_type,0x180,4);max_property=property(bow,L"MaxChargeTime",float_type,0x184,4);
        put(weapon,0x180,.75f);put(weapon,0x184,1.5f);frame.weapon=identity(weapon);frame.key.thread_id=GetCurrentThreadId();tables();
    }
};
}
int main(){
    auto memory=std::make_unique<ProgressMemory>();auto& m=*memory;namespace source=torquebow_guide_source;source::Progress p{};
    assert(source::progress(m.frame,p)&&p.count==.75&&p.maximum==1.5);
    Memory::put(m.weapon,0x180,2.f);assert(source::progress(m.frame,p)&&p.count==2); // fatigue tail is not clamped or written.
    for(float bad:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        Memory::put(m.weapon,0x180,bad);assert(!source::progress(m.frame,p));
    }
    Memory::put(m.weapon,0x180,.75f);
    for(float bad:{0.f,-1.f,10001.f,std::numeric_limits<float>::quiet_NaN()}){Memory::put(m.weapon,0x184,bad);assert(!source::progress(m.frame,p));}
    Memory::put(m.weapon,0x184,1.5f);
    Memory::put(m.count_property,0x94,0x184u);assert(!source::progress(m.frame,p));Memory::put(m.count_property,0x94,0x180u);
    Memory::put(m.bow,0x40,m.engine);assert(!source::progress(m.frame,p));Memory::put(m.bow,0x40,m.warfare);
    Memory::put(m.count_property,0x74,8u);assert(!source::progress(m.frame,p));Memory::put(m.count_property,0x74,4u);
    mutate_read=m.weapon+0x180;mutation=[&]{Memory::put(m.weapon,0x180,1.f);};assert(!source::progress(m.frame,p));
    assert(source::progress(m.frame,p)&&p.count==1&&Memory::get<float>(m.weapon,0x184)==1.5f);
    std::puts("Torque Bow typed original charge progress readback passed; owned memory, no timer write or game/Engine execution.");
}
