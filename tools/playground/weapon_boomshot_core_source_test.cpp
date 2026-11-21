#pragma push_macro("main")
#undef main
#define main previous_motion_bridge_cases
#include "weapon_boomshot_core_motion_test.cpp"
#undef main
#pragma pop_macro("main")
#include <cwchar>
namespace {
unsigned source_failure{},inventory_reads{},game_reads{},clock_reads{};bool native_reload{};float original_game_time=1.25f;
std::uint64_t actual_native_serial=777;
Fixture* source_fixture{};
namespace lancerreload_inventory {
struct Identity {
 h::ObjectIdentity world{},pawn{},weapon{};
 std::uint64_t game_start{},native_start{},registration{},lifecycle{},world_generation{},hold_generation{},session{},space{},input{};
 bool valid()const{return h::identity_valid(world)&&h::identity_valid(pawn)&&h::identity_valid(weapon)&&game_start&&native_start&&registration&&lifecycle&&world_generation&&hold_generation&&session&&space&&input;}
 bool operator==(Identity const& b)const{return h::same(world,b.world)&&h::same(pawn,b.pawn)&&h::same(weapon,b.weapon)&&game_start==b.game_start&&native_start==b.native_start&&registration==b.registration&&lifecycle==b.lifecycle&&world_generation==b.world_generation&&hold_generation==b.hold_generation&&session==b.session&&space==b.space&&input==b.input;}
};
struct Counts {int capacity{},used{},reserve{};bool infinite{};bool operator==(Counts const& b)const{return capacity==b.capacity&&used==b.used&&reserve==b.reserve&&infinite==b.infinite;}};
Counts supplied_counts{};bool counts_override{};
Identity identity(h::Frame const& f){return {f.world,f.pawn,f.weapon,f.key.game_start,f.key.native_started_qpc,f.key.registration_cookie,
 f.key.lifecycle,f.key.world_generation,10,f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch};}
bool same_instance_across_hold(Identity const& before,Identity const& after){
 if(!before.valid()||!after.valid()||after.hold_generation<before.hold_generation||after.lifecycle<before.lifecycle)return false;
 auto normalized=after;normalized.hold_generation=before.hold_generation;normalized.lifecycle=before.lifecycle;return normalized==before;
}
bool read(h::Frame const&,Counts& out){++inventory_reads;if(source_failure==5)return false;out={1,0,2,false};
 if(source_failure==6)out.capacity=2;if(source_failure==7)out.used=-1;if(source_failure==8)out.reserve=-1;
 if(source_failure==9&&inventory_reads>1)++out.reserve;if(source_failure==16)out.used=1;if(counts_override)out=supplied_counts;return true;}
}
namespace nativeequipment {bool current(void*,h::Frame const& f){return is_gt&&source_failure!=1&&h::frame_active(f,clock_value);}}
namespace boomshot_carrier {bool selected(h::Frame const&){return source_failure!=2;}}
namespace weaponhand {
bool observe_game(h::Frame const&,h::GripGameState& out,bool candidates,bool support){assert(!candidates&&!support);++game_reads;
 out={};out.known=out.main_hand_held=true;out.kind=h::WeaponKind::firearm;if(source_failure==3)out.busy=true;
 if(source_failure==4)out.animation_owns_hands=true;if(source_failure==10&&game_reads>1)out.main_hand_held=false;return true;}
}
namespace lancerreload_state {
bool bool_call(std::uint64_t,wchar_t const* name,bool& out){assert(!std::wcscmp(name,L"IsReloading"));out=native_reload;return source_failure!=11;}
bool time_field(std::uint64_t,wchar_t const* name,float& out){assert(!std::wcscmp(name,L"TimeSeconds"));++clock_reads;out=original_game_time;
 if(source_failure==12)out=std::numeric_limits<float>::quiet_NaN();if(source_failure==13&&clock_reads>1)out+=1;return true;}
bool owns(h::Frame const&,std::uint64_t serial){return native_reload&&serial==actual_native_serial&&source_failure!=14;}
}
namespace handrig {
struct ReplyResult {gears_rig_tracking::Reply reply{};};
ReplyResult read(gears_rig_tracking::Frame& out,std::uint64_t,unsigned){out=source_fixture->rig;
 if(source_failure==15)out.world_pose_mask&=~gears_rig_tracking::right_grip_pose;
 return {gears_rig_tracking::Reply::accepted};}
}
#include "weapon_boomshot_core_source.inl"
void reset_source(Owned& x,Fixture& fixture){source_failure=inventory_reads=game_reads=clock_reads=0;native_reload=false;is_gt=true;
 fixture.rig.controllers=x.f.controllers;fixture.rig.stereo=x.f.stereo;fixture.rig.scene_invocation=x.f.key.frame;
 fixture.rig.published_ns=clock_value-1000000;source_fixture=&fixture;}
}
int main(){unsigned cases{};
 {Owned x;Fixture fixture;reset_source(x,fixture);boomshot_core_source::Snapshot source{};
  assert(boomshot_core_source::read(x.f,source)&&source.input.sync_ns==x.f.controllers.sync_ns&&source.input.sequence==x.f.controllers.sequence&&
   source.input.game_tick==1250000&&source.context.input==x.f.controllers.input_epoch&&source.acquired_end_ns==clock_value);
  ++cases;
  x.next(false);reset_source(x,fixture);c::BoomshotOwnerEvents::InputEvent event{};c::BoomshotFrame frame{};
  assert(boomshot_core_source::input(x.f,x.owner,x.events,event,frame)==boomshot_core_source::Result::ready&&event.raw_sequence()==x.f.controllers.sequence&&
   frame.stamp.monotonic_ns==x.f.controllers.sync_ns&&!frame.original_can_fire&&frame.original_interaction_allowed);
  assert(boomshot_core_source::input(x.f,x.owner,x.events,event,frame)==boomshot_core_source::Result::waiting_input&&!event.stamp().valid());++cases;
 }
 for(unsigned reason=1;reason<=15;++reason){Owned x;Fixture fixture;reset_source(x,fixture);source_failure=reason;boomshot_core_source::Snapshot source{};
  if(reason==14)native_reload=true;assert(!boomshot_core_source::read(x.f,source,reason==14?777:0)&&!source.input.sequence);++cases;
 }
 {Owned x;Fixture fixture;reset_source(x,fixture);is_gt=false;boomshot_core_source::Snapshot source{};assert(!boomshot_core_source::read(x.f,source));is_gt=true;++cases;}
 {Owned x;Fixture fixture;reset_source(x,fixture);native_reload=true;boomshot_core_source::Snapshot source{};
  assert(!boomshot_core_source::read(x.f,source));inventory_reads=game_reads=clock_reads=0;
  assert(boomshot_core_source::read(x.f,source,777)&&source.input.sync_ns==x.f.controllers.sync_ns);++cases;}
 {Owned x;Fixture fixture;reset_source(x,fixture);x.next();reset_source(x,fixture);source_failure=16;
  c::BoomshotOwnerEvents::InputEvent event{};c::BoomshotFrame frame{};
  assert(boomshot_core_source::input(x.f,x.owner,x.events,event,frame)==boomshot_core_source::Result::inventory_changed&&!event.stamp().valid());++cases;}
 {Owned x;Fixture fixture;reset_source(x,fixture);c::BoomshotOwnerEvents::OriginalEvent original{};
  assert(x.events.original(clock_value-1000000,1250000,original)==c::BoomshotOwnerEvents::Result::accepted);
  c::BoomshotOwnerEvents::InputEvent event{};c::BoomshotFrame frame{};
  assert(boomshot_core_source::input(x.f,x.owner,x.events,event,frame)==boomshot_core_source::Result::waiting_input&&!event.stamp().valid());++cases;}
 for(unsigned bad=0;bad<3;++bad){Owned x;Fixture fixture;reset_source(x,fixture);
  if(bad==0){x.f.left_grip_down=0;x.f.left_tracked=0;}if(bad==1)x.f.recentered=1;if(bad==2)x.f.heading_deferred=1;
  boomshot_core_source::Snapshot source{};assert(!boomshot_core_source::read(x.f,source)&&!source.input.sequence);++cases;}
 std::printf("Boomshot actual GT source: %u boundary cases passed; original inventory/game clock, exact Scene Rig, normal reload serial vs inspection and unchanged input timestamps; engine boundaries stubbed, real Rig matcher/events/core only\n",cases);return 0;
}
