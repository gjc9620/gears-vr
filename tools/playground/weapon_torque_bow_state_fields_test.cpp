#include "weapon_torque_bow_fields_fixture.h"
namespace unified_weapon_fire {
#include "r51-original-fingerprint.inl"
}
namespace bow_values {
namespace meta=gripdamage_metadata;using bow_fields::BowObserveFields;
wchar_t const* failed_field=L"";
#include "weapon_torque_bow_state_fields.inl"
}
namespace {
struct Values {Batch batch;bow_values::Fields fields{batch.r};explicit Values(Fixture& f){assert(fields.weapon.begin(batch.r,f.m->weapon));}};
}
int main(){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
 {Fixture f;assert(original(f));Memory::put(f.m->weapon,0x130,0x82u);Values v(f);bool flag{};
  assert(v.fields.flag(f.m->weapon,L"bPendingEndFire",flag,B::pending_end)&&flag);
  weaponhand::BoolField old{};assert(weaponhand::boolean(v.batch.r,f.m->weapon,L"bPendingEndFire",old));
  unified_weapon_fire::Fingerprint expected{};expected.word(f.m->weapon);expected.word(old.property.offset);expected.word(old.mask);expected.word(old.bits&old.mask);assert(expected.value==v.fields.hash.value);assert(v.fields.weapon.finish(v.batch.r));}
 for(unsigned mask:{0u,3u}){Fixture f;Memory::put(f.properties[0],0xd0,mask);Values v(f);bool flag{};assert(!v.fields.flag(f.m->weapon,L"bPendingEndFire",flag,B::pending_end));}
 {Fixture f;Values a(f);bool bit{};assert(a.fields.flag(f.m->weapon,L"bPendingEndFire",bit,B::pending_end)&&!bit);const auto h=a.fields.hash.value;
  Memory::put(f.m->weapon,0x130,0x80u);Values b(f);assert(b.fields.flag(f.m->weapon,L"bPendingEndFire",bit,B::pending_end)&&!bit&&b.fields.hash.value==h);
  Memory::put(f.m->weapon,0x130,2u);Values c(f);assert(c.fields.flag(f.m->weapon,L"bPendingEndFire",bit,B::pending_end)&&bit&&c.fields.hash.value!=h);}
 {Fixture f;Values v(f);bool bit{};assert(!v.fields.flag(f.m->pawn,L"bPendingEndFire",bit,B::pending_end));}
 {Fixture f;Memory::put(f.m->weapon,0x138,f.m->pc);Values a(f);uptr p{};
  assert(a.fields.pointer(f.m->weapon,L"InvManager",p,false,B::inv_manager)&&p==f.m->pc);
  meta::PointerField old{};assert(weaponhand::read_pointer(a.batch.r,f.m->weapon,L"InvManager",old));unified_weapon_fire::Fingerprint expected{};expected.word(f.m->weapon);expected.word(old.property.offset);expected.word(old.value);assert(expected.value==a.fields.hash.value);
  Memory::put(f.m->weapon,0x138,f.m->pawn);assert(a.fields.pointer(f.m->weapon,L"InvManager",p,false,B::inv_manager)&&p==f.m->pawn);assert(a.fields.weapon.finish(a.batch.r));}
 {Fixture f;Memory::put(f.m->weapon,0x148,f.m->mesh);Values v(f);uptr p{};assert(v.fields.pointer(f.m->weapon,L"PSC_TorqueBowChargeUp",p,true,B::charge_fx)&&p==f.m->mesh);assert(!v.fields.pointer(f.m->weapon,L"PSC_TorqueBowChargeUp",p,false,B::charge_fx));assert(!v.fields.pointer(f.m->weapon,L"InvManager",p,true,B::inv_manager));}
 {Fixture f;Values v(f);uptr p{};assert(!v.fields.pointer(f.m->pawn,L"InvManager",p,false,B::inv_manager));assert(!v.fields.pointer(f.m->weapon,L"InvManager",p,false,B::fire_mode));}
 {Fixture f;Values v(f);Memory::put(f.properties[1],0x94,0x1e0u);uptr p{};assert(!v.fields.pointer(f.m->weapon,L"InvManager",p,false,B::inv_manager));}
 {Fixture f;Values v(f);fail_read=f.m->weapon+0x138;uptr p{};assert(!v.fields.pointer(f.m->weapon,L"InvManager",p,false,B::inv_manager));fail_read=0;}
 {Fixture f;Values v(f);Memory::put(f.properties[0],0x50,f.m->object_type);bool p{};assert(!v.fields.flag(f.m->weapon,L"bPendingEndFire",p,B::pending_end));}
 {Fixture f;Values v(f);bool p{};assert(v.fields.flag(f.m->pawn,L"bNoWeaponFiring",p)&&!p);uptr ptr{};assert(v.fields.pointer(f.m->pawn,L"Weapon",ptr)&&ptr==f.m->weapon);assert(v.fields.weapon.finish(v.batch.r));}
 std::puts("Production Bow Fields: masked flag/fingerprint agreement, zero/multibit rejection, fresh value changes, pointer/hash agreement, component kind, wrong receiver/index, descriptor/type mutation, unreadable and unchanged nonweapon fallback passed.");
}
