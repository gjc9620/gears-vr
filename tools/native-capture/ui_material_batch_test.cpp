#include "ui_material_batch.h"
#include <cassert>
#include <map>
#include <vector>
#include <cstdio>
using namespace gears_ui_material;
struct Memory {
 std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
 void block(std::uintptr_t a,std::size_t n){blocks[a].resize(n);}
 template<class T> void put(std::uintptr_t a,T const& v){for(auto& b:blocks)if(a>=b.first&&a-b.first<=b.second.size()&&sizeof(v)<=b.second.size()-(a-b.first)){std::memcpy(b.second.data()+a-b.first,&v,sizeof(v));return;}assert(false);}
 bool read(void* out,void const* p,std::size_t n){auto a=reinterpret_cast<std::uintptr_t>(p);for(auto const& b:blocks)if(a>=b.first&&a-b.first<=b.second.size()&&n<=b.second.size()-(a-b.first)){std::memcpy(out,b.second.data()+a-b.first,n);return true;}return false;}
};
struct Fixture {
 Config c{0x10000000};Memory m;Before before{};Receipt receipt{};Consumer consumer{};
 Fixture(){
  for(auto p:{0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000,0x8000,0x9000,0xa000,0xb000,0xc000})m.block(p,0x200);
  m.block(c.base+0x15a420,32);m.put(c.base+0x15a420,execute_bytes);m.block(c.base+0x159fcc,32);m.put(c.base+0x159fcc,factory_bytes);
  m.block(c.base+0x136c5f0,24);m.put(c.base+0x136c5f0,std::array<std::uintptr_t,3>{c.base+0x3a514c,c.base+0x1ea9e0,c.base+0x1a44e0});
  m.put(0x109c,std::uintptr_t(0x2000));m.put(0x2028,d::Array{0x3000,1,1});m.put(0x3000,std::int32_t(7));m.put(0x2048,d::Array{0x4000,1,1});m.put(0x4000,std::int32_t(7));m.put(0x4004,d::Array{});
  assert(snapshot_before(c,read(),0x1000,before));
  m.put(0x4004,d::Array{0x5000,1,1});m.put(0x5000,std::uintptr_t(0x6000));m.put(0x6000,c.base+0x136c5f0);m.put(0x6008,std::uintptr_t(0x7000));m.put(0x7000,std::uintptr_t(0xdead));m.put(0x7060,d::Array{0x8000,1,1});
  m.put(0x8000,std::array<float,8>{10,20,30,40,0,0,1,1});m.put(0x8020,unsigned(17));
  assert(snapshot_after(c,read(),before,3,receipt));
  m.put(0x9000,c.base+0x136c5d8);m.put(0x9008,std::uintptr_t(0xa000));m.put(0x9010,std::uintptr_t(0x7000));m.put(0x9020,unsigned(8));m.put(0xa000,std::uintptr_t(0xb000));m.put(0xb010,std::uintptr_t(0xc000));
  assert(begin_consumer(c,read(),receipt,0x9000,consumer));
  auto mesh=c.base+0x1ad6980;m.block(mesh,0xec);m.put(mesh+0x10,std::uintptr_t(0xc000));m.put(mesh+0x18,unsigned(32));m.put(mesh+0x28,std::uintptr_t(0xdead));m.put(mesh+0xd4,unsigned(2));m.put(mesh+0xdc,unsigned(3));m.put(mesh+0xe8,std::uint16_t(0x411));
  const float p[4][4]={{40,20,1,0},{10,20,0,0},{40,60,1,1},{10,60,0,1}};
  for(unsigned i=0;i<4;++i){m.put(0xc000+i*32,std::array<float,8>{p[i][0],p[i][1],0,0,0,0,p[i][2],p[i][3]});}
 }
 auto read(){return [this](void* out,void const* p,std::size_t n){return m.read(out,p,n);};}
};
int main(){Fixture f;Quad q{};assert(capture_quad(f.c,f.read(),f.consumer,q));Candidate candidate{};
 assert(classify_draw(f.c,f.consumer,q,f.c.base+0x1af345,{false,4,1,0,0,0},candidate));
 assert(!classify_draw(f.c,f.consumer,q,f.c.base+0x177c4a,{false,4,1,0,0,0},candidate));
 assert(!classify_draw(f.c,f.consumer,q,f.c.base+0x1af345,{false,6,1,0,0,0},candidate));
 Receipt sentinel{};sentinel.payload=123;assert(!snapshot_after(f.c,f.read(),f.before,4,sentinel)&&sentinel.payload==123);
 f.m.put(0x9018,unsigned(1));Consumer out{};out.command=123;assert(!begin_consumer(f.c,f.read(),f.receipt,0x9000,out)&&out.command==123);f.m.put(0x9018,unsigned(0));
 f.m.put(0x9020,unsigned(0));assert(!begin_consumer(f.c,f.read(),f.receipt,0x9000,out));f.m.put(0x9020,unsigned(8));
 f.m.put(0x8000,float(11));assert(!begin_consumer(f.c,f.read(),f.receipt,0x9000,out));f.m.put(0x8000,float(10));
 f.m.put(f.c.base+0x1ad6980+0x18,unsigned(48));Quad rejected{};rejected.source=123;assert(!capture_quad(f.c,f.read(),f.consumer,rejected)&&rejected.source==123);
 f.m.blocks.erase(0x7000);f.m.blocks.erase(0xa000);assert(finish_consumer(f.consumer,36));assert(!finish_consumer(f.consumer,35));
 Fixture count;count.m.put(0x7060,d::Array{0x8000,2,2});assert(!begin_consumer(count.c,count.read(),count.receipt,0x9000,out));
 Fixture table;table.m.put(table.c.base+0x136c5f0,std::uintptr_t(1));assert(!validate_binding(table.c,table.read()));
 Fixture realloc;realloc.m.block(0xd000,8);realloc.m.put(0xd000,std::uintptr_t(0x6000));realloc.m.blocks.erase(0x5000);realloc.m.put(0x4004,d::Array{0xd000,1,1});assert(snapshot_after(realloc.c,realloc.read(),realloc.before,2,sentinel));
 Fixture capacity;capacity.m.put(0x7060,d::Array{0x8000,1,8});assert(begin_consumer(capacity.c,capacity.read(),capacity.receipt,0x9000,out));capacity.m.put(0x7060,d::Array{0x8000,1,129});assert(!begin_consumer(capacity.c,capacity.read(),capacity.receipt,0x9000,out));
 Fixture patched;patched.m.put(patched.c.base+0x15a420,unsigned(0));assert(!validate_binding(patched.c,patched.read()));assert(begin_consumer(patched.c,patched.read(),patched.receipt,0x9000,out));assert(snapshot_after(patched.c,patched.read(),patched.before,1,sentinel));
 Fixture old;old.m.put(0x4004,d::Array{0x5000,1,2});old.m.put(0x5000,std::uintptr_t(0xd000));Before saved{};assert(snapshot_before(old.c,old.read(),0x1000,saved));old.m.put(0x5008,std::uintptr_t(0x6000));old.m.put(0x4004,d::Array{0x5000,2,2});assert(snapshot_after(old.c,old.read(),saved,3,sentinel));old.m.put(0x5000,std::uintptr_t(0xe000));assert(!snapshot_after(old.c,old.read(),saved,3,sentinel));
 Fixture indexed;indexed.m.put(indexed.c.base+0x1ad6980+0x1c,std::uintptr_t(0x1234));Quad iq{};assert(capture_quad(indexed.c,indexed.read(),indexed.consumer,iq));assert(classify_draw(indexed.c,indexed.consumer,iq,indexed.c.base+0x177c4a,{true,4,1,0,0,0},candidate)&&!candidate.mutation_allowed);
 std::puts("ui_material_batch tests passed");
}

