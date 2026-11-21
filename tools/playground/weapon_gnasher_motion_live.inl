// Included inside gnasher_runtime, after raw(). Ring reads supply motion DATA
// only; all engine calls/rendering still consume the one current Frame.
namespace producer_motion {
namespace mc=gears_gnasher_motion_context;namespace seg=gears_gnasher_producer_segment;
mc::Context previous{};bool known{};
struct Step {mc::Context current{};weapon::GnasherProducerMotion motion{};bool captured{},available{};};
Step prepare(hand::Frame const& f,rig::Frame const& prepared,weapon::GnasherMechanicalSample const& sample,
    weapon::GnasherMechanicalInteraction const& mechanism) noexcept {
    Step step{};
    step.captured=mc::capture(f,prepared,weaponhand::clock_ns(nullptr),vrequip::model.hold_generation(),
        profile.geometry.mechanical.calibration_id,profile.geometry.curve_content,shot,sample,step.current);
    if(!step.captured){known=false;return step;}
    if(!known||!mc::same(previous,step.current)||
       !mechanism.source_matches(previous.raw.input.sequence,previous.raw.input.sync_ns))return step;
    const auto span=sample.monotonic_ns-previous.raw.input.sync_ns;
    if(span<=100000000||span>200000000)return step;
    gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    gears_hand_continuity::MotionHistory history{};seg::Segment segment{};
    if(!nativeequipment::read_input_continuity(f,raw,publisher,nullptr,nullptr,&history)||
       !gears_hand_continuity::same_tracking(raw,step.current.raw)||
       publisher.pid!=step.current.publisher.pid||publisher.start!=step.current.publisher.start||
       seg::verify(previous.raw,previous.publisher,raw,publisher,history,profile.geometry.mechanical,segment)!=seg::Status::ready||
       !seg::lever_motion(segment,profile.geometry.mechanical,sample.identity,step.motion.lever))return step;
    step.motion.identity=sample.identity;step.motion.reference_generation=sample.reference_generation;
    step.motion.support_grasp_token=sample.support_grasp_token;step.available=true;return step;
}
bool close(hand::Frame const& f,weapon::GnasherMechanicalSample const& sample,Step const& step) noexcept {
    if(!step.available)return false;
    hand::GripSnapshot ownership{};rig::Frame fresh{};mc::Context current{};
    return hand::frame_active(f,weaponhand::clock_ns(nullptr))&&nativeequipment::current(nullptr,f)&&
        vrequip::model.hold_generation()==step.current.hold_generation&&
        handrig::read(fresh,0,unsigned(f.stereo.source)).reply==rig::Reply::accepted&&
        mc::capture(f,fresh,weaponhand::clock_ns(nullptr),vrequip::model.hold_generation(),
            profile.geometry.mechanical.calibration_id,profile.geometry.curve_content,shot,sample,current)&&
        mc::same(step.current,current)&&gears_hand_continuity::same_tracking(step.current.raw,current.raw)&&
        weaponhand::client.snapshot_on_gt(ownership)&&
        gears_gnasher_raw::support_held(f,ownership,weaponhand::clock_ns(nullptr))&&
        ownership.left_hand.id==step.current.support_token;
}
void trace(hand::Frame const& f,Step const& step,bool source_closed,unsigned rejection) noexcept {
    static unsigned rows{};if(!step.available||rows>=8||!log_file)return;++rows;
    fwprintf(log_file,L"gnasher_producer_motion frame=%llu closed=%u rejection=%u count=%u publisher=%llu publisher_start=%llu hold=%llu support=%llu reference=%llu stream=%llu tracking0=%llu tracking1=%llu\n",
        f.key.frame,unsigned(source_closed),rejection,step.motion.lever.count,step.current.publisher.pid,step.current.publisher.start,
        step.current.hold_generation,step.current.support_token,step.current.reference_generation,
        step.current.raw.stream_epoch,step.current.raw.tracking_epoch[0],step.current.raw.tracking_epoch[1]);
    for(unsigned i=0;i<step.motion.lever.count;++i){auto const& p=step.motion.lever.points[i];
        fwprintf(log_file,L"gnasher_producer_motion_point frame=%llu index=%u sequence=%llu sync_ns=%lld angle=%.12g\n",
            f.key.frame,i,p.sequence,p.monotonic_ns,p.lever_angle_radians);}
    fflush(log_file);
}
void consumed(Step const& step,weapon::GnasherMechanicalInteraction const& mechanism) noexcept {
    known=step.captured&&mechanism.source_matches(step.current.raw.input.sequence,step.current.raw.input.sync_ns);
    previous=known?step.current:mc::Context{};
}
}
