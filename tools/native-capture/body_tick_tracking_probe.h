#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../vr-common/native_rig_tracking_frame.h"
#include "../vr-common/controller_continuity_bridge.h"
#pragma push_macro("small")
#undef small
#include "full_body_c0_planner.h"
#pragma pop_macro("small")
#include <array>
#include <cstdio>
#include <type_traits>

namespace gears_body_tick_probe {
enum class Failure:std::uint32_t {okay,context,lock,reference,stereo,hands,publisher,continuity,source,age,replay,exception};
struct Record {
    std::uint64_t tick{},thread{},cookie{};std::int64_t begin_ns{},end_ns{};
    Failure failure{Failure::exception};std::uint32_t valid{},newer{},basis_available{};
    gears_native_rig_tracking::CoreSnapshot original_scene{};
    gears_vr::StereoSample sampled{};gears_vr::ControllerInputFrame hands{};
    gears_hand_continuity::TrackedInput continuity{};
    gears_rig_tracking::NativeReference reference{};
    std::uint64_t reference_revision{},publisher_pid{},publisher_start{},compose{};
    std::int64_t basis_observed_ns{};gears_vr::StereoMatrix base_view{};
    gears_full_body_c0::Matrix64 component{};gears_full_body_c0::rig::Signature geometry{};
};
struct Header {
    char magic[8]{'B','T','I','P','R','B','0','1'};
    std::uint32_t version{1},header_bytes{sizeof(Header)},record_bytes{sizeof(Record)},count{},capacity{32},reserved{};
    std::uint64_t pid{},native_started{},read_only{1},writer{};
};
static_assert(std::is_trivially_copyable_v<Record> && sizeof(Header)==64);
}
namespace body_tick_tracking_probe_live {
#if defined(GEARS_BODY_TICK_TRACKING_PROBE) && GEARS_BODY_TICK_TRACKING_PROBE
bool configure(wchar_t const*,FILE*);
void sample(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&)noexcept;
void end_tick(std::uint64_t)noexcept;
void note_basis(std::uint64_t,std::uint64_t,gears_full_body_c0::Matrix64 const&,gears_vr::StereoMatrix const&,gears_full_body_c0::rig::Signature const&)noexcept;
void summary(FILE*,bool)noexcept;
#else
inline constexpr bool enabled=false;
inline bool configure(wchar_t const* ini,FILE*){return GetPrivateProfileIntW(L"BodyTickTrackingProbe",L"enabled",0,ini)==0;}
inline void sample(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&)noexcept{}
inline void end_tick(std::uint64_t)noexcept{}
inline void note_basis(std::uint64_t,std::uint64_t,gears_full_body_c0::Matrix64 const&,gears_vr::StereoMatrix const&,gears_full_body_c0::rig::Signature const&)noexcept{}
inline void summary(FILE*,bool)noexcept{}
#endif
}
