#pragma once
#include "cover_physx_actor_backlink.h"
#include "cover_physx_query_source.h"
#include "cover_physx_shape_tables.h"
#include <atomic>
// Exact pinned original Image/current owner required. This inventory is not a
// lifetime/world fence or a self filter. No original method is executed.
namespace gears_cover_physx_actor_shapes {
namespace s=gears_cover_physx_query_source;namespace raw=s::pose;
constexpr unsigned capacity=64;
enum class Code {observed,request,guard,read,module,links,capacity,changed};
struct Row {std::uintptr_t internal{},public_shape{},table{},parent{};};
struct Snapshot {
    std::uintptr_t actor{},scene{},body{},root{},root_table{};unsigned type{},count{};
    std::array<std::uintptr_t,3> internal_array{},public_array{};
    std::array<Row,capacity> rows{};
};
inline bool same(Snapshot const& a,Snapshot const& b){
    if(a.actor!=b.actor||a.scene!=b.scene||a.body!=b.body||a.root!=b.root||a.root_table!=b.root_table||a.type!=b.type||a.count!=b.count||a.internal_array!=b.internal_array||a.public_array!=b.public_array)return false;
    for(unsigned n=0;n<capacity;++n)if(a.rows[n].internal!=b.rows[n].internal||a.rows[n].public_shape!=b.rows[n].public_shape||a.rows[n].table!=b.rows[n].table||a.rows[n].parent!=b.rows[n].parent)return false;return true;
}
struct Workspace {Snapshot first{},last{};std::atomic<bool> active{};};
namespace detail {
inline bool in_image(std::uintptr_t image_base,std::uint32_t size,std::uintptr_t at,std::size_t n){return at>=image_base&&at-image_base<size&&n<=size-(at-image_base);}
template<class Read>bool slot(Read& read,std::uintptr_t table,unsigned off,std::uintptr_t expected){std::uintptr_t fn{};return raw::field(read,fn,table,off)&&fn==expected;}
inline bool array(std::array<std::uintptr_t,3> const& v,unsigned& count){
    if(!v[0]){count=0;return !v[1]&&!v[2];}
    if(v[1]<v[0]||v[2]<v[1]||((v[1]-v[0])&7)||((v[2]-v[0])&7)||(v[2]-v[0])/8>256||!s::pointer(v[0],std::size_t((v[2]-v[0])?v[2]-v[0]:8)))return false;
    if((v[1]-v[0])/8>capacity)return false;count=unsigned((v[1]-v[0])/8);return true;
}
template<class Read>Code sample(std::uintptr_t image_base,std::uint32_t size,std::uintptr_t actor,std::uintptr_t scene,Read& read,Snapshot& out){
    out={};out.actor=actor;out.scene=scene;std::uintptr_t table{},actual{},reverse{};
    if(!raw::field(read,table,actor,0)||!raw::field(read,actual,actor,0x38)||!raw::field(read,out.body,actor,0x40)||!raw::field(read,out.root,actor,0x48))return Code::read;
    if(table!=image_base+0x2330a8||actual!=scene)return Code::links;
    if(!slot(read,table,0x98,image_base+0x135810)||!slot(read,table,0xa0,image_base+0x135b10))return Code::module;
    if(out.body&&!s::pointer(out.body,0x18))return Code::links;
    namespace back=gears_cover_physx_actor_backlink;
    const auto relation=back::sample(image_base,actor,read,reverse);
    if(relation==back::Code::read)return Code::read;
    if(relation==back::Code::unsupported)return Code::module;
    if(relation!=back::Code::current)return Code::links;
    if(!out.root)return Code::observed;
    if(!s::pointer(out.root,0x170)||!raw::field(read,out.root_table,out.root,0)||!raw::field(read,out.type,out.root,0x168))return Code::read;
    if(!in_image(image_base,size,out.root_table,0x48)||!slot(read,out.root_table,8,image_base+0xcb80))return Code::module;
    if(out.type==9){
        if(out.root_table!=image_base+0x227950||!slot(read,out.root_table,0x38,image_base+0x28a90)||!slot(read,out.root_table,0x398,image_base+0x28a60))return Code::module;
        for(unsigned n=0;n<3;++n)if(!raw::field(read,out.internal_array[n],out.root,0x1b0+n*8)||!raw::field(read,out.public_array[n],out.root,0x1d8+n*8))return Code::read;
        unsigned public_count{};if(!array(out.internal_array,out.count)||!array(out.public_array,public_count))return Code::capacity;
        if(out.count!=public_count)return Code::links;
    }else{
        if(out.type>8||!slot(read,out.root_table,0x38,image_base+0xc4f0))return Code::module;out.count=1;
    }
    for(unsigned n=0;n<out.count;++n){auto& row=out.rows[n];row.internal=out.root;
        if(out.type==9&&(!raw::field(read,row.internal,out.internal_array[0],n*8)||!raw::field(read,row.public_shape,out.public_array[0],n*8)))return Code::read;
        if(!s::pointer(row.internal,0xd8))return Code::links;
        std::uintptr_t internal_table{},body{},back{};
        if(!raw::field(read,internal_table,row.internal,0)||!raw::field(read,back,row.internal,0xa0)||!raw::field(read,body,row.internal,0xd0))return Code::read;
        if(!in_image(image_base,size,internal_table,0x48)||!slot(read,internal_table,0x40,image_base+0xc4e0))return Code::module;
        if(out.type==9){if(back!=row.public_shape||!raw::field(read,row.parent,row.internal,8)||row.parent!=out.root)return Code::links;}else row.public_shape=back;
        if(body!=out.body||!s::pointer(row.public_shape,0x40))return Code::links;
        if(!raw::field(read,row.table,row.public_shape,0)||!raw::field(read,back,row.public_shape,0x30)||back!=row.internal||
           !raw::field(read,back,row.public_shape,0x38)||back!=actor)return Code::links;
        bool supported=false;for(auto rva:gears_cover_physx_shape_types::tables)supported=supported||row.table==image_base+rva;
        if(!supported||!slot(read,row.table,8,image_base+0x13e730))return Code::module;
        for(unsigned k=0;k<n;++k)if(out.rows[k].internal==row.internal||out.rows[k].public_shape==row.public_shape)return Code::links;
    }return Code::observed;
}
}
// CodeGuard must certify the r91 original getters/count/dispatch bytes and
// current Image bounds, in addition to the admitted owner. Separate so this
// reader cannot silently treat metadata reads as executable-code authority.
template<class Read,class CodeGuard>Code observe(std::uintptr_t image_base,std::uint32_t size,std::uintptr_t actor,std::uintptr_t scene,Read read,CodeGuard guard,Workspace& work,Snapshot& output)noexcept{
    if(!image_base||(image_base&0xffff)||size<0x277490||size>UINTPTR_MAX-image_base||!s::pointer(actor,0x50)||!s::pointer(scene,8)||s::aliases(&work,sizeof work,&output,sizeof output))return Code::request;
    bool expected=false;if(!work.active.compare_exchange_strong(expected,true))return Code::request;
    struct Finish{std::atomic<bool>& active;~Finish(){active=false;}}finish{work.active};
    try{if(!guard())return Code::guard;auto code=detail::sample(image_base,size,actor,scene,read,work.first);if(code!=Code::observed)return code;
        if(!guard())return Code::guard;code=detail::sample(image_base,size,actor,scene,read,work.last);if(code!=Code::observed)return code;
        if(!same(work.first,work.last))return Code::changed;if(!guard())return Code::guard;output=work.first;return Code::observed;
    }catch(...){return Code::read;}
}
}
