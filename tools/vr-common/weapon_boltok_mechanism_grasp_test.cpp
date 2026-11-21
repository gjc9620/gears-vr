#include "weapon_boltok_mechanism_grasp.h"
#include <cstdio>
#include <fstream>
#include <cstring>
namespace draft=gears_boltok_mechanism_grasp;namespace vr=gears_vr;namespace weapon=gears_weapon;namespace api=gears_hand_authored_grasp;
unsigned test_checks{},test_failures{};
#define CHECK(x) do{++test_checks;if(!(x)){++test_failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
template<class T>auto test_bytes(T const& value){std::array<unsigned char,sizeof(T)> data{};std::memcpy(data.data(),&value,sizeof value);return data;}
double qerror(vr::Quaternion a,vr::Quaternion b){const double s=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0?-1.:1.;return std::abs(a.x-s*b.x)+std::abs(a.y-s*b.y)+std::abs(a.z-s*b.z)+std::abs(a.w-s*b.w);}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 std::array<gears_hand_authored_mount::ReferenceRecord68,115> reference{};std::ifstream file(argv[1],std::ios::binary);
 CHECK(bool(file.read(reinterpret_cast<char*>(reference.data()),sizeof reference))&&file.peek()==EOF);
 gears_hand_authored_mount::CanonicalGeometry canonical{};CHECK(gears_hand_authored_mount::canonical_geometry(reference.data(),reference.size(),canonical));
 api::Binding binding{};binding.geometry_signature=gears_hand_authored_mount::marcus115_signature;
 for(unsigned i=0;i<15;++i){float q[4]{};std::memcpy(q,canonical.data()+(12+i)*36+8,16);binding.reference[i]={q[0],q[1],q[2],q[3]};}
 gears_weapon_hand::Frame source{};source.controllers.sequence=123;source.controllers.sync_ns=987654321;const auto before=test_bytes(source);
 api::Request request{};CHECK(draft::held_request(source,{{1,2,3},{}},1.,100.,request));CHECK(api::reference_matches(request,binding));
 CHECK(request.side==api::Side::left&&request.mode==api::Mode::left_part&&request.kind==api::object::Kind::magazine&&!request.proximity&&!request.has_trigger_endpoint);
 CHECK(test_bytes(request.source)==before&&test_bytes(source)==before&&request.profile_id==0x424f4c544f4b0002ULL);
 api::Joints joints{};CHECK(api::rotations(request,binding,0,joints));for(unsigned i=0;i<15;++i)CHECK(qerror(joints[i],draft::pinch[i])<1e-12);
 // Independently retained fitted grip MATRIX, before quaternion conversion.
 const double axes[3][3]={{-.5707077168118322,-.29395566254082267,-.7667351370790382},{.7944462508078404,.03854142176949945,-.6061103145346989},{.207720621385532,-.9550416887592769,.21153608718853847}};
 const auto mat=vr::rotation_matrix(draft::grip_in_support_raw.orientation);for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)CHECK(std::abs(mat[r][c]-axes[r][c])<1e-12);
 const vr::Vector pivot{2.089574098587036,21.655614852905273,5.3229660987854};
 const vr::Vector original_grip{12.401876527273325,23.457264515566884,10.810835635585912};
 const vr::Vector original_wrist{13.569515048870386,18.239594648050463,9.762646760023838};
 CHECK(weapon::length(weapon::difference(weapon::sum(pivot,draft::grip_in_support_raw.position),original_grip))<1e-12);
 for(double units:{50.,100.,200.})for(unsigned root_case=0;root_case<4;++root_case){
  const auto root=vr::normalized(vr::Quaternion{.1*root_case,-.17,.23,.9});const vr::Vector root_position{1.1,-2.2,.8};
  for(unsigned degree=0;degree<=80;degree+=5){
   const double half=degree*3.14159265358979323846/360.;const vr::Quaternion swing{0,std::sin(half),0,std::cos(half)};
   const auto rotated=vr::normalized(vr::multiply(root,swing));
   const vr::Pose support{weapon::sum(root_position,weapon::rotate(root,weapon::scaled(pivot,1/units))),rotated};
   vr::Pose actual{};CHECK(draft::cosmetic_grip_world(support,1.,units,actual));
   const auto local=weapon::sum(pivot,weapon::rotate(swing,weapon::difference(original_grip,pivot)));
   const auto expected=weapon::sum(root_position,weapon::rotate(root,weapon::scaled(local,1/units)));
   CHECK(weapon::length(weapon::difference(actual.position,expected))<1e-12);
   CHECK(qerror(actual.orientation,vr::multiply(rotated,draft::grip_in_support_raw.orientation))<1e-12);
   CHECK(draft::held_request(source,support,1.,units,request)&&test_bytes(request.source)==before);
   CHECK(weapon::length(weapon::difference(request.grip.position,weapon::scaled(actual.position,units)))<1e-10);
   // Use the ORIGINAL HOST raw grip, independently of any C0 skin output.
   // Fixed local witness must land on the original moving Object02 origin.
   vr::Vector contact{};CHECK(draft::mechanical_contact_from_raw_grip(request.grip,1.,units,contact));
   const vr::Vector cylinder{0.,21.472944259643555,8.96826696395874};
   const auto local_contact=weapon::sum(pivot,weapon::rotate(swing,weapon::difference(cylinder,pivot)));
   const auto expected_contact=weapon::sum(weapon::scaled(root_position,units),weapon::rotate(root,local_contact));
   CHECK(weapon::length(weapon::difference(contact,expected_contact))<1e-9);
   gears_hand_authored_mount::Mount mount{};CHECK(gears_hand_authored_mount::marcus115(binding.geometry_signature,gears_hand_authored_mount::Side::left,1.,units,mount));
   gears_hand_object_grasp::Target target{};target.kind=request.kind;target.grip=request.grip;target.weight=1.;target.preset=api::object::Preset::authored;target.has_authored_pose=true;target.authored_pose=request.closed;
   const vr::Pose physical{weapon::scaled(support.position,units),support.orientation};vr::Pose wrist{{91,92,93},{}};const auto wrist_before=test_bytes(wrist);
   const bool fitted=gears_hand_object_grasp::apply(target,physical,mount.calibration,units,wrist);
   const auto local_wrist=weapon::sum(pivot,weapon::rotate(swing,weapon::difference(original_wrist,pivot)));
   const auto expected_wrist=weapon::sum(weapon::scaled(root_position,units),weapon::rotate(root,local_wrist));
   // This deliberately distant pivot-centred HOST input is 11.82 raw units
   // from the cosmetic grip. At units50 that exceeds the existing22cm
   // positional bound: preserve the refusal, never widen it for this draft.
   if(units==50.)CHECK(!fitted&&test_bytes(wrist)==wrist_before);
   else {CHECK(fitted);CHECK(weapon::length(weapon::difference(wrist.position,expected_wrist))<1e-9);}
  }
 }
 const vr::Pose sentinel{{91,92,93},{}};const auto request_before=test_bytes(request);
 for(unsigned negative=0;negative<8;++negative){vr::Pose out=sentinel;auto support=sentinel;double scale=1.,units=100.;
  if(negative==0)scale=0;if(negative==1)scale=.5;if(negative==2)scale=2;if(negative==3)scale=NAN;
  if(negative==4)units=0;if(negative==5)units=501;if(negative==6)support.orientation={0,0,0,0};if(negative==7)support.position.x=INFINITY;
  CHECK(!draft::cosmetic_grip_world(support,scale,units,out)&&test_bytes(out)==test_bytes(sentinel));
  CHECK(!draft::held_request(source,support,scale,units,request)&&test_bytes(request)==request_before);
  vr::Vector contact{91,92,93};const auto contact_before=test_bytes(contact);CHECK(!draft::mechanical_contact_from_raw_grip(support,scale,units,contact)&&test_bytes(contact)==contact_before);
 }
 auto bad=binding;bad.geometry_signature.back()^=1;CHECK(!api::reference_matches(request,bad));bad=binding;bad.reference[14].x+=.01;CHECK(!api::reference_matches(request,bad));
 CHECK(test_bytes(source)==before);
 std::printf("Boltok mechanism draft: %u checks %u failures; retained actual reference, original pivot/current support transform, invalid inputs preserve output; no runtime/skin acceptance\n",test_checks,test_failures);
 return test_failures?1:0;
}
