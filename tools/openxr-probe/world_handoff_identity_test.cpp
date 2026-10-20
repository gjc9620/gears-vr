#include "../vr-common/gpu_share.h"
#include "world_handoff_display.h"
#include <cassert>
int main(){
    gears_vr::GpuPacket p{};
    p.magic=0x47564731;p.version=gears_vr::gpu_packet_version;p.size=sizeof(p);
    p.pid=123;p.generation=4;p.width=1920;p.height=1080;p.format=28;p.ready=1;
    const auto valid=[&]{return gears_vr::gpu_display_identity_matches(p,123,4,1920,1080,28,false);};
    assert(valid());
    p.world_generation=2;p.world_quiescing=1;assert(valid()); // World handoff is not resource replacement.
    for(unsigned i=0;i<11;++i){const auto saved=p;
        switch(i){case 0:p.ready=0;break;case 1:p.ready=2;break;case 2:p.stopped=1;break;
        case 3:++p.pid;break;case 4:++p.generation;break;case 5:++p.width;break;
        case 6:++p.height;break;case 7:++p.format;break;case 8:++p.allow_simulated;break;
        case 9:++p.version;break;case 10:--p.size;break;}
        assert(!valid());p=saved;
    }
    assert(valid());
    // Mirror GameGpuScreen's observe input: a valid world snapshot alone
    // cannot seed a baseline or pending handoff through an invalid receiver.
    for(unsigned fault=0;fault<2;++fault){
        const auto saved=p;
        WorldHandoffDisplay initial,established;
        established.observe({true,2,false});
        if(fault==0)++p.pid;else ++p.generation;
        const auto observe=[&](WorldHandoffDisplay& policy){
            policy.observe({true&&valid(),2,true});
        };
        observe(initial);observe(established);
        assert(!initial.eligible()&&!established.eligible());
        p=saved;
        initial.observe({valid(),2,false});established.observe({valid(),2,false});
        assert(!initial.eligible()&&!established.eligible());
    }
}
