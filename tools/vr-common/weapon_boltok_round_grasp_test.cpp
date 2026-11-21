#include "weapon_boltok_round_grasp.h"
#include "weapon_boltok_visual_geometry.h"
#include <cstdio>
#include <fstream>
#include <cstring>
namespace draft=gears_boltok_round_grasp;namespace vr=gears_vr;namespace weapon=gears_weapon;namespace api=gears_hand_authored_grasp;
unsigned test_checks{},test_failures{};
#define CHECK(x) do{++test_checks;if(!(x)){++test_failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
template<class T>auto test_bytes(T const& value){std::array<unsigned char,sizeof(T)> data{};std::memcpy(data.data(),&value,sizeof value);return data;}
double test_qerror(vr::Quaternion a,vr::Quaternion b){const double s=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0?-1.:1.;return std::abs(a.x-s*b.x)+std::abs(a.y-s*b.y)+std::abs(a.z-s*b.z)+std::abs(a.w-s*b.w);}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 std::array<gears_hand_authored_mount::ReferenceRecord68,115> reference{};std::ifstream file(argv[1],std::ios::binary);
 CHECK(bool(file.read(reinterpret_cast<char*>(reference.data()),sizeof reference))&&file.peek()==EOF);
 gears_hand_authored_mount::CanonicalGeometry canonical{};CHECK(gears_hand_authored_mount::canonical_geometry(reference.data(),reference.size(),canonical));
 api::Binding binding{};binding.geometry_signature=gears_hand_authored_mount::marcus115_signature;
 for(unsigned i=0;i<15;++i){float q[4]{};std::memcpy(q,canonical.data()+(12+i)*36+8,16);binding.reference[i]={q[0],q[1],q[2],q[3]};}
 gears_weapon_hand::Frame source{};source.controllers.sequence=123;source.controllers.sync_ns=987654321;const auto source_before=test_bytes(source);
 api::Request request{};const vr::Pose physical{{11,22,33},{}};CHECK(draft::held_request(source,physical,request));CHECK(api::reference_matches(request,binding));
 CHECK(request.side==api::Side::left&&request.mode==api::Mode::left_part&&request.kind==api::object::Kind::loading&&!request.proximity&&!request.has_trigger_endpoint);
 CHECK(test_bytes(request.source)==source_before&&test_bytes(source)==source_before&&test_qerror(request.grip.orientation,physical.orientation)<1e-12);
 api::Joints local{};CHECK(api::rotations(request,binding,0,local));
 for(unsigned i=0;i<15;++i)CHECK(api::unit(local[i])&&test_qerror(local[i],draft::pinch[i])<1e-12);
 // Saved authored MATRIX, independently compared with the emitted quaternion.
 const double authored_axes[3][3]={{.8071230039110001,.5903833132446106,6.938893903907228e-18},{.07725796607074341,-.10562067093050889,-.9914007668701901},{-.5853064694980703,.8001823650359369,-.13086068717991278}};
 const auto matrix=vr::rotation_matrix(draft::item_in_grip_raw.orientation);
 for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)CHECK(std::abs(matrix[row][col]-authored_axes[row][col])<1e-12);
 for(double units:{50.,100.,200.})for(unsigned turn=0;turn<4;++turn){
  const auto orientation=vr::normalized(vr::Quaternion{.1*turn,-.17,.23,.9});const vr::Pose world{{1.1,-2.2,.8},orientation};
  weapon::BoltokVisualGeometry geometry{1.,units,7};weapon::BoltokChamberGeometry ports{};CHECK(geometry.ports(ports));
  vr::Pose witness{};CHECK(draft::base_witness_in_grip_metres(1.,units,witness));
  for(unsigned slot=0;slot<6;++slot){
   auto port=ports.ports[slot];port.item_in_left_grip=witness;
   const auto port_position=weapon::sum(world.position,weapon::rotate(world.orientation,port.port_in_weapon.position));
   const auto port_orientation=vr::normalized(vr::multiply(world.orientation,port.port_in_weapon.orientation));
   const auto outward=weapon::rotate(world.orientation,port.outward_axis_in_weapon);
   weapon::FeedInsertionStroke stroke{},old_mount{};CHECK(stroke.arm(1,2,3,4)&&old_mount.arm(1,2,3,4));
   unsigned step=0;
   for(double depth:{.06,.04,.02,0.}){
    const auto target=weapon::sum(port_position,weapon::scaled(outward,depth));
    const auto grip_orientation=vr::normalized(vr::multiply(port_orientation,vr::inverse(witness.orientation)));
    const vr::Pose grip{weapon::difference(target,weapon::rotate(grip_orientation,witness.position)),grip_orientation};
    // HOST generated physical grip only; never a tracked/native observation.
    vr::Pose rendered{};CHECK(draft::item_world({weapon::scaled(grip.position,units),grip.orientation},1.,units,rendered));
    const auto rendered_witness=weapon::scaled(weapon::sum(rendered.position,weapon::rotate(rendered.orientation,{0,.3,0})),1/units);
    CHECK(weapon::length(weapon::difference(rendered_witness,target))<1e-12&&test_qerror(rendered.orientation,port_orientation)<1e-12);
    weapon::FeedSample sample{1,2,3,4,step+1,1000000000ll+step*20000000ll,world,grip,true,true};
    CHECK(stroke.observe(sample,port)==(step==3?weapon::FeedResult::inserted:weapon::FeedResult::approaching));
    CHECK(old_mount.observe(sample,ports.ports[slot])==weapon::FeedResult::rejected);++step;
   }
  }
 }
 const vr::Pose sentinel{{91,92,93},{}};
 for(unsigned negative=0;negative<8;++negative){vr::Pose out=sentinel;auto raw_grip=physical;double scale=1,units=100;
  if(negative==0)scale=0;if(negative==1)scale=.5;if(negative==2)scale=2;if(negative==3)scale=NAN;
  if(negative==4)units=0;if(negative==5)units=501;if(negative==6)raw_grip.orientation={0,0,0,0};if(negative==7)raw_grip.position.x=INFINITY;
  CHECK(!draft::item_world(raw_grip,scale,units,out)&&test_bytes(out)==test_bytes(sentinel));
 }
 for(double scale:std::array<double,3>{{.5,2.,double(NAN)}}){vr::Pose out=sentinel;CHECK(!draft::item_in_grip_metres(scale,100,out)&&test_bytes(out)==test_bytes(sentinel));CHECK(!draft::base_witness_in_grip_metres(scale,100,out)&&test_bytes(out)==test_bytes(sentinel));}
 std::printf("Boltok round draft: %u checks %u failures; original reference + explicit authored mount + raw rendered/feed equivalence; synthetic stroke inputs, no runtime integration/skin acceptance\n",test_checks,test_failures);
 return test_failures?1:0;
}
