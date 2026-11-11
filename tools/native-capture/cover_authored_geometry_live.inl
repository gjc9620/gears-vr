#ifndef GEARS_COVER_AUTHORED_GEOMETRY_LIVE_INCLUDED
#define GEARS_COVER_AUTHORED_GEOMETRY_LIVE_INCLUDED
#include "cover_authored_inventory_live.inl"
// Pure owned-buffer join. Caller separately closes current roles, inventory,
// source code and this SAME C0 frame. No reads, calls or publication authority.
namespace cover_authored_geometry_live {
namespace live=cover_authored_inventory_live;namespace a=gears_cover_authored_array;
namespace g=a::g;namespace roles=cover_physics_roles_live;namespace b=weapon_hand_scene_binding;
enum class Code {converted,request,alias,source,inventory,unsupported,raw,mapping,geometry};
struct Result {g::names::Mapping mapping{};g::Geometry geometry{};
    // Preserve bNoRBCollision without assuming its query eligibility semantics.
    std::array<bool,g::max_primitives> no_rb_collision{};std::array<unsigned,g::max_primitives> collision_word{};};
struct Workspace {std::array<g::names::Input,64> names{};std::array<g::Shape,g::max_primitives> shapes{};
    g::Workspace conversion{};Result candidate{};g::names::Status mapping_code{};g::Code geometry_code{};};
inline bool decoded_equal(a::Row const& x,a::Row const& y)noexcept{
    const auto& p=x.shape.authored;const auto& q=y.shape.authored;
    return x.shape.array_index==y.shape.array_index&&p.body_index==q.body_index&&g::names::same(p.bone,q.bone)&&p.kind==q.kind&&
      !std::memcmp(&p.tm,&q.tm,sizeof p.tm)&&!std::memcmp(p.raw_dimensions.data(),q.raw_dimensions.data(),sizeof p.raw_dimensions)&&
      !std::memcmp(p.receiver_scale.data(),q.receiver_scale.data(),sizeof p.receiver_scale)&&
      x.collision_disabled==y.collision_disabled&&x.collision_word==y.collision_word;
}
inline Code convert(roles::Snapshot const& typed,live::Snapshot const& inventory,g::body::Model const& model,
    g::body::Frame const& frame,Workspace& work,Result& output)noexcept{
    const g::body::d::Range ranges[]={g::body::d::range(&typed,sizeof typed),g::body::d::range(&inventory,sizeof inventory),
      g::body::d::range(&model,sizeof model),g::body::d::range(&frame,sizeof frame),g::body::d::range(&work,sizeof work),g::body::d::range(&output,sizeof output)};
    if(!g::body::d::disjoint(ranges,6))return Code::alias;
    const auto count=inventory.membership.count;
    if(!count||count>64||!inventory.count||inventory.count>g::max_primitives)return Code::request;
    if(typed.array.count!=int(count)||typed.root<0||typed.root>=int(count)||
       !b::same(typed.mesh,inventory.membership.mesh)||!b::same(typed.asset,inventory.membership.asset)||
       !b::same(typed.instance,inventory.membership.instance)||!b::same(typed.mesh,inventory.scaling.mesh)||
       inventory.membership.array.count!=int(count))return Code::source;
    std::array<float,3> scale{};
    if(gears_cover_primitive_metrics::point_scale(inventory.scaling.input,scale)!=gears_cover_primitive_metrics::Code::computed||
       scale!=inventory.scaling.value)return Code::source;
    unsigned at{};work.candidate={};
    for(unsigned n=0;n<count;++n){const auto& member=inventory.membership.rows[n];
      if(typed.bodies[n].index!=int(n)||typed.bodies[n].owner!=typed.mesh.address)return Code::source;
      work.names[n]={n,member.bone};
      if(inventory.inventory[n].count[3])return Code::unsupported;
      for(unsigned k=0;k<4;++k){const auto& header=inventory.headers[n][k];
        const auto expected=inventory.inventory[n].count[k];
        if(expected>64||header.count!=int(expected)||header.capacity<header.count||header.capacity>256||
           (!header.data?(header.count||header.capacity):!roles::query_source::pointer(header.data,k==3?1:std::size_t((std::max)(1,header.capacity))*a::stride)))return Code::inventory;
        if(k==3)continue;
        if(expected>inventory.count-at)return Code::inventory;
        const auto offset=0x6cu+16*k;if(member.setup.address>UINTPTR_MAX-offset)return Code::source;
        a::Source source{};source.header=member.setup.address+offset;source.body_index=n;source.bone=member.bone;
        source.kind=static_cast<g::metrics::Kind>(k);source.reflected_extent=k==0?72:k==1?80:76;
        source.collision_mask=inventory.collision_masks[n][k];source.receiver_scale=scale;
        if(!a::valid_source(source))return Code::source;
        for(unsigned i=0;i<expected;++i){const auto& row=inventory.rows[at];a::Row decoded{};
          if(a::decode(source,row.raw,i,decoded)!=a::Code::captured||!decoded_equal(decoded,row))return Code::raw;
          work.shapes[at]=decoded.shape;work.candidate.no_rb_collision[at]=decoded.collision_disabled;
          work.candidate.collision_word[at]=decoded.collision_word;++at;
        }
      }
    }
    if(at!=inventory.count)return Code::inventory;
    for(unsigned n=count;n<64;++n){for(unsigned k=0;k<4;++k)if(inventory.inventory[n].count[k])return Code::inventory;}
    work.mapping_code=g::names::bind(model,frame,work.names.data(),count,unsigned(typed.root),work.candidate.mapping);
    if(work.mapping_code!=g::names::Status::okay)return Code::mapping;
    work.geometry_code=g::convert(model,frame,work.candidate.mapping,inventory.inventory.data(),count,
      work.shapes.data(),at,work.conversion,work.candidate.geometry);
    if(work.geometry_code!=g::Code::converted)return Code::geometry;
    output=work.candidate;return Code::converted;
}
}
#endif
