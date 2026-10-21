#include "ngx_packed_stage.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
using namespace gears_ngx_stereo;
struct Pixels {
    std::array<Extent,4> size{{{1280,720},{1280,720},{640,720},{640,720}}};
    std::array<std::vector<unsigned>,4> data;
    std::array<PackedState,4> state{PackedState::destination,PackedState::source,PackedState::destination,PackedState::destination};
    Pixels(){for(unsigned i=0;i<4;++i)data[i].resize(size[i].width*size[i].height);}
    bool transition(PackedRole role,PackedState before,PackedState after){auto i=unsigned(role);assert(state[i]==before);state[i]=after;return true;}
    bool copy(PackedRole from,Rect box,PackedRole to,Point at){auto a=unsigned(from),b=unsigned(to);
        assert(state[a]==PackedState::source&&state[b]==PackedState::destination&&contains(size[a],box)&&contains(size[b],{at.x,at.y,box.width,box.height}));
        for(unsigned y=0;y<box.height;++y)for(unsigned x=0;x<box.width;++x)data[b][(at.y+y)*size[b].width+at.x+x]=data[a][(box.y+y)*size[a].width+box.x+x];return true;}
};
int main(){
    Identity id{1,2,3,4,5};PackedStage stage;Pixels p;
    std::array<Resource,4> resources;for(unsigned i=0;i<4;++i)resources[i]={10+i,id.device,p.size[i],26,0,1,0};
    Layout l;l.physical={p.size[0],p.size[0],p.size[0],p.size[1]};l.full_output={1280,720};l.eye_output={640,720};l.input={426,480};l.stride=640;l.packed_input=true;l.input_base[1]={426,0};assert(valid(l));
    for(unsigned frame=1;frame<=2;++frame){
        for(unsigned y=0;y<480;++y)for(unsigned x=0;x<426;++x)p.data[0][y*1280+426+x]=1000000*frame+y*426+x;
        auto original=p.data[0];std::fill(p.data[1].begin(),p.data[1].end(),111+frame);
        PackedRequest r{id,frame,91,frame,resources,{426,0,426,480},{640,720}};
        CopyFacts f;f.identity=id;f.ticket=frame;f.caller=0x1c6093;f.list=r.list;f.generation=r.generation;f.list_device=id.device;f.direct=f.copy_source=f.copy_dest=true;
        f.resources={resources[0],Resource{30,id.device,p.size[0],41,0,1,0},Resource{31,id.device,p.size[0],34,0,1,0},resources[1]};f.input_base=l.input_base;f.feature_output=l.eye_output;
        CopyPlan plan;assert(!plan_copy(l,id,f,0,plan)); // Overlap remains forbidden without actual backup receipts.
        assert(stage.backup(p,r));f.input_backup=stage.input_backup();f.output_backup=stage.output_backup();assert(plan_copy(l,id,f,0,plan));
        auto invalid=f;invalid.input_backup.ticket++;assert(!plan_copy(l,id,invalid,0,plan));invalid=f;invalid.output_backup.storage.identity=f.resources[0].identity;assert(!plan_copy(l,id,invalid,0,plan));
        p.copy(PackedRole::output,{0,0,640,720},PackedRole::color,{0,0});
        assert(stage.restore_input(p,frame)&&stage.right_ready(frame));
        for(unsigned y=0;y<480;++y)for(unsigned x=0;x<426;++x)assert(p.data[0][y*1280+426+x]==original[y*1280+426+x]);
        std::fill(p.data[1].begin(),p.data[1].end(),222+frame);p.copy(PackedRole::output,{0,0,640,720},PackedRole::color,{640,0});assert(stage.restore_output(p,frame));
        for(unsigned y=0;y<720;++y)for(unsigned x=0;x<1280;++x)assert(p.data[0][y*1280+x]==(x<640?111+frame:222+frame));
    }
    assert(stage.backups==2&&stage.input_restores==2&&stage.output_restores==2&&!stage.failed());
    std::puts("Packed staging preserves right SDK input and both final output regions across list reuse (CPU pixels only)");
}
