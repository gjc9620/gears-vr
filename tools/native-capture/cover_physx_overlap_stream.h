#pragma once
#include "cover_physx_stream.h"

// Original A9F10/188440 shape callback, not the A71F0 64-byte sweep packet.
// This is a bounded raw collector/ABI adapter only. Its caller still owes
// current code/typed source/owner certification and complete world coverage.
namespace gears_cover_physx_query {
struct AllOverlap {
    std::uint32_t raw_count{},shape_count{},reports{},reserved{};
    std::array<std::uintptr_t,max_stream_hits> shapes{};
};
static_assert(sizeof(std::uintptr_t)==8&&std::is_trivially_copyable_v<AllOverlap>);
namespace detail {
struct OverlapReporter;
using OverlapFunction=bool(__fastcall*)(OverlapReporter*,std::uint32_t,std::uintptr_t const*)noexcept;
struct OverlapReporter {
    OverlapFunction const* vtable{};AllOverlap* result{};std::uintptr_t const* batch{};
    std::uint32_t thread{};StreamLimits limits{};bool truncated{},malformed{},untrusted_batch{};
};
static_assert(std::is_standard_layout_v<OverlapReporter>&&offsetof(OverlapReporter,vtable)==0);
inline thread_local OverlapReporter* active_overlap_reporter{};
inline bool __fastcall overlap_report(OverlapReporter* receiver,std::uint32_t count,std::uintptr_t const* values)noexcept{
    auto* overlap_owner=active_overlap_reporter;
    if(!overlap_owner||receiver!=overlap_owner){if(overlap_owner)overlap_owner->malformed=true;return false;}
    if(receiver->thread!=GetCurrentThreadId()||!receiver->result||!receiver->batch||!stream_limits(receiver->limits)||
       count>stream_batch_hits){receiver->malformed=true;return false;}
    if(receiver->truncated||receiver->malformed||receiver->untrusted_batch)return false;
    // Fallback scenes may receive a remaining subspan. Never read an original
    // allocator's unknown buffer when fallback has exhausted the caller batch.
    const auto base_address=reinterpret_cast<std::uintptr_t>(receiver->batch);
    const auto data_address=reinterpret_cast<std::uintptr_t>(values);
    constexpr auto extent=std::size_t(stream_batch_hits)*sizeof(std::uintptr_t);
    auto disjoint=[](std::uintptr_t a,std::size_t an,std::uintptr_t b,std::size_t bn){return a<=b?b-a>=an:a-b>=bn;};
    const auto result_address=reinterpret_cast<std::uintptr_t>(receiver->result);
    const auto receiver_address=reinterpret_cast<std::uintptr_t>(receiver);
    if(!disjoint(result_address,sizeof(AllOverlap),base_address,extent)||
       !disjoint(result_address,sizeof(AllOverlap),receiver_address,sizeof(OverlapReporter))||
       !disjoint(receiver_address,sizeof(OverlapReporter),base_address,extent)){
        receiver->malformed=true;return false;
    }
    if(!base_address||base_address>UINTPTR_MAX-extent||data_address<base_address||
       data_address-base_address>extent||(data_address-base_address)%sizeof(std::uintptr_t)||
       std::size_t(count)*sizeof(std::uintptr_t)>extent-(data_address-base_address)){
        receiver->untrusted_batch=true;return false;
    }
    auto& packet=*receiver->result;
    if(packet.shape_count>receiver->limits.max_hits){receiver->malformed=true;return false;}
    if(packet.reports>=receiver->limits.max_reports||count>receiver->limits.max_hits-packet.shape_count){receiver->truncated=true;return false;}
    ++packet.reports;
    if(count)std::memcpy(packet.shapes.data()+packet.shape_count,values,std::size_t(count)*sizeof(std::uintptr_t));
    packet.shape_count+=count;return true;
}
inline constexpr std::array<OverlapFunction,1> overlap_vtable{{overlap_report}};
class OverlapScope {
    OverlapReporter* owner_{};
public:
    explicit OverlapScope(OverlapReporter& receiver)noexcept{
        if(!active_query_reporter&&!active_overlap_reporter){owner_=&receiver;active_overlap_reporter=owner_;active_query_reporter=owner_;}
    }
    OverlapScope(OverlapScope const&)=delete;OverlapScope& operator=(OverlapScope const&)=delete;
    bool admitted()const noexcept{return owner_!=nullptr;}
    void close()noexcept{
        if(owner_){if(active_overlap_reporter==owner_)active_overlap_reporter=nullptr;else owner_->malformed=true;
            if(active_query_reporter==owner_)active_query_reporter=nullptr;else owner_->malformed=true;owner_=nullptr;}
    }
    ~OverlapScope(){close();}
};
// No production caller yet: original entry/consumer certificates and query
// source closure must precede invocation. A successful return alone says
// nothing about callback completion, ownership or endpoint occupancy.
template<class Shape> bool overlap_shapes(std::uintptr_t function,std::uintptr_t public_scene,Shape const& geometry,
    std::uintptr_t* batch,OverlapReporter& receiver,std::uint32_t& returned)noexcept{
    using Function=std::uint32_t(__fastcall*)(std::uintptr_t,Shape const*,std::uint32_t,std::uint32_t,
        std::uintptr_t*,void*,std::uint32_t,void const*,bool);
    __try{returned=reinterpret_cast<Function>(function)(public_scene,&geometry,3,stream_batch_hits,batch,&receiver,UINT32_MAX,nullptr,true);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
}
