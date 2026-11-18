#pragma once
#include "../vr-common/controller_input_bridge.h"
#include <cstdint>

// Bounded recovery for a transient zero-wait controller mutex contention.
// The pinned stereo sample is never replaced: every read and the final match
// check use this exact frame. Only a genuine WAIT_TIMEOUT (contended=true) may
// be retried. Abandoned/invalid mutex failures remain immediate failures.
namespace gears_cover_controller_match_retry {
using StereoSample=gears_vr::StereoSample;
using ControllerInputFrame=gears_vr::ControllerInputFrame;
using ControllerMatchingRead=gears_vr::ControllerMatchingRead;
inline constexpr unsigned maximum_attempts=8;
inline constexpr std::int64_t maximum_retry_budget_ns=250000;

struct RetryInfo {
    enum class Stop : unsigned {none,read_error,attempt_limit,budget,clock_regression,revalidation};
    unsigned attempts{};
    bool recovered{};
    bool exhausted{};
    ControllerMatchingRead last_read{};
    Stop stop{Stop::none}; // Receiver-local diagnostics only, never wire data.
};

// Read protocol:
//   bool read(StereoSample const& pinned, std::int64_t now,
//             ControllerInputFrame& out, bool allow_sim,
//             bool& contended, ControllerMatchingRead& diagnostic) noexcept
// The callback forwards the exact pinned sample to the bridge and exposes both
// its contended flag and its complete diagnostic. Pause must not sleep.
template<class Read,class Clock,class Pause>
bool read_matching_retry(StereoSample const& pinned,std::int64_t& now_ns,
    ControllerInputFrame& out,bool allow_sim,Read&& read,Clock&& clock,Pause&& pause,
    RetryInfo* info=nullptr) noexcept {
    using Code=ControllerMatchingRead::Code;
    out={};
    RetryInfo local{};
    ControllerInputFrame candidate{};
    bool contended=false;
    ControllerMatchingRead diagnostic{};
    auto attempt=[&](std::int64_t at) noexcept {
        ++local.attempts;
        candidate={};
        contended=false;
        diagnostic={};
        const bool result=read(pinned,at,candidate,allow_sim,contended,diagnostic);
        if(!result&&diagnostic.code==Code::accepted)diagnostic.code=Code::frame_mismatch;
        local.last_read=diagnostic;
        return result;
    };
    auto finish=[&](RetryInfo::Stop stop,bool exhausted) noexcept {
        local.stop=stop;
        local.exhausted=exhausted;
        out={};
        if(info)*info=local;
        return false;
    };

    if(attempt(now_ns)){
        out=candidate;
        if(info)*info=local;
        return true;
    }
    if(diagnostic.code!=Code::mutex_unavailable||!contended)
        return finish(RetryInfo::Stop::read_error,false);

    const auto start=now_ns;
    while(local.attempts<maximum_attempts){
        pause();
        const auto retry_now=clock();
        if(retry_now<=0||retry_now<now_ns)
            return finish(RetryInfo::Stop::clock_regression,false);
        if(retry_now-start>maximum_retry_budget_ns)
            return finish(RetryInfo::Stop::budget,true);
        now_ns=retry_now;
        if(!attempt(now_ns)){
            if(diagnostic.code!=Code::mutex_unavailable||!contended)
                return finish(RetryInfo::Stop::read_error,false);
            continue;
        }

        // A successful bridge read still has to match the unchanged pinned
        // frame at a fresh observation time. The caller's now advances; source
        // timestamps, sequence and the pinned stereo sample never do.
        const auto verify_now=clock();
        if(verify_now<=0||verify_now<now_ns||
           verify_now-start>maximum_retry_budget_ns){
            local.last_read.code=Code::frame_mismatch;
            return finish(verify_now<=0||verify_now<now_ns?RetryInfo::Stop::clock_regression:
                RetryInfo::Stop::budget,verify_now>0&&verify_now>=now_ns);
        }
        now_ns=verify_now;
        if(!gears_vr::controller_frame_matches(candidate,pinned,verify_now,allow_sim)){
            local.last_read.code=Code::frame_mismatch;
            return finish(RetryInfo::Stop::revalidation,false);
        }
        local.recovered=true;
        out=candidate;
        if(info)*info=local;
        return true;
    }
    return finish(RetryInfo::Stop::attempt_limit,true);
}
} // namespace gears_cover_controller_match_retry