// Optional skin publication through the existing Native Broker and sole C0.
// Physical parts/inventory always use the separate unmodified physical wrist.
namespace snub_grasp {
namespace hand=gears_weapon_hand;namespace api=gears_hand_part_grasp;
namespace visual=snub_visual;namespace geometry=gears_weapon_lancer_grasp;
using P=gears_vr::Pose;using V=gears_vr::Vector;using Q=gears_vr::Quaternion;
enum class Part:unsigned {magazine=1,slide=2};
lancerreload_inventory::Identity direction_identity{};std::uint64_t direction_token{},direction_reference{},direction_profile{};
unsigned direction_control{};double magazine_direction{};bool direction_known{};
unsigned report_rows{},report_state{~0u};
void report(hand::Frame const& f,Part part,unsigned gate,bool published) noexcept {
    const unsigned state=unsigned(part)*256+gate*2+unsigned(published);
    if(!log_file||report_rows>=16||state==report_state)return;
    ++report_rows;report_state=state;
    fwprintf(log_file,L"snub_skin part=%u gate=%u published=%u frame=%llu sequence=%llu token=%llu life=%llu input_age_ns=%lld cosmetic_only=1\n",
        unsigned(part),gate,unsigned(published),f.key.frame,f.controllers.sequence,f.press_token,f.key.lifecycle,
        static_cast<long long>(weaponhand::clock_ns(nullptr)-f.controllers.sync_ns));fflush(log_file);
}
bool skin_target(Part part,V contact,V axis,P const& physical_wrist,double model_scale,double units,
    double direction,P& out) noexcept {
    if((part!=Part::magazine&&part!=Part::slide)||!gears_vr::finite(contact)||
       !gears_vr::stereo_pose_valid(physical_wrist)||!std::isfinite(model_scale)||model_scale<=0)return false;
    Q wrist=physical_wrist.orientation;
    if(part==Part::magazine&&!geometry::cylinder_orientation(wrist,axis,direction,wrist))return false;
    // Marcus115 landmarks of the unchanged cylinder / precision profiles.
    // These select skin only; they never replace the part's physical contact.
    const auto palm=part==Part::magazine?geometry::starter_contact_in_wrist:geometry::loading_contact_in_wrist;
    const auto offset=gears_vr::transform(gears_vr::rotation_matrix(wrist),
        {palm.x*model_scale,palm.y*model_scale,palm.z*model_scale});
    return lancer_grasp::grip_for_wrist({{contact.x-offset.x,contact.y-offset.y,contact.z-offset.z},wrist},
        units,model_scale,out);
}
bool publish(hand::Frame const& f,hand::Decision const& d,Part part,V contact,V axis) noexcept {
    const auto call=lancersaw_visual::function<api::Publish>(api::publish_export);
    if(!call)return false;
    unsigned gate=1;bool accepted=false;
    struct Receipt {hand::Frame const& f;Part part;unsigned& gate;bool& accepted;~Receipt(){report(f,part,gate,accepted);}} receipt{f,part,gate,accepted};
    if(!visual::granted(f,d)||!f.left_grip_known||d.press_token!=f.press_token||!visual::eligible(f))return false;
    gears_committed_interaction::Frame actual{};P physical{};
    gate=2;if(!visual::committed_pair(f,actual,visual::PairPurpose::physical_observation)||
        !lancerreload_visual::physical_wrist(actual,physical))return false;
    double direction=1;
    const auto identity=lancerreload_inventory::identity(f);
    if(part==Part::magazine){
        const bool same=direction_known&&direction_identity==identity&&direction_token==f.press_token&&
            direction_reference==f.reference_generation&&direction_profile==f.controllers.profile_generation&&direction_control==f.interaction_control;
        if(!same){
            const auto row=gears_vr::transform(gears_vr::rotation_matrix(physical.orientation),{0,0,1});
            direction=row.x*axis.x+row.y*axis.y+row.z*axis.z<0?-1.:1.;
        }else direction=magazine_direction; // Cosmetic axis sign only, never a saved permission or world pose.
    }
    api::Request q{};q.pose.source=f;
    q.pose.kind=gears_hand_object_grasp::Kind::magazine; // V3 generic held-part skin slot, not Lancer's loading owner.
    q.preset=part==Part::magazine?api::Preset::cylinder:api::Preset::precision;
    gate=3;if(!skin_target(part,contact,axis,physical,actual.model_scale,actual.right.units_per_metre,direction,q.pose.grip))return false;
    gate=4;if(!nativeequipment::current(nullptr,f))return false;
    accepted=call(&q);
    if(accepted&&part==Part::magazine){
        magazine_direction=direction;direction_identity=identity;direction_token=f.press_token;
        direction_reference=f.reference_generation;direction_profile=f.controllers.profile_generation;
        direction_control=f.interaction_control;direction_known=true;
    }
    gate=accepted?0u:5u;return accepted;
}
bool magazine(hand::Frame const& f,hand::Decision const& d,std::uint64_t item) noexcept {
    if(!visual::skin_available()||!item||!visual::held||!visual::ready||!visual::geometry.valid||!visual::current(f))return false;
    const auto handle=visual::handle;const auto generation=visual::generation;
    const auto checked=[&]() noexcept {
        const auto r=weapon_part_assets::find(handle);
        return r&&r->owner.generation==generation&&r->stage==weapon_part_assets::Stage::visible&&!r->visibility_revoke&&
            hand::same(r->pose_lease,d.lease)&&visual::granted(f,d)&&visual::current(f);
    };
    std::array<float,16> raw{};P actual{};double size{};
    if(!weapon_part_assets::read_scene_matrix(f,handle,generation,raw,checked)||
       !hammerburst_runtime::matrix(raw,actual,size)||std::abs(size-1.)>1e-3)return false;
    const auto centre=gears_vr::transform(gears_vr::rotation_matrix(actual.orientation),visual::geometry.centre);
    const auto axis=gears_vr::transform(gears_vr::rotation_matrix(actual.orientation),visual::geometry.outward);
    return publish(f,d,Part::magazine,{actual.position.x+centre.x,actual.position.y+centre.y,actual.position.z+centre.z},axis);
}
bool slide(hand::Frame const& f,hand::Decision const& d,std::uint64_t serial) noexcept {
    namespace charging=snub_charging;
    if(!visual::skin_available()||!serial||!hammerburst_animation::ownership.active||!hammerburst_animation::ownership.stopped||
       hammerburst_animation::ownership.serial!=serial||!charging::contact_known||
       !hand::same(charging::observed_part_key,f.key)||charging::observed_part_sequence!=f.controllers.sequence||
       !(charging::contact_identity==lancerreload_inventory::identity(f))||!visual::granted(f,d))return false;
    const auto axis=gears_vr::transform(gears_vr::rotation_matrix(charging::observed_part.orientation),{0,1,0});
    return publish(f,d,Part::slide,charging::observed_part.position,axis);
}
} // namespace snub_grasp
