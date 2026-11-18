#pragma once
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/rig_tracking_frame.h"
#include "../vr-common/weapon_gnasher_live_basis.h"
#include "../vr-common/weapon_gnasher_hold_geometry.h"
#include "../vr-common/weapon_gnasher_calibration.h"
// Data/action composition on the existing GT owners; no extra input or writer.
namespace {namespace gnasher_runtime {
enum class MainResult {not_applicable,applied,rejected};
void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept;
void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept;
void tick(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&,
          gears_weapon_hand::Decision const&) noexcept;
MainResult correct_main(gears_weapon_hand::Frame const&,gears_rig_tracking::Frame const&,
                       gears_vr::Viewpoint&) noexcept;
}}
