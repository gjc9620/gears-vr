#include "hand_personal_reference.h"
#include "cover_skeleton_ik.h"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <type_traits>

namespace p=gears_hand_personal;
namespace r=gears_hand_personal_reference;
namespace m=gears_hand_authored_mount;
namespace ik=gears_cover_skeleton_ik;
namespace {
unsigned checks{};
void check(bool okay){++checks;if(!okay){std::fprintf(stderr,"personal calibration check %u failed\n",checks);std::exit(1);}}
bool near(double a,double b,double tolerance=1e-12){return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=tolerance;}
template<class T> std::array<unsigned char,sizeof(T)> bytes(T const& value){
    std::array<unsigned char,sizeof(T)> out{};std::memcpy(out.data(),&value,sizeof(value));return out;
}
void put(m::CanonicalGeometry& geometry,std::uint32_t index,std::int32_t parent,
    std::array<float,3> translation,std::array<float,4> q={{0,0,0,1}}){
    auto* at=geometry.data()+std::size_t(index)*36;
    std::memcpy(at,&index,4);std::memcpy(at+4,&parent,4);std::memcpy(at+8,q.data(),16);std::memcpy(at+24,translation.data(),12);
}
m::CanonicalGeometry synthetic(){
    m::CanonicalGeometry out{};
    for(std::uint32_t i=0;i<115;++i)put(out,i,0,{{0,0,0}});
    // Deliberately nonzero ancestors: local shoulder offsets alone give the
    // wrong shoulder width. Claimed model signature here is a HOST seam only.
    put(out,6,0,{{1,-5,2}});put(out,8,6,{{-1,-15,-2}});
    put(out,10,8,{{30,0,0}});put(out,11,10,{{25,0,0}});
    put(out,30,0,{{1,5,2}});put(out,32,30,{{-1,15,-2}});
    put(out,34,32,{{35,0,0}});put(out,35,34,{{20,0,0}});
    return out;
}
std::array<m::Mount,2> mounts(double scale=1,double units=100){
    std::array<m::Mount,2> out{};
    check(m::marcus115(m::marcus115_signature,m::Side::left,scale,units,out[0]));
    check(m::marcus115(m::marcus115_signature,m::Side::right,scale,units,out[1]));return out;
}
p::Dimensions measure(m::CanonicalGeometry const& geometry,double scale=1,double units=100){
    const auto authored=mounts(scale,units);p::Dimensions out{};
    check(r::dimensions(geometry,authored[0],authored[1],out)==p::Status::okay);return out;
}
void scaling_and_profile(){
    const auto geometry=synthetic();const auto model=measure(geometry);
    check(near(model.shoulder_span_metres,.4));
    check(near(model.upper_metres[0],.3)&&near(model.lower_metres[0],.25));
    check(near(model.upper_metres[1],.35)&&near(model.lower_metres[1],.2));
    p::Measurement measurement{1,1.7,-.15};p::Profile profile{};
    check(p::derive(measurement,model,profile)==p::Status::okay);
    const double extension=model.authored_controller_extension_metres[0]+model.authored_controller_extension_metres[1];
    check(near(profile.scale,(1.7-.4-extension)/1.1));
    check(profile.provenance==p::Provenance::controller_span_model_estimate);
    check(near(.4+extension+profile.upper_metres[0]+profile.lower_metres[0]+profile.upper_metres[1]+profile.lower_metres[1],1.7));
    for(auto multiplier:{.5,1.,1.7,2.})for(unsigned side=0;side<2;++side){
        const auto u=model.upper_metres[side]*multiplier,l=model.lower_metres[side]*multiplier;
        double us=7,ls=9;
        check(p::scales(profile,side?m::Side::right:m::Side::left,u,l,us,ls)==p::Status::okay);
        check(near(u*us,profile.upper_metres[side])&&near(l*ls,profile.lower_metres[side]));
    }
    auto other_height=measurement;other_height.hands_below_head_metres=.05;p::Profile height_profile{};
    check(p::derive(other_height,model,height_profile)==p::Status::okay);
    check(height_profile.scale==profile.scale&&height_profile.upper_metres==profile.upper_metres&&height_profile.lower_metres==profile.lower_metres);
    for(auto policy:{std::array<double,3>{{2,100,2}},std::array<double,3>{{1,200,.5}},std::array<double,3>{{2,200,1}}}){
        const auto changed=measure(geometry,policy[0],policy[1]);
        check(near(changed.shoulder_span_metres,model.shoulder_span_metres*policy[2]));
        for(unsigned side=0;side<2;++side){
            check(near(changed.upper_metres[side],model.upper_metres[side]*policy[2]));
            check(near(changed.lower_metres[side],model.lower_metres[side]*policy[2]));
            check(near(changed.authored_controller_extension_metres[side],model.authored_controller_extension_metres[side]*policy[2]));
        }
    }
    auto rotated=geometry;const float h=static_cast<float>(std::sqrt(.5));put(rotated,0,0,{{7,-9,4}},{{0,0,h,h}});
    const auto invariant=measure(rotated);
    check(near(invariant.shoulder_span_metres,model.shoulder_span_metres));
    check(near(invariant.upper_metres[0],model.upper_metres[0])&&near(invariant.lower_metres[1],model.lower_metres[1]));
    // Padding is not identity or authorization and is not compared as data.
    auto padded=measurement;auto* raw=reinterpret_cast<unsigned char*>(&padded);
    for(std::size_t i=sizeof(padded.version);i<offsetof(p::Measurement,armspan_metres);++i)raw[i]=0xa5;
    p::Profile padded_profile{};check(p::derive(padded,model,padded_profile)==p::Status::okay);
    check(padded_profile.scale==profile.scale);
    raw=reinterpret_cast<unsigned char*>(&padded_profile);
    for(std::size_t i=offsetof(p::Profile,provenance)+sizeof(padded_profile.provenance);i<sizeof(padded_profile);++i)raw[i]=0x5a;
    double us{},ls{};check(p::scales(padded_profile,m::Side::left,.3,.25,us,ls)==p::Status::okay);
    const auto authored=mounts();const auto original_mounts=bytes(authored);
    const gears_vr::Pose grip{{17,-3,5},gears_vr::normalized({.2,-.3,.4,.8})};gears_vr::Pose before{},after{};
    check(gears_hand_wrist::native_wrist_target(grip,authored[0].calibration,100,before)==gears_hand_wrist::Status::okay);
    check(p::derive(measurement,model,profile)==p::Status::okay);
    check(gears_hand_wrist::native_wrist_target(grip,authored[0].calibration,100,after)==gears_hand_wrist::Status::okay);
    check(bytes(before)==bytes(after)&&bytes(authored)==original_mounts);
}
void rejected_profiles(){
    const auto model=measure(synthetic());const p::Measurement measurement{1,1.7,-.15};p::Profile output{};
    check(p::derive(measurement,model,output)==p::Status::okay);const auto saved=bytes(output);
    for(unsigned bad=0;bad<5;++bad){
        auto input=measurement;
        switch(bad){case 0:input.version=0;break;case 1:input.armspan_metres=.899;break;case 2:input.armspan_metres=2.401;break;
            case 3:input.hands_below_head_metres=.101;break;case 4:input.armspan_metres=std::numeric_limits<double>::quiet_NaN();break;}
        check(p::derive(input,model,output)==p::Status::invalid_measurement);check(bytes(output)==saved);
    }
    for(unsigned bad=0;bad<12;++bad){
        auto input=model;
        switch(bad){case 0:input.key.model=m::Model::unknown;break;case 1:input.key.geometry_signature[0]^=1;break;
            case 2:input.key.bone_count=114;break;case 3:input.key.model_to_world_scale=0;break;case 4:input.key.units_per_metre=0;break;
            case 5:input.shoulder_span_metres=0;break;case 6:input.upper_metres[1]=-1;break;case 7:input.lower_metres[0]=0;break;
            case 8:input.authored_controller_extension_metres[1]=-1;break;case 9:input.shoulder_span_metres=2.4;break;
            case 10:input.upper_metres={{(std::numeric_limits<double>::max)(),(std::numeric_limits<double>::max)()}};break;
            case 11:input.authored_controller_extension_metres[0]=std::numeric_limits<double>::infinity();break;}
        const auto status=p::derive(measurement,input,output);check(status!=p::Status::okay);check(bytes(output)==saved);
    }
    auto tiny=model;tiny.shoulder_span_metres=.8;tiny.authored_controller_extension_metres={{.05,.04}};
    tiny.upper_metres[0]=(std::numeric_limits<double>::denorm_min)();
    check(p::derive({1,.9,-.15},tiny,output)==p::Status::numerical_failure);check(bytes(output)==saved);
    check(p::derive(output.measurement,model,output)==p::Status::alias);check(bytes(output)==saved);
    check(p::derive(measurement,output.model,output)==p::Status::alias);check(bytes(output)==saved);
    for(unsigned bad=0;bad<7;++bad){
        auto input=output;double upper=7,lower=9;const auto u=bytes(upper),l=bytes(lower);auto side=m::Side::left;double length=.3;
        switch(bad){case 0:input.provenance=p::Provenance::unknown;break;case 1:input.scale*=2;break;
            case 2:input.upper_metres[0]*=2;break;case 3:input.model.key.geometry_signature[0]^=1;break;
            case 4:side=m::Side::unknown;break;case 5:length=0;break;case 6:length=(std::numeric_limits<double>::denorm_min)();break;}
        check(p::scales(input,side,length,.25,upper,lower)!=p::Status::okay);check(bytes(upper)==u&&bytes(lower)==l);
    }
    double upper=7,lower=9;
    check(p::scales(output,m::Side::left,.3,.25,upper,upper)==p::Status::alias);check(upper==7&&lower==9);
    check(p::scales(output,m::Side::left,.3,.25,output.scale,lower)==p::Status::alias);check(bytes(output)==saved&&lower==9);
}
void rejected_geometry(){
    const auto original=synthetic();const auto authored=mounts();auto output=measure(original);const auto saved=bytes(output);
    for(unsigned bad=0;bad<8;++bad){
        auto geometry=original;
        if(bad==0){const std::uint32_t wrong=2;std::memcpy(geometry.data()+114*36,&wrong,4);}
        if(bad==1)put(geometry,114,114,{{0,0,0}});
        if(bad==2)put(geometry,0,1,{{0,0,0}});
        if(bad==3)put(geometry,113,0,{{0,0,0}},{{0,0,0,0}});
        if(bad==4)put(geometry,113,0,{{std::numeric_limits<float>::quiet_NaN(),0,0}});
        if(bad==5)put(geometry,10,9,{{30,0,0}});
        if(bad==6)put(geometry,11,10,{{0,0,0}});
        if(bad==7)put(geometry,32,30,{{-1,-25,-2}});
        check(r::dimensions(geometry,authored[0],authored[1],output)!=p::Status::okay);check(bytes(output)==saved);
    }
    for(unsigned bad=0;bad<8;++bad){
        auto value=authored;
        switch(bad){case 0:value[0].side=m::Side::right;break;case 1:value[1].calibration.known=false;break;
            case 2:value[1].chain[0]=8;break;case 3:value[1].geometry_signature[1]^=1;break;case 4:value[1].provenance=m::Provenance::unknown;break;
            case 5:value[1].calibration.grip_to_wrist_rotation.w+=.1;break;case 6:value=mounts(2);value[0]=authored[0];break;
            case 7:value=mounts(1,200);value[0]=authored[0];break;}
        check(r::dimensions(original,value[0],value[1],output)==p::Status::invalid_model);check(bytes(output)==saved);
    }
}
template<class T> bool load(char const* path,T& out){
    std::ifstream stream(path,std::ios::binary);if(!stream)return false;
    stream.read(reinterpret_cast<char*>(&out),std::streamsize(sizeof(out)));
    return stream.gcount()==std::streamsize(sizeof(out))&&stream.peek()==std::ifstream::traits_type::eof();
}
void real_reference(char const* canonical_path,char const* reference_path){
    m::CanonicalGeometry geometry{},decoded{};static std::array<m::ReferenceRecord68,115> reference{};
    check(load(canonical_path,geometry));check(load(reference_path,reference));
    check(m::canonical_geometry(reference.data(),reference.size(),decoded));check(geometry==decoded);
    const auto dimensions=measure(geometry);
    // Compare against actual existing Native pure-buffer FK, not another IK.
    static ik::Atom atoms[115]{};static std::int32_t parents[115]{};static ik::Workspace workspace{};
    static ik::Matrix space[115]{},world[115]{};ik::Matrix identity{};
    identity.m[0]=identity.m[5]=identity.m[10]=identity.m[15]=1;
    for(std::size_t i=0;i<115;++i){
        const auto* at=geometry.data()+i*36;std::memcpy(&parents[i],at+4,4);
        std::memcpy(atoms[i].q,at+8,16);std::memcpy(atoms[i].translation,at+24,12);atoms[i].scale=1;
    }
    check(ik::forward(atoms,parents,115,identity,workspace,space,world)==ik::Status::okay);
    const auto distance=[&](std::size_t a,std::size_t b){return r::detail::distance(ik::detail::position(space[a]),ik::detail::position(space[b]))/100;};
    const double native[]={distance(8,32),distance(8,10),distance(10,11),distance(32,34),distance(34,35)};
    const double actual[]={dimensions.shoulder_span_metres,dimensions.upper_metres[0],dimensions.lower_metres[0],dimensions.upper_metres[1],dimensions.lower_metres[1]};
    double maximum{};for(unsigned i=0;i<5;++i){maximum=(std::max)(maximum,std::abs(native[i]-actual[i]));check(near(native[i],actual[i],1e-6));}
    p::Profile profile{};check(p::derive({1,1.7,-.15},dimensions,profile)==p::Status::okay);
    std::printf("RO3 reference data only: shoulder %.17g m, upper [%.17g, %.17g] m, lower [%.17g, %.17g] m, mount H %.17g m, estimate s %.17g, Native FK max residual %.17g m\n",
        dimensions.shoulder_span_metres,dimensions.upper_metres[0],dimensions.upper_metres[1],dimensions.lower_metres[0],dimensions.lower_metres[1],
        dimensions.authored_controller_extension_metres[0]+dimensions.authored_controller_extension_metres[1],profile.scale,maximum);
}
} // namespace
int main(int argc,char* argv[]){
    static_assert(std::is_standard_layout<p::Measurement>::value&&std::is_trivially_copyable<p::Profile>::value);
    if(argc!=1&&argc!=3){std::fprintf(stderr,"usage: test [canonical4140.bin reference7820.bin]\n");return 2;}
    scaling_and_profile();rejected_profiles();rejected_geometry();
    if(argc==3)real_reference(argv[1],argv[2]);
    std::printf("hand personal model estimate: %u checks passed; synthetic geometry is a host seam, no physical calibration or source/write permission\n",checks);return 0;
}
