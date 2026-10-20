#pragma once
#include "final_transfer_policy.h"
#include "frame_link.h"

// Observe-only CPU snapshots. Entry may precede lazy engine binding; neither
// equality nor a complete entry grants transfer admission or GPU proof.
namespace gears_final_transfer_bracket {
struct Draw {
    uintptr_t raw_list{},caller{};unsigned kind{},elements{},instances{};
    bool operator==(Draw const& b)const{return raw_list==b.raw_list&&caller==b.caller&&kind==b.kind&&elements==b.elements&&instances==b.instances;}
};
struct DrawCalls {
    uint64_t all_draws{},matched_draws{},bypass_draws{};Draw first{},first_matched{};
    void call(Draw value,bool bypass){
        if(bypass){++bypass_draws;return;}
        if(++all_draws==1)first=value;
    }
    void match(Draw value){if(++matched_draws==1)first_matched=value;}
    bool exactly_one_routed_draw()const{return all_draws==1&&matched_draws==1&&!bypass_draws&&first==first_matched;}
};
struct Snapshot {
    gears_final_transfer::Input input{};
    gears_frame_link::ListRef list{};
    uintptr_t raw_list{},context{},source_object{},chain{};
    uint64_t world{},inventory{},resize{},ticket{};
    unsigned reason{},pair_rejection{};bool pair_linked{};
    long long qpc{};
    gears_viewport::BindingProvenance provenance{};
};
enum PendingFailure:unsigned {pending_invalid=1,pending_target=2,pending_provenance=4,pending_rectangles=8,
    pending_not_dirty=16,pending_changed=32,pending_dirty_bind=64,pending_actual=128,pending_unattested=256};
struct Pending {
    bool readable{},stable{},known_buffer{},descriptor_lost{};
    unsigned count{},viewport_count{},scissor_count{};std::array<unsigned char,3> dirty{};
    uintptr_t view{},handle{};gears_viewport::Target target{};gears_viewport::DescriptorTrace trace{};
    std::array<float,6> viewport{};std::array<int32_t,4> scissor{};
};
inline bool equal_pending(Pending const& a,Pending const& b){
    return a.readable==b.readable&&a.count==b.count&&a.viewport_count==b.viewport_count&&a.scissor_count==b.scissor_count&&
        a.dirty==b.dirty&&a.view==b.view&&a.handle==b.handle&&a.target==b.target&&a.trace==b.trace&&
        a.viewport==b.viewport&&a.scissor==b.scissor&&a.known_buffer==b.known_buffer&&a.descriptor_lost==b.descriptor_lost;
}
inline gears_viewport::Rect pending_viewport(Pending const& p){return {p.viewport[0],p.viewport[1],p.viewport[2],p.viewport[3]};}
inline gears_viewport::Rect pending_scissor(Pending const& p){return {float(p.scissor[0]),float(p.scissor[1]),
    float(int64_t(p.scissor[2])-p.scissor[0]),float(int64_t(p.scissor[3])-p.scissor[1])};}
inline unsigned pending_reason(Pending const& p,Snapshot const& bound){
    unsigned reason{};auto const& t=p.target;auto const& r=bound.input.rectangles;
    if(!p.readable||!p.stable||p.count!=1||p.viewport_count!=1||p.scissor_count!=1||!p.view||!p.handle)reason|=pending_invalid;
    if(p.descriptor_lost||!p.known_buffer||!t.resource||t.mip||t.format!=24||!t.width||t.width>8192||t.width%2||!t.height||t.height>8192||
       t.width!=bound.input.dimensions[0]||t.height!=bound.input.dimensions[1])reason|=pending_target;
    if(!p.trace.write_serial||!p.trace.origin_create_serial||!p.trace.creation_live)reason|=pending_provenance;
    for(float v:p.viewport)if(!std::isfinite(v))reason|=pending_rectangles;
    if(!(pending_viewport(p)==gears_viewport::Rect{0,0,float(t.width),float(t.height)})||p.viewport[4]!=0||p.viewport[5]!=1||
       p.scissor[0]>0||p.scissor[1]>0||p.scissor[2]<int64_t(t.width)||p.scissor[3]<int64_t(t.height))reason|=pending_rectangles;
    for(auto flag:p.dirty)if(flag>1)reason|=pending_invalid;
    if((!p.dirty[0]&&(bound.input.binding.targets!=1||!(bound.input.binding.target==t)||
        bound.provenance.handle!=p.handle||!(bound.provenance.trace==p.trace)))||
       (!p.dirty[1]&&(!r.tracked||r.viewport_count!=1||!r.depth_known||!(r.viewport==pending_viewport(p))||r.min_depth!=p.viewport[4]||r.max_depth!=p.viewport[5]))||
       (!p.dirty[2]&&(!r.tracked||r.scissor_count!=1||!(r.scissor==pending_scissor(p)))))reason|=pending_not_dirty;
    return reason;
}
struct ConsumedBinds {
    std::array<uint64_t,3> calls{},matched{},mismatched{},bypassed{},wrong_list{},wrong_args{};
    void consume(unsigned kind,bool list_matches,bool args_match,bool bypass){
        ++calls[kind];if(bypass)++bypassed[kind];
        else {if(!list_matches)++wrong_list[kind];if(!args_match)++wrong_args[kind];
            if(list_matches&&args_match)++matched[kind];else ++mismatched[kind];}
    }
};
inline unsigned compare_pending(Pending const& entry,Pending const& draw,Snapshot const& actual,ConsumedBinds const& binds){
    unsigned reason=pending_reason(draw,actual);auto const& r=actual.input.rectangles;
    if(entry.view!=draw.view||entry.handle!=draw.handle||!(entry.target==draw.target)||!(entry.trace==draw.trace)||
       entry.count!=draw.count||entry.viewport_count!=draw.viewport_count||entry.scissor_count!=draw.scissor_count||
       entry.viewport!=draw.viewport||entry.scissor!=draw.scissor)reason|=pending_changed;
    if(actual.input.binding.targets!=1||!(entry.target==actual.input.binding.target)||actual.provenance.handle!=entry.handle||
       !(actual.provenance.trace==entry.trace)||!r.tracked||r.viewport_count!=1||r.scissor_count!=1||!r.depth_known||
       !(r.viewport==pending_viewport(entry))||!(r.scissor==pending_scissor(entry))||r.min_depth!=entry.viewport[4]||r.max_depth!=entry.viewport[5])reason|=pending_actual;
    for(unsigned i=0;i<3;++i)if((entry.dirty[i]&&!binds.matched[i])||binds.mismatched[i]||binds.bypassed[i])reason|=pending_dirty_bind;
    return reason;
}
enum Difference:unsigned {list=1,source=2,target=4,rectangles=8,world=16,inventory=32,pair=64,quad=128};
inline unsigned compare(Snapshot const& a,Snapshot const& b){
    unsigned out{};
    if(a.raw_list!=b.raw_list||a.list.identity!=b.list.identity||a.list.generation!=b.list.generation)out|=list;
    auto const& x=a.input.source;auto const& y=b.input.source;
    if(a.source_object!=b.source_object||a.context!=b.context||a.input.slots!=b.input.slots||
       x.resource!=y.resource||x.dimension!=y.dimension||x.width!=y.width||x.height!=y.height||
       x.depth!=y.depth||x.mips!=y.mips||x.format!=y.format||x.samples!=y.samples)out|=source;
    if(a.input.binding.targets!=b.input.binding.targets||!(a.input.binding.target==b.input.binding.target)||
       a.input.known_buffer!=b.input.known_buffer||a.input.descriptor_lost!=b.input.descriptor_lost)out|=target;
    auto const& r=a.input.rectangles;auto const& s=b.input.rectangles;
    if(r.tracked!=s.tracked||!(r.viewport==s.viewport)||!(r.scissor==s.scissor)||r.viewport_count!=s.viewport_count||
       r.scissor_count!=s.scissor_count||r.depth_known!=s.depth_known||r.min_depth!=s.min_depth||r.max_depth!=s.max_depth)out|=rectangles;
    if(a.world!=b.world)out|=world;
    if(a.chain!=b.chain||a.inventory!=b.inventory||a.resize!=b.resize)out|=inventory;
    if(a.ticket!=b.ticket||a.pair_linked!=b.pair_linked||a.pair_rejection!=b.pair_rejection)out|=pair;
    if(a.input.args!=b.input.args||a.input.dimensions!=b.input.dimensions||a.input.quad_caller!=b.input.quad_caller)out|=quad;
    return out;
}
// Separate quotas prevent early incomplete entries from consuming useful rows.
struct Budget {
    static constexpr unsigned maximum_attempts=64,rejected_limit=4,complete_limit=12;
    unsigned attempted{},rejected{},complete{},skipped{},dropped{};
    bool preflight(){
        if(attempted>=maximum_attempts||(rejected==rejected_limit&&complete==complete_limit)){++skipped;return false;}
        ++attempted;return true;
    }
    int retain(bool valid){
        if(valid){if(complete==complete_limit){++dropped;return -1;}return int(rejected_limit+complete++);}
        if(rejected==rejected_limit){++dropped;return -1;}return int(rejected++);
    }
};
}
