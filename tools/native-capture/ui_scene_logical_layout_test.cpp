#include "ui_scene_logical_layout.h"
#include <cassert>
#include <cstdio>
using namespace gears_ui_scene_layout;
struct Row {
    struct Child {uint64_t id{};uintptr_t self{},callee{},caller{},script{};long long enter{},exit{};
        unsigned body_calls{},script_size{};bool returned{},current_after{};};
    unsigned kind=1,call_count=3,child_count=2,call_overflow{},child_overflow{},simple_executes{},material_executes{},unclassified_canvas{};
    uint64_t token=9;uintptr_t owner=12,root_frame=0x7100,fcanvas=0x1000;
    long long enter=1,exit=20;bool before_ok=true,returned=true,root_current=true,delta_ok=true;
    std::array<gears_ui_logical_group::Call,64> calls{};std::array<Child,128> children{};
    gears_ui_scene_batch::Delta delta;
};
static std::array<unsigned char,80> matrix(){std::array<unsigned char,80> result{};
    const float m[16]{2,0,0,0,0,-2,0,0,0,0,1,0,-1,1,0,1};std::memcpy(result.data(),m,sizeof(m));return result;}
static void quad(gears_ui_scene_batch::Range& r,float l,float t,float right,float b){
    for(auto p:{std::array<float,4>{l,t,0,1},{right,t,0,1},{l,b,0,1},{right,b,0,1}}){
        gears_ui_batch::Vertex v{};std::memcpy(v.data(),p.data(),sizeof(p));r.vertices.push_back(v);}r.end+=4;
}
static Row fixture(){Row row;row.delta.fcanvas=row.fcanvas;
    gears_ui_scene_batch::Range atlas{};atlas.key=7;atlas.item=0x4000;atlas.payload=0x5000;atlas.transform=matrix();
    quad(atlas,.2f,.1f,.4f,.2f);quad(atlas,.05f,.9f,.1f,.95f);row.delta.simple.push_back(atlas);
    atlas.item=0x4100;atlas.payload=0x5100;atlas.key=2;atlas.vertices.clear();atlas.end=0;
    quad(atlas,.4f,.1f,.6f,.2f);row.delta.simple.push_back(atlas);
    gears_ui_material::Receipt material{};material.fcanvas=row.fcanvas;material.item=0x4200;material.payload=0x5200;
    material.key=19;material.proxy=0x9900;material.transform=matrix();
    const float record[8]{.6f,.4f,.2f,.2f,0,0,1,1};std::memcpy(material.record.data(),record,sizeof(record));row.delta.material.push_back(material);
    row.children[0]={101,row.owner,44,row.root_frame,0x8000,2,10,2,100,true,true};
    row.children[1]={202,row.owner,44,row.root_frame,0x8000,11,19,1,100,true,true};
    for(unsigned i=0;i<row.call_count;++i){auto& call=row.calls[i];call.ancestry_complete=call.append_complete=true;
        call.count=2;call.frames[0]={0x7000,{44,row.owner,0x8001,row.root_frame}};
        call.frames[1]={row.root_frame,{55,row.owner,0x9001,0}};
        call.logical_count=1;call.logical_ids[0]=i==2?202:101;}
    row.calls[0].member_count=2;row.calls[0].members[0]={row.fcanvas,0x4000,0x5000,7,0,4,false};
    row.calls[0].members[1]={row.fcanvas,0x4100,0x5100,2,0,4,false};
    row.calls[1].member_count=1;row.calls[1].members[0]={row.fcanvas,0x4200,0x5200,19,0,0,true};
    row.calls[2].member_count=1;row.calls[2].members[0]={row.fcanvas,0x4000,0x5000,7,4,8,false};return row;
}
int main(){auto row=fixture();const gears_ui_group::Bounds target{.2,.3,.8,.8,true};Candidate candidate;
    assert(fit_invocation(row,101,target,1,candidate));assert(candidate.root_token==9&&candidate.invocation==101&&candidate.function==44);
    assert(candidate.calls==2&&candidate.geometry.simple.size()==2&&candidate.geometry.material.size()==1);
    assert(candidate.geometry.simple[0].begin==0&&candidate.geometry.simple[0].end==4&&candidate.geometry.simple[0].vertices.size()==4);
    assert(std::abs(candidate.bounds.left-.2)<1e-6&&std::abs(candidate.bounds.bottom-.6)<1e-6);
    assert(fit_invocation(row,202,target,1,candidate));assert(candidate.calls==1&&candidate.geometry.simple.size()==1&&candidate.geometry.material.empty());
    assert(candidate.geometry.simple[0].begin==4&&std::abs(candidate.bounds.top-.9)<1e-6);
    auto reject=[&](Row const& value,uint64_t id=101){candidate.root_token=999;
        assert(!fit_invocation(value,id,target,1,candidate)&&candidate.root_token==999);};
    reject(row,0);reject(row,404);
    for(unsigned fault=0;fault<22;++fault){auto bad=row;
        switch(fault){
        case 0:bad.kind=2;break; // A PathChoice scene is not a WarHUD invocation.
        case 1:bad.root_current=false;break;
        case 2:bad.returned=false;break;
        case 3:bad.simple_executes=1;break;
        case 4:bad.material_executes=1;break;
        case 5:bad.unclassified_canvas=1;break;
        case 6:bad.call_overflow=1;break;
        case 7:bad.child_overflow=1;break;
        case 8:bad.children[0].body_calls=3;break;
        case 9:bad.children[0].current_after=false;break;
        case 10:bad.children[0].exit=30;break;
        case 11:bad.calls[0].append_complete=false;break;
        case 12:bad.calls[0].frames[0].fields.code=0x8100;break;
        case 13:bad.calls[0].frames[0].fields.previous=0x7777;break;
        case 14:bad.calls[0].logical_count=0;break;
        case 15:bad.calls[2].members[0].begin=0;break; // Foreign invocation aliases selected geometry.
        case 16:bad.calls[1].members[0].payload=0x9999;break;
        case 17:bad.calls[0].members[0].begin=1;break;
        case 18:bad.delta.material[0].proxy=0;break;
        case 19:bad.children[1].id=101;break;
        case 20:bad.call_count=65;break;
        case 21:bad.calls[0].ancestry_complete=false;break;
        }reject(bad);
    }
    // Exact nested references can be deduplicated; partial overlap cannot.
    row.calls[0].members[2]=row.calls[0].members[0];row.calls[0].member_count=3;
    assert(fit_invocation(row,101,target,1,candidate)&&candidate.geometry.simple.size()==2);
    row.calls[0].members[2].end=8;reject(row);
    row=fixture();candidate.root_token=999;
    assert(!fit_invocation(row,101,{0,-.1,1,.8,true},1,candidate)&&candidate.root_token==999);
    assert(fit_invocation(row,101,target,1,candidate));
    for(auto point:{std::array<double,2>{candidate.bounds.left,candidate.bounds.top},{candidate.bounds.right,candidate.bounds.bottom}}){
        const double x=point[0]*candidate.recipe.scale+candidate.recipe.translation_x;
        const double y=point[1]*candidate.recipe.scale+candidate.recipe.translation_y;
        assert(x>=target.left-1e-12&&x<=target.right+1e-12&&y>=target.top-1e-12&&y<=target.bottom+1e-12);}
    std::puts("UiSceneLogicalLayoutTest passed: unique invocation, multi-atlas/material fit and atomic rejection");
}
