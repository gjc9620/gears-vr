// Uses the existing producer ring. Never creates a Frame, edge or grant.
namespace feed_history {
namespace input=gears_hand_continuity;
namespace relative=gears_controller_relative_motion;
using Context=latch_history::Context;
Context previous{},sampled{};
input::MotionHistory history{};
gears_weapon::FeedSample prior{};
unsigned gate{},point_count{};
bool project_pose(input::TrackedInput const& raw,Context const& c,P& out) noexcept {
    V position{};Q rotation{};
    if(!relative::project(raw,c.mount,c.size,position))return false;
    const auto q=gears_vr::multiply(gears_vr::inverse(raw.input.hands[1].grip.orientation),raw.input.hands[0].grip.orientation);
    if(!gears_vr::rig_tracking::detail::quaternion(gears_vr::xr_to_ue(gears_vr::rotation_matrix(q)),rotation))return false;
    out={position,gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(c.mount.orientation),rotation))};
    return gears_vr::stereo_pose_valid(out);
}
void capture(hand::Frame const& f,P const& left) noexcept {
    sampled={};history={};Context c{};gate=1;point_count=0;
    if(!nativeequipment::read_input_continuity(f,c.raw,c.publisher,nullptr,nullptr,&history)||
       !input::same_input(f.controllers,c.raw.input)||!c.publisher.pid||!c.publisher.start)return;
    gate=2;const auto& right=f.controllers.hands[1].grip;
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
    gate=3;P projected{};
    if(!project_pose(c.raw,c,projected)||!gears_hammerburst_feed_history::close_pose(projected,left))return;
    c.frame=f;c.units=units;c.valid=true;sampled=c;gate=0;
}
bool advance(hand::Frame const& f,gears_weapon::FeedSample const& current,gears_weapon::FeedResult& result) noexcept {
    gate=4;
    if(stage!=Stage::new_magazine||feed_item!=current.item||feed_sequence!=prior.sequence||feed_ns!=prior.monotonic_ns||
       !latch_history::same_context(previous,sampled))return false;
    unsigned control{};
    if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))control=gears_vr::control_squeeze;
    else if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))control=gears_vr::control_trigger;
    else return false;
    gate=5;relative::Segment segment{};
    if(!relative::build(previous.raw,sampled.raw,history,control,sampled.mount,sampled.size,segment)||segment.count<3)return false;
    std::array<gears_weapon::FeedSample,9> points{};points[0]=prior;
    gate=6;
    for(unsigned i=1;i<segment.count;++i){const auto sequence=segment.points[i].sequence;
        input::TrackedInput const* raw=nullptr;
        for(unsigned n=0;n<history.count;++n)if(history.frames[n].input.sequence==sequence)raw=&history.frames[n];
        P left{};if(!raw||!project_pose(*raw,sampled,left))return false;
        points[i]=current;points[i].sequence=sequence;points[i].monotonic_ns=raw->input.sync_ns;points[i].left_grip=left;
    }
    gate=7;
    if(!gears_hammerburst_feed_history::same_sample(points[segment.count-1],current))return false;
    points[segment.count-1]=current;
    if(!gears_hammerburst_feed_history::advance(insertion,prior,current,points.data(),segment.count,feed,result))return false;
    point_count=segment.count;gate=0;return true;
}
bool advance_extraction(hand::Frame const& f,gears_weapon::FeedSample const& current) noexcept {
    gate=4;point_count=0;
    if(stage!=Stage::old_magazine||prior.item!=current.item||
       !latch_history::same_context(previous,sampled))return false;
    unsigned control{};
    if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))control=gears_vr::control_squeeze;
    else if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))control=gears_vr::control_trigger;
    else return false;
    gate=5;relative::Segment segment{};
    if(!relative::build(previous.raw,sampled.raw,history,control,sampled.mount,sampled.size,segment)||segment.count<3)return false;
    std::array<gears_weapon::FeedSample,9> points{};points[0]=prior;gate=6;
    for(unsigned i=1;i<segment.count;++i){const auto sequence=segment.points[i].sequence;
        input::TrackedInput const* raw=nullptr;
        for(unsigned n=0;n<history.count;++n)if(history.frames[n].input.sequence==sequence)raw=&history.frames[n];
        P left{};if(!raw||!project_pose(*raw,sampled,left))return false;
        points[i]=current;points[i].sequence=sequence;points[i].monotonic_ns=raw->input.sync_ns;points[i].left_grip=left;
    }
    gate=7;if(!gears_hammerburst_feed_history::same_sample(points[segment.count-1],current))return false;
    points[segment.count-1]=current;
    if(!gears_hammerburst_extraction_history::advance(extraction,prior,current,points.data(),segment.count,feed))return false;
    point_count=segment.count;gate=0;return true;
}
void remember(gears_weapon::FeedSample const& sample) noexcept {previous=sampled;prior=sample;}
void clear() noexcept {previous={};sampled={};prior={};history={};}
}
