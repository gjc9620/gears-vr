#include "weapon_torque_bow_fields_fixture.h"
int main(){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
 {Fixture f;assert(original(f));Batch a;assert(a.b.begin(a.r,f.m->weapon));for(unsigned i=0;i<11;++i){weaponhand::BoolField v{};assert(a.b.binding(a.r,static_cast<B::Index>(i),v)&&a.schema.identities[v.property.identity].address==f.properties[i]);}assert(a.b.finish(a.r));weaponhand::BoolField v{};assert(!a.b.binding(a.r,B::pending_end,v));}
 {Fixture f;Memory::put(f.properties[3],0x50,f.m->object_type);assert(!original(f));Batch a;assert(!a.b.begin(a.r,f.m->weapon));}
 {Fixture f;Memory::put(f.properties[2],0x74,4u);assert(!original(f));Batch a;assert(!a.b.begin(a.r,f.m->weapon));}
 {Fixture f;Batch a;assert(a.b.begin(a.r,f.m->weapon));Memory::put(f.properties[4],0x94,0x1d0u);weaponhand::BoolField v{};assert(!a.b.binding(a.r,B::audio,v));assert(!a.b.finish(a.r));}
 {Fixture f;Batch a;assert(a.b.begin(a.r,f.m->weapon));f.m->property(f.m->concrete,L"AimingSound",f.m->object_type,0x1d0,8);f.m->tables();assert(!a.b.finish(a.r));}
 {Fixture f;f.m->property(f.m->concrete,L"AimingSound",f.m->object_type,0x1d0,8);f.m->tables();assert(original(f));Batch a;assert(a.b.begin(a.r,f.m->weapon)&&a.b.finish(a.r));}
 {Fixture f;const auto derived=f.m->cls(L"DerivedBow",f.m->warfare,f.m->concrete);f.m->property(derived,L"InvManager",f.m->bool_type,0x1d0,4,1);Memory::put(f.m->weapon,0x50,derived);f.m->tables();Batch a;assert(!a.b.begin(a.r,f.m->weapon));}
 {Fixture f;Batch a;assert(a.b.begin(a.r,f.m->weapon));Memory::put(f.m->weapon,0x50,f.m->weapon_class);weaponhand::BoolField v{};assert(!a.b.binding(a.r,B::audio,v)&&!a.b.finish(a.r));}
 {Fixture f;fail_read=f.properties[0]+0x68;Batch a;assert(!a.b.begin(a.r,f.m->weapon));fail_read=0;}
 {Fixture f;Batch a;assert(a.b.begin(a.r,f.m->weapon));Memory::put(f.properties[0],0x68,f.properties[0]);assert(!a.b.finish(a.r));}
 {Fixture f;std::uint64_t old{},grouped{};read_observer=[&](uptr,std::size_t){++old;};assert(original(f)&&original(f));read_observer=[&](uptr,std::size_t){++grouped;};Batch a;assert(a.b.begin(a.r,f.m->weapon));for(unsigned i=0;i<11;++i){weaponhand::BoolField v{};assert(a.b.binding(a.r,static_cast<B::Index>(i),v));}assert(a.b.finish(a.r));read_observer={};std::printf("Own fixture read calls: original two selections=%llu, grouped begin/current/finish=%llu; not game FPS proof.\n",old,grouped);assert(grouped<old);}
 std::puts("Bow query-local eleven-field descriptors: original selection agreement, exact types/widths, shadow rejection, later duplicate, mutation, incarnation, unreadable/cycle and consumed binding passed.");
}