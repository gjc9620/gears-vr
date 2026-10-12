#include <windows.h>
#include <cstdio>
#include <limits>
#include "source_eye_args.h"
int main() {
    for(unsigned index=0;index<24;++index) {
        float angle=float(index)*.25f,c=std::cos(angle),s=std::sin(angle);
        float m[16]={c,s,0,0,-s,c,0,0,0,0,1,0,0,0,0,1};
        float position[3]={9719.f,-35049.f,-632.85f};
        for(unsigned k=0;k<3;++k)for(unsigned i=0;i<3;++i)m[12+k]-=position[i]*m[4*i+k];
        gears_source_args::Snapshot original{};original.bytes.fill(0x5a);
        memcpy(original.bytes.data()+0x70,m,sizeof(m));
        for(float units:{-1.f,0.f,1.f}) {
            auto eye=original;gears_eye_args::Result result{};
            if(!gears_eye_args::shift_right(eye,units,result))return 1;
            for(size_t i=0;i<eye.bytes.size();++i)
                if((i<0xa0 || i>=0xa4) && eye.bytes[i]!=original.bytes[i])return 2;
            float changed[16];memcpy(changed,eye.bytes.data()+0x70,sizeof(changed));
            for(unsigned k=0;k<3;++k) {
                double transformed=changed[12+k],baseline=m[12+k];
                for(unsigned i=0;i<3;++i) {
                    transformed+=double(result.after[i])*changed[4*i+k];
                    baseline+=double(result.before[i])*m[4*i+k];
                }
                // Account for float rounding at nonzero world coordinates.
                if(std::fabs(transformed-baseline)>.01)return 3;
            }
        }
        for(float invalid:{3.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
            auto eye=original;gears_eye_args::Result result{};
            if(gears_eye_args::shift_right(eye,invalid,result) || eye.bytes!=original.bytes)return 4;
        }
        auto scaled=original;float invalid=2;memcpy(scaled.bytes.data()+0x70,&invalid,4);
        auto before=scaled.bytes;gears_eye_args::Result result{};
        if(gears_eye_args::shift_right(scaled,1,result) || scaled.bytes!=before)return 5;
    }
    printf("Offline eye inputs: 24 rotations, signed offsets, immutable non-view fields and invalid-input guards passed\n");return 0;
}
