// Included after the HB gun-local transform helpers. No history point can
// authorize an action: the caller retains its existing current-Frame grant.
namespace latch_history {
namespace input=gears_hand_continuity;
namespace relative=gears_controller_relative_motion;
struct Context {
    hand::Frame frame{};input::TrackedInput raw{};input::Publisher publisher{};
    gears_weapon::HammerburstLatchSample sample{};P mount{};double size{},units{};bool valid{};unsigned capture_gate{1};
};
Context previous{},sampled{};
unsigned gate{};
bool close(V a,V b,double epsilon) noexcept {
    return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)<=epsilon;
}
void capture(hand::Frame const& f,gears_weapon::HammerburstLatchSample const& sample) noexcept {
    sampled={};Context c{};gate=1;
    if(!nativeequipment::read_input_continuity(f,c.raw,c.publisher)||
       !input::same_input(f.controllers,c.raw.input)||!c.publisher.pid||!c.publisher.start)return;
    sampled.capture_gate=2;gate=2;const auto& right=f.controllers.hands[1].grip;
    const double units=nativelancerfollow::bound_units;
    if(!std::isfinite(units)||units<10||units>500)return;
    const auto basis=gears_vr::xr_to_ue(gears_vr::rotation_matrix(right.orientation));
    const auto position=scale(gears_vr::xr_to_ue(right.position),units);
    const gears_vr::Viewpoint r{position,gears_vr::ue_angles(basis)};
    std::array<float,16> raw{};P gun{};Q q{};
    if(!nativelancerfollow::mount.model_matrix(r,raw)||!matrix(raw,gun,c.size)||
       !gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(r.rotation),q))return;
    const auto inverse=gears_vr::inverse(q);
    c.mount={scale(gears_weapon::rotate(inverse,sub(gun.position,position)),1/units),
        gears_vr::normalized(gears_vr::multiply(inverse,gun.orientation))};
    sampled.capture_gate=3;gate=3;V projected{};
    if(!relative::project(c.raw,c.mount,c.size,projected)||!close(projected,sample.hand,1e-5))return;
    c.frame=f;c.sample=sample;c.units=units;c.valid=true;c.capture_gate=0;sampled=c;gate=0;
}
bool same_context(Context const& a,Context const& b) noexcept {
    auto const& x=a.frame;auto const& y=b.frame;
    return a.valid&&b.valid&&x.key.game_pid==y.key.game_pid&&x.key.game_start==y.key.game_start&&
        x.key.native_started_qpc==y.key.native_started_qpc&&x.key.registration_cookie==y.key.registration_cookie&&
        x.key.lifecycle==y.key.lifecycle&&x.key.world_generation==y.key.world_generation&&x.key.thread_id==y.key.thread_id&&
        x.flags==y.flags&&x.reference_generation==y.reference_generation&&x.press_token==y.press_token&&
        x.interaction_control==y.interaction_control&&hand::same(x.world,y.world)&&hand::same(x.controller,y.controller)&&
        hand::same(x.pawn,y.pawn)&&hand::same(x.mesh,y.mesh)&&hand::same(x.weapon,y.weapon)&&
        a.publisher.pid==b.publisher.pid&&a.publisher.start==b.publisher.start&&
        a.units==b.units&&std::abs(a.size-b.size)<1e-6&&close(a.mount.position,b.mount.position,1e-6)&&
        std::abs(a.mount.orientation.x-b.mount.orientation.x)<1e-6&&
        std::abs(a.mount.orientation.y-b.mount.orientation.y)<1e-6&&
        std::abs(a.mount.orientation.z-b.mount.orientation.z)<1e-6&&
        std::abs(a.mount.orientation.w-b.mount.orientation.w)<1e-6;
}
bool advance(hand::Frame const& f,gears_weapon::HammerburstLatchSample const& current,
    bool retained,gears_weapon::HammerburstLatchMotion& result) noexcept {
    gate=4;if(!retained||!same_context(previous,sampled))return false;
    gate=5;input::TrackedInput actual{};input::Publisher publisher{};input::MotionHistory history{};
    if(!nativeequipment::read_input_continuity(f,actual,publisher,nullptr,nullptr,&history)||
       publisher.pid!=sampled.publisher.pid||publisher.start!=sampled.publisher.start||
       !input::same_tracking(actual,sampled.raw))return false;
    unsigned control{};
    if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))control=gears_vr::control_squeeze;
    else if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))control=gears_vr::control_trigger;
    else return false;
    gate=6;relative::Segment segment{};
    const bool built=relative::build(previous.raw,sampled.raw,history,control,sampled.mount,sampled.size,segment);
    if(!built||segment.count<3){
        const auto before_record=hb_history_rejection::records;
        hb_history_rejection::capture(log_file,f.key.frame,f.controllers.sequence,control,built,segment.count,
            previous.raw,sampled.raw,history,sampled.mount,sampled.size);
        if(hb_history_rejection::records>before_record){
            const auto origin=nativeequipment::input_continuity_reader.motion_observation();
            fwprintf(log_file,L"hammerburst_history_origin record=%u latest=%llu selected=%llu stopped=%llu stored=%llu read_ns=%lld reason=%u action=0\n",
                hb_history_rejection::records,origin.latest,origin.selected,origin.stopped,origin.stored,origin.read_ns,origin.reason);fflush(log_file);
        }
        return false;
    }
    std::array<gears_weapon::HammerburstLatchSample,9> points{};
    points[0]=previous.sample;
    for(unsigned i=1;i<segment.count;++i){auto const& p=segment.points[i];
        points[i]={p.position,p.sequence,std::uint64_t(p.sync_ns/1000000),current.grip_token,true,true};
    }
    // Projection must match the CURRENT real measurement before replacing its
    // numerically equivalent final point. No endpoint/time is manufactured.
    gate=7;if(!close(points[segment.count-1].hand,current.hand,1e-5)||
       points[segment.count-1].sequence!=current.sequence||points[segment.count-1].milliseconds!=current.milliseconds)return false;
    points[segment.count-1]=current;
    result=gears_weapon::hammerburst_latch_history(stroke,current,points,segment.count);gate=8;
    return true;
}
}
