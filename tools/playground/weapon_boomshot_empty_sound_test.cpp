#define GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL 1
#include "../vr-common/weapon_main_mount_provider.h"
#include "weapon_hand_offline_fixture.h"
namespace {
#include "weapon_lancer_asset_read.inl"
#include "weapon_boomshot_empty_sound.inl"
struct SoundMemory:Memory {
 uptr war{},boomer{},active{},fn{},cue{};std::array<unsigned char,5> code{{0x1b,1,0x16,0x04,0x0b}};
 SoundMemory(){
  war=cls(L"WarWeapon",warfare,weapon_class);boomer=cls(L"Weap_Boomer_base",warfare,war);put(weapon,0x50,boomer);
  const auto state_type=cls(L"State",core),function_type=cls(L"Function",core),sound_type=cls(L"SoundCue",engine);
  active=node(L"Active",state_type,war);put(war,0x80,active);fn=node(L"PlayClickClickEmptyClip",function_type,active);put(active,0x80,fn);
  put(fn,0x88,0u);put(fn,0x106,std::uint16_t(0));put(fn,0x108,std::uint16_t(0xffff));put(fn,0x114,base+0x1d6ca0);
  put(fn,0x8c,meta::Table{reinterpret_cast<uptr>(code.data()),int(code.size()),int(code.size())});
  cue=node(L"InheritedEmptyCue",sound_type,warfare);property(war,L"FireNoAmmoSound",object_type,0x180,8);put(weapon,0x180,cue);
  frame.weapon=identity(weapon);frame.key.thread_id=GetCurrentThreadId();tables();
 }
};
}
int main(){unsigned count{};const auto check=[&](bool v){assert(v);++count;};
 auto memory=std::make_unique<SoundMemory>();auto& m=*memory;namespace sound=boomshot_empty_sound;sound::Entry entry{};
 check(sound::read(m.frame,entry)&&entry.function.address==m.fn&&entry.cue.address==m.cue);const auto valid=entry;
 check(sound::read(m.frame,entry)&&sound::same(valid,entry));
 Memory::put(m.fn,0x106,std::uint16_t(4));check(!sound::read(m.frame,entry));Memory::put(m.fn,0x106,std::uint16_t(0));
 Memory::put(m.fn,0x108,std::uint16_t(0));check(!sound::read(m.frame,entry));Memory::put(m.fn,0x108,std::uint16_t(0xffff));
 Memory::put(m.fn,0xf8,std::uint16_t(113));check(!sound::read(m.frame,entry));Memory::put(m.fn,0xf8,std::uint16_t(0));
 Memory::put(m.fn,0xf4,0x400u);check(!sound::read(m.frame,entry));Memory::put(m.fn,0xf4,0u);
 Memory::put(m.fn,0x114,base+0x1d6ca8);check(!sound::read(m.frame,entry));Memory::put(m.fn,0x114,base+0x1d6ca0);
 Memory::put(m.weapon,0x180,uptr{});check(!sound::read(m.frame,entry));Memory::put(m.weapon,0x180,m.cue);
 Memory::put(m.cue,0x50,m.actor_class);check(!sound::read(m.frame,entry));Memory::put(m.cue,0x50,valid.cue.klass);
 Memory::put(m.active,0x40,m.engine);check(!sound::read(m.frame,entry));Memory::put(m.active,0x40,m.war);
 Memory::put(m.fn,0x40,m.war);check(!sound::read(m.frame,entry));Memory::put(m.fn,0x40,m.active);
 Memory::put(m.war,0x40,m.engine);check(!sound::read(m.frame,entry));Memory::put(m.war,0x40,m.warfare);
 const auto foreign=m.cls(L"Weap_Bow_base",m.warfare,m.war);Memory::put(m.weapon,0x50,foreign);check(!sound::read(m.frame,entry));Memory::put(m.weapon,0x50,m.boomer);
 mutate_read=reinterpret_cast<uptr>(m.code.data());mutation=[&]{m.code[0]^=1;};check(!sound::read(m.frame,entry));m.code[0]^=1;
 check(sound::read(m.frame,entry)&&sound::same(valid,entry));m.code[0]^=1;check(sound::read(m.frame,entry)&&!sound::same(valid,entry));
 std::printf("Boomshot original empty cue metadata: %u cases passed; actual Reader/identity/owner/schema/script rereads on owned memory, Boomer-only, changed script identity observable; synthetic script, no original execution/audio proof\n",count);
}
