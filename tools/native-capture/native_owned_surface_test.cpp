#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "native_owned_surface.inl"
#include "owned_scene_retirement.h"
#include "owned_scene_trial.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <iterator>
using namespace gears_native_owned;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Mock {
 uintptr_t module=0x140000000,s=0x20013c,l=0x300000,r=0x400000,d=0x500000;
 unsigned creates{},clears{},width{},height{},releases{};DWORD tid=42;
 bool ready=true,code_ok=true,metadata_ok=true,chain_ok=true,create_ok=true,create_zero{},create_wrong_return{},
      desc_ok=true,device_ok=true,acquire_ok=true,clear_ok=true,gamma_ok=true,color_ok=true;
 uintptr_t release_fault{},tls_block=0x600000,context=0x700000;
 float current_gamma=2.2f;unsigned gamma_calls{},color_calls{};
 std::vector<unsigned> order;
 DWORD thread(){return tid;}
 bool tls(uintptr_t& b,uintptr_t& c){b=tls_block;c=context;return ready;}
 bool certify(uintptr_t& failed){failed=!code_ok?0x15abc8:metadata_ok?0:0x8a52a4;return code_ok&&metadata_ok;}
 bool read(void* out,uintptr_t address,size_t n){
  uint64_t value{};
  if(address==s-0x13c)value=module+0x133a180;
  else if(address==s-0x134)value=1;
  else if(address==s-0x130)value=chain_ok?l:0;
  else if(address==l)value=module+0x133cc28;
  else if(address==l+0xc)value=r;
  else if(address==r)value=module+0x133ecf8;
  else if(address==r+0xc)value=d;
  else return false;memcpy(out,&value,n);return true;
 }
 bool create(uintptr_t c,uint64_t& slot,unsigned w,unsigned h,uintptr_t& returned){CHECK(c==context);
  ++creates;width=w;height=h;order.push_back(1);slot=create_zero?0:s;
  returned=create_wrong_return?0:reinterpret_cast<uintptr_t>(&slot);return create_ok;}
 bool acquire(uintptr_t raw,uintptr_t& held){CHECK(raw==d);order.push_back(2);held=acquire_ok?d:0;return acquire_ok;}
 bool describe(uintptr_t raw,D3D12_RESOURCE_DESC& desc){CHECK(raw==d);order.push_back(3);desc={};
  desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;
  desc.DepthOrArraySize=1;desc.MipLevels=1;desc.Format=desc_ok?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_UNKNOWN;
  desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;return true;}
 bool device(uintptr_t raw,uintptr_t& dev){CHECK(raw==d);order.push_back(4);dev=device_ok?0x800000:0;return device_ok;}
 bool release_com(uintptr_t raw){++releases;order.push_back(raw==0x800000?5:6);return raw!=release_fault;}
 bool clear(uint64_t& slot){CHECK(slot==s);++clears;order.push_back(7);slot=0;return clear_ok;}
 bool gamma(float& value){++gamma_calls;if(gamma_ok)value=current_gamma;return gamma_ok;}
 bool color(uintptr_t self,void* out,uintptr_t& returned){++color_calls;CHECK(self);
  // Exercise the same nested virtual call as native 1161d8, not a cached value.
  const auto* table=*reinterpret_cast<const uintptr_t* const*>(self);
  using Gamma=float (__fastcall*)(void*);
  const float g=reinterpret_cast<Gamma>(table[4])(reinterpret_cast<void*>(self));
  float values[3]={g,1.f/g,2.2f/g};unsigned char bytes[16]{};memcpy(bytes,values,12);
  memcpy(out,bytes,color_ok?16:4);
  returned=reinterpret_cast<uintptr_t>(out);return color_ok;}
};
using TestOwner=Owner<Mock>;
static DrainProof drain(uint64_t generation=9){return {generation,true,true,true,true,true};}
static bool create(TestOwner& owner,Mock& ops,Quality quality=Quality::low){return owner.create_on_rhi(ops,quality,9);}
static void metadata_certificate(const char* blob_path){
 const unsigned char abc[]={'a','b','c'};
 constexpr char expected[]="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
 CHECK(gears_native_metadata::matches(abc,3,expected));
 const unsigned char bad[]={'a','b','d'};CHECK(!gears_native_metadata::matches(bad,3,expected));
 if(!blob_path)return;
 std::ifstream file(blob_path,std::ios::binary);CHECK(file.good());
 const std::vector<unsigned char> blob((std::istreambuf_iterator<char>(file)),{});
 constexpr uintptr_t module=0x140000000,imported=0x12340000;
 uintptr_t corrupt{},failed{};bool bad_import=false;
 auto read=[&](void* out,uintptr_t address,size_t n){
  if(address==module+0x131b4a0&&n==8){const uintptr_t value=imported+(bad_import?1:0);memcpy(out,&value,8);return true;}
  if(address<module+0x1000)return false;const auto offset=address-module-0x1000;
  if(offset>blob.size()||n>blob.size()-offset)return false;
  memcpy(out,blob.data()+offset,n);if(address==module+corrupt&&n)static_cast<unsigned char*>(out)[n-1]^=1;return true;
 };
 CHECK(gears_native_metadata::certify(read,module,imported,failed)&&!failed);
 for(auto const& span:gears_native_metadata::spans){corrupt=span.rva;
  CHECK(!gears_native_metadata::certify(read,module,imported,failed)&&failed==corrupt);}
 corrupt=0;bad_import=true;CHECK(!gears_native_metadata::certify(read,module,imported,failed)&&failed==0x131b4a0);
}
int main(int argc,char** argv){
 CHECK(argc<=2);metadata_certificate(argc==2?argv[1]:nullptr);
 static_assert(!std::is_copy_constructible<TestOwner>::value&&!std::is_move_constructible<TestOwner>::value,"stable owner");
 for(Quality quality:{Quality::low,Quality::high}){Mock ops;TestOwner owner;CHECK(create(owner,ops,quality));
  CHECK(ops.width==(quality==Quality::low?4096u:8192u)&&ops.height==(quality==Quality::low?2048u:4096u));
  CHECK(owner.retained()&&!ops.releases&&!ops.clears);auto* a=owner.borrow_adapter();CHECK(a);
  using Getter=const uint64_t* (__fastcall*)(TestOwner::Adapter*);using Dimension=uint32_t (__fastcall*)(TestOwner::Adapter*);
  using Gamma=float (__fastcall*)(TestOwner::Adapter*);using Color=void* (__fastcall*)(TestOwner::Adapter*,void*);
  const auto* ref=reinterpret_cast<Getter>(a->vtable[1])(a);CHECK(*ref==ops.s);CHECK(ref==reinterpret_cast<Getter>(a->vtable[1])(a));
  CHECK(reinterpret_cast<Dimension>(a->vtable[2])(a)==ops.width);CHECK(reinterpret_cast<Dimension>(a->vtable[3])(a)==ops.height);
  CHECK(reinterpret_cast<Gamma>(a->vtable[4])(a)==2.2f);unsigned char color[16];memset(color,0xff,16);
  CHECK(reinterpret_cast<Color>(a->vtable[5])(a,color)==color);CHECK(color[12]==0&&color[13]==255&&color[15]==255);
  float g{};memcpy(&g,color,4);CHECK(g==2.2f);
  ops.current_gamma=1.8f;CHECK(reinterpret_cast<Color>(a->vtable[5])(a,color)==color);memcpy(&g,color,4);
  CHECK(g==1.8f&&ops.color_calls==2&&ops.gamma_calls==3);
  CHECK(owner.borrow_resource()==reinterpret_cast<ID3D12Resource*>(ops.d));CHECK(!create(owner,ops));
  CHECK(owner.retire_on_rhi(drain()));CHECK(!owner.retained());CHECK(*ref==0);CHECK(!owner.borrow_adapter());
  CHECK(owner.retire_on_rhi(drain()));CHECK(ops.creates==1&&ops.clears==1&&ops.releases==2);
  CHECK(ops.order==std::vector<unsigned>({1,2,3,4,5,6,7}));}
 {Mock ops;TestOwner owner;ops.ready=false;CHECK(!create(owner,ops));CHECK(owner.snapshot().phase==Phase::empty);CHECK(!ops.creates);
  ops.ready=true;CHECK(create(owner,ops));CHECK(owner.retire_on_rhi(drain()));}
 for(unsigned invalid=0;invalid<2;++invalid){Mock ops;TestOwner owner;
  CHECK(!owner.create_on_rhi(ops,invalid==0?static_cast<Quality>(99):Quality::low,invalid==1?0:9));
  CHECK(!ops.creates&&!owner.retained());}
 for(unsigned missing=0;missing<6;++missing){Mock ops;TestOwner owner;CHECK(create(owner,ops));auto proof=drain();
  if(missing==0)proof.generation=10;if(missing==1)proof.new_submissions_disabled=false;
  if(missing==2)proof.renderers_retired=false;if(missing==3)proof.canvas_commands_retired=false;
  if(missing==4)proof.producer_cpu_access_retired=false;if(missing==5)proof.gpu_fence_completed=false;
  CHECK(!owner.retire_on_rhi(proof));CHECK(owner.retained()&&!ops.releases&&!ops.clears);
  CHECK(owner.retire_on_rhi(drain()));}
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));ops.tid=43;CHECK(!owner.retire_on_rhi(drain()));CHECK(!ops.releases);
  ops.tid=42;CHECK(owner.retire_on_rhi(drain()));}
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));++ops.context;CHECK(!owner.retire_on_rhi(drain()));CHECK(!ops.releases);
  --ops.context;CHECK(owner.retire_on_rhi(drain()));}
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));ops.code_ok=false;CHECK(!owner.retire_on_rhi(drain()));
  CHECK(!ops.releases&&!ops.clears&&owner.retained());}
 for(unsigned fault=0;fault<8;++fault){Mock ops;TestOwner owner;
  if(fault==0)ops.code_ok=false;if(fault==1)ops.metadata_ok=false;if(fault==2)ops.chain_ok=false;
  if(fault==3){ops.create_ok=false;ops.create_zero=true;}if(fault==4)ops.create_wrong_return=true;
  if(fault==5)ops.acquire_ok=false;if(fault==6)ops.desc_ok=false;if(fault==7)ops.device_ok=false;
  CHECK(!create(owner,ops));CHECK(!owner.borrow_adapter());CHECK(!ops.clears&&!ops.releases);
  if(fault>=2)CHECK(owner.retained());if(fault==3||fault==5||fault==7)CHECK(owner.snapshot().cleanup_uncertain);
  if(fault==5||fault==7){CHECK(!owner.retire_on_rhi(drain()));CHECK(!ops.clears&&!ops.releases);}}
 {Mock ops;TestOwner owner;ops.desc_ok=false;CHECK(!create(owner,ops));CHECK(owner.retained());
  CHECK(owner.retire_on_rhi(drain()));CHECK(!owner.retained()&&ops.clears==1&&ops.releases==1);}
 for(uintptr_t failed:{uintptr_t(0x800000),uintptr_t(0x500000)}){Mock ops;TestOwner owner;CHECK(create(owner,ops));ops.release_fault=failed;
  CHECK(!owner.retire_on_rhi(drain()));CHECK(owner.retained()&&!ops.clears);CHECK(owner.snapshot().cleanup_uncertain);
  auto releases=ops.releases;CHECK(!owner.retire_on_rhi(drain()));CHECK(ops.releases==releases);}
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));ops.clear_ok=false;CHECK(!owner.retire_on_rhi(drain()));
  CHECK(owner.retained()&&owner.snapshot().cleanup_uncertain&&owner.snapshot().surface==ops.s);}
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));auto* a=owner.borrow_adapter();
  using Dtor=void* (__fastcall*)(TestOwner::Adapter*,unsigned);CHECK(reinterpret_cast<Dtor>(a->vtable[0])(a,1)==a);
  CHECK(owner.snapshot().adapter_fault&&owner.retained());CHECK(!owner.retire_on_rhi(drain()));CHECK(!ops.clears&&!ops.releases);}
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));auto* a=owner.borrow_adapter();ops.gamma_ok=false;
  using Gamma=float (__fastcall*)(TestOwner::Adapter*);CHECK(reinterpret_cast<Gamma>(a->vtable[4])(a)==1.f);
  CHECK(owner.snapshot().adapter_fault);CHECK(!owner.retire_on_rhi(drain()));CHECK(owner.retained());}
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));auto* a=owner.borrow_adapter();ops.color_ok=false;
  using Color=void* (__fastcall*)(TestOwner::Adapter*,void*);unsigned char bytes[16];memset(bytes,0xff,16);
  CHECK(reinterpret_cast<Color>(a->vtable[5])(a,bytes)==bytes);CHECK(owner.snapshot().adapter_fault);
  float values[3]{};memcpy(values,bytes,12);CHECK(values[0]==1.f&&values[1]==1.f&&values[2]==1.f);
  CHECK(bytes[12]==2&&bytes[13]==255&&bytes[14]==255&&bytes[15]==255);
  CHECK(!owner.retire_on_rhi(drain()));CHECK(!ops.clears&&!ops.releases);}
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));auto* a=owner.borrow_adapter();ops.gamma_ok=false;
  using Color=void* (__fastcall*)(TestOwner::Adapter*,void*);unsigned char bytes[16];memset(bytes,0xff,16);
  CHECK(reinterpret_cast<Color>(a->vtable[5])(a,bytes)==bytes);CHECK(owner.snapshot().adapter_fault);
  float values[3]{};memcpy(values,bytes,12);CHECK(values[0]==1.f&&values[1]==1.f&&values[2]==1.f);
  CHECK(bytes[12]==2&&bytes[15]==255);CHECK(!owner.retire_on_rhi(drain()));}
 {Mock ops;{TestOwner owner;CHECK(create(owner,ops));}CHECK(!ops.clears&&!ops.releases);}
 for(auto dimensions:{std::pair<unsigned,unsigned>{6144,3264},{4302,2285},{8192,8192}}){Mock ops;TestOwner owner;
  CHECK(owner.create_on_rhi(ops,dimensions.first,dimensions.second,9));auto sample=owner.snapshot();
  CHECK(sample.width==dimensions.first&&sample.height==dimensions.second&&ops.width==dimensions.first&&ops.height==dimensions.second);
  CHECK(owner.retire_on_rhi(drain())&&!owner.retained());}
 for(auto dimensions:{std::pair<unsigned,unsigned>{127,3264},{6145,3264},{8194,3264},{6144,0},{6144,8193}}){Mock ops;TestOwner owner;
  CHECK(!owner.create_on_rhi(ops,dimensions.first,dimensions.second,9)&&!ops.creates&&!owner.retained());}
 // Scene shutdown cannot turn a worker request or a copy-only drain into a
 // native release. Exercise the new gate against the actual Owner template.
 for(unsigned missing=0;missing<5;++missing){Mock ops;TestOwner owner;CHECK(create(owner,ops));
  gears_owned_retirement::Gate gate;auto proof=drain();proof.gpu_fence_completed=false;
  if(missing==0)proof.generation=0;if(missing==1)proof.new_submissions_disabled=false;
  if(missing==2)proof.renderers_retired=false;if(missing==3)proof.canvas_commands_retired=false;
  if(missing==4)proof.producer_cpu_access_retired=false;
  CHECK(!gate.request(proof));CHECK(!gate.retire_on_rhi(owner));CHECK(owner.retained()&&!ops.releases&&!ops.clears);
 }
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));gears_owned_retirement::Gate gate;
  auto proof=drain();proof.gpu_fence_completed=false;CHECK(gate.request(proof));
  CHECK(!gate.gpu_drained(10,true,true,true));CHECK(!gate.gpu_drained(9,false,true,true));
  CHECK(!gate.gpu_drained(9,true,false,true));CHECK(!gate.gpu_drained(9,true,true,false));
  CHECK(!gate.retire_on_rhi(owner)&&owner.retained()&&!ops.releases&&!ops.clears);
  CHECK(gate.gpu_drained(9,true,true,true));ops.tid=43;
  CHECK(!gate.retire_on_rhi(owner)&&owner.retained()&&!ops.releases&&!ops.clears);
  ops.tid=42;++ops.context;CHECK(!gate.retire_on_rhi(owner)&&!ops.releases);--ops.context;
  CHECK(gate.retire_on_rhi(owner)&&!owner.retained());CHECK(gate.retire_on_rhi(owner));
  CHECK(ops.clears==1&&ops.releases==2&&gate.phase()==gears_owned_retirement::Phase::released);
 }
 {Mock ops;TestOwner owner;CHECK(create(owner,ops));auto* adapter=owner.borrow_adapter();
  gears_owned_retirement::Gate gate;gears_owned_retirement::Handoff handoff,foreign;
  gears_owned_retirement::Handoff::Receipt first{},second{};
  const gears_owned_retirement::ResourceIdentity identity{1,9,12,ops.d,0x900000};
  auto proof=drain();proof.gpu_fence_completed=false;
  CHECK(!handoff.begin(gate,identity,{9,true,false,true,true,false}));
  CHECK(handoff.begin(gate,identity,proof));CHECK(!handoff.receipt(gate,false,true,first));
  CHECK(!handoff.claim_bootstrap(gate,2,false)); // CPU/GPU retirement alone is not arm.
  CHECK(!owner.recreate_on_rhi(ops,2560,1400,13)&&ops.creates==1);
  CHECK(gate.gpu_drained(9,true,true,true)&&gate.retire_on_rhi(owner));
  CHECK(!handoff.receipt(gate,false,false,first));CHECK(!handoff.receipt(gate,true,true,first));
  CHECK(handoff.receipt(gate,false,true,first));CHECK(!foreign.current(gate,first));
  CHECK(!handoff.arm(gate,first,3,false));CHECK(!handoff.arm(gate,first,2,true));
  CHECK(handoff.arm(gate,first,2,false)&&handoff.arm(gate,first,2,false));
  CHECK(!handoff.can_recreate(2,13,true)&&ops.creates==1); // Stop after arm does not create.
  CHECK(!handoff.can_recreate(1,13,false)&&!handoff.can_recreate(2,12,false));
  CHECK(!handoff.can_recreate(2,13,false)); // A renderer must claim bootstrap first.
  gears_owned_scene::Trial old_trial;old_trial.resource=ops.d;old_trial.adapter=reinterpret_cast<uintptr_t>(adapter);
  old_trial.ticket=12;old_trial.returned=old_trial.destroyed=true;
  CHECK(old_trial.ticket&&!handoff.claim_bootstrap(gate,1,false));
  CHECK(!handoff.claim_bootstrap(gate,2,true)&&!handoff.bootstrap_claimed());
  auto permit=[&](){return !old_trial.ticket||handoff.claim_bootstrap(gate,2,false);};
  CHECK(permit()&&handoff.bootstrap_claimed()&&old_trial.ticket==12); // Old guard would reject this.
  CHECK(!permit()&&old_trial.ticket==12); // Only one unbound stereo clone may reach RHI.
  CHECK(handoff.arm(gate,first,2,false)&&!permit()); // Idempotent arm cannot mint another permit.
  CHECK(!handoff.can_recreate(2,13,true)&&old_trial.ticket==12); // Stop after claim still wins.
  CHECK(!owner.recreate_on_rhi(ops,2560,1400,9));
  ops.tid=43;CHECK(!owner.recreate_on_rhi(ops,2560,1400,13)&&ops.creates==1);ops.tid=42;
  CHECK(old_trial.ticket==12&&!permit()); // Failed RHI does not erase the retired ticket or reissue.
  ++ops.context;CHECK(!owner.recreate_on_rhi(ops,2560,1400,13)&&ops.creates==1);--ops.context;
  CHECK(handoff.can_recreate(2,13,false)&&owner.recreate_on_rhi(ops,2560,1401,13));
  CHECK(owner.borrow_adapter()==adapter&&owner.snapshot().generation==13&&owner.snapshot().height==1401);
  CHECK(handoff.recreated(gate,2,13)&&!handoff.current(gate,first));
  CHECK(!handoff.claim_bootstrap(gate,2,false)&&!handoff.bootstrap_claimed());
  gears_owned_scene::Trial rebuilt;CHECK(rebuilt.ready(ops.d,reinterpret_cast<uintptr_t>(adapter),13));
  CHECK(rebuilt.select(0xa00000,14,reinterpret_cast<uintptr_t>(adapter)));
  CHECK(!rebuilt.select(0xa00010,15,reinterpret_cast<uintptr_t>(adapter))); // Original one-owned-frame policy.
  CHECK(!handoff.arm(gate,first,2,false));
  const gears_owned_retirement::ResourceIdentity next{2,13,20,ops.d,0x900000};
  CHECK(handoff.begin(gate,next,{13,true,true,true,true,false}));
  CHECK(gate.gpu_drained(13,true,true,true)&&gate.retire_on_rhi(owner));
  CHECK(handoff.receipt(gate,false,true,second)&&second.serial()>first.serial());
  CHECK(!handoff.current(gate,first)&&handoff.current(gate,second));
  CHECK(handoff.arm(gate,second,3,false)&&!handoff.claim_bootstrap(gate,2,false));
  CHECK(handoff.claim_bootstrap(gate,3,false)&&!handoff.claim_bootstrap(gate,3,false));
  CHECK(ops.creates==2&&ops.clears==2&&ops.releases==4&&!owner.retained());
 }
 std::printf("native owned surface: %u CPU checks passed\n",checks);
}
