#ifdef NDEBUG
#undef NDEBUG
#endif
#include "weapon_boltok_mechanics.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
namespace src=gears_hand_continuity;
const BoltokReloadIdentity identity{1,2,3,4,5,6,7,8,9};
Vector at(double a){return {.08*std::cos(a),.08*std::sin(a),0};}
src::TrackedInput input(unsigned sequence,double angle){
    src::TrackedInput r{};r.stream_epoch=1;r.tracking_epoch[0]=r.tracking_epoch[1]=1;auto& p=r.input;
    p.source=gears_vr::StereoSource::RuntimeTracked;p.session=6;p.space_epoch=7;p.input_epoch=8;p.profile_generation=9;
    p.sequence=sequence;p.sync_ns=1000000000+std::int64_t(sequence-1)*22000000;
    p.predicted_time=p.sync_ns+10000000;p.received_ns=p.sync_ns+1000;p.focused=1;
    for(auto& h:p.hands){h.grip_active=h.aim_active=h.trigger_active=h.squeeze_active=1;h.grip_flags=h.aim_flags=15;}
    p.hands[0].squeeze=1;p.hands[0].pressed[gears_vr::control_squeeze]=1;
    const auto q=at(angle);p.hands[0].grip.position={q.y,q.z,-q.x};assert(src::tracked_input_shape_valid(r));return r;
}
BoltokReloadStamp stamp(src::TrackedInput const& r){return {identity,r.input.sequence,r.input.sync_ns,1000+std::int64_t(r.input.sequence),true,true,true};}
HammerburstLatchGeometry geometry(){HammerburstLatchGeometry g{};g.axis={0,0,1};g.closed_contact=at(0);g.open_angle=1.4;g.current_instance_verified=true;return g;}
src::MotionHistory window(unsigned first,double start,double step){src::MotionHistory h{};h.count=5;
    for(unsigned i=0;i<5;++i)h.frames[i]=input(first+i,start+step*i);return h;}
bool begin(BoltokSwingStroke& s,src::TrackedInput r=input(1,0),double angle=0,bool closing=false){const auto f=stamp(r);
    return s.begin(geometry(),f,at(angle),7,true,angle,closing,f.observed_ns)&&
        s.bind_original_motion(r,{},gears_vr::control_squeeze,true,f.observed_ns);}
int main(){
    unsigned cases{};BoltokSwingStroke s;assert(begin(s));auto h=window(2,.14,.14);
    // Original9816 source spacings, with explicitly synthetic geometry/identity.
    const std::int64_t recorded_gaps[]{22264700,22001200,23849900,20633500,22220700};
    auto clock=std::int64_t(1000000000);
    for(unsigned i=0;i<5;++i){clock+=recorded_gaps[i];auto& p=h.frames[i].input;
        p.sync_ns=clock;p.predicted_time=clock+10000000;p.received_ns=clock+1000;}
    auto raw=h.frames[4];auto f=stamp(raw);
    BoltokSwingStroke legacy;assert(legacy.begin(geometry(),stamp(input(1,0)),at(0),7,true,0,false,1000000000));
    assert(!legacy.move(f,at(.7),7,true,f.observed_ns+50000000)); // Original110.97ms red remains.
    assert(s.move_original_motion(f,at(.7),7,true,f.observed_ns+50000000,raw,h,{},true));assert(std::abs(s.wanted_angle()-.7)<1e-12);++cases;
    h=window(7,.84,.14);raw=h.frames[h.count-1];f=stamp(raw);
    assert(s.move_original_motion(f,at(1.4),7,true,f.observed_ns,raw,h,{},true));
    assert(!s.acknowledge(f,0,f.observed_ns));assert(s.acknowledge(f,1.4,f.observed_ns));
    BoltokReloadPlan plan{};plan.identity=identity;plan.generation=1;plan.original_reload_serial=2;
    BoltokReloadStep step{BoltokStepKind::move_original_parts,boltok_object_part(3),3,4,5,6,7,0};BoltokMotionReceipt receipt{};
    assert(s.receipt(plan,step,receipt)&&receipt.tracked_samples==11&&receipt.maximum_sample_gap_ns==23849900);++cases;
    for(unsigned bad=0;bad<18;++bad){assert(begin(s));h=window(2,.14,.14);raw=h.frames[h.count-1];f=stamp(raw);
        auto local=at(.7);std::uint64_t token=7;bool held=true,rigid=true;auto now=f.observed_ns;gears_vr::Pose root{};
        switch(bad){
        case 0:now+=100000001;break;case 1:f.identity.weapon++;break;case 2:f.game_tick=0;break;
        case 3:token++;break;case 4:held=false;break;case 5:h.frames[0]=input(3,.28);break;
        case 6:h.frames[2].tracking_epoch[0]++;break;case 7:h.frames[2].tracking_epoch[1]++;break;
        case 8:h.frames[2].input.hands[0].released[gears_vr::control_squeeze]=1;break;
        case 9:h.frames[2].input.hands[1].grip_flags=0;break;case 10:h.frames[2].stream_epoch++;break;
        case 11:root.position.x=.000101;break;case 12:root.orientation={0,0,std::sin(.00015),std::cos(.00015)};break;
        case 13:local.x+=.000251;break;case 14:raw.input.hands[0].grip.position.x+=.001;break;
        case 15:rigid=false;break;case 16:h.frames[2]=input(4,1.2);break;case 17:h.reserved=1;break;}
        assert(!s.move_original_motion(f,local,token,held,now,raw,h,root,rigid));
        assert(!s.original_motion_bound()&&!s.acknowledge(f,1.4,now));++cases;
    }
    // A historical endpoint followed by retreat cannot complete at the old point.
    assert(begin(s));h=window(2,.14,.14);raw=h.frames[h.count-1];f=stamp(raw);
    assert(s.move_original_motion(f,at(.7),7,true,f.observed_ns,raw,h,{},true));h.count=7;
    for(unsigned i=0;i<7;++i)h.frames[i]=input(7+i,i<=4?.84+.14*i:1.4-.14*(i-4));
    raw=h.frames[6];f=stamp(raw);
    assert(s.move_original_motion(f,at(1.12),7,true,f.observed_ns,raw,h,{},true));
    assert(!s.acknowledge(f,1.4,f.observed_ns));++cases;
    // Current measured animation alone, with no original hand motion, is insufficient.
    assert(begin(s));h=window(2,0,0);raw=h.frames[h.count-1];f=stamp(raw);
    assert(s.move_original_motion(f,at(0),7,true,f.observed_ns,raw,h,{},true));
    assert(s.wanted_angle()==0&&!s.acknowledge(f,1.4,f.observed_ns));++cases;
    // A frozen float-registration correction must never accumulate into motion.
    const Vector offset{.0001,0,0};const auto first=input(1,0);const auto start=stamp(first);
    assert(s.begin(geometry(),start,sum(at(0),offset),7,true,0,false,start.observed_ns));
    assert(s.bind_original_motion(first,{},gears_vr::control_squeeze,true,start.observed_ns));
    for(unsigned n=0;n<50;++n){h=window(2+n*5,0,0);raw=h.frames[h.count-1];f=stamp(raw);
        assert(s.move_original_motion(f,sum(at(0),offset),7,true,f.observed_ns,raw,h,{},true));assert(s.wanted_angle()==0);}
    assert(!s.acknowledge(f,1.4,f.observed_ns));++cases;
    // Closure still needs real inward hand path and the CURRENT measured closed bone.
    assert(begin(s,input(1,1.4),1.4,true));h=window(2,1.26,-.14);raw=h.frames[h.count-1];f=stamp(raw);
    assert(s.move_original_motion(f,at(.7),7,true,f.observed_ns,raw,h,{},true));h=window(7,.56,-.14);raw=h.frames[h.count-1];f=stamp(raw);
    assert(s.move_original_motion(f,at(0),7,true,f.observed_ns,raw,h,{},true));assert(!s.acknowledge(f,1.4,f.observed_ns));assert(s.acknowledge(f,0,f.observed_ns));++cases;
    assert(begin(s));raw=input(2,.14);f=stamp(raw);assert(!s.move(f,at(.14),7,true,f.observed_ns));++cases;
    std::printf("BoltokOriginalMotionTest passed:%u cases; original110.97ms consumer red, original source spacings with synthetic geometry, fixed whole-stroke registration, strict current100ms/identity/token/mount, original geometric limits and real endpoint ACK\n",cases);
}
