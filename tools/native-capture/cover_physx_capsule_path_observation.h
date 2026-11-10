#pragma once
#include "cover_physx_capsule_path.h"

namespace gears_cover_capsule_path {
struct HitSpan {std::uint32_t first{},count{},raw_count{},reports{};};
// Flat all-hit observations, retaining duplicates and segment provenance.
// Endpoint is the ORIGINAL unfiltered overlap boolean. Neither empty spans
// nor endpoint.raw_hit=false certify self exclusion or current world occupancy.
struct RawPath {
    Plan plan{};
    std::array<HitSpan,max_segments> spans{};
    q::AllSweepOwners collected{};
    q::Overlap endpoint{};
};
} // namespace gears_cover_capsule_path
