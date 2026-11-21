#include "../vr-common/weapon_support_grip.h"
#include <cassert>
#include <cstdio>
namespace hand=gears_weapon_hand;
bool fresh=true,native_owned=true,granted=true,observe_ok=true,sequence_ok=true,controls_ok=true;
unsigned observations{},controls_read{},acquisitions{};
namespace lancerreload_inventory {
struct Identity {std::uint64_t weapon{},life{};bool operator==(Identity b)const{return weapon==b.weapon&&life==b.life;}};
Identity identity(hand::Frame const& f){return {f.weapon.address,f.key.lifecycle};}
}
namespace nativeequipment {bool current(void*,hand::Frame const&){return fresh;}}
namespace lancerreload_state {
bool owns(hand::Frame const&,std::uint64_t){return native_owned;}
bool magazine_owned(hand::Frame const&){return granted;}
}
namespace lancermain_asset {enum class HeldAsset {longshot};}
namespace hammerburst_animation {
struct Observation {hand::ObjectIdentity node{},sequence{};};struct Controls {hand::ObjectIdentity node{};};
struct {bool active{},stopped{};std::uint64_t serial{};lancerreload_inventory::Identity inventory{};Observation last{};}ownership;
enum class Acquire {rejected,acquired,recovery_required};Acquire outcome=Acquire::acquired;
bool observe_kind(hand::Frame const&,Observation& out,lancermain_asset::HeldAsset){
 ++observations;if(!observe_ok)return false;out.node.address=200;out.sequence.address=300;return true;}
bool prepare_controls(hand::Frame const&,Observation const& value,Controls& out,bool enclosed){
 ++controls_read;assert(enclosed&&value.node.address==200);out.node=value.node;return controls_ok;}
Acquire acquire_prepared(hand::Frame const& f,std::uint64_t serial,Observation const& value,Controls const& controls){
 ++acquisitions;assert(value.node.address==200&&controls.node.address==200);
 if(outcome!=Acquire::rejected){ownership={true,outcome==Acquire::acquired,serial,lancerreload_inventory::identity(f),value};}
 return outcome;
}
}
namespace longshot_clip {
namespace anim=hammerburst_animation;namespace inv=lancerreload_inventory;
enum class Result {rejected,pending,ready,recovery_required};
std::uint64_t owned_serial{},pending_original_serial{},pending_attempt_frame{};
inv::Identity owned_identity{},pending_original_identity{};bool neutral_pending{};unsigned adopt_gate{},observe_rejection{};
bool original_sequence(hand::Frame const&,anim::Observation const& value){assert(value.sequence.address==300);return sequence_ok;}
#include "weapon_longshot_clip_adopt.inl"
}
struct Fixture {
 hand::Frame f{};
 Fixture(){
  fresh=native_owned=granted=observe_ok=sequence_ok=controls_ok=true;
  observations=controls_read=acquisitions=0;hammerburst_animation::ownership={};
  hammerburst_animation::outcome=hammerburst_animation::Acquire::acquired;
  f.weapon.address=64;f.key.lifecycle=1;f.key.frame=2;
  longshot_clip::owned_serial=longshot_clip::pending_attempt_frame=0;
  longshot_clip::owned_identity={};longshot_clip::neutral_pending=false;
  longshot_clip::pending_original_serial=7;longshot_clip::pending_original_identity=lancerreload_inventory::identity(f);
 }
 longshot_clip::Result run(){return longshot_clip::try_adopt_original(f);}
};
int main(){using R=longshot_clip::Result;
 {Fixture v;assert(v.run()==R::ready&&acquisitions==1&&hammerburst_animation::ownership.stopped);
  assert(longshot_clip::owned_serial==7&&!longshot_clip::pending_original_serial&&longshot_clip::neutral_pending);
  assert(hammerburst_animation::ownership.last.node.address==200); // Exact observed selected node, never an old/default node.
 }
 for(unsigned reason=0;reason<7;++reason){Fixture v;
  switch(reason){case 0:fresh=false;break;case 1:native_owned=false;break;case 2:granted=false;break;
   case 3:observe_ok=false;break;case 4:sequence_ok=false;break;case 5:controls_ok=false;break;
   case 6:hammerburst_animation::outcome=hammerburst_animation::Acquire::rejected;break;}
  assert(v.run()==R::pending&&!longshot_clip::owned_serial&&longshot_clip::pending_original_serial==7&&!hammerburst_animation::ownership.active);
 }
 {Fixture v;observe_ok=false;assert(v.run()==R::pending&&observations==1);observe_ok=true;
  assert(v.run()==R::pending&&observations==1);++v.f.key.frame;
  assert(v.run()==R::ready&&observations==2&&acquisitions==1);}
 {Fixture v;hammerburst_animation::outcome=hammerburst_animation::Acquire::recovery_required;
  assert(v.run()==R::recovery_required&&longshot_clip::owned_serial==7&&hammerburst_animation::ownership.active);}
 {Fixture v;++v.f.key.lifecycle;assert(v.run()==R::recovery_required&&observations==0&&acquisitions==0);}
 {Fixture v;hammerburst_animation::ownership.active=true;hammerburst_animation::ownership.serial=99;
  assert(v.run()==R::recovery_required&&acquisitions==0&&hammerburst_animation::ownership.serial==99);}
 std::puts("Longshot original clip adoption: exact selected LS owner, no Play API, once-only shared Stop acquisition, fresh-frame retry, unknown rejection and recovery debt passed");
}
