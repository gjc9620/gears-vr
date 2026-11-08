#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

// Private parsing scratch only. No object/field/value/eligibility witness is
// stored here. Its sole live owner is the claimed synchronous PG callback.
namespace weapon_metadata_parse {
using Address=std::uintptr_t;
using U32=std::uint32_t;
inline constexpr U32 name_units=96,name_buckets=4096,name_probes=16,visit_buckets=8192;
struct NameBound {Address entry{};U32 index{},length{};};
struct Workspace {
    std::array<NameBound,name_buckets> names{};
    std::array<Address,visit_buckets> addresses{};
    std::array<U32,visit_buckets> epochs{};
    U32 epoch{};bool walk_owned{};
};
static_assert(sizeof(Address)==8&&std::is_trivially_copyable_v<Workspace>);
inline thread_local Workspace* active{};
// Cycle-membership scratch for ONE metadata query only. It is deliberately
// separate from active: installing it must not enable name-length retention.
inline thread_local Workspace* traversal_only{};

class TraversalQuery {
    bool installed_{};
public:
    explicit TraversalQuery(bool allow_query=true) noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL)
        if(allow_query&&!active&&!traversal_only){
            static thread_local Workspace scratch;
            std::memset(scratch.epochs.data(),0,sizeof(scratch.epochs));
            scratch.epoch=0;scratch.walk_owned=false;
            traversal_only=&scratch;installed_=true;
        }
#else
        (void)allow_query;
#endif
    }
    ~TraversalQuery() noexcept {if(installed_)traversal_only=nullptr;}
    TraversalQuery(TraversalQuery const&)=delete;
    TraversalQuery& operator=(TraversalQuery const&)=delete;
};

// Precondition: the caller exclusively owns this inactive workspace (the
// existing entered claim). No large temporary or game-hook stack allocation.
inline Workspace* activate(Workspace& workspace) noexcept {
    auto* previous=active;
    std::memset(&workspace,0,sizeof(workspace));active=&workspace;return previous;
}
inline void restore(Workspace* previous) noexcept {active=previous;}
// Ordinary PlayerTick also performs the same bounded reflection walks as the
// mount provider. Reuse its parsing acceleration without retaining field,
// object or input values between ticks. Nested scopes borrow the outer scratch.
class TickScope {
    Workspace* previous_{};bool installed_{};
public:
    TickScope() noexcept {
        if(!active){
            static thread_local Workspace scratch;
            previous_=activate(scratch);installed_=true;
        }
    }
    ~TickScope(){if(installed_)restore(previous_);}
    TickScope(TickScope const&)=delete;
    TickScope& operator=(TickScope const&)=delete;
};
inline std::size_t bucket(Address value,U32 mask) noexcept {
    auto x=std::uint64_t(value)>>3;
    x^=x>>30;x*=0xbf58476d1ce4e5b9ull;x^=x>>27;x*=0x94d049bb133111ebull;x^=x>>31;
    return static_cast<std::size_t>(x&mask);
}
inline std::size_t name_bucket(Address entry,U32 index) noexcept {
    return bucket(entry^(std::uint64_t(index)*0x9e3779b97f4a7c15ull),name_buckets-1);
}
inline U32 length_bound(Address entry,U32 index) noexcept {
    if(!active||!entry)return 0;
    auto slot=name_bucket(entry,index);
    for(U32 n=0;n<name_probes;++n,slot=(slot+1)&(name_buckets-1)){
        auto const& value=active->names[slot];
        if(!value.entry)return 0;
        if(value.entry==entry&&value.index==index)return value.length;
    }
    return 0; // Collisions/capacity never invent a length or reject a name.
}
inline void remember_length(Address entry,U32 index,U32 length) noexcept {
    if(!active||!entry||!length||length>=name_units)return;
    auto slot=name_bucket(entry,index);
    for(U32 n=0;n<name_probes;++n,slot=(slot+1)&(name_buckets-1)){
        auto& value=active->names[slot];
        if(!value.entry){value={entry,index,length};return;}
        if(value.entry==entry&&value.index==index)return;
    }
}

// One existing field-chain walk. The caller keeps its original bounded seen
// array/count and writes every visited address there, so nested walks and an
// exhausted hash table can use the exact original linear membership check.
class Walk {
    Workspace* owner_{};U32 epoch_{};bool hashed_{};
#if defined(GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL)
    // The same caller-owned append-only visited prefix also exists when no
    // workspace is installed (or a nested walk owns it). A tiny local Bloom
    // filter can prove absence; a possible match STILL runs the original
    // exact array comparison. No game object/value/selector is cached here.
    static constexpr U32 filter_bits=8192;
    std::array<std::uint64_t,filter_bits/64> filter_{};
    Address const* filter_prefix_{};U32 filter_count_{};
    void filter_add(Address address) noexcept {
        const auto a=bucket(address,filter_bits-1);
        const auto b=bucket(address^Address(0x9e3779b97f4a7c15ull),filter_bits-1);
        filter_[a/64]|=std::uint64_t(1)<<(a%64);
        filter_[b/64]|=std::uint64_t(1)<<(b%64);
    }
    bool filter_maybe(Address address,Address const* previous,U32 count) noexcept {
        // Every production caller preserves its local prefix and appends
        // each visited address. Also handle a prepopulated/new array or a
        // restarted prefix; never infer its contents from the queried value.
        if(filter_prefix_!=previous||count<filter_count_){filter_={};filter_prefix_=previous;filter_count_=0;}
        for(;filter_count_<count;++filter_count_)filter_add(previous[filter_count_]);
        const auto a=bucket(address,filter_bits-1);
        const auto b=bucket(address^Address(0x9e3779b97f4a7c15ull),filter_bits-1);
        return (filter_[a/64]&(std::uint64_t(1)<<(a%64)))&&
            (filter_[b/64]&(std::uint64_t(1)<<(b%64)));
    }
#endif
#if defined(GEARS_WEAPON_METADATA_PARSE_TEST_STATS)
    std::uint64_t probes_{},comparisons_{};
#endif
public:
    Walk() noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL)
        auto* const workspace=active?active:traversal_only;
        if(workspace&&!workspace->walk_owned){
            owner_=workspace;owner_->walk_owned=true;
            if(!++owner_->epoch){std::memset(owner_->epochs.data(),0,sizeof(owner_->epochs));owner_->epoch=1;}
            epoch_=owner_->epoch;hashed_=true;
        }
#endif
    }
    Walk(Walk const&)=delete;Walk& operator=(Walk const&)=delete;
    ~Walk() noexcept {if(owner_)owner_->walk_owned=false;}
    bool seen(Address address,Address const* previous,U32 count) noexcept {
        if(hashed_){
            auto slot=bucket(address,visit_buckets-1);
            for(U32 n=0;n<visit_buckets;++n,slot=(slot+1)&(visit_buckets-1)){
#if defined(GEARS_WEAPON_METADATA_PARSE_TEST_STATS)
                ++probes_;
#endif
                if(owner_->epochs[slot]!=epoch_){owner_->epochs[slot]=epoch_;owner_->addresses[slot]=address;return false;}
                if(owner_->addresses[slot]==address)return true;
            }
            hashed_=false; // The original caller array includes every prefix.
        }
#if defined(GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL)
        if(!filter_maybe(address,previous,count))return false;
#endif
        for(U32 i=0;i<count;++i){
#if defined(GEARS_WEAPON_METADATA_PARSE_TEST_STATS)
            ++comparisons_;
#endif
            if(previous[i]==address)return true;
        }
        return false;
    }
#if defined(GEARS_WEAPON_METADATA_PARSE_TEST_STATS)
    std::uint64_t probes() const noexcept {return probes_;}
    std::uint64_t comparisons() const noexcept {return comparisons_;}
#endif
};
}
