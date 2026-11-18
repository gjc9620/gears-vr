// Stack-only, one-use observation from bind_contact to the immediately
// following bolt sample. HUD Engine calls run BEFORE capture; no original
// call may be inserted between capture and consume. This is never authority.
struct BonesReceipt {
    gears_longshot_curve::Receipt context{};
    gears_vr::StereoSample stereo{};P reference{};
    std::uint32_t stereo_status{},recentered{},heading_deferred{},reserved{},reserved2{};
    P root{},bolt{};double scale{},units{};
    float observed_time{},observed_rate{};bool observed_playing{},observed_looping{},stopped{};
    std::uint64_t serial{},hold{};bool complete{},used{};
};
BonesReceipt capture_bones(hand::Frame const& f,P root,P bolt,double scale) noexcept {
    BonesReceipt out{};out.context=gears_longshot_curve::capture(f,hammerburst_animation::ownership.last);
    out.stereo=f.stereo;out.reference=f.reference;out.stereo_status=f.stereo_status;
    out.recentered=f.recentered;out.heading_deferred=f.heading_deferred;out.reserved=f.reserved;out.reserved2=f.reserved2;
    out.root=root;out.bolt=bolt;out.scale=scale;out.units=nativelancerfollow::bound_units;
    out.serial=longshot_clip::owned_serial;out.hold=nativelancerfollow::bound_key.hold_generation;
    const auto& animation=hammerburst_animation::ownership;
    out.observed_time=animation.last.time;out.observed_rate=animation.last.rate;
    out.observed_playing=animation.last.playing;out.observed_looping=animation.last.looping;out.stopped=animation.stopped;
    out.complete=out.context.complete&&out.serial&&gears_vr::stereo_pose_valid(root)&&
        gears_vr::stereo_pose_valid(bolt)&&std::isfinite(scale)&&scale>.1&&scale<10;
    return out;
}
bool consume_bones(BonesReceipt& r,hand::Frame const& f,P& root,P& bolt,double& scale) noexcept {
    const bool unused=!r.used;r.used=true;
    if(!unused||!r.complete||!r.serial||r.serial!=generation||r.serial!=longshot_clip::owned_serial||
       r.hold!=nativelancerfollow::bound_key.hold_generation||r.units!=nativelancerfollow::bound_units||
       !hand::same(r.context.component,nativelancerfollow::bound_key.component)||
       !hand::same(r.context.asset,nativelancerfollow::bound_key.asset)||
       !gears_longshot_curve::matches(r.context,f,hammerburst_animation::ownership.last)||
       !r.stopped||!hammerburst_animation::ownership.stopped||
       r.observed_time!=hammerburst_animation::ownership.last.time||r.observed_rate!=hammerburst_animation::ownership.last.rate||
       r.observed_playing!=hammerburst_animation::ownership.last.playing||r.observed_looping!=hammerburst_animation::ownership.last.looping||
       !gears_vr::stereo_same(r.stereo,f.stereo)||!gears_vr::stereo_same_pose(r.reference,f.reference)||
       r.stereo_status!=f.stereo_status||r.recentered!=f.recentered||r.heading_deferred!=f.heading_deferred||r.reserved!=f.reserved||r.reserved2!=f.reserved2||
       !gears_vr::stereo_pose_valid(r.root)||!gears_vr::stereo_pose_valid(r.bolt)||
       !std::isfinite(r.scale)||r.scale<=.1||r.scale>=10)return false;
    root=r.root;bolt=r.bolt;scale=r.scale;return true;
}
