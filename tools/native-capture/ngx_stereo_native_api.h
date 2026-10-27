#pragma once
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
struct NativeOptimalSettings {
    Extent output{},optimal{},maximum{},minimum{};uint32_t quality{};float sharpness{};
    NativeResult result{dispatch_unavailable};bool called{};
    bool supports(Extent input)const{
        return called&&native_result_accepted(result)&&bounded(output)&&bounded(optimal)&&bounded(maximum)&&bounded(minimum)&&
            std::isfinite(sharpness)&&minimum.width<=optimal.width&&minimum.height<=optimal.height&&
            optimal.width<=maximum.width&&optimal.height<=maximum.height&&bounded(input)&&
            input.width>=minimum.width&&input.height>=minimum.height&&input.width<=maximum.width&&input.height<=maximum.height&&
            input.width<=output.width&&input.height<=output.height;
    }
};
struct NativeDispatchFunctions {
    NativeCreateDispatch create{};NativeEvaluateDispatch evaluate{};NativeReleaseDispatch release{};
    // Host closes over the typed original CopyTextureRegion arguments at the
    // admitted native boundary. The callback performs exactly this boxed copy
    // via the unfiltered original and throws/rejects if those arguments drift.
    void* copy_context{};void (*copy)(void*,CopyPlan const&){};
    // Optional for a raw dispatcher backend; mandatory for a live ControlOwner.
    NativeOptimalQuery optimal{};
    bool ready()const{return create&&evaluate&&release&&copy;}
};
class NativeDispatchBackend final:public AdapterBackend {
    NativeDispatchFunctions functions_;
    std::atomic<uint64_t> create_attempts_{};
public:
    explicit NativeDispatchBackend(NativeDispatchFunctions functions):functions_(functions){}
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
    uint64_t create_attempts()const{return create_attempts_.load();}
};
}
