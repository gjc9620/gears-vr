#define GEARS_PHYSICS_ROLES_TEST_MAIN PriorStructFieldMain
#include "cover_physics_roles_live_test.cpp"
#undef GEARS_PHYSICS_ROLES_TEST_MAIN
#include "cover_struct_field_live.inl"
int main(){arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 physics_setup();const auto type=arena+0xa0000,float_type=arena+0xa0400,first=arena+0xa0800,tail=arena+0xa0c00;
 klass(float_type,180,L"FloatProperty",0);object(type,181,script_kind,0,L"KSphereElem");put(type+0x88,72u);
 typed_property(first,182,type,L"Radius",float_type,4,64);typed_property(tail,183,type,L"Other",float_type,4,0);put(type+0x80,first);put(first+0x68,tail);
 gears_anchor::Identity identity{};C(h::identity(type,identity));auto proof=std::make_unique<h::Proof>();h::Property out{};
 C(cover_struct_field_live::select(*proof,identity,72,L"Radius",L"FloatProperty",4,64,nullptr,out)&&out.id.address==first&&out.offset==64);
 put(tail+0x68,first);C(!h::current(*proof));put(tail+0x68,uintptr_t{});
 for(unsigned bad=0;bad<6;++bad){*proof={};out.id.address=123;
  if(bad==0)put(first+0x74,8u);if(bad==1)put(first+0x94,68u);if(bad==2)put(type+0x88,80u);
  if(bad==3)put(tail+0x68,first);if(bad==4)put(first+0x70,2u);if(bad==5)put(tail+0x40,fixture_pawn);
  C(!cover_struct_field_live::select(*proof,identity,72,L"Radius",L"FloatProperty",4,64,nullptr,out)&&out.id.address==123);
  put(first+0x74,4u);put(first+0x94,64u);put(type+0x88,72u);put(tail+0x68,uintptr_t{});put(first+0x70,1u);put(tail+0x40,type);
 }
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("CoverStructField checks=%u failures=%u actual reflection helpers with synthetic registry; no native authorization\n",checks,failures);return failures?1:0;}
