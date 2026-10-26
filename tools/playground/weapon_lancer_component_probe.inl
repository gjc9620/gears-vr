// Bounded read-only component observations on ordinary PostPlayerTick.
// No engine getter, Native call, bridge read, writer or shot permission.
#include "weapon_component_sample_budget.h"
#include "weapon_component_matrix_read.inl"
#include "weapon_component_pose_read.inl"
namespace nativeequipment_component {
sampling::Budget budget;ULONGLONG next_sample{};
void print_matrix(wchar_t const* label,Matrix const& m) noexcept {
    fwprintf(log_file,L" %ls=",label);for(unsigned i=0;i<m.size();++i)fwprintf(log_file,L"%ls%.9g",i?L",":L"",m[i]);
}
void tick(hand::Frame const& f,hand::GripGameState const& copied_game,std::int64_t copy_begin_ns) noexcept {
    const auto begin=weaponhand::clock_ns(nullptr);
    if(!log_file||!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||!weaponhand::client.armed()||
       !weaponhand::client.registration_alive(f.key.registration_cookie,f.key.native_started_qpc)||
       !hand::frame_active(f,begin)||!copied_game.known||!copied_game.main_hand_held||copied_game.busy||
       copied_game.animation_owns_hands||copied_game.kind!=hand::WeaponKind::firearm||copy_begin_ns<=0||copy_begin_ns>begin)return;
    const auto& right=f.controllers.hands[1];
    if(!gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active))return;
    const auto generation=vrequip::model.hold_generation();
    if(!generation||!vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return;
    const auto ms=GetTickCount64();if(ms<next_sample)return;
    sampling::Identity id{f.key,{f.world,f.controller,f.pawn,f.mesh,f.weapon},generation,f.reference_generation,
        f.controllers.session,f.controllers.space_epoch,f.controllers.profile_generation,f.controllers.input_epoch,unsigned(f.controllers.source)};
    unsigned attempt{};
    const auto& grip=right.grip;if(!budget.claim(id,grip,attempt))return;next_sample=ms+50;
    const wchar_t* reason=L"asset_first";bool accepted=false;
    lancermain_asset::Evidence first{},last{};Matrix gun{},gun_after{},avatar{},avatar_after{};
    vrequip::MeshPoseProbe pose{},pose_after{};
    cohort::Snapshot bones_before{},bones_after{};bool bones_before_valid=false,bones_after_valid=false;
    const auto sample=[&]() {
        if(lancermain_asset::read(f,first)!=lancermain_asset::Status::ready)return false;
        reason=L"avatar_matrix";if(!matrix(f.mesh,L"LocalToWorld",avatar))return false;
        bones_before_valid=pose_cohort(f.mesh,avatar,bones_before);
        reason=L"gun_matrix";if(!matrix(first.component,L"LocalToWorld",gun))return false;
        reason=L"mesh_probe";if(!vrequip::mesh_pose_probe(f.weapon.address,pose)||pose.mesh!=first.component.address||
           pose.asset!=first.asset.address||pose.hidden||std::memcmp(pose.local_to_world,gun.data(),64))return false;
        reason=L"attachment";if(!nativeequipment::read_attached_frame(f,first.component.address))return false;
        reason=L"asset_final";if(lancermain_asset::read(f,last)!=lancermain_asset::Status::ready||
           !hand::same(first.component,last.component)||!hand::same(first.asset,last.asset))return false;
        reason=L"matrix_changed";if(!matrix(last.component,L"LocalToWorld",gun_after)||gun_after!=gun||
           !matrix(f.mesh,L"LocalToWorld",avatar_after)||avatar_after!=avatar||
           !vrequip::mesh_pose_probe(f.weapon.address,pose_after)||pose_after.mesh!=pose.mesh||pose_after.asset!=pose.asset||
           pose_after.hidden||pose_after.absolute_translation!=pose.absolute_translation||
           pose_after.absolute_rotation!=pose.absolute_rotation||pose_after.absolute_scale!=pose.absolute_scale||
           std::memcmp(&pose_after.translation,&pose.translation,sizeof(pose.translation))||
           std::memcmp(&pose_after.rotation,&pose.rotation,sizeof(pose.rotation))||std::memcmp(pose_after.local_to_world,gun.data(),64))return false;
        bones_after_valid=pose_cohort(f.mesh,avatar_after,bones_after);
        reason=L"game_or_generation";hand::GripGameState final_game{};
        if(!weaponhand::client.source_game_on_gt(f.key,final_game)||!hand::same_game(copied_game,final_game)||
           vrequip::model.hold_generation()!=generation||!nativeequipment::domain_ready()||!weaponhand::client.armed())return false;
        reason=L"source_expired";if(!hand::frame_active(f,weaponhand::clock_ns(nullptr)))return false;
        reason=L"observed";return true;
    };
    try{accepted=sample();}catch(...){reason=L"cpp_exception";}
    const auto end=weaponhand::clock_ns(nullptr);
    if(accepted&&(end<begin||!hand::frame_active(f,end)||!weaponhand::client.registration_alive(f.key.registration_cookie,f.key.native_started_qpc)))
        {accepted=false;reason=L"source_expired_at_record";}
    fwprintf(log_file,L"lancer_component_observation accepted=%u reason=%ls attempt=%u pid=%u thread=%u game_start=%llu native_started=%llu cookie=%llu lifecycle=%llu frame=%llu world_generation=%llu hold_generation=%llu source_sequence=%llu source_sync_ns=%lld copy_begin_ns=%lld begin_ns=%lld end_ns=%lld component=%llx asset=%llx raw_right=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g absolute=%u,%u,%u hidden=%u",
        unsigned(accepted),reason,attempt,f.key.game_pid,f.key.thread_id,f.key.game_start,f.key.native_started_qpc,f.key.registration_cookie,
        f.key.lifecycle,f.key.frame,f.key.world_generation,generation,f.controllers.sequence,f.controllers.sync_ns,copy_begin_ns,begin,end,
        first.component.address,first.asset.address,grip.position.x,grip.position.y,grip.position.z,grip.orientation.x,grip.orientation.y,grip.orientation.z,grip.orientation.w,
        unsigned(pose.absolute_translation),unsigned(pose.absolute_rotation),unsigned(pose.absolute_scale),unsigned(pose.hidden));
    fwprintf(log_file,L" reference_generation=%llu input_session=%llu space_epoch=%llu profile_generation=%llu input_epoch=%llu input_source=%u",
        id.reference_generation,id.input_session,id.space_epoch,id.profile_generation,id.input_epoch,id.source);
    fwprintf(log_file,L" right_grip_flags=%llu right_grip_active=%u",right.grip_flags,right.grip_active);
    print_matrix(L"gun_LocalToWorld",gun);print_matrix(L"avatar_LocalToWorld",avatar);
    const auto changed=bones_before_valid&&bones_after_valid?pose_change_mask(bones_before,bones_after):0u;
    const auto paired=accepted&&bones_before_valid&&bones_after_valid&&!changed;
    fwprintf(log_file,L" pose_pair_accepted=%u pose_before_valid=%u pose_after_valid=%u pose_change_mask=%u pose_epoch_before=%u pose_epoch_after=%u",
        unsigned(paired),unsigned(bones_before_valid),unsigned(bones_after_valid),changed,bones_before.epoch,bones_after.epoch);
    std::uint64_t bone_name_before{},bone_name_after{};
    if(bones_before_valid)std::memcpy(&bone_name_before,bones_before.reference_values.data()+52*68,8);
    if(bones_after_valid)std::memcpy(&bone_name_after,bones_after.reference_values.data()+52*68,8);
    fwprintf(log_file,L" pose_avatar_mesh=%llx pose_avatar_asset_before=%llx pose_avatar_asset_after=%llx pose_ref52_before=%llx pose_ref52_after=%llx",
        f.mesh.address,bones_before.asset,bones_after.asset,bone_name_before,bone_name_after);
    if(bones_before_valid){print_matrix(L"weapon_end_before",cohort::world(bones_before,52));print_matrix(L"weapon_r_before",cohort::world(bones_before,51));print_matrix(L"right_wrist_before",cohort::world(bones_before,35));}
    if(bones_after_valid){print_matrix(L"weapon_end_after",cohort::world(bones_after,52));print_matrix(L"weapon_r_after",cohort::world(bones_after,51));print_matrix(L"right_wrist_after",cohort::world(bones_after,35));}
    fwprintf(log_file,L" observation_only=1 source_is_parent_V1=1 same_C0_not_proved=1 final_render_phase_not_proved=1\n");fflush(log_file);
}
void reset_after_drain() noexcept {budget={};next_sample=0;}
} // namespace nativeequipment_component
