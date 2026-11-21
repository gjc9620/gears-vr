#include "weapon_boomshot_animation_tracks.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>
namespace tracks=gears_weapon::boomshot_tracks;
namespace barrel=gears_weapon::boomshot_barrel;
int main(int argc,char** argv){
    tracks::Samples values{};values.count=8;values.length=2;
    const double q[]={0,0,.3,1,1,.8,.5,0};
    for(unsigned i=0;i<8;++i)values.positions[2][i]={0,barrel::closed_raw.y-barrel::travel_raw*q[i],barrel::closed_raw.z};
    tracks::Path path{};assert(path.bind(values));
    assert(path.opening_begin()==1&&path.opening_end()==3&&path.closing_begin()==4&&path.closing_end()==7);
    float time{};assert(path.time(.3,barrel::Direction::opening,time)&&std::abs(time-.5f)<1e-6);
    assert(path.time(.5,barrel::Direction::closing,time)&&std::abs(time-1.5f)<1e-6);
    assert(!path.time(-.01,barrel::Direction::opening,time));
    assert(!path.time(std::numeric_limits<double>::quiet_NaN(),barrel::Direction::opening,time));
    auto bad=values;bad.positions[2][2].x=.1;assert(!path.bind(bad));
    bad=values;bad.positions[2][3]=bad.positions[2][2];assert(!path.bind(bad));
    bad=values;bad.positions[2][6]=bad.positions[2][4];assert(!path.bind(bad));
    bad=values;bad.positions[2][0]=bad.positions[2][4];assert(!path.bind(bad));
    bad=values;bad.positions[2][7].y=std::numeric_limits<double>::quiet_NaN();assert(!path.bind(bad));
    assert(argc==2);std::ifstream file(argv[1],std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    assert(bytes.size()==1092);
    std::array<std::int32_t,12> offsets{{0,1,12,1,24,1,36,1,48,86,1080,1}};
    tracks::Samples actual{};assert(tracks::decode(offsets,bytes.data(),bytes.size(),86,2.866666555404663f,actual));
    assert(path.bind(actual));assert(path.opening_begin()==66&&path.opening_end()==68&&path.closing_begin()==72&&path.closing_end()==75);
    for(auto direction:{barrel::Direction::opening,barrel::Direction::closing})for(unsigned i=0;i<=100;++i){
        double key{};assert(barrel::original_key(double(i)/100,direction,key));
        assert(path.time(double(i)/100,direction,time)&&std::abs(time-key*actual.length/actual.count)<1e-6);}
    auto offsets_bad=offsets;offsets_bad[8]=47;assert(!tracks::decode(offsets_bad,bytes.data(),bytes.size(),86,actual.length,bad));
    offsets_bad=offsets;offsets_bad[9]=85;assert(!tracks::decode(offsets_bad,bytes.data(),bytes.size(),86,actual.length,bad));
    offsets_bad=offsets;offsets_bad[10]=INT32_MAX;assert(!tracks::decode(offsets_bad,bytes.data(),bytes.size(),86,actual.length,bad));
    offsets_bad=offsets;offsets_bad[11]=86;assert(!tracks::decode(offsets_bad,bytes.data(),bytes.size(),86,actual.length,bad));
    assert(!tracks::decode(offsets,bytes.data(),bytes.size()-1,86,actual.length,bad));
    auto corrupt=bytes;float invalid=2;std::memcpy(corrupt.data()+1080,&invalid,4);
    assert(!tracks::decode(offsets,corrupt.data(),corrupt.size(),86,actual.length,bad));
    invalid=std::numeric_limits<float>::quiet_NaN();std::memcpy(corrupt.data()+48,&invalid,4);
    assert(!tracks::decode(offsets,corrupt.data(),corrupt.size(),86,actual.length,bad));
    assert(actual.count==86&&path.opening_begin()==66); // failed decodes/binds preserve prior output
    std::cout<<"Boomshot tracks: original1092-byte/86-key decode,202 inverse samples and malformed/foreign motion rejection passed; no live bone/timing claim\n";
}
