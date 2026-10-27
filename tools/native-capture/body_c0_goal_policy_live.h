#pragma once
namespace body_c0_goal_policy_live {
bool configure(wchar_t const*,FILE*);
void pose_begin(gears_continuous_hand_entry::Entry const&) noexcept;
// Delegates Native pose_end exactly once, including OFF/rejected/exception paths.
void pose_end(gears_continuous_hand_entry::Entry const&,bool,bool) noexcept;
// Admitted Scene/GT data query, intended after FP/run and before stereo/Rig.
// Returns the exact successful Goal's pre-HMD matrix. Never returns desiredEye.
bool read_view(gears_anchor::Receipt const*,gears_vr::StereoResult const&,double,gears_vr::CoverViewResult&) noexcept;
void request_drain();unsigned pending_count();void summary(FILE*);
}
