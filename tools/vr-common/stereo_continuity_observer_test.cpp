#include "stereo_frame.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <limits>
namespace gears_vr {
// Frozen pre-observer gate, source SHA256 712415292ce70397605f3978439ef46192c7e7c34aa42ecca40d05610f428e36
class FrozenStereoGate {
    std::optional<StereoSample> last_;
    std::optional<Pose> reference_;
    std::uint64_t reference_generation_{};
    std::int64_t minimum_sample_ns_{};
    bool recenter_pending_{};
    ReferenceContinuityCounter continuity_;
public:
    void reset(){continuity_.boundary();last_.reset();reference_.reset();reference_generation_=0;minimum_sample_ns_=0;recenter_pending_=false;}
    void request_recenter(){continuity_.boundary();recenter_pending_=true;}
    ReferenceContinuity continuity() const noexcept {return continuity_.snapshot(reference_.has_value(),recenter_pending_);}
    // A native world commit retains session/replay guards. Its old XR sample
    // must not consume the pending recenter or allocate a new-world renderer.
    bool advance_world(std::int64_t minimum_sample_ns){
        if(minimum_sample_ns<=minimum_sample_ns_)return false;
        // Old-world hand continuity must end before a fresh heading arrives.
        continuity_.boundary();
        minimum_sample_ns_=minimum_sample_ns;recenter_pending_=true;return true;
    }
    StereoResult update(StereoSample const& s,std::int64_t now_ns,bool allow_sim=false) {
        auto reject=[&](StereoStatus status){StereoResult result;result.status=status;
            if(status!=StereoStatus::stale)continuity_.break_observed();
            if(reference_)result.reference=*reference_;
            result.reference_generation=reference_generation_;return result;};
        if(last_&&s.session!=last_->session)return reject(StereoStatus::session_changed);
        if(!stereo_source_allowed(s.source,allow_sim)||(last_&&s.source!=last_->source))return reject(StereoStatus::invalid);
        if(!s.session||!s.sequence||!s.space_epoch)return reject(StereoStatus::invalid);
        if(last_&&s.space_epoch<last_->space_epoch)return reject(StereoStatus::replayed);
        if(last_&&(s.sequence<last_->sequence||(s.sequence==last_->sequence&&!stereo_same(s,*last_))))
            return reject(StereoStatus::replayed);
        if(last_&&(s.received_ns<last_->received_ns||s.predicted_time<last_->predicted_time))
            return reject(StereoStatus::replayed);
        if(!stereo_render_active(s))return reject(StereoStatus::inactive);
        if(!stereo_tracking_valid(s,allow_sim))return reject(StereoStatus::untracked);
        if(now_ns<=0||s.received_ns<=0||s.received_ns>now_ns){
            continuity_.break_observed();return reject(StereoStatus::stale);
        }
        if(now_ns-s.received_ns>100000000){
            // Preserve the old status order, but only a genuinely age-only
            // failure may keep a continuity certificate. Already observed bad
            // geometry or a different LOCAL origin cannot hide behind age.
            if(!stereo_geometry_valid(s)||(last_&&s.space_epoch!=last_->space_epoch))
                continuity_.break_observed();
            return reject(StereoStatus::stale);
        }
        if(!stereo_geometry_valid(s))return reject(StereoStatus::invalid);
        if(s.received_ns<=minimum_sample_ns_)return reject(StereoStatus::stale);
        const bool new_space=last_&&s.space_epoch!=last_->space_epoch;
        if(new_space){continuity_.boundary();reference_.reset();} // No old-origin reference survives this boundary.
        bool first=!reference_,recentered=false,deferred=false;
        last_=s; // Even an unavailable initial heading cannot erase replay guards.
        if(first||recenter_pending_){
            Pose heading{};
            if(stereo_heading_reference(s.head,heading)){
                continuity_.boundary();
                reference_=heading;++reference_generation_;recentered=!first||new_space;
                // A request before initialization is satisfied by generation1.
                recenter_pending_=false;
            }else deferred=true;
        }
        if(!reference_){auto result=reject(StereoStatus::heading_unavailable);
            result.sample=s;result.heading_deferred=true;return result;}
        if(!deferred&&!recenter_pending_)continuity_.accepted();
        return {first?StereoStatus::initial:StereoStatus::accepted,s,*reference_,reference_generation_,recentered,deferred};
    }
};

struct StereoContinuityObserverTestAccess {
    static void exhaust_next(StereoGate& g){g.continuity_=ReferenceContinuityCounter((std::numeric_limits<std::uint64_t>::max)());}
};
}
namespace v=gears_vr;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
struct Sink {std::atomic<std::uint64_t> epoch{},calls{};std::atomic<unsigned> flags{};};
void observe(void* context,v::ReferenceContinuity s) noexcept {
    auto& sink=*static_cast<Sink*>(context);sink.epoch.store(s.epoch,std::memory_order_relaxed);
    sink.flags.store(unsigned(s.stable)|(unsigned(s.pending)<<1)|(unsigned(s.exhausted)<<2),std::memory_order_release);
    sink.calls.fetch_add(1,std::memory_order_relaxed);
}
bool readable(Sink const& sink,std::uint64_t frame_epoch){return sink.flags.load(std::memory_order_acquire)==1&&sink.epoch.load(std::memory_order_relaxed)==frame_epoch;}
v::StereoSample sample(std::uint64_t n){v::StereoSample s{};s.session=7;s.sequence=n;s.received_ns=1000000000+std::int64_t(n)*1000000;s.predicted_time=s.received_ns;
    s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;s.head={{0,1.7,0},{0,0,0,1}};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose=s.head;s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.7,.7,.7,-.7};}return s;}
bool result_equal(v::StereoResult const& a,v::StereoResult const& b){
    if(a.status!=b.status||a.reference_generation!=b.reference_generation||a.recentered!=b.recentered||a.heading_deferred!=b.heading_deferred||
       std::memcmp(&a.reference,&b.reference,sizeof a.reference)||!v::stereo_same(a.sample,b.sample))return false;
    if(std::memcmp(&a.sample.head,&b.sample.head,sizeof a.sample.head))return false;
    for(unsigned i=0;i<2;++i)if(std::memcmp(&a.sample.eyes[i],&b.sample.eyes[i],sizeof a.sample.eyes[i]))return false;
    return true;
}
void compare(v::StereoGate& gate,v::FrozenStereoGate& frozen,v::StereoSample const& s,std::int64_t now,bool allow=false){
    const auto a=gate.update(s,now,allow),b=frozen.update(s,now,allow);C(result_equal(a,b));
    const auto x=gate.continuity(),y=frozen.continuity();C(x.epoch==y.epoch&&x.stable==y.stable&&x.pending==y.pending&&x.exhausted==y.exhausted);
    v::StereoMatrix view{1,0,0,0,0,1,0,0,0,0,1,0,3,4,5,1},projection{1,0,0,0,0,1,0,0,0,0,1,1,0,0,-1,0};
    v::StereoMatrices ma{},mb{};C(v::stereo_matrices(view,projection,a.reference,a.sample,ma,allow)==v::stereo_matrices(view,projection,b.reference,b.sample,mb,allow));
    C(!std::memcmp(&ma,&mb,sizeof ma));
}
int main(){
    // Exact old result/reference/pose and matrix bytes for default-off and on.
    for(unsigned enabled=0;enabled<2;++enabled){v::StereoGate gate;v::FrozenStereoGate frozen;Sink sink;
        if(enabled)gate.set_continuity_observer(observe,&sink);
        for(std::uint64_t n=1;n<=600;++n){auto s=sample(n);auto now=s.received_ns;const auto op=unsigned(n%23);
            if(op==0){gate.reset();frozen.reset();}
            if(op==1){gate.request_recenter();frozen.request_recenter();}
            if(op==2){C(gate.advance_world(now-1)==frozen.advance_world(now-1));}
            if(op==3){s.head_flags=0;}if(op==4){s.visible=s.focused=0;}if(op==5){now+=100000001;}
            if(op==6){s.head.orientation.w=0;}if(op==7){s.space_epoch=2;}if(op==8){s.session=8;}
            if(op==9){s.received_ns=now+1;}if(op==10){s.sequence=0;}if(op==11){s.source=v::StereoSource::MetaOperatorSimulated;}
            if(op==12){s.head.orientation={.7071067811865475,0,0,.7071067811865475};}
            if(op==13){s.eyes[0].fov.left=(std::numeric_limits<double>::quiet_NaN)();}
            if(op==14){now+=100000001;s.space_epoch=3;}if(op==15){s.predicted_time=1;}
            if(op==16){gate.reset();frozen.reset();s.head.orientation={.7071067811865475,0,0,.7071067811865475};}
            compare(gate,frozen,s,now,op==11);
        }
    }
    Sink sink;v::StereoGate gate;gate.set_continuity_observer(observe,&sink);C(sink.calls==1&&sink.flags==0);
    auto s=sample(1);C(bool(gate.update(s,s.received_ns)));auto old=sink.epoch.load();C(readable(sink,old));
    auto count=sink.calls.load();s=sample(2);C(bool(gate.update(s,s.received_ns)));C(sink.calls==count&&readable(sink,old));
    C(gate.update(s,s.received_ns+100000001).status==v::StereoStatus::stale);C(sink.calls==count&&readable(sink,old));
    gate.request_recenter();C(!readable(sink,old)&&sink.flags==2);count=sink.calls.load();
    s=sample(3);s.head.orientation={.7071067811865475,0,0,.7071067811865475};C(bool(gate.update(s,s.received_ns)));C(sink.flags==2&&sink.calls==count);
    s=sample(4);C(bool(gate.update(s,s.received_ns)));C(sink.flags==1&&!readable(sink,old));old=sink.epoch;
    s=sample(5);s.head_flags=0;C(!gate.update(s,s.received_ns));C(!readable(sink,old)&&sink.flags==0);count=sink.calls;
    C(!gate.update(s,s.received_ns));C(sink.calls==count);s=sample(6);C(bool(gate.update(s,s.received_ns)));C(!readable(sink,old));old=sink.epoch;
    gate.reset();C(!readable(sink,old)&&sink.flags==0);s=sample(7);C(bool(gate.update(s,s.received_ns)));old=sink.epoch;
    C(gate.advance_world(s.received_ns));C(!readable(sink,old)&&sink.flags==2);count=sink.calls;C(!gate.advance_world(s.received_ns));C(sink.calls==count);
    s=sample(8);C(bool(gate.update(s,s.received_ns)));old=sink.epoch;s=sample(9);s.space_epoch=2;C(bool(gate.update(s,s.received_ns)));C(!readable(sink,old)&&sink.flags==1);
    v::StereoGate exhausted;Sink e;v::StereoContinuityObserverTestAccess::exhaust_next(exhausted);exhausted.set_continuity_observer(observe,&e);
    C(e.flags==0);exhausted.reset();C(e.flags==4);s=sample(1);C(bool(exhausted.update(s,s.received_ns)));C(e.flags==4&&!readable(e,e.epoch));
    exhausted.request_recenter();C(e.flags==6);s=sample(2);C(bool(exhausted.update(s,s.received_ns)));C(e.flags==4);
    std::printf("stereo_continuity_observer: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
