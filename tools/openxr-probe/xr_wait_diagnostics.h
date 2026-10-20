#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <limits>

// One XR thread, fixed storage, no per-frame IO. Durations are CPU wall waits,
// not GPU execution time or headset/VD display latency.
namespace xr_wait_diagnostics {
enum class Operation : unsigned {wait_frame,wait_eye0,wait_eye1,local_fence,end_frame,count};
enum class Context : unsigned {focused_render,other_render,not_rendering,unknown,count};
enum class Outcome {success,timeout,error};
inline Context context(int state,bool should_render) noexcept {
    return !should_render?Context::not_rendering:state==5?Context::focused_render:Context::other_render;
}
inline constexpr std::array<std::uint64_t,10> bounds_ns{{10000,100000,500000,1000000,2000000,5000000,11112000,33334000,100000000,1000000000}};
inline void add(std::uint64_t& value,std::uint64_t amount=1) noexcept {
    const auto maximum=std::numeric_limits<std::uint64_t>::max();
    value=amount>maximum-value?maximum:value+amount;
}
struct Statistics {
    std::uint64_t count{},total_ns{},maximum_ns{},timeouts{},errors{},invalid_clock{};
    std::array<std::uint64_t,bounds_ns.size()+1> buckets{};
    void observe(std::int64_t start,std::int64_t finish,Outcome outcome) noexcept {
        add(count);if(outcome==Outcome::timeout)add(timeouts);if(outcome==Outcome::error)add(errors);
        if(start<0||finish<start){add(invalid_clock);return;}
        const auto duration=std::uint64_t(finish)-std::uint64_t(start);
        add(total_ns,duration);if(duration>maximum_ns)maximum_ns=duration;
        unsigned bucket=0;while(bucket<bounds_ns.size()&&duration>bounds_ns[bucket])++bucket;
        add(buckets[bucket]);
    }
};
struct Clock {
    std::int64_t operator()() const noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
template<bool Enabled,class Time=Clock> class Recorder {
    Time clock_{};
    bool dumped_{};
public:
    std::array<std::array<Statistics,unsigned(Context::count)>,unsigned(Operation::count)> values{};
    explicit Recorder(Time clock=Time{}):clock_(clock){}
    template<class Call,class GetContext,class Classify>
    auto measure(Operation operation,Call call,GetContext get_context,Classify classify) {
        if constexpr(!Enabled)return call();
        else {
            const auto begin=clock_();const auto result=call();const auto end=clock_();
            values[unsigned(operation)][unsigned(get_context())].observe(begin,end,classify(result));
            return result;
        }
    }
    void dump(FILE* output=stdout) noexcept {
        if constexpr(Enabled){
            if(dumped_)return;dumped_=true;
            const char* operations[]={"xrWaitFrame","xrWaitSwapchainImage_eye0","xrWaitSwapchainImage_eye1","local_d3d12_fence_wait","xrEndFrame"};
            const char* contexts[]={"focused_should_render","other_should_render","should_render_false","unknown"};
            for(unsigned op=0;op<values.size();++op)for(unsigned ctx=0;ctx<values[op].size();++ctx){
                const auto& s=values[op][ctx];
                if(!s.count)continue;
                fprintf(output,"{\"xr_wait_diagnostics\":true,\"schema\":1,\"operation\":\"%s\",\"context\":\"%s\",\"semantics\":\"cpu_wall_wait_not_gpu_or_display_latency\",\"count\":%llu,\"total_ns\":%llu,\"maximum_ns\":%llu,\"timeouts\":%llu,\"errors\":%llu,\"invalid_clock\":%llu,\"saturating\":true,\"bucket_upper_ns\":[",operations[op],contexts[ctx],(unsigned long long)s.count,(unsigned long long)s.total_ns,(unsigned long long)s.maximum_ns,(unsigned long long)s.timeouts,(unsigned long long)s.errors,(unsigned long long)s.invalid_clock);
                for(unsigned i=0;i<bounds_ns.size();++i)fprintf(output,"%s%llu",i?",":"",(unsigned long long)bounds_ns[i]);
                fprintf(output,",null],\"buckets\":[");
                for(unsigned i=0;i<s.buckets.size();++i)fprintf(output,"%s%llu",i?",":"",(unsigned long long)s.buckets[i]);
                fprintf(output,"]}\n");
            }
            fflush(output);
        }
    }
};
#if defined(GEARS_XR_WAIT_DIAGNOSTICS) && GEARS_XR_WAIT_DIAGNOSTICS
inline Recorder<true> recorder;
#else
inline Recorder<false> recorder;
#endif
inline Context current_context=Context::unknown;
struct SessionDump {~SessionDump(){recorder.dump();}};
}
