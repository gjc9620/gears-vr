#include "ngx_owned_publication.h"
#include <cassert>
#include <cstdio>
using namespace gears_ngx_stereo;
using namespace gears_ngx_owned;
RecordedOutput output(Identity identity,uint64_t ticket){
    RecordedOutput o;o.identity=identity;o.ticket=ticket;o.color=40;
    auto& l=o.layout;l.full_output={1792,980};l.input={597,653};l.eye_output={896,980};l.stride=896;
    l.input_base[1]={896,0};for(auto& e:l.physical)e=l.full_output;
    for(unsigned eye=0;eye<2;++eye){auto& c=o.copies[eye];
        c.source_box={0,0,0,896,980,1};c.destination={eye*896,0,0};
        c.source=50;c.target=40;c.list=60+eye;c.generation=4;c.ticket=ticket;c.ordinal=eye;}
    return o;
}
int main(){
    Facts f;f.world=1;f.ticket=100;
    assert(publication(f)==Publication::native);
    const Identity first{1,1,3,4,1};f.owner=true;f.epoch=f.input_epoch=first;
    f.input_world=1;f.input_ticket=100;f.input_renderer=70;f.last_eye=1;
    f.transition=true;f.no_sdk_gap=true;
    auto old=output(first,99);f.output=&old;
    assert(publication(f)==Publication::discard_candidate); // Never reuse a prior image at bootstrap.
    f.copy_pending=true;assert(publication(f)==Publication::rejected);f.copy_pending=false;
    f.input_epoch.epoch=2;assert(publication(f)==Publication::rejected);f.input_epoch=first;
    f.transition=false;f.output=nullptr;
    assert(publication(f)==Publication::discard_candidate); // The first real MV pair also awaits real completion.
    f.output=&old;f.transport_accepted=true;
    assert(publication(f)==Publication::rejected); // Full old receipt is still the wrong ticket.
    auto current=output(first,100);f.output=&current;
    assert(publication(f)==Publication::controlled);
    f.render_candidate=true;assert(publication(f)==Publication::discard_candidate); // Geometry was collected before SDK acceptance.
    f.copy_pending=true;assert(publication(f)==Publication::rejected);f.copy_pending=false;
    --f.input_ticket;assert(publication(f)==Publication::rejected);++f.input_ticket;f.render_candidate=false;
    for(unsigned eye=0;eye<2;++eye){
        auto changed=current;--changed.copies[eye].ticket;f.output=&changed;
        assert(publication(f)==Publication::rejected);
        changed=current;++changed.copies[eye].destination[0];
        assert(publication(f)==Publication::rejected);
    }
    f.output=&current;
    f.ticket=101;f.no_sdk_gap=false;
    assert(publication(f)==Publication::rejected); // Cached feature existence is not absence of SDK use.
    f.no_sdk_gap=true;assert(publication(f)==Publication::native);
    f.render_candidate=true;assert(publication(f)==Publication::rejected);f.render_candidate=false;
    f.input_ticket=102;assert(publication(f)==Publication::rejected); // A later use cannot certify an earlier frame.
    f.ticket=f.input_ticket=103;f.no_sdk_gap=false;
    const Identity replacement{3,1,3,4,3};f.epoch=f.input_epoch=replacement;
    current=output(first,103);assert(publication(f)==Publication::rejected); // Same texture/ticket, retired epoch.
    f.transition=true;assert(publication(f)==Publication::discard_candidate);
    f.transition=false;current=output(replacement,103);assert(publication(f)==Publication::controlled);
    f.failed=true;assert(publication(f)==Publication::rejected);f.failed=false;
    f.input_world=2;assert(publication(f)==Publication::rejected);f.input_world=1;
    f.world=2;assert(publication(f)==Publication::rejected);
    std::puts("Owned SDK publication: native gap, unpublished transition, stale epoch/copy and accepted output checks passed (CPU policy only)");
}
