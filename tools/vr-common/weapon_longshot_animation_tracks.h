#pragma once
#include "pose_math.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace gears_weapon::longshot_tracks {
// Constant-key Float96NoW or IntervalFixed32NoW supplied by the current
// original instance. Compressed key count may differ from NumFrames.
// The reader certifies reflected property types, format and source ownership.
struct Samples {
    std::array<std::array<gears_vr::Pose,256>,4> bones{};
    unsigned count{};
    float length{};
};
inline bool decode(std::array<std::int32_t,16> const& offsets,
    unsigned char const* bytes,std::size_t size,unsigned frames,float length,Samples& out,unsigned rotation_format=1) noexcept {
    if(!bytes||size<96||size>65536||frames<3||frames>256||
       !std::isfinite(length)||length<=0||length>10||(rotation_format!=1&&rotation_format!=3))return false;
    const int keys=offsets[15];
    if(keys<3||keys>int(frames))return false;
    struct Span {std::size_t begin{},end{};};
    std::array<Span,8> spans{};Samples result{};result.count=unsigned(keys);result.length=length;
    for(unsigned bone=0;bone<4;++bone){
        const auto translation=offsets[bone*4],translations=offsets[bone*4+1];
        const auto rotation=offsets[bone*4+2],rotations=offsets[bone*4+3];
        if(translation<0||rotation<0||(translations!=1&&translations!=keys)||
           (rotations!=1&&rotations!=keys))return false;
        const auto tb=std::size_t(translations)*12;
        const auto rotation_stride=rotations>1&&rotation_format==3?4u:12u;
        const auto qb=std::size_t(rotations)*rotation_stride+(rotations>1?24:0);
        if(std::size_t(translation)>size||tb>size-std::size_t(translation)||
           std::size_t(rotation)>size||qb>size-std::size_t(rotation))return false;
        spans[bone*2]={std::size_t(translation),std::size_t(translation)+tb};
        spans[bone*2+1]={std::size_t(rotation),std::size_t(rotation)+qb};
        std::array<float,6> range{};
        if(rotations>1){
            std::memcpy(range.data(),bytes+rotation,24);
            for(unsigned i=0;i<6;++i)if(!std::isfinite(range[i])||(i>=3&&range[i]<0))return false;
        }
        for(unsigned key=0;key<unsigned(keys);++key){
            std::array<float,3> p{};std::array<double,3> q{};
            const auto ti=translations==1?0:key,qi=rotations==1?0:key;
            std::memcpy(p.data(),bytes+translation+12*ti,12);
            const auto source=bytes+rotation+(rotations>1?24:0)+rotation_stride*qi;
            if(rotations>1&&rotation_format==3){
                std::uint32_t packed{};std::memcpy(&packed,source,4);
                q={{(double((packed>>21)&2047)/1023-1)*range[3]+range[0],
                    (double((packed>>10)&2047)/1023-1)*range[4]+range[1],
                    (double(packed&1023)/511-1)*range[5]+range[2]}};
            }else{std::array<float,3> raw{};std::memcpy(raw.data(),source,12);for(unsigned axis=0;axis<3;++axis)q[axis]=raw[axis];}
            double squared{};
            for(unsigned axis=0;axis<3;++axis){
                if(!std::isfinite(p[axis])||std::abs(p[axis])>100000||!std::isfinite(q[axis])||
                   (rotations>1&&rotation_format==1&&(q[axis]<range[axis]-1e-5f||q[axis]>range[axis]+range[axis+3]+1e-5f)))return false;
                squared+=double(q[axis])*q[axis];
            }
            if(squared>1.00001)return false;
            auto& value=result.bones[bone][key];value.position={p[0],p[1],p[2]};
            value.orientation=gears_vr::normalized({q[0],q[1],q[2],std::sqrt((std::max)(0.,1-squared))});
            if(key){const auto previous=result.bones[bone][key-1].orientation;
                auto& next=value.orientation;
                if(previous.x*next.x+previous.y*next.y+previous.z*next.z+previous.w*next.w<0)
                    next={-next.x,-next.y,-next.z,-next.w};}
        }
    }
    std::sort(spans.begin(),spans.end(),[](Span a,Span b){return a.begin<b.begin;});
    if(spans.front().begin||spans.back().end!=size)return false;
    for(unsigned i=1;i<spans.size();++i)if(spans[i].begin!=spans[i-1].end)return false;
    out=result;return true;
}
// The supported game's stored animation quaternions and its measured
// parent-relative bone matrices use inverse rotation conventions.65324
// established this on the actual stopped LS_Reload closed bolt: translation
// agreed below1um while XYZ signs were inverse and W identical. Conversion
// is fixed, never selected by whichever orientation happens to pass a gate.
inline bool native_relative_rotations(Samples& values) noexcept {
    if(values.count<3||values.count>256||!std::isfinite(values.length)||values.length<=0||values.length>10)return false;
    for(const auto& bone:values.bones)for(unsigned i=0;i<values.count;++i){
        const auto& pose=bone[i];const auto q=pose.orientation;
        const auto norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
        if(!gears_vr::finite(pose.position)||!std::isfinite(norm)||std::abs(norm-1)>.001)return false;
    }
    // Validate the entire input before changing any sample.
    for(auto& bone:values.bones)for(unsigned i=0;i<values.count;++i){auto& q=bone[i].orientation;q={-q.x,-q.y,-q.z,q.w};}
    return true;
}
// A sample includes both translation and rotation. The reference timing law
// uses SequenceLength/decoded key count, which may differ from NumFrames.
// Native time-to-bone agreement is still required before any mechanical
// completion; a successful static decode cannot certify engine interpolation.
inline bool sample(Samples const& values,unsigned bone,double time,gears_vr::Pose& out) noexcept {
    if(values.count<3||values.count>256||bone>=4||!std::isfinite(time)||
       time<0||!std::isfinite(values.length)||values.length<=0||time>values.length)return false;
    const double position=time*values.count/values.length;
    const auto first=(std::min)(unsigned(position),values.count-1);
    const auto second=(std::min)(first+1,values.count-1);
    const double t=(std::min)(1.,position-first);
    const auto& a=values.bones[bone][first];auto b=values.bones[bone][second];
    if(a.orientation.x*b.orientation.x+a.orientation.y*b.orientation.y+
       a.orientation.z*b.orientation.z+a.orientation.w*b.orientation.w<0)
        b.orientation={-b.orientation.x,-b.orientation.y,-b.orientation.z,-b.orientation.w};
    gears_vr::Pose result{};
    result.position={a.position.x+(b.position.x-a.position.x)*t,
        a.position.y+(b.position.y-a.position.y)*t,a.position.z+(b.position.z-a.position.z)*t};
    result.orientation=gears_vr::normalized({a.orientation.x+(b.orientation.x-a.orientation.x)*t,
        a.orientation.y+(b.orientation.y-a.orientation.y)*t,a.orientation.z+(b.orientation.z-a.orientation.z)*t,
        a.orientation.w+(b.orientation.w-a.orientation.w)*t});
    out=result;return true;
}
}
