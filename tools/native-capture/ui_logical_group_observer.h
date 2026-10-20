#pragma once
#include "ui_scene_batch.h"
#include "ui_group_placement.h"
#include "ui_call_boundary.h"
#include "ui_observer.h"

// Buffered CPU evidence only. No receipt produced here authorizes replay or
// proves GPU target/ticket/scissor, UObject lifetime or input hit regions.
namespace gears_ui_logical_group {
inline constexpr unsigned call_limit=64,frame_limit=12;
inline constexpr unsigned member_limit=32;
// Exact UObject/name-slot evidence for an observed FFrame node. This is not an
// executable-address lookup and never turns a name into hook authorization.
template<class Read> bool identity(gears_ui::Config const& c,Read read,std::uintptr_t at,gears_ui::Identity& out){
    gears_ui::Reader<Read> r{read};gears_ui::Registry objects{},names{};gears_ui::Identity id{};id.address=at;
    std::uintptr_t entry{};
    if(!gears_ui::registry(r,c.objects_header,objects)||!gears_ui::registry(r,c.names_header,names)||
       !r.field(id.index,at,c.object_internal_index)||!r.field(id.name_index,at,c.object_name)||
       !r.field(id.number,at,c.object_name+4)||!r.field(id.cls,at,c.object_class)||!r.field(id.outer,at,c.object_outer)||
       id.name_index>=unsigned(names.count)||!r.field(entry,names.data,id.name_index*8)||!entry)return false;
    bool terminated=false;for(unsigned i=0;i<id.name.size();++i){
        if(!r.field(id.name[i],entry,c.name_entry_text+i*2))return false;
        if(!id.name[i]){terminated=true;break;}}
    if(!terminated||!gears_ui::fresh_identity(c,r,objects,names,id))return false;
    out=id;return true;
}
struct Member {std::uintptr_t fcanvas{},item{},payload{};std::int32_t key{};unsigned begin{},end{};bool material{};};
struct Frame {std::uintptr_t address{};gears_ui_call_boundary::Frame fields{};};
struct Call {unsigned draw{},scope{};std::uintptr_t canvas{},frame{},function{},matched_frame{};
    unsigned count{},member_count{};bool ancestry_complete{},append_complete{};
    unsigned logical_count{};std::array<std::uint64_t,frame_limit> logical_ids{};
    std::array<Frame,frame_limit> frames{};std::array<Member,member_limit> members{};};
inline bool contains(gears_ui_scene_batch::Delta const& delta,Member const& member){
    if(member.fcanvas!=delta.fcanvas||!member.item||!member.payload)return false;
    if(member.material){for(auto const& m:delta.material)if(m.item==member.item&&m.payload==member.payload&&m.key==member.key)return true;
    }else if(member.end>member.begin){for(auto const& r:delta.simple)
        if(r.item==member.item&&r.payload==member.payload&&r.key==member.key&&member.begin>=r.begin&&member.end<=r.end)return true;}
    return false;
}
inline bool mapped(gears_ui_scene_batch::Delta const& delta,Call const& call){
    if(!call.append_complete||call.member_count>member_limit)return false;
    for(unsigned i=0;i<call.member_count;++i)if(!contains(delta,call.members[i]))return false;
    return true;
}
template<class Read> bool ancestry(Read read,std::uintptr_t frame,std::uintptr_t root,Call& out){
    Call result=out;result.count=0;result.ancestry_complete=false;
    for(unsigned i=0;i<frame_limit;++i){
        if(!frame)return false;
        for(unsigned j=0;j<i;++j)if(result.frames[j].address==frame)return false;
        auto& item=result.frames[i];item.address=frame;
        if(!gears_ui_call_boundary::frame(read,frame,item.fields))return false;
        ++result.count;if(frame==root){result.ancestry_complete=true;break;}frame=item.fields.previous;
    }
    if(!result.ancestry_complete)return false;
    // Same-thread sequential double read, not an engine atomic snapshot.
    for(unsigned i=0;i<result.count;++i){gears_ui_call_boundary::Frame again{};
        auto const& f=result.frames[i];
        if(!gears_ui_call_boundary::frame(read,f.address,again)||again.node!=f.fields.node||
           again.object!=f.fields.object||again.code!=f.fields.code||again.previous!=f.fields.previous)return false;
    }
    out=result;return true;
}
inline bool project(gears_ui_group::Bounds& bounds,std::array<unsigned char,80> const& transform,double x,double y){
    std::array<float,16> m{};std::memcpy(m.data(),transform.data(),64);
    for(float f:m)if(!std::isfinite(f))return false;
    if(!std::isfinite(x)||!std::isfinite(y))return false;
    const double w=x*m[3]+y*m[7]+m[15];
    if(!std::isfinite(w)||w<=0)return false;
    return gears_ui_group::include(bounds,((x*m[0]+y*m[4]+m[12])/w+1)/2,
        (1-(x*m[1]+y*m[5]+m[13])/w)/2);
}
// The copied material record is the certified native XY/size/UV quad used by
// ui_material_batch.h::quad. Negative extents retain their native orientation.
inline bool bounds(gears_ui_scene_batch::Delta const& delta,gears_ui_group::Bounds& out){
    if(!delta.fcanvas||delta.simple.size()+delta.material.size()>gears_ui_scene_batch::item_limit)return false;
    gears_ui_group::Bounds result{};unsigned vertices=0;
    for(auto const& range:delta.simple){
        if(!range.item||!range.payload||range.end<=range.begin||range.end-range.begin!=range.vertices.size()||
           range.vertices.size()%4||range.vertices.size()>gears_ui_scene_batch::vertex_limit-vertices)return false;
        vertices+=unsigned(range.vertices.size());
        for(auto const& vertex:range.vertices){float p[4]{};std::memcpy(p,vertex.data(),sizeof(p));
            if(p[2]!=0||p[3]!=1||!project(result,range.transform,p[0],p[1]))return false;}
    }
    for(auto const& material:delta.material){
        if(material.fcanvas!=delta.fcanvas||!material.item||!material.payload||!material.proxy||material.kind)return false;
        float p[8]{};std::memcpy(p,material.record.data(),sizeof(p));for(float f:p)if(!std::isfinite(f))return false;
        for(double x:{double(p[0]),double(p[0])+p[2]})for(double y:{double(p[1]),double(p[1])+p[3]})
            if(!project(result,material.transform,x,y))return false;
    }
    if(!gears_ui_group::valid(result))return false;out=result;return true;
}
// Fixed ring retains recent evidence, rather than consuming all capacity on
// initial empty HUD frames before a later pickup prompt appears.
template<class T,unsigned Capacity=8> struct Ring {
    static_assert(Capacity>0&&Capacity<=32);
    std::array<T,Capacity> rows{};unsigned count{},next{};std::uint64_t overwritten{};
    void append(T value){rows[next]=std::move(value);next=(next+1)%Capacity;
        if(count<Capacity)++count;else ++overwritten;}
    T const& oldest(unsigned i)const{return rows[(next+Capacity-count+i)%Capacity];}
};
}
