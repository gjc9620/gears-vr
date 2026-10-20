#include "game_ui_material.h"
#include "game_ui_material_target.h"
#include "gpu_viewport_stats.h"
#include <memory>
#include <cstdio>
#include <stdexcept>
using namespace gears_game_material;
#define CHECK(x) do{if(!(x)){std::printf("failed line %d: %s\n",__LINE__,#x);return __LINE__;}}while(0)
int main(){
    CHECK(plan_kind(0)==Plan::ordinary&&plan_kind(1)==Plan::reject&&plan_kind(2)==Plan::hud&&plan_kind(3)==Plan::hud&&plan_kind(4)==Plan::reject);
    for(bool hud_plane_enabled:{false,true}){
        CHECK(plan_kind(0,hud_plane_enabled)==Plan::ordinary);
        CHECK(plan_kind(2,hud_plane_enabled)==(hud_plane_enabled?Plan::hud:Plan::ordinary));
        CHECK(plan_kind(3,hud_plane_enabled)==(hud_plane_enabled?Plan::hud:Plan::ordinary));
        CHECK(plan_kind(1,hud_plane_enabled)==Plan::reject);
        CHECK(plan_kind(4,hud_plane_enabled)==Plan::reject);
        CHECK(plan_kind(std::numeric_limits<unsigned>::max(),hud_plane_enabled)==Plan::reject);
    }
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
    // Runtime regression: certified Material writes land on the intermediate
    // stereo source, before the attested transfer to the swapchain backbuffer.
    for(unsigned mode=0;mode<10;++mode){
        auto tracker=std::make_unique<gears_frame_link::Tracker>();
        auto ref=tracker->reset_success(1);
        const gears_frame_link::Target source{100,3840,2160},destination{101,3840,2161};
        CHECK(tracker->draw_final(ref,7,0,source,{0,0,1920,2160}));
        if(mode!=1)CHECK(tracker->draw_final(ref,7,1,source,{1920,0,1920,2160}));
        if(mode==2)CHECK(tracker->draw_unknown(ref,source.resource));
        if(mode==3)tracker->reset_success(1); // Reset invalidates the old generation.
        if(mode==4){CHECK(tracker->execute(200,&ref,1));ref=tracker->reset_success(2);}
        if(mode==5){auto other=tracker->reset_success(2);
            CHECK(tracker->draw_final(other,8,0,{102,3840,2160},{0,0,1920,2160}));}
        if(mode==6)tracker->invalidate_all();
        auto target=source;if(mode==9)--target.height;
        auto admission=material_target_pair(*tracker,ref,target,mode==7?28:24,false,mode!=8);
        CHECK(admission.linked==(mode==0));
        if(mode==0){
            CHECK(admission.ticket==7&&!admission.from_executed);
            CHECK(tracker->draw_overlay_pair(ref,7,source,admission.eyes,3));
            CHECK(tracker->draw_transfer(ref,source,destination));
            CHECK(tracker->execute(200,&ref,1));
            auto result=tracker->present(destination.resource,200);
            CHECK(result.linked&&result.ticket==7&&result.eyes[1].height==2161);
        }
        // Existing backbuffer executed-continuation support is retained, but
        // it must never admit that same source as an intermediate continuation.
        if(mode==4)CHECK(material_target_pair(*tracker,ref,source,24,true,true).linked);
    }
    std::puts("game Material certificate rejection/completion/two-eye restoration checks passed");
}
