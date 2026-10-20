#include "crosshair_finite_query.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_crosshair_finite_query;
static std::uint32_t bits(float value){std::uint32_t result{};std::memcpy(&result,&value,sizeof(result));return result;}
static gears_vr::StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
struct Fixture {
    Authority authority{1,2,3,4,5,6,7,8,7,9,10,11,bits(1.05f)};
    gears_crosshair_scene::Metadata data{};
    gears_vr::StereoMatrix view{identity()};
    std::int64_t now{150},frequency{1000};
    Fixture(){data.controller=1;data.pawn=2;data.weapon=3;data.local=5;data.family=data.owner=7;data.source=8;
        data.world=9;data.invocation=10;data.thread=11;data.tick=12;data.root_frame=13;data.stamp=authority.scene_stamp;
        data.begin_qpc=100;data.assignment_qpc=110;data.finish_qpc=120;data.scene_qpc=130;
        data.point={100,-70,2}; // Deliberately far off the optical ray.
        auto& writer=data.writer_view;writer.valid=writer.camera_binding_valid=true;writer.camera=4;writer.local=5;
        writer.level=6;writer.assignment_stamp=bits(1.f);writer.direction={0,0,1};}
    Reason run(gears_vr::StereoFiniteAim& out)const{return authorize(data,authority,view,now,frequency,out);}
};
static bool same(gears_vr::StereoFiniteAim const& a,gears_vr::StereoFiniteAim const& b){
    return a.valid==b.valid&&a.point==b.point&&a.world==b.world&&a.tick==b.tick;}
static void reject(Fixture const& fixture,Reason reason){
    gears_vr::StereoFiniteAim out{true,{7,8,9},10,11};const auto before=out;
    const auto actual=fixture.run(out);
    if(actual!=reason)std::fprintf(stderr,"finite query rejection: expected=%u actual=%u\n",unsigned(reason),unsigned(actual));
    assert(actual==reason&&same(out,before));
}
int main(){
    Fixture base;gears_vr::StereoFiniteAim out{};
    auto ads=base;ads.data.writer_view.aiming_known=ads.data.writer_view.aiming=true;
    bool aiming=false;
    assert(authorize(ads.data,ads.authority,ads.view,ads.now,ads.frequency,out,&aiming)==Reason::accepted&&aiming);
    ads.view[12]=1;aiming=false;
    assert(authorize(ads.data,ads.authority,ads.view,ads.now,ads.frequency,out,&aiming)==Reason::camera&&aiming);
    ads.now=151;aiming=true;
    assert(authorize(ads.data,ads.authority,ads.view,ads.now,ads.frequency,out,&aiming)==Reason::age&&!aiming);
    assert(base.run(out)==Reason::accepted&&out.valid&&out.point==base.data.point&&out.world==9&&out.tick==12);
    // Every expected identity/key field must be both nonzero and exact.
    for(auto member:{&Authority::controller,&Authority::pawn,&Authority::weapon,&Authority::camera,
        &Authority::local,&Authority::level,&Authority::family,&Authority::source,&Authority::owner}){
        auto f=base;f.authority.*member=0;reject(f,Reason::binding);
        f=base;++(f.authority.*member);reject(f,Reason::binding);
    }
    for(unsigned fault=0;fault<13;++fault){auto f=base;
        if(fault==0)f.authority.world=0;if(fault==1)++f.authority.world;
        if(fault==2)f.authority.invocation=0;if(fault==3)++f.authority.invocation;
        if(fault==4)f.authority.thread=0;if(fault==5)++f.authority.thread;
        if(fault==6)f.data.tick=0;if(fault==7)f.data.root_frame=0;
        if(fault==8)++f.data.stamp;if(fault==9)f.data.writer_view.valid=false;
        if(fault==10)f.data.writer_view.camera_binding_valid=false;
        if(fault==11)f.data.writer_view.local=99;
        if(fault==12)f.data.owner=99;
        reject(f,Reason::binding);
    }
    // QPC and engine-clock bounds are independent. Equality is allowed within
    // each continuity bracket; neither clock supplies the other's authority.
    auto f=base;f.now=151;reject(f,Reason::age); // 51 ms since begin.
    f=base;f.data.begin_qpc=99;reject(f,Reason::age);
    f=base;f.data.begin_qpc=f.data.assignment_qpc=100;
    assert(f.run(out)==Reason::accepted); // Exactly 50 ms since both.
    ++f.now;reject(f,Reason::age);
    f=base;f.data.begin_qpc=f.data.assignment_qpc=f.data.finish_qpc=f.data.scene_qpc=f.now;
    assert(f.run(out)==Reason::accepted); // Zero host age doesn't authorize stale engine stamps.
    f.authority.scene_stamp=f.data.stamp=bits(1.051f);reject(f,Reason::age);
    f=base;f.authority.scene_stamp=f.data.stamp=f.data.writer_view.assignment_stamp;
    assert(f.run(out)==Reason::accepted); // Engine equality is allowed, not required.
    for(unsigned fault=0;fault<10;++fault){f=base;
        if(fault==0)f.frequency=0;if(fault==1)f.frequency=-1000;
        if(fault==2)f.data.begin_qpc=0;if(fault==3)f.data.begin_qpc=-1;
        if(fault==4)f.data.assignment_qpc=99;if(fault==5)f.data.finish_qpc=109;
        if(fault==6)f.data.scene_qpc=119;if(fault==7)f.now=129;
        if(fault==8)f.now=-1;if(fault==9)f.data.assignment_qpc=-1;
        reject(f,Reason::clock);
    }
    for(float invalid:{-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){
        f=base;f.data.writer_view.assignment_stamp=bits(invalid);reject(f,Reason::clock);
        f=base;f.authority.scene_stamp=f.data.stamp=bits(invalid);reject(f,Reason::clock);
    }
    f=base;f.data.writer_view.assignment_stamp=bits(2.f);reject(f,Reason::clock); // Discontinuity/backwards.
    f=base;f.authority.scene_stamp=f.data.stamp=bits(10.f);reject(f,Reason::age);
    f=base;f.authority.scene_stamp=f.data.stamp=f.data.writer_view.assignment_stamp=bits(0.f);
    assert(f.run(out)==Reason::accepted); // Nonnegative engine time includes zero.
    for(unsigned axis=0;axis<3;++axis)for(float invalid:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){
        f=base;f.data.point[axis]=invalid;reject(f,Reason::geometry);
    }
    f=base;f.data.point[2]=0;reject(f,Reason::geometry);
    f=base;f.data.point[2]=-1;reject(f,Reason::geometry);
    f=base;f.view[14]=-3;reject(f,Reason::geometry); // Translation matters for a WORLD point.
    f=base;f.view[0]=2;reject(f,Reason::geometry);
    f=base;f.view[3]=1;reject(f,Reason::geometry);
    f=base;f.view[12]=std::numeric_limits<float>::quiet_NaN();reject(f,Reason::geometry);
    // Same UObject and a fresh timestamp cannot lend an old query to a moved
    // render camera. Recoil-sized pitch changes used to pass this admission.
    f=base;f.view[12]=500;reject(f,Reason::camera);
    for(double pitch:{-.042376,.042376}){
        f=base;f.data.point={0,0,800};const float cs=float(std::cos(pitch)),sn=float(std::sin(pitch));
        f.view={1,0,0,0,0,cs,sn,0,0,-sn,cs,0,0,0,0,1};
        reject(f,Reason::camera);
    }
    for(unsigned axis=0;axis<3;++axis){
        f=base;f.data.writer_view.location[axis]=1;reject(f,Reason::camera);
        for(float invalid:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
            f=base;f.data.writer_view.location[axis]=invalid;reject(f,Reason::camera);
            f=base;f.data.writer_view.direction[axis]=invalid;reject(f,Reason::camera);
        }
    }
    f=base;f.data.writer_view.direction={0,0,0};reject(f,Reason::camera);
    f=base;f.data.writer_view.direction={0,0,-1};reject(f,Reason::camera);
    f=base;f.data.writer_view.direction={0,.002f,1};reject(f,Reason::camera);
    f=base;f.data.writer_view.direction={0,.0005f,1};assert(f.run(out)==Reason::accepted);
    // A common world-origin translation is legal; off-ray collision points
    // remain legal too. Neither is confused with an out-of-date camera.
    f=base;f.view[12]=-16384;f.view[13]=8192;f.view[14]=-4096;
    f.data.writer_view.location={16384,-8192,4096};
    for(unsigned axis=0;axis<3;++axis)f.data.point[axis]+=f.data.writer_view.location[axis];
    assert(f.run(out)==Reason::accepted);
    // Actual quantized rotations/cancellation at larger world coordinates.
    f=base;const float cs=float(std::cos(.73)),sn=float(std::sin(.73));
    f.view={cs,0,sn,0,0,1,0,0,-sn,0,cs,0,0,0,0,1};
    f.data.writer_view.location={12345,-23456,34567};
    f.data.writer_view.direction={sn,0,cs};
    for(unsigned col=0;col<3;++col){double sum=0;for(unsigned row=0;row<3;++row)
        sum+=double(f.data.writer_view.location[row])*f.view[4*row+col];
        f.view[12+col]=float(-sum);
        f.data.point[col]=f.data.writer_view.location[col]+1000*f.data.writer_view.direction[col];}
    assert(f.run(out)==Reason::accepted);
    // Large positive QPC values do not overflow ordered age subtraction.
    f=base;f.now=(std::numeric_limits<std::int64_t>::max)();
    f.data.begin_qpc=f.now-50;f.data.assignment_qpc=f.now-40;f.data.finish_qpc=f.now-30;f.data.scene_qpc=f.now-20;
    assert(f.run(out)==Reason::accepted);
    std::puts("Experimental finite HUD-query eligibility binding/clock/age/world geometry tests passed");
}
