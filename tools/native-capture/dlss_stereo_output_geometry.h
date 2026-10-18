#pragma once
#include "dlss_output_control.h"
#include "ngx_stereo_output_join.h"

namespace gears_dlss_output {
// Explicit alternative for the independent SR path. The legacy geometry()
// keeps its original behavior. Both source+48 and render-target+58 must still
// describe the attested reduced render rectangle; neither is rewritten to the
// post-SR display dimensions. This CPU geometry receipt does not prove Execute,
// resource barriers, shader consumption or final publication.
inline bool stereo_geometry(View const& view,Dimensions const& dimensions,
    gears_ngx_stereo::RecordedOutput const& output,gears_ngx_stereo::Identity identity,
    uint64_t ticket,uint64_t bound_color,Args& forwarded){
    using namespace gears_ngx_stereo;
    if(!valid_view(view)||!valid(output.layout)||dimensions[2]!=output.layout.physical[0].width||
       dimensions[3]!=output.layout.physical[0].height)return false;
    auto const base=output.layout.input_base[view.eye];auto const input=output.layout.input;
    if(view.source!=gears_dlss_route::Float4{float(base.x),float(base.y),float(input.width),float(input.height)}||
       view.target!=std::array<int32_t,4>{int32_t(base.x),int32_t(base.y),int32_t(input.width),int32_t(input.height)})return false;
    Rect source{},destination{};
    if(!output_geometry(output,identity,ticket,bound_color,view.eye,{base.x,base.y,input.width,input.height},
                        {dimensions[0],dimensions[1]},source,destination))return false;
    // Native quad ABI: destination first, then texture source. Different
    // display/physical widths are intentional test coverage for this order.
    forwarded={float(destination.x),float(destination.y),float(destination.width),float(destination.height),
               float(source.x),float(source.y),float(source.width),float(source.height)};return true;
}
}
