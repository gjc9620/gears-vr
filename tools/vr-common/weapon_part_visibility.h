#pragma once
#include "weapon_hand_participant.h"
#include <cstddef>
#include <cmath>
#include <cstring>

namespace gears_weapon_part_visibility {
// A visibility-only transaction. No arbitrary bone, pose, pointer or scale is
// accepted: Native admits only the named original5-bone Snub magazine.
struct Request {
    std::uint32_t size{sizeof(Request)},version{1};
    gears_weapon_hand::Frame source{};
    gears_weapon_hand::ObjectIdentity component{},asset{},node{};
    std::uint64_t transaction{};
};
enum class Result:std::uint32_t {rejected,pending,hidden,restoring,restored,unavailable,inactive_restored};
struct Receipt {
    std::uint32_t size{sizeof(Receipt)},version{1};Result result{Result::rejected};
    std::uint64_t handle{},writes{};std::uint32_t pose_epoch{};
    bool original_space_hidden{},cpu_restored{},original_inactive{},original_destroyed{};
};
struct Control {std::uint32_t size{sizeof(Control)},version{1};std::uint64_t handle{},cookie{};};
inline constexpr char begin_export[]="GearsVrHideSnubMagazineV1";
inline constexpr char restore_export[]="GearsVrRestoreSnubMagazineV1";
inline constexpr char poll_export[]="GearsVrPollSnubMagazineV1";
#if defined(_WIN32)
using Begin=Result(__cdecl*)(Request const*,Receipt*) noexcept;
using Update=Result(__cdecl*)(Control const*,Receipt*) noexcept;
#else
using Begin=Result(*)(Request const*,Receipt*) noexcept;
using Update=Result(*)(Control const*,Receipt*) noexcept;
#endif
// Original FAtom local/cache arrays and64-byte component-space matrices;
// SpaceBases is NOT another FAtom array. No guessed UObject
// construction. The live adapter certifies identities and original code.
struct Array {std::uintptr_t data{};std::int32_t count{},capacity{};};
struct Atom {float rotation[4]{},position[3]{},scale{};};
struct SpaceMatrix {float value[16]{};};
static_assert(sizeof(Atom)==32&&offsetof(Atom,scale)==28);
static_assert(sizeof(SpaceMatrix)==64);
struct Source {
    std::uintptr_t component{},asset{},node{};std::uint64_t identity_generation{};
    Array local{},cache{},space{};std::uint32_t epoch{};bool verified{};
};
inline bool same(Array a,Array b) noexcept {return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;}
inline bool valid(Array a,bool empty=false) noexcept {
    return a.count>=0&&a.capacity>=a.count&&a.capacity<=512&&
        (a.count==5||(empty&&a.count==0))&&(!a.capacity||
            (a.data&&!(a.data&3)&&a.data<=UINTPTR_MAX-std::size_t(a.capacity)*32));
}
inline bool valid_space(Array a) noexcept {
    return a.count==5&&a.capacity>=a.count&&a.capacity<=512&&a.data&&!(a.data&3)&&
        a.data<=UINTPTR_MAX-std::size_t(a.capacity)*sizeof(SpaceMatrix);
}
inline bool identity(Source const& a,Source const& b) noexcept {
    return a.verified&&b.verified&&a.identity_generation&&a.identity_generation==b.identity_generation&&
        a.component==b.component&&a.asset==b.asset&&a.node==b.node;
}
class ScaleLease {
    Source source_{};Array cache_{};bool bound_{},local_owned_{},cache_owned_{},restoring_{};
    std::uint64_t writes_{};std::uint32_t last_write_epoch_{};
    static std::uintptr_t at(Array a) noexcept {return a.data+4*32+28;}
    template<class Read>static bool scale(Read& read,Array a,float& out) noexcept {
        return valid(a)&&read(at(a),out)&&std::isfinite(out);
    }
    template<class Read>static bool space_scale(Read& read,Array a,float& out) noexcept {
        SpaceMatrix matrix{};
        if(!valid_space(a)||!read(a.data+4*sizeof(SpaceMatrix),matrix))return false;
        const auto& m=matrix.value;for(const auto v:m)if(!std::isfinite(v))return false;
        if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return false;
        bool zero=true;double dot[3][3]{};
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
            for(unsigned k=0;k<3;++k)dot[i][j]+=double(m[i*4+k])*m[j*4+k];
            if(i==j&&dot[i][j]!=0)zero=false;
        }
        if(zero){out=0;return true;}
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
            if(std::abs(dot[i][j]-(i==j?1.:0.))>1e-4)return false;
        out=1;return true;
    }
public:
    bool bound() const noexcept {return bound_;}
    bool restoring() const noexcept {return restoring_;}
    bool cpu_restored() const noexcept {return restoring_&&!local_owned_&&!cache_owned_;}
    std::uint64_t writes() const noexcept {return writes_;}
    std::uint32_t last_write_epoch() const noexcept {return last_write_epoch_;}
    void request_restore() noexcept {if(bound_)restoring_=true;}
    // The original object has been positively identified as deleted/recycled;
    // no stale memory is touched. Absence of metadata is NOT this receipt.
    void original_destroyed() noexcept {*this={};}
    template<class Read>bool begin(Source const& s,Read read) noexcept {
        if(bound_||!s.verified||!s.identity_generation||!s.component||!s.asset||!s.node||!s.epoch||
           !valid(s.local)||!valid(s.cache,true)||!valid_space(s.space))return false;
        float value{},space{};if(!scale(read,s.local,value)||value!=1.f||!space_scale(read,s.space,space)||space!=1.f)return false;
        source_=s;bound_=true;return true;
    }
    bool matches(Source const& s,std::uintptr_t out,std::uint64_t track) const noexcept {
        return bound_&&!restoring_&&identity(source_,s)&&same(source_.local,s.local)&&
            valid(s.local)&&out==s.local.data+4*32&&track==4;
    }
    // Called only AFTER the certified original decoder's normal return. The
    // decoder supplied scale1; zero only this one owned scalar. The original
    // sequence then copies it to its cache and computes SpaceBases/render data.
    template<class Read,class Write>bool decoder_return(Source const& s,std::uintptr_t out,
        std::uint64_t track,Read read,Write write) noexcept {
        if(!matches(s,out,track)||!s.epoch)return false;
        float value{};if(!scale(read,s.local,value)||value!=1.f)return false;
        local_owned_=true; // Journal BEFORE a possibly ambiguous memory write.
        const bool stored=write(at(s.local),0.f);
        float actual{};if(!scale(read,s.local,actual))return false;
        if(actual==0.f){++writes_;last_write_epoch_=s.epoch;return true;}
        if(stored&&actual==1.f)local_owned_=false;
        return false;
    }
    template<class Read>bool observe_cache(Source const& s,Read read) noexcept {
        if(!bound_||!identity(source_,s)||!same(source_.local,s.local)||!valid(s.cache,true))return false;
        if(cache_owned_&&!same(cache_,s.cache)){
            // An original cache clear/reallocation is positive retirement of
            // the old array. Never follow its old backing pointer. A new live
            // cache must independently show original scale1 before retiring.
            float value{};
            if(s.cache.count==0||(scale(read,s.cache,value)&&value==1.f)){cache_owned_=false;cache_={};}
            else return false;
        }
        if(!writes_||s.cache.count==0)return true;
        Atom a{},b{};
        if(!read(s.local.data+4*32,a)||!read(s.cache.data+4*32,b))return false;
        // Exact full-atom equality observes the original sequence's copy.
        // A zero in an unrelated cache is never adopted as our property.
        if(a.scale==0.f&&b.scale==0.f&&!std::memcmp(&a,&b,sizeof(a))){cache_=s.cache;cache_owned_=true;}
        return true;
    }
    template<class Read>bool hidden(Source const& s,Read read) noexcept {
        float v{};return bound_&&!restoring_&&writes_&&identity(source_,s)&&
            same(source_.local,s.local)&&s.epoch>=last_write_epoch_&&space_scale(read,s.space,v)&&v==0.f;
    }
    template<class Read,class Write>bool restore(Source const& s,Read read,Write write,bool original_inactive=false) noexcept {
        if(!bound_||!restoring_||!identity(source_,s)||!same(source_.local,s.local)||
           !valid(s.local)||!valid(s.cache,true)||!valid_space(s.space))return false;
        if(!observe_cache(s,read))return false;
        const auto restore_one=[&](Array a,bool& owned){
            if(!owned)return true;float value{};if(!scale(read,a,value))return false;
            if(value==1.f){owned=false;return true;}
            if(value!=0.f)return false; // Someone changed our scalar: keep debt.
            const bool stored=write(at(a),1.f);float after{};
            if(!scale(read,a,after)||after!=1.f)return false;
            owned=false;return stored||after==1.f;
        };
        if(cache_owned_&&!same(cache_,s.cache))return false;
        if(!restore_one(s.local,local_owned_)||!restore_one(cache_,cache_owned_))return false;
        float value{};
        // Directly restored CPU storage is not a GPU/SpaceBases receipt. Wait
        // for the original evaluator to regenerate the visible transform.
        if(!original_inactive&&(!space_scale(read,s.space,value)||value!=1.f||s.epoch<last_write_epoch_))return false;
        // A positively observed stow/world handoff permits CPU-only retirement
        // of an inactive original component. Its next original evaluation uses
        // the restored local/cache values. This is NOT a visible/GPU receipt.
        bound_=false;return true;
    }
};
} // namespace gears_weapon_part_visibility
