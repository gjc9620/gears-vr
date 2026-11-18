#pragma once
#include <cstdint>
// Presentation selection only. Inventory authority remains the revisioned
// transaction/writer and its current acknowledged receipt.
namespace gears_boltok_group_mask {
inline bool select_missing(unsigned current,unsigned quantity,unsigned& out) noexcept {
    if(current>63||!quantity||quantity>6)return false;
    unsigned selected=0,remaining=quantity;
    for(unsigned slot=0;slot<6&&remaining;++slot)
        if(!(current&(1u<<slot))){selected|=1u<<slot;--remaining;}
    if(remaining)return false;
    out=selected;return true;
}
inline bool valid_ack(unsigned current,unsigned group,unsigned quantity) noexcept {
    if(current>63||group>63||!quantity||quantity>6||(current&group))return false;
    unsigned count=0;for(auto bits=group;bits;bits>>=1)count+=bits&1u;
    return count==quantity;
}
}
