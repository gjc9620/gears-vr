#pragma once
#include "ui_index_membership.h"
namespace gears_ui_sources {
struct Summary {
    bool valid{};
    std::uintptr_t payload{};
    std::uint64_t generation{},native_root_min{},native_root_max{};
    unsigned ordinal{},source_mask{},native_indices{},unknown{};
};
inline constexpr unsigned native_mask=1u<<unsigned(gears_ui_receipts::Source::NativeSubtitle);
// Immutable copied evidence only, not pixel visibility or command completion.
inline bool capture(gears_ui_receipts::Claim const& claim,gears_ui_indices::Snapshot const& s,unsigned ordinal,Summary& out){
    if(!gears_ui_indices::detail::bounded_snapshot(s)||ordinal>=s.segment_count||claim.generation!=s.generation||
       !gears_ui_indices::detail::claim_valid(claim,s.payload,s.vertex_count))return false;
    const auto& segment=s.segments[ordinal];if(!segment.index_count)return false;
    Summary r{};r.payload=s.payload;r.generation=s.generation;r.ordinal=ordinal;
    unsigned entries=0,kind_mask=0;std::array<unsigned,3> known{};
    for(unsigned i=0;i<segment.index_count;++i){const unsigned global=s.global_vertices[segment.index_offset+i];bool found=false;
        for(std::size_t n=0;n<claim.count;++n){auto const& e=claim.entries[n];if(global<e.receipt.begin||global>=e.receipt.end)continue;
            if(found)return false;found=true;entries|=1u<<unsigned(n);kind_mask|=1u<<(e.kind-1);++known[e.kind-1];
            const auto source=unsigned(e.source);if(source<1||source>3)return false;r.source_mask|=1u<<source;
            if(e.source==gears_ui_receipts::Source::NativeSubtitle){++r.native_indices;
                if(!r.native_root_min||e.root_invocation<r.native_root_min)r.native_root_min=e.root_invocation;
                if(e.root_invocation>r.native_root_max)r.native_root_max=e.root_invocation;}
        }if(!found)++r.unknown;
    }
    if(entries!=segment.entry_mask||kind_mask!=segment.kind_mask||known!=segment.known||r.unknown!=segment.unknown)return false;
    r.valid=true;out=r;return true;
}
inline bool native_subtitle(Summary const& s,std::uintptr_t payload,std::uint64_t generation,unsigned ordinal){
    return s.valid&&s.payload==payload&&s.generation==generation&&s.ordinal==ordinal&&
        s.source_mask==native_mask&&!s.unknown&&s.native_indices&&s.native_root_min&&s.native_root_max>=s.native_root_min;
}
}
