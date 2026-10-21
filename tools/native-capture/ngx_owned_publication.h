#pragma once
#include "ngx_stereo_adapter.h"

// CPU admission only. Live hooks supply all identity, SDK-use, copy and gap
// observations. The native Owner, UI, Execute and resource-state proofs remain
// independent requirements; this policy manufactures no GPU completion.
namespace gears_ngx_owned {
enum class Publication { native, discard_candidate, controlled, rejected };
struct Facts {
    uint64_t world{},ticket{},input_world{},input_ticket{},input_renderer{};
    gears_ngx_stereo::Identity epoch{},input_epoch{};
    bool owner{},failed{},no_sdk_gap{},transition{},copy_pending{},transport_accepted{},render_candidate{};
    unsigned last_eye{};
    gears_ngx_stereo::RecordedOutput const* output{};
};
inline Publication publication(Facts const& f){
    using namespace gears_ngx_stereo;
    if(!f.world||!f.ticket)return Publication::rejected;
    if(!f.owner)return Publication::native;
    if(f.failed||!valid(f.epoch)||f.epoch.world!=f.world)return Publication::rejected;
    if(f.render_candidate&&f.input_ticket!=f.ticket)return Publication::rejected;
    if(f.input_ticket!=f.ticket)
        return f.input_ticket<f.ticket&&f.no_sdk_gap?Publication::native:Publication::rejected;
    if(f.input_world!=f.world||!f.input_renderer||!same(f.input_epoch,f.epoch)||f.copy_pending||f.last_eye!=1)
        return Publication::rejected;
    // Even a coincidentally retained old receipt cannot publish this pair.
    if(f.transition||!f.transport_accepted||f.render_candidate)return Publication::discard_candidate;
    if(!f.output||!same(f.output->identity,f.epoch))return Publication::rejected;
    auto const& o=*f.output;auto const& l=o.layout;
    for(unsigned eye=0;eye<2;++eye){Rect source{},destination{};auto const p=l.input_base[eye];
        if(!output_geometry(o,f.epoch,f.ticket,o.color,eye,{p.x,p.y,l.input.width,l.input.height},l.full_output,source,destination))
            return Publication::rejected;
    }
    return Publication::controlled;
}
}
