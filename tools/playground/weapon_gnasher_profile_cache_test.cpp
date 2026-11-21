#include "../vr-common/weapon_hand_client.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#ifdef NDEBUG
#error GnasherProfileCacheTest requires active checks
#endif
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#x);std::abort();}}while(false)
namespace hand=gears_weapon_hand;namespace vr=gears_vr;
std::uint64_t current_hold=1;
namespace inv {
struct Identity {hand::ObjectIdentity world{},pawn{},weapon{};std::uint64_t registration{},native{},world_generation{},hold{};};
Identity identity(hand::Frame const& f){return {f.world,f.pawn,f.weapon,f.key.registration_cookie,f.key.native_started_qpc,f.key.world_generation,current_hold};}
bool same_instance_across_hold(Identity const& a,Identity const& b){return a.hold&&b.hold>=a.hold&&hand::same(a.world,b.world)&&hand::same(a.pawn,b.pawn)&&hand::same(a.weapon,b.weapon)&&a.registration==b.registration&&a.native==b.native&&a.world_generation==b.world_generation;}
}
namespace gnasher_runtime {
// Geometry payload is opaque to this production cache. No engine, pose
// solver, firing/inventory or actual frame-permit boundary is substituted.
struct Profile {unsigned geometry{};inv::Identity identity{};hand::Frame source{};bool known{};std::uint64_t retained_shot{};bool retained_down{};}profile;
std::array<Profile,16> saved_profiles{};
unsigned proposal{},publication{},main{},decision_source{},committed_decision{},observed_geometry{},curve{},closed{};
bool decision_known{},main_required{},basis_known{},curve_known{},down_seen{};
vr::Pose main_in_component{};
std::uint64_t animation_serial{},last_sequence{},last_frame{},shot{};
hand::FrameKey route_deferred_key{};
#include "weapon_gnasher_profile_cache.inl"
}
namespace g=gnasher_runtime;
hand::ObjectIdentity object(std::uint64_t n){return {n,0x1000,0x2000,unsigned(n/8),7,0,1};}
hand::Frame frame(std::uint64_t weapon=0x5000){hand::Frame f{};
 f.key={11,22,33,1,10,1,44,55};f.world=object(0x6000);f.pawn=object(0x6020);f.controller=object(0x6040);f.weapon=object(weapon);
 f.stereo.session=1;f.stereo.space_epoch=2;f.controllers.input_epoch=3;f.controllers.profile_generation=4;f.reference_generation=5;
 f.controllers.sequence=10;f.controllers.sync_ns=100;return f;
}
void dirty(){g::proposal=g::publication=g::main=g::decision_source=g::committed_decision=g::observed_geometry=g::curve=g::closed=9;
 g::decision_known=g::main_required=g::basis_known=g::curve_known=true;g::animation_serial=g::last_sequence=g::last_frame=9;}
void clean(){CHECK(!g::proposal&&!g::publication&&!g::main&&!g::decision_source&&!g::committed_decision&&!g::observed_geometry&&!g::curve&&!g::closed);
 CHECK(!g::decision_known&&!g::main_required&&!g::basis_known&&!g::curve_known&&!g::animation_serial&&!g::last_sequence&&!g::last_frame);}
void reset(){g::profile={};g::saved_profiles={};g::shot=0;g::down_seen=false;g::route_deferred_key={};current_hold=1;dirty();}
void install(hand::Frame const& f,unsigned geometry){g::profile={geometry,inv::identity(f),f,true};}
int main(){
 // Actual strict pose-consumer comparator: a newly exposed mount changes an
 // already committed observation. The GN bootstrap must defer its route,
 // rather than weakening this shared comparator or renewing a source clock.
 reset();auto bootstrap=frame();CHECK(g::select_profile(bootstrap));install(bootstrap,70);
 hand::GripGameState bare{};bare.known=bare.main_hand_held=true;bare.kind=hand::WeaponKind::firearm;
 auto routed=bare;routed.mount_known=true;routed.mount_revision=71;
 hand::Decision d{};d.key=bootstrap.key;
 CHECK(!hand::same_pose_consumer_observation(bootstrap,d,routed,bare));
 CHECK(hand::game_difference(routed,bare)&8u);
 CHECK(!g::profile_route_ready(bootstrap));
 auto reread=bare;if(g::profile_route_ready(bootstrap))reread=routed;
 CHECK(hand::same_pose_consumer_observation(bootstrap,d,reread,bare));
 auto newer_input=bootstrap;++newer_input.controllers.sequence;++newer_input.controllers.sync_ns;
 CHECK(!g::profile_route_ready(newer_input)); // A newer mailbox sample is not a new committed Frame.
 auto next_frame=bootstrap;++next_frame.key.frame;CHECK(g::profile_route_ready(next_frame));
 CHECK(hand::same_pose_consumer_observation(next_frame,d,routed,routed));
 CHECK(g::profile.source.controllers.sync_ns==100&&g::profile.source.controllers.sequence==10);
 reset();auto first=frame();CHECK(g::select_profile(first)&&!g::profile.known);clean();install(first,71);g::shot=101;g::down_seen=true;
 auto second=frame(0x5100);++current_hold;dirty();CHECK(g::select_profile(second)&&!g::profile.known&&!g::shot&&!g::down_seen);clean();
 CHECK(g::saved_profiles[0].known&&g::saved_profiles[0].geometry==71&&g::saved_profiles[0].retained_shot==101&&g::saved_profiles[0].retained_down);
 install(second,72);g::shot=202;g::down_seen=false;++current_hold;auto again=first;++again.key.frame;again.controllers.sequence=20;again.controllers.sync_ns=200;
 dirty();CHECK(g::select_profile(again)&&g::profile.geometry==71&&g::shot==101&&g::down_seen);clean();
 CHECK(!g::profile_route_ready(again));auto after_restore=again;++after_restore.key.frame;CHECK(g::profile_route_ready(after_restore));
 CHECK(g::profile.source.controllers.sync_ns==100&&g::profile.source.controllers.sequence==10); // No old source clock renewed.
 dirty();CHECK(g::select_profile(again)&&g::proposal==9&&g::last_sequence==9); // Same actor does not erase current-frame derived state.
 for(unsigned field=0;field<6;++field){auto changed=again;
  if(field==0)++changed.controller.registry_index;if(field==1)++changed.stereo.session;if(field==2)++changed.stereo.space_epoch;
  if(field==3)++changed.controllers.input_epoch;if(field==4)++changed.controllers.profile_generation;if(field==5)++changed.reference_generation;
  CHECK(!g::select_profile(changed)&&g::profile.geometry==71&&g::shot==101&&g::down_seen&&g::proposal==9);}
 auto recycled=again;++recycled.weapon.registry_index;CHECK(g::select_profile(recycled)&&!g::profile.known&&!g::down_seen);clean();
 reset();first=frame();install(first,71);g::shot=303;g::down_seen=true;auto world=frame();world.world=object(0x7000);++world.key.world_generation;
 CHECK(g::select_profile(world)&&!g::profile.known&&!g::shot&&!g::down_seen);clean();CHECK(g::saved_profiles[0].retained_shot==303);
 reset();first=frame();install(first,71);g::shot=404;g::down_seen=true;
 for(unsigned i=0;i<g::saved_profiles.size();++i){auto other=frame(0x8000+i*8);g::saved_profiles[i]={i+100,inv::identity(other),other,true,i+1,false};}
 dirty();CHECK(!g::select_profile(frame(0x9000))&&g::profile.geometry==71&&g::shot==404&&g::down_seen&&g::proposal==9);
 std::puts("Production Gnasher profile cache: strict real consumer rejects same-Frame mount change; route deferred until next Frame, same-Frame newer input remains deferred, retained profile restore deferred; actor/cohort/witness/no-clock-renewal/no-eviction checks passed. Metadata-only, no game/action acceptance.");
}
