#pragma once
#include "final_transfer_bracket.h"
namespace gears_final_transfer_normalize {
using Input=gears_final_transfer::Input;
struct Plan {bool valid{},modified{};std::array<float,8> original{},effective{};};
inline bool allowed_mode(bool enabled,bool pending_attested,bool recording,bool world,bool self_testing,bool owned,bool faulted){
    return enabled&&pending_attested&&recording&&world&&!self_testing&&!owned&&!faulted;
}
inline bool shape(Input const& in){
    auto const& s=in.source;auto const& t=in.binding.target;auto const& r=in.rectangles;
    if(in.quad_caller!=0x9a7377||in.slots!=1||!in.known_buffer||in.descriptor_lost||in.binding.targets!=1||
       !s.resource||!t.resource||s.resource==t.resource||s.dimension!=3||s.depth!=1||s.mips!=1||s.samples!=1||
       s.format!=24||t.format!=24||t.mip||!s.width||s.width%2||s.width>8192||!s.height||s.height>8192||
       !t.width||t.width%2||t.width>8192||!t.height||t.height>8192||
       in.dimensions!=std::array<unsigned,4>{unsigned(t.width),t.height,s.width,s.height})return false;
    for(float v:in.args)if(!std::isfinite(v))return false;
    if(in.args[0]!=0||in.args[1]!=0||in.args[2]!=float(t.width)||in.args[3]!=float(t.height)||in.args[6]<=0||in.args[7]<=0||
       !r.tracked||r.viewport_count!=1||r.scissor_count!=1||!r.depth_known||r.min_depth!=0||r.max_depth!=1||
       !(r.viewport==gears_viewport::Rect{0,0,float(t.width),float(t.height)}))return false;
    auto const& c=r.scissor;
    return std::isfinite(c.x)&&std::isfinite(c.y)&&std::isfinite(c.width)&&std::isfinite(c.height)&&
        std::isfinite(c.x+c.width)&&std::isfinite(c.y+c.height)&&c.x<=0&&c.y<=0&&c.width>=0&&c.height>=0&&
        c.x+c.width>=float(t.width)&&c.y+c.height>=float(t.height);
}
inline Plan plan(Input const& in){
    Plan out{};out.original=out.effective=in.args;if(!shape(in))return out;
    out.valid=true;out.effective[4]=out.effective[5]=0;out.effective[6]=float(in.source.width);out.effective[7]=float(in.source.height);
    out.modified=out.effective!=out.original;return out;
}
// Separate from the unchanged legacy geometry predicate. Its caller must also
// own and validate the active normalized scope's fresh identity/pending proof.
inline bool eligible_full_source(Input const& in){
    return shape(in)&&in.draw_caller==0x1af92b&&in.kind==1&&in.elements==6&&in.instances==1&&
        in.args[4]==0&&in.args[5]==0&&in.args[6]==float(in.source.width)&&in.args[7]==float(in.source.height);
}
inline unsigned draw_proof(gears_final_transfer_bracket::Snapshot entry,gears_final_transfer_bracket::Snapshot const& draw,
    Plan const& p,gears_final_transfer_bracket::Pending const& pending_entry,gears_final_transfer_bracket::Pending const& pending_draw,
    gears_final_transfer_bracket::ConsumedBinds const& binds){
    if(!p.valid||!eligible_full_source(draw.input)||draw.input.args!=p.effective)return 1;
    entry.input.args=p.effective;
    const auto difference=gears_final_transfer_bracket::compare(entry,draw);
    if(difference&~unsigned(gears_final_transfer_bracket::target|gears_final_transfer_bracket::rectangles))return 2;
    if(gears_final_transfer_bracket::compare_pending(pending_entry,pending_draw,draw,binds))return 4;
    return 0;
}
inline bool completion(gears_final_transfer_bracket::DrawCalls const& calls,uint64_t accepted,bool exception,bool nested,unsigned post_failure){
    return calls.exactly_one_routed_draw()&&accepted==1&&!exception&&!nested&&!post_failure;
}
}
