#pragma once
#include "native_action_context.h"

namespace gears_native_context {
struct SceneArguments {void* local{};void* family{};void* position{};void* rotation{};void* viewport{};void* drawer{};};
using SceneBody=void*(*)(SceneArguments const&);
// The storage crossing SEH is POD. Original exceptions and opaque pointer/RAX
// are forwarded unchanged. /EHsc destructors alone cannot close this marker.
inline void* invoke_scene(SceneBody body,SceneArguments const& arguments){
    Ticket ticket{};void* value{};bool entered{};
    __try {
        entered=enter(thread_ledger,ticket,Kind::scene);
        value=body(arguments);
    }__finally {if(entered)(void)leave(thread_ledger,ticket);}
    return value;
}
} // namespace gears_native_context
