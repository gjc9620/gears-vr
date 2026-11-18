// Included in longshot_bolt. Motion facts come from the existing authenticated
// producer ring. No old point becomes a Frame, lease, seek or bone receipt.
namespace history {
namespace source=gears_hand_continuity;namespace relative=gears_controller_relative_motion;
struct Context {
    hand::Frame frame{};source::TrackedInput raw{};source::Publisher publisher{};
    gears_rig_tracking::ObservedContinuity observed{};
    gears_rig_tracking::NativeReference reference{};
    P part_in_right_m{};double scale{},units{};bool valid{},producer_available{},root_static{};
    unsigned capture_gate{~0u};
};
Context sampled{},previous{};
struct Receipt {unsigned gate{},transport_failure{};bool attempted{};source::MotionHistory window{};};
bool close_position(V a,V b,double tolerance) noexcept {
    return gears_vr::finite(a)&&gears_vr::finite(b)&&
        std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)<=tolerance;
}
bool fixed_root(tracks::Samples const& samples) noexcept {
    if(samples.count<3||samples.count>samples.bones[0].size()||!std::isfinite(samples.length)||
       samples.length<=0||samples.length>10||!gears_vr::stereo_pose_valid(samples.bones[0][0]))return false;
    for(unsigned i=1;i<samples.count;++i)
        if(!gears_vr::stereo_same_pose(samples.bones[0][0],samples.bones[0][i]))return false;
    return true;
}
bool capture(hand::Frame const& f,gears_rig_tracking::Frame const& rig,P const& root,
    double scale,double units,V measured,tracks::Samples const& samples,Context& out) noexcept {
    namespace vr=gears_vr;
    out={};out.capture_gate=1;
    out.producer_available=(rig.flags&gears_rig_tracking::producer_continuity_available)!=0;
    out.root_static=fixed_root(samples);
    const auto mask=gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose;
    if(!out.producer_available||(rig.world_pose_mask&mask)!=mask||!out.root_static)return false;
    out.capture_gate=2;
    if(rig.native.game_pid!=f.key.game_pid||rig.native.game_start!=f.key.game_start||
       rig.native.native_started_qpc!=f.key.native_started_qpc||rig.native.registration_cookie!=f.key.registration_cookie||
       rig.native.game_thread!=f.key.thread_id||rig.world_generation!=f.key.world_generation||
       rig.reference_generation!=f.reference_generation||
       !hand::same(rig.world,f.world)||!hand::same(rig.controller,f.controller)||!hand::same(rig.pawn,f.pawn)||
       !hand::same(rig.mesh,f.mesh)||!hand::same(rig.weapon,f.weapon)||!source::same_input(f.controllers,rig.controllers))return false;
    out.capture_gate=3;
    if(!rig.controller_publisher_pid||!rig.controller_publisher_start||
       !rig.producer.stream||!rig.producer.tracking[0]||!rig.producer.tracking[1]||
       rig.native_reference.flags!=gears_rig_tracking::reference_stable||!rig.native_reference.epoch||
       !rig.observed.stream||!rig.observed.reference||!rig.observed.head_tracking||
       !rig.observed.hand_tracking[0]||!rig.observed.hand_tracking[1]||
       !std::isfinite(units)||units<10||units>500||rig.units_per_metre!=units||!std::isfinite(scale)||scale<=.1||scale>=10||
       !vr::stereo_pose_valid(root)||!vr::stereo_pose_valid(rig.world_grip[1]))return false;
    Context value{};value.frame=f;value.raw={rig.controllers,rig.producer.stream,
        {rig.producer.tracking[0],rig.producer.tracking[1]}};
    value.publisher={rig.controller_publisher_pid,rig.controller_publisher_start};
    value.observed=rig.observed;value.reference=rig.native_reference;value.scale=scale;value.units=units;
    value.part_in_right_m.position=local(rig.world_grip[1],root.position,units);
    value.part_in_right_m.orientation=vr::normalized(vr::multiply(vr::inverse(rig.world_grip[1].orientation),root.orientation));
    value.producer_available=value.root_static=true;value.capture_gate=4;out=value;
    V projected{};
    if(!relative::project(value.raw,value.part_in_right_m,scale,projected)||!close_position(projected,measured,.00025))return false;
    value.valid=true;value.capture_gate=0;out=value;return true;
}
bool same_context(Context const& a,Context const& b) noexcept {
    const auto& x=a.frame;const auto& y=b.frame;
    return a.valid&&b.valid&&x.key.game_pid==y.key.game_pid&&x.key.game_start==y.key.game_start&&
        x.key.native_started_qpc==y.key.native_started_qpc&&x.key.registration_cookie==y.key.registration_cookie&&
        x.key.lifecycle==y.key.lifecycle&&x.key.world_generation==y.key.world_generation&&x.key.thread_id==y.key.thread_id&&
        x.flags==y.flags&&x.reference_generation==y.reference_generation&&x.interaction_control==y.interaction_control&&
        hand::same(x.world,y.world)&&hand::same(x.controller,y.controller)&&hand::same(x.pawn,y.pawn)&&
        hand::same(x.mesh,y.mesh)&&hand::same(x.weapon,y.weapon)&&
        a.publisher.pid==b.publisher.pid&&a.publisher.start==b.publisher.start&&a.units==b.units&&
        a.reference.epoch==b.reference.epoch&&a.reference.flags==b.reference.flags&&
        a.observed.stream==b.observed.stream&&a.observed.reference==b.observed.reference&&
        a.observed.head_tracking==b.observed.head_tracking&&
        a.observed.hand_tracking[0]==b.observed.hand_tracking[0]&&a.observed.hand_tracking[1]==b.observed.hand_tracking[1];
}
template<class Read>
bool select(Context const& a,Context const& b,Read read,relative::Segment& out,Receipt& receipt) noexcept {
    receipt.attempted=true;receipt.gate=1;
    if(!a.valid||!b.valid)return false;
    receipt.gate=2;if(!same_context(a,b))return false;
    receipt.gate=3;
    if(std::abs(a.scale-b.scale)>.000001||!close_position(a.part_in_right_m.position,b.part_in_right_m.position,.0001)||
       angle(a.part_in_right_m.orientation,b.part_in_right_m.orientation)>.02)return false;
    source::TrackedInput actual{};source::Publisher publisher{};
    receipt.gate=4;if(!read(actual,publisher,receipt.window))return false;
    receipt.gate=5;if(publisher.pid!=b.publisher.pid||publisher.start!=b.publisher.start)return false;
    receipt.gate=6;if(!source::same_tracking(actual,b.raw))return false;
    unsigned control{};
    if(b.frame.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))control=gears_vr::control_squeeze;
    else if(b.frame.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))control=gears_vr::control_trigger;
    else return false;
    receipt.gate=7;
    if(!relative::build(a.raw,b.raw,receipt.window,control,b.part_in_right_m,b.scale,out))return false;
    receipt.gate=9;return true;
}
void report(hand::Frame const& f,Context const& a,Context const& b,Receipt const& receipt,
    relative::Segment const& segment,tracks::BoltResult result) noexcept {
    static unsigned rows{},points{};static std::array<std::uint64_t,3> previous_report{};
    const std::array<std::uint64_t,3> key{generation,f.press_token,receipt.gate};
    if(!log_file||!receipt.attempted||rows>=12||key==previous_report)return;
    previous_report=key;++rows;
    if(receipt.gate==3){
        const auto ap=a.part_in_right_m.position,bp=b.part_in_right_m.position;
        const double position_difference=std::hypot(std::hypot(ap.x-bp.x,ap.y-bp.y),ap.z-bp.z);
        const double rotation_difference=angle(a.part_in_right_m.orientation,b.part_in_right_m.orientation);
        fwprintf(log_file,L"longshot_bolt_history_pair frame=%llu serial=%llu scale_before=%.17g scale_current=%.17g scale_difference=%.17g position_difference_m=%.17g rotation_difference_degrees=%.17g scale_ok=%u position_ok=%u rotation_ok=%u before_root=%.17g,%.17g,%.17g current_root=%.17g,%.17g,%.17g existing_context_only=1 decision_already_made=1\n",
            f.key.frame,generation,a.scale,b.scale,std::abs(a.scale-b.scale),position_difference,rotation_difference,
            unsigned(std::abs(a.scale-b.scale)<=.000001),unsigned(close_position(ap,bp,.0001)),unsigned(rotation_difference<=.02),
            ap.x,ap.y,ap.z,bp.x,bp.y,bp.z);
    }
    // One complete failed build per process, after the action decision. Keep
    // the original ring count/rows, including a truncated ring, for exact
    // offline replay through the unchanged production relative::build.
    static bool raw_reported{};
    if(receipt.gate==7&&!raw_reported){
        raw_reported=true;gears_longshot_history_evidence::Record record{};
        record.game_pid=f.key.game_pid;record.game_start=f.key.game_start;record.frame=f.key.frame;
        record.serial=generation;record.token=f.press_token;
        record.control=b.frame.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip)?
            gears_vr::control_squeeze:gears_vr::control_trigger;
        record.previous=a.raw;record.current=b.raw;record.window=receipt.window;
        record.fixed_part_in_right_m=b.part_in_right_m;record.scale=b.scale;
        const bool written=gears_longshot_history_evidence::write(log_file,record);
        fwprintf(log_file,L"longshot_bolt_history_raw_end serial=%llu frame=%llu complete=%u motion_only=1 action_already_decided=1\n",
            generation,f.key.frame,unsigned(written));
    }
    const auto p=b.part_in_right_m.position;const auto q=b.part_in_right_m.orientation;
    fwprintf(log_file,L"longshot_bolt_history frame=%llu serial=%llu token=%llu gate=%u transport_failure=%u result=%u count=%u before_valid=%u current_valid=%u before_capture=%u current_capture=%u producer_available=%u root_static=%u source_before=%llu source_current=%llu stamp_before=%lld stamp_current=%lld stream=%llu tracking=%llu,%llu publisher=%llu,%llu native_reference=%llu root_in_right_m=%.9g,%.9g,%.9g root_rotation=%.9g,%.9g,%.9g,%.9g scale=%.9g current_Frame_only=1 no_history_bone_ack=1\n",
        f.key.frame,generation,f.press_token,receipt.gate,receipt.transport_failure,unsigned(result),segment.count,
        unsigned(a.valid),unsigned(b.valid),a.capture_gate,b.capture_gate,unsigned(b.producer_available),unsigned(b.root_static),
        a.raw.input.sequence,b.raw.input.sequence,a.raw.input.sync_ns,b.raw.input.sync_ns,b.raw.stream_epoch,
        b.raw.tracking_epoch[0],b.raw.tracking_epoch[1],b.publisher.pid,b.publisher.start,b.reference.epoch,
        p.x,p.y,p.z,q.x,q.y,q.z,q.w,b.scale);
    if(receipt.gate==9&&points<27){
        for(unsigned i=0;i<segment.count&&points<27;++i){const auto& point=segment.points[i];
            const source::TrackedInput* raw=point.sequence==a.raw.input.sequence?&a.raw:nullptr;
            for(unsigned n=0;!raw&&n<receipt.window.count;++n)
                if(receipt.window.frames[n].input.sequence==point.sequence)raw=&receipt.window.frames[n];
            if(!raw)break;const auto& l=raw->input.hands[0].grip;const auto& r=raw->input.hands[1].grip;
            fwprintf(log_file,L"longshot_bolt_history_point serial=%llu token=%llu index=%u sequence=%llu source_sync_ns=%lld point=%.9g,%.9g,%.9g left=%.9g,%.9g,%.9g left_q=%.9g,%.9g,%.9g,%.9g right=%.9g,%.9g,%.9g right_q=%.9g,%.9g,%.9g,%.9g original_producer=1 motion_only=1\n",
                generation,f.press_token,i,point.sequence,point.sync_ns,point.position.x,point.position.y,point.position.z,
                l.position.x,l.position.y,l.position.z,l.orientation.x,l.orientation.y,l.orientation.z,l.orientation.w,
                r.position.x,r.position.y,r.position.z,r.orientation.x,r.orientation.y,r.orientation.z,r.orientation.w);++points;
        }
    }
    fflush(log_file);
}
}
