#pragma once
#include "../vr-common/weapon_lancer_authored_volume.h"
#include <array>

// Pure cosmetic wire geometry for one selected workbench acquisition volume.
// This creates no game reads, colliders, grants, writers or acquisition permit.
namespace lancer_debug_volume {
namespace vr=gears_vr;
namespace a=gears_authored_contact_volume;
struct Segment {vr::Vector start{},end{};};
struct Wire {std::array<Segment,64> lines{};unsigned count{};};

inline vr::Vector add(vr::Vector x,vr::Vector y) noexcept {return {x.x+y.x,x.y+y.y,x.z+y.z};}
inline vr::Vector mul(vr::Vector x,double s) noexcept {return {x.x*s,x.y*s,x.z*s};}
inline double dot(vr::Vector x,vr::Vector y) noexcept {return x.x*y.x+x.y*y.y+x.z*y.z;}
inline double length(vr::Vector x) noexcept {return std::sqrt(dot(x,x));}
inline vr::Vector workbench_to_asset(vr::Vector wb) noexcept {
    return {-100.*wb.x,-100.*wb.z,100.*wb.y};
}
inline vr::Vector asset_to_workbench(vr::Vector raw) noexcept {
    return gears_lancer_authored_volume::asset_point_to_workbench(raw);
}
inline bool matrix_valid(std::array<float,16> const& m) noexcept {
    for(float x:m)if(!std::isfinite(x))return false;
    if(std::abs(m[3])+std::abs(m[7])+std::abs(m[11])+std::abs(m[15]-1.f)>1e-4f)return false;
    for(unsigned i=0;i<3;++i){const double s=std::hypot(std::hypot(double(m[i*4]),double(m[i*4+1])),double(m[i*4+2]));
        if(!std::isfinite(s)||s<.1||s>10)return false;}
    return true;
}
inline bool raw_part_point(vr::Vector raw,std::array<double,16> const& delta,vr::Vector& out) noexcept {
    for(double n:delta)if(!std::isfinite(n))return false;
    out={raw.x*delta[0]+raw.y*delta[4]+raw.z*delta[8]+delta[12],
         raw.x*delta[1]+raw.y*delta[5]+raw.z*delta[9]+delta[13],
         raw.x*delta[2]+raw.y*delta[6]+raw.z*delta[10]+delta[14]};
    return vr::finite(out);
}
inline bool edge(Wire& w,vr::Vector x,vr::Vector y) noexcept {
    if(w.count>=w.lines.size()||!vr::finite(x)||!vr::finite(y)||length({x.x-y.x,x.y-y.y,x.z-y.z})<1e-7)return false;
    w.lines[w.count++]={x,y};return true;
}
inline bool volume_wire(a::Volume const& volume,double units,double model_scale,
    std::array<float,16> const& mesh,bool current_part,std::array<double,16> const& delta,
    Wire& out) noexcept {
    out={};
    if(!a::valid(volume)||!std::isfinite(units)||units<=0||!std::isfinite(model_scale)||model_scale<=0||
       !matrix_valid(mesh))return false;
    const double mesh_scale=(std::hypot(std::hypot(double(mesh[0]),double(mesh[1])),double(mesh[2]))+
        std::hypot(std::hypot(double(mesh[4]),double(mesh[5])),double(mesh[6]))+
        std::hypot(std::hypot(double(mesh[8]),double(mesh[9])),double(mesh[10])))/3.;
    if(!std::isfinite(mesh_scale)||std::abs(mesh_scale-model_scale)>1e-3)return false;
    const auto local_to_world=[&](vr::Vector local,vr::Vector& world)noexcept {
        const auto rotated=vr::transform(vr::rotation_matrix(volume.local.orientation),local);
        const auto raw=workbench_to_asset(add(volume.local.position,rotated));
        vr::Vector moved=raw;
        if(current_part&&!raw_part_point(raw,delta,moved))return false;
        world={double(mesh[0])*moved.x+double(mesh[4])*moved.y+double(mesh[8])*moved.z+double(mesh[12]),
               double(mesh[1])*moved.x+double(mesh[5])*moved.y+double(mesh[9])*moved.z+double(mesh[13]),
               double(mesh[2])*moved.x+double(mesh[6])*moved.y+double(mesh[10])*moved.z+double(mesh[14])};
        return vr::finite(world);
    };
    if(volume.shape==a::Shape::sphere){
        // Three orthogonal great circles, eight segments each.
        for(unsigned plane=0;plane<3;++plane)for(unsigned i=0;i<8;++i){
            const auto at=[&](unsigned k){const double angle=6.28318530717958647692*k/8.;
                const double c=volume.radius*std::cos(angle),s=volume.radius*std::sin(angle);
                return plane==0?vr::Vector{c,s,0}:plane==1?vr::Vector{c,0,s}:vr::Vector{0,c,s};};
            vr::Vector p{},q{};if(!local_to_world(at(i),p)||!local_to_world(at(i+1),q)||!edge(out,p,q))return false;
        }
        return true;
    }
    if(volume.shape==a::Shape::box){
        const std::array<vr::Vector,8> corners{{{+volume.half.x,+volume.half.y,+volume.half.z},
            {-volume.half.x,+volume.half.y,+volume.half.z},{-volume.half.x,-volume.half.y,+volume.half.z},
            {+volume.half.x,-volume.half.y,+volume.half.z},{+volume.half.x,+volume.half.y,-volume.half.z},
            {-volume.half.x,+volume.half.y,-volume.half.z},{-volume.half.x,-volume.half.y,-volume.half.z},
            {+volume.half.x,-volume.half.y,-volume.half.z}}};
        std::array<vr::Vector,8> world{};
        for(unsigned i=0;i<8;++i)if(!local_to_world(corners[i],world[i]))return false;
        constexpr unsigned edges[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},
            {6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        for(auto e:edges)if(!edge(out,world[e[0]],world[e[1]]))return false;
        return true;
    }
    if(volume.shape==a::Shape::capsule){
        constexpr double pi=3.14159265358979323846;
        const auto ring=[&](double y,unsigned count,double x_scale,double z_scale)noexcept {
            for(unsigned i=0;i<count;++i){vr::Vector p{},q{};
                const double a=2.*pi*i/count,b=2.*pi*(i+1)/count;
                if(!local_to_world({volume.radius*x_scale*std::cos(a),y,volume.radius*z_scale*std::sin(a)},p)||
                   !local_to_world({volume.radius*x_scale*std::cos(b),y,volume.radius*z_scale*std::sin(b)},q)||
                   !edge(out,p,q))return false;
            }return true;
        };
        // Original wire layout: 8+8 end rings, four axial seams, and two
        // hemispheres (two planes x 8 segments each) for 52 total segments.
        if(!ring(-volume.half_length,8,1.,1.)||!ring(volume.half_length,8,1.,1.))return false;
        for(unsigned i=0;i<4;++i){vr::Vector p{},q{};
            const double a=2.*pi*i/4.;
            const vr::Vector local{volume.radius*std::cos(a),0.,volume.radius*std::sin(a)};
            if(!local_to_world({local.x,-volume.half_length,local.z},p)||
               !local_to_world({local.x,volume.half_length,local.z},q)||!edge(out,p,q))return false;
        }
        for(double sign:{-1.,1.})for(unsigned plane=0;plane<2;++plane)for(unsigned i=0;i<8;++i){
            const double a=pi*i/8.,b=pi*(i+1)/8.;
            const auto at=[&](double angle){const double y=sign*(volume.half_length+volume.radius*std::sin(angle));
                const double c=volume.radius*std::cos(angle);
                return plane==0?vr::Vector{c,y,0}:vr::Vector{0,y,c};};
            vr::Vector p{},q{};if(!local_to_world(at(a),p)||!local_to_world(at(b),q)||!edge(out,p,q))return false;
        }
        return true;
    }
    return false;
}
}
