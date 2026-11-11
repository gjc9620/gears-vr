#include "cover_physx_pose_math.h"
#define GEARS_AUTHORED_INVENTORY_TEST_MAIN PriorCacheMatchInventoryMain
#include "cover_authored_inventory_live_test.cpp"
#undef GEARS_AUTHORED_INVENTORY_TEST_MAIN
#include "cover_authored_cache_match_live.inl"
static float from_bits(unsigned value){float out{};std::memcpy(&out,&value,4);return out;}
// Independent constant fixture: float32 values from the recorded original
// builder operations for raw radius2, box2/3/4, length3 and cache scale6.00005.
// No call to creation::convert builds these expected descriptor values.
static void cache_fixture(cover_authored_inventory_live::Snapshot const& inventory,unsigned body,
                          gears_cover_physx_cached_shapes::Snapshot& output){
 namespace cached=gears_cover_physx_cached_shapes;output={};output.source={0x140000000,0x2000000};const auto setup=inventory.membership.rows[body].setup.address;
 output.cache_source={setup,setup+0xcc,setup+0xdc};output.requested_scale={6,6,6};output.selected.row.descriptor=arena+0x90000;
 output.selected.row.scale={from_bits(0x40c00069),from_bits(0x40c00069),from_bits(0x40c00069)};
 const unsigned tags0[]={1,2,2,3},tags1[]={2,3};const auto* tags=body?tags1:tags0;output.count=body?2:4;
 output.vector={arena+0x91000,arena+0x91000+8*output.count,arena+0x91000+8*output.count};
 for(unsigned n=0;n<output.count;++n){auto& row=output.rows[n];row.tag=tags[n];row.flags=8;row.address=arena+0x92000+0x100*n;
  row.table=0x140000000+(row.tag==1?0x1347dc0:row.tag==2?0x13486f0:0x1347dd8);
  row.local.values[0]=row.local.values[4]=row.local.values[8]=1;
  if(row.tag==3){row.local.values[0]=-1;row.local.values[3]=row.local.values[6]=from_bits(0x80000000);row.local.values[4]=row.local.values[8]=0;row.local.values[5]=row.local.values[7]=1;}
  if(!body&&n==2)row.local.values[9]=from_bits(0x3df5c315);
  if(row.tag==2)row.half={from_bits(0x3e147b24),from_bits(0x3e51ebea),from_bits(0x3e87ae57)};
  else{row.radius=from_bits(0x3e87ae57);if(row.tag==3)row.full_segment_length=from_bits(0x3eb85250);}
  memcpy(row.bytes.data(),&row.table,8);memcpy(row.bytes.data()+8,&row.tag,4);memcpy(row.bytes.data()+12,&row.local,sizeof row.local);memcpy(row.bytes.data()+60,&row.flags,4);
  if(row.tag==2)memcpy(row.bytes.data()+128,row.half.data(),12);else{memcpy(row.bytes.data()+128,&row.radius,4);if(row.tag==3)memcpy(row.bytes.data()+132,&row.full_segment_length,4);}
 }
}
__declspec(noinline) static int match_cases(){
 namespace match=cover_authored_cache_match_live;namespace inv=cover_authored_inventory_live;namespace cached=gears_cover_physx_cached_shapes;
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 inventory_fixture();roles::Snapshot typed{};C(observe(typed));auto read_work=std::make_unique<inv::Workspace>();auto inventory=std::make_unique<inv::Snapshot>();
 auto owner=[&](roles::Snapshot const& expected){roles::Snapshot fresh{};return roles::sample(actor,physics_expected,guard,workspace,fresh)==roles::Code::current&&roles::same(fresh,expected);};
 C(inv::capture(typed,safe_copy,owner,*read_work,*inventory)==inv::Code::captured);
 auto cache=std::make_unique<cached::Snapshot>();match::Result output{},sentinel{};sentinel.count=123;
 for(unsigned body=0;body<2;++body){cache_fixture(*inventory,body,*cache);
  C(match::match(*inventory,body,*cache,output)==match::Code::matched&&output.count==(body?2u:4u)&&output.authored_count==(body?3u:4u));
  C(h::same(output.setup,inventory->membership.rows[body].setup)&&output.body_index==body);
  for(unsigned n=0;n<output.count;++n)C(output.rows[n].authored_index==(body?4u+n:n)&&output.rows[n].cache_index==n);
  if(body)C(output.disabled[2]);
 }
 cache_fixture(*inventory,0,*cache);auto saved_cache=std::make_unique<cached::Snapshot>(*cache);auto saved_inventory=std::make_unique<inv::Snapshot>(*inventory);
 for(unsigned bad=0;bad<13;++bad){output=sentinel;
  if(bad==0)cache->cache_source.setup+=8;if(bad==1)cache->requested_scale={1,1,1};if(bad==2)cache->selected.row.scale={6.1f,6.1f,6.1f};
  if(bad==3)cache->rows[0].radius=99;if(bad==4)cache->rows[0].bytes[128]^=1;
  if(bad==5)cache->rows[2].local.values[9]=0;if(bad==6)--cache->count;
  if(bad==7)std::swap(cache->rows[0],cache->rows[1]);if(bad==8)inventory->rows[0].raw[64]^=1;
  if(bad==9)inventory->inventory[0].count[3]=1;if(bad==10)inventory->collision_masks[0][1]=2;
  if(bad==11)++inventory->rows[0].shape.authored.bone.number;if(bad==12)inventory->scaling.value={1,1,1};
  C(match::match(*inventory,0,*cache,output)!=match::Code::matched&&output.count==123);*cache=*saved_cache;*inventory=*saved_inventory;
 }
 // Recomputing descriptors with CURRENT scale6 instead of selected6.00005
 // must fail, even though both cache keys are within the original tolerance.
 cache->selected.row.scale={6,6,6};output=sentinel;C(match::match(*inventory,0,*cache,output)==match::Code::mismatch&&output.count==123);
 cache_fixture(*inventory,1,*cache);inventory->rows[6].collision_word=0;inventory->rows[6].collision_disabled=false;memset(inventory->rows[6].raw.data()+72,0,4);
 C(match::match(*inventory,1,*cache,output)==match::Code::mismatch&&output.count==123);*inventory=*saved_inventory;
 // Every authored primitive remains represented as disabled when the original
 // creation rules produce an empty cached shape vector.
 cache_fixture(*inventory,0,*cache);cache->count=0;cache->vector={};
 for(unsigned n=0;n<4;++n){auto& row=inventory->rows[n];row.collision_word=1;row.collision_disabled=true;
  const unsigned kind=unsigned(row.shape.authored.kind),offset=kind==0?68:kind==1?76:72;const unsigned word=1;memcpy(row.raw.data()+offset,&word,4);}
 C(match::match(*inventory,0,*cache,output)==match::Code::matched&&output.count==0&&output.authored_count==4&&output.disabled[0]&&output.disabled[3]);
 *inventory=*saved_inventory;
 C(match::match(*inventory,0,*saved_cache,*reinterpret_cast<match::Result*>(inventory.get()))==match::Code::alias);
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("AuthoredCacheMatch checks=%u failures=%u owned source-to-template geometry only; no active identity\n",checks,failures);return failures?1:0;
}
int main(){if(PriorCacheMatchInventoryMain())return 1;return match_cases();}
