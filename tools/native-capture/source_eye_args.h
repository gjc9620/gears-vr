#pragma once
#include <cmath>
#include "native_source_view_args.h"

// Offline input transformation only: no addresses, allocation or native calls.
// Synthetic game units, not calibrated IPD/metres or asymmetric eye projection.
namespace gears_eye_args {
struct Result {float before[3]{},after[3]{},world_delta[3]{};};
inline bool shift_right(gears_source_args::Snapshot& input,float units,Result& result) {
    if(!std::isfinite(units) || std::fabs(units)>2.f)return false;
    float m[16];memcpy(m,input.bytes.data()+0x70,sizeof(m));
    for(float value:m)if(!std::isfinite(value))return false;
    if(std::fabs(m[3])>1e-5f || std::fabs(m[7])>1e-5f ||
       std::fabs(m[11])>1e-5f || std::fabs(m[15]-1.f)>1e-5f)return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
        double dot=0;for(unsigned k=0;k<3;++k)dot+=double(m[4*i+k])*m[4*j+k];
        if(std::fabs(dot-double(i==j))>1e-5)return false;
    }
    Result computed{};
    float shifted=m[12]-units;
    if(!std::isfinite(shifted))return false;
    const float represented=m[12]-shifted;
    // Reject float quantization that would silently erase a requested eye gap.
    if(units!=0.f && (shifted==m[12] || std::fabs(represented-units)>std::fabs(units)*.01f))return false;
    for(unsigned i=0;i<3;++i) {
        double position=0;for(unsigned k=0;k<3;++k)position-=double(m[12+k])*m[4*i+k];
        computed.before[i]=float(position);
        computed.world_delta[i]=represented*m[4*i];
        computed.after[i]=computed.before[i]+computed.world_delta[i];
        if(!std::isfinite(computed.before[i]) || !std::isfinite(computed.after[i]))return false;
    }
    // View-space horizontal translation only; all projection/colour/flags and
    // borrowed resource values remain untouched. Native construction must later
    // regenerate derived matrices/geometry from this modified snapshot.
    memcpy(input.bytes.data()+0xa0,&shifted,4);result=computed;return true;
}
}
