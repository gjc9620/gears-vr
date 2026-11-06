#pragma once
#include "continuous_hand_owner_entry.h"
namespace fresh_pose_producer_live {
// Read-only extension of the existing opt-in original-entry observation. It
// cannot retire an overwrite obligation or admit a new hand pose.
struct PendingBranch {
    uint64_t qpc{},tick{},root{},mesh{},method{},caller{},cache_data{},local_data{};
    unsigned canonical{},depth{},root_epoch{},mesh_epoch{};
    int32_t child_count{},cache_count{},cache_capacity{},local_count{},local_capacity{};
    unsigned kind{},mode{};int32_t descriptors{},layer_count{};
    int32_t sequence_count{},sequence_selected{-1};unsigned sequence_blended{};
    std::array<uint32_t,4> sequence_weights{};
    std::array<uint64_t,4> child_nodes{},child_methods{};
    std::array<uint32_t,4> weights{},child_epochs{};
    std::array<int32_t,4> child_cache_counts{};
};
unsigned pending_branches(uintptr_t,std::array<PendingBranch,16>&) noexcept;
bool configure(wchar_t const*,FILE*);bool reserve();bool reserved_target(void*);
void pose_begin(gears_continuous_hand_entry::Entry const&) noexcept;
void pose_end(gears_continuous_hand_entry::Entry const&,bool) noexcept;
bool blend_world_current() noexcept;
void request_drain();unsigned pending_count();void summary(FILE*);void print(FILE*,bool);
}
