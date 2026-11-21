#include "../vr-common/weapon_torque_bow_main_grip.h"
#include "../vr-common/weapon_lancer_wrist_mount.h"
#include "weapon_torque_bow_arrow_tracking_fixture.h"
#include "weapon_torque_bow_arrow_tracking.h"
#include "../vr-common/weapon_torque_bow_feed.h"
#include <cstring>
#include <cwchar>
#include <limits>
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace w=gears_weapon;
#define C BOW_ARROW_CHECK
namespace test {
bool current=true,bow=true,identity=true,mutate{},copy_ok=true;unsigned copies{};
hand::ObjectIdentity mesh{};std::array<float,16> avatar{};
}
namespace weaponhand {bool current_gt(void*)noexcept{return true;}}
namespace nativeequipment {
bool current(void*,hand::Frame const&)noexcept{return test::current;}
bool identify(void*,uptr,hand::ObjectIdentity& out)noexcept{out=test::mesh;return test::identity;}
}
namespace playground {bool instance(uptr,wchar_t const*)noexcept{return test::bow;}}
bool copy(void* out,uptr address,std::size_t size)noexcept{
 if(!test::copy_ok||address!=test::mesh.address+0xa0||size!=sizeof test::avatar)return false;
 std::memcpy(out,test::avatar.data(),size);++test::copies;if(test::mutate)test::avatar[0]+=.01f;return true;
}
FILE* log_file{};
#include "weapon_torque_bow_main_mount.inl"
int main(){
 bow_arrow_tracking_fixture::Fixture source;const auto& f=source.hand;
 const vr::Viewpoint right{{1,2,3},{.1,.2,.3}};std::array<float,16> matrix{.3f,0,0,0,0,.3f,0,0,0,0,.3f,0,4,5,6,1};
 test::mesh=f.mesh;test::avatar=matrix;gears_weapon::WeaponMeshFollow fixed;
 C(torquebow_main_mount::bind(f,right,matrix,100,fixed)&&fixed.bound()&&test::copies==2);
 for(auto flag:{&test::current,&test::bow,&test::identity,&test::copy_ok}){
  gears_weapon::WeaponMeshFollow refused;*flag=false;C(!torquebow_main_mount::bind(f,right,matrix,100,refused)&&!refused.bound());*flag=true;
 }
 gears_weapon::WeaponMeshFollow changing;test::mutate=true;C(!torquebow_main_mount::bind(f,right,matrix,100,changing)&&!changing.bound());
 test::mutate=false;test::avatar=matrix;
 for(float value:{.4f,-.3f,std::numeric_limits<float>::quiet_NaN()}){
  gears_weapon::WeaponMeshFollow bad;test::avatar[0]=value;C(!torquebow_main_mount::bind(f,right,matrix,100,bad)&&!bad.bound());
 }
 test::avatar=matrix;const auto before=fixed;C(!torquebow_main_mount::bind(f,right,matrix,100,fixed));
 vr::Viewpoint a{},b{};C(before.solve(right,a)&&fixed.solve(right,b)&&a.position.x==b.position.x&&a.position.y==b.position.y&&a.position.z==b.position.z);
 // The normal RawRig -> original Root -> original arrow/feed authoring chain
 // consumes the same fixed main mount, with no recaptured hand/weapon pose.
 const auto& physical=source.rig.world_grip[1];gears_weapon::WeaponMeshFollow chain;
 const vr::Viewpoint raw_right{physical.position,vr::ue_angles(vr::rotation_matrix(physical.orientation))};
 C(torquebow_main_mount::bind(f,raw_right,matrix,source.rig.units_per_metre,chain));
 gears_torque_bow_arrow_tracking::RootSample root{};
 C(gears_torque_bow_arrow_tracking::root_sample(f,source.rig,chain,bow_arrow_tracking_fixture::now,root));
 C(root.input_sequence==f.controllers.sequence&&root.input_sync_ns==f.controllers.sync_ns&&std::abs(root.scale-.3)<1e-6);
 gears_hand_authored_mount::Mount left{};gears_torque_bow_arrow_mount::Mount arrow{};w::TorqueBowFeedCalibration feed{};
 C(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,gears_hand_authored_mount::Side::left,
   root.scale,source.rig.units_per_metre,left));
 C(gears_torque_bow_arrow_mount::author_original(left.calibration,root.scale,root.scale,source.rig.units_per_metre,arrow));
 C(w::author_torque_bow_feed(1,root.scale,arrow,feed)&&feed.valid());
 std::printf("Torque Bow fixed main binder: %u checks passed; actual binder and geometry, injected Native/memory; no game/C0/GPU acceptance.\n",bow_arrow_tracking_fixture::checks);
}
