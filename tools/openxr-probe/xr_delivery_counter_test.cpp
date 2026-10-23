#include "xr_delivery_counter.h"
#include <cstdio>
#include <cstdlib>
static unsigned checks{};
static void check(bool value){++checks;if(!value)std::abort();}
int main(){using namespace gears_xr_delivery;Counter count;
    Key a{11,1,1,1,101};count.observe(false,a);check(!count.distinct&&!count.repeated);
    count.observe(true,a);check(count.distinct==1&&!count.repeated);
    count.observe(true,a);count.observe(true,a);check(count.distinct==1&&count.repeated==2);
    auto b=a;b.sequence=2;b.ticket=102;count.observe(false,b);check(count.distinct==1&&same(count.last,a));
    count.observe(true,b);check(count.distinct==2&&same(count.last,b));
    count.observe(true,a);check(count.rejected==1&&count.distinct==2);
    auto wrong=b;wrong.ticket=999;count.observe(true,wrong);check(count.rejected==2&&same(count.last,b));
    wrong=b;wrong.world=2;count.observe(true,wrong);check(count.rejected==3);
    wrong=b;wrong.session=12;wrong.sequence=3;count.observe(true,wrong);check(count.rejected==4);
    auto world=b;world.world=2;world.sequence=4;world.ticket=104;count.observe(true,world);check(count.distinct==3);
    wrong=world;wrong.world=1;wrong.sequence=5;count.observe(true,wrong);check(count.rejected==5);
    Key resize{11,2,2,1,105};count.observe(true,resize);check(count.distinct==4&&same(count.last,resize));
    count.observe(true,world);check(count.rejected==6);
    count.observe(true,{});check(count.rejected==7);
    count.observe(false,{});check(count.rejected==7);
    std::printf("{\"xr_delivery_counter_checks\":%u,\"copies_alone_not_counted\":true,\"failed_end_frame_not_counted\":true}\n",checks);
}
