#pragma once
#include "weapon_button_veto.h"
// Legacy source names and export are preserved. The shared wire is unchanged;
// new weapons need not depend on a Longshot-specific API name.
namespace gears_weapon_longshot_scope_input {
inline constexpr char export_name[]="GearsCaptureLongshotScopeInputV1";
using Request=gears_weapon_button_veto::Request;
using Result=gears_weapon_button_veto::Result;
using Update=gears_weapon_button_veto::Update;
}
