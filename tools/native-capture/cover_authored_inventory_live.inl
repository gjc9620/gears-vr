#ifndef GEARS_COVER_AUTHORED_INVENTORY_LIVE_INCLUDED
#define GEARS_COVER_AUTHORED_INVENTORY_LIVE_INCLUDED
#include "cover_point_scale_live.inl"
namespace cover_authored_inventory_live {
namespace b=weapon_hand_scene_binding;namespace roles=cover_physics_roles_live;
namespace scale=cover_point_scale_live;namespace asset=cover_asset_setup_live;
namespace setup=cover_body_setup_live;namespace a=gears_cover_authored_array;namespace g=a::g;
enum class Code {captured,request,alias,busy,guard,metadata,layout,array,capacity,unsupported,changed,exception};
struct Snapshot {asset::Snapshot membership{};scale::Snapshot scaling{};unsigned count{};
    std::array<g::Inventory,64> inventory{};std::array<std::array<roles::pose::ArrayHeader,4>,64> headers{};
    std::array<std::array<unsigned,3>,64> collision_masks{};
    std::array<a::Row,g::max_primitives> rows{};};
// Heap/TLS owned by caller; no large inventory/proof arrays on the game stack.
struct Workspace {bool busy{};unsigned failed_body{~0u},failed_kind{~0u};a::Code array_code{};
    b::Proof membership{},producer{};std::array<b::Proof,64> shapes{};
    std::array<std::array<setup::Layout,3>,64> layouts{};
    a::Workspace array_work{};a::Snapshot array_result{};Snapshot first{},last{};};
inline bool same(Snapshot const& x,Snapshot const& y){
    if(x.count>g::max_primitives||!asset::same(x.membership,y.membership)||!scale::same(x.scaling,y.scaling)||x.count!=y.count)return false;
    for(unsigned n=0;n<64;++n){if(x.inventory[n].count!=y.inventory[n].count||x.collision_masks[n]!=y.collision_masks[n])return false;
      for(unsigned k=0;k<4;++k)if(!roles::pose::header_equal(x.headers[n][k],y.headers[n][k]))return false;}
    for(unsigned n=0;n<x.count;++n)if(x.rows[n].raw!=y.rows[n].raw||x.rows[n].shape.array_index!=y.rows[n].shape.array_index||
      x.rows[n].shape.authored.body_index!=y.rows[n].shape.authored.body_index||x.rows[n].shape.authored.kind!=y.rows[n].shape.authored.kind)return false;
    return true;
}
inline bool aggregate_schema(b::Proof& proof,setup::Layout const& layout,b::Property& convex){
    const auto& type=layout.array.aggregate;
    if(!cover_struct_field_live::select(proof,type,72,L"ConvexElems",L"ArrayProperty",16,48,nullptr,convex))return false;
    b::Property render{};std::uintptr_t opaque_type_address{};gears_anchor::Identity pointer{};unsigned extent{};
    if(!cover_struct_field_live::select(proof,type,72,L"RenderInfo",L"StructProperty",8,64,nullptr,render)||
       !b::source_field(proof,opaque_type_address,render.id.address,0xd0)||!b::certified(proof,opaque_type_address,pointer)||
       !b::named(pointer,L"Pointer")||!b::kind(proof,pointer,L"ScriptStruct")||
       !b::source_field(proof,extent,opaque_type_address,0x88)||extent!=8)return false;
    // Exact aggregate member set; RenderInfo is opaque and never dereferenced.
    std::uintptr_t child{};if(!b::source_field(proof,child,type.address,0x80))return false;
    unsigned count{};while(child&&count<5){gears_anchor::Identity id{};
      if(!b::certified(proof,child,id)||!b::source_field(proof,child,id.address,0x68))return false;++count;}
    if(child||count!=5)return false;
    std::uintptr_t element_pointer{},target{};gears_anchor::Identity inner_id{},element{};unsigned dimension{},stride{},offset{};
    return b::source_field(proof,element_pointer,convex.id.address,0xd0)&&b::certified(proof,element_pointer,inner_id)&&
      !inner_id.number&&inner_id.outer==convex.id.address&&b::kind(proof,inner_id,L"StructProperty")&&
      b::source_field(proof,dimension,element_pointer,0x70)&&dimension==1&&
      b::source_field(proof,stride,element_pointer,0x74)&&stride>0&&stride<=1024&&
      b::source_field(proof,offset,element_pointer,0x94)&&!offset&&
      b::source_field(proof,target,element_pointer,0xd0)&&b::certified(proof,target,element)&&
      b::named(element,L"KConvexElem")&&element.outer==layout.aggregate.owner.address&&b::kind(proof,element,L"ScriptStruct")&&
      b::source_field(proof,extent,target,0x88)&&extent==stride;
}
inline bool proofs_current(Workspace const& work,unsigned count){
    if(!b::metadata_current()||!b::current(work.membership)||!b::current(work.producer))return false;
    for(unsigned n=0;n<count;++n)if(!b::current(work.shapes[n]))return false;return true;
}
template<class Read> Code pass(roles::Snapshot const& typed,Read read,Workspace& work,Snapshot& out){
    out={};if(!asset::prepare(work.membership,typed,out.membership)||!scale::prepare(work.producer,typed.mesh,out.scaling))return Code::layout;
    for(unsigned n=0;n<out.membership.count;++n){work.failed_body=n;auto& proof=work.shapes[n];
      const auto& member=out.membership.rows[n];
      for(unsigned k=0;k<3;++k){work.failed_kind=k;
        if(!setup::prepare(proof,member.setup,member.bone,k,work.layouts[n][k])||
           work.layouts[n][k].aggregate.offset!=0x6c)return Code::layout;}
      b::Property convex{};auto const& first=work.layouts[n][0];work.failed_kind=3;
      if(!aggregate_schema(proof,first,convex))return Code::layout;
      auto& header=out.headers[n][3];const auto base_address=first.aggregate_base;
      if(base_address>UINTPTR_MAX-convex.offset||
         !b::source_field(proof,header.data,base_address,convex.offset)||
         !b::source_field(proof,header.count,base_address,convex.offset+8)||
         !b::source_field(proof,header.capacity,base_address,convex.offset+12))return Code::array;
      if(header.count<0||header.count>64||header.capacity<header.count||header.capacity>256||
         (!header.data?(header.count||header.capacity):!roles::query_source::pointer(header.data,1)))return Code::array;
      out.inventory[n].count[3]=unsigned(header.count);
      if(header.count)return Code::unsupported;
      for(unsigned k=0;k<3;++k){work.failed_kind=k;auto const& layout=work.layouts[n][k];a::Source source{};
        if(layout.aggregate_base>UINTPTR_MAX-layout.array.field.offset)return Code::array;
        source.header=layout.aggregate_base+layout.array.field.offset;source.body_index=n;source.bone=member.bone;
        source.kind=static_cast<g::metrics::Kind>(k);source.reflected_extent=layout.array.element.extent;
        source.collision_mask=layout.array.element.collision_mask;source.receiver_scale=out.scaling.value;
        out.collision_masks[n][k]=source.collision_mask;
        // No external guard under binding::mutex. All source work here is
        // synchronous read-only; the outer typed guard brackets both passes.
        auto current=[&](a::Source const&){return b::metadata_current()&&b::current(proof);};
        work.array_code=scale::capture(out.scaling,work.producer,out.membership,work.membership,layout,proof,source,
          read,current,work.array_work,work.array_result);
        if(work.array_code!=a::Code::captured)return Code::array;
        const auto& array=work.array_result;out.headers[n][k]=array.header;out.inventory[n].count[k]=array.count;
        if(array.count>g::max_primitives-out.count)return Code::capacity;
        for(unsigned i=0;i<array.count;++i)out.rows[out.count++]=array.rows[i];
      }
    }
    return proofs_current(work,out.membership.count)?Code::captured:Code::changed;
}
template<class Read,class Guard> Code capture(roles::Snapshot const& typed,Read read,Guard guard,Workspace& work,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard,roles::Snapshot const&>,"Full current typed roles guard required");
    const g::body::d::Range ranges[]={g::body::d::range(&typed,sizeof typed),g::body::d::range(&work,sizeof work),g::body::d::range(&output,sizeof output)};
    if(!g::body::d::disjoint(ranges,3))return Code::alias;if(work.busy)return Code::busy;
    if(typed.array.count<=0||typed.array.count>64)return Code::request;
    work.busy=true;struct Release{Workspace& w;~Release(){w.busy=false;}} release{work};
    work.failed_body=work.failed_kind=~0u;const auto expected=typed;
    try{
      if(!guard(expected))return Code::guard;
      {
        std::unique_lock<std::mutex> lock(b::mutex,std::try_to_lock);if(!lock.owns_lock())return Code::metadata;
        if(!b::context_ready()||!b::metadata_current())return Code::metadata;
        const auto generation=b::metadata_generation;const auto metadata=b::metadata_bytes;
        work.membership={};work.producer={};for(auto& p:work.shapes)p={};
        auto code=pass(expected,read,work,work.first);if(code!=Code::captured)return code;
        code=pass(expected,read,work,work.last);if(code!=Code::captured)return code;
        if(!same(work.first,work.last)||!proofs_current(work,work.first.membership.count)||
           generation!=b::metadata_generation||metadata!=b::metadata_bytes)return Code::changed;
      }
      if(!guard(expected))return Code::guard;
      if(!roles::same(typed,expected))return Code::changed;
      output=work.last;work.failed_body=work.failed_kind=~0u;return Code::captured;
    }catch(...){return Code::exception;}
}
}
#endif
