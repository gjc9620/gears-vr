#pragma once

namespace gears_mounted {
// A class allowlist only. Callers still attest the complete possession, world,
// registry and camera-timestamp certificate before admitting a frame.
template<class Has> bool supported(Has has){
    return has(L"Vehicle")&&(has(L"Turret")||
        (has(L"Vehicle_Junker")&&has(L"WarVehicle"))||
        (has(L"WarJunkerSpotlightPawn")&&has(L"WarWeaponPawn")));
}
}
