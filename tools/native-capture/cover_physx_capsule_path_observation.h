#pragma once
#include "cover_physx_capsule_path.h"

namespace gears_cover_capsule_path {
struct HitSpan {std::uint32_t first{},count{},raw_count{},reports{};};
// Flat all-hit observations, retaining duplicates and segment provenance.
// Endpoint retains the original overlap shape stream and the same per-shape
// ownership observations as the path. Empty packets still do not certify
// complete world coverage, self exclusion or current world occupancy.
struct RawPath {
    Plan plan{};
    std::array<HitSpan,max_segments> spans{};
    q::AllSweepOwners collected{};
    q::AllOverlapOwners endpoint{};
};
} // namespace gears_cover_capsule_path
