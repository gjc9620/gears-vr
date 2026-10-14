#pragma once
#include "ui_canvas_batch.h"
#include <mutex>

namespace gears_ui_receipts {
inline constexpr std::size_t group_capacity=16,entry_capacity=32;
struct Entry {
    gears_ui_batch::Receipt receipt{};
    unsigned kind{}; // 1 reticle, 2 weapon UI, 3 HUD; caller-certified label only.
    std::uint64_t root_invocation{};
};
struct Claim {
    std::uint64_t generation{};
    std::uintptr_t payload{},canvas{},fcanvas{},item{};
    std::int32_t sort_key{};
    std::size_t count{};
    std::uint32_t observed_end{};
    std::uint64_t unknown_vertices{}; // Prefix/gaps through observed_end only.
    unsigned kind_mask{};
    std::array<Entry,entry_capacity> entries{};
};
struct Stats {
    std::uint64_t publish_attempts{},published{},rejected{},capacity_rejected{},groups_poisoned{},
        claimed{},poison_claimed{},retired{},resets{},unknown_vertices_seen{};
    std::size_t active_groups{},poisoned_groups{};
    bool disabled{};
};
// Diagnostic candidates only. Tokens are monotonic per Store, NOT allocation
// epochs or ABA proofs. No caller, scene ticket, queued-command identity, batch
// purity, draw execution or target freshness is established here. Consumer
// must validate current vertex bytes; Claim has an independent bounded copy.
class Store {
    struct Group {bool active{},poisoned{};Claim claim{};};
    std::array<Group,group_capacity> groups_{};
    std::uint64_t last_generation_{};
    Stats stats_{};
    mutable std::mutex mutex_;
    Group* find(std::uintptr_t payload){for(auto& g:groups_)if(g.active&&g.claim.payload==payload)return &g;return nullptr;}
    void poison(Group& g){if(!g.poisoned){++stats_.groups_poisoned;g.poisoned=true;
        // Keep only the tombstone identity and generation; no successful data
        // survives conflict/overflow. This payload cannot republish yet.
        Claim tombstone{};tombstone.payload=g.claim.payload;tombstone.generation=g.claim.generation;g.claim=tombstone;}}
    void disable(){stats_.disabled=true;for(auto& g:groups_)if(g.active)poison(g);}
    static bool valid(gears_ui_batch::Receipt const& r,unsigned kind,std::uint64_t invocation){
        return r.payload&&r.canvas&&r.fcanvas&&r.item&&kind>=1&&kind<=3&&invocation&&r.end>r.begin&&r.end-r.begin==4;
    }
    static bool metadata(Claim const& c,gears_ui_batch::Receipt const& r){return c.payload==r.payload&&c.canvas==r.canvas&&
        c.fcanvas==r.fcanvas&&c.item==r.item&&c.sort_key==r.sort_key;}
public:
    Store()=default;Store(Store const&)=delete;Store& operator=(Store const&)=delete;
    bool publish(gears_ui_batch::Receipt const& r,unsigned kind,std::uint64_t root_invocation){
        std::lock_guard<std::mutex> lock(mutex_);++stats_.publish_attempts;
        auto reject=[&](){++stats_.rejected;return false;};
        if(stats_.disabled)return reject();
        auto group=find(r.payload);
        if(!valid(r,kind,root_invocation)){if(group)poison(*group);return reject();}
        if(group&&group->poisoned)return reject();
        if(!group){
            for(auto& candidate:groups_)if(!candidate.active){group=&candidate;break;}
            if(!group||last_generation_==(std::numeric_limits<std::uint64_t>::max)()){
                ++stats_.capacity_rejected;disable();return reject();
            }
            group->active=true;auto& c=group->claim;c.generation=++last_generation_;
            c.payload=r.payload;c.canvas=r.canvas;c.fcanvas=r.fcanvas;c.item=r.item;c.sort_key=r.sort_key;
        }
        auto& c=group->claim;
        if(!metadata(c,r)||(c.count&&r.begin<c.observed_end)){poison(*group);return reject();}
        if(c.count==entry_capacity){++stats_.capacity_rejected;poison(*group);return reject();}
        const std::uint64_t gap=std::uint64_t(r.begin)-c.observed_end;
        c.unknown_vertices+=gap;stats_.unknown_vertices_seen+=gap;
        c.entries[c.count++]={r,kind,root_invocation};c.observed_end=r.end;c.kind_mask|=1u<<(kind-1);
        ++stats_.published;return true;
    }
    bool claim(std::uintptr_t payload,Claim& out){
        std::lock_guard<std::mutex> lock(mutex_);auto group=find(payload);if(!group)return false;
        if(group->poisoned||stats_.disabled){++stats_.poison_claimed;*group={};return false;}
        Claim candidate=group->claim;*group={};out=candidate;++stats_.claimed;return true;
    }
    bool retire(std::uintptr_t payload){
        std::lock_guard<std::mutex> lock(mutex_);auto group=find(payload);if(!group)return false;
        *group={};++stats_.retired;return true;
    }
    Stats stats() const {
        std::lock_guard<std::mutex> lock(mutex_);auto result=stats_;
        for(auto const& g:groups_)if(g.active){++result.active_groups;if(g.poisoned)++result.poisoned_groups;}
        return result;
    }
    // Explicit external boundary only. Never implicit eviction/recovery and no
    // generation rewind. Runtime for the present diagnostic does NOT reset.
    void reset(){std::lock_guard<std::mutex> lock(mutex_);groups_={};stats_.disabled=false;++stats_.resets;}
};
}
