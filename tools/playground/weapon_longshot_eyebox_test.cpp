#include "../vr-common/weapon_longshot_eye_contact.h"
#include "../vr-common/weapon_longshot_scope_view.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/rig_tracking_frame.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <crtdbg.h>
using uptr=std::uintptr_t;FILE* log_file{};
namespace hand=gears_weapon_hand;using P=gears_vr::Pose;using V=gears_vr::Vector;
hand::Frame frame;hand::GripGameState actual_game;P actual_mesh{},actual_held{};
bool fresh=true,rig_matches=true,held_longshot=true,plan_matches=true;
namespace weaponhand {std::int64_t clock_ns(void*){return frame.controllers.sync_ns;}}
namespace lancerreload_inventory {
struct Identity {std::uint64_t world{},weapon{};bool operator==(Identity const& b)const{return world==b.world&&weapon==b.weapon;}};
Identity identity(hand::Frame const& f){return {f.world.address,f.weapon.address};}
}
namespace nativelancerfollow {
double bound_units=100.;
struct Key {hand::ObjectIdentity component{4},asset{5},weapon{3};std::uint64_t hold_generation{1};}bound_key;
struct {bool valid{true};hand::Frame source{};Key key{};gears_vr::Viewpoint target{};}plan;
struct Mount {bool model_matrix(gears_vr::Viewpoint const&,std::array<float,16>& out){out={};return true;}}mount;
}
namespace nativeequipment {bool current(void*,hand::Frame const&){return fresh;}}
namespace longshot_runtime {enum class Stage {idle,reloading};Stage stage{};bool selected(hand::Frame const&){return held_longshot;}}
namespace longshot_fire {
gears_weapon::LongshotReloadIdentity reload_identity(hand::Frame const& f){return {f.world.address,f.pawn.address,f.weapon.address,f.key.world_generation,f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation,9};}
}
namespace handrig {
struct Result {gears_rig_tracking::Reply reply;};
Result read(gears_rig_tracking::Frame& out,unsigned,unsigned){out={};out.units_per_metre=100;out.base_view={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};out.world_pose_mask=gears_rig_tracking::right_grip_pose;return {gears_rig_tracking::Reply::accepted};}
}
namespace gears_equipment_rig {enum class Status {matched,rejected};Status match(hand::Frame const&,gears_rig_tracking::Frame const&,std::int64_t,unsigned){return rig_matches?Status::matched:Status::rejected;}}
namespace vrequip {
struct MeshPoseProbe {std::uintptr_t mesh{4},asset{5};float local_to_world[16]{};};
bool mesh_pose_probe(std::uintptr_t,MeshPoseProbe& out){out={};return true;}
}
namespace hammerburst_runtime {bool matrix(std::array<float,16> const&,P& out,double& scale){out=actual_mesh;scale=1;return true;}}
namespace longshot_bolt {
V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V mul(V a,double n){return {a.x*n,a.y*n,a.z*n};}
}
namespace snub_visual {double length(V v){return std::hypot(std::hypot(v.x,v.y),v.z);}}
#include "weapon_longshot_eyebox.inl"
void step(bool scope_ready=false){++frame.key.frame;++frame.controllers.sequence;frame.controllers.sync_ns+=20000000;
 if(plan_matches)nativelancerfollow::plan.source=frame;
 nativelancerfollow::plan.target={actual_held.position,gears_vr::ue_angles(gears_vr::rotation_matrix(actual_held.orientation))};
 longshot_eyebox::observe(frame,actual_game,scope_ready);}
void place(double eye_x){
 actual_mesh.orientation={.7071067811865475,0,0,.7071067811865475};
 auto d=gears_vr::transform(gears_vr::rotation_matrix(actual_mesh.orientation),gears_weapon::longshot_eye_contact::ocular_center_raw);
 actual_mesh.position={eye_x*100-d.x,-d.y,6-d.z};
 actual_held=actual_mesh;
}
int main(){
 _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 frame.world.address=1;frame.pawn.address=2;frame.weapon.address=3;frame.key.world_generation=4;frame.stereo.session=5;frame.stereo.space_epoch=6;
 frame.controllers.input_epoch=7;frame.controllers.profile_generation=8;frame.reference_generation=9;
 frame.stereo.head={};frame.stereo.view_flags=15;frame.stereo.eyes[0].pose={{-.032,0,0}, {}};
 frame.stereo.eyes[1].pose={{.032,0,0}, {}};
 frame.controllers.hands[1].buttons_active=gears_vr::controller_stick_click;frame.controllers.sync_ns=1000000000;
 actual_game.known=actual_game.main_hand_held=1;place(-.032);
 for(unsigned i=0;i<7;++i)step();
 assert(longshot_eyebox::current.valid&&longshot_eyebox::current.near_eye&&longshot_eyebox::current.active_eye==0);
 assert(longshot_eyebox::matches(frame,frame.controllers.sync_ns));
 auto& click=frame.controllers.hands[1];click.pressed[gears_vr::control_stick]=1;click.buttons=gears_vr::controller_stick_click;
 step(true);assert(longshot_eyebox::current.switch_requested);
 step(true);assert(!longshot_eyebox::current.switch_requested);
 click.released[gears_vr::control_stick]=1;click.buttons=0;step(true);
 click.pressed[gears_vr::control_stick]=2;click.buttons=gears_vr::controller_stick_click;step(true);
 assert(longshot_eyebox::current.switch_requested);
 actual_mesh.position.x+=1000;step();
 assert(longshot_eyebox::current.near_eye); // Restored ordinary component is not the current held model.
 gears_weapon_longshot_scope_view::Request request{};
 assert(longshot_eyebox::view_request(frame,20,request)&&request.version==2&&request.active==1&&request.eye==0);
 assert(std::abs(request.model_world.position.x-actual_held.position.x)<1e-8&&request.model_scale==1);
 assert(!longshot_eyebox::view_request(frame,0,request)&&request.active==0);
 actual_mesh=actual_held;
 plan_matches=false;step();assert(!longshot_eyebox::current.valid);plan_matches=true;
 assert(!longshot_eyebox::view_request(frame,20,request));
 for(unsigned i=0;i<7;++i)step();assert(longshot_eyebox::current.near_eye);
 nativelancerfollow::plan.valid=false;step();assert(!longshot_eyebox::current.valid);nativelancerfollow::plan.valid=true;
 for(unsigned i=0;i<7;++i)step();assert(longshot_eyebox::current.near_eye);
 ++nativelancerfollow::plan.key.hold_generation;step();assert(!longshot_eyebox::current.valid);--nativelancerfollow::plan.key.hold_generation;
 for(unsigned i=0;i<7;++i)step();assert(longshot_eyebox::current.near_eye);
 assert(!longshot_eyebox::matches(frame,frame.controllers.sync_ns+100000001));
 ++frame.controllers.profile_generation;assert(!longshot_eyebox::matches(frame,frame.controllers.sync_ns));--frame.controllers.profile_generation;
 ++frame.reference_generation;assert(!longshot_eyebox::matches(frame,frame.controllers.sync_ns));--frame.reference_generation;
 place(.032);for(unsigned i=0;i<6;++i)step();assert(longshot_eyebox::current.near_eye&&longshot_eyebox::current.active_eye==1);
 actual_game.busy=1;step();assert(!longshot_eyebox::current.near_eye);actual_game.busy=0;
 for(unsigned i=0;i<7;++i)step();assert(longshot_eyebox::current.near_eye);
 longshot_runtime::stage=longshot_runtime::Stage::reloading;step();assert(!longshot_eyebox::current.near_eye);longshot_runtime::stage=longshot_runtime::Stage::idle;
 rig_matches=false;step();assert(!longshot_eyebox::current.valid);rig_matches=true;
 held_longshot=false;step();assert(!longshot_eyebox::current.valid);held_longshot=true;
 frame.stereo.view_flags=0;step();assert(!longshot_eyebox::current.valid);
 std::puts("Longshot production eyebox: left/right eye relief, dwell, current identity/reference/profile, age, reload/menu and rig loss passed");
}
