#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace gears_weapon::part_geometry {
// Frozen offline-engine-input.v2 corner format. These are temporary INPUT
// records for the original vertex initializers, never fabricated UObjects or
// native GPU buffer layouts. Source bytes/hashes are checked by the loader.
struct Vertex {
    std::array<float,3> position{};
    std::array<std::uint8_t,4> tangent_x{},tangent_y{},normal{};
    std::array<std::array<float,2>,4> uv{};
    std::uint32_t color{};
};
static_assert(sizeof(Vertex)==60&&offsetof(Vertex,tangent_x)==12&&
    offsetof(Vertex,tangent_y)==16&&offsetof(Vertex,normal)==20&&
    offsetof(Vertex,uv)==24&&offsetof(Vertex,color)==56);
struct Section {
    std::uint32_t material{},first_index{},triangles{},min_vertex{},max_vertex{};
};
struct Input {
    const std::uint8_t* vertices{};std::size_t vertex_bytes{};
    const std::uint8_t* indices{};std::size_t index_bytes{};
    const Section* sections{};std::size_t section_count{};
};
struct Conversion {
    // Row-major map from authored local metres to this engine's local axes.
    // The live owner certifies units/basis against its current body mount.
    std::array<std::array<double,3>,3> axes{};
    double units_per_metre{};
    std::uint64_t calibration_generation{};
    bool current_basis_verified{};
};
struct Bounds {
    std::array<float,3> centre{},extent{};float radius{};
};
struct Prepared {
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
    std::vector<Section> sections;
    Bounds bounds{};
    std::uint64_t calibration_generation{};
    bool winding_reversed{};
};
enum class Result {ready,invalid_input,invalid_conversion,invalid_vertex,
    invalid_indices,invalid_sections};
using V=std::array<double,3>;
inline double dot(V const& a,V const& b) noexcept {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline V cross(V const& a,V const& b) noexcept {
    return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
inline V rotate(Conversion const& c,V const& v) noexcept {
    return {dot(c.axes[0],v),dot(c.axes[1],v),dot(c.axes[2],v)};
}
inline bool normalise(V& value) noexcept {
    const double size=std::sqrt(dot(value,value));
    if(!std::isfinite(size)||size<.5||size>1.5)return false;
    for(auto& v:value)v/=size;
    return true;
}
inline bool unpack(std::array<std::uint8_t,4> const& packed,V& out) noexcept {
    for(unsigned i=0;i<3;++i)out[i]=double(packed[i])/127.5-1.;
    return normalise(out);
}
inline std::array<std::uint8_t,4> pack(V const& value,std::uint8_t extra) noexcept {
    std::array<std::uint8_t,4> out{};
    for(unsigned i=0;i<3;++i)out[i]=static_cast<std::uint8_t>(
        std::lround((std::clamp(value[i],-1.,1.)+1.)*127.5));
    out[3]=extra;return out; // Original initializer computes final tangent sign.
}
inline bool conversion_valid(Conversion const& c,bool& reverse) noexcept {
    if(!c.current_basis_verified||!c.calibration_generation||
       !std::isfinite(c.units_per_metre)||c.units_per_metre<10||c.units_per_metre>500)return false;
    for(unsigned i=0;i<3;++i){
        for(auto v:c.axes[i])if(!std::isfinite(v))return false;
        for(unsigned j=0;j<=i;++j)
            if(std::abs(dot(c.axes[i],c.axes[j])-(i==j?1.:0.))>1e-6)return false;
    }
    const double determinant=dot(c.axes[0],cross(c.axes[1],c.axes[2]));
    if(std::abs(std::abs(determinant)-1.)>1e-6)return false;
    reverse=determinant<0;return true;
}
inline Result prepare(Input const& input,Conversion const& c,Prepared& out) {
    bool reverse{};
    if(!conversion_valid(c,reverse))return Result::invalid_conversion;
    // Native position/tangent initializers internally allocate 2*N. Retain N
    // input vertices/indices; reject sizes that would overflow 16-bit storage.
    if(!input.vertices||!input.indices||!input.sections||!input.vertex_bytes||
       input.vertex_bytes%sizeof(Vertex)||!input.index_bytes||input.index_bytes%6||
       input.vertex_bytes/sizeof(Vertex)>32767||input.index_bytes/2>196602||
       !input.section_count||input.section_count>32)return Result::invalid_input;
    Prepared next{};next.calibration_generation=c.calibration_generation;next.winding_reversed=reverse;
    next.vertices.resize(input.vertex_bytes/sizeof(Vertex));
    next.indices.resize(input.index_bytes/2);
    std::memcpy(next.vertices.data(),input.vertices,input.vertex_bytes);
    std::memcpy(next.indices.data(),input.indices,input.index_bytes);
    V low{},high{};bool first=true;
    for(auto& vertex:next.vertices){
        V p{};for(unsigned i=0;i<3;++i){
            p[i]=vertex.position[i];if(!std::isfinite(p[i])||std::abs(p[i])>100.)return Result::invalid_vertex;
        }
        for(const auto& uv:vertex.uv)for(auto v:uv)
            if(!std::isfinite(v)||std::abs(v)>100000.)return Result::invalid_vertex;
        V x{},y{},n{};
        if(!unpack(vertex.tangent_x,x)||!unpack(vertex.tangent_y,y)||!unpack(vertex.normal,n)||
           std::abs(dot(x,y))>.035||std::abs(dot(x,n))>.035||std::abs(dot(y,n))>.035||
           std::abs(dot(cross(x,y),n))<.995)return Result::invalid_vertex;
        const auto tx=rotate(c,x),ty=rotate(c,y),tn=rotate(c,n);
        vertex.tangent_x=pack(tx,vertex.tangent_x[3]);vertex.tangent_y=pack(ty,vertex.tangent_y[3]);
        vertex.normal=pack(tn,vertex.normal[3]);
        p=rotate(c,p);
        for(unsigned i=0;i<3;++i){
            p[i]*=c.units_per_metre;vertex.position[i]=static_cast<float>(p[i]);
            // Bounds describe the SAME quantised positions handed to native.
            p[i]=vertex.position[i];
            if(first)low[i]=high[i]=p[i];
            else{low[i]=(std::min)(low[i],p[i]);high[i]=(std::max)(high[i],p[i]);}
        }
        first=false;
    }
    for(auto index:next.indices)if(index>=next.vertices.size())return Result::invalid_indices;
    if(reverse)for(std::size_t i=0;i<next.indices.size();i+=3)std::swap(next.indices[i+1],next.indices[i+2]);
    std::size_t covered=0;
    for(std::size_t i=0;i<input.section_count;++i){
        const auto& section=input.sections[i];
        if(section.material>31||!section.triangles||section.first_index!=covered||
           std::uint64_t(section.triangles)*3>next.indices.size()-covered||
           section.min_vertex>section.max_vertex||section.max_vertex>=next.vertices.size())return Result::invalid_sections;
        unsigned min=65535,max=0;
        const auto end=covered+std::size_t(section.triangles)*3;
        for(auto j=covered;j<end;++j){min=(std::min)(min,unsigned(next.indices[j]));max=(std::max)(max,unsigned(next.indices[j]));}
        if(section.min_vertex!=min||section.max_vertex!=max)return Result::invalid_sections;
        next.sections.push_back(section);covered=end;
    }
    if(covered!=next.indices.size())return Result::invalid_sections;
    V centre{};
    for(unsigned i=0;i<3;++i){
        centre[i]=(high[i]+low[i])*.5;
        next.bounds.centre[i]=static_cast<float>(centre[i]);
        const auto bound=(std::max)(std::abs(high[i]-next.bounds.centre[i]),
            std::abs(low[i]-next.bounds.centre[i]));
        next.bounds.extent[i]=std::nextafter(static_cast<float>(bound),std::numeric_limits<float>::infinity());
    }
    double radius=0;
    for(const auto& vertex:next.vertices){
        V delta{};for(unsigned i=0;i<3;++i)delta[i]=double(vertex.position[i])-next.bounds.centre[i];
        radius=(std::max)(radius,std::sqrt(dot(delta,delta)));
    }
    // Round outward: a floating-point radius must never cull its own vertices.
    next.bounds.radius=std::nextafter(static_cast<float>(radius),std::numeric_limits<float>::infinity());
    if(!std::isfinite(next.bounds.radius)||next.bounds.radius<=0)return Result::invalid_vertex;
    out=std::move(next);return Result::ready;
}
// Join parts that share EXACTLY the same local-to-world transform. Inputs
// have already undergone their one authored-to-engine conversion: preserve
// every vertex byte and material slot, rebase only indices/section ranges.
// This is not suitable for articulated parts while they are moving.
template<std::size_t N>
inline Result merge_rigid(std::array<Prepared,N> const& parts,Prepared& out) {
    static_assert(N>0);
    Prepared next{};next.calibration_generation=parts[0].calibration_generation;
    next.winding_reversed=parts[0].winding_reversed;
    if(!next.calibration_generation)return Result::invalid_conversion;
    for(auto const& part:parts){
        if(&part==&out||part.calibration_generation!=next.calibration_generation||
           part.winding_reversed!=next.winding_reversed)return Result::invalid_conversion;
        if(part.vertices.empty()||part.indices.empty()||part.indices.size()%3||part.sections.empty()||
           part.vertices.size()>32767-next.vertices.size()||
           part.indices.size()>196602-next.indices.size()||
           part.sections.size()>32-next.sections.size())return Result::invalid_input;
        const auto vertex_base=static_cast<std::uint32_t>(next.vertices.size());
        const auto index_base=static_cast<std::uint32_t>(next.indices.size());
        for(auto index:part.indices){
            if(index>=part.vertices.size())return Result::invalid_indices;
            next.indices.push_back(static_cast<std::uint16_t>(index+vertex_base));
        }
        std::size_t covered{};
        for(auto section:part.sections){
            if(section.material>31||!section.triangles||section.first_index!=covered||
               std::uint64_t(section.triangles)*3>part.indices.size()-covered||
               section.min_vertex>section.max_vertex||section.max_vertex>=part.vertices.size())return Result::invalid_sections;
            unsigned low=65535,high=0;
            const auto end=covered+std::size_t(section.triangles)*3;
            for(auto i=covered;i<end;++i){low=(std::min)(low,unsigned(part.indices[i]));high=(std::max)(high,unsigned(part.indices[i]));}
            if(section.min_vertex!=low||section.max_vertex!=high)return Result::invalid_sections;
            section.first_index+=index_base;section.min_vertex+=vertex_base;section.max_vertex+=vertex_base;
            next.sections.push_back(section);covered=end;
        }
        if(covered!=part.indices.size())return Result::invalid_sections;
        next.vertices.insert(next.vertices.end(),part.vertices.begin(),part.vertices.end());
    }
    V low{},high{};bool first=true;
    for(auto const& vertex:next.vertices){
        for(unsigned i=0;i<3;++i){const double v=vertex.position[i];
            if(!std::isfinite(v))return Result::invalid_vertex;
            if(first)low[i]=high[i]=v;
            else{low[i]=(std::min)(low[i],v);high[i]=(std::max)(high[i],v);}
        }
        first=false;
    }
    for(unsigned i=0;i<3;++i){
        next.bounds.centre[i]=static_cast<float>((high[i]+low[i])*.5);
        const auto extent=(std::max)(std::abs(high[i]-next.bounds.centre[i]),std::abs(low[i]-next.bounds.centre[i]));
        next.bounds.extent[i]=std::nextafter(static_cast<float>(extent),std::numeric_limits<float>::infinity());
    }
    double radius{};
    for(auto const& vertex:next.vertices){V delta{};
        for(unsigned i=0;i<3;++i)delta[i]=double(vertex.position[i])-next.bounds.centre[i];
        radius=(std::max)(radius,std::sqrt(dot(delta,delta)));
    }
    next.bounds.radius=std::nextafter(static_cast<float>(radius),std::numeric_limits<float>::infinity());
    if(!std::isfinite(next.bounds.radius)||next.bounds.radius<=0)return Result::invalid_vertex;
    out=std::move(next);return Result::ready;
}
} // namespace gears_weapon::part_geometry
