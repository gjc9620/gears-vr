#pragma once
#include "ui_canvas_batch.h"
#include <mutex>
#include <memory>
#include <new>
#include <algorithm>

namespace gears_ui_receipts {
inline constexpr std::size_t group_capacity=64,entry_capacity=32,vertex_capacity=1024;
using Vertex=gears_ui_batch::Vertex;
enum class Source : unsigned {Canvas=1,NativeSubtitle=2,CanvasText=3};
struct RangeMetadata {
    std::uintptr_t canvas{},fcanvas{},item{},payload{};
    std::int32_t sort_key{};
    std::uint32_t begin{},end{};
};
inline RangeMetadata range_metadata(gears_ui_batch::Receipt const& r){return {r.canvas,r.fcanvas,r.item,r.payload,r.sort_key,r.begin,r.end};}
inline bool provenance_valid(RangeMetadata const& r,unsigned kind,Source source){
    return kind>=1&&kind<=3&&((source==Source::NativeSubtitle&&!r.canvas&&kind==3)||
        ((source==Source::Canvas||source==Source::CanvasText)&&r.canvas));
}
struct VertexBuffer {std::array<Vertex,vertex_capacity> data{};};
using VertexAllocator=std::shared_ptr<VertexBuffer>(*)();
inline std::shared_ptr<VertexBuffer> allocate_vertices(){return std::make_shared<VertexBuffer>();}
inline constexpr unsigned text_group_range_capacity=64;
struct TextGroupRange {RangeMetadata receipt{};std::uint32_t vertex_offset{};};
// Complete producer-call geometry, shared across atlas payloads. These are
// copied positions only; group membership never establishes GPU index ownership.
struct TextGroupGeometry {
    std::uint64_t root_invocation{};
    std::uintptr_t fcanvas{};std::int32_t sort_key{};
    std::array<float,16> matrix{};
    unsigned vertex_count{},range_count{};
    std::array<std::array<float,2>,vertex_capacity> positions{};
    std::array<TextGroupRange,text_group_range_capacity> ranges{};
};
using TextGroup=std::shared_ptr<const TextGroupGeometry>;
inline bool text_group_range_valid(TextGroupGeometry const& group,RangeMetadata const& r,
    std::uint64_t root,Source source,unsigned offset,unsigned length){
    if(source!=Source::NativeSubtitle||r.canvas||!root||group.root_invocation!=root||
       group.fcanvas!=r.fcanvas||group.sort_key!=r.sort_key||!group.vertex_count||
       group.vertex_count>vertex_capacity||group.range_count<2||group.range_count>text_group_range_capacity||
       offset>group.vertex_count||length>group.vertex_count-offset||r.end<=r.begin||length!=r.end-r.begin)return false;
    unsigned matches=0;
    for(unsigned i=0;i<group.range_count;++i){auto const& part=group.ranges[i];auto const& p=part.receipt;
        if(part.vertex_offset==offset&&p.canvas==r.canvas&&p.fcanvas==r.fcanvas&&p.item==r.item&&
           p.payload==r.payload&&p.sort_key==r.sort_key&&p.begin==r.begin&&p.end==r.end)++matches;}
    return matches==1;
}
struct Entry {
    RangeMetadata receipt{};
    unsigned kind{}; // 1 reticle, 2 weapon UI, 3 HUD; caller-certified label only.
    std::uint64_t root_invocation{};
    std::uint32_t vertex_offset{};
    Source source=Source::Canvas;
    bool whole_text_group{}; // Explicit producer proof; multi-atlas fragments are false.
    TextGroup text_group;
    std::uint32_t text_group_offset{};
};
struct Claim {
    std::uint64_t generation{};
    std::uintptr_t payload{},canvas{},fcanvas{},item{};
    std::int32_t sort_key{};
    std::size_t count{};
    std::uint32_t observed_end{};
    std::uint64_t unknown_vertices{}; // Prefix/gaps through observed_end only.
    unsigned kind_mask{};
    std::uint32_t vertex_count{};
    // Immutable owning copy; Claim copies share bytes that Store never mutates
    // after claim. No game heap pointer or giant inline vertex array is kept.
    std::shared_ptr<const VertexBuffer> vertices;
    std::array<Entry,entry_capacity> entries{};
};
struct Stats {
    std::uint64_t publish_attempts{},published{},rejected{},capacity_rejected{},groups_poisoned{},
        claimed{},poison_claimed{},retired{},resets{},unknown_vertices_seen{},allocation_rejected{},groups_full{};
    std::size_t active_groups{},poisoned_groups{};
    bool disabled{};
};
// Diagnostic candidates only. Tokens are monotonic per Store, NOT allocation
// epochs or ABA proofs. No caller, scene ticket, queued-command identity, batch
// purity, draw execution or target freshness is established here. Consumer
// must validate current vertex bytes; Claim has an independent bounded copy.
class Store {
    struct Group {bool active{},poisoned{},world_retired{};Claim claim{};std::shared_ptr<VertexBuffer> storage;};
    std::array<Group,group_capacity> groups_{};
    std::uint64_t last_generation_{};
    Stats stats_{};
    mutable std::mutex mutex_;
    VertexAllocator allocate_;
    Group* find(std::uintptr_t payload){for(auto& g:groups_)if(g.active&&g.claim.payload==payload)return &g;return nullptr;}
    void poison(Group& g){if(!g.poisoned){++stats_.groups_poisoned;g.poisoned=true;
        // Keep only the tombstone identity and generation; no successful data
        // survives conflict/overflow. This payload cannot republish yet.
        Claim tombstone{};tombstone.payload=g.claim.payload;tombstone.generation=g.claim.generation;g.claim=tombstone;g.storage.reset();}}
    void disable(){stats_.disabled=true;for(auto& g:groups_)if(g.active)poison(g);}
    static bool valid(RangeMetadata const& r,unsigned kind,std::uint64_t invocation,Source source){
        return r.payload&&r.fcanvas&&r.item&&provenance_valid(r,kind,source)&&invocation&&r.end>r.begin&&(r.end-r.begin)%4==0;
    }
    static bool metadata(Claim const& c,RangeMetadata const& r){return c.payload==r.payload&&c.canvas==r.canvas&&
        c.fcanvas==r.fcanvas&&c.item==r.item&&c.sort_key==r.sort_key;}
public:
    explicit Store(VertexAllocator allocator=allocate_vertices):allocate_(allocator){}
    Store(Store const&)=delete;Store& operator=(Store const&)=delete;
    bool publish(gears_ui_batch::Receipt const& r,unsigned kind,std::uint64_t root_invocation){
        return publish_range(range_metadata(r),r.vertices.data(),r.vertices.size(),kind,root_invocation,Source::Canvas);
    }
    // Span is caller-owned meaningful 44-byte copies, not a game data pointer.
    // One entry covers the WHOLE range; never silently truncate text to quads.
    // Buffers allocate at most1024 vertices per payload, and any overflow or
    // allocation failure poisons all that group's candidates until consumption.
    bool publish_range(RangeMetadata const& r,Vertex const* vertices,std::size_t length,unsigned kind,
        std::uint64_t root_invocation,Source source=Source::Canvas,bool whole_text_group=false,
        TextGroup text_group={},std::uint32_t text_group_offset=0){
        std::lock_guard<std::mutex> lock(mutex_);++stats_.publish_attempts;
        auto reject=[&](){++stats_.rejected;return false;};
        if(stats_.disabled)return reject();
        auto group=find(r.payload);
        if(!valid(r,kind,root_invocation,source)||!vertices||length!=std::size_t(r.end-r.begin)||
           (text_group&&!text_group_range_valid(*text_group,r,root_invocation,source,text_group_offset,unsigned(length)))){
            if(group)poison(*group);return reject();}
        if(group&&group->poisoned)return reject();
        if(!group){
            for(auto& candidate:groups_)if(!candidate.active){group=&candidate;break;}
            // A new key has no retained candidate to invalidate. Drop only this
            // range when full; never evict another key or clear its tombstone.
            // A later admitted range records its missing prefix as unknown;
            // consumers still require actual per-index membership and bytes.
            if(!group){++stats_.capacity_rejected;++stats_.groups_full;return reject();}
            if(last_generation_==(std::numeric_limits<std::uint64_t>::max)()){
                ++stats_.capacity_rejected;disable();return reject();
            }
            group->active=true;auto& c=group->claim;c.generation=++last_generation_;
            c.payload=r.payload;c.canvas=r.canvas;c.fcanvas=r.fcanvas;c.item=r.item;c.sort_key=r.sort_key;
        }
        auto& c=group->claim;
        if(!metadata(c,r)||(c.count&&r.begin<c.observed_end)){poison(*group);return reject();}
        if(c.count==entry_capacity||length>vertex_capacity-c.vertex_count){++stats_.capacity_rejected;poison(*group);return reject();}
        if(!group->storage){
            try{group->storage=allocate_?allocate_():nullptr;}catch(std::bad_alloc const&){group->storage.reset();}
            if(!group->storage||group->storage.use_count()!=1){++stats_.allocation_rejected;poison(*group);return reject();}
            c.vertices=group->storage;
        }
        const std::uint64_t gap=std::uint64_t(r.begin)-c.observed_end;
        c.unknown_vertices+=gap;stats_.unknown_vertices_seen+=gap;
        std::copy(vertices,vertices+length,group->storage->data.begin()+c.vertex_count);
        c.entries[c.count++]={r,kind,root_invocation,c.vertex_count,source,whole_text_group,std::move(text_group),text_group_offset};c.vertex_count+=std::uint32_t(length);
        c.observed_end=r.end;c.kind_mask|=1u<<(kind-1);
        ++stats_.published;return true;
    }
    bool claim(std::uintptr_t payload,Claim& out){
        std::lock_guard<std::mutex> lock(mutex_);auto group=find(payload);if(!group)return false;
        if(group->poisoned||stats_.disabled){++stats_.poison_claimed;*group={};return false;}
        Claim candidate=group->claim;*group={};out=candidate;++stats_.claimed;return true;
    }
    bool retire(std::uintptr_t payload){
        std::lock_guard<std::mutex> lock(mutex_);auto group=find(payload);if(!group||group->world_retired)return false;
        *group={};++stats_.retired;return true;
    }
    // World boundary: revoke candidates, retaining keys until their consumer.
    // Detached Claims own immutable bytes; caller must first drain consumers.
    void retire_all(){std::lock_guard<std::mutex> lock(mutex_);for(auto& g:groups_)if(g.active){poison(g);g.world_retired=true;}}
    Stats stats() const {
        std::lock_guard<std::mutex> lock(mutex_);auto result=stats_;
        for(auto const& g:groups_)if(g.active){++result.active_groups;if(g.poisoned)++result.poisoned_groups;}
        return result;
    }
    // Explicit external boundary only. Never implicit eviction/tombstone reset
    // or generation rewind. Runtime does NOT call this to recover saturation.
    void reset(){std::lock_guard<std::mutex> lock(mutex_);groups_={};stats_.disabled=false;++stats_.resets;}
};
}
