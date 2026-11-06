#include "weapon_lancer_insertion.h"
#include "weapon_lancer_motion_samples.h"
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
    LancerMagazineExtractionStroke extraction;
    assert(!extraction.begin(sample(1.5,1,100),false));
    assert(!extraction.begin(sample(10,1,100),true));
    assert(extraction.begin(sample(1.5,1,100),true));
    assert(!extraction.observe(sample(2.5,2,116)));
    assert(extraction.observe(sample(5,3,132)));
    assert(!extraction.observe(sample(6,4,148))); // one completion only
    assert(extraction.begin(sample(1.5,1,200),true));
    assert(!extraction.observe(sample(10,2,201))); // teleport resets
    assert(!extraction.observe(sample(11,3,220)));
    assert(extraction.begin(sample(1.5,1,300),true));
    auto lost=sample(2,2,316);lost.tracked=false;
    assert(!extraction.observe(lost));
    assert(!extraction.observe(sample(5,3,332)));
    // Source0 owner13780: actual still-held/tracked cancellations at an
    // ordinary initial inward adjustment, not release or tracking loss.
    assert(extraction.begin(sample(0,1,400),true));
    assert(!extraction.observe(sample(-1.89392,2,470))&&extraction.active());
    assert(!extraction.observe(sample(-3.92132,3,540))&&extraction.active());
    assert(!extraction.observe(sample(-.5,4,610))&&extraction.active());
    assert(!extraction.observe(sample(0,5,680))&&extraction.active());
    assert(!extraction.observe(sample(2,6,750))&&extraction.active());
    assert(extraction.observe(sample(4.5,7,820))&&!extraction.active());
    // A held angular/lateral miss pauses the same grasp, with no removal
    // awarded outside its mechanical envelope. Return and actually pull.
    assert(extraction.begin(sample(0,1,900),true));
    auto adjusted=sample(0,2,950);adjusted.magazine.rotation.yaw+=1.4;
    assert(!extraction.observe(adjusted)&&extraction.active());
    adjusted=sample(6,3,1050);adjusted.magazine.position.x+=12;
    assert(!extraction.observe(adjusted)&&extraction.active());
    assert(!extraction.observe(sample(2,4,1100))&&extraction.active());
    assert(extraction.observe(sample(5,5,1150))&&!extraction.active());
    // The bounded visual seat stop does not change the physical sample or
    // mechanical depth. A displayed correction alone cannot extract a mag.
    auto inward=sample(-1.89392,1,1200);auto original=inward.magazine;
    gears_vr::Viewpoint displayed{};assert(lancer_extraction_display_target(inward,displayed));
    assert(std::abs(displayed.position.z-(300+lancer_magazine_upper_anchor_raw.z))<1e-6);
    assert(inward.magazine.position.z==original.position.z);
    inward=sample(-12,2,1250);assert(lancer_extraction_display_target(inward,displayed));
    assert(std::abs(displayed.position.z-inward.magazine.position.z+6)<1e-6);
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
    row=sample(9,5,4300);
    assert(stroke.observe(row)==LancerInsertResult::approaching); // fresh baseline, no integrated gap travel
    assert(stroke.observe(sample(5,6,4316))==LancerInsertResult::approaching);
    assert(stroke.observe(sample(1.5,7,4332))==LancerInsertResult::inserted);

    row=sample(9,1,4500);assert(stroke.arm(row));
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    assert(stroke.observe(sample(1.5,2,4751))==LancerInsertResult::rejected); // long gap cannot invent insertion
    assert(stroke.observe(sample(1.5,3,4767))==LancerInsertResult::rejected);
    assert(stroke.observe(sample(9,4,4801))==LancerInsertResult::approaching);
    row=sample(5,5,4817);row.tracked=false;
    assert(stroke.observe(row)==LancerInsertResult::rejected);
    assert(stroke.observe(sample(1.5,6,4833))==LancerInsertResult::rejected); // real tracking loss stays disarmed

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
    {
    // Actual geometry may be less frequent than fresh controller input. A
    // skipped 13 Hz frame must not erase an ordinary, still-held short pull.
    assert(extraction.begin(sample(1,1,50000),true));
    assert(extraction.observe(sample(7.6667,16,50167)));
    row=sample(10,1,51000);assert(stroke.arm(row));
    assert(stroke.observe(row)==LancerInsertResult::approaching);
    assert(stroke.observe(sample(2,16,51167))==LancerInsertResult::inserted);
    // The inclusive boundary does not waive explicit tracking loss, a new
    // identity, release, excessive speed, or the absence of an initial grab.
    assert(extraction.begin(sample(1,1,52000),true));
    assert(extraction.observe(sample(5,2,52250)));
    assert(extraction.begin(sample(1,1,53000),true));
    lost=sample(5,2,53167);lost.tracked=false;
    assert(!extraction.observe(lost)&&!extraction.active());
    assert(extraction.begin(sample(1,1,54000),true));
    lost=sample(5,2,54167);lost.grip_down=false;
    assert(!extraction.observe(lost)&&!extraction.active());
    assert(extraction.begin(sample(1,1,55000),true));
    lost=sample(5,2,55167);++lost.session;
    assert(!extraction.observe(lost)&&!extraction.active());
    }
    {
    extraction.reset();
    assert(extraction.begin(sample(1.5,1,6000),true));
    assert(!extraction.observe(sample(1.5,2,6251))&&extraction.active());
    assert(!extraction.observe(sample(3,3,6267))&&extraction.active());
    assert(extraction.observe(sample(5,4,6283))&&!extraction.active());
    assert(extraction.begin(sample(1.5,1,7000),true));
    assert(!extraction.observe(sample(5,2,7251))&&extraction.active());
    assert(!extraction.observe(sample(5,3,7267))); // Reappearing/holding still is not travel.
    assert(!extraction.observe(sample(8,4,7283))&&extraction.active());
    assert(extraction.observe(sample(9.5,5,7299))&&!extraction.active()); // Entire new stroke.
    assert(extraction.begin(sample(1.5,1,7200),true));
    assert(!extraction.observe(sample(21,2,7451))&&extraction.active());
    assert(!extraction.observe(sample(21,3,7467))&&extraction.active());
    assert(!extraction.observe(sample(15,4,7499))&&extraction.active()); // New valid baseline only.
    assert(!extraction.observe(sample(19,5,7515))&&extraction.active());
    assert(extraction.observe(sample(19.5,6,7531))&&!extraction.active());
    assert(extraction.begin(sample(0,1,7540),true));
    assert(!extraction.observe(sample(-3.92132,2,7791))&&extraction.active());
    assert(!extraction.observe(sample(0,3,7811))&&extraction.active());
    assert(extraction.observe(sample(4.5,4,7831))&&!extraction.active());
    assert(extraction.begin(sample(1.5,1,7400),true));
    assert(!extraction.observe(sample(5,2,7651))&&extraction.active());
    lost=sample(10,3,7667);lost.grip_down=false;
    assert(!extraction.observe(lost)&&!extraction.active());
    assert(extraction.begin(sample(1.5,1,7600),true));
    lost=sample(5,2,7703);++lost.weapon;
    assert(!extraction.observe(lost)&&!extraction.active());
    assert(extraction.begin(sample(1.5,1,8000),true));
    lost=sample(1.5,2,8103);lost.tracked=false;
    assert(!extraction.observe(lost)&&!extraction.active());
    assert(extraction.failure().reason==1); // Real loss still invalidates the gesture.
    }
    {
    LancerMotionSamples captured;
    auto start=sample(1.5,1,9000);
    assert(extraction.begin(start,true));captured.begin(1,41,start.sequence);
    // A sparse PlayerTick must consume the actual intervening poses, while
    // without crediting any unobserved movement across a gap.
    assert(captured.push(1,41,sample(2,2,9050)));
    assert(captured.push(1,41,sample(3,3,9100)));
    bool completed=false;
    assert(captured.consume(1,41,sample(5,4,9150),[&](auto const& s){
        if(!completed)completed=extraction.observe(s);
    }));assert(completed);
    unsigned repeats=0;
    assert(captured.consume(1,41,sample(5,4,9150),[&](auto const&){++repeats;}));assert(!repeats);
    assert(!captured.push(1,42,sample(6,5,9170))); // another grip cannot inherit it
    assert(!captured.consume(2,41,sample(6,5,9170),[](auto const&){}));
    captured.clear();assert(!captured.consume(1,41,sample(6,5,9170),[](auto const&){}));

    start=sample(8,1,10000);assert(stroke.arm(start));stroke.observe(start);captured.begin(4,77,1);
    assert(captured.push(4,77,sample(6,2,10050)));
    assert(captured.push(4,77,sample(4,3,10100)));
    auto inserted=LancerInsertResult::rejected;
    assert(captured.consume(4,77,sample(2,4,10150),[&](auto const& s){inserted=stroke.observe(s);}));
    assert(inserted==LancerInsertResult::inserted);
    start=sample(8,1,11000);assert(stroke.arm(start));stroke.observe(start);captured.begin(4,78,1);
    lost=sample(6,2,11050);lost.tracked=false;assert(captured.push(4,78,lost));
    assert(captured.consume(4,78,sample(2,3,11100),[&](auto const& s){inserted=stroke.observe(s);}));
    assert(inserted==LancerInsertResult::rejected); // actual loss cannot be bridged
    captured.begin(1,91,1);
    for(unsigned i=2;i<34;++i)assert(captured.push(1,91,sample(2,i,12000+i)));
    assert(!captured.push(1,91,sample(5,34,12034)));
    assert(!captured.consume(1,91,sample(5,35,12035),[](auto const&){})); // no silent history loss
    }
    std::puts("Lancer magazine insertion stroke and captured motion passed");
}
