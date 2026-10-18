#pragma once
#include "ngx_stereo_adapter.h"
#include <stdexcept>

namespace gears_ngx_stereo {
// Windows x64 native ABI recovered from the complete game dispatchers. Pass
// MinHook's original trampoline addresses here, never the hooked entry points.
// No symbol is dynamically resolved, installed or authenticated by this type.
#if defined(_MSC_VER)
#define GEARS_NGX_NATIVE_CALL __fastcall
#else
#define GEARS_NGX_NATIVE_CALL
#endif
using NativeCreateDispatch=NativeResult(GEARS_NGX_NATIVE_CALL*)(uint64_t,uint32_t,uint64_t,uint64_t*);
using NativeEvaluateDispatch=NativeResult(GEARS_NGX_NATIVE_CALL*)(uint64_t,uint64_t,uint64_t,uint64_t);
using NativeReleaseDispatch=NativeResult(GEARS_NGX_NATIVE_CALL*)(uint64_t);
#undef GEARS_NGX_NATIVE_CALL
struct NativeDispatchFunctions {
    NativeCreateDispatch create{};NativeEvaluateDispatch evaluate{};NativeReleaseDispatch release{};
    // Host closes over the typed original CopyTextureRegion arguments at the
    // admitted native boundary. The callback performs exactly this boxed copy
    // via the unfiltered original and throws/rejects if those arguments drift.
    void* copy_context{};void (*copy)(void*,CopyPlan const&){};
    bool ready()const{return create&&evaluate&&release&&copy;}
};
class NativeDispatchBackend final:public AdapterBackend {
    NativeDispatchFunctions functions_;
public:
    explicit NativeDispatchBackend(NativeDispatchFunctions functions):functions_(functions){}
    NativeResult create(uint64_t command,uint32_t feature,uint64_t parameters,uint64_t* handle)override{
        return functions_.create?functions_.create(command,feature,parameters,handle):dispatch_unavailable;
    }
    NativeResult evaluate(uint64_t command,uint64_t handle,uint64_t parameters,uint64_t callback)override{
        return functions_.evaluate?functions_.evaluate(command,handle,parameters,callback):dispatch_unavailable;
    }
    NativeResult release(uint64_t handle)override{
        return functions_.release?functions_.release(handle):dispatch_unavailable;
    }
    void copy(CopyPlan const& plan)override{
        if(!functions_.copy)throw std::logic_error("NGX typed copy boundary unavailable");
        functions_.copy(functions_.copy_context,plan);
    }
};
}
