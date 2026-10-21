#include "ngx_final_xr_ticket.h"
#include <cassert>
#include <cstdio>
using namespace gears_ngx_final_xr;
static Packet fixture(unsigned eye=1,uint64_t ticket=42,uint64_t list=80){
    Packet p;p.draw={2,ticket,list,7,10,11,1280,720,eye};p.image.id=ticket;
    auto& s=p.image.sample;s.session=9;s.sequence=18;s.received_ns=1000000000;s.predicted_time=1001234567;
    s.head_flags=s.view_flags=15;s.focused=s.should_render=1;s.head.orientation={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.orientation={};s.eyes[i].pose.position.x=i?.032:-.032;
        s.eyes[i].fov={-.8,.7,.75,-.65};p.image.rect[i]={int(i*640),0,640,720};}
    return p;
}
int main(){
    auto left=fixture(0),right=fixture();assert(pair(left,right,false));
    for(unsigned n=0;n<14;++n){auto p=right;switch(n){
        case 0:++p.draw.world;break;case 1:++p.draw.ticket;++p.image.id;break;
        case 2:++p.draw.list;break;case 3:++p.draw.generation;break;case 4:++p.draw.source;break;
        case 5:++p.draw.target;break;case 6:p.draw.width=1282;break;case 7:++p.draw.height;break;
        case 8:p.draw.eye=0;break;case 9:++p.image.sample.sequence;break;case 10:++p.image.sample.space_epoch;break;
        case 11:++p.image.sample.predicted_time;break;case 12:p.image.rect[1].x=639;break;case 13:p.image.sample.source=gears_vr::StereoSource::MetaOperatorSimulated;break;}
        assert(!pair(left,p,false));
    }
    const Member member{80,7};Pending pending;auto index=pending.push(right,false);assert(index<Pending::capacity);
    assert(pending.push(right,false)==Pending::capacity);
    assert(pending.take(nullptr,0,true).match==Match::none&&pending.occupied(index));
    auto selected=pending.take(&member,1,true);assert(selected.match==Match::one&&selected.index==index&&same_image(selected.packet.image,right.image));
    assert(!pending.occupied(index)&&pending.take(&member,1,true).match==Match::none);
    // Two complete frames in one actual batch cannot share a single final
    // staging texture while claiming the first frame's pose.
    assert(pending.push(right,false)<Pending::capacity);
    assert(pending.push(fixture(1,43,81),false)<Pending::capacity);
    Member both[]{{80,7},{81,7}};assert(pending.take(both,2,true).match==Match::ambiguous);
    assert(pending.take(both,2,true).match==Match::none);
    for(unsigned bad=0;bad<4;++bad){assert(pending.push(right,false)<Pending::capacity);
        Member changed=member;auto count=1u;bool readable=true;
        if(bad==0)changed.generation++;if(bad==1)changed.list=0;if(bad==2)readable=false;
        Member duplicate[]{member,member};if(bad==3)count=2;
        assert(pending.take(bad==3?duplicate:&changed,count,readable).match==Match::invalid);
        assert(pending.take(&member,1,true).match==Match::none);
    }
    index=pending.push(right,false);assert(index<Pending::capacity);pending.reset(80);
    assert(!pending.occupied(index)&&pending.take(&member,1,true).match==Match::none);
    for(unsigned i=0;i<Pending::capacity;++i)assert(pending.push(fixture(1,100+i,200+i),false)<Pending::capacity);
    assert(pending.push(fixture(1,999,999),false)==Pending::capacity);
    puts("NgxFinalXrTicketTest passed: frozen pose, stereo pair, actual batch membership, ambiguity, reset and capacity");
}
