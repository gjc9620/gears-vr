#include "game_ui_material.h"
#include "gpu_viewport_stats.h"
#include <cstdio>
#include <stdexcept>
using namespace gears_game_material;
#define CHECK(x) do{if(!(x)){std::printf("failed line %d: %s\n",__LINE__,#x);return __LINE__;}}while(0)
int main(){
    CHECK(plan_kind(0)==Plan::ordinary&&plan_kind(1)==Plan::reject&&plan_kind(2)==Plan::hud&&plan_kind(3)==Plan::hud&&plan_kind(4)==Plan::reject);
    UploadCertificate u{};u.context=11;u.native_context=12;u.quad.source=13;
    auto q=u.quad;
    CHECK(upload_matches(u,11,14,14,32,5,q));
    CHECK(!upload_matches(u,10,14,14,32,5,q));
    CHECK(!upload_matches(u,11,14,15,32,5,q));
    CHECK(!upload_matches(u,11,14,14,16,5,q));
    CHECK(!upload_matches(u,11,14,14,32,4,q));
    q.vertices[2][7]=1;CHECK(!upload_matches(u,11,14,14,32,5,q));q=u.quad;
    q.source=99;CHECK(!upload_matches(u,11,14,14,32,5,q));q=u.quad;
    q.index_data=1;CHECK(!upload_matches(u,11,14,14,32,5,q));q=u.quad;
    u.native_context=0;CHECK(!upload_matches(u,11,14,14,32,5,q));u.native_context=12;
    Completion c;CHECK(!c.blocks_present());CHECK(c.begin()&&c.begin()&&c.pending()==2&&c.blocks_present());
    CHECK(!c.end(true,true)&&c.blocks_present());CHECK(c.end(true,false)&&!c.blocks_present());
    CHECK(c.begin()&&!c.end(false,false)&&!c.blocks_present());
    CHECK(c.end(false,true)&&c.blocks_present()&&!c.begin()); // Unpaired completion fails closed.
    gears_viewport::DrawState saved{true,{0,0,3840,2160},{0,0,3840,2160},1,1,true,.2f,.8f};
    const auto plan=gears_viewport::overlay_plan(saved,3840,2160);CHECK(plan.valid);
    auto viewport=saved.viewport,scissor=saved.scissor;float minimum{},maximum{};unsigned calls=0;
    auto vp=[&](gears_viewport::Rect r,float a,float b){viewport=r;minimum=a;maximum=b;};
    auto sc=[&](gears_viewport::Rect r){scissor=r;};
    auto mask=gears_viewport::replay_overlay(saved,plan,vp,sc,[&](unsigned eye){
        if(viewport.x!=float(eye*1920)||viewport.width!=1920||scissor.x!=float(eye*1920))throw std::runtime_error("wrong eye");++calls;});
    CHECK(mask==3&&calls==2&&viewport==saved.viewport&&scissor==saved.scissor&&minimum==.2f&&maximum==.8f);
    bool threw=false;calls=0;volatile bool fail_draw=true;
    try{gears_viewport::replay_overlay(saved,plan,vp,sc,[&](unsigned){++calls;if(fail_draw)throw std::runtime_error("draw failure");});}
    catch(std::runtime_error const&){threw=true;}
    CHECK(threw&&calls==1&&viewport==saved.viewport&&scissor==saved.scissor&&minimum==.2f&&maximum==.8f);
    auto invalid=plan;invalid.valid=false;calls=0;
    CHECK(!gears_viewport::replay_overlay(saved,invalid,vp,sc,[&](unsigned){++calls;})&&calls==0);
    std::puts("game Material certificate rejection/completion/two-eye restoration checks passed");
}
