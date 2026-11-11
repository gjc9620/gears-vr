
#include "weapon_hand_client.h"
#include "weapon_hammerburst_latch.h"
#include "controller_relative_motion.h"
#include "weapon_hammerburst_extraction_history.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_weapon;
using P=gears_vr::Pose;using V=gears_vr::Vector;using Q=gears_vr::Quaternion;
namespace hand=gears_weapon_hand;
namespace input=gears_hand_continuity;
namespace relative=gears_controller_relative_motion;
namespace policy=gears_hammerburst_extraction_history;
enum class Stage{old_magazine,new_magazine};Stage stage=Stage::old_magazine;
HammerburstMagazineExtraction extraction;FeedPortMount feed;
namespace latch_history {struct Context {
    hand::Frame frame{};input::TrackedInput raw{};input::Publisher publisher{};
    gears_weapon::HammerburstLatchSample sample{};P mount{};double size{},units{};bool valid{};unsigned capture_gate{1};
}; bool close(V a,V b,double epsilon) noexcept {
    return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)<=epsilon;
} bool same_context(Context const& a,Context const& b) noexcept {
    auto const& x=a.frame;auto const& y=b.frame;
    return a.valid&&b.valid&&x.key.game_pid==y.key.game_pid&&x.key.game_start==y.key.game_start&&
        x.key.native_started_qpc==y.key.native_started_qpc&&x.key.registration_cookie==y.key.registration_cookie&&
        x.key.lifecycle==y.key.lifecycle&&x.key.world_generation==y.key.world_generation&&x.key.thread_id==y.key.thread_id&&
        x.flags==y.flags&&x.reference_generation==y.reference_generation&&x.press_token==y.press_token&&
        x.interaction_control==y.interaction_control&&hand::same(x.world,y.world)&&hand::same(x.controller,y.controller)&&
        hand::same(x.pawn,y.pawn)&&hand::same(x.mesh,y.mesh)&&hand::same(x.weapon,y.weapon)&&
        a.publisher.pid==b.publisher.pid&&a.publisher.start==b.publisher.start&&
        a.units==b.units&&std::abs(a.size-b.size)<1e-6&&close(a.mount.position,b.mount.position,1e-6)&&
        std::abs(a.mount.orientation.x-b.mount.orientation.x)<1e-6&&
        std::abs(a.mount.orientation.y-b.mount.orientation.y)<1e-6&&
        std::abs(a.mount.orientation.z-b.mount.orientation.z)<1e-6&&
        std::abs(a.mount.orientation.w-b.mount.orientation.w)<1e-6;
}}
namespace feed_history {
using Context=latch_history::Context;Context previous{},sampled{};
input::MotionHistory history{};FeedSample prior{};unsigned gate{},point_count{};
bool project_pose(input::TrackedInput const& raw,Context const& c,P& out) noexcept {
    V position{};Q rotation{};
    if(!relative::project(raw,c.mount,c.size,position))return false;
    const auto q=gears_vr::multiply(gears_vr::inverse(raw.input.hands[1].grip.orientation),raw.input.hands[0].grip.orientation);
    if(!gears_vr::rig_tracking::detail::quaternion(gears_vr::xr_to_ue(gears_vr::rotation_matrix(q)),rotation))return false;
    out={position,gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(c.mount.orientation),rotation))};
    return gears_vr::stereo_pose_valid(out);
}
bool advance_extraction(hand::Frame const& f,gears_weapon::FeedSample const& current) noexcept {
    gate=4;point_count=0;
    if(stage!=Stage::old_magazine||prior.item!=current.item||
       !latch_history::same_context(previous,sampled))return false;
    unsigned control{};
    if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))control=gears_vr::control_squeeze;
    else if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))control=gears_vr::control_trigger;
    else return false;
    gate=5;relative::Segment segment{};
    if(!relative::build(previous.raw,sampled.raw,history,control,sampled.mount,sampled.size,segment)||segment.count<3)return false;
    std::array<gears_weapon::FeedSample,9> points{};points[0]=prior;gate=6;
    for(unsigned i=1;i<segment.count;++i){const auto sequence=segment.points[i].sequence;
        input::TrackedInput const* raw=nullptr;
        for(unsigned n=0;n<history.count;++n)if(history.frames[n].input.sequence==sequence)raw=&history.frames[n];
        P left{};if(!raw||!project_pose(*raw,sampled,left))return false;
        points[i]=current;points[i].sequence=sequence;points[i].monotonic_ns=raw->input.sync_ns;points[i].left_grip=left;
    }
    gate=7;if(!gears_hammerburst_feed_history::same_sample(points[segment.count-1],current))return false;
    points[segment.count-1]=current;
    if(!gears_hammerburst_extraction_history::advance(extraction,prior,current,points.data(),segment.count,feed))return false;
    point_count=segment.count;gate=0;return true;
}
}
FeedSample sample(std::uint64_t seq,std::int64_t ns,double depth){return {1,2,3,4,seq,ns,{},{{0,0,depth},{}},true,true};}
HammerburstMagazineExtraction seed(FeedSample const& p){
 HammerburstMagazineExtraction e;auto a=p;--a.sequence;a.monotonic_ns-=60000000;a.left_grip.position.z=0;
 assert(e.observe(a,feed)&&e.observe(p,feed)&&!e.complete());return e;
}
input::TrackedInput raw(FeedSample const& s){
 input::TrackedInput r{};r.stream_epoch=r.tracking_epoch[0]=r.tracking_epoch[1]=1;
 auto& f=r.input;f.session=f.space_epoch=1;f.sequence=s.sequence;f.focused=1;
 f.sync_ns=f.received_ns=f.predicted_time=s.monotonic_ns;
 for(auto& h:f.hands){h.grip_active=1;h.grip_flags=gears_vr::controller_pose_required_flags;h.grip.orientation.w=1;}
 f.hands[0].grip.position.y=s.left_grip.position.z;
 f.hands[0].squeeze_active=1;f.hands[0].squeeze=1;f.hands[0].pressed[gears_vr::control_squeeze]=7;f.hands[0].released[gears_vr::control_squeeze]=6;
 assert(input::tracked_input_shape_valid(r));return r;
}
int main(){
 unsigned cases{};const auto p=sample(4754,164386844683500,.00776483857);
 std::array<FeedSample,6> points{};points[0]=p;
 for(unsigned i=1;i<6;++i)points[i]=sample(p.sequence+i,p.monotonic_ns+111442300*i/5,
     p.left_grip.position.z+(.020643711-p.left_grip.position.z)*i/5);
 auto current=points.back();auto direct=seed(p);assert(!direct.observe(current,feed)&&!direct.complete());
 auto e=seed(p);assert(policy::advance(e,p,current,points.data(),6,feed)&&!e.complete());
 assert(e.previous_sample()&&gears_hammerburst_feed_history::same_sample(*e.previous_sample(),current));
 auto finish=sample(4760,current.monotonic_ns+60000000,.05);assert(e.observe(finish,feed)&&e.complete());cases+=2;
 for(unsigned bad=0;bad<20;++bad){
  auto a=points;auto before=p,end=current;auto mount=feed;unsigned count=6;
  switch(bad){
  case 0:++a[2].weapon;break;case 1:++a[2].item;break;case 2:++a[2].world;break;
  case 3:++a[2].space_epoch;break;case 4:++a[2].sequence;break;
  case 5:a[2].monotonic_ns=a[1].monotonic_ns;break;case 6:a[2].tracked=false;break;
  case 7:a[2].item_held=false;break;case 8:a[2].left_grip.orientation.w=std::numeric_limits<double>::quiet_NaN();break;
  case 9:a[2].weapon_pose.position.x=.01;break;case 10:before.left_grip.position.z+=.001;break;
  case 11:end.left_grip.position.z+=.001;break;case 12:count=2;break;case 13:count=10;break;
  case 14:mount.maximum_sample_gap_ns=1000;break;case 15:a[2].left_grip.position.x=.1;break;
  case 16:a[4].left_grip.position.z=.3;break;
  case 17:a[2].left_grip.orientation={0,0,1,0};break;
  case 18:mount.maximum_sample_gap_ns=0;break;case 19:a[0].tracked=false;break;
  }
  auto trial=seed(p);assert(!policy::advance(trial,before,end,a.data(),count,mount));
  assert(!trial.complete()&&gears_hammerburst_feed_history::same_sample(*trial.previous_sample(),p));
  assert(!trial.observe(current,feed));++cases;
 }
 {HammerburstMagazineExtraction cold;assert(!policy::advance(cold,p,current,points.data(),6,feed)&&!cold.complete());++cases;}
 for(unsigned bad=0;bad<9;++bad){
  using namespace feed_history;
  previous={};sampled={};history={};prior=p;stage=Stage::old_magazine;
  previous.valid=sampled.valid=true;previous.size=sampled.size=1;previous.units=sampled.units=100;
  previous.publisher=sampled.publisher={123,456};previous.raw=raw(p);sampled.raw=raw(current);
  hand::Frame frame{};frame.interaction_control=unsigned(gears_vr::LeftInteractionControl::grip);
  previous.frame=sampled.frame=frame;history.count=5;
  for(unsigned i=1;i<6;++i)history.frames[i-1]=raw(points[i]);
  extraction=seed(p);
  if(bad==1)++sampled.publisher.start;if(bad==2)++sampled.frame.key.lifecycle;
  if(bad==3)history.frames[2].tracking_epoch[0]++;
  if(bad==4)history.frames[2].input.hands[0].released[gears_vr::control_squeeze]++;
  if(bad==5){for(unsigned i=0;i<4;++i)history.frames[i]=history.frames[i+1];--history.count;}
  if(bad==6)stage=Stage::new_magazine;
  if(bad==7)sampled.mount.position.x=.01;
  auto endpoint=current;if(bad==8)endpoint.left_grip.position.x=.01;
  bool ok=advance_extraction(frame,endpoint);
  if(!bad){assert(ok&&point_count==6&&gate==0&&extraction.observe(finish,feed)&&extraction.complete());}
  else assert(!ok&&!extraction.complete()&&gears_hammerburst_feed_history::same_sample(*extraction.previous_sample(),p));
  ++cases;
 }
 std::printf("Extraction history: %u cases; original111.4423ms direct rejection, real extraction kernel and exact producer projection/advance caller, transactional invalid-history rejection. Intermediate points synthetic, no game/engine/Native-capture proof.\n",cases);
}
