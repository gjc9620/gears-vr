// Normal-GT adapter only. Raw matched controller poses drive the stroke;
// current original barrel matrices provide the independent later-frame ACK.
#include "weapon_boomshot_inspection_clip.inl"
namespace boomshot_mechanism_clip {
enum class Mode {none,native_reload,inspection};
Mode mode() noexcept {
    const bool native=boomshot_clip::serial!=0,inspection=boomshot_inspection_clip::owner.active();
    return native==inspection?Mode::none:native?Mode::native_reload:Mode::inspection;
}
std::uint64_t serial() noexcept {
    const auto current=mode();return current==Mode::native_reload?boomshot_clip::serial:
        current==Mode::inspection?boomshot_inspection_clip::owner.serial():0;
}
bool owned() noexcept {
    const auto current=mode();return current==Mode::native_reload?boomshot_clip::owned():
        current==Mode::inspection&&boomshot_inspection_clip::backend.mine();
}
bool ready(gears_weapon_hand::Frame const& f) noexcept {
    const auto current=mode();return current==Mode::native_reload?boomshot_clip::service(f)==boomshot_clip::Result::ready:
        current==Mode::inspection&&boomshot_inspection_clip::owner.service(f)==boomshot_inspection_clip::Result::ready;
}
bool seek(gears_weapon_hand::Frame const& f,double q,gears_weapon::boomshot_barrel::Direction direction) noexcept {
    const auto current=mode();return current==Mode::native_reload?boomshot_clip::seek(f,q,direction):
        current==Mode::inspection&&boomshot_inspection_clip::owner.seek(f,q,direction);
}
}
namespace boomshot_barrel {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace motion=gears_weapon::boomshot_barrel;namespace rigabi=gears_rig_tracking;
using V=gears_vr::Vector;using P=gears_vr::Pose;
motion::Stroke stroke{};inv::Identity identity{};gears_vr::Quaternion closed_rotation{};
rigabi::Frame continuity{};std::uint64_t generation{};double component_scale{};bool prepared{};
double last_progress{};bool last_progress_known{};
motion::Direction direction{};
boomshot_mechanism_clip::Mode clip_mode{};
V sub(V a,V b) noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
V local(P const& root,V point,double scale) noexcept {
    const auto result=gears_weapon::rotate(gears_vr::inverse(root.orientation),sub(point,root.position));
    return {result.x/scale,result.y/scale,result.z/scale};
}
bool bone(hand::Frame const& f,P& root,P& relative,double& scale) noexcept {
    hammerburst_animation::Parts parts{};P barrel{};double barrel_scale{};
    if(!boomshot_parts::read(f,parts)||!hammerburst_runtime::matrix(parts.world[0],root,scale)||
       !hammerburst_runtime::matrix(parts.world[2],barrel,barrel_scale)||std::abs(scale-barrel_scale)>.001)return false;
    relative.position=local(root,barrel.position,scale);
    relative.orientation=gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(root.orientation),barrel.orientation));
    return gears_vr::stereo_pose_valid(relative)&&nativeequipment::current(nullptr,f);
}
bool current(hand::Frame const& f) noexcept {
    return prepared&&clip_mode==boomshot_mechanism_clip::mode()&&generation==boomshot_mechanism_clip::serial()&&boomshot_mechanism_clip::owned()&&
        identity==inv::identity(f)&&nativeequipment::current(nullptr,f);
}
bool prepare(hand::Frame const& f) noexcept {
    if(prepared)return current(f);
    if(!boomshot_mechanism_clip::ready(f))return false;
    P root{},relative{};double scale{},q{};
    if(!bone(f,root,relative,scale)||!motion::progress(relative.position,q)||q>.015)return false;
    // Capture a constant relative rotation from an ACTUAL closed original
    // instance. Do not import another weapon's quaternion-sign convention.
    closed_rotation=relative.orientation;identity=inv::identity(f);generation=boomshot_mechanism_clip::serial();clip_mode=boomshot_mechanism_clip::mode();
    component_scale=scale;last_progress=q;last_progress_known=true;prepared=true;return true;
}
bool granted(hand::Frame const& f,hand::Decision const& d) noexcept {
    return hand::same(f.key,d.key)&&!d.release_required&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        d.lease.kind==hand::LeaseKind::magazine&&hand::lease_valid(d.lease,f.key.lifecycle)&&
        d.lease.id==f.press_token&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
bool sample(hand::Frame const& f,hand::Decision const& d,std::uint64_t operation,
    motion::Sample& out,rigabi::Frame& rig) noexcept {
    if(!current(f)||!operation||!granted(f,d))return false;
    P root{},relative{};double scale{},q{};
    if(!bone(f,root,relative,scale)||std::abs(scale-component_scale)>.001||!motion::progress(relative.position,q))return false;
    const auto source=static_cast<std::uint32_t>(f.stereo.source);const auto now=weaponhand::clock_ns(nullptr);
    if(handrig::read(rig,0,source).reply!=rigabi::Reply::accepted||
       gears_equipment_rig::match(f,rig,now,source)!=gears_equipment_rig::Status::matched||
       (rig.world_pose_mask&(rigabi::left_grip_pose|rigabi::right_grip_pose))!=(rigabi::left_grip_pose|rigabi::right_grip_pose))return false;
    const auto units=rig.units_per_metre;
    if(!std::isfinite(units)||units<10||units>500||std::abs(units-nativelancerfollow::bound_units)>.001)return false;
    float time{};if(!lancerreload_state::time_field(f.world.address,L"TimeSeconds",time)||
        !std::isfinite(time)||time<0||time>1000000000.f)return false;
    const double dot=std::abs(relative.orientation.x*closed_rotation.x+relative.orientation.y*closed_rotation.y+
        relative.orientation.z*closed_rotation.z+relative.orientation.w*closed_rotation.w);
    motion::Sample value{};
    value.stamp={{f.world.address,f.pawn.address,f.weapon.address,f.key.lifecycle},generation,
        f.controllers.sequence,rig.native_reference.epoch,std::int64_t(double(time)*1000000.),f.controllers.sync_ns};
    value.hand_m=local(root,rig.world_grip[0].position,scale*units);
    value.frame=f.key.frame;value.lease=d.lease.id;value.press=f.press_token;value.operation=operation;
    value.actual_progress=q;
    value.position_error_m=std::hypot(relative.position.x-motion::closed_raw.x,relative.position.z-motion::closed_raw.z)/units;
    value.rotation_error_degrees=2*std::acos((std::clamp)(dot,0.,1.))*180/3.14159265358979323846;
    value.grip_held=value.left_tracked=value.right_tracked=value.lease_current=value.bone_measured=true;
    value.grant_proven=false; // Only begin() may authenticate the purpose receipt.
    if(!current(f)||!lancerreload_state::item_owned(f))return false;
    last_progress=q;last_progress_known=true;out=value;return true;
}
bool snapshot_progress(inv::Identity const& expected,std::uint64_t serial,double& out,bool gun_only=false) noexcept {
    if(!weaponhand::current_gt(nullptr)||!prepared||!last_progress_known||!(identity==expected)||generation!=serial||
       clip_mode!=(gun_only?boomshot_mechanism_clip::Mode::inspection:boomshot_mechanism_clip::Mode::native_reload))return false;
    out=last_progress;return std::isfinite(out)&&out>=0&&out<=1;
}
bool same_continuity(rigabi::Frame const& a,rigabi::Frame const& b) noexcept {
    return a.native_reference.epoch==b.native_reference.epoch&&a.binding_generation==b.binding_generation&&
        a.view_policy==b.view_policy&&a.units_per_metre==b.units_per_metre&&
        a.reference_generation==b.reference_generation&&a.controller_publisher_pid==b.controller_publisher_pid&&
        a.controller_publisher_start==b.controller_publisher_start&&a.observed.stream==b.observed.stream&&
        a.observed.reference==b.observed.reference&&a.observed.head_tracking==b.observed.head_tracking&&
        a.observed.hand_tracking[0]==b.observed.hand_tracking[0]&&a.observed.hand_tracking[1]==b.observed.hand_tracking[1]&&
        a.producer.stream==b.producer.stream&&a.producer.tracking[0]==b.producer.tracking[0]&&a.producer.tracking[1]==b.producer.tracking[1];
}
bool begin(hand::Frame const& f,hand::Decision const& d,std::uint64_t operation,motion::Direction wanted,
    gears_weapon::BoomshotMechanismGrant& grant) noexcept {
    const auto purpose=wanted==motion::Direction::opening?gears_weapon::BoomshotContact::opening:gears_weapon::BoomshotContact::closing;
    if(!grant.begin_allowed(f,d,purpose,weaponhand::clock_ns(nullptr)))return false;
    if(!prepare(f))return false;motion::Sample input{};rigabi::Frame rig{};
    if(!sample(f,d,operation,input,rig))return false;
    const auto now=weaponhand::clock_ns(nullptr);input.grant_proven=grant.begin_allowed(f,d,purpose,now);
    if(!stroke.begin(input,wanted,rig.units_per_metre,now))return false;
    if(!grant.started(f,d,purpose,weaponhand::clock_ns(nullptr))){stroke.release();return false;}
    continuity=rig;direction=wanted;
    // Observation only, after BOTH the actual RAW kernel and grant accepted.
    // Preserve its original stamp/baseline; no additional getter or clock.
    static unsigned begin_rows{};
    if(log_file&&begin_rows<32){++begin_rows;
        fwprintf(log_file,L"boomshot_barrel_begin serial=%llu operation=%llu direction=%u game_start=%llu native_start=%llu registration=%llu lifecycle=%llu world_generation=%llu hold_generation=%llu session=%llu space=%llu input=%llu world=%llx pawn=%llx weapon=%llx frame=%llu sequence=%llu sync_ns=%lld token=%llu reference=%llu profile=%llu hand_x=%.17g hand_y=%.17g hand_z=%.17g progress=%.17g units=%.17g kernel_started=1 grant_started=1 observation_only=1\n",
            generation,operation,unsigned(wanted),identity.game_start,identity.native_start,identity.registration,identity.lifecycle,
            identity.world_generation,identity.hold_generation,identity.session,identity.space,identity.input,
            identity.world.address,identity.pawn.address,identity.weapon.address,f.key.frame,f.controllers.sequence,f.controllers.sync_ns,
            f.press_token,f.reference_generation,f.controllers.profile_generation,input.hand_m.x,input.hand_m.y,input.hand_m.z,
            input.actual_progress,rig.units_per_metre);
        fflush(log_file);
    }
    return true;
}
motion::Result observe(hand::Frame const& f,hand::Decision const& d,std::uint64_t operation) noexcept {
    motion::Sample input{};rigabi::Frame rig{};
    if(!sample(f,d,operation,input,rig)||!same_continuity(continuity,rig)){stroke.release();return motion::Result::interrupted;}
    const auto result=stroke.observe(input,weaponhand::clock_ns(nullptr));
    if(result==motion::Result::moved&&!boomshot_mechanism_clip::seek(f,stroke.wanted(),direction)){
        stroke.release();return motion::Result::interrupted;}
    return result;
}
// One receipt from the actual raw-controller kernel, tied to THIS current
// Native/clip context. No restamping or animation-time endpoint inference.
bool take_completion(hand::Frame const& f,hand::Decision const& d,std::uint64_t operation,
    motion::Completion& out) noexcept {
    out={};
    if(!weaponhand::current_gt(nullptr)||!current(f)||!granted(f,d))return false;
    motion::Completion proof{};if(!stroke.take_completion(proof))return false;
    const auto& first=proof.began();const auto& last=proof.ended();
    if(!proof.valid()||proof.direction()!=direction||first.operation!=operation||last.operation!=operation||
       first.lease!=d.lease.id||last.lease!=d.lease.id||first.press!=f.press_token||last.press!=f.press_token||
       last.frame!=f.key.frame||last.stamp.sequence!=f.controllers.sequence||last.stamp.monotonic_ns!=f.controllers.sync_ns||
       last.stamp.binding!=generation||last.stamp.identity.world!=f.world.address||last.stamp.identity.pawn!=f.pawn.address||
       last.stamp.identity.weapon!=f.weapon.address||last.stamp.identity.generation!=f.key.lifecycle||
       !nativeequipment::current(nullptr,f))return false;
    out=proof;return true;
}
void release() noexcept {stroke.release();continuity={};}
// Only the reload owner may retire calibration after native/animation debts
// have both drained. Releasing a Grip never resets the four-second ledger.
bool retire() noexcept {
    if(boomshot_clip::serial||boomshot_inspection_clip::owner.active())return false;
    stroke={};identity={};closed_rotation={};continuity={};generation=0;component_scale=last_progress=0;last_progress_known=false;prepared=false;clip_mode={};return true;
}
}
