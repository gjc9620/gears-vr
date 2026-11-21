#include "../vr-common/weapon_gnasher_round_pose.h"
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/weapon_part_input_catalog.h"
#include "../vr-common/body_slots.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cwchar>
#ifdef NDEBUG
#error GnasherRoundTest requires active checks
#endif
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::abort();}}while(false)
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;namespace vr=gears_vr;
namespace w=gears_weapon;namespace pose=gears_gnasher_round_pose;
FILE* log_file{};uptr base=1;
bool source_ok=true,asset_ok=true,rig_ok=true,hide_ok=true,destroy_ok=true,material_ok=true,main_held=true;
bool hand_snapshot_ok=true,raw_ownership_ok=true;
unsigned creates{},poses{},hides{},destroys{},material_count{},resource_metadata_reads{};
namespace gnasher_runtime {bool route_enabled=true;bool round_route_enabled() noexcept{return route_enabled;}}
hand::ObjectIdentity object(uptr p){return {p,0x8000,0x9000,unsigned(p/8),7,0,1};}
namespace lancerreload_runtime {std::atomic<bool> stop_requested{};}
namespace nativeequipment {w::ArmspanCalibration calibration=w::default_body_slot_layout();bool current(void*,hand::Frame const&){return source_ok;}}
namespace vrequip {struct Model {std::uint64_t hold_generation()const{return 1;}bool held_for(uptr,uptr,uptr)const{return main_held;}}model;}
namespace unified_weapon_fire {bool manual_profile(){return true;}template<class R>bool retained_metadata(R const&){return true;}}
namespace playground {bool instance(uptr,wchar_t const*){return true;}}
namespace gripdamage_metadata {
struct Snapshot {uptr module_base{};unsigned objects{},names{};};enum class Stage{final_check};
struct Reader {Reader(Snapshot&,Stage){}};struct PointerField{uptr value{};};
bool tables(uptr,unsigned&,unsigned&){return true;}
}
namespace weaponhand {
hand::GripSnapshot observed_grip{};
struct Client {bool snapshot_on_gt(hand::GripSnapshot& out){out=observed_grip;return hand_snapshot_ok;}}client;
std::int64_t clock_ns(void*){return 2000000000;}
bool object(gripdamage_metadata::Reader&,hand::ObjectIdentity const&){return true;}
bool read_pointer(gripdamage_metadata::Reader&,uptr,wchar_t const*,gripdamage_metadata::PointerField& out,bool){out.value=0;return true;}
}
namespace gears_gnasher_raw {
bool ownership_matches(hand::Frame const&,hand::GripSnapshot const&,std::int64_t){return raw_ownership_ok;}
}
namespace lancerreload_inventory {
struct Identity {hand::ObjectIdentity world{},pawn{},weapon{};std::uint64_t context{},hold{};
 bool valid()const{return hand::identity_valid(weapon)&&context&&hold;}
 bool operator==(Identity const& b)const{return hand::same(world,b.world)&&hand::same(pawn,b.pawn)&&hand::same(weapon,b.weapon)&&context==b.context&&hold==b.hold;}};
Identity identity(hand::Frame const& f){return {f.world,f.pawn,f.weapon,f.key.native_started_qpc,1};}
bool same_instance_across_hold(Identity a,Identity b){if(!a.valid()||!b.valid()||b.hold<a.hold)return false;b.hold=a.hold;return a==b;}
}
namespace nativelancerfollow {
struct Key {hand::ObjectIdentity weapon{},component{},asset{};std::uint64_t hold_generation{};}bound_key;
struct Mount {bool bound()const{return true;}}mount;
bool bound_key_known=true;double bound_units=100;
std::uint64_t bound_session{},bound_space{},bound_input{},bound_world{},bound_reference{};
}
namespace lancermain_asset {
struct Evidence {hand::ObjectIdentity component{},asset{};};enum class HeldAsset{gnasher,other};enum class AssetScope{basic_hold};enum class Status{ready,rejected};
Status read(hand::Frame const&,Evidence& out,AssetScope,HeldAsset* kind){++resource_metadata_reads;out={nativelancerfollow::bound_key.component,nativelancerfollow::bound_key.asset};*kind=asset_ok?HeldAsset::gnasher:HeldAsset::other;return asset_ok?Status::ready:Status::rejected;}
bool identify(gripdamage_metadata::Reader&,uptr p,hand::ObjectIdentity& out){out=object(p);return true;}
}
namespace gears_rig_tracking {
enum class Reply{accepted,rejected};inline constexpr unsigned left_grip_pose=2;
struct Frame {unsigned world_pose_mask{};double units_per_metre{};std::array<vr::Pose,2> world_grip{};}prepared;
}
namespace handrig {struct Result{gears_rig_tracking::Reply reply;};Result read(gears_rig_tracking::Frame& out,unsigned,std::uint32_t){out=gears_rig_tracking::prepared;return {rig_ok?gears_rig_tracking::Reply::accepted:gears_rig_tracking::Reply::rejected};}}
namespace gears_equipment_rig {enum class Status{matched,rejected};Status match(hand::Frame const&,gears_rig_tracking::Frame const&,std::int64_t,std::uint32_t){return rig_ok?Status::matched:Status::rejected;}}
namespace weapon_part_inputs {
enum class Result{ready,rejected};
Result load(char const* id,w::part_geometry::Conversion const& c,w::part_geometry::Prepared& out){
 CHECK(std::string(id)=="gnasher-round-lod0"&&c.units_per_metre==100&&c.axes[1][2]==1&&c.axes[2][1]==-1);out.calibration_generation=c.calibration_generation;return Result::ready;
}
}
namespace weapon_part_assets {
struct Handle{unsigned slot{},generation{};};struct Resource{bool lighting_bound{},visible{},alive{};}resource;
enum class Creation{rejected,pending,ready};std::atomic<bool> stop_requested{};
vr::Viewpoint last_pose{};
Resource* find(Handle h){return h.generation&&resource.alive?&resource:nullptr;}
bool destroy(Handle,std::uint64_t){++destroys;if(!destroy_ok)return false;resource={};return true;}
Creation create(hand::Frame const&,std::uint64_t,w::part_geometry::Prepared const&,std::vector<hand::ObjectIdentity> const& m,Handle& out){++creates;material_count=unsigned(m.size());resource.alive=true;out={0,creates};return Creation::pending;}
Creation advance(hand::Frame const&,Handle,std::uint64_t){return Creation::ready;}
bool held_weapon_material(hand::Frame const&,hand::ObjectIdentity& out){out=object(0x7000);return material_ok;}
bool bind_light_environment(hand::Frame const&,Handle,std::uint64_t,hand::ObjectIdentity const& environment,bool empty){CHECK(!environment.address&&empty);resource.lighting_bound=true;return true;}
bool set_visible(Handle,std::uint64_t,bool visible){CHECK(!visible);++hides;if(!hide_ok)return false;resource.visible=false;return true;}
bool set_pose(hand::Frame const&,Handle,std::uint64_t,hand::Decision const&,vr::Viewpoint const& p,bool visible){CHECK(resource.lighting_bound&&visible);++poses;resource.visible=true;last_pose=p;return true;}
}
#include "weapon_gnasher_round.inl"
struct Fixture {
 hand::Frame f{};hand::Decision d{};pose::Mount mount;w::GnasherMechanicalConfig config;
 Fixture(){using namespace gnasher_round;
  source_ok=asset_ok=rig_ok=hide_ok=destroy_ok=material_ok=main_held=true;hand_snapshot_ok=raw_ownership_ok=true;creates=poses=hides=destroys=material_count=resource_metadata_reads=0;gnasher_runtime::route_enabled=true;
  weapon_part_assets::stop_requested=false;lancerreload_runtime::stop_requested=false;weapon_part_assets::resource={};
  handle={};identity=released_identity=last_item_identity={};generation=item_serial=last_item=released_item=last_sequence=last_frame=0;ready=false;held={};held_mount={};
  f.key={11,22,33,1,10,1,44,55};f.world=object(0x6000);f.pawn=object(0x6020);f.weapon=object(0x5000);f.reference_generation=8;f.controllers.profile_generation=7;
  f.stereo.head.position={0,1.7,0};f.controllers.hands[0].grip.position={-.2295,1.585,.05};f.press_token=20;f.left_tracked=f.left_grip_known=f.left_grip_down=true;f.controllers.sequence=30;
  d.key=f.key;d.press_token=f.press_token;d.lease={hand::LeaseKind::ammo,0,f.key.lifecycle,f.press_token};d.new_grant=1;d.release_required=0;d.status=hand::DecisionStatus::granted;
  weaponhand::observed_grip={};weaponhand::observed_grip.left_hand=d.lease;
  nativelancerfollow::bound_key={f.weapon,object(0x6100),object(0x6200),1};nativelancerfollow::bound_units=100;
  nativelancerfollow::bound_session=f.stereo.session;nativelancerfollow::bound_space=f.stereo.space_epoch;
  nativelancerfollow::bound_input=f.controllers.input_epoch;nativelancerfollow::bound_world=f.key.world_generation;nativelancerfollow::bound_reference=f.reference_generation;
  gears_rig_tracking::prepared={gears_rig_tracking::left_grip_pose,100,{{{{100,250,-300},{}},{}}}};
  mount={9,8,{{.01,-.02,.03},{}}};config.calibration_id=9;config.lever_calibrated=config.feed_calibrated=true;config.capacity=8;config.game_ticks_per_second=1000000;
  config.lever.left_front_grip_in_main.position={0,0,.08};config.lever.right_handle_home_in_main={0,-.2,0};config.lever.hinge_axis_in_main={1,0,0};config.feed.item_in_left_grip=mount.tip_in_grip_metres();CHECK(config.valid());
 }
 void prepare(){gnasher_round::tick(f);CHECK(creates==1&&!gnasher_round::ready&&!weapon_part_assets::resource.visible);++f.key.frame;d.key=f.key;gnasher_round::tick(f);CHECK(gnasher_round::ready&&material_count==5);}
 bool capture(){return gnasher_round::capture(f,d,20,mount,config);}
};
int main(){namespace r=gnasher_round;
 {Fixture x;gnasher_runtime::route_enabled=false;r::tick(x.f);
  CHECK(!resource_metadata_reads&&!creates&&!r::generation&&!r::item_serial);
  gnasher_runtime::route_enabled=true;x.prepare();CHECK(r::ready&&creates==1);}
 {Fixture x;x.prepare();CHECK(x.capture());gnasher_runtime::route_enabled=false;
  lancerreload_runtime::stop_requested=true;r::tick(x.f);
  CHECK(!r::generation&&!r::item_serial&&destroys==1&&!poses);}
 {Fixture x;x.prepare();CHECK(!r::captured_for(x.f,20));CHECK(x.capture());
  CHECK(r::captured_for(x.f,20)&&!r::captured_for(x.f,21));source_ok=false;CHECK(r::retains_ammo_route(x.f));source_ok=true;
  CHECK(!x.capture());CHECK(r::show(x.f,x.d)&&poses==1);CHECK(r::show(x.f,x.d)&&poses==1);
  const auto p=weapon_part_assets::last_pose.position;CHECK(std::abs(p.x-101)<1e-10&&std::abs(p.y-248)<1e-10&&std::abs(p.z+297)<1e-10);
  const auto before=r::identity;hide_ok=false;CHECK(!r::release_for_stow(before,20)&&r::item_serial==20);hide_ok=true;
  CHECK(r::release_for_stow(before,20)&&!r::item_serial&&!weapon_part_assets::resource.visible&&!r::captured_for(x.f,20)&&!r::retains_ammo_route(x.f));
  const auto calls=hides;CHECK(r::release_for_stow(before,20)&&hides==calls);CHECK(!r::release_for_stow(before,21));
  auto wrong=before;++wrong.context;CHECK(!r::release_for_stow(wrong,20));CHECK(!x.capture());}
 for(unsigned bad=0;bad<7;++bad){Fixture x;x.prepare();CHECK(x.capture());
  if(bad==0)hand_snapshot_ok=false;if(bad==1)raw_ownership_ok=false;if(bad==2)++weaponhand::observed_grip.left_hand.id;
  if(bad==3)++x.f.press_token;if(bad==4)x.f.left_grip_down=false;if(bad==5)x.f.left_tracked=false;if(bad==6)x.f.left_grip_known=false;
  CHECK(!r::captured_for(x.f,20));CHECK(r::item_serial==20&&!hides);}
 for(unsigned bad=0;bad<5;++bad){Fixture x;x.prepare();if(bad==0)x.d.new_grant=0;if(bad==1)++x.mount.reference_generation;if(bad==2)x.config.feed.item_in_left_grip.position.y+=.01;
  if(bad==3)x.f.controllers.hands[0].grip.position={};if(bad==4)x.d.lease.kind=hand::LeaseKind::support;
  CHECK(!x.capture()&&!r::item_serial&&!poses);}
 {Fixture x;x.prepare();CHECK(x.capture());rig_ok=false;CHECK(!r::show(x.f,x.d)&&!poses&&r::item_serial==20);CHECK(r::release_for_stow(r::identity,20));}
 {Fixture x;x.prepare();CHECK(x.capture());gears_rig_tracking::prepared.units_per_metre=99;CHECK(!r::show(x.f,x.d)&&!poses);}
 {Fixture x;x.prepare();CHECK(x.capture());const auto id=r::identity;destroy_ok=false;source_ok=false;r::tick(x.f);CHECK(r::generation&&r::item_serial==20);
  destroy_ok=true;r::tick(x.f);CHECK(!r::generation&&!r::item_serial&&r::release_for_stow(id,20));}
 {Fixture x;x.prepare();CHECK(x.capture());lancerreload_runtime::stop_requested=true;r::tick(x.f);CHECK(!r::generation&&!r::item_serial&&destroys==1&&!poses);}
 {Fixture x;material_ok=false;r::tick(x.f);CHECK(!creates&&!r::generation);}
 {Fixture x;main_held=false;r::tick(x.f);CHECK(!creates&&!r::generation);}
 {Fixture x;x.prepare();CHECK(x.capture());const auto id=r::identity;++nativelancerfollow::bound_space;r::tick(x.f);
  CHECK(!r::generation&&r::release_for_stow(id,20)&&!poses);}
 {Fixture x;x.mount.root_in_grip_metres.orientation={0,.6,0,.8};x.config.feed.item_in_left_grip=x.mount.tip_in_grip_metres();
  CHECK(x.mount.matches_feed(x.config));const vr::Pose raw{{100,-200,300},{.2,.3,.4,std::sqrt(.71)}};
  for(double units:{10.,100.,500.}){vr::Pose root{};CHECK(pose::world_root(x.mount,raw,units,root));
   const auto display_tip=w::scaled(w::sum(root.position,w::rotate(root.orientation,{0,pose::authored_length_metres*units,0})),1/units);
   const auto physical_tip=w::sum(w::scaled(raw.position,1/units),w::rotate(raw.orientation,x.config.feed.item_in_left_grip.position));
   CHECK(w::length(w::difference(display_tip,physical_tip))<1e-10);
  }
  vr::Pose invalid{};CHECK(!pose::world_root(x.mount,raw,9,invalid));++x.config.calibration_id;CHECK(!x.mount.matches_feed(x.config));}
 // Independent actual authored input dimensions, not generated fixture data.
 std::ifstream file("assets/weapon-6dof/offline-v2/engine-input/gnasher-round-lod0.vtx60",std::ios::binary);CHECK(file.good());
 std::vector<w::part_geometry::Vertex> vertices(2880);file.read(reinterpret_cast<char*>(vertices.data()),std::streamsize(vertices.size()*60));CHECK(file.gcount()==172800);
 double low=1,high=-1,radius=0;for(auto const& v:vertices){low=(std::min)(low,double(v.position[2]));high=(std::max)(high,double(v.position[2]));radius=(std::max)(radius,std::hypot(double(v.position[0]),double(v.position[1])));}
 CHECK(std::abs(low)<1e-8&&std::abs(high-pose::authored_length_metres)<1e-8&&std::abs(radius-pose::authored_radius_metres)<1e-8);
 std::puts("Production Gnasher closed-round resource: hidden original-engine preparation,5 material slots, same raw mount/tip, actual authored dimensions, strict real ammo grant, no repeated pose, withdrawal receipt, deferred cleanup and stop checks passed (engine/rig boundaries injected; no game ammo writes)");
}
