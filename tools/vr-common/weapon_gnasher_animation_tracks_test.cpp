#include "weapon_gnasher_animation_tracks.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#v);std::exit(1);}}while(false)
namespace t=gears_weapon::gnasher_tracks;
int main(int argc,char** argv){
 CHECK(argc==2);FILE* file{};CHECK(!fopen_s(&file,argv[1],"rb")&&file);
 unsigned frames{},size{};float length{};std::array<std::int32_t,12> offsets{};
 CHECK(fread(&frames,4,1,file)==1&&fread(&length,4,1,file)==1&&fread(&size,4,1,file)==1);
 CHECK(fread(offsets.data(),4,12,file)==12&&size<65536);std::vector<unsigned char> bytes(size);CHECK(fread(bytes.data(),1,size,file)==size);fclose(file);
 t::Samples values{};CHECK(t::decode(offsets,bytes.data(),bytes.size(),frames,length,values,3));
 CHECK(values.count==18&&frames==35);unsigned peak{};CHECK(t::chamber_shape(values,peak));
 double time{};CHECK(!t::time_for_angle(values,.4,false,t::Timing::uncertified,time));
 for(auto timing:{t::Timing::keys_minus_one,t::Timing::keys}){
  double down{},back{};CHECK(t::time_for_angle(values,.4,false,timing,down));CHECK(t::time_for_angle(values,.4,true,timing,back));
  CHECK(down>0&&back>down&&back<length);CHECK(t::time_for_angle(values,0,true,timing,time)&&time==length);
 }
 CHECK(!t::time_for_angle(values,1,false,t::Timing::keys,time));
 CHECK(!t::time_for_angle(values,.4,false,static_cast<t::Timing>(99),time));
 auto bad=offsets;bad[3]=2;CHECK(!t::decode(bad,bytes.data(),bytes.size(),frames,length,values,3));
 bad=offsets;bad[6]=bad[2];CHECK(!t::decode(bad,bytes.data(),bytes.size(),frames,length,values,3));
 CHECK(!t::decode(offsets,bytes.data(),bytes.size()-1,frames,length,values,3));
 CHECK(!t::decode(offsets,bytes.data(),bytes.size(),frames,length,values,9));
 CHECK(!t::decode(offsets,bytes.data(),bytes.size(),2,length,values,3));
 std::printf("Gnasher original SG_Chamber decoder: 3 tracks, %u keys / %u frames; independent branches and malformed stream rejection passed; decoder checks do not certify runtime timing\n",values.count,frames);
}
