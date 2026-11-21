#define GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL 1
#include "../vr-common/weapon_main_mount_provider.h"
#include "weapon_hand_offline_fixture.h"
#include <limits>
namespace {
#include "weapon_lancer_asset_read.inl"
namespace unified_weapon_fire {
bool owner_in_receiver(gripdamage_metadata::Reader& r,uptr receiver,uptr owner)noexcept {
 unsigned object{};if(!r.path(receiver,object))return false;auto cls=r.result.identities[object].klass;
 std::array<uptr,gripdamage_metadata::max_parents> seen{};unsigned count{};
 while(cls){if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==cls)return false;seen[count++]=cls;
  gripdamage_metadata::Structure value{};if(!r.structure(cls,value))return false;if(cls==owner)return true;cls=value.super;}return false;
}
}
#define BOOMSHOT_ENDPOINT_SOUND_METADATA_FIXTURE 1
#include "weapon_boomshot_endpoint_sound.inl"
#undef BOOMSHOT_ENDPOINT_SOUND_METADATA_FIXTURE
namespace audio=boomshot_endpoint_sound;
struct AudioMemory:Memory {
 uptr sequence{},sequence_class{},notifies{},inner{},structure{},time{},notify{},comment{},sound_class{},sound_notify{},root{},weapons{},reloads{},function{},sound_param{},noise_param{};
 std::array<uptr,4> event{};std::array<uptr,3> cue{};std::array<unsigned char,80> data{};std::array<unsigned char,5> code{{1,2,3,4,5}};
 struct Call {bool valid=true;struct {uptr function{};unsigned size{12};unsigned char params[64]{};}r;}call;
 AudioMemory(){
  const auto name_type=node(L"NameProperty",class_type,core),float_type=node(L"FloatProperty",class_type,core),array_type=node(L"ArrayProperty",class_type,core),
   struct_type=node(L"StructProperty",class_type,core),script_type=node(L"ScriptStruct",class_type,core),function_type=node(L"Function",class_type,core);
  sequence_class=cls(L"AnimSequence",engine);const auto set_type=cls(L"AnimSet",engine);root=node(L"Weapon_Boomer",package_type);
  weapons=node(L"Weapons",package_type,root);reloads=node(L"Reloads",package_type,root);const auto set=node(L"OriginalBoomshotSet",set_type,root);
  sequence=node(L"OriginalReload",sequence_class,set);notifies=property(sequence_class,L"Notifies",array_type,0x110,16);
  structure=node(L"AnimNotifyEvent",script_type,sequence_class);put(structure,0x88,20u);
  time=property(structure,L"Time",float_type,0,4);notify=property(structure,L"Notify",object_type,4,8);comment=property(structure,L"Comment",name_type,12,8);
  inner=node(L"Notifies",struct_type,notifies);put(inner,0x70,1u);put(inner,0x74,20u);put(inner,0xd0,structure);put(notifies,0xd0,inner);
  sound_class=cls(L"SoundCue",engine);cue={node(L"BoomerFoley01Cue",sound_class,weapons),node(L"BoomerInsertShellCue",sound_class,reloads),node(L"BoomerFoley03Cue",sound_class,weapons)};
  const auto notify_class=cls(L"AnimNotify",engine);put(notify,0xd0,notify_class);sound_notify=cls(L"AnimNotify_Sound",engine,notify_class);const auto script_notify=cls(L"AnimNotify_Script",engine,notify_class);
  property(sound_notify,L"SoundCue",object_type,0x100,8);
  for(unsigned i=0;i<4;++i){event[i]=node(i==0?L"OpenSound":i==1?L"SeatSound":i==2?L"CloseSound":L"ScriptNotify",i<3?sound_notify:script_notify,sequence);
   if(i<3)put(event[i],0x100,cue[i]);const float seconds=.1f+float(i);std::memcpy(data.data()+20*i,&seconds,4);std::memcpy(data.data()+20*i+4,&event[i],8);}
  put(sequence,0x110,meta::Table{reinterpret_cast<uptr>(data.data()),4,4});
  function=node(L"WeaponPlaySoundLocal",function_type,weapon_class);put(function,0x88,12u);put(function,0x106,std::uint16_t(12));put(function,0x108,std::uint16_t(0xffff));
  put(function,0x114,base+0x1d6ca0);put(function,0x8c,meta::Table{reinterpret_cast<uptr>(code.data()),int(code.size()),int(code.size())});
  sound_param=property(function,L"Sound",object_type,0,8);noise_param=property(function,L"NoiseLoudness",float_type,8,4);
  put(sound_param,0x78,std::uint64_t(0x80));put(noise_param,0x78,std::uint64_t(0x90));put(sound_param,0xd0,sound_class);call.r.function=function;tables();
 }
 bool read(audio::Cues& out){meta::Snapshot s{};s.module_base=base;meta::Reader r{s,meta::Stage::final_check};
  const bool result=meta::tables(base,s.objects,s.names)&&audio::cues(r,identity(sequence),out)&&audio::retained(r);
  return result;}
 bool schema(audio::PlayLayout& out){meta::Snapshot s{};s.module_base=base;meta::Reader r{s,meta::Stage::final_check};
  return meta::tables(base,s.objects,s.names)&&audio::play_schema(call,weapon,r,out)&&audio::retained(r);}
};
}
int main(){unsigned count{};audio::Cues cues{};audio::PlayLayout layout{};
 {auto m=std::make_unique<AudioMemory>();assert(m->read(cues)&&cues.cue[0].address==m->cue[0]&&cues.cue[1].address==m->cue[1]&&cues.cue[2].address==m->cue[2]);
  assert(m->schema(layout)&&layout.bytes==12&&layout.sound==0&&layout.noise==8);++count;}
 for(unsigned reason=0;reason<17;++reason){auto m=std::make_unique<AudioMemory>();
  switch(reason){case 0:Memory::put(m->sequence,0x110,meta::Table{0,4,4});break;case 1:Memory::put(m->sequence,0x110,meta::Table{reinterpret_cast<uptr>(m->data.data()),65,65});break;
   case 2:Memory::put(m->inner,0x74,129u);break;case 3:Memory::put(m->structure,0x88,24u);break;case 4:Memory::put(m->notify,0x94,18u);break;
   case 5:Memory::put(m->notify,0x94,0u);break;case 6:Memory::put(m->comment,0x68,m->time);break;case 7:Memory::put(m->event[0],0x40,m->pawn);break;
   case 8:Memory::put(m->cue[0],0x40,m->reloads);break;case 9:Memory::put(m->root,0x48,m->text(L"Weapon_Bow"));break;
   case 10:Memory::put(m->event[1],0x100,uptr{});break;case 11:Memory::put(m->cue[2],0x50,m->actor_class);break;
   case 12:{const float v=std::numeric_limits<float>::quiet_NaN();std::memcpy(m->data.data(),&v,4);break;}
   case 13:Memory::put(m->structure,0x40,m->mesh_class);break;
   case 14:Memory::put(m->inner,0x40,m->sequence_class);break;
   case 15:mutate_read=reinterpret_cast<uptr>(m->data.data());mutation=[&]{m->data[0]^=1;};break;
   case 16:Memory::put(m->notify,0xd0,m->actor_class);break;}
  assert(!m->read(cues));++count;}
 for(unsigned reason=0;reason<13;++reason){auto m=std::make_unique<AudioMemory>();
  switch(reason){case 0:m->call.r.size=64;break;case 1:Memory::put(m->function,0x108,std::uint16_t(0));break;
   case 2:Memory::put(m->function,0xf8,std::uint16_t(33));break;case 3:Memory::put(m->function,0xf4,0x400u);break;
   case 4:Memory::put(m->function,0x114,base+0x1d6ca8);break;case 5:Memory::put(m->function,0x40,m->actor_class);break;
   case 6:Memory::put(m->sound_param,0xd0,m->actor_class);break;case 7:Memory::put(m->noise_param,0x94,4u);break;
   case 8:Memory::put(m->noise_param,0x78,std::uint64_t(0x480));break;case 9:Memory::put(m->noise_param,0x74,8u);break;
   case 10:Memory::put(m->function,0x8c,meta::Table{0,5,5});break;case 11:Memory::put(m->noise_param,0x68,m->sound_param);break;
   case 12:Memory::put(m->function,0x48,m->text(L"WeaponPlaySound"));break;}
  assert(!m->schema(layout));++count;}
 std::printf("Boomshot endpoint sound metadata: %u cases passed; actual reflected notify struct/array and current full original cue paths, bounded closing bytes, original local sound ABI/script/offsets; owned memory and synthetic script only, no getter/audio execution\n",count);return 0;
}
