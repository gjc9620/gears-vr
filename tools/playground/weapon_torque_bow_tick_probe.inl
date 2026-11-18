// Core's probe is native object state, not a reflected IsProbing function.
// Read the exact bit manipulated by original Enable/Disable. A missing state
// frame is unknown and rejects; no guessed class fallback or probe write.
#include "weapon_torque_bow_probe_bits.h"
#include "weapon_torque_bow_probe_contract.inl"
namespace torquebow_tick_probe {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
namespace bits=gears_torque_bow_probe_bits;
bool read(uptr actor,bool& out) noexcept {
 struct Probe {uptr actor;unsigned gate{1};~Probe() noexcept {
  static unsigned rows{};if(gate&&log_file&&rows++<2){fwprintf(log_file,L"torquebow_native_tick_probe_rejected actor=%llx gate=%u observation_only=1\n",actor,gate);fflush(log_file);}
 }} probe{actor};
 out=false;if(!weaponhand::current_gt(nullptr)||!torquebow_probe_contract::current())return false;
 meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
 hand::ObjectIdentity identity{},tick_function{};unsigned tick=UINT_MAX;meta::NameText tick_text{};
 probe.gate=2;if(!meta::tables(base,schema.objects,schema.names)||!lancermain_asset::identify(r,actor,identity))return false;
 probe.gate=3;
 // Select the receiver's actual inherited Tick event; unrelated predefined
 // probe slots may be empty and are not part of this read's authority.
 playground::Args event(actor,L"Tick");unsigned item{},kind{};
 if(!event.valid||!lancermain_asset::identify(r,event.r.function,tick_function)||tick_function.name_number||
    !r.path(event.r.function,item)||!r.path(schema.identities[item].klass,kind)||
    !weaponhand::core_type(r,kind,L"Function"))return false;
 tick=tick_function.name_index;
 if(tick<300||tick>363||!meta::fresh_name(schema.names,tick,tick_text)||!meta::exact_text(tick_text,L"Tick"))return false;
 bits::Sample first{},last{};
 probe.gate=4;if(!bits::read(r,actor,tick,first)||
    !weaponhand::derives(r,first.state,L"State",L"Core")||
    !unified_weapon_fire::retained_metadata(r)||!weaponhand::object(r,identity)||
    !bits::read(r,actor,tick,last)||!bits::same(first,last))return false;
 meta::Table objects{},names{};meta::NameText final_name{};
 playground::Args final_event(actor,L"Tick");
 probe.gate=5;if(!meta::tables(base,objects,names)||!meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names)||
    !final_event.valid||final_event.r.function!=tick_function.address||!weaponhand::object(r,tick_function)||
    !meta::fresh_name(names,tick,final_name)||!meta::same_name_text(tick_text,final_name)||
    !torquebow_probe_contract::current()||!weaponhand::current_gt(nullptr))return false;
 out=bits::enabled(first,tick);probe.gate=0;
 static unsigned rows{};if(log_file&&rows++<4){fwprintf(log_file,L"torquebow_native_tick_probe actor=%llx state_frame=%llx state_node=%llx tick_name=%u mask=%llx enabled=%u observation_only=1\n",actor,first.frame,first.state,tick,first.mask,unsigned(out));fflush(log_file);}
 return true;
}
}
