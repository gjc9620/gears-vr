#pragma once
#include "continuous_hand_owner_entry.h"
namespace fresh_pose_producer_live {
bool configure(wchar_t const*,FILE*);bool reserve();bool reserved_target(void*);
void pose_begin(gears_continuous_hand_entry::Entry const&) noexcept;
void pose_end(gears_continuous_hand_entry::Entry const&,bool) noexcept;
void request_drain();unsigned pending_count();void summary(FILE*);void print(FILE*,bool);
}
