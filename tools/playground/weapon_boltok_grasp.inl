// Included inside boltok_runtime after its existing guarded Native resolver.
// This is a cosmetic publisher only; the common Native module owns all C0
// writes and repeats broker/current-rig/final-held admission independently.
namespace main_skin {
namespace authored=gears_hand_authored_grasp;
unsigned rows{};bool last_known{},last_accepted{};
void publish(hand::Frame const& f) noexcept {
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_boltok||
       !hand::same(f.weapon,nativelancerfollow::bound_key.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return;
    const auto call=function<authored::Publish>(authored::publish_export);if(!call)return;
    authored::Request request{};
    if(!gears_boltok_grasp_pose::main_request(f,request))return;
    const bool accepted=call(&request);
    if(log_file&&rows<16&&(!last_known||last_accepted!=accepted)){
        ++rows;fwprintf(log_file,L"boltok_main_skin accepted=%u profile=%llu frame=%llu sequence=%llu sync_ns=%lld side=1 finger_only=1\n",
            unsigned(accepted),request.profile_id,f.key.frame,f.controllers.sequence,f.controllers.sync_ns);fflush(log_file);
    }
    last_known=true;last_accepted=accepted;
}
void publish_without_mechanism(uptr pc) noexcept {
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_boltok)return;
    hand::Frame f{};hand::GripGameState game{};hand::Decision decision{};
    if(weaponhand::client.copy_frame_on_gt(f,game,decision)&&f.controller.address==pc)publish(f);
}
}
