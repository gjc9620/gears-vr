#pragma once
#include <array>
#include <cstdint>

namespace gears_crosshair_view {
// Copied writer locals only; validity does not certify camera equivalence.
struct Sample {
    bool valid{};
    std::array<float,3> location{},start{},direction{};
    std::array<std::int32_t,3> rotation{};
    bool camera_binding_valid{};
    std::uintptr_t camera{},level{},local{};
    std::uint32_t assignment_stamp{};
    bool aiming_known{},aiming{};
    // Set only by Native after consuming a completed, authenticated held-HUD
    // call whose mutated locals match this assignment's writer sample.
    std::uint64_t weapon_query_call{};
    std::int64_t weapon_query_deadline_qpc{};
};
inline bool equal(Sample const& a,Sample const& b){
    return a.valid==b.valid&&a.location==b.location&&a.start==b.start&&
        a.direction==b.direction&&a.rotation==b.rotation&&a.camera_binding_valid==b.camera_binding_valid&&
        a.camera==b.camera&&a.level==b.level&&a.local==b.local&&a.assignment_stamp==b.assignment_stamp&&
        a.aiming_known==b.aiming_known&&a.aiming==b.aiming&&
        a.weapon_query_call==b.weapon_query_call&&a.weapon_query_deadline_qpc==b.weapon_query_deadline_qpc;
}
}
