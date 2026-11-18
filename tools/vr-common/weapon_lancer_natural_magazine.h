#pragma once
#include "weapon_lancer_magazine_pose.h"
#include "weapon_lancer_owner_grabposes.h"
// Original Lancer120v/156tri: owner's LEFT capture converted from installed
// gun coordinates into the SAME detached item basis in lancer_magazine_cache.
// Both item_for_wrist and wrist_for_item consume this one paired transform.
// Authoring data only; not controller calibration or gameplay permission.
namespace gears_lancer_natural_magazine {
inline constexpr gears_weapon::LancerMagazineGripPose grip{gears_lancer_owner_grabposes::magazine_wrist_in_item};
}
