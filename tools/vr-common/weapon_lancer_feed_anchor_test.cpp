#include "weapon_lancer_feed_anchor.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main(){
    const std::array<float,16> rotated{
        0,1,0,0,-1,0,0,0,0,0,1,0,100,200,300,1};
    gears_weapon::LancerFeedWorld feed{};
    assert(gears_weapon::lancer_feed_world(rotated,feed));
    const auto local=gears_weapon::lancer_magazine_upper_anchor_raw;
    assert(std::abs(feed.point_raw.x-(100-local.y))<1e-5);
    assert(std::abs(feed.point_raw.y-(200+local.x))<1e-5);
    assert(std::abs(feed.point_raw.z-(300+local.z))<1e-5);
    assert(feed.outward_axis.x==0&&feed.outward_axis.y==0&&feed.outward_axis.z==-1);
    auto invalid=rotated;
    invalid[0]=.2f;
    assert(!gears_weapon::lancer_feed_world(invalid,feed));
    invalid=rotated;invalid[10]=-1;
    assert(!gears_weapon::lancer_feed_world(invalid,feed));
    std::puts("Lancer fixed gun-frame feed anchor passed");
}
