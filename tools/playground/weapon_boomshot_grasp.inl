// Optional cosmetic endpoint for this weapon's six authored profiles. Actual
// contact/lease/pose producers remain separate; Native repeats final Broker,
// source identity and model-reference admission through the sole C0 writer.
namespace boomshot_grasp {
namespace hand=gears_weapon_hand;namespace api=gears_hand_authored_grasp;
namespace authored=gears_boomshot_grasp_pose;
bool publish(hand::Frame const& f,authored::Part part,api::Signature const& actual_geometry,
    api::Joints const& model_reference,gears_vr::Pose const& current_part_grip,bool proximity) noexcept {
    if(!boomshot_carrier::selected(f)||!weaponhand::native_module)return false;
    // Cosmetic fingers use the current already-bound gun. Revalidating the
    // projectile asset on every finger publication adds no hand authority;
    // Native independently checks the current model and sole C0 consumer.
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::mount.bound()||
       !hand::same(f.weapon,nativelancerfollow::bound_key.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation())return false;
    const auto address=GetProcAddress(weaponhand::native_module,api::publish_export);
    if(!address||!weaponhand::executable(address,weaponhand::native_module))return false;
    api::Request request{};
    if(!authored::request(part,actual_geometry,model_reference,f,current_part_grip,proximity,request)||
       !nativeequipment::current(nullptr,f))return false;
    const bool accepted=reinterpret_cast<api::Publish>(address)(&request);
    static std::array<unsigned,6> rows{},last{{~0u,~0u,~0u,~0u,~0u,~0u}};
    const auto index=unsigned(part);const unsigned state=unsigned(accepted)*2+unsigned(proximity);
    if(log_file&&index<rows.size()&&rows[index]<8&&last[index]!=state){
        ++rows[index];last[index]=state;
        fwprintf(log_file,L"boomshot_authored_skin part=%u profile=%llu accepted=%u proximity=%u frame=%llu sequence=%llu token=%llu cosmetic_only=1\n",
            index,request.profile_id,unsigned(accepted),unsigned(proximity),f.key.frame,f.controllers.sequence,f.press_token);fflush(log_file);
    }
    return accepted;
}
bool publish_marcus115(hand::Frame const& f,authored::Part part,
    gears_vr::Pose const& current_part_grip,bool proximity) noexcept {
    api::Joints reference{};const auto side=part==authored::Part::main_grip?api::Side::right:api::Side::left;
    const auto& expected=gears_hand_authored_mount::marcus115_signature;
    if(!gears_hand_marcus115_reference::read(expected,side,reference))return false;
    // Expected model is not a liveness claim: V4 must independently match it
    // against the authenticated current C0 model before using these locals.
    return publish(f,part,expected,reference,current_part_grip,proximity);
}
}
