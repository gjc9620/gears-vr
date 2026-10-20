#pragma once
#include "ui_hud_plan.h"
#include "ui_group_placement.h"
#include "ui_index_membership.h"
#include "ui_text_group_geometry.h"

namespace gears_ui_layout {
// Identity until separately tuned in the game/headset. Angular panel FOV is
// independent: element size and displacement from panel center are two controls.
struct Parameters {double element_scale=1,center_scale=1;};
inline bool valid(Parameters p){return std::isfinite(p.element_scale)&&p.element_scale>=1&&p.element_scale<=2&&
    std::isfinite(p.center_scale)&&p.center_scale>=0&&p.center_scale<=1;}
inline bool identity(Parameters p){return p.element_scale==1&&p.center_scale==1;}
using Bounds=gears_ui_group::Bounds;
using Matrix=std::array<float,16>;
using gears_ui_group::include;
using gears_ui_group::valid;
// UE row-vector native transform, then D3D NDC -> top-left normalized canvas.
// Bounds are computed after the original mapping; copied XY need not be pixels.
inline bool project(Bounds& bounds,Matrix const& matrix,double x,double y){
    if(!std::isfinite(x)||!std::isfinite(y))return false;
    for(float value:matrix)if(!std::isfinite(value))return false;
    const double cx=x*matrix[0]+y*matrix[4]+matrix[12];
    const double cy=x*matrix[1]+y*matrix[5]+matrix[13];
    const double w=x*matrix[3]+y*matrix[7]+matrix[15];
    return std::isfinite(w)&&w>0&&include(bounds,(cx/w+1)/2,(1-cy/w)/2);
}
inline bool quad_bounds(std::array<std::array<unsigned char,32>,4> const& vertices,Matrix const& matrix,Bounds& out){
    Bounds bounds{};
    for(auto const& vertex:vertices){float xyz[3]{};std::memcpy(xyz,vertex.data(),sizeof(xyz));
        if(xyz[2]!=0||!project(bounds,matrix,xyz[0],xyz[1]))return false;}
    if(!valid(bounds))return false;out=bounds;return true;
}
// Compose source p'=C+k(c-C)+s(p-c) with the same frozen-eye panel map.
// The source clip follows the group's affine map, then clips to its eye; the
// original scissor is never replaced by an unclipped full-eye rectangle.
inline bool compose(gears_viewport::DrawState const& saved,unsigned width,unsigned height,
    gears_viewport::OverlayPlan const& panel,Bounds bounds,
    Parameters p,gears_viewport::OverlayPlan& out){
    if(!panel.valid||!valid(p)||!valid(bounds)||!gears_viewport::overlay_plan(saved,width,height).valid)return false;
    if(identity(p)){out=panel;return true;}
    const double cx=(bounds.left+bounds.right)/2,cy=(bounds.top+bounds.bottom)/2;
    const double tx=.5+p.center_scale*(cx-.5)-p.element_scale*cx;
    const double ty=.5+p.center_scale*(cy-.5)-p.element_scale*cy;
    return gears_ui_group::compose(saved,width,height,panel,{p.element_scale,tx,ty},out,true);
}
inline constexpr unsigned run_capacity=256;
struct Run {unsigned first{},count{},entry{};}; // Segment-relative index offsets.
struct Segment {bool valid{};unsigned first{},count{};};
struct Groups {
    std::array<Bounds,gears_ui_receipts::entry_capacity> bounds{};
    std::array<Segment,gears_ui_indices::segment_capacity> segments{};
    std::array<Run,run_capacity> runs{};
    unsigned count{};
};
// Complete-call native geometry can span atlas payloads. Current receipt bytes
// and actual indices remain independent requirements; Canvas admits one quad.
inline bool capture(gears_ui_receipts::Claim const& claim,gears_ui_indices::Snapshot const& snapshot,Matrix const& matrix,Groups& out){
    if(!gears_ui_indices::detail::bounded_snapshot(snapshot)||claim.generation!=snapshot.generation||
       !gears_ui_indices::detail::claim_valid(claim,snapshot.payload,snapshot.vertex_count))return false;
    Groups result{};
    for(unsigned n=0;n<claim.count;++n){auto const& entry=claim.entries[n];auto const length=entry.receipt.end-entry.receipt.begin;
        if((entry.kind!=2&&entry.kind!=3)||
           ((entry.source==gears_ui_receipts::Source::CanvasText||entry.source==gears_ui_receipts::Source::NativeSubtitle)&&!entry.whole_text_group&&!entry.text_group)||
           (entry.source==gears_ui_receipts::Source::Canvas&&length!=4))continue;
        auto& bounds=result.bounds[n];bool supported=true;
        if(entry.text_group){auto const& group=*entry.text_group;
            if(!gears_ui_receipts::text_group_range_valid(group,entry.receipt,entry.root_invocation,entry.source,
                   entry.text_group_offset,length)||matrix!=group.matrix)continue;
            for(unsigned j=0;j<group.vertex_count;++j)if(!project(bounds,matrix,group.positions[j][0],group.positions[j][1])){supported=false;break;}
        }
        for(unsigned j=0;j<length;++j){float position[4]{};
            std::memcpy(position,claim.vertices->data[entry.vertex_offset+j].data(),sizeof(position));
            if(position[2]!=0||position[3]!=1||!project(bounds,matrix,position[0],position[1])||
               (entry.text_group&&entry.text_group->positions[entry.text_group_offset+j]!=std::array<float,2>{position[0],position[1]})){supported=false;break;}}
        if(!supported||!valid(bounds))bounds={};
    }
    for(unsigned ordinal=0;ordinal<snapshot.segment_count;++ordinal){auto const& source=snapshot.segments[ordinal];
        if(!source.index_count||source.unknown||!source.kind_mask||(source.kind_mask&~6u))continue;
        const unsigned begin=result.count;bool supported=true;std::uint64_t native_root{};
        gears_ui_receipts::TextGroupGeometry const* native_group=nullptr;
        for(unsigned i=0;i<source.index_count;i+=3){unsigned owner=unsigned(claim.count);
            for(unsigned j=0;j<3;++j){const auto global=snapshot.global_vertices[source.index_offset+i+j];unsigned found=unsigned(claim.count);
                for(unsigned n=0;n<claim.count;++n)if(global>=claim.entries[n].receipt.begin&&global<claim.entries[n].receipt.end){found=n;break;}
                if(found==claim.count||!valid(result.bounds[found])||(j&&owner!=found)){supported=false;break;}owner=found;}
            if(!supported)break;
            auto const& entry=claim.entries[owner];
            if(entry.source==gears_ui_receipts::Source::NativeSubtitle){
                if(native_root&&(native_root!=entry.root_invocation||native_group!=entry.text_group.get())){supported=false;break;}
                native_root=entry.root_invocation;native_group=entry.text_group.get();
            }
            if(result.count>begin&&result.runs[result.count-1].entry==owner)result.runs[result.count-1].count+=3;
            else {if(result.count==run_capacity){supported=false;break;}result.runs[result.count++]={i,3,owner};}
        }
        if(supported)result.segments[ordinal]={true,begin,result.count-begin};else result.count=begin;
    }
    out=result;return true;
}
struct ReplayRun {unsigned first{},count{};gears_viewport::OverlayPlan plan{};};
struct Replay {unsigned count{};std::array<ReplayRun,run_capacity> runs{};};
// Full preflight before any GPU draw; failure leaves caller's old replay intact.
inline bool prepare(Groups const& groups,unsigned ordinal,gears_viewport::DrawState const& saved,
    unsigned width,unsigned height,gears_viewport::OverlayPlan const& panel,
    Parameters p,Replay& out){
    if(identity(p)||!valid(p)||ordinal>=groups.segments.size())return false;
    auto const segment=groups.segments[ordinal];
    if(!segment.valid||!segment.count||segment.first>groups.count||segment.count>groups.count-segment.first||groups.count>run_capacity)return false;
    Replay result{};
    for(unsigned i=0;i<segment.count;++i){auto const run=groups.runs[segment.first+i];auto& target=result.runs[i];
        if(run.entry>=groups.bounds.size()||!run.count||run.first%3||run.count%3||
           !compose(saved,width,height,panel,groups.bounds[run.entry],p,target.plan))return false;
        target.first=run.first;target.count=run.count;
    }
    result.count=segment.count;out=result;return true;
}
template<class Viewport,class Scissor,class Draw>
unsigned replay(gears_viewport::DrawState const& saved,Replay const& plans,Viewport viewport,Scissor scissor,Draw draw){
    if(!plans.count||plans.count>run_capacity)return 0;
    struct Restore {gears_viewport::DrawState const& saved;Viewport& viewport;Scissor& scissor;
        ~Restore(){viewport(saved.viewport,saved.min_depth,saved.max_depth);scissor(saved.scissor);}} restore{saved,viewport,scissor};
    unsigned mask=0;
    for(unsigned eye=0;eye<2;++eye){for(unsigned i=0;i<plans.count;++i){auto const& run=plans.runs[i];
        viewport(run.plan.viewports[eye],saved.min_depth,saved.max_depth);scissor(run.plan.scissors[eye]);draw(run.first,run.count);}mask|=1u<<eye;}
    return mask;
}
}
