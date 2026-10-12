#include <windows.h>
#include <cstdio>
#include <limits>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "source_eye_args.h"
namespace {
std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields;std::istringstream stream(line);std::string field;
    while(std::getline(stream,field,','))fields.push_back(field);
    return fields;
}
int replay(const char* path) {
    std::ifstream stream(path);std::string line;
    if(!stream || !std::getline(stream,line))return 10;
    auto header=split(line);
    auto column=[&](const std::string& name)->size_t {
        auto found=std::find(header.begin(),header.end(),name);
        return found==header.end()?header.size():size_t(found-header.begin());
    };
    size_t kind=column("kind"),valid=column("valid"),indices[16]{};
    if(kind==header.size() || valid==header.size())return 11;
    for(unsigned i=0;i<16;++i){indices[i]=column("matrix"+std::to_string(i));if(indices[i]==header.size())return 11;}
    unsigned samples=0;double max_error=0;
    while(std::getline(stream,line)) {
        auto fields=split(line);if(fields.size()!=header.size())return 12;
        if(fields[kind]!="1" || (std::stoul(fields[valid])&5)!=5)continue;
        float m[16];for(unsigned i=0;i<16;++i)m[i]=std::stof(fields[indices[i]]);
        gears_source_args::Snapshot source{};memcpy(source.bytes.data()+0x70,m,sizeof(m));
        for(float units:{-1.f,1.f}) {
            auto eye=source;gears_eye_args::Result result{};
            if(!gears_eye_args::shift_right(eye,units,result))return 13;
            float changed[16];memcpy(changed,eye.bytes.data()+0x70,sizeof(changed));
            for(unsigned k=0;k<3;++k) {
                double transformed=changed[12+k],baseline=m[12+k];
                for(unsigned i=0;i<3;++i){transformed+=double(result.after[i])*changed[4*i+k];baseline+=double(result.before[i])*m[4*i+k];}
                max_error=(std::max)(max_error,std::fabs(transformed-baseline));
            }
            for(size_t i=0;i<source.bytes.size();++i)
                if((i<0xa0 || i>=0xa4) && eye.bytes[i]!=source.bytes[i])return 14;
        }
        ++samples;
    }
    printf("Offline recorded eye inputs: samples=%u max_origin_residual_error=%.9g game_units\n",samples,max_error);
    return samples>=100 && max_error<.01?0:15;
}
}
int main(int argc,char** argv) {
    if(argc>2)return 16;
    if(argc==2){try{return replay(argv[1]);}catch(...){return 17;}}
    for(unsigned index=0;index<24;++index) {
        float angle=float(index)*.25f,c=std::cos(angle),s=std::sin(angle);
        float m[16]={c,s,0,0,-s,c,0,0,0,0,1,0,0,0,0,1};
        float position[3]={9719.f,-35049.f,-632.85f};
        for(unsigned k=0;k<3;++k)for(unsigned i=0;i<3;++i)m[12+k]-=position[i]*m[4*i+k];
        gears_source_args::Snapshot original{};original.bytes.fill(0x5a);
        memcpy(original.bytes.data()+0x70,m,sizeof(m));
        for(float units:{-2.f,-1.f,0.f,1.f,2.f}) {
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
        auto nonfinite=original;invalid=std::numeric_limits<float>::quiet_NaN();
        memcpy(nonfinite.bytes.data()+0xa8,&invalid,4);before=nonfinite.bytes;
        if(gears_eye_args::shift_right(nonfinite,1,result) || nonfinite.bytes!=before)return 6;
        auto nonaffine=original;invalid=.5f;memcpy(nonaffine.bytes.data()+0xac,&invalid,4);before=nonaffine.bytes;
        if(gears_eye_args::shift_right(nonaffine,1,result) || nonaffine.bytes!=before)return 7;
        auto enormous=original;invalid=1e30f;memcpy(enormous.bytes.data()+0xa0,&invalid,4);before=enormous.bytes;
        if(gears_eye_args::shift_right(enormous,1,result) || enormous.bytes!=before)return 8;
    }
    printf("Offline eye inputs: 24 rotations, signed offsets, immutable non-view fields and invalid-input guards passed\n");return 0;
}
