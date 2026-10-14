#include "view_origin.h"
#include <cstdio>
#include <cstring>
using namespace gears_vr;
#define CHECK(condition) do{if(!(condition)){std::printf("Failed line %d: %s\n",__LINE__,#condition);return __LINE__;}}while(0)
static StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
static Quaternion axis(unsigned a,double angle){Quaternion q{};q.w=std::cos(angle/2);if(a==0)q.x=std::sin(angle/2);if(a==1)q.y=std::sin(angle/2);if(a==2)q.z=std::sin(angle/2);return q;}
static StereoMatrix view(Quaternion q){auto v=identity();auto r=rotation_matrix(q);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)v[4*i+j]=static_cast<float>(r[i][j]);
    v[12]=123;v[13]=-456;v[14]=789;return v;}
// Independent general4x4 Gauss-Jordan inverse, with partial pivoting. This
// does not use the implementation's adjugate or rigid-transform shortcut.
static Vector inverse_origin(StereoMatrix const& view){
    double augmented[4][8]{};
    for(unsigned i=0;i<4;++i){for(unsigned j=0;j<4;++j)augmented[i][j]=view[4*i+j];augmented[i][4+i]=1;}
    for(unsigned col=0;col<4;++col){unsigned pivot=col;for(unsigned i=col+1;i<4;++i)if(std::abs(augmented[i][col])>std::abs(augmented[pivot][col]))pivot=i;
        for(unsigned j=0;j<8;++j)std::swap(augmented[col][j],augmented[pivot][j]);
        double divisor=augmented[col][col];for(double& value:augmented[col])value/=divisor;
        for(unsigned i=0;i<4;++i)if(i!=col){double factor=augmented[i][col];for(unsigned j=0;j<8;++j)augmented[i][j]-=factor*augmented[col][j];}}
    return {augmented[3][4],augmented[3][5],augmented[3][6]};
}
static Vector project(StereoMatrix const& v,Vector p){return {p.x*v[0]+p.y*v[4]+p.z*v[8]+v[12],p.x*v[1]+p.y*v[5]+p.z*v[9]+v[13],p.x*v[2]+p.y*v[6]+p.z*v[10]+v[14]};}
int main(){
    // Observed native world-axis permutation, independent numeric oracle.
    // This checks arithmetic only; no live Pawn/eye anchor is verified here.
    StereoMatrix observed_axes{0,0,1,0,1,0,0,0,0,1,0,0,35049,632.8499756f,-9719,1};
    auto observed_before=observed_axes;StereoMatrix anchored{};
    CHECK(view_with_origin(observed_axes,{9879,-35097,-648.8499756},anchored));
    CHECK(anchored[12]==35097.f&&anchored[13]==648.8499756f&&anchored[14]==-9879.f);
    for(unsigned i=0;i<16;++i)if(i<12||i==15)CHECK(std::memcmp(&anchored[i],&observed_axes[i],sizeof(float))==0);
    CHECK(observed_axes==observed_before);
    double maximum_error{};unsigned accepted{},rejected{};
    for(auto q:{Quaternion{},axis(0,.7),axis(1,-1.2),axis(2,.4),multiply(axis(1,.7),multiply(axis(0,-.3),axis(2,.2)))})
        for(Vector origin:{Vector{},Vector{1,2,3},Vector{-50,90,-70},Vector{35049,-632.849976,9719},Vector{-99999.123,88888.45,77777.001}}){
            auto input=view(q),before=input,output=identity();CHECK(view_with_origin(input,origin,output));++accepted;
            CHECK(input==before);
            for(unsigned i=0;i<16;++i)if(i<12||i==15)CHECK(std::memcmp(&output[i],&input[i],sizeof(float))==0);
            auto recovered=inverse_origin(output);double error=stereo_distance(recovered,origin);CHECK(error<=view_origin_max_error);maximum_error=(std::max)(maximum_error,error);
            // Desired origin maps to zero within the quantization bound.
            auto zero=project(output,origin);CHECK(std::hypot(std::hypot(zero.x,zero.y),zero.z)<=view_origin_max_error*1.0002);
            for(Vector offset:{Vector{2,5,7},Vector{-17,21,30}}){auto actual=project(output,{origin.x+offset.x,origin.y+offset.y,origin.z+offset.z});
                auto rotated=project(input,offset);rotated={rotated.x-input[12],rotated.y-input[13],rotated.z-input[14]};
                CHECK(stereo_distance(actual,rotated)<=view_origin_max_error*1.0002);}
            auto aliased=input;CHECK(view_with_origin(aliased,origin,aliased));CHECK(aliased==output);
        }
    for(unsigned failure=0;failure<9;++failure){auto input=identity();Vector origin{1,2,3};auto output=view(axis(2,.5)),sentinel=output;
        if(failure==0)origin.x=std::numeric_limits<double>::quiet_NaN();
        if(failure==1)origin.y=std::numeric_limits<double>::infinity();
        if(failure==2)input[5]=std::numeric_limits<float>::quiet_NaN();
        if(failure==3)input[12]=std::numeric_limits<float>::infinity();
        if(failure==4)input[0]=2;
        if(failure==5)input[0]=-1;
        if(failure==6)input[3]=.001f;
        if(failure==7)origin.x=(std::numeric_limits<double>::max)();
        if(failure==8)origin={10000000.25,0,0}; // float translation loses.25, above.02.
        CHECK(!view_with_origin(input,origin,output));CHECK(output==sentinel);++rejected;
    }
    auto output=identity();CHECK(view_with_origin(identity(),{.019,0,0},output));
    // Boundary is numerical, not a blanket "large coordinates forbidden":
    // an exactly representable large origin succeeds, fractional one rejects.
    CHECK(view_with_origin(identity(),{10000000,0,0},output));
    CHECK(stereo_distance(inverse_origin(output),{10000000,0,0})==0);
    CHECK(view_with_origin(identity(),{1000000.019,0,0},output));
    auto sentinel=output;
    CHECK(!view_with_origin(identity(),{1000000.021,0,0},output)&&output==sentinel);
    CHECK(!view_with_origin(identity(),{1000000.019,1000000.019,0},output)&&output==sentinel);
    auto alias_invalid=identity();alias_invalid[5]=2;auto before=alias_invalid;
    CHECK(!view_with_origin(alias_invalid,{1,2,3},alias_invalid)&&alias_invalid==before);
    std::printf("view-origin CPU observed-axis numeric oracle + inverse/point/alias/finite/quantization tests passed accepted=%u rejected=%u max_error=%.9g native_units bound=%.2f\n",accepted,rejected,maximum_error,view_origin_max_error);return 0;
}
