#pragma once
#include "continuous_hand_owner_entry.h"
#include "cover_pose_phase.h"
namespace body_recoil_observer_live {
struct WeightCall {
    gears_continuous_hand_entry::Entry const* entry{};WeightCall const* previous{};
    uint64_t tick{},compose{};unsigned depth{};bool counted{},selected{};
};
static_assert(std::is_trivially_copyable_v<WeightCall> && std::is_trivially_destructible_v<WeightCall>);
void compose_begin(gears_continuous_hand_entry::Entry const&) noexcept;
void compose_end(gears_continuous_hand_entry::Entry const&,bool) noexcept;
void weight_begin(gears_continuous_hand_entry::Entry const&,WeightCall&) noexcept;
void weight_end(WeightCall const&,gears_cover_pose_phase::CallPacket const&,bool) noexcept;
bool configure(wchar_t const*,FILE*);void request_drain();unsigned pending_count();
void summary(FILE*);void print(FILE*,bool);
}
