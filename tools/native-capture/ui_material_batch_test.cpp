#include "ui_material_batch.h"
#include <cassert>
#include <map>
#include <vector>
#include <cstdio>
#include <limits>
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
  m.block(c.base+0x15a3ca,5);m.put(c.base+0x15a3ca,mesh_flag_and_bytes);m.block(c.base+0x15a3d6,5);m.put(c.base+0x15a3d6,mesh_flag_or_bytes);
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
void view_plane_tests(){
 const auto configure=[](Fixture& f,float width=1920,float height=1080,float x=0,float y=0,float bias_x=0,float bias_y=0){
  std::array<float,16> identity{},p{};for(unsigned i=0;i<16;i+=5)identity[i]=1;
  p[0]=2/width;p[5]=-2/height;p[10]=0.5f;p[14]=0.25f;p[15]=1;
  p[12]=-1-x*p[0]+bias_x*2/width;p[13]=1-y*p[5]-bias_y*2/height;
  f.m.put(f.consumer.view+0x48,std::array<float,4>{x,y,width,height});f.m.put(f.consumer.view+0x70,identity);f.m.put(f.consumer.view+0xb0,p);
  std::memcpy(f.consumer.receipt.transform.data(),p.data(),64);
 };
 const auto put_projection=[](Fixture& f,unsigned element,float value){
  f.m.put(f.consumer.view+0xb0+element*4,value);std::memcpy(f.consumer.receipt.transform.data()+element*4,&value,4);
 };
 ViewPlane out{};ViewPlaneReject reason{};
 Fixture standard;configure(standard);assert(material_view_plane(standard.read(),standard.consumer,out,&reason));
 assert(out.source_width==1920&&out.source_height==1080&&std::abs(out.aspect-1920.0f/1080)<1e-6f&&reason==ViewPlaneReject::none);
 Fixture maximum;configure(maximum,16384,16384,16384,16384,1,-1);
 assert(material_view_plane(maximum.read(),maximum.consumer,out,&reason)&&out.aspect==1);
 for(float bias:{-1.0f,-0.5f,0.0f,0.5f,1.0f}){
  Fixture shifted;configure(shifted,1920,1080,3,4,bias,bias);
  assert(material_view_plane(shifted.read(),shifted.consumer,out,&reason));
 }
 // Z-only terms cannot change z=0 XY/W; preserve them without modification.
 Fixture z;configure(z);for(unsigned element:{2u,6u,8u,9u,10u,11u,14u})put_projection(z,element,0.37f);
 assert(material_view_plane(z.read(),z.consumer,out,&reason));
 // Every rejection leaves the caller's successful output untouched.
 const ViewPlane sentinel{7,8,9};
 for(unsigned misuse=0;misuse<16;++misuse){
  Fixture f;configure(f);out=sentinel;
  const float nan=std::numeric_limits<float>::quiet_NaN();
  if(misuse==0)f.consumer.view=0;
  if(misuse==1)f.m.blocks.erase(f.consumer.view);
  if(misuse==2)f.m.put(f.consumer.view+0x50,0.0f);
  if(misuse==3)f.m.put(f.consumer.view+0x54,-1.0f);
  if(misuse==4)f.m.put(f.consumer.view+0x50,16385.0f);
  if(misuse==5)f.m.put(f.consumer.view+0x48,nan);
  if(misuse==6)f.consumer.receipt.transform[0]^=1;
  if(misuse==7)put_projection(f,10,nan);
  if(misuse==8)f.m.put(f.consumer.view+0x70,2.0f);
  if(misuse==9)put_projection(f,1,0.001f); // XY rotation/shear.
  if(misuse==10)put_projection(f,4,0.001f);
  if(misuse==11)put_projection(f,3,0.001f); // XY-dependent W.
  if(misuse==12)put_projection(f,7,0.001f);
  if(misuse==13)put_projection(f,15,2.0f);
  if(misuse==14)put_projection(f,0,(2.0f+2.0f/1920)/1920); // One-pixel scale error is not translation bias.
  if(misuse==15)configure(f,1920,1080,0,0,1.01f,0);
  assert(!material_view_plane(f.read(),f.consumer,out,&reason));
  const auto expected=misuse<2?ViewPlaneReject::read:misuse<6?ViewPlaneReject::extent:
   misuse==6?ViewPlaneReject::copy:misuse==7?ViewPlaneReject::nonfinite:misuse==8?ViewPlaneReject::identity:
   misuse<14?ViewPlaneReject::xy_w:misuse==14?ViewPlaneReject::scale:ViewPlaneReject::offset;
  assert(reason==expected&&out.source_width==7&&out.source_height==8&&out.aspect==9);
 }
 Fixture changing;configure(changing);unsigned reads{};
 auto racing=[&](void* dst,void const* src,std::size_t n){
  if(++reads==4)changing.m.put(changing.consumer.view+0x50,1919.0f);
  return changing.m.read(dst,src,n);
 };
 out=sentinel;assert(!material_view_plane(racing,changing.consumer,out,&reason)&&reason==ViewPlaneReject::changed&&out.aspect==9);
 // Source and receipt matrices remain byte-identical after successful reads.
 std::array<float,16> saved{};assert(standard.m.read(saved.data(),reinterpret_cast<void const*>(standard.consumer.view+0xb0),64));
 assert(!std::memcmp(saved.data(),standard.consumer.receipt.transform.data(),64));
}
int main(){view_plane_tests();Fixture f;Quad q{};assert(capture_quad(f.c,f.read(),f.consumer,q));Candidate candidate{};
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
 Fixture live_flags;live_flags.m.put(live_flags.c.base+0x1ad6980+0xe8,std::uint16_t(0x2415));
 QuadDiagnostic diagnostic{};assert(capture_quad(live_flags.c,live_flags.read(),live_flags.consumer,iq,&diagnostic)&&diagnostic.step==0&&diagnostic.flags==0x2415);
 for(unsigned bit=0;bit<16;++bit){const auto value=std::uint16_t(0x2415^(1u<<bit));live_flags.m.put(live_flags.c.base+0x1ad6980+0xe8,value);
  const bool fixed=(mesh_fixed_flag_mask&(1u<<bit))!=0;assert(capture_quad(live_flags.c,live_flags.read(),live_flags.consumer,iq,&diagnostic)!=fixed);if(fixed)assert(diagnostic.step==2);}
 live_flags.m.put(live_flags.c.base+0x15a3ca,unsigned(0));assert(!validate_runtime_binding(live_flags.c,live_flags.read()));
 std::puts("ui_material_batch tests passed");
}
