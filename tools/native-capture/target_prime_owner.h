#pragma once
#include "continuous_owner.h"

namespace gears_prime {
struct OwnerArgument {
    std::uint32_t size,version,pid,reserved;
    std::uint64_t creation_time;
};
static_assert(sizeof(OwnerArgument)==24,"Owned prime ABI");
inline bool valid(const OwnerArgument& value) {
    return value.size==sizeof(OwnerArgument)&&value.version==1&&value.pid&&
        !value.reserved&&value.creation_time;
}
inline bool same_owner(DWORD pid,std::uint64_t created,DWORD other_pid,std::uint64_t other_created) {
    return pid&&created&&pid==other_pid&&created==other_created;
}
}
