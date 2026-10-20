#include "ngx_stereo_layout.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace gears_ngx_stereo;
static Layout fixture(uint32_t width=2560,uint32_t height=1080,uint32_t in_width=853,uint32_t in_height=720){
    Layout l;for(auto& e:l.physical)e={width,height};l.full_output={width,height};l.input={in_width,in_height};l.stride=width/2;l.eye_output={l.stride,height};l.input_base={Point{0,0},Point{l.stride,0}};return l;
}
static Identity identity(){return {1,2,3,4,5};}
static CopyFacts facts(Layout l){
    CopyFacts f;f.identity=identity();f.ticket=99;f.caller=0x1c6093;f.list=80;f.generation=6;f.list_device=3;f.direct=f.copy_source=f.copy_dest=true;
    for(unsigned i=0;i<4;++i)f.resources[i]={10+i,3,l.physical[i],26,0,1,0};
    f.resources[1].format=45;f.resources[2].format=34;
    f.input_base=l.input_base;f.feature_output=l.eye_output;return f;
}
static void layouts(){
    for(auto l:{fixture(),fixture(1920,1080,640,720),fixture(3840,2160,1280,1440),fixture(128,64,64,64),fixture(8192,8192,4096,8192)}){
        assert(valid(l));auto f=facts(l);CopyPlan p;CopySequence sequence(l,identity());
        assert(sequence.next(f,0,p));assert((p.source_box==std::array<uint32_t,6>{0,0,0,l.stride,l.full_output.height,1}));assert(p.destination[0]==0);
        f.evaluated_ordinal=1;assert(sequence.next(f,1,p));assert(p.destination[0]==l.stride&&p.destination[1]==0&&p.destination[2]==0&&sequence.complete());
        assert(!sequence.next(f,1,p)&&!sequence.complete());
    }
    auto l=fixture();l.input_base[1].x=853;assert(!valid(l)); // Would overwrite the remaining 347 input columns.
    for(unsigned n=0;n<13;++n){l=fixture();switch(n){
        case 0:l.full_output.width=2559;break;case 1:l.full_output.width=8194;break;case 2:l.input.width=1281;break;
        case 3:l.input.height=1081;break;case 4:l.eye_output.width=853;break;case 5:l.stride=853;break;
        case 6:l.physical[1].width=1280;break;case 7:l.physical[2].height=719;break;case 8:l.physical[3].width=1279;break;
        case 9:l.physical[0].height=1079;break;case 10:l.input_base[1].x=UINT32_MAX;break;case 11:l.input.height=63;break;case 12:l.full_output.height=1081;l.eye_output.height=1081;break;}
        assert(!valid(l));
    }
    assert(!contains({8192,8192},{UINT32_MAX,0,2,2}));assert(!contains({8192,8192},{0,0,UINT32_MAX,2}));
    assert(overlap({UINT32_MAX-2,0,3,2},{UINT32_MAX-1,0,1,2}));assert(!overlap({0,0,1280,1080},{1280,0,853,720}));
}
static void copies(){
    auto l=fixture();auto good=facts(l);CopyPlan p;
    for(unsigned n=0;n<27;++n){auto f=good;switch(n){
        case 0:f.identity.epoch++;break;case 1:f.identity.world++;break;case 2:f.identity.device++;break;case 3:f.identity.parameters++;break;case 4:f.identity.layout++;break;
        case 5:f.ticket=0;break;case 6:f.caller++;break;case 7:f.list=0;break;case 8:f.generation=0;break;
        case 9:f.direct=false;break;case 10:f.copy_source=false;break;case 11:f.copy_dest=false;break;case 12:f.resources[0].identity=f.resources[3].identity;break;
        case 13:f.resources[2].device++;break;case 14:f.resources[0].format=27;break;case 15:f.resources[3].mip=1;break;
        case 16:f.resources[0].samples=2;break;case 17:f.resources[3].subresource=1;break;case 18:f.resources[2].extent.width--;break;
        case 19:f.feature_output.width=853;break;case 20:f.output_base.x=1280;break;case 21:f.input_base[1].x=853;break;
        case 22:f.evaluated_ordinal=1;break;case 23:f.resources[2].identity=0;break;case 24:f.resources[1].identity=f.resources[2].identity;break;case 25:f.feature_output.height=UINT32_MAX;break;case 26:f.list_device=4;break;}
        assert(!plan_copy(l,identity(),f,0,p));
    }
    for(unsigned n=0;n<7;++n){CopySequence s(l,identity());assert(s.next(good,0,p));auto f=good;f.evaluated_ordinal=1;
        switch(n){case 0:f.ticket++;break;case 1:f.list=0;break;case 2:f.generation=0;break;case 3:f.resources[1].identity++;break;case 4:f.identity.epoch++;break;case 5:f.evaluated_ordinal=0;break;case 6:f.output_base.y=1;break;}
        assert(!s.next(f,1,p)&&s.failed()&&!s.complete());assert(!s.next(good,0,p));
    }
    // Observed two-list route: CPU admission preserves each eye's own facts.
    CopySequence separate_lists(l,identity());auto left=good;
    left.list=0x161b4d9b30;left.generation=1988765;
    assert(separate_lists.next(left,0,p)&&p.list==left.list&&p.generation==left.generation);
    auto right=good;right.evaluated_ordinal=1;right.list=0x1150860d0;right.generation=1988767;
    assert(separate_lists.next(right,1,p)&&p.list==right.list&&p.generation==right.generation&&separate_lists.complete());
    CopySequence out_of_order(l,identity());auto f=good;f.evaluated_ordinal=1;assert(!out_of_order.next(f,1,p));
}
int main(){layouts();copies();std::cout<<"ngx stereo layout policy tests passed\n";}
