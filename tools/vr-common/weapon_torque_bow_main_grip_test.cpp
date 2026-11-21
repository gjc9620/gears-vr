#include "weapon_torque_bow_main_grip.h"
#include "weapon_lancer_wrist_mount.h"
#include "rig_tracking_math.h"
#include <cstdio>
#include <cstdlib>
namespace b=gears_torque_bow_main_grip;namespace vr=gears_vr;namespace am=gears_torque_bow_arrow_mount;
unsigned checks{};
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(false)
double distance(vr::Vector a,vr::Vector c){const auto d=am::sub(a,c);return std::hypot(std::hypot(d.x,d.y),d.z);}
std::array<float,16> matrix(vr::Matrix const& r,double scale,vr::Vector p){
 std::array<float,16> out{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out[i*4+j]=float(r[j][i]*scale);
 out[12]=float(p.x);out[13]=float(p.y);out[14]=float(p.z);out[15]=1;return out;
}
int main(){
#ifdef BOW_LEGACY_PICKUP_CHECK
 const auto identity=vr::ue_matrix({});
 gears_weapon::WeaponMeshFollow first,second;const vr::Viewpoint legacy_right{{10,20,30},{}};
 C(first.bind_oriented(legacy_right,matrix(identity,.3,{10,20,30}),identity));
 C(second.bind_oriented(legacy_right,matrix(identity,.3,{13,20,30}),identity));vr::Viewpoint a{},c{};
 C(first.solve(legacy_right,a)&&second.solve(legacy_right,c));
 std::printf("Legacy Bow generic bind retains pickup displacement: %.9g UE units. Expected fixed original contact.\n",distance(a.position,c.position));
 C(distance(a.position,c.position)<1e-6);return 0;
#else
 for(double scale:{.2,.3,1.})for(double units:{30.,100.,400.})for(unsigned pose=0;pose<3;++pose){
  gears_weapon_lancer_wrist_mount::Profile native{};
  C(gears_weapon_lancer_wrist_mount::marcus115(gears_hand_authored_mount::marcus115_signature,scale,units,native));
  const vr::Viewpoint right{{20,-30,50},{.15*pose,-.3*pose,.2*pose}};
  const auto observed=matrix(vr::ue_matrix({.2,.4,.1}),scale,{5,6,7});auto other=observed;other[12]+=13;other[13]-=8;
  gears_weapon::WeaponMeshFollow fixed,again;
  C(b::bind(fixed,right,observed,scale,units,native.mount.calibration));C(b::bind(again,right,other,scale,units,native.mount.calibration));
  vr::Viewpoint solved{},repeat{};C(fixed.solve(right,solved)&&again.solve(right,repeat));C(distance(solved.position,repeat.position)<1e-8);
  const auto R=vr::ue_matrix(solved.rotation);const auto authored_position=am::add(solved.position,vr::transform(R,am::mul(b::wrist_in_mesh_raw,scale)));
  const auto authored_basis=vr::multiply(R,b::wrist_basis_in_mesh);vr::Quaternion hand_q{};
  C(vr::rig_tracking::detail::quaternion(vr::ue_matrix(right.rotation),hand_q));vr::Pose wrist{};
  C(gears_hand_wrist::native_wrist_target({right.position,hand_q},native.mount.calibration,units,wrist)==gears_hand_wrist::Status::okay);
  C(distance(wrist.position,authored_position)<1e-5);const auto native_basis=vr::rotation_matrix(wrist.orientation);
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)C(std::abs(authored_basis[i][j]-native_basis[i][j])<1e-8);
  C(!b::bind(fixed,right,observed,scale,units,native.mount.calibration));
  for(unsigned variant=0;variant<6;++variant){auto bad=observed;
   if(variant==0)bad[0]*=1.01f;if(variant==1)for(unsigned i=0;i<3;++i)bad[i]=-bad[i];
   if(variant==2)bad[3]=1;if(variant==3)bad[15]=0;if(variant==4)bad[5]=NAN;if(variant==5)bad[4]+=.02f;
   gears_weapon::WeaponMeshFollow reject;C(!b::bind(reject,right,bad,scale,units,native.mount.calibration)&&!reject.bound());
  }
  gears_weapon::WeaponMeshFollow wrong;C(!b::bind(wrong,right,observed,scale*1.1,units,native.mount.calibration));
 }
 std::printf("Torque Bow fixed main contact: %u checks passed; original wrist matches existing Native calibration, no pickup offset, no game/skin acceptance.\n",checks);
#endif
}
