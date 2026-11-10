#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <bcrypt.h>
#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <string>
#include <vector>
#include "cover_physx_query_source.h"
#include "cover_physx_stream.h"
#include "cover_physx_shape_owner.h"
#include "cover_physx_asset_actors.h"
#include "cover_physx_collider.h"
#include "cover_physx_components.h"
#include "cover_physx_self_evidence.h"

// Synchronous ORIGINAL scene-query adapter. prime() belongs to Native startup,
// outside DllMain and game callbacks. Query callers must own the admitted C0/GT
// source and hold no private lock across PhysX. The guard certifies that source
// and original F26D4 layouts; it is checked before/after the real query.
// Results are raw observations. No miss, repeated source, or returned hit is
// a world fence, complete self-exclusion, movement or occupancy permission.
namespace gears_cover_physx_query {
namespace source=gears_cover_physx_query_source;
using Digest=source::Digest;
struct Vector {float x{},y{},z{};};
struct Sphere {Vector centre{};float radius{};};
struct Box {Vector centre{},half{};float rotation[9]{};};
struct Capsule {Vector first{},second{};float radius{};};
static_assert(sizeof(Vector)==12&&sizeof(Sphere)==16&&offsetof(Sphere,radius)==12&&sizeof(Box)==60&&sizeof(Capsule)==28&&sizeof(Hit)==64&&alignof(Hit)==8);
static_assert(offsetof(Box,centre)==0&&offsetof(Box,half)==12&&offsetof(Box,rotation)==24&&
    offsetof(Capsule,first)==0&&offsetof(Capsule,second)==12&&offsetof(Capsule,radius)==24);
enum class Code {queried,unavailable,request,reentrant,source,code,changed,fault_unknown,truncated,callback_rejected,
    owner_read,owner_unsupported,owner_links};
struct Overlap {bool raw_hit{};};
struct Sweep {std::uint32_t raw_count{},reserved{};Hit raw_hit{};};
enum class QueryStage:unsigned {idle,source_begin,code_begin,source_repeat,source_guard,
    dispatch,returned,source_end,code_end,guard_end,complete};
// Owner-thread observation only, read after the serialized query returns.
// Counts surround Calls, not an inferred hit/miss or body permission.
struct QueryProgress {bool supported{},exception{};QueryStage stage{};unsigned source_code{};
    std::uint64_t original_started{},original_returned{};};
// Every raw hit is retained in order, with its observed public actor. This
// neither identifies the complete player self set nor filters any hit.
struct AllSweepOwners {
    AllSweep raw{};
    std::array<gears_cover_physx_shape_owner::Observation,max_stream_hits> owners{};
    // Positive per-shape provenance only. Zero MUST remain an obstacle or
    // unknown; neither the array nor a fully self-marked sweep is clearance.
    std::array<unsigned char,max_stream_hits> self_sources{};
    gears_cover_physx_asset_actors::Snapshot asset_bodies{};
    gears_cover_physx_collider::Snapshot collider_source{};
    gears_cover_physx_components::Snapshot component_sources{};
};
namespace detail {
inline bool digest(void const* bytes,std::size_t n,Digest& out) noexcept {
    if(!bytes||n>ULONG_MAX)return false;
    BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};
    bool ok=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(n),0)>=0;
    if(ok)ok=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);return ok;
}
inline bool scalar(float v) noexcept{return std::isfinite(v)&&(v==0||std::isnormal(v));}
inline bool finite(Vector v) noexcept{return scalar(v.x)&&scalar(v.y)&&scalar(v.z);}
inline bool motion(Vector v) noexcept {
    if(!finite(v))return false;
    // The original method normalizes using float squared length. Finite input
    // alone can still overflow or underflow that calculation into NaN/zero.
    const float xx=v.x*v.x,yy=v.y*v.y,zz=v.z*v.z;
    const float length2=(xx+yy)+zz;
    return std::isnormal(length2)&&length2>0;
}
inline bool valid(Sphere const& s) noexcept {
    if(!finite(s.centre)||!scalar(s.radius)||s.radius<=0)return false;
    const float centre[]{s.centre.x,s.centre.y,s.centre.z};
    for(float c:centre)if(std::abs(double(c))+s.radius>(std::numeric_limits<float>::max)())return false;
    return true;
}
template<class Guard> bool source_guard(Guard& guard,source::Snapshot const& snapshot){
    if constexpr(std::is_invocable_r_v<bool,Guard&,source::Snapshot const&>)return guard(snapshot);
    else return guard();
}
inline bool valid(Box const& b) noexcept {
    if(!finite(b.centre)||!finite(b.half)||b.half.x<=0||b.half.y<=0||b.half.z<=0)return false;
    for(float v:b.rotation)if(!scalar(v))return false;
    for(unsigned r=0;r<3;++r)for(unsigned c=r;c<3;++c){double dot=0;
        for(unsigned k=0;k<3;++k)dot+=double(b.rotation[3*r+k])*b.rotation[3*c+k];
        if(std::abs(dot-(r==c?1.:0.))>3e-5)return false;}
    auto const* m=b.rotation;
    const double det=double(m[0])*(double(m[4])*m[8]-double(m[5])*m[7])-
        double(m[1])*(double(m[3])*m[8]-double(m[5])*m[6])+
        double(m[2])*(double(m[3])*m[7]-double(m[4])*m[6]);
    if(std::abs(det-1.)>3e-5)return false;
    const double centres[]{b.centre.x,b.centre.y,b.centre.z},halves[]{b.half.x,b.half.y,b.half.z};
    for(unsigned r=0;r<3;++r){double extent=0;
        for(unsigned k=0;k<3;++k)extent+=std::abs(double(b.rotation[3*r+k]))*halves[k];
        if(std::abs(centres[r])+extent>(std::numeric_limits<float>::max)())return false;}
    return true;
}
inline bool valid(Capsule const& c) noexcept {
    // A sphere must use its authenticated own path. Do not silently reinterpret
    // a zero segment before the original capsule conversion is authenticated.
    if(!finite(c.first)||!finite(c.second)||!scalar(c.radius)||c.radius<=0||
       !motion({c.second.x-c.first.x,c.second.y-c.first.y,c.second.z-c.first.z}))return false;
    const float first[]{c.first.x,c.first.y,c.first.z},second[]{c.second.x,c.second.y,c.second.z};
    for(unsigned i=0;i<3;++i){
        // Original 1A5740 adds float endpoints BEFORE multiplying by 0.5.
        // Individually finite endpoints and a valid axis do not bound this
        // intermediate. Never enter the native broadphase with an Inf centre.
        const float sum=first[i]+second[i];
        const float midpoint=sum*.5f;
        if(!scalar(sum)||!scalar(midpoint)||
           (std::max)(std::abs(double(first[i])),std::abs(double(second[i])))+c.radius>
           (std::numeric_limits<float>::max)())return false;
    }
    return true;
}
inline bool sweep_motion(Box const&,Vector delta) noexcept {return motion(delta);}
inline bool sweep_motion(Capsule const&,Vector delta) noexcept {
    if(!motion(delta))return false;
    // Original D7E30's vertex branch rejects candidate distances above the
    // authenticated 100-unit constant even if the public cast is longer.
    // Reject the entire out-of-domain request, never clamp it or report miss.
    // This bound is on translation, NOT the capsule's own segment length.
    constexpr double limit=gears_cover_physx_query_code::capsule_vertex_limit;
    const double length2=double(delta.x)*delta.x+double(delta.y)*delta.y+double(delta.z)*delta.z;
    const float xx=delta.x*delta.x,yy=delta.y*delta.y,zz=delta.z*delta.z;
    return length2<=limit*limit&&(xx+yy)+zz<=float(limit*limit);
}
template<class Shape> bool overlap(std::uintptr_t fn,std::uintptr_t scene,Shape const& shape,bool& result) noexcept {
    using Function=bool(__fastcall*)(std::uintptr_t,Shape const*,std::uint32_t,std::uint32_t,void const*);
    __try {result=reinterpret_cast<Function>(fn)(scene,&shape,3,UINT32_MAX,nullptr);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class Shape> bool sweep(std::uintptr_t fn,std::uintptr_t scene,Shape const& shape,Vector const& delta,Sweep& result) noexcept {
    // Public arguments1..10. Flags3 select original static+dynamic closest-hit
    // mode. No streaming callback/self-exclusion is claimed by this operation.
    using Function=std::uint32_t(__fastcall*)(std::uintptr_t,Shape const*,Vector const*,std::uint32_t,
        void*,std::uint32_t,Hit*,void*,std::uint32_t,void const*);
    __try {result.raw_count=reinterpret_cast<Function>(fn)(scene,&shape,&delta,3,nullptr,1,&result.raw_hit,nullptr,UINT32_MAX,nullptr);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class Shape> bool sweep_stream(std::uintptr_t fn,std::uintptr_t scene,Shape const& shape,Vector const& delta,
    Hit* batch,StreamReporter& reporter,std::uint32_t& count)noexcept{
    using Function=std::uint32_t(__fastcall*)(std::uintptr_t,Shape const*,Vector const*,std::uint32_t,
        void*,std::uint32_t,Hit*,void*,std::uint32_t,void const*);
    // Flags11 = original static+dynamic+all-hit branch. Caller owns the batch;
    // supplying it avoids the original dynamic-stack/allocator fallback.
    __try {count=reinterpret_cast<Function>(fn)(scene,&shape,&delta,11,nullptr,stream_batch_hits,batch,&reporter,UINT32_MAX,nullptr);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
class Image {
    HMODULE module_{};HANDLE file_{INVALID_HANDLE_VALUE};std::uintptr_t base_{};std::uint32_t size_{};
    bool retained_{};
public:
    Image()=default;Image(Image const&)=delete;Image& operator=(Image const&)=delete;
    ~Image(){if(!retained_)close();}
    std::uintptr_t base()const noexcept{return base_;}
    std::uint32_t size()const noexcept{return size_;}
    // After an SEH fault the original call may still own a PhysX lock. Even an
    // erroneous owner teardown must not unload its code or release its file
    // pin. These handles deliberately remain for the process lifetime. The
    // owner must report pending() and keep ordinary shutdown unacknowledged.
    void retain_for_process() noexcept {retained_=true;}
    void close() noexcept {if(retained_)return;if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);if(module_)FreeLibrary(module_);
        file_=INVALID_HANDLE_VALUE;module_=nullptr;base_=0;size_=0;}
    bool prime() noexcept {if(module_)return true;try{
        // Acquire an existing module only. Never LoadLibrary or start a target.
        if(!GetModuleHandleExW(0,L"PhysXCore.dll",&module_))return false;
        std::vector<wchar_t> actual(32768),exe(32768);
        const auto n=GetModuleFileNameW(module_,actual.data(),DWORD(actual.size()));
        const auto k=GetModuleFileNameW(nullptr,exe.data(),DWORD(exe.size()));
        if(!n||n>=actual.size()-1||!k||k>=exe.size()-1){close();return false;}
        std::wstring parent(exe.data());const auto slash=parent.find_last_of(L"\\/");
        if(slash==std::wstring::npos){close();return false;}
        const auto expected=parent.substr(0,slash)+L"\\PhysXCore.dll";
        if(_wcsicmp(expected.c_str(),actual.data())){close();return false;}
        file_=CreateFileW(actual.data(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file_==INVALID_HANDLE_VALUE){close();return false;}
        LARGE_INTEGER length{};if(!GetFileSizeEx(file_,&length)||length.QuadPart!=gears_cover_physx_query_code::file_bytes){close();return false;}
        std::vector<unsigned char> bytes(std::size_t(length.QuadPart));DWORD got{};Digest d{};
        if(!ReadFile(file_,bytes.data(),DWORD(bytes.size()),&got,nullptr)||got!=bytes.size()||
           !detail::digest(bytes.data(),bytes.size(),d)||d!=gears_cover_physx_query_code::image_sha256){close();return false;}
        MODULEINFO info{};
        if(!GetModuleInformation(GetCurrentProcess(),module_,&info,sizeof info)||info.lpBaseOfDll!=module_||!info.SizeOfImage){close();return false;}
        base_=reinterpret_cast<std::uintptr_t>(module_);size_=info.SizeOfImage;return true;
    }catch(...){close();return false;}}
};
namespace detail {
struct OriginalCalls {
    template<class Shape>static bool overlap(std::uintptr_t fn,std::uintptr_t scene,Shape const& shape,bool& result)noexcept{
        return gears_cover_physx_query::detail::overlap(fn,scene,shape,result);}
    template<class Shape>static bool sweep(std::uintptr_t fn,std::uintptr_t scene,Shape const& shape,Vector const& delta,Sweep& result)noexcept{
        return gears_cover_physx_query::detail::sweep(fn,scene,shape,delta,result);}
    template<class Shape>static bool sweep_stream(std::uintptr_t fn,std::uintptr_t scene,Shape const& shape,Vector const& delta,
        Hit* batch,StreamReporter& reporter,std::uint32_t& count)noexcept{
        return gears_cover_physx_query::detail::sweep_stream(fn,scene,shape,delta,batch,reporter,count);}
};
inline Code owner_code(gears_cover_physx_shape_owner::Code value)noexcept{
    namespace owner=gears_cover_physx_shape_owner;
    switch(value){
    case owner::Code::observed:return Code::queried;
    case owner::Code::request:return Code::request;
    case owner::Code::guard:return Code::source;
    case owner::Code::source_changed:case owner::Code::changed:return Code::changed;
    case owner::Code::read:return Code::owner_read;
    case owner::Code::unsupported:return Code::owner_unsupported;
    case owner::Code::module:return Code::code;
    case owner::Code::links:return Code::owner_links;
    }return Code::source;
}
inline Code asset_code(gears_cover_physx_asset_actors::Code value)noexcept{
    namespace asset=gears_cover_physx_asset_actors;
    switch(value){
    case asset::Code::observed:return Code::queried;
    case asset::Code::request:return Code::request;
    case asset::Code::guard:return Code::source;
    case asset::Code::source_changed:case asset::Code::changed:return Code::changed;
    case asset::Code::read:return Code::owner_read;
    case asset::Code::unsupported:return Code::owner_unsupported;
    case asset::Code::links:return Code::owner_links;
    }return Code::source;
}
inline Code collider_code(gears_cover_physx_collider::Code value)noexcept{
    namespace cx=gears_cover_physx_collider;
    switch(value){
    case cx::Code::observed:return Code::queried;
    case cx::Code::request:return Code::request;
    case cx::Code::guard:return Code::source;
    case cx::Code::source_changed:case cx::Code::changed:return Code::changed;
    case cx::Code::read:return Code::owner_read;
    case cx::Code::unsupported:return Code::owner_unsupported;
    case cx::Code::links:return Code::owner_links;
    }return Code::source;
}
inline Code components_code(gears_cover_physx_components::Code value)noexcept{
    namespace p=gears_cover_physx_components;
    switch(value){
    case p::Code::observed:return Code::queried;
    case p::Code::request:return Code::request;
    case p::Code::guard:return Code::source;
    case p::Code::source_changed:case p::Code::changed:return Code::changed;
    case p::Code::read:return Code::owner_read;
    case p::Code::unsupported:return Code::owner_unsupported;
    case p::Code::links:return Code::owner_links;
    }return Code::source;
}
}
// Production binds the original Image and original calls below. Explicit Host
// substitutes can exercise this SAME transaction without executing the DLL.
template<class ImageType,class Calls> class QueryEngine {
    ImageType image_{};std::atomic<bool> active_{},uncertain_{};
    QueryProgress progress_{true};
    source::Snapshot before_{},after_{};
    AllSweep stream_candidate_{};
    std::array<gears_cover_physx_shape_owner::Observation,max_stream_hits> owners_candidate_{};
    gears_cover_physx_asset_actors::Workspace asset_work_{};
    gears_cover_physx_asset_actors::Snapshot asset_before_{},asset_after_{};
    gears_cover_physx_collider::Snapshot collider_before_{},collider_after_{};
    gears_cover_physx_components::Source component_key_{};
    gears_cover_physx_components::Workspace component_work_{};
    gears_cover_physx_components::Snapshot components_before_{},components_after_{};
    void fault() noexcept {uncertain_=true;image_.retain_for_process();}
    struct Finish {std::atomic<bool>& value;~Finish(){value=false;}};
    template<class Read,class Guard> Code begin(source::Source const& s,Read read,Guard guard) {
        progress_.exception=false;progress_.stage=QueryStage::source_begin;progress_.source_code=0;
        if(!image_.base()||uncertain_.load())return Code::unavailable;
        if(s.thread!=GetCurrentThreadId())return Code::source;
        const auto first=source::capture(image_.base(),image_.size(),s,read,guard,before_);progress_.source_code=unsigned(first);
        if(first!=source::Code::current)return Code::source;
        progress_.stage=QueryStage::code_begin;
        if(!source::code_current(image_.base(),image_.size(),read,detail::digest))return Code::code;
        progress_.stage=QueryStage::source_repeat;
        const auto repeat=source::capture(image_.base(),image_.size(),s,read,guard,after_);progress_.source_code=unsigned(repeat);
        if(repeat!=source::Code::current||
           !source::same(before_,after_))return Code::changed;
        progress_.stage=QueryStage::source_guard;
        return detail::source_guard(guard,before_)?Code::queried:Code::source;
    }
    template<class Read,class Guard> Code end(source::Source const& s,Read read,Guard guard) {
        progress_.stage=QueryStage::source_end;
        const auto last=source::capture(image_.base(),image_.size(),s,read,guard,after_);progress_.source_code=unsigned(last);
        if(last!=source::Code::current||
           !source::same(before_,after_))return Code::changed;
        progress_.stage=QueryStage::code_end;
        if(!source::code_current(image_.base(),image_.size(),read,detail::digest))return Code::code;
        progress_.stage=QueryStage::guard_end;
        return detail::source_guard(guard,after_)?Code::queried:Code::changed;
    }
public:
    QueryEngine()=default;QueryEngine(QueryEngine const&)=delete;QueryEngine& operator=(QueryEngine const&)=delete;
    // prime/close are owner-serialized startup/stop operations. A query fault
    // may have abandoned an original lock: no automatic unlock/reset/retry.
    bool prime() noexcept {return !active_.load()&&!uncertain_.load()&&image_.prime();}
    bool close() noexcept {if(pending())return false;image_.close();return true;}
    bool pending()const noexcept{return active_.load()||uncertain_.load();}
    bool uncertain()const noexcept{return uncertain_.load();}
    QueryProgress observation()const noexcept{return progress_;}
    template<class Shape,class Read,class Guard> Code overlap(source::Source const& s,Shape const& shape,
        Read read,Guard guard,Overlap& output) noexcept {
        static_assert(std::is_same<Shape,Box>::value||std::is_same<Shape,Capsule>::value||std::is_same_v<Shape,Sphere>);
        if(source::aliases(&output,sizeof output,&s,sizeof s)||source::aliases(&output,sizeof output,&shape,sizeof shape)||
           source::aliases(&output,sizeof output,this,sizeof *this))return Code::request;
        const source::Source request=s;const Shape geometry=shape;
        if(!detail::valid(shape))return Code::request;
        bool expected=false;if(!active_.compare_exchange_strong(expected,true))return Code::reentrant;Finish finish{active_};
        try{const auto ready=begin(request,read,guard);if(ready!=Code::queried)return ready;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape))return Code::changed;
            // Sphere has its own original overlap entry. Do not feed a
            // zero-axis capsule to the separate sweep/overlap paths.
            Overlap result{};constexpr auto rva=std::is_same_v<Shape,Sphere>?0x1683d0:std::is_same<Shape,Box>::value?0x168350:0x1682d0;
            progress_.stage=QueryStage::dispatch;++progress_.original_started;
            if(!Calls::overlap(image_.base()+rva,before_.public_scene,geometry,result.raw_hit)){fault();return Code::fault_unknown;}
            ++progress_.original_returned;progress_.stage=QueryStage::returned;
            const auto closed=end(request,read,guard);if(closed!=Code::queried)return closed;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape))return Code::changed;
            output=result;progress_.stage=QueryStage::complete;return Code::queried;
        }catch(...){progress_.exception=true;return Code::source;}
    }
    template<class Shape,class Read,class Guard> Code sweep(source::Source const& s,Shape const& shape,Vector const& delta,
        Read read,Guard guard,Sweep& output) noexcept {
        static_assert(std::is_same<Shape,Box>::value||std::is_same<Shape,Capsule>::value);
        if(source::aliases(&output,sizeof output,&s,sizeof s)||source::aliases(&output,sizeof output,&shape,sizeof shape)||
           source::aliases(&output,sizeof output,&delta,sizeof delta)||source::aliases(&output,sizeof output,this,sizeof *this))return Code::request;
        const source::Source request=s;const Shape geometry=shape;const Vector movement=delta;
        if(!detail::valid(shape)||!detail::sweep_motion(geometry,movement))return Code::request;
        bool expected=false;if(!active_.compare_exchange_strong(expected,true))return Code::reentrant;Finish finish{active_};
        try{const auto ready=begin(request,read,guard);if(ready!=Code::queried)return ready;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta))return Code::changed;
            Sweep result{};constexpr auto rva=std::is_same<Shape,Box>::value?0x171770:0x1716f0;
            progress_.stage=QueryStage::dispatch;++progress_.original_started;
            if(!Calls::sweep(image_.base()+rva,before_.public_scene,geometry,movement,result)){fault();return Code::fault_unknown;}
            ++progress_.original_returned;progress_.stage=QueryStage::returned;
            if(result.raw_count>1){fault();return Code::fault_unknown;}
            const auto closed=end(request,read,guard);if(closed!=Code::queried)return closed;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta))return Code::changed;
            output=result;progress_.stage=QueryStage::complete;return Code::queried;
        }catch(...){progress_.exception=true;return Code::source;}
    }
    template<class Shape,class Read,class Guard,class Output> Code sweep_all(source::Source const& s,Shape const& shape,Vector const& delta,
        StreamLimits const& limits,Read read,Guard guard,Output& output,gears_cover_physx_collider::Source const* component_source=nullptr,
        gears_cover_physx_components::Source const* component_catalog=nullptr)noexcept{
        static_assert(std::is_same_v<Shape,Box>||std::is_same_v<Shape,Capsule>);
        static_assert(std::is_same_v<Output,AllSweep>||std::is_same_v<Output,AllSweepOwners>);
        if(source::aliases(&output,sizeof output,this,sizeof *this)||source::aliases(&output,sizeof output,&s,sizeof s)||
           source::aliases(&output,sizeof output,&shape,sizeof shape)||source::aliases(&output,sizeof output,&delta,sizeof delta)||
           source::aliases(&output,sizeof output,&limits,sizeof limits))return Code::request;
        if(component_source&&(source::aliases(component_source,sizeof *component_source,this,sizeof *this)||source::aliases(component_source,sizeof *component_source,&output,sizeof output)))return Code::request;
        if(component_catalog&&(source::aliases(component_catalog,sizeof *component_catalog,this,sizeof *this)||source::aliases(component_catalog,sizeof *component_catalog,&output,sizeof output)))return Code::request;
        if constexpr(std::is_same_v<Output,AllSweep>){if(component_source||component_catalog)return Code::request;}
        const auto request=s;const Shape geometry=shape;const Vector movement=delta;const auto budget=limits;
        const auto collider_key=component_source?*component_source:gears_cover_physx_collider::Source{};
        if(!detail::valid(geometry)||!detail::sweep_motion(geometry,movement)||!detail::stream_limits(budget))return Code::request;
        if constexpr(std::is_same_v<Output,AllSweepOwners> &&
            (!std::is_invocable_r_v<bool,Guard&,gears_cover_physx_asset_actors::Snapshot const&>||
             !std::is_invocable_r_v<bool,Guard&,gears_cover_physx_collider::Snapshot const&>))return Code::source;
        if constexpr(std::is_same_v<Output,AllSweepOwners> && !std::is_invocable_r_v<bool,Guard&,gears_cover_physx_components::Snapshot const&>)return Code::source;
        if constexpr(std::is_same_v<Output,AllSweepOwners>){if(!component_source||!component_catalog)return Code::source;}
        if(detail::active_stream_reporter)return Code::reentrant;
        bool expected=false;if(!active_.compare_exchange_strong(expected,true))return Code::reentrant;Finish finish{active_};
        if constexpr(std::is_same_v<Output,AllSweepOwners>)component_key_=*component_catalog;
        try{const auto ready=begin(request,read,guard);if(ready!=Code::queried)return ready;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta)||
               std::memcmp(&budget,&limits,sizeof limits))return Code::changed;
            struct AssetGuard {
                Guard& caller;source::Snapshot const& scope;
                bool operator()(){return detail::source_guard(caller,scope);}
                bool operator()(gears_cover_physx_asset_actors::Snapshot const& actors){
                    if constexpr(std::is_invocable_r_v<bool,Guard&,gears_cover_physx_asset_actors::Snapshot const&>)
                        return source::same(scope,actors.query_source)&&caller(actors);
                    else return false;
                }
                bool operator()(gears_cover_physx_collider::Snapshot const& value){
                    if constexpr(std::is_invocable_r_v<bool,Guard&,gears_cover_physx_collider::Snapshot const&>)
                        return source::same(scope,value.query_source)&&caller(value);
                    else return false;
                }
                bool operator()(gears_cover_physx_components::Snapshot const& value){
                    if constexpr(std::is_invocable_r_v<bool,Guard&,gears_cover_physx_components::Snapshot const&>)
                        return source::same(scope,value.query_source)&&caller(value);
                    else return false;
                }
            } asset_guard{guard,before_};
            if constexpr(std::is_same_v<Output,AllSweepOwners>){
                const auto code=gears_cover_physx_asset_actors::observe(image_.base(),image_.size(),before_,read,asset_guard,asset_work_,asset_before_);
                if(code!=gears_cover_physx_asset_actors::Code::observed)return detail::asset_code(code);
                const auto cx=gears_cover_physx_collider::observe(image_.base(),image_.size(),before_,*component_source,read,asset_guard,collider_before_);
                if(cx!=gears_cover_physx_collider::Code::observed)return detail::collider_code(cx);
                const auto parts=gears_cover_physx_components::observe(image_.base(),image_.size(),before_,*component_catalog,read,asset_guard,component_work_,components_before_);
                if(parts!=gears_cover_physx_components::Code::observed)return detail::components_code(parts);
            }
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta)||
               std::memcmp(&budget,&limits,sizeof limits))return Code::changed;
            if constexpr(std::is_same_v<Output,AllSweepOwners>){if(!gears_cover_physx_collider::same(collider_key,*component_source)||
                !gears_cover_physx_components::same(component_key_,*component_catalog))return Code::changed;}
            stream_candidate_={};std::array<Hit,stream_batch_hits> batch{};
            detail::StreamReporter reporter{detail::stream_vtable.data(),&stream_candidate_,batch.data(),GetCurrentThreadId(),budget};
            detail::StreamScope callback(reporter);if(!callback.admitted())return Code::reentrant;
            std::uint32_t raw_count{};constexpr auto rva=std::is_same_v<Shape,Box>?0x171770:0x1716f0;
            progress_.stage=QueryStage::dispatch;++progress_.original_started;
            const bool returned=Calls::sweep_stream(image_.base()+rva,before_.public_scene,geometry,movement,batch.data(),reporter,raw_count);
            callback.close();if(!returned){fault();return Code::fault_unknown;}
            ++progress_.original_returned;progress_.stage=QueryStage::returned;
            const auto closed=end(request,read,guard);if(closed!=Code::queried)return closed;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta)||
               std::memcmp(&budget,&limits,sizeof limits))return Code::changed;
            if(reporter.malformed)return Code::callback_rejected;
            if(reporter.truncated)return Code::truncated;
            if(raw_count!=stream_candidate_.hit_count)return Code::callback_rejected;
            stream_candidate_.raw_count=raw_count;
            if constexpr(std::is_same_v<Output,AllSweepOwners>){
                // The active query and Image pin span enumeration AND all
                // owner reads. No second query or stream filtering occurs.
                std::fill(owners_candidate_.begin(),owners_candidate_.end(),gears_cover_physx_shape_owner::Observation{});
                auto current_owner=[&](source::Snapshot const& raw){return source::same(before_,raw)&&detail::source_guard(guard,raw);};
                for(unsigned i=0;i<stream_candidate_.hit_count;++i){
                    const auto owner=gears_cover_physx_shape_owner::observe(image_.base(),image_.size(),before_,
                        stream_candidate_.hits[i],read,current_owner,owners_candidate_[i]);
                    if(owner!=gears_cover_physx_shape_owner::Code::observed)return detail::owner_code(owner);
                }
                const auto asset_closed=gears_cover_physx_asset_actors::observe(image_.base(),image_.size(),before_,read,asset_guard,asset_work_,asset_after_);
                if(asset_closed!=gears_cover_physx_asset_actors::Code::observed)return detail::asset_code(asset_closed);
                if(!gears_cover_physx_asset_actors::same(asset_before_,asset_after_))return Code::changed;
                const auto cx=gears_cover_physx_collider::observe(image_.base(),image_.size(),before_,*component_source,read,asset_guard,collider_after_);
                if(cx!=gears_cover_physx_collider::Code::observed)return detail::collider_code(cx);
                if(!gears_cover_physx_collider::same(collider_before_,collider_after_))return Code::changed;
                const auto parts=gears_cover_physx_components::observe(image_.base(),image_.size(),before_,*component_catalog,read,asset_guard,component_work_,components_after_);
                if(parts!=gears_cover_physx_components::Code::observed)return detail::components_code(parts);
                if(!gears_cover_physx_components::same(components_before_,components_after_))return Code::changed;
                const auto owner_closed=end(request,read,guard);if(owner_closed!=Code::queried)return owner_closed;
                if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta)||
                   std::memcmp(&budget,&limits,sizeof limits)||!gears_cover_physx_collider::same(collider_key,*component_source)||
                   !gears_cover_physx_components::same(component_key_,*component_catalog))return Code::changed;
                output.raw=stream_candidate_;output.owners=owners_candidate_;output.asset_bodies=asset_after_;output.collider_source=collider_after_;output.component_sources=components_after_;
                output.self_sources.fill(0);
                if(gears_cover_physx_self_evidence::qualified(asset_after_,collider_after_,components_after_))
                    for(unsigned i=0;i<stream_candidate_.hit_count;++i)
                        output.self_sources[i]=gears_cover_physx_self_evidence::match(owners_candidate_[i],asset_after_,collider_after_,components_after_);
            }else output=stream_candidate_;
            progress_.stage=QueryStage::complete;return Code::queried;
        }catch(...){progress_.exception=true;return Code::source;}
    }
};
using Queries=QueryEngine<Image,detail::OriginalCalls>;
} // namespace gears_cover_physx_query
