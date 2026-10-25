#include "run_camera_stabilizer.h"
#include <cstdio>
#include <cstring>
#include <limits>

using namespace gears_run_camera;
static unsigned checks{},failures{};
#define CHECK(x) do {++checks;if(!(x)){++failures;std::printf("FAIL line %u: %s\n",unsigned(__LINE__),#x);}} while(false)
static bool near(double a,double b){return std::abs(a-b)<.001;}
static Sample sample(){
    Sample s{};s.context_verified=s.eligible=s.standing_sm0_verified=true;
    s.key.session=s.key.world_generation=s.key.reference_generation=s.key.space_epoch=1;
    auto id=[](std::uintptr_t n){gears_anchor::Identity v{};v.address=n;v.cls=n+100;v.index=unsigned(n);v.name[0]=L'A';return v;};
    s.key.world=id(1);s.key.pawn=id(2);s.key.mesh=id(3);s.key.controller=id(4);
    s.frame=1;s.sample_ns=1000000000;s.eye_height=76;s.units_per_metre=100;return s;
}
static void next(Sample& s,std::uint64_t ns=100000000){++s.frame;s.sample_ns+=ns;}
static void calibrate(Kernel& k,Sample& s,Result& r){
    CHECK(k.update(s,s.sample_ns,r)==Status::calibrating);next(s);
    CHECK(k.update(s,s.sample_ns,r)==Status::calibrating);next(s);
    CHECK(k.update(s,s.sample_ns,r)==Status::calibrating);CHECK(k.calibrated());next(s);
}
static void unchanged(Result const& a,Result const& b){CHECK(std::memcmp(&a,&b,sizeof(a))==0);}
int main(){
    Kernel k;Sample s=sample();Result out{};calibrate(k,s,out);s.special_move=4;
    const double hmd[7]={.03,1.7,-.02,0,0,0,1};double hmd_before[7]{};std::memcpy(hmd_before,hmd,sizeof(hmd));
    // AQ4-inspired 43-unit terrain rise is legitimate root displacement. Eye
    // bob/height animation cannot affect SM4; PC yaw/pitch changes must survive.
    for(unsigned i=0;i<80;++i){
        const double t=double(i)/79;s.pawn_location={120*t,-200*t,43*t};
        s.eye_height=50+20*std::sin(t*40);s.controller_yaw_radians=t*1.2;s.controller_pitch_radians=.2*std::sin(t*3);
        CHECK(k.update(s,s.sample_ns,out)==Status::applied);
        CHECK(near(out.origin.z,43*t+76+10));
        CHECK(near(out.origin.x,s.pawn_location.x+35*std::cos(s.controller_yaw_radians)));
        CHECK(near(out.origin.y,s.pawn_location.y+35*std::sin(s.controller_yaw_radians)));
        CHECK(near(out.view[10],std::sin(s.controller_pitch_radians)));CHECK(near(out.view[8],0));
        CHECK(near(out.protected_height,76));next(s);
    }
    CHECK(std::memcmp(hmd,hmd_before,sizeof(hmd))==0);
    const auto saved=out;CHECK(k.update(s,s.sample_ns+50000001,out)==Status::rejected);unchanged(saved,out);
    CHECK(!k.calibrated());CHECK(k.update(s,s.sample_ns,out)==Status::rejected);unchanged(saved,out);
    // An exact retry is refused without refreshing freshness or returning data.
    k.reset();s=sample();calibrate(k,s,out);s.special_move=4;
    CHECK(k.update(s,s.sample_ns,out)==Status::applied);auto old=out;
    CHECK(k.update(s,s.sample_ns,out)==Status::rejected);CHECK(k.reason()==Reason::duplicate);unchanged(old,out);
    next(s);CHECK(k.update(s,s.sample_ns,out)==Status::applied);
    // Every identity/epoch component independently requires fresh calibration.
    for(unsigned axis=0;axis<10;++axis){
        Kernel fresh;auto a=sample();Result r{};calibrate(fresh,a,r);a.special_move=4;
        CHECK(fresh.update(a,a.sample_ns,r)==Status::applied);const auto before=r;next(a);
        switch(axis){case 0:++a.key.session;break;case 1:++a.key.world_generation;break;case 2:++a.key.reference_generation;break;
        case 3:++a.key.space_epoch;break;case 4:++a.key.world.index;break;case 5:++a.key.pawn.cls;break;
        case 6:++a.key.mesh.outer;break;case 7:++a.key.controller.name_index;break;case 8:a.context_verified=false;break;
        default:a.key.pawn.address=0;break;}
        CHECK(fresh.update(a,a.sample_ns,r)==Status::rejected);CHECK(!fresh.calibrated());unchanged(before,r);
    }
    // Cadence outage, backwards clock, calibration bob/crouch and zero epochs.
    for(unsigned bad=0;bad<8;++bad){
        Kernel fresh;auto a=sample();Result r{};calibrate(fresh,a,r);a.special_move=4;
        const auto before=r;
        switch(bad){case 0:a.sample_ns+=300000000;break;case 1:a.key.space_epoch=0;break;
        case 2:a.controller_pitch_radians=1.58;break;case 3:a.controller_yaw_radians=std::numeric_limits<double>::quiet_NaN();break;
        case 4:a.units_per_metre=0;break;case 5:a.eligible=false;break;
        case 6:a.pawn_location.z=std::numeric_limits<double>::infinity();break;default:a.sample_ns=1;break;}
        CHECK(fresh.update(a,a.sample_ns,r)==Status::rejected);unchanged(before,r);
    }
    k.reset();s=sample();out={};
    for(unsigned i=0;i<8;++i){s.eye_height=i%2?76:80;CHECK(k.update(s,s.sample_ns,out)==Status::calibrating);CHECK(!k.calibrated());next(s);}
    s.standing_sm0_verified=false;CHECK(k.update(s,s.sample_ns,out)==Status::inactive);s.special_move=4;next(s);
    CHECK(k.update(s,s.sample_ns,out)==Status::rejected);
    // Sprint exit/other special moves cannot keep overriding posture height.
    k.reset();s=sample();calibrate(k,s,out);s.special_move=4;CHECK(k.update(s,s.sample_ns,out)==Status::applied);
    next(s);s.special_move=12;old=out;CHECK(k.update(s,s.sample_ns,out)==Status::inactive);unchanged(old,out);
    next(s);s.special_move=4;CHECK(k.update(s,s.sample_ns,out)==Status::rejected);
    // Sustained authenticated SM4 has no arbitrary timeout that reintroduces bob.
    k.reset();s=sample();calibrate(k,s,out);s.special_move=4;
    for(unsigned i=0;i<350;++i){s.eye_height=40+double(i%40);CHECK(k.update(s,s.sample_ns,out)==Status::applied);CHECK(near(out.origin.z,86));next(s);}
    for(double pitch:{-1.57079632679489661923,1.57079632679489661923}){
        s.controller_pitch_radians=pitch;CHECK(k.update(s,s.sample_ns,out)==Status::applied);
        CHECK(near(out.origin.z,86));CHECK(near(out.origin.x,35));CHECK(near(out.view[10],std::sin(pitch)));next(s);
    }
    // Long stable standing stays calibrated; no arbitrary two-second hole.
    k.reset();s=sample();calibrate(k,s,out);
    for(unsigned i=0;i<30;++i){CHECK(k.update(s,s.sample_ns,out)==Status::calibrating);CHECK(k.calibrated());next(s);}
    s.special_move=4;CHECK(k.update(s,s.sample_ns,out)==Status::applied);old=out;
    next(s);s.units_per_metre=101;CHECK(k.update(s,s.sample_ns,out)==Status::rejected);unchanged(old,out);
    for(double invalid_height:{-1.,0.,19.,251.,std::numeric_limits<double>::quiet_NaN()}){
        Kernel fresh;auto a=sample();Result r{};a.eye_height=invalid_height;
        CHECK(fresh.update(a,a.sample_ns,r)==Status::rejected);CHECK(!fresh.calibrated());
    }
    k.reset();s=sample();out={};old=out;
    CHECK(k.update(s,s.sample_ns-1,out)==Status::rejected);unchanged(old,out);
    std::printf("RunCameraStabilizer: checks=%u failures=%u; pure CPU only\n",checks,failures);return failures?1:0;
}
