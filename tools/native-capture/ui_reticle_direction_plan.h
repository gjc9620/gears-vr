#pragma once
#include "gpu_viewport_stats.h"
#include "../vr-common/stereo_ticket_store.h"
#include "../vr-common/ui_plane_projection.h"
#include "ui_index_membership.h"

namespace gears_ui_reticle {
struct Extent {
    bool valid{};
    double left{},top{},right{},bottom{};
    std::uint64_t root_invocation{};
};
// Uses only immutable owning Claim bytes and the bounded copied index snapshot.
// No native payload read or Claim pointer survives. Admission is per referenced
// vertex, so additional crosshair helpers elsewhere in the batch cannot lend
// a misleading kind/root/source label to this segment.
inline bool capture_extent(gears_ui_receipts::Claim const& claim,
    gears_ui_indices::Snapshot const& snapshot,unsigned ordinal,Extent& out){
    if(!gears_ui_indices::detail::bounded_snapshot(snapshot)||ordinal>=snapshot.segment_count||
       snapshot.generation!=claim.generation||
       !gears_ui_indices::detail::claim_valid(claim,snapshot.payload,snapshot.vertex_count))return false;
    auto const& segment=snapshot.segments[ordinal];
    if(!segment.index_count||segment.kind_mask!=1||segment.unknown||segment.known[0]!=segment.index_count)return false;
    Extent result{};unsigned entries=0;
    for(unsigned i=0;i<segment.index_count;++i){
        const auto global=snapshot.global_vertices[segment.index_offset+i];bool found=false;
        for(std::size_t n=0;n<claim.count;++n){auto const& entry=claim.entries[n];auto const& range=entry.receipt;
            if(global<range.begin||global>=range.end)continue;
            if(found||entry.kind!=1||entry.source!=gears_ui_receipts::Source::Canvas||
               (result.root_invocation&&result.root_invocation!=entry.root_invocation))return false;
            const auto copied=entry.vertex_offset+(global-range.begin);
            if(copied>=claim.vertex_count||copied>=gears_ui_receipts::vertex_capacity)return false;
            float position[4]{};std::memcpy(position,claim.vertices->data[copied].data(),sizeof(position));
            for(float f:position)if(!std::isfinite(f))return false;
            if(position[2]!=0||position[3]!=1)return false;
            if(!result.valid){result.left=result.right=position[0];result.top=result.bottom=position[1];result.valid=true;}
            else {result.left=(std::min)(result.left,double(position[0]));result.right=(std::max)(result.right,double(position[0]));
                result.top=(std::min)(result.top,double(position[1]));result.bottom=(std::max)(result.bottom,double(position[1]));}
            result.root_invocation=entry.root_invocation;entries|=1u<<unsigned(n);found=true;
        }
        if(!found)return false;
    }
    if(!result.valid||entries!=segment.entry_mask)return false;out=result;return true;
}
// Expanding crosshairs can be split into four native draw segments. Their
// individual quadrants are off-center, but the same semantic invocation has
// one screen-center aim. Freeze the union only from already certified pure
// reticle segments; never borrow geometry from a different root or HUD draw.
inline void group_extents(std::array<Extent,gears_ui_indices::segment_capacity>& extents,unsigned count){
    if(count>extents.size())return;
    const auto original=extents;
    for(unsigned i=0;i<count;++i){
        auto const& own=original[i];if(!own.valid||!own.root_invocation)continue;
        auto grouped=own;
        for(unsigned j=0;j<count;++j){auto const& other=original[j];
            if(!other.valid||other.root_invocation!=own.root_invocation)continue;
            grouped.left=(std::min)(grouped.left,other.left);grouped.top=(std::min)(grouped.top,other.top);
            grouped.right=(std::max)(grouped.right,other.right);grouped.bottom=(std::max)(grouped.bottom,other.bottom);
        }
        extents[i]=grouped;
    }
}
inline bool centered(Extent const& extent,unsigned width,unsigned height){
    if(!extent.valid||!extent.root_invocation||!width||!height||width>8192||height>8192)return false;
    for(double value:{extent.left,extent.top,extent.right,extent.bottom})if(!std::isfinite(value))return false;
    const double w=extent.right-extent.left,h=extent.bottom-extent.top;
    return w>0&&h>0&&w<=double(width)/3&&h<=double(height)/3&&
        std::abs((extent.left+extent.right)/2-double(width)/2)<=1&&
        std::abs((extent.top+extent.bottom)/2-double(height)/2)<=1;
}
// Caller must certify a pure reticle draw currently centered in the source
// viewport and the SAME actual ticket/target. A caller-authorized owning world
// point uses finite projection; otherwise use the original game screen-center
// infinite-distance direction. No weapon aim/hit or camera equivalence is proved.
// Infinite projection excludes translations. Shape/size and original depth remain
// unchanged unless source_aspect requests source-pixel shape preservation on an
// output with another aspect. Scissors follow the same affine map.
inline bool plan(gears_viewport::DrawState const& saved,unsigned full_width,unsigned full_height,
                 gears_vr::StereoTicketGeometry const& geometry,gears_viewport::OverlayPlan& out,double source_aspect=0){
    auto result=gears_viewport::overlay_plan(saved,full_width,full_height);
    if(!result.valid||!std::isfinite(source_aspect)||source_aspect<0||!gears_vr::stereo_rigid(geometry.source_view)||
       !gears_vr::stereo_projection_supported(geometry.source_projection))return false;
    double direction[4]{};
    if(geometry.finite_aim.valid){
        if(!geometry.finite_aim.world||!geometry.finite_aim.tick)return false;
        for(unsigned i=0;i<3;++i){direction[i]=geometry.finite_aim.point[i];
            if(!std::isfinite(direction[i]))return false;}
        direction[3]=1; // Owning WORLD point: frozen eye translations matter.
    }else{
        double inverse[3][3]{};
        if(!gears_vr::ui_plane_detail::inverse_rotation(geometry.source_view,inverse))return false;
        auto const& p=geometry.source_projection;
        const double center_camera[]{-double(p[8])/p[0],-double(p[9])/p[5],1};
        // w=0 deliberately excludes every translation in the legacy path.
        for(unsigned col=0;col<3;++col){for(unsigned row=0;row<3;++row)
            direction[col]+=center_camera[row]*inverse[row][col];
            if(!std::isfinite(direction[col]))return false;}
    }
    const double eye_width=double(full_width)/2;
    for(unsigned eye=0;eye<2;++eye){
        auto const& view=geometry.eyes.view[eye];auto const& projection=geometry.eyes.projection[eye];
        if(!gears_vr::stereo_rigid(view)||!gears_vr::stereo_projection_supported(projection))return false;
        double camera[4]{},clip[4]{};
        for(unsigned col=0;col<4;++col){for(unsigned row=0;row<4;++row)
            camera[col]+=direction[row]*view[4*row+col];
            if(!std::isfinite(camera[col]))return false;}
        if(camera[2]<=0||camera[3]!=direction[3])return false;
        for(unsigned col=0;col<4;++col){for(unsigned row=0;row<4;++row)
            clip[col]+=camera[row]*projection[4*row+col];
            if(!std::isfinite(clip[col]))return false;}
        if(clip[3]<=0)return false;
        const double nx=clip[0]/clip[3],ny=clip[1]/clip[3];
        // Leaving either eye's frustum is ordinary clipping, not loss of the
        // world target. Rejecting here made the caller retry an infinite ray,
        // dropping seated/head translation and near-target binocular parallax.
        if(!std::isfinite(nx)||!std::isfinite(ny))return false;
        auto& viewport=result.viewports[eye];auto& scissor=result.scissors[eye];
        const double origin=eye*eye_width;
        const double dx=eye_width*nx/2,dy=-double(full_height)*ny/2;
        if(source_aspect){
            const double height=eye_width/source_aspect;
            if(!std::isfinite(height)||height<=0||height>32767)return false;
            viewport.y=float((double(full_height)-height)/2);viewport.height=float(height);
            if(!std::isfinite(viewport.height)||viewport.height<=0)return false;
        }
        viewport.x=float(double(viewport.x)+dx);viewport.y=float(double(viewport.y)+dy);
        for(double value:{double(viewport.x),double(viewport.y),double(viewport.x)+viewport.width,double(viewport.y)+viewport.height})
            if(!std::isfinite(value)||value<-32768||value>32767)return false;
        // Use actual float displacement sent to D3D when clipping/rounding.
        const double tx=double(viewport.x)-origin,ty=viewport.y;
        const double l=(std::max)(origin,double(scissor.x)+tx);
        const double r=(std::min)(origin+eye_width,double(scissor.x)+scissor.width+tx);
        const double scale=double(viewport.height)/full_height;
        const double t=(std::max)(0.,double(scissor.y)*scale+ty);
        const double b=(std::min)(double(full_height),(double(scissor.y)+scissor.height)*scale+ty);
        // Clipping is not a projection failure: retrying an infinite aim here
        // would give members of one reticle group different viewport maps.
        // Keep this eye's finite map and draw with an in-eye empty D3D scissor.
        if(!(r>l&&b>t)){scissor={float(origin),0,0,0};continue;}
        scissor={float(std::floor(l)),float(std::floor(t)),float(std::ceil(r)-std::floor(l)),float(std::ceil(b)-std::floor(t))};
    }
    out=result;return true;
}
// A finite target must never turn into an unrelated infinite direction when
// it crosses/gets behind an eye or cannot fit a legal D3D viewport. Keep the
// owning target and suppress this draw until it can be projected again. The
// caller still replays/restores both eyes' original draw state normally.
inline bool display_plan(gears_viewport::DrawState const& saved,unsigned width,unsigned height,
    gears_vr::StereoTicketGeometry const& geometry,gears_viewport::OverlayPlan& out,
    double source_aspect=0,bool* suppressed=nullptr){
    if(suppressed)*suppressed=false;
    if(plan(saved,width,height,geometry,out,source_aspect))return true;
    if(!geometry.finite_aim.valid)return false;
    auto hidden=gears_viewport::overlay_plan(saved,width,height);
    if(!hidden.valid)return false;
    for(unsigned eye=0;eye<2;++eye)hidden.scissors[eye]={float(eye*width/2),0,0,0};
    out=hidden;if(suppressed)*suppressed=true;return true;
}
}
