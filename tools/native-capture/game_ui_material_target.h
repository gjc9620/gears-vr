#pragma once
#include "frame_link.h"
namespace gears_game_material {
inline gears_frame_link::Result material_target_pair(gears_frame_link::Tracker& tracker,
    gears_frame_link::ListRef ref,gears_frame_link::Target target,unsigned format,
    bool known_buffer,bool final_transfer_enabled){
    if(known_buffer)return tracker.overlay_pair(ref,target);
    // The attested final transfer consumes this format. Its own later source
    // identity/geometry checks still decide whether the image can be published.
    // A material write may only extend a complete local stereo pair; an unknown
    // or executed-only source remains rejected.
    if(!final_transfer_enabled||format!=24||!target.width||target.width%2||
       target.width>8192||!target.height||target.height>8192)return {};
    return tracker.current_recorded_pair(ref,target);
}
}
