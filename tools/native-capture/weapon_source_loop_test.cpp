#define main original_fixture_main
#include "weapon_hand_scene_binding_test.cpp"
#undef main
static h::Proof synthetic;
static unsigned plan_checks=0;
#define P(x) do{++plan_checks;if(!(x)){printf("SOURCE_FAIL line=%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));P(arena);setup();h::Binding binding{};P(take(binding));real_memory_mode=true;
 for(auto pp:{&h::base_proof,&h::instigator_proof})for(unsigned i=0;i<pp->source_count;++i)for(unsigned k=0;k<pp->sources[i].size;++k){
 auto address=pp->sources[i].address+k;auto old=get<unsigned char>(address);put(address,static_cast<unsigned char>(old^1));P(!h::source_loop_original(*pp)&&!h::source_loop_exact(*pp));put(address,old);}

 for(unsigned count:{3640u,1027u})for(unsigned scatter=0;scatter<2;++scatter){synthetic={};auto& p=synthetic;p.source_count=count;
 for(unsigned i=0;i<count;++i){auto& s=p.sources[i];unsigned node=scatter?(i*2003u)%count:i;s.address=arena+uintptr_t(node)*256+0x68;s.size=8;uint64_t v=0x123456789abcdef0ull+i;put(s.address,v);memcpy(s.bytes.data(),&v,8);}
 P(h::source_loop_original(p)&&h::source_loop_exact(p));
 for(unsigned i=0;i<count;++i)for(unsigned k=0;k<8;++k){auto& src=p.sources[i];auto address=src.address+k;auto old=get<unsigned char>(address);put(address,static_cast<unsigned char>(old^1));P(!h::source_loop_original(p)&&!h::source_loop_exact(p));src.bytes[k]^=1;P(h::source_loop_original(p)&&h::source_loop_exact(p));src.bytes[k]^=1;put(address,old);}
 auto saved=p.sources[0];for(unsigned size=1;size<=8;++size){p.sources[0].size=size;P(h::source_loop_original(p)==h::source_loop_exact(p));}p.sources[0]=saved;
 p.sources[0].address=arena+0xff000;memcpy(reinterpret_cast<void*>(p.sources[0].address),saved.bytes.data(),8);P(h::source_loop_original(p)&&h::source_loop_exact(p));p.sources[0]=saved;
 --p.source_count;P(h::source_loop_original(p)&&h::source_loop_exact(p));++p.source_count;
 for(auto invalid:{uintptr_t(0),UINTPTR_MAX-1}){p.sources[0].address=invalid;P(!h::source_loop_original(p)&&!h::source_loop_exact(p));}p.sources[0]=saved;
 p.sources[0].size=9;P(!h::source_loop_original(p)&&!h::source_loop_exact(p));p.sources[0]=saved;
 p.source_count=4097;P(!h::source_loop_original(p)&&!h::source_loop_exact(p));p.source_count=count;
 }
 synthetic={};auto& p=synthetic;p.source_count=1;p.sources[0].address=arena+0x1000;p.sources[0].size=8;uint64_t value=1234;put(p.sources[0].address,value);memcpy(p.sources[0].bytes.data(),&value,8);
 DWORD old{};for(unsigned mode=0;mode<2;++mode){P(VirtualProtect(reinterpret_cast<void*>(arena+0x1000),4096,PAGE_READWRITE|PAGE_GUARD,&old));
 P(!(mode?h::source_loop_exact(p):h::source_loop_original(p)));P(mode?h::source_loop_exact(p):h::source_loop_original(p));}
 P(VirtualProtect(reinterpret_cast<void*>(arena+0x1000),4096,PAGE_NOACCESS,&old));P(!h::source_loop_original(p)&&!h::source_loop_exact(p));P(VirtualProtect(reinterpret_cast<void*>(arena+0x1000),4096,PAGE_READWRITE,&old));
 // Stub-injected read denial is intentionally NOT silently presented as covered by direct reads.
 real_memory_mode=false;denied=p.sources[0].address;P(!h::source_loop_original(p)&&h::source_loop_exact(p));denied=0;real_memory_mode=true;
 // Identity loop still executes the original safe_copy reader before direct source reads.
 setup();P(take(binding));P(h::current(h::base_proof));auto id_address=h::base_proof.identities[0].address;auto id_byte=get<unsigned char>(id_address+8);put(id_address+8,static_cast<unsigned char>(id_byte^1));P(!h::current(h::base_proof));put(id_address+8,id_byte);P(h::current(h::base_proof));

 // Production direct reads: all widths at unaligned and cross-page positions.
 real_memory_mode=true;synthetic={};synthetic.source_count=1;
 const auto boundary=arena+0xfc000;
 for(unsigned size=1;size<=8;++size)for(int delta=-8;delta<=8;++delta){
  auto& src=synthetic.sources[0];src.address=uintptr_t(intptr_t(boundary)+delta);src.size=size;
  for(unsigned k=0;k<8;++k){src.bytes[k]=static_cast<unsigned char>(0x30+k);put(src.address+k,src.bytes[k]);}
  P(h::source_loop_original(synthetic)&&h::source_loop_exact(synthetic));
  for(unsigned k=0;k<size;++k){const auto saved=get<unsigned char>(src.address+k);put(src.address+k,static_cast<unsigned char>(saved^1));
   P(!h::source_loop_original(synthetic)&&!h::source_loop_exact(synthetic));put(src.address+k,saved);}
 }
 for(unsigned mode=0;mode<2;++mode){auto& src=synthetic.sources[0];src.address=boundary-4;src.size=8;
  for(unsigned k=0;k<8;++k){src.bytes[k]=static_cast<unsigned char>(0x50+k);put(src.address+k,src.bytes[k]);}
  P(VirtualProtect(reinterpret_cast<void*>(boundary),4096,PAGE_READWRITE|PAGE_GUARD,&old));
  P(!(mode?h::source_loop_exact(synthetic):h::source_loop_original(synthetic)));
  P(mode?h::source_loop_exact(synthetic):h::source_loop_original(synthetic));
 }
 P(VirtualProtect(reinterpret_cast<void*>(boundary),4096,PAGE_NOACCESS,&old));
 // Exact-width last-readable4-byte load must not touch the next page.
 auto& edge=synthetic.sources[0];edge.address=boundary-4;edge.size=4;
 memcpy(edge.bytes.data(),reinterpret_cast<void const*>(edge.address),4);
 P(h::source_loop_original(synthetic)&&h::source_loop_exact(synthetic));
 edge.size=8;P(!h::source_loop_original(synthetic)&&!h::source_loop_exact(synthetic));
 P(VirtualProtect(reinterpret_cast<void*>(boundary),4096,PAGE_READWRITE,&old));
 P(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE));printf("source_loop checks=%u failures=0\n",plan_checks);return 0;}
