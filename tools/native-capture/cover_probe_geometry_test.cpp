#include <cfenv>
#include "cover_probe_geometry.h"
#include <cstdio>
#include <limits>
namespace p=gears_cover_probe_geometry;namespace q=p::q;
static unsigned checks{},failed{};
#define CHECK(x) do{++checks;if(!(x)){++failed;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
static bool probe_close(float a,float b){return std::abs(double(a)-b)<1e-6;}
int main(int argc,char**){q::Vector eye{100,200,300};q::Sphere sphere{};p::Path path{};
 if(argc>1){
  // Exact three former C0 probe constructor expressions, with units50/metre.
  sphere={eye,float(50*.01)};const float centimetre=float(50*.01);
  path.capsule={{eye.x,eye.y,eye.z-centimetre},{eye.x,eye.y,eye.z+centimetre},centimetre};path.delta={centimetre,0,0};
 }else{CHECK(p::sphere(eye,50,sphere)==p::Code::prepared);CHECK(p::path(eye,50,path)==p::Code::prepared);}
 CHECK(sphere.centre.x==2&&sphere.centre.y==4&&sphere.centre.z==6&&probe_close(sphere.radius,.01f));
 CHECK(probe_close(path.capsule.first.z,5.99f)&&probe_close(path.capsule.second.z,6.01f)&&probe_close(path.capsule.radius,.01f));
 CHECK(probe_close(path.delta.x,.01f)&&path.delta.y==0&&path.delta.z==0);
 if(argc>1){std::printf("ProbeUnitsCounter checks=%u failures=%u original expressions; no query\n",checks,failed);return failed?1:0;}
 CHECK(std::abs(path.epsilon-1e-5)<1e-12);
 eye={-4000,0,600};CHECK(p::sphere(eye,100,sphere)==p::Code::prepared&&probe_close(sphere.centre.x,-80)&&probe_close(sphere.centre.z,12)&&probe_close(sphere.radius,.02f));
 CHECK(p::path(eye,100,path)==p::Code::prepared&&probe_close(path.delta.x,.02f)&&std::abs(path.epsilon-2e-5)<1e-12);
 for(double units:{0.,-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::min(),std::numeric_limits<double>::max()}){
  sphere.radius=123;path.epsilon=123;CHECK(p::sphere(eye,units,sphere)==p::Code::value&&sphere.radius==123);CHECK(p::path(eye,units,path)==p::Code::value&&path.epsilon==123);}
 eye.x=std::numeric_limits<float>::quiet_NaN();sphere.radius=123;CHECK(p::sphere(eye,50,sphere)==p::Code::value&&sphere.radius==123);
 eye={0,0,1e20f};path.epsilon=123;CHECK(p::path(eye,50,path)==p::Code::value&&path.epsilon==123);
 CHECK(p::sphere(sphere.centre,50,sphere)==p::Code::alias&&sphere.radius==123);
 CHECK(p::path(path.capsule.first,50,path)==p::Code::alias&&path.epsilon==123);
 const auto old=std::fegetround();CHECK(std::fesetround(FE_UPWARD)==0);eye={1,2,3};const auto rounded=p::sphere(eye,50,sphere);
 CHECK(std::fesetround(old)==0);CHECK(rounded==p::Code::rounding&&sphere.radius==123);
 CHECK(!GetModuleHandleW(L"PhysXCore.dll"));
 std::printf("ProbeGeometry checks=%u failures=%u game-to-native diagnostic units only; no query\n",checks,failed);return failed?1:0;
}
