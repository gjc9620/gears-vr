#include "eye_chain_policy.h"
#include <cstdio>
#include <limits>
#define CHECK(value) do{if(!(value)){std::printf("Failed line %d: %s\n",__LINE__,#value);return 1;}}while(0)
int main() {
    using namespace gears_eye_chain;
    Limits limits{4096,4096,{4096,4096},{4096,4096},{1,1}};
    Plan plan{};
    CHECK(make_plan(4096,2048,limits,plan));
    CHECK(plan.eye_width==2048&&plan.eye_height==2048&&plan.crops[1].x==2048);
    CHECK(make_plan(8192,4096,limits,plan));
    CHECK(plan.source_width==8192&&plan.eye_width==4096&&plan.crops[0].x==0&&plan.crops[1].x==4096);
    CHECK(plan.crops[0].width==4096&&plan.crops[1].width==4096&&plan.crops[0].height==4096);
    gears_vr::StereoImageTicket ticket{};ticket.id=7;
    ticket.rect[0]=plan.crops[0];ticket.rect[1]=plan.crops[1];
    CHECK(ticket_crops_match(plan,ticket));
    for(unsigned field=0;field<5;++field) {
        auto wrong=ticket;
        if(field==0)wrong.id=0;
        if(field==1)++wrong.rect[1].x;
        if(field==2)--wrong.rect[0].width;
        if(field==3)--wrong.rect[1].height;
        if(field==4)++wrong.rect[0].y;
        CHECK(!ticket_crops_match(plan,wrong));
    }
    for(unsigned dimension : {0u,8193u,std::numeric_limits<unsigned>::max()}) {
        auto sentinel=plan;
        CHECK(!make_plan(dimension,4096,limits,plan));
        CHECK(plan.source_width==sentinel.source_width&&plan.crops[1].x==sentinel.crops[1].x);
        CHECK(!make_plan(8192,dimension,limits,plan));
    }
    CHECK(!make_plan(8191,4096,limits,plan));
    for(unsigned field=0;field<6;++field) {
        auto restricted=limits;
        if(field==0)restricted.system_width=4095;
        if(field==1)restricted.system_height=4095;
        if(field==2)restricted.view_width[0]=4095;
        if(field==3)restricted.view_width[1]=4095;
        if(field==4)restricted.view_height[1]=4095;
        if(field==5)restricted.view_samples[0]=0;
        CHECK(!make_plan(8192,4096,restricted,plan));
    }
    CHECK(make_plan(3840,2160,limits,plan)&&plan.eye_width==1920&&plan.eye_height==2160);
    CHECK(make_plan(3842,2005,limits,plan)&&plan.eye_width==1921&&plan.eye_height==2005);
    gears_vr::RenderSizeLimits size_limits{{3072,3072},{3264,3264},{4096,4096},{4096,4096},4096,4096};
    CHECK(gears_vr::make_render_size_plan(size_limits,1.,limits.render_size)==gears_vr::RenderSizeError::none);
    limits.independent_render_size=true;
    CHECK(make_plan(6144,3264,limits,plan)&&plan.eye_width==3072&&plan.eye_height==3264);
    CHECK(!make_plan(3840,2160,limits,plan)); // No XR upscale from a desktop-sized source.
    CHECK(!make_plan(6144,3263,limits,plan));
    std::puts("eye chain policy tests passed: Low/High/exact crops/limits/invalid/provenance");return 0;
}
