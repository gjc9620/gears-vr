
#define main existing_supply_suite
#include "preserved-supply-suite.inl"
#undef main
namespace policy {
namespace hand=gears_weapon_hand;
enum class Stage {idle,opening,old_magazine,waiting_ammo,new_magazine,closing,recovering};
FILE* log_file{};Stage stage{};hand::Lease held{};std::uint64_t new_item{};bool released{};
struct Opening {struct Identity {std::uint64_t lifecycle{};} identity;} opening;
struct Counter {unsigned resets{};void reset(){++resets;}} stroke,insertion,extraction;
void unavailable_frame(hand::FrameReadObservation const& failure) noexcept {
    const bool pose_gap=failure.failure==hand::FrameReadFailure::publication||
        failure.failure==hand::FrameReadFailure::stale_before;
    if(pose_gap&&stage==Stage::new_magazine&&new_item&&held.kind==hand::LeaseKind::ammo&&
       held.id==new_item&&hand::lease_valid(held,opening.identity.lifecycle)){
        static unsigned rows{};
        if(log_file&&rows<8){++rows;
            fwprintf(log_file,L"hammerburst_pose_gap_incumbent failure=%u frame=%llu sequence=%llu item=%llu action=0\n",
                unsigned(failure.failure),failure.frame,failure.sequence,new_item);fflush(log_file);}
        return;
    }
    held={};stroke.reset();insertion.reset();extraction.reset();released=false;
}
void reset(){stage=Stage::new_magazine;new_item=7;held={hand::LeaseKind::ammo,0,1,7};opening.identity.lifecycle=1;released=true;stroke={};insertion={};extraction={};}
}
int main(){
 existing_supply_suite();unsigned cases{};
 using namespace policy;
 for(unsigned reason=0;reason<=unsigned(FrameReadFailure::implicit_clear);++reason){
  reset();FrameReadObservation f{};f.failure=FrameReadFailure(reason);unavailable_frame(f);
  bool keep=f.failure==FrameReadFailure::publication||f.failure==FrameReadFailure::stale_before;
  assert((held.kind==LeaseKind::ammo)==keep);assert(insertion.resets==(keep?0u:1u));++cases;
 }
 for(unsigned change=0;change<7;++change){
  reset();if(change==0)stage=Stage::idle;if(change==1)stage=Stage::old_magazine;
  if(change==2)held.kind=LeaseKind::magazine;if(change==3)new_item=0;
  if(change==4)++held.id;if(change==5)++held.lifecycle;if(change==6)held={};
  FrameReadObservation f{};f.failure=FrameReadFailure::stale_before;unavailable_frame(f);
  assert(held.kind==LeaseKind::none&&insertion.resets==1);++cases;
 }
 for(bool repaired:{false,true}){
  Host h;HandClient c(h.adapter());h.connect(c);h.frame.controllers.hands[0].grip.position.x=2.;
  h.game.request_ammo=true;h.step();h.next();h.step();h.next(true);h.step();
  Frame out{};GripGameState game{};Decision d{};
  {GT gt;assert(c.copy_frame_on_gt(out,game,d));}
  assert(d.new_grant&&d.lease.kind==LeaseKind::ammo);
  h.hb_lease=d.lease;h.hb_publication.remember(h.frame,d);h.hb_supply_held=true;
  reset();held=d.lease;new_item=d.lease.id;opening.identity.lifecycle=d.lease.lifecycle;
  h.now+=100000001;FrameReadObservation failure{};
  {GT gt;assert(!c.copy_frame_on_gt(out,game,d,&failure));}
  assert(failure.failure==FrameReadFailure::stale_before&&out.key.frame==0);
  if(repaired)unavailable_frame(failure);else held={}; // Actual old reset block.
  h.hb_supply_held=held.kind!=LeaseKind::none;
  h.frame.now_ns=h.now;h.frame.stereo.received_ns=h.now;h.next(true);h.step();
  {GT gt;assert(c.copy_frame_on_gt(out,game,d));}
  if(repaired){
   assert(d.status==DecisionStatus::retained&&!d.new_grant&&same(d.lease,held));
   h.next(false);h.step();{GT gt;assert(c.copy_frame_on_gt(out,game,d));}
   assert(d.status==DecisionStatus::released&&d.lease.kind==LeaseKind::none);
  }else assert(d.status==DecisionStatus::lease_lost&&d.lease.kind==LeaseKind::none);
  ++cases;
 }
 std::printf("HB exact unavailable helper + actual Client/owner: %u additional cases; old reset reproduces lease_lost, new retains only fresh successor, real UP releases\n",cases);
}
