#pragma once
#include <array>
#include <cmath>

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
