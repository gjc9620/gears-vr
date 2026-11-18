// Current real hand lease and cartridge pose only. Resource construction
// remains in weapon_longshot_round.inl; inventory remains with its transaction.
#include "../vr-common/weapon_longshot_round_presentation.h"
#include "../vr-common/hand_authored_grasp.h"
#include "../vr-common/weapon_longshot_round_grasp_profile.h"
namespace longshot_round {
void reset_presentation() noexcept;
bool granted(hand::Frame const& f,hand::Decision const& d) noexcept {
    return hand::same(f.key,d.key)&&d.press_token==f.press_token&&!d.release_required&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        d.lease.kind==hand::LeaseKind::ammo&&hand::lease_valid(d.lease,f.key.lifecycle)&&
        d.lease.id==f.press_token&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
bool hide() noexcept {
    if(!generation){held_lease={};item_serial=last_pose_sequence=0;authored_pinch=false;reset_presentation();return true;}
    const auto resource=asset::find(handle);
    if(!resource||!asset::set_visible(handle,generation,false))return false;
    held_lease={};item_serial=last_pose_sequence=0;authored_pinch=false;reset_presentation();return true;
}
bool capture_shoulder(hand::Frame const& f,hand::Decision const& d,std::uint64_t item) noexcept {
    if(!item||item!=d.lease.id||item_serial||!ready||!current(f)||!granted(f,d)||!d.new_grant)return false;
    const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
    if(gears_weapon::body_slot(nativeequipment::calibration,body,f.controllers.hands[0].grip.position,0)!=
       gears_weapon::BodySlot::shoulder_ammo)return false;
    gears_hand_authored_mount::Mount anatomical{};
    const double units=nativelancerfollow::bound_units;
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
       gears_hand_authored_mount::Side::left,1,units,anatomical))return false;
    // Match the admitted precision profile's pad landmark. This ONE mount
    // supplies both displayed round and physical insertion tip; fitting has
    // not yet been accepted from real game pixels.
    gears_vr::Pose mount{};
    const bool authored=lancersaw_visual::function<gears_hand_authored_grasp::Publish>(gears_hand_authored_grasp::publish_export)!=nullptr;
    const bool mounted=authored?
        gears_weapon_longshot_round_grasp::mount(anatomical.calibration.grip_to_wrist_metres,
            anatomical.calibration.grip_to_wrist_rotation,units,mount):
        gears_weapon_longshot_grasp::cartridge_mount(anatomical.calibration.grip_to_wrist_metres,
            anatomical.calibration.grip_to_wrist_rotation,units,mount);
    if(!mounted)return false;
    if(!gears_vr::stereo_pose_valid(mount)||!current(f))return false;
    reset_presentation();item_in_grip=mount;held_lease=d.lease;item_serial=item;last_pose_sequence=0;authored_pinch=authored;
    // Accepted mount POD for later real-input insertion planning. It is the
    // exact same mount used by rendering and the physical tip below, never
    // an alternate pose or an ammunition/endpoint receipt.
    static unsigned mount_report_rows{};
    if(log_file&&mount_report_rows<16){++mount_report_rows;
        const auto p=mount.position;const auto q=mount.orientation;
        fwprintf(log_file,L"longshot_round_mount item=%llu weapon=%llx frame=%llu source_sync_ns=%lld units=%.9g position=%.9g,%.9g,%.9g rotation=%.9g,%.9g,%.9g,%.9g authored=%u pose_profile=%llu\n",
            item,static_cast<unsigned long long>(f.weapon.address),f.key.frame,f.controllers.sync_ns,
            units,p.x,p.y,p.z,q.x,q.y,q.z,q.w,unsigned(authored_pinch),
            authored_pinch?gears_weapon_longshot_round_grasp::profile_id:0ull);fflush(log_file);
    }
    return true;
}
bool physical_pose(hand::Frame const& f,hand::Decision const& d,gears_vr::Pose& item) noexcept {
    if(!item_serial||!ready||!current(f)||!granted(f,d)||!hand::same(held_lease,d.lease))return false;
    gears_rig_tracking::Frame rig{};const auto source=static_cast<std::uint32_t>(f.stereo.source);
    if(handrig::read(rig,0,source).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),source)!=gears_equipment_rig::Status::matched||
       !(rig.world_pose_mask&gears_rig_tracking::left_grip_pose))return false;
    const auto& left=rig.world_grip[0];const auto offset=gears_weapon::rotate(left.orientation,item_in_grip.position);
    item={{left.position.x+offset.x,left.position.y+offset.y,left.position.z+offset.z},
        gears_vr::normalized(gears_vr::multiply(left.orientation,item_in_grip.orientation))};
    return gears_vr::stereo_pose_valid(item)&&current(f);
}
#include "weapon_longshot_round_presentation.inl"
bool show(hand::Frame const& f,hand::Decision const& d) noexcept {
    gears_vr::Pose item{};presentation::Sample sample{};
    const bool prepared=presentation_cycle!=0;
    const bool available=prepared?presentation_sample(f,d,sample,false):physical_pose(f,d,item);
    if(prepared&&available)item=sample.display;
    if(!available){
        // Withdraw visible geometry on a rejected current source, without
        // discarding the inventory owner's still-owed reservation/cleanup.
        if(generation&&asset::find(handle))asset::set_visible(handle,generation,false);
        last_pose_sequence=0;reset_presentation();return false;
    }
    if(f.controllers.sequence==last_pose_sequence&&f.key.frame==last_presented_frame){presentation_visible=prepared;return true;}
    if(f.controllers.sequence<last_pose_sequence||f.key.frame<last_presented_frame){reset_presentation();return false;}
    if(!asset::set_pose(f,handle,generation,d,{item.position,gears_vr::ue_angles(gears_vr::rotation_matrix(item.orientation))},true)){
        if(generation&&asset::find(handle))asset::set_visible(handle,generation,false);
        reset_presentation();return false;
    }
    last_pose_sequence=f.controllers.sequence;last_presented_frame=f.key.frame;presentation_visible=prepared;return true;
}
}
