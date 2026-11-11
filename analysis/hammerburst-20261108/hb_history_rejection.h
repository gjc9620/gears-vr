#pragma once
#include "../vr-common/controller_relative_motion.h"
#include <cstdio>
#include <cwchar>
// Called only AFTER the original pure build/count rejection. It observes
// existing caller-owned values; no ring reread, clock or permission is made.
namespace hb_history_rejection {
inline unsigned records{};
inline void pose(FILE* f,wchar_t const* tag,gears_vr::Pose const& p) noexcept {
    std::fwprintf(f,L" %ls_px=%.17g %ls_py=%.17g %ls_pz=%.17g %ls_qx=%.17g %ls_qy=%.17g %ls_qz=%.17g %ls_qw=%.17g",
        tag,p.position.x,tag,p.position.y,tag,p.position.z,tag,p.orientation.x,tag,p.orientation.y,tag,p.orientation.z,tag,p.orientation.w);
}
inline void raw(FILE* f,unsigned record,wchar_t const* role,unsigned index,gears_hand_continuity::TrackedInput const& r) noexcept {
    auto const& s=r.input;
    std::fwprintf(f,L"hammerburst_history_raw record=%u role=%ls index=%u source=%u session=%llu sequence=%llu space=%llu profile=%llu input_epoch=%llu predicted_ns=%lld received_ns=%lld sync_ns=%lld focused=%u display=%u display_ns=%lld stream=%llu track_left=%llu track_right=%llu\n",
        record,role,index,unsigned(s.source),s.session,s.sequence,s.space_epoch,s.profile_generation,s.input_epoch,
        s.predicted_time,s.received_ns,s.sync_ns,s.focused,s.display_mode,s.display_observed_ns,r.stream_epoch,r.tracking_epoch[0],r.tracking_epoch[1]);
    for(unsigned n=0;n<2;++n){auto const& h=s.hands[n];
        std::fwprintf(f,L"hammerburst_history_hand record=%u role=%ls index=%u hand=%u sequence=%llu grip_active=%u grip_flags=%llu aim_active=%u aim_flags=%llu trigger_active=%u trigger=%.17g squeeze_active=%u squeeze=%.17g stick_active=%u stick_x=%.17g stick_y=%.17g buttons=%u buttons_active=%u last_squeeze_flags=%llu last_squeeze_sequence=%llu",
            record,role,index,n,s.sequence,h.grip_active,h.grip_flags,h.aim_active,h.aim_flags,h.trigger_active,double(h.trigger),h.squeeze_active,double(h.squeeze),h.stick_active,double(h.stick_x),double(h.stick_y),h.buttons,h.buttons_active,h.last_squeeze_press_flags,h.last_squeeze_press_sequence);
        pose(f,L"grip",h.grip);pose(f,L"aim",h.aim);pose(f,L"last_squeeze",h.last_squeeze_press_grip);
        for(unsigned c=0;c<gears_vr::control_count;++c)std::fwprintf(f,L" p%u=%u r%u=%u",c,h.pressed[c],c,h.released[c]);
        std::fwprintf(f,L"\n");
    }
}
inline void capture(FILE* f,std::uint64_t frame,std::uint64_t sequence,unsigned control,bool built,unsigned points,
    gears_hand_continuity::TrackedInput const& previous,gears_hand_continuity::TrackedInput const& current,
    gears_hand_continuity::MotionHistory const& history,gears_vr::Pose const& mount,double scale) noexcept {
    if(!f||records>=4)return;const auto record=++records;
    std::fwprintf(f,L"hammerburst_history_rejection record=%u frame=%llu sequence=%llu control=%u built=%u points=%u previous_sequence=%llu current_sequence=%llu history_count=%u history_reserved=%u capacity=%u scale=%.17g action=0",
        record,frame,sequence,control,unsigned(built),points,previous.input.sequence,current.input.sequence,history.count,history.reserved,gears_hand_continuity::motion_history_capacity,scale);
    pose(f,L"mount",mount);std::fwprintf(f,L"\n");raw(f,record,L"previous",0,previous);raw(f,record,L"current",0,current);
    for(unsigned i=0;i<history.count&&i<gears_hand_continuity::motion_history_capacity;++i)raw(f,record,L"history",i,history.frames[i]);
    std::fflush(f);
}
}
