#pragma once
#include "pose_math.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
// Gnasher's original three tracks. No padded synthetic fourth track and no
// Longshot timing/rotation convention is imported into this weapon.
namespace gears_weapon::gnasher_tracks {
struct Samples {
    std::array<std::array<gears_vr::Pose,256>,3> bones{};
    unsigned count{};
    float length{};
};
inline bool decode(std::array<std::int32_t,12> const& offsets,
    unsigned char const* bytes,std::size_t size,unsigned frames,float length,Samples& out,unsigned rotation_format=1) noexcept {
    if(!bytes||size<72||size>65536||frames<3||frames>256||
       !std::isfinite(length)||length<=0||length>10||(rotation_format!=1&&rotation_format!=3))return false;
    const int keys=offsets[11];
    if(keys<3||keys>int(frames))return false;
    struct Span {std::size_t begin{},end{};};
    std::array<Span,6> spans{};Samples result{};result.count=unsigned(keys);result.length=length;
    for(unsigned bone=0;bone<3;++bone){
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

inline bool chamber_shape(Samples const& values,unsigned& peak) noexcept {
    if(values.count<3||values.count>256)return false;
    double maximum{};peak=0;
    const auto home=values.bones[2][0];
    for(unsigned i=0;i<values.count;++i){
        for(unsigned bone=0;bone<2;++bone){const auto& a=values.bones[bone][i];const auto& b=values.bones[bone][0];
            if(std::hypot(std::hypot(a.position.x-b.position.x,a.position.y-b.position.y),a.position.z-b.position.z)>.002||
               std::abs(a.orientation.x*b.orientation.x+a.orientation.y*b.orientation.y+a.orientation.z*b.orientation.z+a.orientation.w*b.orientation.w)<.99999)return false;}
        const auto& p=values.bones[2][i];const auto q=p.orientation;
        if(std::hypot(std::hypot(p.position.x-home.position.x,p.position.y-home.position.y),p.position.z-home.position.z)>.002||
           std::abs(q.y)>.001||std::abs(q.z)>.001||q.x>.001)return false;
        const double angle=2*std::atan2(-q.x,std::abs(q.w));
        if(angle>maximum){maximum=angle;peak=i;}
    }
    double previous{};
    for(unsigned i=0;i<values.count;++i){const auto q=values.bones[2][i].orientation;
        const double angle=2*std::atan2(-q.x,std::abs(q.w));
        if(i&&(i<=peak?angle+.0001<previous:angle>previous+.0001))return false;
        previous=angle;
    }
    const auto first=values.bones[2][0].orientation,last=values.bones[2][values.count-1].orientation;
    return maximum>.4&&maximum<.8&&peak>0&&peak<values.count-1&&std::abs(first.x)<.001&&std::abs(last.x)<.001;
}
// Timing is deliberately explicit. A decode alone cannot certify which
// interpolation schedule the current original engine actually evaluates.
enum class Timing {uncertified,keys_minus_one,keys};
inline bool time_for_angle(Samples const& values,double angle,bool returning,Timing timing,double& out) noexcept {
    unsigned peak{};
    if((timing!=Timing::keys_minus_one&&timing!=Timing::keys)||!std::isfinite(angle)||angle<0||!chamber_shape(values,peak))return false;
    const auto at=[&](unsigned i){auto q=values.bones[2][i].orientation;return 2*std::atan2(-q.x,std::abs(q.w));};
    const auto maximum=at(peak);if(angle>maximum+.00001)return false;
    const double divisor=timing==Timing::keys_minus_one?values.count-1:values.count;
    if(angle<=1e-8){out=returning?values.length:0;return true;}
    // Search each branch independently, preserving the original down/return
    // order and its peak plateau. Do not restart SG_Chamber on a regrasp.
    unsigned a{},b{};bool found{};
    for(unsigned i=returning?peak:0;i+1<(returning?values.count:peak+1);++i){
        const auto left=at(i),right=at(i+1);
        if(returning?(left>=angle&&right<=angle):(left<=angle&&right>=angle)){
            if(std::abs(right-left)<1e-9)continue;a=i;b=i+1;found=true;break;}
    }
    if(!found){if(std::abs(angle-maximum)<1e-5){out=values.length*peak/divisor;return true;}return false;}
    auto qa=values.bones[2][a].orientation,qb=values.bones[2][b].orientation;
    double lo=0,hi=1;
    for(unsigned i=0;i<30;++i){const double t=(lo+hi)*.5;
        const auto q=gears_vr::normalized({qa.x*(1-t)+qb.x*t,qa.y*(1-t)+qb.y*t,qa.z*(1-t)+qb.z*t,qa.w*(1-t)+qb.w*t});
        const auto current=2*std::atan2(-q.x,std::abs(q.w));
        if(returning?current>angle:current<angle)lo=t;else hi=t;}
    out=values.length*(a+(lo+hi)*.5)/divisor;return std::isfinite(out)&&out>=0&&out<=values.length;
}
}
