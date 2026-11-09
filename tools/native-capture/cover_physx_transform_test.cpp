#include "cover_physx_transform.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
namespace api=gears_cover_physx_transform;
namespace b=api::body;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
bool near(double a,double d){return std::abs(a-d)<2e-5;}
b::Matrix64 rigid(b::Quat q,b::Vec p){
    b::Matrix64 out{};const b::Vec axes[]={b::d::rotate(q,{1,0,0}),b::d::rotate(q,{0,1,0}),b::d::rotate(q,{0,0,1})};
    for(unsigned i=0;i<3;++i){out.values[4*i]=float(axes[i].x);out.values[4*i+1]=float(axes[i].y);out.values[4*i+2]=float(axes[i].z);}
    out.values[12]=float(p.x);out.values[13]=float(p.y);out.values[14]=float(p.z);out.values[15]=1;return out;
}
void geometry(b::Matrix64 const& source){
    api::Matrix34 next{};C(api::convert(source,next)==api::Code::converted);
    // Independent point-transform identity: Physics(Nx*p) must be the same
    // scaled world point as UE's row-vector transform for all basis points.
    const b::Vec probes[]={{0,0,0},{1,0,0},{0,1,0},{0,0,1},{2.25,-3.5,4.75}};
    for(auto p:probes){const double v[]={p.x,p.y,p.z};
        for(unsigned row=0;row<3;++row){
            double native=next.values[9+row];
            for(unsigned k=0;k<3;++k)native+=next.values[3*row+k]*(v[k]*double(api::game_to_physics));
            const double game=(v[0]*source.values[row]+v[1]*source.values[4+row]+v[2]*source.values[8+row]+source.values[12+row])*double(api::game_to_physics);
            C(near(native,game));
        }
    }
}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    std::uint32_t bits{};std::memcpy(&bits,&api::game_to_physics,4);C(bits==0x3ca3d70a);
    b::Matrix64 captured{};std::ifstream input(argv[1],std::ios::binary);
    C(bool(input.read(reinterpret_cast<char*>(&captured),sizeof captured))&&input.peek()==std::char_traits<char>::eof());
    if(failures)return 1;geometry(captured);
    const double n=std::sqrt(.93);b::Quat q{};
    C(b::d::normalized({.2/n,-.3/n,.4/n,.8/n},q));
    geometry(rigid(q,{120,-240,70}));geometry(rigid({0,0,0,1},{0,0,0}));
    const auto original=rigid(q,{10,-20,30});api::Matrix34 rejected{},before{};
    std::memset(&rejected,0x5a,sizeof rejected);before=rejected;
    auto fail=[&](b::Matrix64 source,api::Code code){C(api::convert(source,rejected)==code);C(!std::memcmp(&rejected,&before,sizeof before));};
    auto bad=original;bad.values[0]=NAN;fail(bad,api::Code::nonfinite);
    bad=original;bad.values[7]=.001f;fail(bad,api::Code::affine);
    bad=original;for(unsigned i=0;i<3;++i)bad.values[i]*=2;fail(bad,api::Code::nonrigid);
    bad=rigid({0,0,0,1},{0,0,0});bad.values[0]=-1;fail(bad,api::Code::nonrigid);
    bad=original;bad.values[12]=FLT_MIN;fail(bad,api::Code::numeric);
    // Actual overlapping storage, kept suitably aligned for both POD types.
    alignas(b::Matrix64) unsigned char storage[sizeof(b::Matrix64)]{};
    auto* alias_source=::new(storage)b::Matrix64(original);unsigned char saved[sizeof storage];std::memcpy(saved,storage,sizeof storage);
    auto* alias_output=reinterpret_cast<api::Matrix34*>(storage+sizeof(float));
    C(api::convert(*alias_source,*alias_output)==api::Code::alias);C(!std::memcmp(saved,storage,sizeof storage));
    std::printf("CoverPhysXTransform checks=%u failed=%u original-layout-game-to-physics-only\n",checks,failures);return failures?1:0;
}
