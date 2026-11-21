#pragma once
#define GEARS_WEAPON_METADATA_PARSE_TEST_STATS 1
#include "weapon_hand_offline_fixture.h"
namespace bow_fields {namespace meta=gripdamage_metadata;
#include "weapon_torque_bow_observe_fields.inl"
}
namespace {
using B=bow_fields::BowObserveFields;
const wchar_t* labels[]={L"bPendingEndFire",L"InvManager",L"CurrentFireMode",L"PSC_TorqueBowChargeUp",L"AimingSound",L"PSC_Trail",L"PSC_FailedTrail",L"EM_EndPoint",L"EM_EndPointFullCharge",L"EM_EndPointFail",L"SimulatedGrenade"};
struct Fixture {std::unique_ptr<Memory> m=std::make_unique<Memory>();uptr byte{};std::array<uptr,11> properties{};
 Fixture(){byte=m->node(L"ByteProperty",m->class_type,m->core);
 for(unsigned i=0;i<11;++i)properties[i]=m->property(m->concrete,labels[i],i==0?m->bool_type:i==2?byte:i>=3&&i<=6?m->component_type:m->object_type,0x130+i*8,i==0?4:i==2?1:8,i==0?2:0);m->tables();}
};
struct Batch {meta::Snapshot schema{};meta::Reader r{schema,meta::Stage::final_check};B b;
 Batch(){schema.module_base=base;assert(meta::tables(base,schema.objects,schema.names));}
};
bool original(Fixture& f){meta::Snapshot s{};s.module_base=base;meta::Reader r{s,meta::Stage::final_check};if(!meta::tables(base,s.objects,s.names))return false;
 for(unsigned i=0;i<11;++i){weaponhand::BoolField v{};bool ok=false;
 // Runtime label array is intentionally dispatched to the unchanged typed selector through fixed-size literals.
 if(i==0)ok=weaponhand::field(r,f.m->weapon,L"bPendingEndFire",L"BoolProperty",4,v);
 if(i==1)ok=weaponhand::field(r,f.m->weapon,L"InvManager",L"ObjectProperty",8,v);
 if(i==2)ok=weaponhand::field(r,f.m->weapon,L"CurrentFireMode",L"ByteProperty",1,v);
 if(i==3)ok=weaponhand::field(r,f.m->weapon,L"PSC_TorqueBowChargeUp",L"ComponentProperty",8,v);
 if(i==4)ok=weaponhand::field(r,f.m->weapon,L"AimingSound",L"ComponentProperty",8,v);
 if(i==5)ok=weaponhand::field(r,f.m->weapon,L"PSC_Trail",L"ComponentProperty",8,v);
 if(i==6)ok=weaponhand::field(r,f.m->weapon,L"PSC_FailedTrail",L"ComponentProperty",8,v);
 if(i==7)ok=weaponhand::field(r,f.m->weapon,L"EM_EndPoint",L"ObjectProperty",8,v);
 if(i==8)ok=weaponhand::field(r,f.m->weapon,L"EM_EndPointFullCharge",L"ObjectProperty",8,v);
 if(i==9)ok=weaponhand::field(r,f.m->weapon,L"EM_EndPointFail",L"ObjectProperty",8,v);
 if(i==10)ok=weaponhand::field(r,f.m->weapon,L"SimulatedGrenade",L"ObjectProperty",8,v);
 if(!ok||s.identities[v.property.identity].address!=f.properties[i])return false;}return true;
}
}
