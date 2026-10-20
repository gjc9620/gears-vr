#pragma once
#include "ui_logical_group_observer.h"

// CPU layout candidate only. Consumes copied live-observer evidence; it grants
// no UObject, GPU replay, input-region or pickup-semantic authorization.
namespace gears_ui_scene_layout {
using Member=gears_ui_logical_group::Member;
struct Candidate {
    uint64_t root_token{},invocation{};uintptr_t owner{},function{};
    unsigned calls{};gears_ui_scene_batch::Delta geometry;
    gears_ui_group::Bounds bounds{};gears_ui_group::Recipe recipe{};
};
inline bool same_resource(Member const& a,Member const& b){
    return a.fcanvas==b.fcanvas&&a.item==b.item&&a.payload==b.payload;
}
inline bool overlaps(Member const& a,Member const& b){
    return same_resource(a,b)&&(a.material||b.material||(a.begin<b.end&&b.begin<a.end));
}
inline bool same(Member const& a,Member const& b){return same_resource(a,b)&&a.key==b.key&&
    a.material==b.material&&a.begin==b.begin&&a.end==b.end;}
// Row has the existing ui_logical_group_live::Row shape. No observer hooks,
// names or function addresses are added by this adapter. In particular, the
// same UFunction called twice must be selected by a different invocation ID.
template<class Row> bool fit_invocation(Row const& root,uint64_t invocation,
    gears_ui_group::Bounds target,double preferred_scale,Candidate& out){
    if(root.kind!=1||!root.token||!root.owner||!root.root_frame||!root.fcanvas||
       !root.before_ok||!root.returned||!root.root_current||!root.delta_ok||
       root.exit<root.enter||root.simple_executes||root.material_executes||root.unclassified_canvas||
       root.call_overflow||root.child_overflow||root.call_count>root.calls.size()||
       root.child_count>root.children.size()||root.delta.fcanvas!=root.fcanvas||!invocation)return false;
    const typename Row::Child* child=nullptr;
    for(unsigned i=0;i<root.child_count;++i)if(root.children[i].id==invocation){
        if(child)return false;child=&root.children[i];}
    if(!child||child->self!=root.owner||!child->callee||!child->caller||!child->script||!child->script_size||
       !child->returned||!child->current_after||child->enter<root.enter||child->exit<child->enter||child->exit>root.exit)return false;
    gears_ui_group::Bounds all_bounds{};
    if(!gears_ui_logical_group::bounds(root.delta,all_bounds))return false;
    std::vector<Member> selected,others;unsigned selected_calls=0;
    for(unsigned i=0;i<root.call_count;++i){auto const& call=root.calls[i];bool matches=false,body=false;
        if(!call.ancestry_complete||!call.count||call.count>call.frames.size()||
           call.frames[call.count-1].address!=root.root_frame||call.logical_count>call.logical_ids.size()||
           call.member_count>call.members.size()||!gears_ui_logical_group::mapped(root.delta,call))return false;
        for(unsigned j=0;j<call.logical_count;++j){if(!call.logical_ids[j])return false;
            for(unsigned k=0;k<j;++k)if(call.logical_ids[k]==call.logical_ids[j])return false;
            if(call.logical_ids[j]==invocation)matches=true;}
        for(unsigned j=0;j<call.count;++j){auto const& frame=call.frames[j];auto const& f=frame.fields;
            if(!frame.address||!f.node||!f.object||!f.code)return false;
            for(unsigned k=0;k<j;++k)if(call.frames[k].address==frame.address)return false;
            if(j+1<call.count&&f.previous!=call.frames[j+1].address)return false;
            if(f.node==child->callee&&f.object==child->self&&f.previous==child->caller&&
               f.code>=child->script&&f.code-child->script<child->script_size)body=true;}
        if(matches){if(!body)return false;++selected_calls;}
        for(unsigned j=0;j<call.member_count;++j){auto const& member=call.members[j];
            if(member.material&&(member.begin||member.end))return false;
            (matches?selected:others).push_back(member);}
    }
    if(!selected_calls||selected_calls!=child->body_calls||selected.empty())return false;
    Candidate result{};result.root_token=root.token;result.invocation=invocation;
    result.owner=child->self;result.function=child->callee;result.calls=selected_calls;result.geometry.fcanvas=root.fcanvas;
    for(size_t i=0;i<selected.size();++i){auto const& member=selected[i];bool duplicate=false;
        for(auto const& other:others)if(overlaps(member,other))return false;
        for(size_t j=0;j<i;++j)if(overlaps(member,selected[j])){
            if(!same(member,selected[j]))return false;duplicate=true;}
        if(duplicate)continue;
        unsigned found=0;
        if(member.material){for(auto const& source:root.delta.material)
            if(source.item==member.item&&source.payload==member.payload&&source.key==member.key){
                result.geometry.material.push_back(source);++found;}
        }else{for(auto const& source:root.delta.simple)
            if(source.item==member.item&&source.payload==member.payload&&source.key==member.key&&
               source.begin<=member.begin&&member.end<=source.end){
                if((member.begin-source.begin)%4||(member.end-member.begin)%4)return false;
                auto range=source;range.begin=member.begin;range.end=member.end;
                range.vertices.assign(source.vertices.begin()+member.begin-source.begin,
                    source.vertices.begin()+member.end-source.begin);
                result.geometry.simple.push_back(std::move(range));++found;}}
        if(found!=1)return false;
    }
    if(!gears_ui_logical_group::bounds(result.geometry,result.bounds)||
       !gears_ui_group::fit(result.bounds,target,preferred_scale,result.recipe))return false;
    out=std::move(result);return true;
}
}
