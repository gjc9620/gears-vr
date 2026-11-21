#include "../vr-common/weapon_boomshot_insertion.h"
#pragma push_macro("main")
#undef main
#define main previous_source_boundary_cases
#include "weapon_boomshot_core_source_test.cpp"
#undef main
#pragma pop_macro("main")
#include <functional>
#include "../vr-common/body_slots.h"
namespace {
unsigned native_calls{},normal_fault{},original_reads{};bool fire_neutral=true;std::function<void()> during_original;
unsigned inventory_writer_calls{},visual_seat_calls{},original_exit_calls{},original_reward_calls{},partial_restore_calls{};
h::ObjectIdentity retired_actor{};bool retirement_unreadable{};
namespace lancerreload_runtime {
bool weapon_retired(h::ObjectIdentity const& expected)noexcept{return is_gt&&!retirement_unreadable&&h::same(expected,retired_actor);}
}
namespace lancerreload_inventory {
using Inventory=c::ReloadTxnInventory<Identity>;using Request=c::ReloadTxnRequest<Identity>;using Receipt=c::ReloadTxnReceipt<Identity>;
Counts owned_counts{1,1,2,false};
bool counts_equal(Inventory const& a,Inventory const& b){return a.identity==b.identity&&a.capacity==b.capacity&&a.loaded==b.loaded&&a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;}
class ObservedInventory {
 Identity context_{};Counts last_{};std::uint64_t revision_{};
public:
 bool sample(h::Frame const& f,Inventory& out){if(!nativeequipment::current(nullptr,f))return false;const auto id=identity(f);
  if(!revision_){context_=id;revision_=1;last_=owned_counts;}return sample_owned(id,out);}
 bool sample_owned(Identity const& id,Inventory& out){out={};if(!revision_||!(context_==id)||normal_fault==23)return false;
  if(!(last_==owned_counts)){++revision_;last_=owned_counts;}
  out={id,revision_,unsigned(owned_counts.capacity),unsigned(owned_counts.capacity-owned_counts.used),unsigned(owned_counts.reserve),owned_counts.infinite};return out.valid();}
 bool rebind_hold(h::Frame const& f,Identity const& expected,Inventory& out){const auto next=identity(f);
  if(!(context_==expected)||(!(next==expected)&&!same_instance_across_hold(expected,next))||!nativeequipment::current(nullptr,f))return false;
  if(!sample_owned(expected,out)||!(last_==owned_counts))return false;context_=next;out.identity=next;return out.valid();}
};
enum class CommitResult {rejected,committed,reconciliation_required};
struct InsertionWriter {
 CommitResult commit(h::Frame const& f,Request const& request,ObservedInventory& observer,bool(*owns)(h::Frame const&,std::uint64_t),Receipt& receipt){
  ++inventory_writer_calls;assert(owns(f,request.reload_serial)&&request.before.loaded==0&&request.expected.loaded==1&&request.before.capacity==1);
  if(normal_fault==21)return CommitResult::rejected;
  owned_counts.reserve=int(request.expected.reserve);
  if(normal_fault==20)return CommitResult::reconciliation_required;
  owned_counts.used=0;receipt.token=request.token;receipt.accepted=true;if(normal_fault==33)normal_fault=23;
  if(!observer.sample_owned(request.identity,receipt.observed))return CommitResult::reconciliation_required;
  return CommitResult::committed;
 }
};
}
namespace lancerreload_state {
enum class BeginResult {rejected,acquired,recovery_required};
struct Lease {lancerreload_inventory::Identity identity{};std::uint64_t serial{};bool acquired{};float reward_start{};std::int64_t accepted_begin_ms{},perfect_end_ms{};};
Lease lease{};std::uint64_t next_native_serial=777;
bool retains_serial(std::uint64_t serial){return lease.acquired&&serial==lease.serial;}
bool owned_state(){return lease.acquired&&native_reload;}
struct RewardReceipt {bool dispatched{},applied{},call_confirmed{},failed{};};
bool capture_reward_eligibility(lancerreload_inventory::Inventory const&,RewardReceipt&){return normal_fault!=25;}
bool normal_mode(std::uint64_t){return normal_fault!=29;}
bool byte_value(std::uint64_t,wchar_t const* label,unsigned char& value){assert(!std::wcscmp(label,L"CurrentFireMode"));value=normal_fault==29?1:0;return true;}
bool recover_serial(std::uint64_t id){++original_exit_calls;assert(retains_serial(id));if(normal_fault==26)return false;native_reload=false;lease={};return true;}
bool restore_partial_insert(lancerreload_inventory::Request const& q,lancerreload_inventory::ObservedInventory& observer){
 ++partial_restore_calls;if(normal_fault==32)return false;lancerreload_inventory::Inventory counts{};
 if(!observer.sample_owned(q.identity,counts))return false;
 if(lancerreload_inventory::counts_equal(counts,q.before)||lancerreload_inventory::counts_equal(counts,q.expected))return true;
 auto partial=q.before;partial.reserve=q.expected.reserve;
 if(!lancerreload_inventory::counts_equal(counts,partial)||!retains_serial(q.reload_serial)||!(lease.identity==q.identity))return false;
 lancerreload_inventory::owned_counts.reserve=int(q.before.reserve);return observer.sample_owned(q.identity,counts)&&lancerreload_inventory::counts_equal(counts,q.before);
}
bool perfect_reward(lancerreload_inventory::Request const& q,lancerreload_inventory::Inventory const& opening,lancerreload_inventory::ObservedInventory&,RewardReceipt& r){
 assert(q.result==c::ReloadTxnResult::perfect&&opening.loaded==0&&q.expected.loaded==1);
 if(!r.dispatched){++original_reward_calls;r.dispatched=true;r.call_confirmed=normal_fault!=27;}
 if(normal_fault==28)return false;r.applied=r.call_confirmed;return r.applied;
}
BeginResult begin_for_part(h::Frame const& f,std::uint64_t& serial,h::LeaseKind kind,Lease const* suspended,bool* dispatched){
 assert(kind==h::LeaseKind::magazine||(suspended&&kind==h::LeaseKind::ammo));serial=0;*dispatched=false;++native_calls;if(normal_fault==1)return BeginResult::rejected;
 *dispatched=true;if(during_original)during_original();if(normal_fault==2)return BeginResult::recovery_required;
 serial=suspended?suspended->serial:next_native_serial++;actual_native_serial=serial;lease={lancerreload_inventory::identity(f),serial,true,original_game_time,clock_value/1000000,clock_value/1000000+4000};native_reload=true;
 if(suspended){lease.reward_start=suspended->reward_start;lease.accepted_begin_ms=suspended->accepted_begin_ms;lease.perfect_end_ms=suspended->perfect_end_ms;}
 if(suspended&&normal_fault==38)lease.reward_start+=1;
 if(normal_fault==3)++lease.identity.weapon.registry_index;
 if(normal_fault==4)clock_value+=200000000;
 return BeginResult::acquired;
}
}
namespace boomshot_original {
struct Parts {h::ObjectIdentity component{},asset{};};
struct Source {Parts parts{};lancerreload_inventory::Counts counts{};h::ObjectIdentity player_projectile{},carrier{},carrier_component{};};
bool prepare(h::Frame const& f,Source& out){++original_reads;out={};out.parts={object(0x6000,20),object(0x6100,21)};
 out.carrier=object(0x6200,22);out.carrier_component=object(0x6300,23);return normal_fault!=5&&lancerreload_inventory::read(f,out.counts);}
}
namespace boomshot_clip {extern std::uint64_t serial;}
namespace boomshot_barrel {
bool prepared{};
bool bone(h::Frame const&,gears_vr::Pose& root,gears_vr::Pose& relative,double& scale){root={};relative={};scale=1;
 relative.position=m::closed_raw;relative.position.y-=m::travel_raw*(normal_fault==6?.5:bone_q);return true;}
bool prepare(h::Frame const&){if(!prepared&&bone_q>.015)return false;prepared=true;return true;}
bool snapshot_progress(lancerreload_inventory::Identity const&,std::uint64_t,double& out,bool=false){if(!measured_q_known)return false;out=measured_q;return true;}
bool retire(){if(boomshot_clip::serial)return false;prepared=false;measured_q_known=false;stroke={};return true;}
}
namespace boomshot_inspection_clip {
#include "weapon_boomshot_inspection_owner.inl"
struct Backend {
 struct Prepared{};lancerreload_inventory::Identity identity{};lancerreload_inventory::Counts counts{};
 struct Curve {std::uint64_t content{9001};}curve;
 std::uint64_t serial{};unsigned plays{},stops{},seeks{},finishes{};double sought_q{};
 bool play_sent{},stop_sent{},playing{},finish_sent{},finish_returned{},journaled{};
 bool mine()const{return journaled&&serial;}
 bool context()const{return mine()&&!native_reload&&counts==lancerreload_inventory::owned_counts&&normal_fault!=59;}
 bool prepare(h::Frame const& f,Prepared&){const auto actual_counts=lancerreload_inventory::owned_counts;return normal_fault!=50&&nativeequipment::current(nullptr,f)&&!native_reload&&
  !(actual_counts.used&&(actual_counts.reserve||actual_counts.infinite))&&fire_neutral;}
 bool journal(h::Frame const& f,std::uint64_t id,Prepared const&){identity=lancerreload_inventory::identity(f);counts=lancerreload_inventory::owned_counts;serial=id;journaled=true;
  play_sent=stop_sent=finish_sent=finish_returned=playing=false;return true;}
 Dispatch play(h::Frame const&,Prepared const&){if(normal_fault==51)return Dispatch::not_dispatched;++plays;play_sent=true;playing=true;if(during_original)during_original();return normal_fault==52?Dispatch::unknown:Dispatch::returned;}
 bool drop_unstarted(){if(play_sent)return false;journaled=false;serial=0;return true;}
 bool capture(){return context()&&normal_fault!=53;}
 Dispatch stop(){++stops;stop_sent=true;if(normal_fault!=54)playing=false;return normal_fault==54?Dispatch::unknown:Dispatch::returned;}
 bool stopped(){return context()&&stop_sent&&!playing;}
 bool controls(h::Frame const& f){return context()&&nativeequipment::current(nullptr,f)&&normal_fault!=55;}
 bool seek(h::Frame const&,double q,m::Direction){++seeks;sought_q=q;return normal_fault!=56;}
 bool finish_started()const{return finish_sent;}
 bool retire_if_replaced(){return false;}
 bool finish(){if(!context())return false;if(!finish_sent){finish_sent=true;++finishes;finish_returned=normal_fault!=57;}
  if(!finish_returned||normal_fault==58)return false;journaled=false;serial=0;return true;}
}backend;
Owner<Backend> owner(backend);
void reset(){owner.~Owner();backend={};::new(static_cast<void*>(&owner)) Owner<Backend>(backend);}
bool fire_neutral(h::Frame const&){return ::fire_neutral;}
}
namespace boomshot_clip {
enum class Result {rejected,pending,ready,recovery_required};std::uint64_t serial{};
lancerreload_inventory::Identity identity{};
struct Curve {std::uint64_t content{9001};};Curve curve{};unsigned seeks{};double sought_q{};
Result start(h::Frame const& f,std::uint64_t id){serial=id;identity=lancerreload_inventory::identity(f);return Result::ready;}
Result service(h::Frame const&){return normal_fault==37?Result::pending:Result::ready;}
bool seek(h::Frame const&,double q,m::Direction){++seeks;sought_q=q;return normal_fault!=36;}
bool owned(){return serial&&serial==lancerreload_state::lease.serial&&native_reload;}
bool hand_back_after_native_exit(){assert(!lancerreload_state::retains_serial(serial));if(normal_fault==30)return false;serial=0;identity={};return true;}
}
namespace nativeequipment {gears_weapon::ArmspanCalibration calibration=gears_weapon::default_body_slot_layout();}
namespace boomshot_carrier_original {
enum class RawResult {unavailable,pending,ready};
double raw_depth=.13;
struct Owner {bool attached{};unsigned grabs{},drops{};
 bool pending_seat{};
 lancerreload_inventory::Identity identity{};
 bool active()const{return attached;}
 bool matches(h::Frame const& f)const{return attached&&h::same(identity.world,f.world)&&h::same(identity.pawn,f.pawn)&&h::same(identity.weapon,f.weapon);}
 bool begin(h::Frame const& f,std::uint64_t,std::uint64_t,boomshot_original::Source const&){++grabs;attached=true;identity=lancerreload_inventory::identity(f);return normal_fault!=7;}
 bool detach(){++drops;if(normal_fault==8)return false;attached=false;return true;}
 bool holding(h::Frame const&,std::uint64_t,std::uint64_t)const{return attached&&normal_fault!=22;}
 bool visual_pose(h::Frame const& f,std::uint64_t serial,std::uint64_t item,gears_vr::Pose& out,double& scale_out)const{
  if(!holding(f,serial,item)||normal_fault==70)return false;out={};scale_out=1;return true;}
 RawResult service(h::Frame const&,std::uint64_t,std::uint64_t){return normal_fault==34?RawResult::pending:RawResult::ready;}
 RawResult sample_raw(h::Frame const&,std::uint64_t,std::uint64_t,boomshot_core_source::Snapshot const&,gears_vr::Pose& out){out={};out.position.z=-raw_depth;return RawResult::ready;}
 bool seat(h::Frame const&,std::uint64_t,std::uint64_t,lancerreload_inventory::Inventory const& counts){++visual_seat_calls;assert(counts.loaded==1);
  if(normal_fault==24)return false;if(normal_fault==35){pending_seat=true;return false;}attached=false;return true;}
 bool seat_pending()const{return pending_seat;}
 bool reconcile_seat(){if(normal_fault==35)return false;pending_seat=false;attached=false;return true;}
 bool seat_owned(std::uint64_t serial,std::uint64_t item,lancerreload_inventory::Inventory const& counts){return seat({},serial,item,counts);}
}owner;
}
#include "weapon_boomshot_sessions.inl"
#include "weapon_boomshot_reload_owner.inl"
struct Normal {
 Owned input;Fixture rig;boomshot_reload_owner::Owner owner;unsigned instance{};
 explicit Normal(unsigned unique):instance(unique){normal_fault=native_calls=original_reads=0;original_game_time=1.25f;fire_neutral=true;during_original={};
  inventory_writer_calls=visual_seat_calls=original_exit_calls=original_reward_calls=partial_restore_calls=0;lancerreload_inventory::owned_counts={1,1,2,false};
  retired_actor={};retirement_unreadable=false;
  lancerreload_state::lease={};lancerreload_state::next_native_serial=actual_native_serial=777;boomshot_clip::serial=0;boomshot_clip::seeks=0;boomshot_clip::curve={};boomshot_barrel::prepared=false;boomshot_carrier_original::owner={};source_failure=16;boomshot_inspection_clip::reset();lancerreload_inventory::counts_override=true;
  // Different real ObjectIdentity per case. No old unfinished slot is evicted.
  input.f.weapon=object(0x1400+unique*0x100,30+unique);++input.f.key.lifecycle;++input.f.press_token;input.next(false);input.route.reset();input.grant(c::BoomshotContact::opening);refresh();
 }
 void refresh(){const bool reloading=lancerreload_state::lease.acquired;lancerreload_inventory::supplied_counts=lancerreload_inventory::owned_counts;reset_source(input,rig);source_failure=16;native_reload=reloading;
  rig.rig.weapon=input.f.weapon;rig.recompute();}
 void next(){input.next(false);refresh();}
 bool prepare(){return owner.prepare(input.f,input.d,input.route.grant());}
 auto start(){return owner.start(input.f,input.d,input.route.grant());}
 auto move(){return owner.move(input.f,input.d,input.route.grant());}
 void open(){assert(prepare());next();assert(start()==c::BoomshotEvent::started);next();move();next();hand_q=bone_q=.5;move();next();hand_q=bone_q=1;move();next();assert(move()==c::BoomshotEvent::step_completed);}
 void shoulder_grant(){next();++input.f.press_token;auto& left=input.f.controllers.hands[0];++left.pressed[h::left_control(input.f)];++left.released[h::left_control(input.f)];
  assert(c::shoulder_ammo_center(nativeequipment::calibration,{input.f.stereo.head.position,input.f.reference.orientation},left.grip.position));
  refresh();input.d={};input.d.key=input.f.key;input.d.now_ns=clock_value;input.d.press_token=input.f.press_token;input.d.proposal_nonce=19;input.d.new_grant=1;input.d.release_required=0;input.d.status=h::DecisionStatus::granted;
  input.d.lease={h::LeaseKind::ammo,0,input.f.key.lifecycle,input.f.press_token};assert(input.route.propose(input.f,c::BoomshotContact::take_carrier,clock_value)==c::BoomshotContact::take_carrier&&input.route.remember(input.f,input.d,c::BoomshotContact::take_carrier,clock_value));}
 auto take(){return owner.take(input.f,input.d,input.route.grant());}
 auto carry(){return owner.carry(input.f,input.d,input.route.grant());}
 c::BoomshotInteraction* core(){const auto context=lancerreload_inventory::identity(input.f);auto* slot=boomshot_sessions::find(context);return slot?slot->core(context):nullptr;}
};
}
int main(){unsigned count{};
 {Normal x(1);assert(x.prepare()&&x.owner.pending()&&x.owner.blocks_fire(x.input.f)&&native_calls==0);x.next();
  during_original=[&](){assert(x.owner.stage()==boomshot_reload_owner::Owner::Stage::starting&&x.start()==c::BoomshotEvent::rejected);};
  assert(x.start()==c::BoomshotEvent::started&&native_calls==1&&x.owner.original_serial()==777&&x.core()->stage()==c::BoomshotStage::mechanism);
  assert(!x.owner.cancel_unstarted());x.next();assert(x.move()==c::BoomshotEvent::none&&boomshot_barrel::stroke.active());
  x.next();hand_q=bone_q=.5;assert(x.move()==c::BoomshotEvent::none);x.next();hand_q=bone_q=1;assert(x.move()==c::BoomshotEvent::none);
  x.next();assert(x.move()==c::BoomshotEvent::step_completed&&x.core()->stage()==c::BoomshotStage::take_ammunition&&x.core()->inventory().loaded==0&&x.core()->inventory().reserve==2);
  assert(native_calls==1&&x.owner.pending()&&x.owner.blocks_fire(x.input.f));++count;}
 {Normal x(2);fire_neutral=false;assert(!x.prepare()&&native_calls==0&&!x.owner.pending());++count;}
 {Normal x(3);normal_fault=6;assert(!x.prepare()&&native_calls==0&&!x.owner.pending());++count;}
 {Normal x(4);assert(x.prepare()&&x.owner.cancel_unstarted()&&!x.owner.pending()&&!x.owner.blocks_fire(x.input.f)&&native_calls==0);++count;}
 {Normal x(5);assert(x.prepare());normal_fault=1;x.next();assert(x.start()==c::BoomshotEvent::rejected&&native_calls==1&&x.owner.stage()==boomshot_reload_owner::Owner::Stage::prepared&&x.owner.cancel_unstarted());++count;}
 {Normal x(6);assert(x.prepare());normal_fault=2;x.next();assert(x.start()==c::BoomshotEvent::interrupted&&native_calls==1&&x.owner.stage()==boomshot_reload_owner::Owner::Stage::recovery&&x.owner.blocks_fire(x.input.f)&&!x.owner.cancel_unstarted());
  assert(x.start()==c::BoomshotEvent::rejected&&native_calls==1);++count;}
 {Normal x(7);assert(x.prepare());normal_fault=3;x.next();assert(x.start()==c::BoomshotEvent::interrupted&&x.owner.pending()&&x.owner.blocks_fire(x.input.f)&&!x.owner.cancel_unstarted());++count;}
 {Normal x(8);assert(x.prepare());clock_value+=100000000;x.next();assert(x.start()==c::BoomshotEvent::motion_reset&&native_calls==0);x.next();
  assert(x.start()==c::BoomshotEvent::started&&native_calls==1);++count;}
 {Normal x(9);assert(x.prepare());x.next();normal_fault=4;
  assert(x.start()==c::BoomshotEvent::started&&native_calls==1&&x.core()->stage()==c::BoomshotStage::mechanism&&x.owner.pending());
  assert(x.start()==c::BoomshotEvent::rejected&&native_calls==1);++count;}
 {Normal x(10);x.open();x.shoulder_grant();assert(x.take()==c::BoomshotEvent::grabbed&&x.core()->stage()==c::BoomshotStage::holding_ammunition&&x.core()->held_units()==1);
  assert(x.core()->inventory().loaded==0&&x.core()->inventory().reserve==2&&boomshot_carrier_original::owner.grabs==1&&boomshot_carrier_original::owner.attached);
  assert(x.take()==c::BoomshotEvent::rejected&&boomshot_carrier_original::owner.grabs==1);x.next();assert(x.carry()==c::BoomshotEvent::none);
  x.next();x.input.f.left_grip_down=0;x.input.f.controllers.hands[0].squeeze=0;x.input.f.controllers.hands[0].trigger=0;
  x.refresh();assert(x.carry()==c::BoomshotEvent::released&&x.core()->stage()==c::BoomshotStage::take_ammunition&&x.core()->held_units()==0&&!boomshot_carrier_original::owner.attached&&x.core()->inventory().reserve==2);
  ++count;}
 {Normal x(11);x.open();x.shoulder_grant();normal_fault=7;assert(x.take()==c::BoomshotEvent::interrupted&&x.owner.stage()==boomshot_reload_owner::Owner::Stage::recovery&&x.owner.blocks_fire(x.input.f)&&boomshot_carrier_original::owner.attached&&x.core()->inventory().loaded==0&&x.core()->inventory().reserve==2);++count;}
 {Normal x(12);x.open();x.shoulder_grant();x.input.f.controllers.hands[0].grip.position.x+=2;assert(x.take()==c::BoomshotEvent::rejected&&boomshot_carrier_original::owner.grabs==0&&x.core()->held_units()==0);++count;}
 std::printf("Boomshot normal GT owner: %u cases passed; actual owner/core/event/grant/motion composition, one original start, raw opening, real-shoulder carrier reservation/hold/release, original component boundary, no debit before insertion; original engine boundaries stubbed, no live activation\n",count);return 0;
}
