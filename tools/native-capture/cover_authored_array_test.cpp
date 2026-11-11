#include "cover_authored_array.h"
#include <cstdio>
#include <memory>
namespace a=gears_cover_authored_array;
int main(){unsigned checks=0,failures=0;
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
 alignas(8) std::array<unsigned char,160> bytes{};a::s::pose::ArrayHeader header{reinterpret_cast<uintptr_t>(bytes.data()),2,2};
 a::Source source{};source.header=reinterpret_cast<uintptr_t>(&header);source.bone={10,0};source.reflected_extent=72;source.collision_mask=1;source.receiver_scale={1,1,1};
 for(unsigned i=0;i<2;++i){float tm[16]{};tm[0]=tm[5]=tm[10]=tm[15]=1;float radius=2;unsigned disabled=i;
 memcpy(bytes.data()+80*i,tm,64);memcpy(bytes.data()+80*i+64,&radius,4);memcpy(bytes.data()+80*i+68,&disabled,4);}
 unsigned payload_reads=0;bool mutate=false;
 auto read=[&](void* out,void const* at,size_t count){const auto address=reinterpret_cast<uintptr_t>(at);
 if(at==&header&&count==16){memcpy(out,&header,16);return true;}
 if(address<reinterpret_cast<uintptr_t>(bytes.data())||address+count>reinterpret_cast<uintptr_t>(bytes.data()+bytes.size()))return false;
 if(mutate&&++payload_reads==3){float radius=3;memcpy(bytes.data()+64,&radius,4);}memcpy(out,at,count);return true;};
 auto work=std::make_unique<a::Workspace>();auto out=std::make_unique<a::Snapshot>();const auto expected=source;
 auto guard=[&](a::Source const& observed){return a::same(observed,expected);};
 C(a::capture(source,read,guard,*work,*out)==a::Code::captured&&out->count==2);
 C(out->rows[0].shape.authored.raw_dimensions[0]==2&&out->rows[1].shape.array_index==1);
 C(!out->rows[0].collision_disabled&&out->rows[1].collision_disabled);
 const auto saved=std::make_unique<a::Snapshot>(*out);mutate=true;
 C(a::capture(source,read,guard,*work,*out)==a::Code::changed&&!memcmp(saved.get(),out.get(),sizeof *out));mutate=false;
 C(a::capture(source,read,[](a::Source const&){return false;},*work,*out)==a::Code::guard&&!memcmp(saved.get(),out.get(),sizeof *out));
 header.count=65;C(a::capture(source,read,guard,*work,*out)==a::Code::capacity&&!memcmp(saved.get(),out.get(),sizeof *out));
 header.count=0;header.capacity=0;header.data=0;C(a::capture(source,read,guard,*work,*out)==a::Code::captured&&!out->count);

 source.receiver_scale={1,2,1};C(a::capture(source,read,guard,*work,*out)==a::Code::request&&!out->count);source.receiver_scale={1,1,1};
 header={reinterpret_cast<uintptr_t>(bytes.data()),2,2};
 for(unsigned kind=0;kind<3;++kind){source.kind=static_cast<a::g::metrics::Kind>(kind);source.reflected_extent=kind==0?72:kind==1?80:76;
  const unsigned dimensions=kind==0?1:kind==1?3:2;
  for(unsigned i=0;i<2;++i){memset(bytes.data()+80*i+64,0,16);const float values[]{2,3,4};memcpy(bytes.data()+80*i+64,values,dimensions*4);unsigned word=i?1:0;memcpy(bytes.data()+80*i+64+dimensions*4,&word,4);}
  const auto typed=source;auto current=[&](a::Source const& observed){return a::same(observed,typed);};
  C(a::capture(source,read,current,*work,*out)==a::Code::captured&&out->rows[0].shape.authored.kind==source.kind&&out->rows[1].collision_disabled);
  C(out->rows[0].shape.authored.raw_dimensions[0]==2&&out->rows[0].shape.authored.raw_dimensions[dimensions-1]==float(dimensions+1));
  const auto previous=std::make_unique<a::Snapshot>(*out);
  for(unsigned stop=1;stop<=3;++stop){unsigned calls=0;auto stopping=[&](a::Source const& value){return ++calls!=stop&&a::same(value,typed);};
   C(a::capture(source,read,stopping,*work,*out)==a::Code::guard&&!memcmp(previous.get(),out.get(),sizeof *out));}
 }
 printf("AuthoredArray checks=%u failures=%u synthetic memory/guard only; no native identity/active proof\n",checks,failures);return failures?1:0;
}
