// Included inside handstream only in the private typed Rig profile. This file
// reads data and adapts geometry; it creates no native or engine permission.
namespace rig_source {
constexpr unsigned expected_source=GEARS_PLAYGROUND_HAND_MOTION_EXPECTED_SOURCE;
static_assert(expected_source<=1);
bool object(uptr address,gears_weapon_hand::ObjectIdentity& out){
    out={};
    if(!address)return true;
    if(!registered(address)||playground::deleted(address))return false;
    std::array<unsigned char,0x58> header{};Name name{};
    if(!copy(header.data(),address,header.size()))return false;
    out.address=address;std::memcpy(&out.registry_index,header.data()+8,4);
    std::memcpy(&out.outer,header.data()+0x40,8);std::memcpy(&name,header.data()+0x48,8);
    std::memcpy(&out.klass,header.data()+0x50,8);
    out.name_index=name.index;out.name_number=name.number;out.known=1;
    return gears_weapon_hand::identity_valid(out)&&registered(address)&&!playground::deleted(address);
}
bool objects(XrContext& context){
    auto const& g=context.game;uptr world{},pawn{},mesh{},weapon{};
    context.objects={};
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||
       !handik::pointer(uptr(g.controller),L"WorldInfo",world)||world!=g.world||
       !handik::pointer(uptr(g.controller),L"Pawn",pawn)||pawn!=g.pawn||
       !handik::pointer(pawn,L"Mesh",mesh,true)||mesh!=g.mesh||
       !handik::pointer(pawn,L"Weapon",weapon)||weapon!=g.weapon)return false;
    return object(world,context.objects.world)&&object(uptr(g.controller),context.objects.controller)&&
        object(pawn,context.objects.pawn)&&object(mesh,context.objects.mesh)&&object(weapon,context.objects.weapon);
}
XrContext live_context(XrContext const& sampled){
    // Refresh identity and the local action generation, not the snapshot's
    // sample time or the source's timestamps. During our own partial acquire,
    // clean describes the checked pre-acquire context; the journal separately
    // validates its exact pending fields before every write and rollback.
    auto live=sampled;live.game_thread=GetCurrentThreadId();
    live.action_epoch=trial.epoch;live.input_revision=std::uint64_t(trial.epoch)+1;
    live.idle_ready=live.idle_ready&&trial.xr_ready&&!trial.ended&&trial.phase==2;
    if(!objects(live)||trial.epoch!=live.action_epoch)live.idle_ready=false;
    return live;
}
gears_rig_tracking::ReadResult read(gears_rig_tracking::Frame& out,std::uint64_t minimum){
    auto result=handrig::read(out,minimum);
    if(result.reply!=gears_rig_tracking::Reply::accepted||result.failure!=gears_rig_tracking::ReadFailure::none)return result;
    auto const& start=trial.xr_start;
    if(out.native.game_pid!=start.game_pid||out.native.game_start!=start.game_creation_time||
       out.native.native_started_qpc!=start.native_started_qpc||
       out.native.game_thread!=GetCurrentThreadId()||out.native.game_thread!=game_thread.load(std::memory_order_acquire)||
       out.world_generation!=start.expected_native_world_generation||
       static_cast<unsigned>(out.stereo.source)!=expected_source){
        out={};result.reply=gears_rig_tracking::Reply::invalid;result.failure=gears_rig_tracking::ReadFailure::envelope;
    }
    return result;
}
bool received(gears_rig_tracking::ReadResult const& result) noexcept {
    return result.reply==gears_rig_tracking::Reply::accepted&&result.failure==gears_rig_tracking::ReadFailure::none;
}
bool enable(gears_rig_tracking::Frame const& first){
    if(trial.xr_rig_enabled)return true;
    // first is the successful real Native read above, not a Start-derived or
    // synthetic frame. The following observe still consumes a baseline only.
    trial.xr_rig_enabled=trial.xr_policy.enable({first.native,expected_source,
        first.controller_publisher_pid,first.controller_publisher_start});
    return trial.xr_rig_enabled;
}
bool geometry(gears_rig_tracking::Frame const& rig,std::uint64_t observed_ms,gears_weapon::HandWorldFrame& out) noexcept {
    out={};out.serial=rig.publication_sequence;out.created_ms=observed_ms;
    out.session=rig.stereo.session;out.sequence=rig.stereo.sequence;out.space_epoch=rig.stereo.space_epoch;
    out.input_epoch=rig.controllers.input_epoch;out.world_generation=rig.world_generation;
    out.controller=uptr(rig.controller.address);out.pawn=uptr(rig.pawn.address);out.weapon=uptr(rig.weapon.address);
    out.units_per_metre=rig.units_per_metre;out.valid_mask=std::uint8_t((rig.world_pose_mask>>1)&3u);
    for(unsigned hand=0;hand<2;++hand)if(out.valid_mask&(1u<<hand)){
        auto const& pose=rig.world_grip[hand];
        if(!gears_vr::stereo_pose_valid(pose))return false;
        // Already UE world X-forward/Y-right/Z-up. Angles are radians; there
        // is deliberately no xr_to_ue or second base/reference transform.
        const gears_vr::Viewpoint value{pose.position,gears_vr::ue_angles(gears_vr::rotation_matrix(pose.orientation))};
        if(hand)out.right=value;else out.left=value;
    }
    return true;
}
bool recent(gears_rig_tracking::Frame const& frame,std::int64_t now) noexcept {
    return now>0&&frame.stereo.received_ns>0&&frame.controllers.received_ns>0&&
        now>=frame.stereo.received_ns&&now>=frame.controllers.received_ns&&
        now-frame.stereo.received_ns<=100000000&&now-frame.controllers.received_ns<=100000000;
}
void log_receipt(gears_rig_tracking::Frame const& frame,const wchar_t* stage,
                 std::uint64_t observed_ms,std::int64_t observed_ns){
    if(!log_file)return;
    fwprintf(log_file,L"hand_rig_receipt schema=1 continuity=RigV1 stage=%ls bytes=%zu "
        L"publication=%llu scene=%llu native_cookie=%llu native_activation=%llu game_pid=%u game_start=%llu gt=%u "
        L"world_generation=%llu binding_generation=%llu xr_pid=%u xr_start=%llu source=%u "
        L"raw_session=%llu raw_sequence=%llu producer_stream=%llu tracking_left=%llu tracking_right=%llu "
        L"native_reference_epoch=%llu reference_flags=%u reference_generation=%llu "
        L"observed_stream=%llu observed_reference=%llu observed_head=%llu observed_left=%llu observed_right=%llu "
        L"receipt_ms=%llu receipt_ns=%lld stereo_ns=%lld input_ns=%lld published_ns=%llu "
        L"current_tick_equivalence=0 handworld_adapter_only=1 engine_permission=0 frame_hex=",
        stage,sizeof(frame),frame.publication_sequence,frame.scene_invocation,frame.native.registration_cookie,
        frame.native.native_started_qpc,frame.native.game_pid,frame.native.game_start,frame.native.game_thread,
        frame.world_generation,frame.binding_generation,frame.controller_publisher_pid,frame.controller_publisher_start,
        unsigned(frame.stereo.source),frame.stereo.session,frame.stereo.sequence,frame.producer.stream,
        frame.producer.tracking[0],frame.producer.tracking[1],frame.native_reference.epoch,frame.native_reference.flags,
        frame.reference_generation,frame.observed.stream,frame.observed.reference,frame.observed.head_tracking,
        frame.observed.hand_tracking[0],frame.observed.hand_tracking[1],observed_ms,observed_ns,
        frame.stereo.received_ns,frame.controllers.received_ns,frame.published_ns);
    auto const* bytes=reinterpret_cast<unsigned char const*>(&frame);
    for(std::size_t i=0;i<sizeof(frame);++i)fwprintf(log_file,L"%02x",unsigned(bytes[i]));
    fwprintf(log_file,L"\n");
}
} // namespace rig_source
