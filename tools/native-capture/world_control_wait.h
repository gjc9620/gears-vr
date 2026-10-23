#pragma once
#include <windows.h>
#include <cstdint>

namespace gears_world {
enum class ControlWakeResult {timeout,stop,prepare,commit,failed};
// A successful auto-reset wait owns one receipt until poll takes it. Never
// infer Prepare/Commit authorization merely from a wake or from elapsed time.
class ControlWake {
    bool prepare_{},commit_{},failed_{};
    std::uint64_t prepare_wakes_{},commit_wakes_{},stop_wakes_{},timeouts_{};
    bool take(HANDLE handle,bool& receipt){
        if(receipt){receipt=false;return true;}
        if(!handle)return false;
        const auto result=WaitForSingleObject(handle,0);
        if(result==WAIT_FAILED)failed_=true;
        return result==WAIT_OBJECT_0;
    }
public:
    bool take_prepare(HANDLE handle){return take(handle,prepare_);}
    bool take_commit(HANDLE handle){return take(handle,commit_);}
    bool failed()const{return failed_;}
    std::uint64_t prepare_wakes()const{return prepare_wakes_;}
    std::uint64_t commit_wakes()const{return commit_wakes_;}
    std::uint64_t stop_wakes()const{return stop_wakes_;}
    std::uint64_t timeouts()const{return timeouts_;}
    ControlWakeResult wait(HANDLE stop,HANDLE prepare,HANDLE commit,DWORD milliseconds){
        if(failed_)return ControlWakeResult::failed;
        HANDLE handles[3]{};ControlWakeResult kinds[3]{};DWORD count{};
        // Manual stop has priority and is never reset/consumed by this helper.
        if(stop){handles[count]=stop;kinds[count++]=ControlWakeResult::stop;}
        // Do not consume another auto-reset signal while this kind already
        // has an unpolled receipt. Leave the second signal in the kernel.
        if(prepare&&!prepare_){handles[count]=prepare;kinds[count++]=ControlWakeResult::prepare;}
        if(commit&&!commit_){handles[count]=commit;kinds[count++]=ControlWakeResult::commit;}
        if(!count){Sleep(milliseconds);++timeouts_;return ControlWakeResult::timeout;}
        const auto result=WaitForMultipleObjects(count,handles,FALSE,milliseconds);
        if(result==WAIT_TIMEOUT){++timeouts_;return ControlWakeResult::timeout;}
        if(result<WAIT_OBJECT_0+count){
            const auto kind=kinds[result-WAIT_OBJECT_0];
            if(kind==ControlWakeResult::prepare){prepare_=true;++prepare_wakes_;}
            if(kind==ControlWakeResult::commit){commit_=true;++commit_wakes_;}
            if(kind==ControlWakeResult::stop)++stop_wakes_;
            return kind;
        }
        failed_=true;return ControlWakeResult::failed;
    }
};

class ControlCadence {
    std::uint64_t request_{},began_{};
public:
    static constexpr DWORD idle_ms=20,fast_ms=1;
    static constexpr std::uint64_t burst_ms=250;
    DWORD interval(std::uint64_t request,std::uint64_t now,bool stopping){
        if(stopping){request_=began_=0;return idle_ms;}
        if(request&&request!=request_){request_=request;began_=now;}
        // Retain the tail after commit for the first fresh image/fence poll.
        // The same request cannot extend this bound through repeated signals.
        return request_&&now>=began_&&now-began_<burst_ms?fast_ms:idle_ms;
    }
};
}
