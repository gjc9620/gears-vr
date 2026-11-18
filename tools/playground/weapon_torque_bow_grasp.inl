// Optional V4 cosmetic publisher. Native's existing sole C0 consumer repeats
// full Broker/current-model/reference/held checks. This owns no pose or action.
namespace torquebow_grasp {
namespace hand=gears_weapon_hand;namespace authored=gears_torque_bow_grasp_pose;
namespace api=gears_hand_authored_grasp;namespace artist=gears_torque_bow_artist_grasp;
bool publish(hand::Frame const& f,authored::Part part,gears_vr::Pose const& grip,bool proximity) noexcept {
    if(!weaponhand::current_gt(nullptr)||!weaponhand::native_module||!torquebow_input::backend.geometry_current(f))return false;
    const auto address=GetProcAddress(weaponhand::native_module,api::publish_export);
    if(!address||!weaponhand::executable(address,weaponhand::native_module))return false;
    api::Request request{};const auto& reference=part==authored::Part::main_grip?artist::reference_right:artist::reference_left;
    if(!authored::request(part,gears_hand_authored_mount::marcus115_signature,reference,f,grip,proximity,request)||
       !nativeequipment::current(nullptr,f))return false;
    const bool accepted=reinterpret_cast<api::Publish>(address)(&request);
    static std::array<unsigned,4> rows{},last{{~0u,~0u,~0u,~0u}};
    const auto index=unsigned(part),value=unsigned(accepted)*2+unsigned(proximity);
    if(log_file&&rows[index]<8&&last[index]!=value){++rows[index];last[index]=value;
        fwprintf(log_file,L"torquebow_authored_skin part=%u profile=%llu accepted=%u proximity=%u frame=%llu sequence=%llu token=%llu cosmetic_only=1 minimum_artist=1 surface_fit_accepted=0\n",
            index,request.profile_id,unsigned(accepted),unsigned(proximity),f.key.frame,f.controllers.sequence,f.press_token);fflush(log_file);
    }
    return accepted;
}
void main_hand(hand::Frame const& f) noexcept {publish(f,authored::Part::main_grip,{},false);}
bool left_part(hand::Frame const& f,authored::Part part,gears_vr::Pose const& grip,bool proximity) noexcept {
    if(part!=authored::Part::arrow_shaft&&part!=authored::Part::arrow_feed)return false;
    return publish(f,part,grip,proximity);
}
void support_hand(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& decision) noexcept {
    const auto& backend=torquebow_input::backend;
    if(!backend.geometry_current(f)||!backend.support_known||!game.known||!game.main_hand_held||game.busy||
       game.animation_owns_hands||!game.mount_known||!f.left_tracked||!f.left_grip_known||hand::index_pinch(f)||
       !hand::same(f.key,decision.key)||decision.release_required||game.request_ammo||game.request_magazine)return;
    for(auto const& lease:game.object_incumbents)if(lease.kind!=hand::LeaseKind::none)return;
    bool proximity=false;
    if(f.left_grip_down){
        if(decision.press_token!=f.press_token||decision.lease.kind!=hand::LeaseKind::support||decision.lease.id!=f.press_token||
           !hand::lease_valid(decision.lease,f.key.lifecycle)||
           (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
           !hand::support_contact(f,game,true))return;
    }else{
        if(decision.lease.kind!=hand::LeaseKind::none||!hand::support_contact(f,game,false))return;
        proximity=true;
    }
    publish(f,authored::Part::support,backend.support_grip,proximity);
}
}
