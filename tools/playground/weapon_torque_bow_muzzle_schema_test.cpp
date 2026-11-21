#include <cstdio>
FILE* log_file{};
#define GEARS_WEAPON_METADATA_PARSE_TEST_STATS 1
#include "weapon_hand_offline_fixture.h"
#include <cmath>
namespace {
namespace playground {
struct Vector {float x{},y{},z{};};
uptr trace_function{};unsigned trace_bytes{};bool invoke_ok=true;unsigned invoke_count{};
std::function<void(unsigned char*)> trace;
// Owned request storage and an artificial original-call endpoint only.
struct Args {
    struct Request {uptr function{};unsigned size{};unsigned char params[4096]{};} r;
    bool valid{};
    Args(uptr,wchar_t const*) {r.function=trace_function;r.size=trace_bytes;valid=bool(r.function);}
    bool run(){++invoke_count;if(trace)trace(r.params);return invoke_ok;}
};
}
namespace unified_weapon_fire {
namespace metadata=gripdamage_metadata;using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
struct MuzzleSample {abi::Frame source{};playground::Vector origin{};};
// Same bounded ancestry helper as weapon_unified_fire; no game-call permission.
bool owner_in_receiver(metadata::Reader& r,uptr receiver,uptr owner) noexcept {
    U32 object{};if(!r.path(receiver,object))return false;
    auto cls=r.result.identities[object].klass;std::array<uptr,metadata::max_parents> seen{};unsigned count{};
    while(cls){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==cls)return false;
        seen[count++]=cls;metadata::Structure structure{};
        if(!r.structure(cls,structure))return false;
        if(cls==owner)return true;cls=structure.super;
    }
    return false;
}
}
namespace nativeequipment {
unsigned checks{},fail_check{};
bool current(void*,abi::Frame const&){return ++checks!=fail_check;}
}
#include "weapon_torque_bow_muzzle_clear.inl"
namespace muzzle=torquebow_muzzle_clear;
void fake_native_endpoint() {}
struct TraceFixture:Memory {
    uptr function{},function_type{},struct_type{},script_type{},vector{},rotator{},location{};
    std::array<uptr,5> fields{};
    std::array<unsigned,5> offsets{0,12,24,36,40};unsigned bytes=44;
    TraceFixture(){
        function_type=node(L"Function",class_type,core);struct_type=node(L"StructProperty",class_type,core);
        script_type=node(L"ScriptStruct",class_type,core);const auto object=cls(L"Object",core);
        vector=node(L"Vector",script_type,object);rotator=node(L"Rotator",script_type,object);
        put(vector,0x88,12u);put(rotator,0x88,12u);
        function=node(L"FastTrace",function_type,actor_class);put(function,0x88,bytes);
        put(function,0xf4,0x401u);put(function,0xf8,std::uint16_t(548));
        put(function,0x106,std::uint16_t(bytes));put(function,0x108,std::uint16_t(offsets[4]));
        put(function,0x114,reinterpret_cast<uptr>(&fake_native_endpoint));
        const wchar_t* parameter_labels[]={L"TraceEnd",L"TraceStart",L"BoxExtent",L"bTraceBullet",L"ReturnValue"};
        for(unsigned i=0;i<5;++i){
            fields[i]=property(function,parameter_labels[i],i<3?struct_type:bool_type,offsets[i],i<3?12:4,i<3?0:1);
            put(fields[i],0x78,std::uint64_t(i==4?0x580:i==0?0x80:0x90));
            if(i<3)put(fields[i],0xd0,vector);
        }
        location=property(actor_class,L"Location",struct_type,0x140,12);put(location,0xd0,vector);
        put(pawn,0x140,playground::Vector{1,2,3});tables();
        playground::trace_function=function;playground::trace_bytes=bytes;playground::invoke_ok=true;
        playground::invoke_count=0;nativeequipment::checks=nativeequipment::fail_check=0;
        playground::trace=[&](unsigned char* params){
            playground::Vector start{},end{},extent{};unsigned bullet{},result_before{};
            std::memcpy(&end,params+offsets[0],12);std::memcpy(&start,params+offsets[1],12);
            std::memcpy(&extent,params+offsets[2],12);std::memcpy(&bullet,params+offsets[3],4);
            std::memcpy(&result_before,params+offsets[4],4);
            assert(end.x==4&&end.y==5&&end.z==6&&start.x==1&&start.y==2&&start.z==3);
            assert(extent.x==0&&extent.y==0&&extent.z==0&&bullet==1&&result_before==0);
            const unsigned clear=1;std::memcpy(params+offsets[4],&clear,4);
        };
    }
    ~TraceFixture(){playground::trace={};playground::trace_function=0;}
    bool inspect(muzzle::Layout& out,muzzle::SchemaObservation& observation){
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};
        if(!meta::tables(base,snapshot.objects,snapshot.names))return false;
        playground::Args call(pawn,L"FastTrace");
        return muzzle::schema(call,pawn,reader,out,&observation)&&unified_weapon_fire::retained_metadata(reader);
    }
    bool query(){unified_weapon_fire::MuzzleSample sample{};sample.source=frame;sample.origin={4,5,6};return muzzle::query(sample);}
};
}
int main(){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);unsigned cases_count{};
 {auto m=std::make_unique<TraceFixture>();std::uint64_t reads{};read_observer=[&](uptr,std::size_t){++reads;};assert(m->query());read_observer={};std::printf("Complete FastTrace query owned read calls: %llu\n",reads);}

 {auto m=std::make_unique<TraceFixture>();muzzle::Layout out{};muzzle::SchemaObservation observation{};
  assert(m->inspect(out,observation)&&out.offsets==m->offsets&&out.bytes==44&&observation.seen==31);
  assert(m->query()&&playground::invoke_count==1&&muzzle::query_gate==0&&muzzle::query_called&&muzzle::query_clear);++cases_count;}
 // Non-default reflected positions/masks must be used, never guessed offsets.
 for(unsigned reason=0;reason<6;++reason){auto m=std::make_unique<TraceFixture>();
  const muzzle::TypeGate expected[]={muzzle::TypeGate::bool_class,muzzle::TypeGate::bool_width,
   muzzle::TypeGate::bool_mask_read,muzzle::TypeGate::bool_mask,muzzle::TypeGate::bool_mask,muzzle::TypeGate::bool_flags};
  switch(reason){
   case 0:Memory::put(m->fields[4],0x50,m->struct_type);break;
   case 1:Memory::put(m->fields[4],0x74,2u);break;
   case 2:fail_read=m->fields[4]+0xd0;break;
   case 3:Memory::put(m->fields[4],0xd0,0u);break;
   case 4:Memory::put(m->fields[4],0xd0,3u);break;
   case 5:Memory::put(m->fields[4],0x78,std::uint64_t(0x480));break;
  }
  muzzle::Layout out{};muzzle::SchemaObservation observation{};
  assert(!m->inspect(out,observation)&&observation.gate==muzzle::SchemaGate::type);
  assert(observation.type_gate==expected[reason]&&observation.seen==31&&observation.field_index==4);
  assert(observation.field_address==m->fields[4]&&observation.element_bytes==(reason==1?2u:4u));
  assert(observation.mask_read==(reason>=3?1u:0u));
  assert(observation.mask==(reason==4?3u:reason==5?1u:0u));
  assert(playground::invoke_count==0);++cases_count;
 }
 {auto m=std::make_unique<TraceFixture>();m->offsets={12,28,0,48,56};m->bytes=64;
  for(unsigned i=0;i<5;++i)Memory::put(m->fields[i],0x94,m->offsets[i]);
  Memory::put(m->function,0x88,64u);Memory::put(m->function,0x106,std::uint16_t(64));
  Memory::put(m->function,0x108,std::uint16_t(56));playground::trace_bytes=64;
  assert(m->query());++cases_count;}
 for(unsigned reason=0;reason<24;++reason){auto m=std::make_unique<TraceFixture>();
  switch(reason){
   case 0:Memory::put(m->fields[3],0x48,m->text(L"bTraceComplex"));break;
   case 1:Memory::put(m->fields[1],0x68,m->fields[3]);break; // Missing BoxExtent.
   case 2:Memory::put(m->fields[2],0xd0,m->rotator);break;
   case 3:Memory::put(m->fields[2],0x78,std::uint64_t(0x180));break;
   case 4:Memory::put(m->fields[3],0xd0,3u);break;
   case 5:Memory::put(m->fields[3],0xd0,0u);break;
   case 6:Memory::put(m->fields[4],0x94,36u);Memory::put(m->function,0x108,std::uint16_t(36));break;
   case 7:Memory::put(m->function,0x108,std::uint16_t(0));break;
   case 8:Memory::put(m->function,0xf8,std::uint16_t(549));break;
   case 9:Memory::put(m->function,0x114,m->pawn);break;
   case 10:Memory::put(m->function,0xf4,0x400u);break;
   case 11:Memory::put(m->function,0x40,m->pawn_class);break;
   case 12:Memory::put(m->fields[2],0x40,m->actor_class);break;
   case 13:Memory::put(m->fields[4],0x78,std::uint64_t(0x80));break;
   case 14:Memory::put(m->fields[2],0x70,2u);break;
   case 15:Memory::put(m->fields[2],0x94,40u);break;
   case 16:Memory::put(m->fields[4],0x68,m->fields[0]);break;
   case 17:Memory::put(m->fields[2],0x48,m->text(L"TraceStart"));break;
   case 18:playground::trace_bytes=43;break;
   case 19:fail_read=m->fields[2]+0xd0;break;
   case 20:Memory::put(m->fields[2],0x74,4u);break;
   case 21:Memory::put(m->fields[3],0x78,std::uint64_t(0x180));break;
   case 22:Memory::put(m->vector,0x88,16u);break;
   case 23:Memory::put(m->pawn_class,0x60,uptr(0));break;
  }
  assert(!m->query()&&playground::invoke_count==0);++cases_count;
 }
 {auto m=std::make_unique<TraceFixture>();playground::trace=[&](unsigned char*){};
  assert(!m->query()&&playground::invoke_count==1&&muzzle::query_gate==11&&muzzle::query_called&&!muzzle::query_clear);++cases_count;} // Actual blocked result stays blocked.
 {auto m=std::make_unique<TraceFixture>();playground::invoke_ok=false;
  assert(!m->query()&&playground::invoke_count==1);++cases_count;}
 for(unsigned reason=0;reason<7;++reason){auto m=std::make_unique<TraceFixture>();auto original=playground::trace;
  playground::trace=[&,reason](unsigned char* params){original(params);
   switch(reason){
    case 0:Memory::put(m->fields[3],0xd0,2u);break;
    case 1:Memory::put(m->function,0x114,uptr(0));break;
    case 2:Memory::put(m->pawn,0x140,playground::Vector{2,2,3});break;
    case 3:Memory::put(m->function,0xf4,0x403u);break;
    case 4:Memory::put(m->fields[0],0x94,4u);break;
    case 5:nativeequipment::fail_check=nativeequipment::checks+1;break;
    case 6:Memory::put(m->function,0x108,std::uint16_t(36));break;
   }
  };
  assert(!m->query()&&playground::invoke_count==1);++cases_count;
 }
 for(unsigned check=1;check<=2;++check){auto m=std::make_unique<TraceFixture>();nativeequipment::fail_check=check;
  assert(!m->query()&&playground::invoke_count==0);++cases_count;}
 for(unsigned mutation_case=0;mutation_case<5;++mutation_case){auto m=std::make_unique<TraceFixture>();bool changed{};
  read_observer=[&](uptr,std::size_t){if(changed||muzzle::query_gate!=5)return;changed=true;
   switch(mutation_case){
    case 0:Memory::put(m->pawn,0x50,m->actor_class);break;
    case 1:Memory::put(m->fields[0],0x94,4u);break;
    case 2:Memory::put(m->fields[0],0x50,m->bool_type);break;
    case 3:Memory::put(m->fields[0],0x40,m->pawn);break;
    case 4:Memory::put(m->fields[0],8,999u);break;
   }
  };
  const bool accepted=m->query();read_observer={};assert(changed&&!accepted&&playground::invoke_count==0);++cases_count;
 }
 std::printf("Torque Bow FastTrace: %u cases passed; five typed params, reflected offsets, blocked return and changed context; owned memory/call stub only\n",cases_count);
}
