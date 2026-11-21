#include "weapon_torque_bow_arrow_tracking.h"
#include "weapon_torque_bow_arrow_presentation.h"
#include "weapon_torque_bow_arrow_presentation_fixture.h"
#include "../vr-common/weapon_torque_bow_grasp_pose.h"
#include "weapon_torque_bow_arrow_tracking_fixture.h"
#include "../vr-common/weapon_torque_bow_interaction.h"
#include "../vr-common/weapon_hand_client.h"
#include "../vr-common/committed_hand_interaction.h"
#include "../vr-common/weapon_lancer_starter_mount.h"
#include <vector>
#include <atomic>
#define CHECK BOW_ARROW_CHECK
// High-level binding is injected; the referenced interaction is the actual
// production model. The separate driver regression exercises real bindings.
namespace gears_torque_bow_fire_driver {
struct Driver {
 gears_weapon::TorqueBowInteraction const& value;std::uint64_t generation{};
 bool bound() const noexcept {return generation!=0;}
 std::uint64_t hold_generation() const noexcept {return generation;}
 gears_weapon::TorqueBowInteraction const& interaction() const noexcept {return value;}
};
}
namespace {
namespace h=gears_weapon_hand;namespace vr=gears_vr;using uptr=std::uintptr_t;
namespace test {
using bow_arrow_tracking_fixture::Fixture;
Fixture* input{};bool current=true,appearance=true,hide=true,cleanup=true,matrix=true,update=true,selected=true;
std::uint64_t hold=1;unsigned creates{},destroys{},shows{},withdrawals{},updates{};
std::int64_t time=bow_arrow_tracking_fixture::now;unsigned appearance_reads{},source_copies{},surveys{},veto_prepares{};
}
namespace weaponhand {
unsigned current_gt(void*) noexcept {return 31;}std::int64_t clock_ns(void*) noexcept {return test::time;}
struct Client {bool copy_frame_on_gt(h::Frame& f,h::GripGameState& game) noexcept {
 ++test::source_copies;if(!test::current)return false;f=test::input->hand;game={};game.known=game.main_hand_held=true;return true;
}}client;
}
namespace playground {
struct Vector {float x{},y{},z{};};struct Rotation {int pitch{},yaw{},roll{};};
bool instance(uptr address,wchar_t const*) noexcept {return test::selected&&address==test::input->hand.weapon.address;}
}
namespace nativeequipment {bool current(void*,h::Frame const&) noexcept {return test::current;}}
namespace vrequip {
struct Equipment {
 std::uint64_t hold_generation() const noexcept {return test::hold;}
 std::uint64_t held_weapon() const noexcept {return test::selected?test::input->hand.weapon.address:0;}
 bool held_for(uptr world,uptr pawn,uptr weapon) const noexcept {
  return test::input&&test::input->hand.world.address==world&&test::input->hand.pawn.address==pawn&&test::input->hand.weapon.address==weapon;
 }
} model;
}
namespace torquebow_arrow_appearance {
struct Appearance {
 struct Binding {h::ObjectIdentity asset{};} binding;
 struct Attributes {h::ObjectIdentity light{};} attributes;
 h::ObjectIdentity material{};double world_scale{.3},units_per_metre{100};
};
Appearance selected{};
bool read(h::Frame const&,gears_committed_interaction::Frame const&,Appearance& out) noexcept {
 ++test::appearance_reads;
 if(!test::current||!test::appearance)return false;out=selected;return true;
}
bool same(Appearance const& a,Appearance const& b) noexcept {
 return h::same(a.binding.asset,b.binding.asset)&&h::same(a.material,b.material)&&
  h::same(a.attributes.light,b.attributes.light)&&a.world_scale==b.world_scale&&a.units_per_metre==b.units_per_metre;
}
}
namespace weapon_part_assets {
struct Handle {unsigned slot{},generation{};};enum class Creation {rejected,pending,ready};
enum class Stage {empty,original_mesh_ready,component_created,ready,visible,retiring,failed};
struct Resource {struct Owner{std::uint64_t generation{};}owner;Stage stage{};h::Lease pose_lease{};bool attached{},lighting_bound{},publication_withdrawn{},visibility_revoke{};vr::Viewpoint pose{};};
Resource resource{};Handle active{};unsigned counter{1};bool exists{};
std::atomic<bool> stop_requested{};
Resource* find(Handle handle) noexcept {return exists&&handle.generation==active.generation?&resource:nullptr;}
Creation create_existing_static(h::Frame const&,std::uint64_t generation,h::ObjectIdentity const&,
 std::vector<h::ObjectIdentity> const& materials,playground::Vector const& scale,Handle& handle) noexcept {
 CHECK(!exists&&materials.size()==1&&scale.x==.3f);++test::creates;exists=true;active={0,counter++};handle=active;
 resource={};resource.owner.generation=generation;resource.stage=Stage::original_mesh_ready;return Creation::pending;
}
Creation advance(h::Frame const&,Handle,std::uint64_t) noexcept {
 if(resource.stage==Stage::original_mesh_ready){resource.stage=Stage::component_created;return Creation::pending;}
 resource.stage=Stage::ready;return Creation::ready;
}
bool bind_light_environment(h::Frame const&,Handle,std::uint64_t,h::ObjectIdentity const&,bool) noexcept {resource.lighting_bound=true;return true;}
bool destroy(Handle,std::uint64_t generation) noexcept {
 CHECK(resource.owner.generation==generation);if(!test::cleanup)return false;++test::destroys;exists=false;resource={};return true;
}
bool set_visible(Handle,std::uint64_t,bool visible) noexcept {
 CHECK(!visible);++test::withdrawals;if(!test::hide)return false;resource.stage=Stage::ready;return true;
}
bool set_pose(h::Frame const&,Handle,std::uint64_t,h::Decision const& d,vr::Viewpoint const& pose,bool visible) noexcept {
 resource.pose=pose;resource.pose_lease=d.lease;resource.attached=true;resource.stage=visible?Stage::visible:Stage::ready;
 if(visible)++test::shows;return test::current;
}
bool engine_rotation_axes(h::Frame const&,Resource const&,playground::Rotation const& rotation,vr::Matrix& out) noexcept {
 constexpr double unit=.000095873799242852573;out=vr::ue_matrix({rotation.pitch*unit,rotation.yaw*unit,rotation.roll*unit});return true;
}
template<class Current>bool read_scene_matrix(h::Frame const&,Handle,std::uint64_t,std::array<float,16>& out,Current&& current) noexcept {
 if(!current())return false;const auto basis=vr::ue_matrix(resource.pose.rotation);const auto& position=resource.pose.position;out={};
 for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out[i*4+j]=float(basis[j][i]*.3);
 out[12]=float(position.x+(test::matrix?0:1));out[13]=float(position.y);out[14]=float(position.z);out[15]=1;return current();
}
}
namespace nativeequipment_rig {
enum class Status {unavailable,ready};struct Result {Status status{};};
Result acquire(h::Frame const&,gears_rig_tracking::Frame& out) noexcept {out=test::input->rig;return {Status::ready};}
}
namespace torquebow_arrow_visual {
namespace pool=weapon_part_assets;namespace hand=h;
bool update(h::Frame const&,pool::Handle,std::uint64_t,h::Decision const&) noexcept {++test::updates;return test::update;}
}
#include "weapon_torque_bow_arrow_visual.inl"
namespace nativeequipment {bool domain_ready() noexcept {return test::current;}}
namespace handrig {bool read_interaction(gears_committed_interaction::Frame& out) noexcept {out={};return test::current;}}
namespace torquebow_input {
bool owed{};bool pending() noexcept {return owed;}
struct Driver {gears_weapon::TorqueBowInteraction model;bool bound_value{};bool bound() const noexcept{return bound_value;}
 bool matches_source(h::Frame const& f,std::uint64_t hold) const noexcept{return bound_value&&hold==test::hold&&f.weapon.address==test::input->hand.weapon.address;}
 gears_weapon::TorqueBowInteraction const& interaction() const noexcept {return model;}} driver;
struct Backend {h::Lease reload_lease{};bool owned{};std::uint64_t config_serial{1};
 struct Calibration{std::uint64_t instance_serial{1};}feed_calibration;
 h::Frame geometry_parent{};std::uint64_t geometry_hold{1};gears_vr::Pose support_grip{};bool support_known{};
 bool geometry_current(h::Frame const&)const noexcept{return true;}
 bool prepare_geometry(h::Frame const&)noexcept{return true;}
 bool reload_owned()const noexcept{return owned;}}backend;
bool at_shoulder{};bool shoulder(h::Frame const&) noexcept{return at_shoulder;}
}
namespace lancerreload_manual {void prepare(h::Frame const&) noexcept {++test::veto_prepares;}}
namespace torquebow_original {
struct Preparation {
 unsigned gate{},goto_bytes{},pending_offset{},pending_mask{},source_check_known{},source_current{},
  withdrawal_frame_sequence_match{},withdrawal_candidate_failure{},withdrawal_candidate_difference{};
 uptr clear_pending{},charge_ended{},get_state{},goto_state{},pending_end_property{};
 struct Counts {int capacity{1},used{},reserve{1};}inventory;
 std::int64_t started_ns{},finished_ns{},input_age_begin_ns{},input_age_end_ns{};
};
bool prepare(h::Frame const&,Preparation&) noexcept {++test::surveys;return true;}
}
FILE* log_file{};
namespace nativelancerfollow {struct Key{h::ObjectIdentity asset{};}bound_key;}
#include "weapon_torque_bow_runtime.inl"
}
int main(){
 using namespace bow_arrow_tracking_fixture;namespace visual=torquebow_arrow_visual;namespace pool=weapon_part_assets;
 namespace gw=gears_weapon;Fixture fixture;test::input=&fixture;auto& f=fixture.hand;f.left_grip_down=1;f.press_token=1;
 torquebow_arrow_appearance::selected.binding.asset=object(0x1600,6);
 torquebow_arrow_appearance::selected.material=object(0x1700,7);
 gears_committed_interaction::Frame avatar{};
 CHECK(visual::prepare(f,avatar)==pool::Creation::pending&&visual::pending()&&test::creates==1);
 CHECK(visual::prepare(f,avatar)==pool::Creation::pending&&test::creates==1);
 CHECK(visual::prepare(f,avatar)==pool::Creation::ready&&test::creates==1&&!test::shows);
 gw::TorqueBowIdentity identity{f.world.address,f.pawn.address,f.controller.address,f.weapon.address,
  f.key.world_generation,f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch};
 gw::TorqueBowConfig config{};config.identity=identity;config.instance_config_serial=1;config.capacity=1;config.game_ticks_per_second=1000;
 config.original_class_verified=config.original_charge_entry_verified=config.original_end_fire_verified=config.non_firing_abort_verified=true;
 config.manual_reload_phase_verified=config.reload_plan_verified=true;config.reload_step_count=1;config.reload_steps[0]={gw::TorqueBowReloadAction::insert_arrow,22};
 gw::TorqueBowInteraction model;CHECK(model.bind(config,{1,0,1}));
 const auto stamp=[&](std::uint64_t n){return gw::TorqueBowStamp{identity,n,now-2000000+std::int64_t(n)*1000000,0};};
 CHECK(model.observe({stamp(1),model.inventory(),{true,false,false,false,false},true,true,true,false,f.controllers.sync_ns})==gw::TorqueBowEvent::observed);
 CHECK(model.begin_left_shoulder({stamp(2),1,1,true,true,true,true,0},{11,gw::TorqueBowAmmoSource::left_shoulder,true,true,1,true,true})==gw::TorqueBowEvent::reload_started);
 const gears_torque_bow_fire_driver::Driver driver{model,1};
 h::Decision decision{};decision.key=f.key;decision.press_token=1;decision.lease.id=1;decision.lease.lifecycle=f.key.lifecycle;
 decision.lease.kind=h::LeaseKind::ammo;decision.status=h::DecisionStatus::granted;decision.release_required=0;
 gears_torque_bow_arrow_mount::Mount mount{};
 CHECK(gears_torque_bow_arrow_mount::author({true,{.01,.02,.03},{0,0,0,1}},{.035,.01,-.005},.3,100,mount));
 gears_torque_bow_arrow_tracking::Sample sample{};
 CHECK(visual::show(f,avatar,decision,driver,mount,sample)&&test::shows==1&&test::updates==1&&sample.input_sync_ns==f.controllers.sync_ns);
 auto wrong=decision;wrong.lease.kind=h::LeaseKind::support;
 CHECK(!visual::show(f,avatar,wrong,driver,mount,sample)&&pool::resource.stage==pool::Stage::ready&&!sample.input_sync_ns);
 test::matrix=false;CHECK(!visual::show(f,avatar,decision,driver,mount,sample)&&pool::resource.stage==pool::Stage::ready);test::matrix=true;
 test::update=false;CHECK(!visual::show(f,avatar,decision,driver,mount,sample)&&pool::resource.stage==pool::Stage::ready);test::update=true;
 CHECK(visual::show(f,avatar,decision,driver,mount,sample));
 test::appearance=false;CHECK(!visual::show(f,avatar,decision,driver,mount,sample)&&visual::pending());test::appearance=true;
 ++test::hold;CHECK(visual::prepare(f,avatar)==pool::Creation::rejected&&!visual::pending()&&!pool::exists);
 CHECK(visual::prepare(f,avatar)==pool::Creation::pending);CHECK(visual::prepare(f,avatar)==pool::Creation::pending);
 CHECK(visual::prepare(f,avatar)==pool::Creation::ready);
 CHECK(!visual::show(f,avatar,decision,driver,mount,sample)); // same actor, old model's hold
 gw::TorqueBowInteraction replacement;config.instance_config_serial=2;CHECK(replacement.bind(config,{1,0,1}));
 ++f.key.frame;fixture.rig.scene_invocation=f.key.frame;f.press_token=2;
 ++fixture.rig.stereo.sequence;fixture.rig.controllers.sequence=fixture.rig.stereo.sequence;
 fixture.rig.controllers.hands[0].pressed[vr::control_squeeze]=2;fixture.rig.controllers.hands[0].released[vr::control_squeeze]=1;
 f.stereo=fixture.rig.stereo;f.controllers=fixture.rig.controllers;
 decision.key=f.key;decision.press_token=decision.lease.id=2;
 CHECK(replacement.observe({stamp(1),replacement.inventory(),{true,false,false,false,false},true,true,true,false,f.controllers.sync_ns})==gw::TorqueBowEvent::observed);
 CHECK(replacement.begin_left_shoulder({stamp(2),2,2,true,true,true,true,0},{12,gw::TorqueBowAmmoSource::left_shoulder,true,true,2,true,true})==gw::TorqueBowEvent::reload_started);
 const gears_torque_bow_fire_driver::Driver current_driver{replacement,2};
 CHECK(visual::show(f,avatar,decision,current_driver,mount,sample));
 test::hide=test::cleanup=false;CHECK(!visual::withdraw()&&visual::pending()&&visual::owner.retiring);
 const auto created=test::creates;CHECK(visual::prepare(f,avatar)==pool::Creation::rejected&&test::creates==created&&visual::pending());
 test::hide=test::cleanup=true;CHECK(visual::retire()&&!visual::pending()&&!pool::exists);
 CHECK(visual::prepare(f,avatar)==pool::Creation::pending);pool::exists=false;
 CHECK(visual::retire()&&!visual::pending()); // independently completed common drain
 CHECK(visual::prepare(f,avatar)==pool::Creation::pending);torquebow_arrow_appearance::selected.material=object(0x1800,8);
 CHECK(visual::prepare(f,avatar)==pool::Creation::rejected&&!visual::pending()&&!pool::exists);
 CHECK(visual::prepare(f,avatar)==pool::Creation::pending);CHECK(visual::prepare(f,avatar)==pool::Creation::pending);
 CHECK(visual::prepare(f,avatar)==pool::Creation::ready);
 torquebow_arrow_appearance::selected.material=object(0x1900,9);
 CHECK(!visual::show(f,avatar,decision,current_driver,mount,sample)&&!visual::pending()&&!pool::exists);
 CHECK(visual::prepare(f,avatar)==pool::Creation::pending);CHECK(visual::prepare(f,avatar)==pool::Creation::pending);
 CHECK(visual::prepare(f,avatar)==pool::Creation::ready&&visual::warmed());
 const auto updates_before=test::updates,shows_before=test::shows,destroy_before=test::destroys;
 pool::resource.stage=pool::Stage::visible;visual::on_frame(f);
 CHECK(pool::resource.visibility_revoke&&!pool::resource.publication_withdrawn&&visual::warmed());
 CHECK(test::updates==updates_before&&test::shows==shows_before&&test::destroys==destroy_before);
 ++test::hold;visual::on_frame(f);
 CHECK(visual::owner.retiring&&pool::resource.publication_withdrawn&&pool::resource.stage==pool::Stage::retiring&&!visual::warmed());
 CHECK(test::destroys==destroy_before);CHECK(visual::retire()&&!visual::pending());
 // Actual normal-GT warm wrapper and stop/selection retirement. Engine,
 // appearance and Native source backends remain injected, not game proof.
 const auto fresh=[&](){
  test::time+=250000001;f.now_ns=test::time-1000000;++f.key.frame;
  ++f.stereo.sequence;f.stereo.received_ns=test::time-2000000;
  f.controllers.sequence=f.stereo.sequence;f.controllers.received_ns=test::time-3000000;f.controllers.sync_ns=test::time-4000000;
 };
 CHECK(torquebow_runtime::tick(f.controller.address)&&visual::pending()&&!torquebow_input::pending());
 const auto creations=test::creates;CHECK(!torquebow_runtime::tick(f.controller.address)&&test::creates==creations);
 fresh();CHECK(torquebow_runtime::tick(f.controller.address));fresh();CHECK(torquebow_runtime::tick(f.controller.address)&&visual::warmed());
 const auto reads=test::appearance_reads;fresh();CHECK(torquebow_runtime::tick(f.controller.address)&&test::appearance_reads==reads);
 const auto copies=test::source_copies,surveys=test::surveys;torquebow_input::owed=true;fresh();
 CHECK(torquebow_runtime::tick(f.controller.address)&&test::source_copies==copies&&test::surveys==surveys);torquebow_input::owed=false;
 pool::stop_requested=true;CHECK(!torquebow_runtime::tick(f.controller.address)&&!visual::pending()&&!pool::exists);pool::stop_requested=false;
 fresh();CHECK(torquebow_runtime::tick(f.controller.address)&&visual::pending());test::selected=false;
 CHECK(!torquebow_runtime::tick(f.controller.address)&&!visual::pending()&&!pool::exists);test::selected=true;
 // Existing producer's exact-Frame route cannot drift after its model moves;
 // fresh busy flags are not frozen/replaced with the proposal's old values.
 CHECK(visual::prepare(f,avatar)==pool::Creation::pending);CHECK(visual::prepare(f,avatar)==pool::Creation::pending);
 CHECK(visual::prepare(f,avatar)==pool::Creation::ready);
 config.identity=identity;config.instance_config_serial=3;
 CHECK(torquebow_input::driver.model.bind(config,{1,0,1}));
 const gw::TorqueBowStamp route_stamp{identity,1,test::time,0};
 CHECK(torquebow_input::driver.model.observe({route_stamp,{1,0,1},{true},true,true,true,false,f.controllers.sync_ns})==gw::TorqueBowEvent::observed);
 torquebow_input::driver.bound_value=torquebow_input::at_shoulder=true;
 h::GripGameState route_game{};route_game.known=route_game.main_hand_held=true;route_game.kind=h::WeaponKind::firearm;
 torquebow_runtime::propose(f,route_game);CHECK(route_game.request_ammo);
 torquebow_input::at_shoulder=false;torquebow_input::driver.model.interrupt();
 h::Decision route_decision{};route_decision.key=f.key;torquebow_runtime::committed(f,route_decision);
 h::GripGameState same_scene{};same_scene.busy=true;torquebow_runtime::propose(f,same_scene);
 CHECK(same_scene.request_ammo&&same_scene.busy); // route only, no game-state/age renewal
 ++f.key.frame;h::GripGameState next_scene{};next_scene.known=next_scene.main_hand_held=true;next_scene.kind=h::WeaponKind::firearm;
 torquebow_runtime::propose(f,next_scene);CHECK(!next_scene.request_ammo);
 ++f.key.frame;torquebow_input::backend.owned=true;torquebow_input::backend.reload_lease={h::LeaseKind::ammo,0,f.key.lifecycle,f.press_token};
 h::GripGameState incumbent{};incumbent.known=incumbent.main_hand_held=true;incumbent.kind=h::WeaponKind::firearm;
 torquebow_runtime::propose(f,incumbent);CHECK(h::same(incumbent.object_incumbents[0],torquebow_input::backend.reload_lease));
 // Source-current and proposal readers must receive the same stable Bow
 // support mount; source checks may never omit it or enter Native recursively.
 torquebow_input::backend.geometry_parent=f;--torquebow_input::backend.geometry_parent.key.frame;
 torquebow_input::backend.support_known=true;torquebow_input::backend.support_grip={{.35,.01,.02},{0,0,0,1}};
 nativelancerfollow::bound_key.asset=object(0x1800,8);
 h::GripGameState mount_game{};mount_game.known=mount_game.main_hand_held=true;mount_game.kind=h::WeaponKind::firearm;
 CHECK(torquebow_runtime::fill_support_mount(f,mount_game)&&mount_game.mount_known&&mount_game.acquire_metres<mount_game.release_metres);
 const auto source_calls=test::source_copies;auto same_mount=mount_game;
 CHECK(torquebow_runtime::fill_support_mount(f,same_mount)&&gears_weapon_hand::same_game(mount_game,same_mount)&&test::source_copies==source_calls);
 auto pinch=f;pinch.interaction_control=unsigned(vr::LeftInteractionControl::pinch);h::GripGameState no_mount=mount_game;
 CHECK(!torquebow_runtime::fill_support_mount(pinch,no_mount));
 torquebow_input::backend.geometry_parent=f;CHECK(!torquebow_runtime::fill_support_mount(f,no_mount));
 // Real presentation -> actual visible component caller. The matching palm
 // is returned only after matrix ACK; raw remains the original observation.
 CHECK(visual::retire());bow_arrow_presentation_fixture::Fixture guided;test::input=&guided;
 test::time=now;++test::hold;auto& g=guided.hand;g.left_grip_down=1;g.press_token=1;
 CHECK(guided.place(.03,.015,.08));
 CHECK(visual::prepare(g,avatar)==pool::Creation::pending);CHECK(visual::prepare(g,avatar)==pool::Creation::pending);
 CHECK(visual::prepare(g,avatar)==pool::Creation::ready);
 config.instance_config_serial=37;gw::TorqueBowInteraction guided_model;CHECK(guided_model.bind(config,{1,0,1}));
 CHECK(guided_model.observe({stamp(1),guided_model.inventory(),{true},true,true,true,false,g.controllers.sync_ns})==gw::TorqueBowEvent::observed);
 CHECK(guided_model.begin_left_shoulder({stamp(2),1,1,true,true,true,true,0},{11,gw::TorqueBowAmmoSource::left_shoulder,true,true,1,true,true})==gw::TorqueBowEvent::reload_started);
 const gears_torque_bow_fire_driver::Driver guided_driver{guided_model,test::hold};
 h::Decision grant{};grant.key=g.key;grant.press_token=1;grant.lease={h::LeaseKind::ammo,0,g.key.lifecycle,1};
 grant.status=h::DecisionStatus::granted;grant.release_required=0;
 gears_torque_bow_arrow_presentation::Result presentation{};
 const gears_torque_bow_arrow_presentation::Config guide{guided.feed,guided.fixed,11};
 CHECK(visual::show(g,avatar,grant,guided_driver,guided.arrow,sample,&guide,&presentation)&&presentation.feed_assisted);
 CHECK(visual::owner.feed_assisted&&visual::owner.assist_arrow==11&&sample.input_sync_ns==g.controllers.sync_ns);
 CHECK(gears_torque_bow_arrow_presentation::same_pose(sample.source_world,presentation.raw.source_world,1e-10));
 CHECK(gears_torque_bow_arrow_tracking::close_vector(pool::resource.pose.position,presentation.source_world.position,1e-8));
 auto wrong_guide=guide;wrong_guide.arrow_serial=12;
 CHECK(!visual::show(g,avatar,grant,guided_driver,guided.arrow,sample,&wrong_guide,&presentation)&&!presentation.raw.input_sequence&&!visual::owner.feed_assisted);
 auto old_plan=guided.feed;old_plan.instance_serial=38;const gears_torque_bow_arrow_presentation::Config stale{old_plan,guided.fixed,11};
 CHECK(!visual::show(g,avatar,grant,guided_driver,guided.arrow,sample,&stale,&presentation));
 auto pinch_source=g;pinch_source.interaction_control=unsigned(vr::LeftInteractionControl::pinch);
 CHECK(!visual::show(pinch_source,avatar,grant,guided_driver,guided.arrow,sample,&guide,&presentation));
 std::printf("Torque Bow arrow owner: %u checks passed; actual model/raw tracking/resource caller, injected Engine/appearance/component pool, no game/fit certification.\n",checks);
}
