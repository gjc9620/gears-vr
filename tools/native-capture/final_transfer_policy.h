#pragma once
#include <array>
#include <cmath>
#include "gpu_viewport_stats.h"

// Narrow presentation geometry observed in Native11. This policy describes CPU
// arguments; it does not claim to decode a pixel shader or prove GPU completion.
namespace gears_final_transfer {
// A per-invocation candidate produced only by a guarded, forwarded DLSS quad
// and its actual draw. It is not the completed Output/publication proof.
struct EyeDrawEvidence {
    bool candidate{};uint64_t world{},ticket{};unsigned eye{};
    uintptr_t list{},source{},target{};uint64_t generation{};
    std::array<float,8> args{};std::array<unsigned,4> dimensions{};
};
inline bool eye_caller(uintptr_t caller,bool source_attested=false,bool controlled=false){
    return caller==0xd83b1c||caller==0xd808d8||(caller==0x2b11a0&&source_attested)||(caller==0xd83cbf&&controlled);
}
// quad_valid includes the existing per-eye rectangle and (for 2b11a0)
// identity UV/texture-size checks. The same attested eye pass can render into
// an intermediate or directly into the current backbuffer; its destination
// does not turn it into a full-frame transfer. Draw/Execute/Present guards
// still apply after this predicate.
inline bool eligible_eye_target(uintptr_t caller,bool quad_valid,uint64_t ticket,
    unsigned eye_width,unsigned eye_height,gears_viewport::TargetDraw const& binding,bool attested,
    EyeDrawEvidence const& control={},uint64_t current_world=0){
    if(!quad_valid||!ticket||binding.targets!=1||binding.target.mip||
       binding.target.width!=eye_width||binding.target.height!=eye_height)return false;
    if(caller==0x2b11a0)return attested&&binding.target.format==24;
    if(caller==0xd83cbf){
        if(!attested||!control.candidate||!current_world||control.world!=current_world||control.ticket!=ticket||control.eye>1||
           !control.list||!control.generation||!control.source||control.source==control.target||control.target!=binding.target.resource||
           binding.target.format!=24||!eye_width||eye_width%2||!eye_height||!control.dimensions[2]||control.dimensions[2]%2||!control.dimensions[3]||
           eye_width>8192||eye_height>8192||control.dimensions[2]>8192||control.dimensions[3]>8192||
           control.dimensions[0]!=eye_width||control.dimensions[1]!=eye_height)return false;
        const float x=float(control.eye*(eye_width/2)),u=float(control.eye*(control.dimensions[2]/2));
        return control.args==std::array<float,8>{x,0,float(eye_width/2),float(eye_height),u,0,float(control.dimensions[2]/2),float(control.dimensions[3])};
    }
    return caller==0xd83b1c||caller==0xd808d8;
}
struct Source {
    uintptr_t resource{};unsigned dimension{},width{},height{},depth{},mips{},format{},samples{};
};
struct Input {
    uintptr_t quad_caller{},draw_caller{};
    std::array<float,8> args{};std::array<unsigned,4> dimensions{};
    gears_viewport::TargetDraw binding{};gears_viewport::DrawState rectangles{};
    Source source{};unsigned kind{},elements{},instances{},slots{};
    bool known_buffer{},descriptor_lost{};
};
inline bool eligible(Input const& in){
    auto const& s=in.source;auto const& t=in.binding.target;auto const& r=in.rectangles;
    if(in.quad_caller!=0x9a7377||in.draw_caller!=0x1af92b||in.kind!=1||in.elements!=6||in.instances!=1||
       in.slots!=1||!in.known_buffer||in.descriptor_lost||in.binding.targets!=1||t.mip||
       !s.resource||!t.resource||s.resource==t.resource||s.dimension!=3||s.depth!=1||s.mips!=1||s.samples!=1||
       s.format!=24||t.format!=s.format||!s.width||s.width%2||!s.height||s.width>8192||s.height>8192||
       t.width!=s.width||t.height<s.height||t.height>8192||t.height-s.height>1||
       in.dimensions!=std::array<unsigned,4>{s.width,t.height,s.width,s.height})return false;
    for(float v:in.args)if(!std::isfinite(v))return false;
    const float delta=float(t.height-s.height);
    const std::array<float,8> expected{0,0,float(t.width),float(t.height),0,-delta/2,float(s.width),float(t.height)};
    if(in.args!=expected||!r.tracked||r.viewport_count!=1||r.scissor_count!=1||!r.depth_known||
       r.min_depth!=0||r.max_depth!=1||!(r.viewport==gears_viewport::Rect{0,0,float(t.width),float(t.height)}))return false;
    const auto& c=r.scissor;
    if(!std::isfinite(c.x)||!std::isfinite(c.y)||!std::isfinite(c.width)||!std::isfinite(c.height)||
       c.x>0||c.y>0||c.width<0||c.height<0||c.x+c.width<float(t.width)||c.y+c.height<float(t.height))return false;
    return true;
}
}
