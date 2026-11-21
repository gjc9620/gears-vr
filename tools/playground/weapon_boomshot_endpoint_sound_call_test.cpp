#define main previous_audio_metadata_cases
#include "weapon_boomshot_endpoint_sound_metadata_test.cpp"
#undef main
#define main previous_audio_rig_cases
#define now audio_rig_baseline
#include "weapon_rig_binding_test.cpp"
#undef now
#undef main
namespace {
using Name=meta::RawName;
std::int64_t audio_stamp=audio_rig_baseline;bool audio_current=true,audio_returned=true;unsigned getter_calls{},sound_calls{};
uptr getter_function{},getter_result{},play_function{},last_sound{},getter_receiver{},play_receiver{};float last_noise{};
std::function<void()> getter_mutation,effect_mutation;FILE* log_file{};
namespace weaponhand {std::int64_t audio_fixture_clock(void*)noexcept{return audio_stamp;}}
namespace nativeequipment {bool current(void*,abi::Frame const&)noexcept{return audio_current;}}
namespace lancerreload_inventory {
struct Counts {int capacity{1},used{1},reserve{2};bool infinite{};
 bool operator==(Counts const& b)const{return capacity==b.capacity&&used==b.used&&reserve==b.reserve&&infinite==b.infinite;}}audio_counts;
bool read(abi::Frame const&,Counts& out)noexcept{out=audio_counts;return audio_current;}
}
namespace playground {
struct Args {
 struct {uptr function{};unsigned size{};unsigned char params[64]{};}r;bool valid=true,is_find{};
 Args(uptr receiver,wchar_t const* label){is_find=!std::wcscmp(label,L"FindAnimSequence");
  assert(is_find||!std::wcscmp(label,L"WeaponPlaySoundLocal"));assert(receiver==(is_find?getter_receiver:play_receiver));
  r.function=is_find?getter_function:play_function;r.size=is_find?16:12;}
 bool run(){if(is_find){++getter_calls;std::memcpy(r.params+8,&getter_result,8);if(getter_mutation)getter_mutation();return true;}
  ++sound_calls;std::memcpy(&last_sound,r.params,8);std::memcpy(&last_noise,r.params+8,4);if(effect_mutation)effect_mutation();return audio_returned;}
};
}
#define clock_ns audio_fixture_clock
namespace boomshot_endpoint_sound {
#include "weapon_boomshot_endpoint_sound_call.inl"
}
#undef clock_ns
void fake_audio_find_endpoint(){}
struct AudioCallMemory:AudioMemory {
 uptr reload_property{},sequence_name{},find_function{},find_result_property{};
 AudioCallMemory(){
  audio_stamp=audio_rig_baseline;audio_current=audio_returned=true;getter_calls=sound_calls=0;last_sound=0;last_noise=-1;
  getter_mutation=effect_mutation={};audio::consumed_ordinal=0;lancerreload_inventory::audio_counts={};
  const auto function_type=node(L"Function",class_type,core),name_type=node(L"NameProperty",class_type,core);
  const auto boomer=cls(L"Weap_Boomer_base",warfare,weapon_class);put(weapon,0x50,boomer);
  reload_property=property(weapon_class,L"WeaponReloadAnim",name_type,0x1e0,8);put(weapon,0x1e0,text(L"AR_Reload"));
  sequence_name=property(sequence_class,L"SequenceName",name_type,0x100,8);put(sequence,0x100,text(L"AR_Reload"));
  find_function=node(L"FindAnimSequence",function_type,mesh_class);put(find_function,0xf4,0x401u);put(find_function,0x106,std::uint16_t(16));
  put(find_function,0x108,std::uint16_t(8));put(find_function,0x114,reinterpret_cast<uptr>(&fake_audio_find_endpoint));
  const auto input=property(find_function,L"AnimSeqName",name_type,0,8);find_result_property=property(find_function,L"ReturnValue",object_type,8,8);
  put(input,0x78,std::uint64_t(0x80));put(find_result_property,0x78,std::uint64_t(0x480));put(find_result_property,0xd0,sequence_class);
  Fixture rig(1);frame=rig.hand;frame.world=identity(world);frame.controller=identity(pc);frame.pawn=identity(pawn);frame.mesh=identity(mesh);frame.weapon=identity(weapon);
  frame.key.thread_id=GetCurrentThreadId();getter_function=find_function;getter_result=sequence;play_function=function;getter_receiver=mesh;play_receiver=weapon;tables();
  assert(abi::frame_active(frame,audio_stamp));
 }
 ~AudioCallMemory(){getter_mutation=effect_mutation={};}
 bool play(audio::Cue kind=audio::Cue::opened,std::uint64_t ordinal=1,std::int64_t stamp=audio_rig_baseline){return audio::play(frame,kind,ordinal,stamp);}
};
}
int main(){unsigned count{};
 {auto m=std::make_unique<AudioCallMemory>();const auto before=m->frame;
  assert(m->play()&&getter_calls==2&&sound_calls==1&&last_sound==m->cue[0]&&last_noise==0&&abi::same_frame(before,m->frame));
  assert(!m->play()&&getter_calls==2&&sound_calls==1);assert(m->play(audio::Cue::seated,2)&&last_sound==m->cue[1]&&sound_calls==2);
  assert(m->play(audio::Cue::closed,3)&&last_sound==m->cue[2]&&sound_calls==3);++count;}
 {auto m=std::make_unique<AudioCallMemory>();audio_returned=false;assert(!m->play()&&sound_calls==1);audio_returned=true;
  assert(!m->play()&&sound_calls==1&&getter_calls==2);++count;}
 {auto m=std::make_unique<AudioCallMemory>();effect_mutation=[](){++lancerreload_inventory::audio_counts.reserve;};assert(!m->play()&&sound_calls==1);
  effect_mutation={};assert(!m->play()&&sound_calls==1);++count;}
 for(unsigned reason=0;reason<13;++reason){auto m=std::make_unique<AudioCallMemory>();
  switch(reason){case 0:audio_current=false;break;case 1:lancerreload_inventory::audio_counts.capacity=2;break;
   case 2:Memory::put(m->weapon,0x1e0,m->text(L"OtherReload"));break;case 3:Memory::put(m->sequence,0x100,m->text(L"OtherReload"));break;
   case 4:getter_result=m->pawn;break;case 5:getter_mutation=[&]{if(getter_calls==1)getter_result=m->pawn;};break;
   case 6:getter_mutation=[&]{if(getter_calls==1)Memory::put(m->event[0],0x100,m->cue[1]);};break;
   case 7:getter_mutation=[](){audio_current=false;};break;
   case 8:Memory::put(m->noise_param,0x78,std::uint64_t(0x480));break;
   case 9:Memory::put(m->find_function,0x114,base+0x1234);break;
   case 10:m->frame.left_tracked=0;break;
   case 11:Memory::put(m->weapon,0x50,m->concrete);m->frame.weapon=m->identity(m->weapon);break;
   case 12:getter_mutation=[&]{if(getter_calls==2){const float time=3.f;std::memcpy(m->data.data(),&time,4);}};break;}
  assert(!m->play()&&!sound_calls);audio_current=true;getter_mutation={};assert(!m->play()&&!sound_calls);++count;}
 {auto m=std::make_unique<AudioCallMemory>();assert(!m->play(audio::Cue::opened,1,audio_rig_baseline-100000001)&&!getter_calls&&!sound_calls);++count;}
 {auto m=std::make_unique<AudioCallMemory>();assert(!m->play(audio::Cue::opened,1,audio_rig_baseline+1)&&!getter_calls&&!sound_calls);++count;}
 {auto m=std::make_unique<AudioCallMemory>();assert(!m->play(static_cast<audio::Cue>(3))&&!getter_calls&&!sound_calls);++count;}
 std::printf("Boomshot endpoint sound current bind/dispatch: %u cases passed; production dual getter+metadata binder+parameter writer, three original cues/local zero noise, counts/source postcheck, unknown return and failed binding consume ordinal once; Engine/getter/current/clock boundaries injected, no game/audio proof\n",count);
}
