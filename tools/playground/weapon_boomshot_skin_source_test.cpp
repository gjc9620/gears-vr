#include "../vr-common/weapon_boomshot_hand_mount.h"
#include "../vr-common/committed_hand_interaction.h"
#include "../vr-common/weapon_two_hand_hold.h"
#include "weapon_boomshot_contact_geometry.h"
#define main previous_skin_source_core_cases
#include "weapon_boomshot_core_source_test.cpp"
#undef main
namespace {
using uptr=std::uint64_t;unsigned skin_source_fault{},pose_reads{},pair_reads{};bool skin_debt{};
gears_committed_interaction::Frame actual_pair{};
namespace vrequip {struct Model{std::uint64_t hold_generation()const{return 10;}}model;}
namespace nativelancerfollow {
bool bound_key_known=true;struct Key{h::ObjectIdentity weapon{};std::uint64_t hold_generation{10};}bound_key;
struct Mount{bool bound()const{return true;}bool model_matrix(gears_vr::Viewpoint const&,std::array<float,16>& out)const{
 out={};out[0]=out[5]=out[10]=out[15]=1;return true;}}mount;
}
namespace boomshot_reload_owner {struct Owner{bool blocks_fire(h::Frame const&)const{return skin_debt;}}owner;}
namespace boomshot_pose {
struct Source{h::ObjectIdentity component{},asset{};std::array<std::array<float,16>,3> world{};unsigned revision{1};};
Source posed{};
bool ready(h::Frame const&,Source& out){++pose_reads;out=posed;if(skin_source_fault==2&&pose_reads>1)out.world[0][12]+=1;return skin_source_fault!=1;}
bool same_reference(Source const& a,Source const& b){return a.revision==b.revision&&h::same(a.component,b.component)&&h::same(a.asset,b.asset);}
}
namespace hammerburst_runtime {
bool matrix(std::array<float,16> const& a,gears_vr::Pose& p,double& scale){
 scale=std::hypot(double(a[0]),double(a[1]),double(a[2]));if(!std::isfinite(scale)||scale<=0)return false;
 gears_vr::Matrix basis{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)basis[j][i]=a[i*4+j]/scale;
 if(!gears_vr::rig_tracking::detail::quaternion(basis,p.orientation))return false;p.position={a[12],a[13],a[14]};return true;
}
}
namespace handrig {
bool read_interaction(gears_committed_interaction::Frame& out){++pair_reads;out=actual_pair;if(skin_source_fault==4&&pair_reads>1)out.model_scale+=.1;return skin_source_fault!=3;}
}
#include "weapon_boomshot_skin_source.inl"
namespace skin=boomshot_skin_source;
void setup_skin_source(Owned& x,Fixture& fixture){
 reset_source(x,fixture);skin_source_fault=pose_reads=pair_reads=0;skin_debt=false;nativelancerfollow::bound_key_known=true;nativelancerfollow::bound_key={x.f.weapon,10};
 auto& p=boomshot_pose::posed;p={};p.component=object(0x6000,20);p.asset=object(0x6100,21);
 for(auto& m:p.world)m[0]=m[5]=m[10]=m[15]=1;
 p.world[2][13]=float(m::closed_raw.y);p.world[2][14]=float(m::closed_raw.z);
 actual_pair={};actual_pair.reference=x.f.reference;actual_pair.model_scale=1.2;
 actual_pair.left_grip=actual_pair.left_wrist=actual_pair.physical_left_grip=fixture.rig.world_grip[0];
 auto& r=actual_pair.right;r.parent=x.f.key;r.world=x.f.world;r.controller=x.f.controller;r.pawn=x.f.pawn;r.mesh=x.f.mesh;r.weapon=x.f.weapon;
 r.serial=r.tick=r.compose=1;r.hold_generation=10;r.binding_generation=fixture.rig.binding_generation;r.reference_generation=x.f.reference_generation;
 r.native_reference=fixture.rig.native_reference;r.producer=fixture.rig.producer;
 r.acquisition_begin_ns=r.acquisition_end_ns=r.committed_ns=r.returned_ns=clock_value-500000;
 r.stereo=x.f.stereo;r.controllers=x.f.controllers;r.geometry=gears_hand_authored_mount::marcus115_signature;r.units_per_metre=fixture.rig.units_per_metre;r.right_grip=fixture.rig.world_grip[1];
}
}
int main(){unsigned count{};
 {Owned x;Fixture fixture;setup_skin_source(x,fixture);skin::Source out{};const auto before=x.f;
  assert(skin::read(x.f,out)&&out.hand_scale==1.2&&out.units==fixture.rig.units_per_metre&&pose_reads==2&&pair_reads==2);
  assert(h::same_frame(before,x.f)&&out.rig.controllers.sync_ns==x.f.controllers.sync_ns);++count;}
 for(unsigned fault=1;fault<=4;++fault){Owned x;Fixture fixture;setup_skin_source(x,fixture);skin_source_fault=fault;skin::Source out{};
  assert(!skin::read(x.f,out)&&out.units==0);++count;}
 for(unsigned fault=0;fault<12;++fault){Owned x;Fixture fixture;setup_skin_source(x,fixture);
  auto& r=actual_pair.right;
  switch(fault){
   case 0:actual_pair.model_scale=0;break;
   case 1:r.units_per_metre+=1;break;
   case 2:++r.binding_generation;break;
   case 3:++r.native_reference.epoch;break;
   case 4:++r.producer.tracking[0];break;
   case 5:++r.weapon.registry_index;break;
   case 6:++r.controllers.sequence;break;
   case 7:r.acquisition_begin_ns-=200000000;break;
   case 8:fixture.rig.world_pose_mask&=~gears_rig_tracking::left_grip_pose;break;
   case 9:x.f.recentered=1;break;
   case 10:r.purpose=gears_committed_hand::Purpose::free_hand_visual;break;
   default:++nativelancerfollow::bound_key.hold_generation;
  }
  skin::Source out{};assert(!skin::read(x.f,out)&&out.units==0);++count;
 }
 {Owned x;Fixture fixture;setup_skin_source(x,fixture);gears_vr::Pose relative{};assert(skin::support_relative(x.f,relative)&&gears_weapon_two_hand::geometry(relative));
  skin_debt=true;assert(!skin::support_relative(x.f,relative));skin_debt=false;x.f.interaction_control=unsigned(gears_vr::LeftInteractionControl::pinch);assert(!skin::support_relative(x.f,relative));++count;}
 std::printf("Boomshot current left skin source: %u cases passed; production source brackets + real C0 interaction matcher and Scene Rig matcher, body/gun scales separate, changed/missing/future/foreign source rejected, no input renewal; pose arrays, C0 provider and matrix helper boundaries stubbed, no live original collection proof\n",count);
}
