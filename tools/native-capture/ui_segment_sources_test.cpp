#include "ui_segment_sources.h"
#include <cassert>
#include <cstdio>
struct Fixture {
    gears_ui_receipts::Claim claim{};gears_ui_indices::Snapshot snapshot{};
    Fixture(){gears_ui_receipts::Store store;gears_ui_batch::Receipt r{};r.fcanvas=2;r.item=3;r.payload=4;r.end=4;
        assert(store.publish_range(gears_ui_receipts::range_metadata(r),r.vertices.data(),4,3,17,gears_ui_receipts::Source::NativeSubtitle));
        assert(store.claim(4,claim));snapshot.payload=4;snapshot.generation=claim.generation;snapshot.vertex_count=4;snapshot.segment_count=1;snapshot.total_indices=6;
        auto& s=snapshot.segments[0];s.maximum=3;s.index_count=6;s.kind_mask=4;s.entry_mask=1;s.known[2]=6;s.expected={6,1,0,0,0};
        const unsigned indices[6]={0,1,2,1,3,2};for(unsigned i=0;i<6;++i)snapshot.indices[i]=snapshot.global_vertices[i]=indices[i];}
};
int main(){using namespace gears_ui_sources;Fixture f;Summary s{};
    assert(capture(f.claim,f.snapshot,0,s)&&native_subtitle(s,4,f.claim.generation,0)&&s.native_root_min==17&&s.native_root_max==17);
    assert(!native_subtitle(s,4,f.claim.generation+1,0)&&!native_subtitle(s,4,f.claim.generation,1));
    auto ordinary=f;ordinary.claim.canvas=1;ordinary.claim.entries[0].receipt.canvas=1;ordinary.claim.entries[0].source=gears_ui_receipts::Source::CanvasText;
    assert(capture(ordinary.claim,ordinary.snapshot,0,s)&&!native_subtitle(s,4,ordinary.claim.generation,0));
    Summary sentinel{};sentinel.payload=123;auto stale=f;stale.snapshot.generation++;assert(!capture(stale.claim,stale.snapshot,0,sentinel)&&sentinel.payload==123);
    auto index=f;index.snapshot.indices[0]=9;assert(!capture(index.claim,index.snapshot,0,sentinel));
    auto unknown=f;unknown.snapshot.vertex_count=5;unknown.snapshot.segments[0].maximum=4;unknown.snapshot.indices[0]=unknown.snapshot.global_vertices[0]=4;unknown.snapshot.segments[0].known[2]=5;unknown.snapshot.segments[0].unknown=1;
    assert(capture(unknown.claim,unknown.snapshot,0,s)&&!native_subtitle(s,4,unknown.claim.generation,0));
    auto mixed=s;mixed.unknown=0;mixed.source_mask=native_mask|(1u<<unsigned(gears_ui_receipts::Source::CanvasText));assert(!native_subtitle(mixed,4,mixed.generation,0));
    // Actual copied membership distinguishes different producers even when
    // both use HUD kind3. NativeSubtitle cannot share Canvas!=0 metadata.
    gears_ui_receipts::Store store;gears_ui_batch::Receipt r{};r.canvas=1;r.fcanvas=2;r.item=3;r.payload=4;r.end=4;
    assert(store.publish_range(gears_ui_receipts::range_metadata(r),r.vertices.data(),4,3,17,gears_ui_receipts::Source::Canvas));
    r.begin=4;r.end=8;assert(store.publish_range(gears_ui_receipts::range_metadata(r),r.vertices.data(),4,3,18,gears_ui_receipts::Source::CanvasText));
    auto actual=f;assert(store.claim(4,actual.claim));actual.snapshot.generation=actual.claim.generation;actual.snapshot.vertex_count=8;actual.snapshot.total_indices=12;
    auto& segment=actual.snapshot.segments[0];segment.maximum=7;segment.index_count=12;segment.known[2]=12;segment.entry_mask=3;segment.expected.index_count=12;
    for(unsigned i=0;i<6;++i)actual.snapshot.indices[6+i]=actual.snapshot.global_vertices[6+i]=actual.snapshot.indices[i]+4;
    assert(capture(actual.claim,actual.snapshot,0,s)&&s.source_mask==10&&!native_subtitle(s,4,actual.claim.generation,0));
    auto missing=f;missing.snapshot.segments[0].entry_mask=2;assert(!capture(missing.claim,missing.snapshot,0,sentinel));
    assert(!capture(f.claim,f.snapshot,1,sentinel));std::puts("ui_segment_sources tests passed");
}
