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

// Precondition: the caller exclusively owns this inactive workspace (the
// existing entered claim). No large temporary or game-hook stack allocation.
inline Workspace* activate(Workspace& workspace) noexcept {
    auto* previous=active;
    std::memset(&workspace,0,sizeof(workspace));active=&workspace;return previous;
}
inline void restore(Workspace* previous) noexcept {active=previous;}
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
#if defined(GEARS_WEAPON_METADATA_PARSE_TEST_STATS)
    std::uint64_t probes_{},comparisons_{};
#endif
public:
    Walk() noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL)
        if(active&&!active->walk_owned){
            owner_=active;owner_->walk_owned=true;
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
