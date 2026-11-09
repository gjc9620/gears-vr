#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

// Pure float32 arithmetic from the retained original primitive hit consumers.
// The caller must FIRST authenticate kind, reflected field correspondence and
// this consumer's incoming scalar. No matrix scale is inferred here. Results
// stay in original game units and certify no pose, active body, sweep or hit.
namespace gears_cover_primitive_metrics {
enum class Kind {sphere,box,sphyl,convex};
enum class Code {computed,invalid,unsupported,numeric};
struct Input {
    Kind kind{};
    float receiver_scalar{};
    // Sphere: Radius,0,0. Box: X,Y,Z. Sphyl: Radius,Length,0.
    std::array<float,3> raw{};
};
struct Metrics {
    Kind kind{};
    float radius{},half_segment{};
    std::array<float,3> box_half_extents{};
};
// Scale producer in retained 0x1403a6c34 point-query route. These are caller-
// authenticated current values corresponding to component +1E8/+1EC..1F4
// and, when present, owner +198/+19C..1A4. No native offsets are read here.
struct PointScale {
    float component_scalar{};std::array<float,3> component_xyz{};
    bool owner_present{};float owner_scalar{};std::array<float,3> owner_xyz{};
};
inline Code point_scale(PointScale const& in,std::array<float,3>& out) noexcept {
    const auto a=reinterpret_cast<std::uintptr_t>(&in),b=reinterpret_cast<std::uintptr_t>(&out);
    constexpr auto limit=(std::numeric_limits<std::uintptr_t>::max)();
    if(sizeof(in)-1>limit-a||sizeof(out)-1>limit-b||
       (a<=b+sizeof(out)-1&&b<=a+sizeof(in)-1))return Code::invalid;
    if(!std::isfinite(in.component_scalar)||in.component_scalar<=0||
       (in.owner_present&&(!std::isfinite(in.owner_scalar)||in.owner_scalar<=0)))return Code::invalid;
    std::array<float,3> next{};
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(in.component_xyz[i])||in.component_xyz[i]<=0||
           (in.owner_present&&(!std::isfinite(in.owner_xyz[i])||in.owner_xyz[i]<=0)))return Code::invalid;
        // Original vmulss component scalar*axis, owner scalar*axis, then the
        // two products. Do not reassociate through overflow/underflow.
        const float component=in.component_scalar*in.component_xyz[i];
        if(!std::isfinite(component)||component<=0)return Code::numeric;
        next[i]=component;
        if(in.owner_present){const float owner=in.owner_scalar*in.owner_xyz[i];
            if(!std::isfinite(owner)||owner<=0)return Code::numeric;
            next[i]=owner*component;
            if(!std::isfinite(next[i])||next[i]<=0)return Code::numeric;
        }
    }
    out=next;return Code::computed;
}
inline Code compute(Input const& in,Metrics& out) noexcept {
    if(in.kind==Kind::convex||unsigned(in.kind)>unsigned(Kind::convex))return Code::unsupported;
    if(!std::isfinite(in.receiver_scalar)||in.receiver_scalar<=0)return Code::invalid;
    for(float value:in.raw)if(!std::isfinite(value))return Code::invalid;
    if(in.raw[0]<=0)return Code::invalid;
    if(in.kind==Kind::box){if(in.raw[1]<=0||in.raw[2]<=0)return Code::invalid;}
    else if(in.raw[2]!=0||(in.kind==Kind::sphere?in.raw[1]!=0:in.raw[1]<0))return Code::invalid;
    Metrics next{};next.kind=in.kind;
    // Original SSE does scalar*0.5 FIRST for box extents / sphyl length.
    // Multiplying scalar*dimension first can overflow when this order does not.
    const float half_scalar=in.receiver_scalar*.5f;
    if(in.kind==Kind::box){
        for(unsigned i=0;i<3;++i){next.box_half_extents[i]=half_scalar*in.raw[i];
            if(!std::isfinite(next.box_half_extents[i])||next.box_half_extents[i]<=0)return Code::numeric;}
    }else{
        next.radius=in.receiver_scalar*in.raw[0];
        if(!std::isfinite(next.radius)||next.radius<=0)return Code::numeric;
        if(in.kind==Kind::sphyl){
            next.half_segment=half_scalar*in.raw[1];
            if(!std::isfinite(next.half_segment)||(in.raw[1]>0&&next.half_segment<=0))return Code::numeric;
        }
    }
    out=next;return Code::computed;
}
} // namespace gears_cover_primitive_metrics
