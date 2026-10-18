#pragma once
#include "ngx_stereo_adapter.h"

// Unwired CPU receipt joins. The host supplies observations from its typed
// binding, command recording, Execute/Signal and completion hooks. These
// predicates do not observe the GPU, own resources or authorize an SDK call.
namespace gears_ngx_stereo {
struct RecordedPosition {
    uint64_t list{},generation{},operation{};
};
inline bool valid(RecordedPosition p){return p.list&&p.generation&&p.operation;}
inline bool same_list(RecordedPosition a,RecordedPosition b){return a.list==b.list&&a.generation==b.generation;}
struct OutputDrawFacts {
    Identity identity{};uint64_t ticket{};unsigned eye{};
    Resource bound_color{}; // Actual final draw's typed SRV resource.
    RecordedPosition draw{};
    Rect rendered{};Extent display{};
};
struct RecordedOutputDraw {
    RecordedOutput output{};OutputDrawFacts facts{};
    std::array<RecordedPosition,2> copies{};
    Rect source{},destination{};
};
inline bool record_output_draw(RecordedOutput const& output,
    std::array<RecordedPosition,2> const& copies,OutputDrawFacts const& facts,
    RecordedOutputDraw& result){
    if(!valid(facts.draw)||!same(facts.identity,output.identity)||facts.ticket!=output.ticket)return false;
    auto const& color=facts.bound_color;
    if(color.identity!=output.color||color.device!=output.identity.device||
       !same(color.extent,output.layout.physical[0])||color.format!=26||color.mip||color.samples!=1||color.subresource)return false;
    for(unsigned i=0;i<2;++i){auto const& c=output.copies[i];
        if(!valid(copies[i])||copies[i].list!=c.list||copies[i].generation!=c.generation)return false;
        // Same-list GPU ordering follows command recording position. Across
        // lists, geometry is provisional until the submission join below.
        if(same_list(copies[i],facts.draw)&&copies[i].operation>=facts.draw.operation)return false;
    }
    if(same_list(copies[0],copies[1])&&copies[0].operation>=copies[1].operation)return false;
    Rect source{},destination{};
    if(!output_geometry(output,facts.identity,facts.ticket,color.identity,facts.eye,
        facts.rendered,facts.display,source,destination))return false;
    result={output,facts,copies,source,destination};return true;
}
inline bool matches_execution(RecordedPosition p,ExecuteReceipt const& e,uint64_t device){
    auto const& s=e.submission;
    return valid(p)&&e.order&&s.direct&&s.device==device&&s.list==p.list&&s.generation==p.generation&&s.queue&&s.fence&&s.value&&s.value!=UINT64_MAX;
}
// Deliberately admits only one direct queue and one monotonic fence timeline.
// Multiple queues require explicit Wait edges and are outside this first slice.
inline bool precedes(RecordedPosition a,ExecuteReceipt const& ae,
    RecordedPosition b,ExecuteReceipt const& be){
    auto const& x=ae.submission;auto const& y=be.submission;
    if(x.device!=y.device||x.queue!=y.queue||x.fence!=y.fence||x.value>y.value||ae.order>be.order)return false;
    if(ae.order<be.order)return !same_list(a,b)&&x.value<y.value;
    if(x.value!=y.value||ae.member>be.member)return false;
    // Member order in one Execute batch is NOT a completion edge. D3D12
    // permits cross-list overlap; admitting it needs resource barrier evidence.
    if(ae.member<be.member)return false;
    return same_list(a,b)&&a.operation<b.operation;
}
inline bool submitted_output_draw(RecordedOutputDraw const& recorded,
    std::array<ExecuteReceipt,2> const& copies,ExecuteReceipt const& draw){
    // Ordering/identity join only. Host must separately authenticate the actual
    // Color COPY_DEST -> shader-readable transition (and Output scratch reuse).
    // Revalidate copied receipts so this remains safe on default or stale data.
    RecordedOutputDraw checked{};
    if(!record_output_draw(recorded.output,recorded.copies,recorded.facts,checked))return false;
    const auto device=recorded.output.identity.device;
    for(unsigned i=0;i<2;++i)if(!matches_execution(recorded.copies[i],copies[i],device))return false;
    if(!matches_execution(recorded.facts.draw,draw,device))return false;
    return precedes(recorded.copies[0],copies[0],recorded.copies[1],copies[1])&&
        precedes(recorded.copies[1],copies[1],recorded.facts.draw,draw);
}
inline bool completed_output_draw(RecordedOutputDraw const& recorded,
    std::array<ExecuteReceipt,2> const& copies,ExecuteReceipt const& draw,Completion completed){
    auto const& s=draw.submission;
    // UINT64_MAX is D3D12's removed-device completion sentinel, not retirement.
    return completed.value!=UINT64_MAX&&submitted_output_draw(recorded,copies,draw)&&
        completed.device==s.device&&completed.queue==s.queue&&completed.fence==s.fence&&completed.value>=s.value;
}
}
