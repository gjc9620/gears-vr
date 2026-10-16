#pragma once
#include "ui_canvas_text_batch.h"
#include "ui_payload_receipts.h"
#include <cmath>

namespace gears_ui_text_group {
// The existing batched-item constructor initializes payload+0x80 before the
// item enters FCanvas. Freeze/recheck every member matrix; Execute must match it
// again. A rejected group leaves ordinary receipts on their fallback route.
template<class Read> gears_ui_receipts::TextGroup capture(gears_ui_text_batch::Delta const& delta,
    std::uint64_t root,Read read){
    using namespace gears_ui_receipts;
    if(!root||delta.canvas||!delta.fcanvas||delta.ranges.size()<2||
       delta.ranges.size()>text_group_range_capacity||delta.vertices.empty()||delta.vertices.size()>vertex_capacity)return {};
    std::shared_ptr<TextGroupGeometry> group;
    try{group=std::make_shared<TextGroupGeometry>();}catch(std::bad_alloc const&){return {};}
    group->root_invocation=root;group->fcanvas=delta.fcanvas;group->sort_key=delta.sort_key;
    group->vertex_count=unsigned(delta.vertices.size());group->range_count=unsigned(delta.ranges.size());
    gears_ui_batch::detail::Reader<Read> reader{read};unsigned offset=0;
    for(unsigned i=0;i<group->range_count;++i){auto const& range=delta.ranges[i];
        if(!range.item||!range.payload||range.end<=range.begin||range.end>1048576||
           (range.end-range.begin)%4||range.vertex_offset!=offset||range.end-range.begin>group->vertex_count-offset)return {};
        for(unsigned j=0;j<i;++j)if(delta.ranges[j].item==range.item||delta.ranges[j].payload==range.payload)return {};
        std::array<float,16> matrix{};
        if(!reader.field(matrix,range.payload,0x80))return {};
        for(float value:matrix)if(!std::isfinite(value))return {};
        if(i&&matrix!=group->matrix)return {};group->matrix=matrix;
        group->ranges[i]={{0,delta.fcanvas,range.item,range.payload,delta.sort_key,range.begin,range.end},offset};
        offset+=range.end-range.begin;
    }
    if(offset!=group->vertex_count)return {};
    for(unsigned i=0;i<offset;++i){float position[4]{};std::memcpy(position,delta.vertices[i].data(),sizeof(position));
        if(!std::isfinite(position[0])||!std::isfinite(position[1])||position[2]!=0||position[3]!=1)return {};
        group->positions[i]={position[0],position[1]};}
    for(auto const& range:delta.ranges){std::array<float,16> again{};
        if(!reader.field(again,range.payload,0x80)||again!=group->matrix)return {};}
    return group;
}
}
