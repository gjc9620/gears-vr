#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/weapon_boomshot_animation_tracks.h"
#include <cassert>
#include <cstdio>
namespace hand=gears_weapon_hand;
bool fresh=true,grant=true,observe_ok=true,curve_ok=true,controls_ok=true,return_ok=true;
unsigned observations{},acquisitions{},seeks{},returns{};std::uint64_t content=42;
namespace lancerreload_inventory {
struct Identity {std::uint64_t weapon{},life{};bool operator==(Identity const& b)const{return weapon==b.weapon&&life==b.life;}};
Identity identity(hand::Frame const& f){return {f.weapon.address,f.key.lifecycle};}
}
namespace nativeequipment {bool current(void*,hand::Frame const&){return fresh;}}
namespace lancerreload_state {
struct Lease {bool acquired{};std::uint64_t serial{};lancerreload_inventory::Identity identity{};}lease;
bool owns(hand::Frame const& f,std::uint64_t n){return lease.acquired&&lease.serial==n&&lease.identity==lancerreload_inventory::identity(f);}
bool item_owned(hand::Frame const&){return grant;}
}
namespace lancermain_asset {enum class HeldAsset {boomshot};}
namespace hammerburst_animation {
struct Observation {hand::ObjectIdentity sequence{},node{};};struct Controls {};
struct Owner {bool active{},stopped{};std::uint64_t serial{};lancerreload_inventory::Identity inventory{};Observation last{};}ownership;
enum class Acquire {rejected,acquired,recovery_required};Acquire outcome=Acquire::acquired;
bool observe_kind(hand::Frame const&,Observation& value,lancermain_asset::HeldAsset){
 ++observations;value.sequence.address=300;value.node.address=200;return observe_ok;}
bool prepare_controls(hand::Frame const&,Observation const&,Controls&,bool enclosed){assert(enclosed);return controls_ok;}
Acquire acquire_prepared(hand::Frame const& f,std::uint64_t serial,Observation const& value,Controls const&){
 ++acquisitions;if(outcome!=Acquire::rejected)ownership={true,outcome==Acquire::acquired,serial,lancerreload_inventory::identity(f),value};return outcome;}
bool seek(hand::Frame const&,std::uint64_t serial,float t){assert(serial==ownership.serial&&t>=0&&t<2);++seeks;return true;}
bool hand_back_after_native_exit(){++returns;if(!return_ok)return false;ownership={};return true;}
}
namespace boomshot_animation {
struct Curve {hand::ObjectIdentity sequence{};std::uint64_t content{};gears_weapon::boomshot_tracks::Samples samples{};};
bool read_curve(hand::Frame const&,hammerburst_animation::Observation const& value,Curve& out){
 if(!curve_ok)return false;out.sequence=value.sequence;out.content=content;out.samples.count=8;out.samples.length=2;
 const double q[]={0,0,.3,1,1,.8,.5,0};
 for(unsigned i=0;i<8;++i)out.samples.positions[2][i]={0,gears_weapon::boomshot_barrel::closed_raw.y-gears_weapon::boomshot_barrel::travel_raw*q[i],gears_weapon::boomshot_barrel::closed_raw.z};
 return true;}
}
#include "weapon_boomshot_clip.inl"
struct Fixture {
 hand::Frame f{};
 Fixture(){fresh=grant=observe_ok=curve_ok=controls_ok=return_ok=true;observations=acquisitions=seeks=returns=0;content=42;
  hammerburst_animation::ownership={};hammerburst_animation::outcome=hammerburst_animation::Acquire::acquired;
  boomshot_clip::serial=boomshot_clip::attempt_frame=0;boomshot_clip::identity={};boomshot_clip::curve={};boomshot_clip::pending=false;
  f.weapon.address=64;f.key.lifecycle=1;f.key.frame=2;
  lancerreload_state::lease={true,7,lancerreload_inventory::identity(f)};}
 boomshot_clip::Result start(){return boomshot_clip::start(f,7);}
};
int main(){using R=boomshot_clip::Result;using D=gears_weapon::boomshot_barrel::Direction;
 {Fixture v;assert(v.start()==R::ready&&boomshot_clip::owned()&&acquisitions==1);
  assert(boomshot_clip::service(v.f)==R::ready&&acquisitions==1);
  assert(boomshot_clip::seek(v.f,.3,D::opening)&&seeks==1);
  ++content;assert(!boomshot_clip::seek(v.f,.4,D::opening)&&seeks==1);
  assert(!boomshot_clip::hand_back_after_native_exit()&&returns==0);
  lancerreload_state::lease.acquired=false;return_ok=false;
  assert(!boomshot_clip::hand_back_after_native_exit()&&boomshot_clip::serial==7);
  return_ok=true;assert(boomshot_clip::hand_back_after_native_exit()&&!boomshot_clip::serial&&!hammerburst_animation::ownership.active);}
 for(unsigned reason=0;reason<3;++reason){Fixture v;switch(reason){case 0:fresh=false;break;case 1:grant=false;break;case 2:lancerreload_state::lease.serial=8;break;}
  assert(v.start()==R::rejected&&!boomshot_clip::serial&&acquisitions==0);}
 for(unsigned reason=0;reason<4;++reason){Fixture v;switch(reason){case 0:observe_ok=false;break;case 1:curve_ok=false;break;case 2:controls_ok=false;break;case 3:hammerburst_animation::outcome=hammerburst_animation::Acquire::rejected;break;}
  assert(v.start()==R::pending&&boomshot_clip::serial==7&&boomshot_clip::pending);
  const auto old=observations;assert(boomshot_clip::service(v.f)==R::pending&&observations==old);
  observe_ok=curve_ok=controls_ok=true;hammerburst_animation::outcome=hammerburst_animation::Acquire::acquired;++v.f.key.frame;
  assert(boomshot_clip::service(v.f)==R::ready&&boomshot_clip::owned());}
 {Fixture v;hammerburst_animation::outcome=hammerburst_animation::Acquire::recovery_required;
  assert(v.start()==R::recovery_required&&!boomshot_clip::pending&&boomshot_clip::serial==7);
  assert(boomshot_clip::service(v.f)==R::recovery_required&&acquisitions==1&&!boomshot_clip::seek(v.f,.1,D::opening));}
 {Fixture v;assert(v.start()==R::ready);++v.f.key.lifecycle;
  assert(boomshot_clip::service(v.f)==R::rejected&&!boomshot_clip::seek(v.f,.1,D::opening)&&acquisitions==1);}
 {Fixture v;hammerburst_animation::ownership.active=true;hammerburst_animation::ownership.serial=99;
  assert(v.start()==R::rejected&&acquisitions==0&&hammerburst_animation::ownership.serial==99);}
 {Fixture v;observe_ok=false;assert(v.start()==R::pending);assert(!boomshot_clip::hand_back_after_native_exit());
  lancerreload_state::lease.acquired=false;assert(boomshot_clip::hand_back_after_native_exit()&&returns==0);}
 std::puts("Boomshot clip: original-owner admission, one Stop journal, same-frame retry, content-bound seeks, uncertain dispatch debt and post-native-only handback passed; owned stubs only");
}
