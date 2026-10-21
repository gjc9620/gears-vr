#pragma once
#include "ngx_native_optimal_settings.h"
#include "ngx_stereo_adapter.h"
#include <stdexcept>
#include <cmath>

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
// Game helper RVA be61bc, NOT a public SDK API signature. Its RCX input slot
// is overwritten; duplicate outputW there as original be5888 does. EDX/R8D/R9D
// are outputW/outputH/quality; seven stack pointers are optimal/max/min W,H and
// sharpness. The helper reads the game's global parameter cell internally.
using NativeOptimalQuery=NativeResult(GEARS_NGX_NATIVE_CALL*)(uint32_t,uint32_t,uint32_t,uint32_t,
    uint32_t*,uint32_t*,uint32_t*,uint32_t*,uint32_t*,uint32_t*,float*);
#undef GEARS_NGX_NATIVE_CALL
struct NativeDispatchFunctions {
    NativeCreateDispatch create{};NativeEvaluateDispatch evaluate{};NativeReleaseDispatch release{};
    // Host closes over the typed original CopyTextureRegion arguments at the
    // admitted native boundary. The callback performs exactly this boxed copy
    // via the unfiltered original and throws/rejects if those arguments drift.
    void* copy_context{};void (*copy)(void*,CopyPlan const&){};
    // Optional for a raw dispatcher backend; mandatory for a live ControlOwner.
    NativeOptimalQuery optimal{};
    void* trace_context{};
    void (*trace)(void*,char const*,uint64_t,uint64_t,NativeResult){};
    bool ready()const{return create&&evaluate&&release&&copy;}
};
class NativeDispatchBackend final:public AdapterBackend {
    NativeDispatchFunctions functions_;
    std::atomic<uint64_t> create_attempts_{};
public:
    explicit NativeDispatchBackend(NativeDispatchFunctions functions):functions_(functions){}
    void trace(char const* stage,uint64_t command,uint64_t value,NativeResult result=0){
        if(functions_.trace)functions_.trace(functions_.trace_context,stage,command,value,result);
    }
    NativeOptimalSettings optimal(Extent output,uint32_t quality){
        NativeOptimalSettings out;out.output=output;out.quality=quality;
        if(!functions_.optimal||!bounded(output))return out;
        out.called=true;
        out.result=functions_.optimal(output.width,output.width,output.height,quality,
            &out.optimal.width,&out.optimal.height,&out.maximum.width,&out.maximum.height,
            &out.minimum.width,&out.minimum.height,&out.sharpness);return out;
    }
    NativeResult create(uint64_t command,uint32_t feature,uint64_t parameters,uint64_t* handle)override{
        if(functions_.create)++create_attempts_;
        if(functions_.trace)functions_.trace(functions_.trace_context,"create_enter",command,feature,0);
        const auto result=functions_.create?functions_.create(command,feature,parameters,handle):dispatch_unavailable;
        if(functions_.trace)functions_.trace(functions_.trace_context,"create_leave",command,handle?*handle:0,result);return result;
    }
    NativeResult evaluate(uint64_t command,uint64_t handle,uint64_t parameters,uint64_t callback)override{
        if(functions_.trace)functions_.trace(functions_.trace_context,"eval_enter",command,handle,0);
        const auto result=functions_.evaluate?functions_.evaluate(command,handle,parameters,callback):dispatch_unavailable;
        if(functions_.trace)functions_.trace(functions_.trace_context,"eval_leave",command,handle,result);return result;
    }
    NativeResult release(uint64_t handle)override{
        trace("release_enter",0,handle);
        const auto result=functions_.release?functions_.release(handle):dispatch_unavailable;
        trace("release_return",0,handle,result);return result;
    }
    void copy(CopyPlan const& plan)override{
        if(!functions_.copy)throw std::logic_error("NGX typed copy boundary unavailable");
        functions_.copy(functions_.copy_context,plan);
    }
    uint64_t create_attempts()const{return create_attempts_.load();}
};
}
