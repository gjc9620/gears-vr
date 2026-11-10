#pragma once
#include "weapon_hammerburst_latch.h"
#include <array>

namespace gears_weapon {
// Motion facts only. The live caller must authenticate the original producer
// ring, unchanged grab/context and fixed gun mount, and authorize the CURRENT
// Frame independently. No history point is a Frame, lease, seek or bone ack.
inline HammerburstLatchMotion hammerburst_latch_history(
    HammerburstLatchStroke& stroke,HammerburstLatchSample const& current,
    std::array<HammerburstLatchSample,9> const& original,unsigned count) noexcept {
    const auto same=[](HammerburstLatchSample const& a,HammerburstLatchSample const& b){
        return a.hand.x==b.hand.x&&a.hand.y==b.hand.y&&a.hand.z==b.hand.z&&
            a.sequence==b.sequence&&a.milliseconds==b.milliseconds&&
            a.grip_token==b.grip_token&&a.tracked==b.tracked&&a.gripped==b.gripped;
    };
    const auto reject=[&](){stroke.reset();return HammerburstLatchMotion::rejected;};
    if(!stroke.active()||count<3||count>original.size()||!same(original[count-1],current))return reject();
    const auto first=original[0];const auto previous=stroke.last_hand_in_weapon();
    if(first.milliseconds!=stroke.last_observation_ms()||first.hand.x!=previous.x||
       first.hand.y!=previous.y||first.hand.z!=previous.z||!first.tracked||!first.gripped||
       !first.sequence||first.grip_token!=current.grip_token)return reject();
    auto trial=stroke;
    auto result=HammerburstLatchMotion::rejected;
    for(unsigned i=1;i<count;++i){
        // Original observe retains all gap/order/token/range/speed checks.
        // No interpolation or stationary-gap renewal is permitted here.
        result=trial.observe(original[i],false);
        if(result==HammerburstLatchMotion::rejected)return reject();
    }
    // Intermediate endpoints never escape to a renderer or engine callback.
    stroke=trial;return result;
}
}
