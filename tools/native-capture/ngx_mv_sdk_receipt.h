#pragma once
#include <cstdint>

namespace gears_ngx_mv_target {
// One wrapper's real SDK call receipt. Native callers accept non-BAD results;
// this is deliberately not HRESULT success and not GPU-completion evidence.
class SdkReceipt {
    std::uintptr_t command_{},handle_{},parameters_{};
    bool begun_{},entered_{},returned_{},failed_{},finished_{};
    std::uint32_t result_{};
public:
    bool begin(std::uintptr_t command,std::uintptr_t handle,std::uintptr_t parameters){
        if(begun_||failed_||!command||!handle||!parameters){failed_=true;return false;}
        begun_=true;command_=command;handle_=handle;parameters_=parameters;return true;
    }
    bool dispatch_enter(std::uintptr_t command,std::uintptr_t handle,std::uintptr_t parameters,std::uintptr_t callback){
        if(!begun_||failed_||entered_||finished_||callback||command!=command_||handle!=handle_||parameters!=parameters_){failed_=true;return false;}
        entered_=true;return true;
    }
    bool dispatch_leave(bool returned,std::uint32_t result){
        if(!entered_||returned_||finished_||failed_||!returned){failed_=true;return false;}
        returned_=true;result_=result;return true;
    }
    bool finish(bool wrapper_returned,bool unwinding,std::uint64_t wrapper_result){
        if(finished_){failed_=true;return false;}finished_=true;
        if(!begun_||!entered_||!returned_||failed_||!wrapper_returned||unwinding||
           static_cast<std::uint32_t>(wrapper_result)!=result_||(result_&0xfff00000u)==0xbad00000u){failed_=true;return false;}
        return true;
    }
    std::uint32_t raw_result()const{return result_;}
    bool failed()const{return failed_;}
};
}
