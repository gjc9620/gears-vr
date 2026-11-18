namespace close_release {
using Witness=gears_boltok_close_release::Witness;
using Guard=gears_boltok_close_release::Guard;

Guard guard;

void reset() noexcept {
    guard.reset();
}

Witness from(gears_hand_continuity::TrackedInput const& raw,
    gears_hand_continuity::Publisher const& publisher) noexcept {
    Witness w{};
    w.source=unsigned(raw.input.source);
    w.publisher_pid=publisher.pid;
    w.publisher_start=publisher.start;
    w.session=raw.input.session;
    w.space_epoch=raw.input.space_epoch;
    w.input_epoch=raw.input.input_epoch;
    w.profile_generation=raw.input.profile_generation;
    w.stream=raw.stream_epoch;
    w.left_tracking=raw.tracking_epoch[0];
    w.right_tracking=raw.tracking_epoch[1];
    w.sequence=raw.input.sequence;
    w.pressed=raw.input.hands[0].pressed[gears_vr::control_squeeze];
    w.released=raw.input.hands[0].released[gears_vr::control_squeeze];
    w.sync_ns=raw.input.sync_ns;
    const auto& left=raw.input.hands[0];
    w.held=gears_weapon_hand::control_held(
        left,gears_vr::control_squeeze);
    w.focused=raw.input.focused!=0;
    w.valid=gears_hand_continuity::tracked_input_shape_valid(raw)&&
        gears_vr::controller_pose_usable(
            raw.input.hands[0].grip,
            raw.input.hands[0].grip_flags,
            raw.input.hands[0].grip_active)&&
        gears_vr::controller_pose_usable(
            raw.input.hands[1].grip,
            raw.input.hands[1].grip_flags,
            raw.input.hands[1].grip_active)&&
        left.squeeze_active==1;
    return w;
}

bool begin(hand::Frame const& f) noexcept {
    guard.reset();
    if(!motion::current.valid||
       !hand::same_frame(motion::current.frame,f)||
       !gears_hand_continuity::same_input(motion::current.raw.input,f.controllers))return false;
    return guard.begin(from(motion::current.raw,motion::current.publisher));
}

bool cancel(hand::Frame const& f,std::int64_t actual_endpoint_ns) noexcept {
    if(!perfect_window.armed()||!guard.known()||
       !weaponhand::current_gt(nullptr)||
       !nativeequipment::domain_ready()||
       !nativeequipment::current(nullptr,f))return false;

    gears_vr::StereoSample stereo{};
    gears_vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput latest{};
    gears_hand_continuity::Publisher publisher{};
    gears_hand_continuity::MotionHistory history{};
    const auto read_begin=weaponhand::clock_ns(nullptr);
    const bool sim=(f.flags&hand::allow_sim)!=0;
    const auto failure=nativeequipment::input_continuity_reader.read(
        GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
        sim,stereo,input,latest,publisher,&history);
    const auto postnow=weaponhand::clock_ns(nullptr);
    if(failure!=gears_current_tracking::Failure::none||postnow<read_begin||postnow-read_begin>40000000||
       !nativeequipment::current(nullptr,f))return false;
    if(postnow<=0||postnow<f.controllers.sync_ns||postnow-f.controllers.sync_ns>100000000)return false;
    if(latest.input.sync_ns<=0||latest.input.sync_ns>postnow||
       postnow-latest.input.sync_ns>100000000)return false;
    const auto latest_witness=from(latest,publisher);
    if(!guard.same_domain(latest_witness))return false;
    if(history.count==0||history.count>32)return false;

    for(unsigned i=0;i<history.count;++i){
        const auto& point=history.frames[i];
        if(point.input.sequence>latest.input.sequence||
           point.input.sync_ns>latest.input.sync_ns)continue;
        const auto candidate=from(point,publisher);
        if(!guard.same_domain(candidate))continue;
        if(!guard.revoke(actual_endpoint_ns,postnow,candidate))continue;
        perfect_window.reset();
        static unsigned rows{};
        if(log_file&&rows<8){
            ++rows;
            fwprintf(log_file,
                L"boltok_perfect_cancel reason=original_release_history "
                L"frame=%llu sequence=%llu serial=%llu "
                L"release_sequence=%llu release_sync_ns=%lld "
                L"endpoint_ns=%lld now_ns=%lld\n",
                f.key.frame,f.controllers.sequence,reload_serial,
                point.input.sequence,point.input.sync_ns,
                actual_endpoint_ns,postnow);
            fflush(log_file);
        }
        return true;
    }
    return false;
}
} // namespace close_release
