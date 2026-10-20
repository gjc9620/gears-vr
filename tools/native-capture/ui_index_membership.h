#pragma once
#include "ui_payload_receipts.h"
#include <memory>
#include <new>

namespace gears_ui_indices {
inline constexpr unsigned segment_capacity=64,index_capacity=8192;
struct DrawArgs {
    std::uint32_t index_count{},instance_count{},start_index{};
    std::int32_t base_vertex{};
    std::uint32_t first_instance{};
};
inline bool same(DrawArgs const& a,DrawArgs const& b){return a.index_count==b.index_count&&a.instance_count==b.instance_count&&
    a.start_index==b.start_index&&a.base_vertex==b.base_vertex&&a.first_instance==b.first_instance;}
struct Segment {
    std::int32_t minimum{},maximum{};
    std::uintptr_t texture{};
    std::uint32_t blend{},index_offset{},index_count{},kind_mask{},entry_mask{};
    std::array<std::uint32_t,3> known{}; // Referenced index occurrences per kind; repeats count.
    std::uint32_t unknown{};
    DrawArgs expected{};
};
struct Snapshot {
    std::uintptr_t payload{};
    std::uint64_t generation{};
    std::uint32_t vertex_count{},segment_count{},total_indices{};
    std::array<Segment,segment_capacity> segments{};
    std::array<std::uint32_t,index_capacity> indices{},global_vertices{};
};
namespace detail {
using Array=gears_ui_batch::detail::Array;
using RawSegment=std::array<unsigned char,36>;
template<class T> T value(RawSegment const& raw,unsigned offset){T out{};std::memcpy(&out,raw.data()+offset,sizeof(out));return out;}
inline bool claim_valid(gears_ui_receipts::Claim const& claim,std::uintptr_t payload,unsigned vertex_count){
    if(claim.count>claim.entries.size()||(claim.payload&&claim.payload!=payload))return false;
    if(!claim.count)return !claim.observed_end&&!claim.unknown_vertices&&!claim.kind_mask&&!claim.vertex_count&&!claim.vertices;
    if(!claim.generation||claim.payload!=payload||!claim.fcanvas||!claim.item||!claim.vertices||claim.vertex_count>gears_ui_receipts::vertex_capacity)return false;
    unsigned previous=0,mask=0,copied=0;std::uint64_t unknown=0;
    for(std::size_t i=0;i<claim.count;++i){auto const& e=claim.entries[i];auto const& r=e.receipt;
        if(!gears_ui_receipts::provenance_valid(r,e.kind,e.source)||!e.root_invocation||r.payload!=payload||r.canvas!=claim.canvas||r.fcanvas!=claim.fcanvas||
           r.item!=claim.item||r.sort_key!=claim.sort_key||r.end<=r.begin||(r.end-r.begin)%4||r.begin<previous||r.end>vertex_count||
           e.vertex_offset!=copied||r.end-r.begin>gears_ui_receipts::vertex_capacity-copied)return false;
        copied+=r.end-r.begin;
        unknown+=r.begin-previous;previous=r.end;mask|=1u<<(e.kind-1);
    }
    return claim.observed_end==previous&&claim.unknown_vertices==unknown&&claim.kind_mask==mask&&claim.vertex_count==copied;
}
inline bool bounded_snapshot(Snapshot const& s){
    if(!s.payload||s.vertex_count>1048576||s.segment_count>segment_capacity||s.total_indices>index_capacity)return false;
    unsigned offset=0;
    for(unsigned i=0;i<s.segment_count;++i){auto const& segment=s.segments[i];
        if(segment.minimum<0||segment.maximum<segment.minimum||unsigned(segment.maximum)>=s.vertex_count||
           segment.index_count%3||segment.index_count>index_capacity-offset||segment.index_offset!=offset||
           !same(segment.expected,DrawArgs{segment.index_count,1,0,0,0}))return false;
        std::uint64_t known_count=0;unsigned mask=0;
        for(unsigned kind=0;kind<3;++kind){known_count+=segment.known[kind];if(segment.known[kind])mask|=1u<<kind;}
        if(known_count+segment.unknown!=segment.index_count||mask!=segment.kind_mask||
           (!known_count&&segment.entry_mask)||(known_count&&(!segment.entry_mask||!s.generation)))return false;
        for(unsigned j=0;j<segment.index_count;++j){auto index=s.indices[offset+j];
            if(index>unsigned(segment.maximum-segment.minimum)||s.global_vertices[offset+j]!=unsigned(segment.minimum)+index)return false;
        }
        offset+=segment.index_count;
    }return offset==s.total_indices;
}
}
// Execute-entry snapshot of CURRENT segment/index data. Unknown/mixed references
// stay explicit: an upload range intersecting a receipt is not index membership.
// Receipt vertex-byte matching, allocation lifetime, caller, scene ticket and
// transform provenance remain separate caller responsibilities. No mutation or
// game calls occur. Failure leaves out intact; no heap-data pointers are retained.
template<class Read> bool capture(Read read,std::uintptr_t payload,gears_ui_receipts::Claim const& claim,Snapshot& out){
    gears_ui_batch::detail::Reader<Read> r{read};detail::Array segments{},vertices{};
    if(!r.field(segments,payload,0x38)||!gears_ui_batch::detail::valid(segments,4096,36)||
       segments.count>int(segment_capacity)||!r.field(vertices,payload,0x48)||
       !gears_ui_batch::detail::valid(vertices,1048576,gears_ui_batch::vertex_stride)||
       !detail::claim_valid(claim,payload,unsigned(vertices.count)))return false;
    std::array<detail::RawSegment,segment_capacity> raw{};
    if(segments.count&&!r.bytes(raw.data(),segments.data,std::size_t(segments.count)*36))return false;
    auto storage=std::unique_ptr<Snapshot>(new(std::nothrow) Snapshot{});
    auto repeated=std::unique_ptr<std::array<std::uint32_t,index_capacity>>(
        new(std::nothrow) std::array<std::uint32_t,index_capacity>{});
    if(!storage||!repeated)return false;
    auto& candidate=*storage;candidate.payload=payload;candidate.generation=claim.generation;
    candidate.vertex_count=unsigned(vertices.count);candidate.segment_count=unsigned(segments.count);
    for(unsigned i=0;i<candidate.segment_count;++i){auto const& bytes=raw[i];auto& segment=candidate.segments[i];
        segment.minimum=detail::value<std::int32_t>(bytes,0);segment.maximum=detail::value<std::int32_t>(bytes,4);
        const auto indices=detail::value<detail::Array>(bytes,8);
        segment.texture=detail::value<std::uintptr_t>(bytes,24);segment.blend=detail::value<std::uint32_t>(bytes,32);
        if(segment.minimum<0||segment.maximum<segment.minimum||unsigned(segment.maximum)>=candidate.vertex_count||
           !gears_ui_batch::detail::valid(indices,1048576,4)||indices.count%3||
           unsigned(indices.count)>index_capacity-candidate.total_indices)return false;
        segment.index_offset=candidate.total_indices;segment.index_count=unsigned(indices.count);
        segment.expected={segment.index_count,1,0,0,0};
        if(segment.index_count&&!r.bytes(candidate.indices.data()+segment.index_offset,indices.data,std::size_t(segment.index_count)*4))return false;
        for(unsigned j=0;j<segment.index_count;++j){auto const index=candidate.indices[segment.index_offset+j];
            if(index>unsigned(segment.maximum-segment.minimum))return false;
            const unsigned global=unsigned(segment.minimum)+index;candidate.global_vertices[segment.index_offset+j]=global;
            bool matched=false;
            for(std::size_t n=0;n<claim.count;++n){auto const& entry=claim.entries[n];
                if(global>=entry.receipt.begin&&global<entry.receipt.end){++segment.known[entry.kind-1];
                    segment.kind_mask|=1u<<(entry.kind-1);segment.entry_mask|=1u<<unsigned(n);matched=true;break;}}
            if(!matched)++segment.unknown;
        }
        candidate.total_indices+=segment.index_count;
    }
    // Repeat every index byte and the complete segment records, then headers.
    // This detects content changes even when TArray data/count/capacity stayed.
    auto& again=*repeated;
    for(unsigned i=0;i<candidate.segment_count;++i){auto const indices=detail::value<detail::Array>(raw[i],8);auto const& segment=candidate.segments[i];
        if(segment.index_count&&(!r.bytes(again.data(),indices.data,std::size_t(segment.index_count)*4)||
           std::memcmp(again.data(),candidate.indices.data()+segment.index_offset,std::size_t(segment.index_count)*4)))return false;
    }
    std::array<detail::RawSegment,segment_capacity> checked{};detail::Array after_segments{},after_vertices{};
    if((segments.count&&(!r.bytes(checked.data(),segments.data,std::size_t(segments.count)*36)||
         std::memcmp(raw.data(),checked.data(),std::size_t(segments.count)*36)))||
       !r.field(after_segments,payload,0x38)||!gears_ui_batch::detail::same(segments,after_segments)||
       !r.field(after_vertices,payload,0x48)||!gears_ui_batch::detail::same(vertices,after_vertices)||
       !detail::bounded_snapshot(candidate))return false;
    out=candidate;return true;
}
// Cursor validation is a SECOND phase: every original certified 172741 callback
// consumes one ordinal, including zero draws, independently of viewport/ticket
// admission. Caller checks native return/certified Execute TLS; synthetic replays
// must bypass this cursor. Equal-count callbacks cannot prove distinguishable
// segment identity from five arguments alone; no searching/reordering is done.
class Cursor {
    std::array<DrawArgs,segment_capacity> expected_{};
    unsigned count_{},next_{};
    bool poisoned_=true,finished_=false;
public:
    Cursor()=default;
    explicit Cursor(Snapshot const& snapshot){
        if(!detail::bounded_snapshot(snapshot))return;
        count_=snapshot.segment_count;for(unsigned i=0;i<count_;++i)expected_[i]=snapshot.segments[i].expected;poisoned_=false;
    }
    bool consume(DrawArgs const& args,unsigned& ordinal){
        if(poisoned_||finished_||next_>=count_||!same(args,expected_[next_])){poisoned_=true;return false;}
        ordinal=next_++;return true;
    }
    // Normal whole-Execute exit is a THIRD phase. Even earlier matched draws
    // cannot establish whole-command completion until this count check succeeds.
    // Observation at a GPU callback follows CPU upload; it does not authorize
    // future coordinate writes or affect vertices already uploaded/executed.
    bool finish(){finished_=true;if(next_!=count_)poisoned_=true;return !poisoned_;}
    bool poisoned() const{return poisoned_;}
    bool completed() const{return finished_&&!poisoned_;}
    unsigned observed() const{return next_;}
    unsigned expected_count() const{return count_;}
};
}
