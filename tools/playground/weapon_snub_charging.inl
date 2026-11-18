// The original coupled slide/wing closing tail, driven by the actual wrist.
// Inventory, reward and firing debt belong to snub_runtime, never this pose.
namespace snub_charging {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;
using V=gears_vr::Vector;using P=gears_vr::Pose;
using Stroke=gears_weapon::SnubSlideStroke;
snub_animation::Curve curve;
Stroke stroke;
gears_weapon::SnubSlideGeometry geometry;
gears_weapon::SnubSlideCurve::Sampling sampling{};
std::uint64_t sequence{},bone_frame{};unsigned law_matches{};
double last_actual{},last_wanted{-1};bool release_seen{},issued_full{};
P root{};double root_scale{};
V contact_local{};lancerreload_inventory::Identity contact_identity{};bool contact_known{};
P observed_part{};hand::FrameKey observed_part_key{};std::uint64_t observed_part_sequence{};
// Observation only: never consumed by acquisition, motion, or currentness.
std::uint64_t progress_token{},progress_lifecycle{},progress_hand_frame{};
gears_weapon::HammerburstLatchSample progress_hand{};
unsigned progress_reports{},parts_failure{};bool progress_known{};
std::array<long long,23> progress_signature{};
void reset() noexcept {
    curve={};stroke.reset();geometry={};sampling={};sequence=bone_frame=0;law_matches=0;
    last_actual=0;last_wanted=-1;release_seen=issued_full=false;contact_known=false;
    observed_part={};observed_part_key={};observed_part_sequence=0;
    progress_token=progress_lifecycle=progress_hand_frame=0;progress_hand={};progress_known=false;parts_failure=0;
}
bool parts_rejected(unsigned line) noexcept {parts_failure=line;return false;}
bool parts(hand::Frame const& f,double& actual) noexcept {
    contact_known=false;parts_failure=0;
    snub_animation::Parts bones{};P slide{};double size{};
    if(!snub_animation::read_parts(f,bones)||!hammerburst_runtime::matrix(bones.world[0],root,root_scale)||
       !hammerburst_runtime::matrix(bones.world[1],slide,size)||std::abs(size-root_scale)>1e-3)return parts_rejected(__LINE__);
    const auto relative=snub_visual::mul(gears_weapon::rotate(gears_vr::inverse(root.orientation),
        snub_visual::sub(slide.position,root.position)),1/(root_scale*nativelancerfollow::bound_units));
    const double units=nativelancerfollow::bound_units;
    const V closed{7.087201118469238/units,.014453720301389694/units,10.67758846282959/units};
    if(!gears_vr::finite(relative)||std::hypot(relative.y-closed.y,relative.z-closed.z)>.001)return parts_rejected(__LINE__);
    actual=closed.x-relative.x;
    // The proposal reads current raw hands in gun-local coordinates. Pair
    // this part with the current rendered gun, not the old acquisition pose.
    P gun{};double gun_scale{};V local{};
    if(!snub_visual::gun_pose(f,gun,gun_scale)||
       std::abs(gun_scale-snub_visual::object_scale)>1e-6||
       !gears_weapon::snub_part_contact_local(gun,slide.position,local))return parts_rejected(__LINE__);
    geometry.closed_contact=closed;geometry.axis={-1,0,0};
    geometry.travel=curve.slide.travel();geometry.current_instance_verified=curve.slide.bound();
    if(!nativeequipment::current(nullptr,f))return parts_rejected(__LINE__);
    contact_local=local;contact_identity=lancerreload_inventory::identity(f);contact_known=true;
    observed_part=slide;observed_part_key=f.key;observed_part_sequence=f.controllers.sequence;
    return true;
}
bool hand_sample(hand::Frame const& f,gears_weapon::HammerburstLatchSample& out) noexcept {
    snub_visual::WristObservation wrist_sample{};if(!snub_visual::observe_wrist(f,wrist_sample))return false;
    const auto wrist=wrist_sample.pose;
    const auto local=snub_visual::mul(gears_weapon::rotate(gears_vr::inverse(root.orientation),
        snub_visual::sub(wrist.position,root.position)),1/(root_scale*nativelancerfollow::bound_units));
    out={local,wrist_sample.sequence(),std::uint64_t(wrist_sample.sync_ns()/1000000),f.press_token,
        bool(f.left_tracked),bool(f.left_grip_known&&f.left_grip_down)};
    const bool progress_finite=gears_vr::finite(local);
    if(progress_finite){progress_hand=out;progress_hand_frame=f.key.frame;}
    return progress_finite;
}
enum class Result {waiting,complete,recovery_required};
void arm_progress(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(d.new_grant&&snub_visual::granted(f,d)&&
       (progress_token!=f.press_token||progress_lifecycle!=f.key.lifecycle)){
        progress_token=f.press_token;progress_lifecycle=f.key.lifecycle;progress_known=false;
    }
}
void report_progress(hand::Frame const& f,hand::Decision const& d,std::uint64_t serial,
    unsigned gate,unsigned line,float game_time,double actual) noexcept {
    if(!log_file||progress_reports>=32||!progress_token||progress_token!=f.press_token||progress_lifecycle!=f.key.lifecycle)return;
    const auto quant=[](double value) noexcept {return std::isfinite(value)?std::llround(std::clamp(value,-1000.,1000.)*10000.):-10000001LL;};
    const std::array<long long,23> signature{gate,unsigned(stroke.phase()),unsigned(stroke.last_rejection()),
        quant(actual),quant(stroke.wanted_distance()),quant(last_wanted),unsigned(sampling),parts_failure,snub_animation::curve_failure,
        unsigned(issued_full),unsigned(release_seen),f.left_grip_down,f.left_tracked,f.left_grip_known,unsigned(d.status),d.new_grant,d.release_required,
        unsigned(anim::ownership.seek_pending),quant(progress_hand.hand.x),quant(progress_hand.hand.y),quant(progress_hand.hand.z),
        unsigned(anim::ownership.active),unsigned(anim::ownership.stopped)};
    if(progress_known&&signature==progress_signature)return;
    progress_signature=signature;progress_known=true;++progress_reports;
    const auto progress_observed_ns=gears_vr::monotonic_ns();
    fwprintf(log_file,L"snub_charging_progress gate=%u line=%u serial=%llu frame=%llu sequence=%llu token=%llu life=%llu phase=%u rejection=%u actual=%.9g wanted=%.9g dispatched=%.9g travel=%.9g full=%u issued_full=%u release_seen=%u sampling=%u curve_line=%u parts_line=%u game_time=%.9g observed_ns=%lld input_sync_ns=%lld down=%u tracked=%u known=%u decision=%u new_grant=%u release_required=%u lease_kind=%u lease_id=%llu seek_pending=%u animation_active=%u animation_stopped=%u animation_time=%.9g hand_frame=%llu hand_sequence=%llu hand_ms=%llu hand=%.9g,%.9g,%.9g\n",
        gate,line,serial,f.key.frame,f.controllers.sequence,f.press_token,f.key.lifecycle,unsigned(stroke.phase()),unsigned(stroke.last_rejection()),
        actual,stroke.wanted_distance(),last_wanted,curve.slide.bound()?curve.slide.travel():0.,unsigned(stroke.actual_full_pull()),
        unsigned(issued_full),unsigned(release_seen),unsigned(sampling),snub_animation::curve_failure,parts_failure,double(game_time),
        static_cast<long long>(progress_observed_ns),static_cast<long long>(f.controllers.sync_ns),f.left_grip_down,f.left_tracked,f.left_grip_known,
        unsigned(d.status),d.new_grant,d.release_required,unsigned(d.lease.kind),d.lease.id,unsigned(anim::ownership.seek_pending),
        unsigned(anim::ownership.active),unsigned(anim::ownership.stopped),double(anim::ownership.last.time),progress_hand_frame,
        progress_hand.sequence,progress_hand.milliseconds,progress_hand.hand.x,progress_hand.hand.y,progress_hand.hand.z);
    fflush(log_file);
}
unsigned recovery_reports{};
Result report_recovery(hand::Frame const& f,hand::Decision const& d,std::uint64_t serial,
    unsigned line,float game_time,double actual) noexcept {
    // Failure-only POD provenance. No additional engine/ownership query or
    // retry, and the caller retains its existing recovery result and cleanup.
    if(log_file&&recovery_reports<8){
        ++recovery_reports;const auto recovery_observed_ns=gears_vr::monotonic_ns();
        fwprintf(log_file,L"snub_charging_recovery line=%u serial=%llu frame=%llu sequence=%llu token=%llu life=%llu phase=%u rejection=%u actual=%.9g wanted=%.9g full=%u issued_full=%u release_seen=%u game_time=%.9g observed_ns=%lld input_sync_ns=%lld stereo_received_ns=%lld tracked=%u known=%u down=%u decision=%u new_grant=%u release_required=%u lease_kind=%u lease_id=%llu decision_frame=%llu same_key=%u animation_active=%u animation_stopped=%u animation_serial=%llu seek_pending=%u native_acquired=%u native_serial=%llu\n",
            line,serial,f.key.frame,f.controllers.sequence,f.press_token,f.key.lifecycle,
            unsigned(stroke.phase()),unsigned(stroke.last_rejection()),actual,stroke.wanted_distance(),
            unsigned(stroke.actual_full_pull()),unsigned(issued_full),unsigned(release_seen),double(game_time),
            static_cast<long long>(recovery_observed_ns),static_cast<long long>(f.controllers.sync_ns),static_cast<long long>(f.stereo.received_ns),
            unsigned(f.left_tracked),unsigned(f.left_grip_known),unsigned(f.left_grip_down),unsigned(d.status),
            unsigned(d.new_grant),unsigned(d.release_required),unsigned(d.lease.kind),d.lease.id,d.key.frame,unsigned(hand::same(f.key,d.key)),
            unsigned(anim::ownership.active),unsigned(anim::ownership.stopped),anim::ownership.serial,unsigned(anim::ownership.seek_pending),
            unsigned(lancerreload_state::lease.acquired),lancerreload_state::lease.serial);
        fflush(log_file);
    }
    return Result::recovery_required;
}
Result tick(hand::Frame const& f,hand::Decision const& d,std::uint64_t serial) noexcept {
    arm_progress(f,d);float now{};double actual{};
    const auto waiting=[&](unsigned gate,unsigned line) noexcept {report_progress(f,d,serial,gate,line,now,actual);return Result::waiting;};
    if(!curve.slide.bound()){
        if(!anim::ownership.active||!anim::ownership.stopped)return waiting(4,__LINE__);
        snub_animation::Curve value{};
        if(!snub_animation::read_curve(f,anim::ownership.last,value))return waiting(3,__LINE__);
        curve=value;return waiting(5,__LINE__);
    }
    if(!lancerreload_state::time_field(f.world.address,L"TimeSeconds",now))return waiting(1,__LINE__);
    if(!parts(f,actual))return waiting(2,__LINE__);
    if(!f.left_tracked||!f.left_grip_known)return report_recovery(f,d,serial,__LINE__,now,actual);
    const auto phase=stroke.phase();
    if(!f.left_grip_down){
        release_seen=true;
        if(phase==Stroke::Phase::pulling){
            // waiting_open may only await a full setter already dispatched.
            if(stroke.wanted_distance()>=curve.slide.travel()-.0015&&!issued_full){
                if(!stroke.cancel(now))return report_recovery(f,d,serial,__LINE__,now,actual);
            }else if(!stroke.release(now))return report_recovery(f,d,serial,__LINE__,now,actual);
        }
    }
    if(stroke.phase()==Stroke::Phase::idle){
        if(!release_seen||!f.left_grip_down||!snub_visual::granted(f,d)||!d.new_grant)return waiting(6,__LINE__);
        gears_weapon::HammerburstLatchSample sample{};
        if(!hand_sample(f,sample))return waiting(7,__LINE__);
        if(!stroke.begin(geometry,sample,actual))return waiting(8,__LINE__);
        sequence=sample.sequence;bone_frame=0;release_seen=false;issued_full=false;last_wanted=-1;
        return waiting(10,__LINE__);
    }
    if(stroke.phase()==Stroke::Phase::pulling&&f.left_grip_down){
        if(!snub_visual::granted(f,d))return report_recovery(f,d,serial,__LINE__,now,actual);
        gears_weapon::HammerburstLatchSample sample{};
        if(hand_sample(f,sample)&&sample.sequence>sequence){
            sequence=sample.sequence;
            if(!stroke.move(sample))return report_recovery(f,d,serial,__LINE__,now,actual);
        }
    }
    if(f.key.frame>bone_frame){
        bone_frame=f.key.frame;
        double endpoint{};
        if(sampling==gears_weapon::SnubSlideCurve::Sampling::reference_keys&&
           curve.slide.observe_distance_at_time(anim::ownership.last.time,endpoint,gears_weapon::SnubSlideCurve::Sampling::endpoint_intervals)&&
           std::abs(actual-endpoint)<.0008&&std::abs(actual-stroke.wanted_distance())>.0015&&std::abs(actual-last_actual)<.0003){
            if(++law_matches>=2){sampling=gears_weapon::SnubSlideCurve::Sampling::endpoint_intervals;last_wanted=-1;}
        }else law_matches=0;
        last_actual=actual;
        if(!stroke.acknowledge_bone(actual,f.key.frame,now))return report_recovery(f,d,serial,__LINE__,now,actual);
    }
    if(stroke.phase()==Stroke::Phase::complete){report_progress(f,d,serial,19,__LINE__,now,actual);return Result::complete;}
    if(stroke.phase()==Stroke::Phase::idle){last_wanted=-1;return waiting(14,__LINE__);}
    if(stroke.phase()==Stroke::Phase::returning&&!stroke.advance_return(now))return report_recovery(f,d,serial,__LINE__,now,actual);
    const auto wanted=stroke.wanted_distance();
    if(std::abs(wanted-last_wanted)>1e-7){
        if(!nativeequipment::current(nullptr,f))return waiting(16,__LINE__);
        const bool settling=stroke.phase()==Stroke::Phase::returning||stroke.phase()==Stroke::Phase::waiting_open;
        if(!snub_animation::seek_distance(f,serial,curve,wanted,sampling,settling))
            return anim::ownership.seek_pending?report_recovery(f,d,serial,__LINE__,now,actual):waiting(17,__LINE__);
        last_wanted=wanted;if(wanted>=curve.slide.travel()-.0015)issued_full=true;
    }
    return waiting(0,__LINE__);
}
} // namespace snub_charging
#include "weapon_snub_grasp.inl"
