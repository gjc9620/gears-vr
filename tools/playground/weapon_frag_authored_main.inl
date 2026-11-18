#pragma once
#include "../vr-common/weapon_frag_authored_request.h"
// Existing GT/hand owner only. Cosmetic V4 publication never grants a hold,
// produces a projectile, changes controllers or writes a skeleton.
namespace frag_authored_main {
namespace api=::gears_hand_authored_grasp;namespace hand=::gears_weapon_hand;
unsigned rows{};bool known{},last{};
void publish(hand::Frame const& frame) noexcept {
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_frag||
       !hand::same(frame.weapon,nativelancerfollow::bound_key.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(frame.world.address,frame.pawn.address,frame.weapon.address))return;
    const auto address=weaponhand::native_module?GetProcAddress(weaponhand::native_module,api::publish_export):nullptr;
    if(!address||!weaponhand::executable(address,weaponhand::native_module))return;
    api::Request request{};
    if(!gears_weapon_frag_authored::request(frame,gears_hand_authored_mount::marcus115_signature,request))return;
    const bool accepted=reinterpret_cast<api::Publish>(address)(&request);
    if(log_file&&rows<8&&(!known||accepted!=last)){
        ++rows;fwprintf(log_file,L"frag_authored_main accepted=%u profile=%llu frame=%llu sequence=%llu sync_ns=%lld finger_only=1\n",
            unsigned(accepted),request.profile_id,frame.key.frame,frame.controllers.sequence,frame.controllers.sync_ns);fflush(log_file);
    }
    known=true;last=accepted;
}
}
