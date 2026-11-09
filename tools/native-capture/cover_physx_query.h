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
struct Box {Vector centre{},half{};float rotation[9]{};};
struct Capsule {Vector first{},second{};float radius{};};
struct alignas(8) Hit {std::array<unsigned char,64> bytes{};};
static_assert(sizeof(Vector)==12&&sizeof(Box)==60&&sizeof(Capsule)==28&&sizeof(Hit)==64&&alignof(Hit)==8);
static_assert(offsetof(Box,centre)==0&&offsetof(Box,half)==12&&offsetof(Box,rotation)==24&&
    offsetof(Capsule,first)==0&&offsetof(Capsule,second)==12&&offsetof(Capsule,radius)==24);
enum class Code {queried,unavailable,request,reentrant,source,code,changed,fault_unknown};
struct Overlap {bool raw_hit{};};
struct Sweep {std::uint32_t raw_count{},reserved{};Hit raw_hit{};};
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
    for(unsigned i=0;i<3;++i)if((std::max)(std::abs(double(first[i])),std::abs(double(second[i])))+c.radius>
        (std::numeric_limits<float>::max)())return false;
    return true;
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
class Queries {
    Image image_{};std::atomic<bool> active_{},uncertain_{};
    source::Snapshot before_{},after_{};
    void fault() noexcept {uncertain_=true;image_.retain_for_process();}
    struct Finish {std::atomic<bool>& value;~Finish(){value=false;}};
    template<class Read,class Guard> Code begin(source::Source const& s,Read read,Guard guard) {
        if(!image_.base()||uncertain_.load())return Code::unavailable;
        if(s.thread!=GetCurrentThreadId())return Code::source;
        if(source::capture(image_.base(),image_.size(),s,read,guard,before_)!=source::Code::current)return Code::source;
        if(!source::code_current(image_.base(),image_.size(),read,detail::digest))return Code::code;
        if(source::capture(image_.base(),image_.size(),s,read,guard,after_)!=source::Code::current||
           !source::same(before_,after_))return Code::changed;
        return detail::source_guard(guard,before_)?Code::queried:Code::source;
    }
    template<class Read,class Guard> Code end(source::Source const& s,Read read,Guard guard) {
        if(source::capture(image_.base(),image_.size(),s,read,guard,after_)!=source::Code::current||
           !source::same(before_,after_))return Code::changed;
        if(!source::code_current(image_.base(),image_.size(),read,detail::digest))return Code::code;
        return detail::source_guard(guard,after_)?Code::queried:Code::changed;
    }
public:
    Queries()=default;Queries(Queries const&)=delete;Queries& operator=(Queries const&)=delete;
    // prime/close are owner-serialized startup/stop operations. A query fault
    // may have abandoned an original lock: no automatic unlock/reset/retry.
    bool prime() noexcept {return !active_.load()&&!uncertain_.load()&&image_.prime();}
    bool close() noexcept {if(pending())return false;image_.close();return true;}
    bool pending()const noexcept{return active_.load()||uncertain_.load();}
    bool uncertain()const noexcept{return uncertain_.load();}
    template<class Shape,class Read,class Guard> Code overlap(source::Source const& s,Shape const& shape,
        Read read,Guard guard,Overlap& output) noexcept {
        static_assert(std::is_same<Shape,Box>::value||std::is_same<Shape,Capsule>::value);
        if(source::aliases(&output,sizeof output,&s,sizeof s)||source::aliases(&output,sizeof output,&shape,sizeof shape)||
           source::aliases(&output,sizeof output,this,sizeof *this))return Code::request;
        const source::Source request=s;const Shape geometry=shape;
        if(!detail::valid(shape))return Code::request;
        bool expected=false;if(!active_.compare_exchange_strong(expected,true))return Code::reentrant;Finish finish{active_};
        try{const auto ready=begin(request,read,guard);if(ready!=Code::queried)return ready;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape))return Code::changed;
            Overlap result{};constexpr auto rva=std::is_same<Shape,Box>::value?0x168350:0x1682d0;
            if(!detail::overlap(image_.base()+rva,before_.public_scene,geometry,result.raw_hit)){fault();return Code::fault_unknown;}
            const auto closed=end(request,read,guard);if(closed!=Code::queried)return closed;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape))return Code::changed;
            output=result;return Code::queried;
        }catch(...){return Code::source;}
    }
    template<class Shape,class Read,class Guard> Code sweep(source::Source const& s,Shape const& shape,Vector const& delta,
        Read read,Guard guard,Sweep& output) noexcept {
        static_assert(std::is_same<Shape,Box>::value||std::is_same<Shape,Capsule>::value);
        if(source::aliases(&output,sizeof output,&s,sizeof s)||source::aliases(&output,sizeof output,&shape,sizeof shape)||
           source::aliases(&output,sizeof output,&delta,sizeof delta)||source::aliases(&output,sizeof output,this,sizeof *this))return Code::request;
        const source::Source request=s;const Shape geometry=shape;const Vector movement=delta;
        if(!detail::valid(shape)||!detail::motion(delta))return Code::request;
        bool expected=false;if(!active_.compare_exchange_strong(expected,true))return Code::reentrant;Finish finish{active_};
        try{const auto ready=begin(request,read,guard);if(ready!=Code::queried)return ready;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta))return Code::changed;
            Sweep result{};constexpr auto rva=std::is_same<Shape,Box>::value?0x171770:0x1716f0;
            if(!detail::sweep(image_.base()+rva,before_.public_scene,geometry,movement,result)){fault();return Code::fault_unknown;}
            if(result.raw_count>1){fault();return Code::fault_unknown;}
            const auto closed=end(request,read,guard);if(closed!=Code::queried)return closed;
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta))return Code::changed;
            output=result;return Code::queried;
        }catch(...){return Code::source;}
    }
};
} // namespace gears_cover_physx_query
