// Physical, reversible Longshot bolt travel. The reload runtime owns when
// opening/closing is legal and owns ammo, UI and completion. This adapter
// only issues positions through the existing stopped animation owner.
#include "../vr-common/weapon_longshot_bolt_contact.h"
#include "weapon_longshot_history_evidence.h"
#include <optional>
#include <type_traits>
namespace longshot_bolt {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace tracks=gears_weapon::longshot_tracks;
using V=gears_vr::Vector;using P=gears_vr::Pose;
tracks::BoltPath path;tracks::BoltStroke stroke;longshot_animation::Curve curve;
inv::Identity identity{};V handle_in_bone{};std::uint64_t generation{},last_frame{};
double last_wanted{};bool prepared{};unsigned action_gate{};
#include "weapon_longshot_bolt_trace.inl"
#if __has_include("weapon_source_validation_scope.inl")
// One synchronous Longshot call chain may borrow its enclosing exact-Frame
// witness. Every check still validates all original bytes twice and HandClient
// still checks lifetime/current100ms; no action or observation result is cached.
struct Validation {
    using Scope=weaponhand::source_validation::Scope;
    std::optional<Scope> owned;Scope* selected{};
    explicit Validation(hand::Frame const& f) noexcept {
        auto* enclosing=weaponhand::source_validation::active;
        if(enclosing&&!enclosing->disabled&&enclosing->matches(f))selected=enclosing;
        else selected=&owned.emplace(f);
    }
    Scope& get() noexcept {return *selected;}
};
#endif
inline V sub(V a,V b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V mul(V a,double n) noexcept {return {a.x*n,a.y*n,a.z*n};}
inline V add(V a,V b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V point(P const& p,V v,double scale=1) noexcept {
    return add(p.position,gears_weapon::rotate(p.orientation,mul(v,scale)));
}
inline V local(P const& p,V v,double scale=1) noexcept {
    return mul(gears_weapon::rotate(gears_vr::inverse(p.orientation),sub(v,p.position)),1/scale);
}
bool current(hand::Frame const& f) noexcept {
    return prepared&&identity==inv::identity(f)&&longshot_clip::owned()&&
        generation==longshot_clip::owned_serial&&nativelancerfollow::bound_is_longshot&&
        hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&nativeequipment::current(nullptr,f);
}
bool bones(hand::Frame const& f,P& root,P& bolt,double& scale) noexcept {
    hammerburst_animation::Parts parts{};double bolt_scale{};
    if(!longshot_parts::read_bolt_pair(f,parts)||!hammerburst_runtime::matrix(parts.world[0],root,scale)||
       !hammerburst_runtime::matrix(parts.world[1],bolt,bolt_scale)||std::abs(scale-bolt_scale)>.001)return false;
    return std::isfinite(scale)&&scale>.1&&scale<10&&nativeequipment::current(nullptr,f);
}
#include "weapon_longshot_bones_receipt.inl"
bool native_pose(hand::Frame const& f,P& root,P& bolt,double& scale,P& relative,BonesReceipt* receipt=nullptr) noexcept {
    const bool borrowed=receipt&&consume_bones(*receipt,f,root,bolt,scale);
    if(borrowed?!nativeequipment::current(nullptr,f):!bones(f,root,bolt,scale))return false;
    relative.position=local(root,bolt.position,scale);
    relative.orientation=gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(root.orientation),bolt.orientation));
    return gears_vr::stereo_pose_valid(relative);
}
double angle(gears_vr::Quaternion a,gears_vr::Quaternion b) noexcept {
    const double dot=std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w);
    return 2*std::acos((std::clamp)(dot,0.,1.))*180/3.14159265358979323846;
}
#include "weapon_longshot_bolt_history.inl"
#include "weapon_longshot_seek_budget.inl"
bool prepare_serviced(hand::Frame const& f) noexcept {
    struct Report {
        hand::Frame const& frame;unsigned gate{1},curve_gate{};
        double units{-1},scale{-1},distance{-1},degrees{-1};P actual{},closed{};
        ~Report() noexcept {
            static unsigned rows{};static std::array<unsigned,2> previous{{~0u,~0u}};
            const std::array<unsigned,2> key{{gate,curve_gate}};
            if(!log_file||rows>=16||key==previous)return;previous=key;++rows;
            const auto& read=longshot_animation::last_read;
            fwprintf(log_file,L"longshot_bolt_prepare gate=%u curve_gate=%u frame=%llu serial=%llu units=%.9g scale=%.9g position_error=%.9g angle_error=%.9g actual=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g closed=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g frames=%d formats=%u,%u,%u offsets=%d bytes=%d source_sync_ns=%lld\n",
                gate,curve_gate,frame.key.frame,longshot_clip::owned_serial,units,scale,distance,degrees,
                actual.position.x,actual.position.y,actual.position.z,actual.orientation.x,actual.orientation.y,actual.orientation.z,actual.orientation.w,
                closed.position.x,closed.position.y,closed.position.z,closed.orientation.x,closed.orientation.y,closed.orientation.z,closed.orientation.w,
                read.frames,unsigned(read.formats[0]),unsigned(read.formats[1]),unsigned(read.formats[2]),read.offsets,read.bytes,frame.controllers.sync_ns);
            fflush(log_file);
        }
    } report{f};
    if(prepared){const bool valid=current(f);if(valid)report.gate=0;return valid;}
    report.gate=2;
    if(!longshot_clip::owned()||!(hammerburst_animation::ownership.inventory==inv::identity(f))||
       !hammerburst_animation::ownership.stopped||hammerburst_animation::ownership.last.playing||
       longshot_clip::neutral_pending||!nativeequipment::current(nullptr,f))return false;
    longshot_animation::Curve captured_curve{};
    report.gate=3;
    const bool curve_ready=longshot_animation::read_curve(f,hammerburst_animation::ownership.last,captured_curve);
    report.curve_gate=longshot_animation::last_read.gate;
    if(!curve_ready)return false;
    const double units=nativelancerfollow::bound_units;
    P root{},bolt{},relative{};double scale{};
    report.units=units;report.gate=4;
    if(!std::isfinite(units)||units<10||units>500||!native_pose(f,root,bolt,scale,relative))return false;
    report.scale=scale;report.actual=relative;report.gate=5;
    P closed{};if(!tracks::sample(captured_curve.samples,3,0,closed))return false;
    report.closed=closed;
    report.distance=snub_visual::length(sub(relative.position,closed.position))/units;
    report.degrees=angle(relative.orientation,closed.orientation);report.gate=6;
    if(report.distance>.002||report.degrees>2)return false;
    vrequip::MeshPoseProbe mesh{};P gun{};double gun_scale{};
    report.gate=7;
    if(!vrequip::mesh_pose_probe(f.weapon.address,mesh)||mesh.mesh!=nativelancerfollow::bound_key.component.address||
       mesh.asset!=nativelancerfollow::bound_key.asset.address)return false;
    std::array<float,16> raw{};std::memcpy(raw.data(),mesh.local_to_world,sizeof mesh.local_to_world);
    report.gate=8;
    if(!hammerburst_runtime::matrix(raw,gun,gun_scale)||std::abs(gun_scale-scale)>.001)return false;
    // Actual original LOD0 knob-head centre, not the former stem vertex356.
    // Map it through CURRENT mesh and CLOSED bone transforms. No offline
    // assumption about the gun's final world matrix is used here.
    const auto handle=local(bolt,point(gun,gears_weapon_longshot_bolt_contact::knob_in_mesh_units,gun_scale),scale);
    tracks::BoltPath candidate;
    report.gate=9;
    if(!candidate.bind(captured_curve.samples,handle,units))return false;
    report.gate=10;if(!nativeequipment::current(nullptr,f))return false;
    curve=captured_curve;path=candidate;handle_in_bone=handle;identity=inv::identity(f);
    generation=longshot_clip::owned_serial;prepared=true;last_wanted=0;last_frame=0;
    report.gate=0;
    // Bounded read-only production geometry for actual-input verification.
    // These model points authorize no movement, endpoint or game operation.
    static unsigned reported_paths{};
    if(log_file&&reported_paths<2){
        for(unsigned i=0;i<=32;++i){
            float time{};P pose{};
            const double progress=double(i)/32;
            if(!path.time_for_progress(progress,time)||!tracks::sample(curve.samples,3,time,pose))break;
            const auto p=mul(point(pose,handle_in_bone),1/units);
            fwprintf(log_file,L"longshot_bolt_path serial=%llu weapon=%llx frame=%llu sample=%u progress=%.9g time=%.9g point=%.9g,%.9g,%.9g units=%.9g\n",
                generation,static_cast<unsigned long long>(f.weapon.address),f.key.frame,i,progress,double(time),p.x,p.y,p.z,units);
        }
        fflush(log_file);++reported_paths;
    }
    return true;
}
// Standalone callers still perform their own original service. Acquiring
// calls prepare_serviced immediately after its ready service, with no Engine
// call in between. No service result, Frame or pose survives this call chain.
bool prepare(hand::Frame const& f) noexcept {
    if(prepared)return current(f);
    if(longshot_clip::service(f)!=longshot_clip::Result::ready)return false;
    return prepare_serviced(f);
}
bool granted(hand::Frame const& f,hand::Decision const& d) noexcept {
    return hand::same(f.key,d.key)&&!d.release_required&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        d.lease.kind==hand::LeaseKind::magazine&&hand::lease_valid(d.lease,f.key.lifecycle)&&
        d.lease.id==f.press_token&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
bool sample(hand::Frame const& f,hand::Decision const& d,tracks::BoltSample& out,V& handle,BonesReceipt* receipt=nullptr) noexcept {
    history::sampled={};
    sample_gate=1;
    if(!current(f)||!longshot_clip::capture())return false;
    P root{},bolt{},relative{};double scale{};
    sample_gate=2;
    if(!native_pose(f,root,bolt,scale,relative,receipt))return false;
    gears_rig_tracking::Frame rig{};const auto source=static_cast<std::uint32_t>(f.stereo.source);
    sample_gate=3;
    if(handrig::read(rig,0,source).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),source)!=gears_equipment_rig::Status::matched||
       !(rig.world_pose_mask&gears_rig_tracking::left_grip_pose))return false;
    const double units=nativelancerfollow::bound_units;
    handle=mul(point(relative,handle_in_bone),1/units);
    sample_gate=4;
    double actual{};if(!path.fit(handle,last_wanted,1,.002,actual))return false;
    P expected{};
    sample_gate=5;
    if(!tracks::sample(curve.samples,3,hammerburst_animation::ownership.last.time,expected))return false;
    out={};out.stamp={longshot_fire::reload_identity(f),f.controllers.sequence,f.controllers.sync_ns,0,
        bool(rig.world_pose_mask&gears_rig_tracking::right_grip_pose),bool(f.left_tracked),true};
    out.frame=f.key.frame;out.lease=d.lease.id;out.path_generation=generation;
    out.hand_in_root_m=local(root,rig.world_grip[0].position,scale*units);
    out.actual_progress=actual;out.position_error_m=snub_visual::length(sub(relative.position,expected.position))/units;
    out.orientation_error_degrees=angle(relative.orientation,expected.orientation);
    out.grip_down=f.left_grip_known&&f.left_grip_down;out.lease_current=granted(f,d);out.new_grant=d.new_grant;out.bone_measured=true;
    sample_gate=6;
    if(!nativeequipment::current(nullptr,f))return false;
    history::capture(f,rig,root,scale,units,out.hand_in_root_m,curve.samples,history::sampled);
    return true;
}
bool begin(hand::Frame const& f,hand::Decision const& d,tracks::BoltDirection direction,hand::Lease const& accepted=hand::Lease{}) noexcept {
    tracks::BoltSample input{};V handle{};
    if(!prepare(f)||!sample(f,d,input,handle)){trace_motion(f,0,false,input,handle,stroke.last_observed_ns(),0);return false;}
    // An already committed activation may finish engine preparation while
    // the SAME current lease remains genuinely held. No old pose is reused.
    if(granted(f,d)&&hand::same(accepted,d.lease))input.new_grant=true;
    const auto now=weaponhand::clock_ns(nullptr);
    const auto previous=stroke.last_observed_ns();
    const bool begun=stroke.begin(path,input,direction,handle,now);
    history::previous=begun?history::sampled:history::Context{};
    trace_motion(f,begun?1u:0u,true,input,handle,previous,now);return begun;
}
tracks::BoltResult tick(hand::Frame const& f,hand::Decision const& d,BonesReceipt* bone_receipt=nullptr) noexcept {
#if __has_include("weapon_source_validation_scope.inl")
    // Synchronous liveness only: every recorded source byte is read again.
    // HandClient still checks the original frame lifetime and input owner.
    Validation validation_owner(f);auto& validation=validation_owner.get();
#endif
    action_gate=0;
    if(!current(f)){trace_motion(f,2,false,{}, {},stroke.last_observed_ns(),0);
#if __has_include("weapon_source_validation_scope.inl")
        trace_validation(f,validation,2);
#endif
        stroke.release();history::previous=history::sampled={};
        return tracks::BoltResult::interrupted;}
    if(f.key.frame==last_frame)return tracks::BoltResult::waiting;
    tracks::BoltSample input{};V handle{};
    if(!sample(f,d,input,handle,bone_receipt)){trace_motion(f,2,false,input,handle,stroke.last_observed_ns(),0);stroke.release();
        history::previous=history::sampled={};return tracks::BoltResult::interrupted;}
    last_frame=f.key.frame;
    const auto previous=stroke.last_observed_ns();const auto before=history::previous;
    history::Receipt receipt{};gears_controller_relative_motion::Segment segment{};bool continuous{};
    if(input.stamp.observed_ns>=previous&&input.stamp.observed_ns-previous>100000000){
        continuous=history::select(before,history::sampled,[&](auto& raw,auto& publisher,auto& window){
            gears_current_tracking::Failure failure{};
            const bool okay=nativeequipment::read_input_continuity(f,raw,publisher,nullptr,&failure,&window);
            receipt.transport_failure=unsigned(failure);return okay;
        },segment,receipt);
    }
    const auto now=weaponhand::clock_ns(nullptr);
    // Evaluate every original motion check without consuming the anchor.
    // Only an otherwise valid MOVE may wait for a later real current Frame.
    auto observed_stroke=stroke;
    const auto result=continuous?observed_stroke.observe_continuous(path,input,segment,now):observed_stroke.observe(path,input,now);
    if(result==tracks::BoltResult::moved&&!hammerburst_animation::ownership.seek_pending&&insufficient_seek_budget(f,now)){
        trace_seek_deferred(f,input,observed_stroke.wanted_progress(),previous,now);
        return tracks::BoltResult::waiting;
    }
    stroke=observed_stroke;
    if(result!=tracks::BoltResult::interrupted&&result!=tracks::BoltResult::rejected&&
       input.stamp.observed_ns>previous&&stroke.last_observed_ns()==input.stamp.observed_ns)
        history::previous=history::sampled;
    if(result==tracks::BoltResult::interrupted){trace_motion(f,3,true,input,handle,previous,now);
#if __has_include("weapon_source_validation_scope.inl")
        trace_validation(f,validation,3);
#endif
        history::previous={};
    }
    if(result==tracks::BoltResult::complete)trace_motion(f,5,true,input,handle,previous,now);
    if(result==tracks::BoltResult::moved){
        const auto wanted=stroke.wanted_progress();float time{};longshot_animation::Curve fresh{};
        if((action_gate=1,!path.time_for_progress(wanted,time))||
           (action_gate=2,!longshot_animation::read_curve(f,hammerburst_animation::ownership.last,fresh))||
           (action_gate=3,!hand::same(fresh.sequence,curve.sequence))||
           (action_gate=4,fresh.content!=curve.content)||
           (action_gate=5,!longshot_clip::seek(f,time,&fresh.observation))){
            trace_motion(f,4,true,input,handle,previous,now);
#if __has_include("weapon_source_validation_scope.inl")
            trace_validation(f,validation,4);
#endif
            history::report(f,before,history::sampled,receipt,segment,tracks::BoltResult::interrupted);
            history::previous={};
            stroke.release();return tracks::BoltResult::interrupted;
        }
        action_gate=0;
        last_wanted=wanted;
        trace_motion(f,6,true,input,handle,previous,now);
#if __has_include("weapon_source_validation_scope.inl")
        trace_validation(f,validation,6);
#endif
    }
    history::report(f,before,history::sampled,receipt,segment,result);
    return result;
}
void reset() noexcept {path={};stroke={};curve={};identity={};handle_in_bone={};history::sampled=history::previous={};generation=last_frame=0;last_wanted=0;prepared=false;}
}
