#include "weapon_lancer_insertion.h"
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {
gears_weapon::LancerInsertionSample sample(double depth,std::uint64_t sequence,
                                           std::uint64_t created_ms){
    gears_weapon::LancerInsertionSample row{};
    row.weapon=1;row.component=2;row.world=3;row.session=4;
    row.space_epoch=5;row.input_epoch=6;row.world_generation=7;
    row.sequence=sequence;row.created_ms=created_ms;
    row.tracked=row.grip_down=true;
    row.gun_mesh={1,0,0,0,0,1,0,0,0,0,1,0,100,200,300,1};
    row.magazine.position={100+gears_weapon::lancer_magazine_upper_anchor_raw.x+1.8,
                           200+gears_weapon::lancer_magazine_upper_anchor_raw.y,
                           300+gears_weapon::lancer_magazine_upper_anchor_raw.z-depth};
    row.magazine.rotation.yaw=3.14159265358979323846/2;
    row.magazine.rotation.roll=3.14159265358979323846;
    return row;
}
}
int main(){
    using namespace gears_weapon;
    LancerMagazineInsertionStroke stroke;
    auto row=sample(10,1,1000);
    assert(stroke.arm(row));
    LancerInsertionGeometry geometry{};
    assert(stroke.observe(row,&geometry)==LancerInsertResult::approaching);
    assert(std::abs(geometry.depth_raw-10)<1e-5);
    assert(std::abs(geometry.lateral_raw-1.8)<1e-5);
    assert(geometry.axis_alignment>.999);
    assert(geometry.clock_alignment>.999);
    row=sample(5,2,1016);
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    row=sample(1.5,3,1032);
    assert(stroke.observe(row)==LancerInsertResult::inserted);
    assert(stroke.inserted());
    assert(stroke.observe(sample(1.5,4,1048))==LancerInsertResult::rejected);

    row=sample(1.5,1,2000);
    assert(stroke.arm(row));
    assert(stroke.observe(row)==LancerInsertResult::rejected); // no distant approach
    row=sample(10,2,2016);
    assert(stroke.observe(row)==LancerInsertResult::rejected); // too fast/outward
    row=sample(10,3,2032);
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    row=sample(5,4,2048);row.magazine.position.x+=10;
    assert(stroke.observe(row)==LancerInsertResult::rejected); // missed port

    row=sample(10,1,3000);assert(stroke.arm(row));
    row.magazine.rotation.roll=0;
    assert(stroke.observe(row)==LancerInsertResult::rejected); // upside down
    row=sample(10,2,3016);row.magazine.rotation.yaw=0;
    assert(stroke.observe(row)==LancerInsertResult::rejected); // reversed clocking
    row=sample(10,3,3032);
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    row=sample(9,4,3048);row.world_generation++;
    assert(stroke.observe(row)==LancerInsertResult::rejected);
    assert(!stroke.inserted());
    assert(stroke.observe(sample(5,5,3064))==LancerInsertResult::rejected);

    row=sample(10,1,4000);assert(stroke.arm(row));
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    assert(stroke.observe(sample(10,1,4000))==LancerInsertResult::rejected);
    row=sample(2,2,4010);
    assert(stroke.observe(row)==LancerInsertResult::rejected); // teleport
    row=sample(10,3,4026);
    assert(stroke.observe(row)==LancerInsertResult::rejected); // outward jump
    row=sample(10,4,4042);
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    row=sample(9,5,4200);
    assert(stroke.observe(row)==LancerInsertResult::rejected); // tracking gap
    assert(stroke.observe(sample(5,6,4216))==LancerInsertResult::rejected);

    row=sample(10,1,5000);
    row.gun_mesh={0,1,0,0,-1,0,0,0,0,0,1,0,100,200,300,1};
    row.magazine.position={100-lancer_magazine_upper_anchor_raw.y,
                           200+lancer_magazine_upper_anchor_raw.x+1.8,
                           300+lancer_magazine_upper_anchor_raw.z-10};
    row.magazine.rotation.yaw=3.14159265358979323846;
    assert(stroke.arm(row));
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    row.sequence=2;row.created_ms=5016;
    row.gun_mesh[12]+=50;row.magazine.position.x+=50;
    row.magazine.position.z+=5;
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    row.sequence=3;row.created_ms=5032;row.magazine.position.z+=3.5;
    assert(stroke.observe(row)==LancerInsertResult::inserted);
    std::puts("Lancer magazine insertion stroke passed");
}
