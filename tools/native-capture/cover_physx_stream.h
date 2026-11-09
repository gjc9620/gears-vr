#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>

// Original A71F0 all-hit callback uses vtable[0](this,uint32,Hit const*).
// It is called synchronously with the caller's fixed64-record batch. This
// collector copies raw bytes only: no object reads, private locks or filters.
namespace gears_cover_physx_query {
struct alignas(8) Hit {std::array<unsigned char,64> bytes{};};
constexpr unsigned stream_batch_hits=64,max_stream_hits=1024,max_stream_reports=256;
struct StreamLimits {std::uint32_t max_hits{max_stream_hits},max_reports{max_stream_reports};};
struct AllSweep {
    std::uint32_t raw_count{},hit_count{},reports{},reserved{};
    std::array<Hit,max_stream_hits> hits{};
};
static_assert(sizeof(Hit)==64&&alignof(Hit)==8&&sizeof(AllSweep)==65552);
static_assert(std::is_trivially_copyable_v<AllSweep>);
namespace detail {
inline bool stream_limits(StreamLimits const& limits)noexcept{return limits.max_hits&&limits.max_hits<=max_stream_hits&&
    limits.max_reports&&limits.max_reports<=max_stream_reports;}
struct StreamReporter;
using StreamFunction=bool(__fastcall*)(StreamReporter*,std::uint32_t,Hit const*)noexcept;
struct StreamReporter {
    StreamFunction const* vtable{};AllSweep* result{};Hit const* batch{};
    std::uint32_t thread{};StreamLimits limits{};bool truncated{},malformed{};
};
static_assert(std::is_standard_layout_v<StreamReporter>&&offsetof(StreamReporter,vtable)==0);
inline thread_local StreamReporter* active_stream_reporter{};
inline bool __fastcall stream_report(StreamReporter* self,std::uint32_t count,Hit const* data)noexcept{
    // Never dereference an unrecognized/expired callback object. Only the
    // synchronous owner below may install the current thread's reporter.
    auto* current_reporter=active_stream_reporter;
    if(!current_reporter||self!=current_reporter){if(current_reporter)current_reporter->malformed=true;return false;}
    if(self->thread!=GetCurrentThreadId()||!self->result||!self->batch||!stream_limits(self->limits)||
       data!=self->batch||count>stream_batch_hits||self->result->hit_count>self->limits.max_hits){self->malformed=true;return false;}
    if(self->truncated||self->malformed)return false;
    auto& out=*self->result;
    if(out.reports>=self->limits.max_reports||count>self->limits.max_hits-out.hit_count){self->truncated=true;return false;}
    // Zero-hit reports are not an end marker; the original iterator decides
    // completion. They still consume the bounded callback budget.
    ++out.reports;
    if(count)std::memcpy(out.hits.data()+out.hit_count,data,std::size_t(count)*sizeof(Hit));
    out.hit_count+=count;return true;
}
inline constexpr std::array<StreamFunction,1> stream_vtable{{stream_report}};
class StreamScope {
    StreamReporter* reporter_{};
public:
    explicit StreamScope(StreamReporter& reporter)noexcept{
        if(!active_stream_reporter){reporter_=&reporter;active_stream_reporter=reporter_;}
    }
    StreamScope(StreamScope const&)=delete;StreamScope& operator=(StreamScope const&)=delete;
    bool admitted()const noexcept{return reporter_!=nullptr;}
    void close()noexcept{if(reporter_){if(active_stream_reporter==reporter_)active_stream_reporter=nullptr;
        else reporter_->malformed=true;reporter_=nullptr;}}
    ~StreamScope(){close();}
};
} // namespace detail
} // namespace gears_cover_physx_query
