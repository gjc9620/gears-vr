#include "ngx_stereo_output_join.h"
#include <cassert>
#include <iostream>
using namespace gears_ngx_stereo;
static RecordedOutput output(){
    RecordedOutput r;r.identity={1,2,3,4,5};r.ticket=7;r.color=100;
    for(auto& e:r.layout.physical)e={2560,1080};r.layout.full_output={2560,1080};
    r.layout.input={853,720};r.layout.eye_output={1280,1080};r.layout.stride=1280;
    r.layout.input_base={Point{0,0},Point{1280,0}};
    for(unsigned i=0;i<2;++i)r.copies[i]={{0,0,0,1280,1080,1},{i*1280,0,0},103,100,50+i,6,7,i};
    return r;
}
static OutputDrawFacts facts(){return {{1,2,3,4,5},7,1,{100,3,{2560,1080},26,0,1,0},
    {52,6,3},{1280,0,853,720},{2560,1080}};}
static ExecuteReceipt execution(uint64_t list,uint64_t order,unsigned member=0){
    return {{3,list,6,70,80,order,true},order,member};
}
int main(){
    auto r=output();auto f=facts();std::array<RecordedPosition,2> p{{{50,6,1},{51,6,2}}};
    RecordedOutputDraw joined{};assert(record_output_draw(r,p,f,joined));
    assert(joined.source.x==1280&&joined.source.width==1280&&joined.destination.width==1280);
    std::array<ExecuteReceipt,2> e{execution(50,1),execution(51,2)};auto draw=execution(52,3);
    assert(submitted_output_draw(joined,e,draw));
    assert(!completed_output_draw(joined,e,draw,{3,70,80,2}));
    assert(completed_output_draw(joined,e,draw,{3,70,80,3}));
    assert(!completed_output_draw(joined,e,draw,{3,70,80,UINT64_MAX}));
    for(unsigned bad=0;bad<6;++bad){auto changed=f;
        switch(bad){case 0:++changed.identity.epoch;break;case 1:++changed.ticket;break;
            case 2:++changed.bound_color.identity;break;case 3:changed.bound_color.extent.width=2133;break;
            case 4:changed.rendered.width=1280;break;case 5:changed.bound_color.samples=2;break;}
        assert(!record_output_draw(r,p,changed,joined));
    }
    assert(record_output_draw(r,p,f,joined));
    for(unsigned bad=0;bad<5;++bad){auto changed=draw;
        switch(bad){case 0:++changed.submission.generation;break;case 1:++changed.submission.queue;break;
            case 2:++changed.submission.fence;break;case 3:changed.order=1;break;case 4:changed.submission.value=0;break;}
        assert(!submitted_output_draw(joined,e,changed));
    }
    // Separate members in one batch can overlap: member order alone rejects.
    e={execution(50,1,0),execution(51,1,1)};draw=execution(52,1,2);
    assert(!submitted_output_draw(joined,e,draw));
    draw.member=0;assert(!submitted_output_draw(joined,e,draw));
    // One list: operation positions resolve ordering inside the same member.
    r.copies[1].list=50;p[1].list=50;f.draw.list=50;
    assert(record_output_draw(r,p,f,joined));e={execution(50,1),execution(50,1)};draw=execution(50,1);
    assert(submitted_output_draw(joined,e,draw));
    f.draw.operation=2;assert(!record_output_draw(r,p,f,joined));
    f.draw.operation=3;p[1].generation=7;assert(!record_output_draw(r,p,f,joined));
    std::cout<<"ngx stereo output join tests passed\n";
}
