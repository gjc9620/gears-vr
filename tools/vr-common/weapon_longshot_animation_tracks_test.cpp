#include "weapon_longshot_animation_tracks.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>
namespace tracks=gears_weapon::longshot_tracks;
namespace {
struct Fixture {
    std::array<std::int32_t,16> offsets{};std::vector<unsigned char> bytes;
    unsigned checks{},format{1};
    template<class T>void append(T const& value){const auto start=bytes.size();bytes.resize(start+sizeof value);std::memcpy(bytes.data()+start,&value,sizeof value);}
    Fixture(unsigned rotation_format=1):format(rotation_format){
        for(unsigned bone=0;bone<4;++bone){
            offsets[bone*4]=int(bytes.size());offsets[bone*4+1]=bone==3?3:1;
            for(int i=0;i<offsets[bone*4+1];++i)append(std::array<float,3>{{float(i==1),0,0}});
            offsets[bone*4+2]=int(bytes.size());offsets[bone*4+3]=bone==3?3:1;
            if(bone==3)append(std::array<float,6>{{0,0,0,0,0,.5f}});
            for(int i=0;i<offsets[bone*4+3];++i){
                if(bone==3&&format==3)append(std::uint32_t((1023u<<21)|(1023u<<10)|(i==1?1022u:511u)));
                else append(std::array<float,3>{{0,0,i==1?.5f:0}});
            }
        }
    }
    bool decode(tracks::Samples& out){return tracks::decode(offsets,bytes.data(),bytes.size(),format==3?6u:3u,1.f,out,format);}
};
}
int main(){
    Fixture f;tracks::Samples samples{};assert(f.decode(samples));
    assert(samples.count==3&&samples.length==1.f);
    assert(samples.bones[3][1].position.x==1);
    assert(std::abs(samples.bones[3][1].orientation.z-.5)<1e-8);
    assert(std::abs(samples.bones[3][1].orientation.w-std::sqrt(.75))<1e-8);
    gears_vr::Pose pose{};assert(tracks::sample(samples,3,1./3,pose)&&pose.position.x==1);
    assert(tracks::sample(samples,3,1./6,pose)&&std::abs(pose.position.x-.5)<1e-8);
    assert(tracks::sample(samples,3,1,pose)&&pose.position.x==0);
    auto opposite=samples;opposite.bones[3][1].orientation={0,0,0,-1};
    assert(tracks::sample(opposite,3,1./6,pose)&&std::abs(pose.orientation.w-1)<1e-10);
    assert(!tracks::sample(samples,4,0,pose));assert(!tracks::sample(samples,3,-.01,pose));
    assert(!tracks::sample(samples,3,1.01,pose));
    assert(!tracks::decode(f.offsets,f.bytes.data(),f.bytes.size(),257,1.f,samples));
    assert(!tracks::decode(f.offsets,f.bytes.data(),f.bytes.size(),3,0,samples));
    auto bad=f;bad.offsets[13]=2;assert(!bad.decode(samples));
    bad=f;bad.offsets[14]=-1;assert(!bad.decode(samples));
    bad=f;bad.offsets[14]=bad.offsets[12];assert(!bad.decode(samples));
    bad=f;bad.bytes.push_back(0);assert(!bad.decode(samples));
    bad=f;float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(bad.bytes.data(),&nan,4);assert(!bad.decode(samples));
    bad=f;const float outside=.75f;std::memcpy(bad.bytes.data()+bad.offsets[14]+24+12+8,&outside,4);assert(!bad.decode(samples));
    bad=f;const std::array<float,3> invalid{{1,1,1}};std::memcpy(bad.bytes.data()+bad.offsets[2],invalid.data(),12);assert(!bad.decode(samples));
    // A malformed stream must never publish a partly decoded sample array.
    samples.count=99;assert(!bad.decode(samples)&&samples.count==99);
    // Normal LS_Reload uses interval compression and fewer compressed keys
    // than NumFrames. This synthetic stream is independent of game assets.
    Fixture interval(3);assert(interval.decode(samples)&&samples.count==3);
    assert(std::abs(samples.bones[3][1].orientation.z-.5)<1e-8);
    assert(std::abs(samples.bones[3][1].orientation.w-std::sqrt(.75))<1e-8);
    assert(samples.bones[3][0].orientation.z==0&&samples.bones[3][2].orientation.z==0);
    auto invalid_interval=interval;invalid_interval.offsets[13]=2;samples.count=99;
    assert(!invalid_interval.decode(samples)&&samples.count==99);
    invalid_interval=interval;invalid_interval.format=2;assert(!invalid_interval.decode(samples)&&samples.count==99);
    invalid_interval=interval;invalid_interval.bytes.push_back(0);assert(!invalid_interval.decode(samples)&&samples.count==99);
    invalid_interval=interval;const float negative=-.5f;
    std::memcpy(invalid_interval.bytes.data()+invalid_interval.offsets[14]+12,&negative,4);
    assert(!invalid_interval.decode(samples)&&samples.count==99);
    std::puts("Longshot four-bone Float96 tracks: compound samples, timing, malformed spans and publication checks passed");
}
