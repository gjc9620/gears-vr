#include "weapon_lancer_magazine_pose.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

int main(){
    gears_vr::Viewpoint left{{100,200,300},{0,0,0}},magazine{};
    assert(gears_weapon::lancer_magazine_from_left(left,magazine));
    assert(std::abs(magazine.position.x-119.03603543)<1e-5);
    assert(std::abs(magazine.position.y-194.43382251)<1e-5);
    assert(std::abs(magazine.position.z-300.13964627)<1e-5);
    const auto basis=gears_vr::ue_matrix(magazine.rotation);
    constexpr double expected[3][3]={
        {.03740158,.13477868,-.99016975},
        {-.03192186,-.99019607,-.13598739},
        {-.99879037,.03669508,-.03273385}};
    for(unsigned axis=0;axis<3;++axis)for(unsigned component=0;component<3;++component)
        assert(std::abs(basis[component][axis]-expected[axis][component])<2e-4);
    left.rotation.yaw=1.2;
    assert(gears_weapon::lancer_magazine_from_left(left,magazine));
    const auto rotated=gears_vr::transform(gears_vr::ue_matrix(left.rotation),
                                          {19.03603543,-5.56617749,.13964627});
    assert(std::abs(magazine.position.x-(left.position.x+rotated.x))<1e-5);
    left.position.x=std::numeric_limits<double>::infinity();
    assert(!gears_weapon::lancer_magazine_from_left(left,magazine));
    std::puts("Lancer original magazine hand mount passed");
}
