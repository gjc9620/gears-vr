#define NOMINMAX
#include "../vr-common/weapon_boomshot_insertion.h"
#include "../vr-common/weapon_hand_client.h"
#include "weapon_rig_binding.h"
#define now rig_fixture_time
#define main previous_rig_cases
#include "weapon_rig_binding_test.cpp"
#undef main
#undef now
#undef CHECK
#define main previous_pose_reader_cases
#include "weapon_boomshot_pose_read_test.cpp"
#undef main
#include "../vr-common/weapon_boomshot_insertion.h"
namespace {
namespace lancerreload_inventory {
struct Counts {int capacity{1},used{},reserve{2};bool infinite{};};
bool same_instance_across_hold(Identity const& a,Identity const& b)noexcept{return a.valid()&&b.valid()&&abi::same(a.weapon,b.weapon)&&b.hold>=a.hold;}
}
namespace nativelancerfollow {double bound_units=100;}
namespace hammerburst_runtime {
// Existing matrix-to-pose engine/helper boundary. Metadata Reader still checks
// strict real affine matrices; this fixture extracts their measured basis.
bool matrix(pose::Matrix const& raw,gears_vr::Pose& out,double& scale)noexcept {
 if(!pose::affine(raw))return false;scale=std::hypot(double(raw[0]),double(raw[1]),double(raw[2]));std::array<std::array<double,3>,3> basis{};
 for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)basis[j][i]=raw[i*4+j]/scale;
 out.position={raw[12],raw[13],raw[14]};return gears_vr::rig_tracking::detail::quaternion(basis,out.orientation);
}
}
#define BOOMSHOT_ORIGINAL_PREPARE_BOUNDARY_FIXTURE 1
#include "weapon_boomshot_original.inl"
#undef BOOMSHOT_ORIGINAL_PREPARE_BOUNDARY_FIXTURE
unsigned seating_fault{},socket_calls{};uptr carrier_component{},carrier_asset{};
namespace boomshot_original {
bool prepare(abi::Frame const& f,Source& out)noexcept {
 out={};if(seating_fault==1||!boomshot_parts::read(f,out.parts))return false;
 out.carrier_component=current_fixture->identity(carrier_component);out.carrier=current_fixture->identity(carrier_asset);
 if(seating_fault==5)++out.parts.component.registry_index;return true;
}
}
namespace boomshot_socket {
struct Observation {Source source{};gears_vr::Pose world{};abi::FrameKey key{};};
bool read(abi::Frame const& f,Observation& out)noexcept {
 ++socket_calls;if(seating_fault==2)return false;pose::Source actual{};
 if(!current_fixture->inspect(actual))return false;meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};Reads reads{};
 la::Evidence asset{actual.component,actual.asset};Observation candidate{};double scale{};
 if(!meta::tables(base,schema.objects,schema.names)||!metadata(f,asset,r,reads,candidate.source)||
    !hammerburst_runtime::matrix(actual.world[1],candidate.world,scale))return false;
 candidate.key=f.key;if(seating_fault==3)candidate.world.position.x+=.003;if(seating_fault==4)candidate.world.orientation={1,0,0,0};
 if(seating_fault==6){current_fixture->bones[1][13]+=.1f;}
 out=candidate;return true;
}
}
#include "weapon_boomshot_seating.inl"
namespace seat=boomshot_seating;
struct SeatingFixture:PoseFixture {
 uptr socket_class{},socket{},location{},rotation{},scale_field{},socket_inner{},static_property{};std::array<uptr,1> sockets{};
 SeatingFixture(){
  setup(*this);seating_fault=socket_calls=0;seat::binding={};seat::next_generation=1;nativelancerfollow::bound_units=100;
  const auto name_type=node(L"NameProperty",class_type,core),object=cls(L"Object",core);
  const auto vector=node(L"Vector",script_type,object),rotator=node(L"Rotator",script_type,object);put(vector,0x88,12u);put(rotator,0x88,12u);
  socket_class=cls(L"SkeletalMeshSocket",engine);socket=node(L"MagazineSocket",socket_class,asset);
  property(socket_class,L"SocketName",name_type,0x100,8);property(socket_class,L"BoneName",name_type,0x108,8);
  location=property(socket_class,L"RelativeLocation",struct_type,0x110,12);rotation=property(socket_class,L"RelativeRotation",struct_type,0x120,12);
  scale_field=property(socket_class,L"RelativeScale",struct_type,0x130,12);put(location,0xd0,vector);put(rotation,0xd0,rotator);put(scale_field,0xd0,vector);
  put(socket,0x100,text(L"Magazine"));put(socket,0x108,text(L"Boomshot_Magazine"));put(socket,0x130,std::array<float,3>{1,1,1});sockets={socket};
  const auto sockets_field=property(asset_class,L"Sockets",array_type,0x140,16);socket_inner=node(L"SocketInner",object_type,sockets_field);
  put(socket_inner,0x70,1u);put(socket_inner,0x74,8u);put(sockets_field,0xd0,socket_inner);put(asset,0x140,meta::Table{reinterpret_cast<uptr>(sockets.data()),1,1});
  const auto static_class=cls(L"StaticMeshComponent",engine,primitive),static_mesh_class=cls(L"StaticMesh",engine);
  carrier_component=node(L"MagazineMesh0",static_class,weapon);carrier_asset=node(L"Locust_Boomshot_Magazine",static_mesh_class,asset);
  property(weapon_class,L"MagazineMesh",component_type,0x1c0,8);static_property=property(static_class,L"StaticMesh",object_type,0x100,8);
  put(weapon,0x1c0,carrier_component);put(carrier_component,0x100,carrier_asset);tables();
  Fixture temporal;const auto ids=frame;frame=temporal.hand;frame.world=ids.world;frame.controller=ids.controller;frame.pawn=ids.pawn;frame.mesh=ids.mesh;frame.weapon=ids.weapon;
  frame.key.thread_id=GetCurrentThreadId();fresh();
 }
 void fresh(){const auto time=weaponhand::clock_ns(nullptr);frame.now_ns=time-1000000;frame.stereo.received_ns=time-2000000;frame.controllers.received_ns=time-3000000;frame.controllers.sync_ns=time-4000000;}
 bool certify(){fresh();return seat::certify(frame);}
 bool read(gears_weapon::boomshot_insertion::Geometry& out){fresh();return seat::read(frame,out);}
};
}
int main(){unsigned count{};
 {auto f=std::make_unique<SeatingFixture>();gears_weapon::boomshot_insertion::Geometry g{};assert(!f->read(g)&&!g.verified);
  assert(f->certify()&&f->read(g)&&g.verified&&g.id==1&&std::abs(g.seated_in_root_m.position.y-.3)<1e-6&&std::abs(g.seated_in_root_m.position.z-.02)<1e-6);
  assert(std::abs(g.seated_in_root_m.orientation.x)> .999999&&g.outward.z==-1&&g.travel_m==.155);
  original_calls=socket_calls=source_current_calls=0;assert(f->read(g)&&!original_calls&&!socket_calls&&!source_current_calls);++count;
  f->component_matrix[12]+=7;Memory::put(f->component,0x180,f->component_matrix);assert(f->read(g)&&std::abs(g.seated_in_root_m.position.y-.3)<1e-6);++count;
  for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)f->bones[1][row*4+col]=0;
  assert(f->read(g)&&g.verified&&!original_calls&&!socket_calls&&!source_current_calls);++count;
 }
 for(unsigned fault=1;fault<=6;++fault){auto f=std::make_unique<SeatingFixture>();seating_fault=fault;assert(!f->certify()&&!seat::binding.known);++count;}
 for(unsigned bad=0;bad<10;++bad){auto f=std::make_unique<SeatingFixture>();assert(f->certify());
  if(bad==0)Memory::put(f->socket,0x120,std::array<int,3>{0,16384,0});if(bad==1)Memory::put(f->socket,0x110,std::array<float,3>{.1f,0,0});
  if(bad==2)Memory::put(f->socket,0x130,std::array<float,3>{2,1,1});if(bad==3)Memory::put(carrier_component,0x100,f->asset);
  if(bad==4)++seat::binding.identity.hold;if(bad==5)f->records[1][60]=1;if(bad==6)nativelancerfollow::bound_units=200;
  if(bad==7){f->component_matrix[0]*=2;f->component_matrix[1]*=2;Memory::put(f->component,0x180,f->component_matrix);}
  if(bad==8)f->frame.recentered=1;if(bad==9)game_thread.store(0);
  gears_weapon::boomshot_insertion::Geometry g{};g.id=999;assert(!f->read(g)&&!g.id&&!g.verified);if(bad==9)game_thread.store(GetCurrentThreadId());++count;
 }
 {auto f=std::make_unique<SeatingFixture>();f->bones[1][13]+=.03f;assert(!f->certify());++count;}
 {auto f=std::make_unique<SeatingFixture>();for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)f->bones[1][row*4+col]=0;
  assert(!f->certify()&&!seat::binding.known);++count;}
 {auto f=std::make_unique<SeatingFixture>();seat::next_generation=UINT64_MAX;assert(!f->certify());++count;}
 {auto f=std::make_unique<SeatingFixture>();assert(f->certify());assert(seat::binding.identity.hold>1);
  --seat::binding.identity.hold;--pose::certification.identity.hold;
  for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)f->bones[1][row*4+col]=0;
  original_calls=socket_calls=0;assert(f->certify()&&seat::binding.generation==2&&seat::binding.identity==lancerreload_inventory::identity(f->frame));
  gears_weapon::boomshot_insertion::Geometry g{};assert(f->read(g)&&g.verified&&std::abs(g.seated_in_root_m.position.y-.3)<1e-6&&socket_calls==0);++count;}
 std::printf("Boomshot authored seating source: %u cases passed; actual pose Reader/reference/socket metadata and original carrier-pointer witness; authored X180/pivot/clearance bound only after current original-getter boundary, fresh data reads contain no getter/client/current call, changed source fails closed; engine getters/matrix conversion stubbed, no live fit acceptance\n",count);return 0;
}
