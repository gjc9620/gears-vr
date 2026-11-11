#pragma once
#include "cover_physx_shape_geometry.h"
// Owned replay of the retained original 1402cfa6c descriptor builder.
// The caller must supply that creation call's scale, NOT infer it from a pose
// or substitute the point-query scale. No current module/ownership attestation.
namespace gears_cover_physx_shape_creation {
namespace g=gears_cover_physx_shape_geometry;using Kind=g::Kind;
struct Input {Kind kind{Kind::unknown};std::array<float,16> tm{};
    std::array<float,3> dimensions{},scale{};unsigned no_rb_collision{};};
struct Descriptor {Kind kind{Kind::unknown};g::w::math::Matrix local{};
    std::array<float,3> half{};float radius{},full_segment_length{};};
enum class Code {converted,alias,disabled,unsupported,pose,value};
inline Code convert(Input const& input,Descriptor& output)noexcept{
    if(g::w::s::aliases(&input,sizeof input,&output,sizeof output))return Code::alias;
    const auto kind=input.kind;
    if(kind!=Kind::sphere&&kind!=Kind::box&&kind!=Kind::capsule)return Code::unsupported;
    if(input.no_rb_collision&1u)return Code::disabled;
    constexpr float factor=.02f,margin=.025f;
    std::array<float,3> scaled{};
    for(unsigned k=0;k<3;++k){if(!g::w::math::scalar(input.scale[k])||input.scale[k]<=0)return Code::value;
      scaled[k]=factor*input.scale[k];if(!g::w::math::scalar(scaled[k])||scaled[k]<=0)return Code::value;}
    Descriptor next{};next.kind=kind;
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c){
      // Original UE rows -> PhysX columns. Capsule first negates UE row X
      // and swaps UE rows Y/Z. Its new local Y therefore follows old Z.
      const unsigned source=kind==Kind::capsule?(c==1?2:c==2?1:0):c;
      const float value=input.tm[4*source+r];next.local.values[3*r+c]=(kind==Kind::capsule&&c==0)?-value:value;
    }
    for(unsigned k=0;k<3;++k)next.local.values[9+k]=input.tm[12+k]*scaled[k];
    if(!g::w::math::rigid(next.local))return Code::pose;
    const unsigned count=kind==Kind::box?3:kind==Kind::capsule?2:1;
    for(unsigned k=0;k<count;++k){const float x=input.dimensions[k];
      if(!g::w::math::scalar(x)||x<0||(k==0&&x==0)|| (kind==Kind::box&&x==0))return Code::value;}
    if(kind==Kind::box){for(unsigned k=0;k<3;++k){const float value=scaled[0]*input.dimensions[k];
      const float half=value*.5f;next.half[k]=half+margin;if(!g::w::math::scalar(next.half[k])||next.half[k]<=0)return Code::value;}}
    else{const float value=scaled[0]*input.dimensions[0];next.radius=value+margin;
      if(!g::w::math::scalar(next.radius)||next.radius<=0)return Code::value;
      if(kind==Kind::capsule){next.full_segment_length=scaled[0]*input.dimensions[1];
        if(!g::w::math::scalar(next.full_segment_length)||(input.dimensions[1]!=0&&next.full_segment_length==0))return Code::value;}}
    output=next;return Code::converted;
}
}
