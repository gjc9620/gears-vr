#pragma once
#include "pose_math.h"

namespace gears_weapon_longshot_bolt_contact {
// Original LOD0 b_LS_Bolt head, export741d9653. Centre fitted to50 distinct
// outer-cap vertices; maximum radial residual0.01647 raw units. The former
// provisional vertex356 was3.926cm up the stem, not the knob-head centre.
// Mesh-local source units, before CURRENT mesh/bone/world transforms.
// This anchor supplies neither contact nor a mechanism/hand ownership grant.
inline constexpr gears_vr::Vector knob_in_mesh_units{
    -4.81249039685874,13.941650242324553,5.356091693579125};
}
