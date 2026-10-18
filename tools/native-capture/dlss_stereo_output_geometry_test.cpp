#include "dlss_stereo_output_geometry.h"
#include <cassert>
#include <iostream>
static gears_ngx_stereo::RecordedOutput output(){
    gears_ngx_stereo::RecordedOutput r;r.identity={1,2,3,4,5};r.ticket=7;r.color=100;
    for(auto& extent:r.layout.physical)extent={2560,1080};r.layout.full_output={2560,1080};
    r.layout.input={853,720};r.layout.eye_output={1280,1080};r.layout.stride=1280;
    r.layout.input_base={gears_ngx_stereo::Point{0,0},gears_ngx_stereo::Point{1280,0}};
    for(unsigned i=0;i<2;++i)r.copies[i]={{0,0,0,1280,1080,1},{i*1280,0,0},103,100,50+i,6,7,i};return r;
}
static gears_dlss_output::View view(unsigned eye){gears_dlss_output::View v;v.mask=31;v.native.read_mask=7;
    v.eye_valid=v.native.eye_valid=v.same_current=true;v.eye=v.native.eye=eye;v.marker=1;
    v.source={float(eye*1280),0,853,720};v.target={int32_t(eye*1280),0,853,720};return v;}
int main(){using namespace gears_dlss_output;auto out=output();Dimensions dimensions{3200,1200,2560,1080};
    for(unsigned eye=0;eye<2;++eye){auto v=view(eye);Args args{};
        assert(!geometry(v,dimensions,args));assert(stereo_geometry(v,dimensions,out,out.identity,7,100,args));
        assert((args==Args{float(eye*1600),0,1600,1200,float(eye*1280),0,1280,1080}));
        for(unsigned bad=0;bad<9;++bad){auto changed=out;auto vv=v;auto sizes=dimensions;auto identity=out.identity;uint64_t ticket=7,color=100;
            switch(bad){case 0:vv.source[2]=1280;break;case 1:vv.target[2]=1280;break;case 2:++identity.epoch;break;
                case 3:++ticket;break;case 4:++color;break;case 5:changed.copies[0].destination[0]=1;break;
                case 6:sizes[2]=2133;break;case 7:vv.same_current=false;break;case 8:sizes[0]=8194;break;}
            assert(!stereo_geometry(vv,sizes,changed,identity,ticket,color,args));}
    }
    std::cout<<"DLSS independent SR output geometry tests passed\n";
}
