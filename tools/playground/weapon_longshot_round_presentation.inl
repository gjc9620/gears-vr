// Included in longshot_round after the original current physical_pose reader.
namespace presentation=gears_weapon_longshot_round_presentation;
presentation::State presented_round;
std::uint64_t presentation_cycle{},last_presented_frame{};
bool presentation_visible{};
void reset_presentation() noexcept {
    presented_round.reset();presentation_cycle=last_presented_frame=0;presentation_visible=false;
}
bool presentation_sample(hand::Frame const& f,hand::Decision const& d,presentation::Sample& out,
                         bool require_visible=true) noexcept {
    if(!presentation_cycle||(require_visible&&!presentation_visible)||!current(f)||!granted(f,d)||
       !hand::same(held_lease,d.lease)||!presented_round.matches(f,d.lease,item_serial,generation,presentation_cycle))return false;
    // Only POD copies follow this live check. Display submission, the actual
    // bone read/inventory writer and Native pose publication recheck authority.
    out=presented_round.sample();return true;
}
bool prepare_presentation(hand::Frame const& f,hand::Decision const& d,gears_vr::Pose const& root,
    gears_vr::Vector port,double scale,std::uint64_t cycle) noexcept {
    gears_vr::Pose raw{};
    if(!cycle||!physical_pose(f,d,raw)||!presented_round.prepare(f,d.lease,item_serial,generation,cycle,
       raw,item_in_grip,root,port,scale,nativelancerfollow::bound_units)){
        reset_presentation();
        if(generation&&asset::find(handle))asset::set_visible(handle,generation,false);
        return false;
    }
    presentation_cycle=cycle;presentation_visible=false;return true;
}
bool presentation_grip(hand::Frame const& f,hand::Decision const& d,gears_vr::Pose& grip) noexcept {
    presentation::Sample sample{};if(!presentation_sample(f,d,sample))return false;
    grip=sample.grip;return true;
}
