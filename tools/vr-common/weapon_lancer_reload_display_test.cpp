#include "weapon_lancer_reload_display.h"
#include "weapon_lancer_reload_parent.h"
#include "hud_action_layout.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_lancer_reload_display;
// Same fade equations as the retained retail DrawActionReloadBar script's
// bActiveReloadAttempted branch. This reproduces the old disappeared bar.
static float alpha(float now,float start,float duration,float marker,float fade,float flash){
    const float elapsed=now-start;
    const float left=(std::min)(start+duration-now,flash-(elapsed-marker));
    return (std::min)(std::clamp(left/flash,0.f,1.f),std::clamp((fade-(elapsed-marker))/fade,0.f,1.f));
}
int main(){
    // Metadata of the actual shipped IsReloading return. The old0x480
    // assumption rejected the adapter before any HUD caller was observed.
    using gears_lancer_reload_parent::query_return_layout;
    assert(query_return_layout(1,4,0x580,0,1));
    assert(!query_return_layout(1,4,0x480,0,1));
    assert(!query_return_layout(1,4,0x180,0,1));
    assert(!query_return_layout(1,4,0x500,0,1));
    assert(!query_return_layout(2,4,0x580,0,1));
    assert(!query_return_layout(1,8,0x580,0,1));
    assert(!query_return_layout(1,4,0x580,4,1));
    assert(!query_return_layout(1,4,0x580,0,2));
    // Relocated64-bit form of the observed first parent HUD condition. Reject
    // second queries, changed short-circuit flow, pointers and script bounds.
    std::array<unsigned char,gears_lancer_reload_parent::span> gate{};
    gate[0]=7;gate[1]=0xbd;gate[2]=2;gate[3]=0x82;gate[4]=0x19;gate[5]=0;
    gate[14]=10;gate[16]=4;gate[17]=0x1c;gate[26]=0x16;gate[27]=0x18;
    gate[28]=11;gate[30]=0x2d;gate[31]=0;gate[40]=0x16;gate[41]=0x1c;gate[50]=0;
    auto put=[&](unsigned at,std::uint64_t value){std::memcpy(gate.data()+at,&value,8);};
    put(6,0x100001000);put(18,0x100002000);put(32,0x100003000);put(42,0x100004000);put(51,0x100001000);
    auto accepted=[&](){return gears_lancer_reload_parent::direct_bar_gate(gate,0x1dc,0x856,
        0x100001000,0x100002000,0x100003000,0x100004000);};
    assert(accepted());
    for(unsigned at:{0u,1u,3u,4u,5u,6u,14u,16u,17u,18u,26u,27u,28u,30u,31u,32u,40u,41u,42u,50u,51u}){
        gate[at]^=1;assert(!accepted());gate[at]^=1;
    }
    assert(!gears_lancer_reload_parent::direct_bar_gate(gate,0x1dc,0x2bd,
        0x100001000,0x100002000,0x100003000,0x100004000));
    assert(!gears_lancer_reload_parent::direct_bar_gate(gate,0x500,0x856,
        0x100001000,0x100002000,0x100003000,0x100004000));
    // Equivalent readability at1080p, actual private3840x2160 Canvas and
    // runtime-sized viewports. No guessed desktop screenshot scale.
    Layout drawing{};
    for(auto size:{std::array<float,2>{{1920,1080}},std::array<float,2>{{3840,2160}},std::array<float,2>{{8192,3288}}}){
        assert(layout(size[0],size[1],256,8,2,drawing));
        assert(std::abs(drawing.width/size[0]-.2f)<.00001f);
        assert(std::abs((drawing.x+drawing.width*.5f)/size[0]-.5f)<.00001f);
        assert(std::abs(drawing.height/size[1]-.025f)<.00001f);
        assert(drawing.x>0&&drawing.x+drawing.width<size[0]&&drawing.y+drawing.height<size[1]);
    }
    assert(layout(3840,2160,245,7,2,drawing));
    assert(drawing.height==54.f); // Actual old atlas ratio produced only21.94px.
    assert(std::abs(drawing.width-245*2*drawing.scale)<.0001f);
    const auto original=drawing;
    assert(layout(3840,2160,245,7,2,drawing,true));
    assert(drawing.resolution_scale==4&&drawing.x==original.x&&drawing.y==original.y&&
        drawing.width==original.width&&drawing.height==original.height);
    assert(std::abs(245*drawing.scale*drawing.resolution_scale-original.width)<.0001f);
    assert(std::abs(drawing.scale*2-original.scale)<.0001f);
    // Retained original atlas dimensions: tick20x40, background245x7. Original
    // UV extents remain20x40; only its drawn size increases40x80 ->80x160.
    assert(20*drawing.resolution_scale==80&&40*drawing.resolution_scale==160);
    assert(layout(3840,2160,245,7,16,drawing,true)&&drawing.resolution_scale==16);
    assert(layout(3840,2160,245,7,2,drawing));
    const float eye_width=2160.f*4096.f/3288.f,eye_left=(3840.f-eye_width)*.5f;
    assert(3840.f*.12f<eye_left); // Captured old placement is left of the fitted eye.
    assert(drawing.x>eye_left&&drawing.x+drawing.width<eye_left+eye_width);
    assert(!layout(3840,2160,0,8,2,drawing));
    assert(!layout(3840,2160,256,8,0,drawing));
    assert(!layout(3840,2160,2,256,2,drawing));
    assert(!layout(std::numeric_limits<float>::quiet_NaN(),2160,256,8,2,drawing));
    const std::array<float,4> vr{{0,0,4,0}};Plan p{};
    assert(alpha(110,100,4,4,.25f,.2f)==0); // old timed fade, physical reload still open
    for(float elapsed:{0.f,1.f,3.8f,4.f,4.01f,10.f,600.f}){
        assert(plan(100+elapsed,100,4,.25f,.2f,vr,p));
        assert(std::abs(p.marker-(std::min)(elapsed,4.f))<.00001f);
        assert(alpha(100+elapsed,p.draw_start,4,p.marker,.25f,.2f)>.99f);
        assert(p.ordinary==(elapsed>4));
        assert(p.window[2]==(elapsed>4?0.f:4.f));
        assert(p.window[3]==(elapsed>4?4.f:0.f));
    }
    // Configured duration, no reward, and a short duration remain visible.
    assert(alpha(110,106.5f,4,4,.5f,1.f)==.5f); // Actual old fade-only margin.
    assert(plan(110,100,4,.5f,1.f,vr,p)&&alpha(110,p.draw_start,4,p.marker,.5f,1.f)==1);
    assert(plan(109,100,6,.25f,.2f,{{0,0,6,0}},p)&&p.marker==6&&p.ordinary);
    assert(plan(101,100,4,.25f,.2f,{{0,0,0,0}},p)&&p.ordinary&&p.window[3]==4);
    assert(plan(110,100,.1f,.25f,.2f,{{0,0,.1f,0}},p)&&p.draw_start==110);
    const auto before=p;
    for(float bad:{-1.f,0.f,61.f,std::numeric_limits<float>::quiet_NaN()}){
        assert(!plan(110,100,bad,.25f,.2f,vr,p)&&p.draw_start==before.draw_start);
    }
    assert(!plan(99,100,4,.25f,.2f,vr,p));
    assert(!plan(110,100,4,.25f,.2f,{{0,0,5,0}},p));
    assert(!plan(110,100,4,-.1f,.2f,vr,p));
    assert(!plan(110,100,4,.5f,std::numeric_limits<float>::quiet_NaN(),vr,p));
    // The real reward remains outside the display adapter. Its confirmed
    // result starts fully visible and expires, including after a late socket.
    for(float age:{0.f,.1f,.5f,.9f}){
        assert(result_plan(120+age,120,2.5f,.5f,1.f,p));
        assert(alpha(120+age,p.draw_start,2.5f,p.marker,.5f,1.f)>0);
    }
    assert(result_plan(120,120,2.5f,.5f,1.f,p)&&alpha(120,p.draw_start,2.5f,p.marker,.5f,1.f)==1);
    assert(!result_plan(121,120,2.5f,.5f,1.f,p));
    assert(!result_plan(119,120,2.5f,.5f,1.f,p));
    assert(!result_plan(120,120,2.5f,0,1.f,p));
    assert(!result_plan(120,120,2.5f,.5f,std::numeric_limits<float>::quiet_NaN(),p));
    // The existing two-float scope restores the real weapon timestamp and
    // HUD marker after drawing; a foreign writer is preserved as cleanup debt.
    std::array<float,2> memory{{100,4}};
    gears_hud_action_layout::Scope scope{{{0x100,0x104}},memory,{{106.25f,4}},0};
    auto read=[&](auto at,float& out){if(at!=0x100&&at!=0x104)return false;out=memory[(at-0x100)/4];return true;};
    auto write=[&](auto at,float v){if(at!=0x100&&at!=0x104)return false;memory[(at-0x100)/4]=v;return true;};
    assert(scope.apply(read,write)&&memory[0]==106.25f);
    assert(scope.restore(read,write)&&memory[0]==100&&memory[1]==4);
    assert(scope.apply(read,write));memory[0]=200;
    assert(!scope.restore(read,write)&&memory[0]==200&&scope.owned==1);
    std::puts("Lancer reload display: retail fade reproduced; pending/late/configured/off windows and scoped clock restoration passed (CPU only)");
}
