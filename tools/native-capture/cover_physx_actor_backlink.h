#pragma once
#include "cover_physx_query_source.h"

// PhysXCore 22051aa7: NpActor constructor 1328b0 constructs Actor at +28,
// then writes its public-owner field at NpActor+30. Actor constructor 1204c0
// sets its table to 2324f8. NpActor+40 is an OPTIONAL dynamic Body pointer,
// not an Actor whose +10 can be interpreted as a public-Actor backlink.
// Read-only qualification; caller still closes scene/body/shape identities.
namespace gears_cover_physx_actor_backlink {
enum class Code {current,read,unsupported,links};
template<class Read> Code sample(std::uintptr_t base,std::uintptr_t actor,Read& read,std::uintptr_t& owner){
    namespace raw=gears_cover_physx_query_source::pose;
    std::uintptr_t table{},back{};
    if(!raw::field(read,table,actor,0x28)||!raw::field(read,back,actor,0x30))return Code::read;
    if(table!=base+0x2324f8)return Code::unsupported;
    owner=back;
    return back==actor?Code::current:Code::links;
}
}
