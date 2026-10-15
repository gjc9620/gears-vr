// Included after install()/Scope. Diagnostic builds only. Arguments stay native
// unless the separate Low capacity reuse experiment is enabled. Preserve RAX.
// This integer return transport does not assert
// a recovered C++ return type. Cached decompiler prototypes are not used.
#if defined(GEARS_NATIVE_RESIZE_PROBE) && GEARS_NATIVE_RESIZE_PROBE
namespace resource_resize_probe {
using Fn=uintptr_t(__fastcall*)(void*,uint32_t,uint32_t);
Fn original{};
gears_resize_probe::Buffer<4096> buffer;
#if defined(GEARS_NATIVE_RESIZE_REUSE) && GEARS_NATIVE_RESIZE_REUSE
constexpr bool reuse=true;
gears_resize_probe::PinnedCapacity capacity;
#else
constexpr bool reuse=false;
#endif
constexpr uintptr_t globals[]={0x1b98b6c,0x1c3c9e8,0x1c3c9ec,0x1ad19d8,0x1c227d0,0x1c227d4,0x1c227d8};
gears_resize_probe::Snapshot snapshot(void* object){
 gears_resize_probe::Snapshot s{};
 uint32_t dims[2]{};
 if(safe_copy(dims,reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(object)+0xf8),sizeof(dims))){s.width=dims[0];s.height=dims[1];s.valid=1;}
 for(unsigned i=0;i<7;++i)if(safe_copy(&s.config[i],reinterpret_cast<void*>(base+globals[i]),4))s.valid|=2u<<i;
 return s;
}
uintptr_t __fastcall hook(void* object,uint32_t width,uint32_t height){
 Scope active_scope;
 if(!recording.load()||self_testing)return original(object,width,height);
 uint64_t index{};auto row=buffer.reserve(index);
 // Even overflow isolates nested calls from their parent's API counters.
 gears_resize_probe::Window window(row);
 const auto before=(row||reuse)?snapshot(object):gears_resize_probe::Snapshot{};
 const auto thread=GetCurrentThreadId();const bool owned=consuming_owned_scene&&consuming_frame_ticket&&consuming_renderer;
 auto forward_width=width,forward_height=height;unsigned policy{};
#if defined(GEARS_NATIVE_RESIZE_REUSE) && GEARS_NATIVE_RESIZE_REUSE
 policy=capacity.choose(reinterpret_cast<uintptr_t>(object),thread,owned,before,forward_width,forward_height);
#endif
 if(row){row->object=reinterpret_cast<uintptr_t>(object);row->caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;
  row->renderer=reinterpret_cast<uintptr_t>(consuming_renderer);row->ticket=consuming_frame_ticket;
  row->owned=owned?1:0;row->thread=thread;row->width=width;row->height=height;
  row->forward_width=forward_width;row->forward_height=forward_height;row->policy=policy;
  row->before=before;row->begin=now();}
 const auto result=original(object,forward_width,forward_height);
 if(row){row->end=now();row->after=snapshot(object);row->result=result;buffer.finish(index);}
 return result;
}
bool verify(FILE* output){
 // SHA of all known chunks in source snapshot artifact 1, independently
 // runtime-matched in PID50924. Verify this process again before any patch.
 BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
 bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
 struct Chunk{uintptr_t rva;ULONG size;};
 const Chunk chunks[]={{0x2b59f4,0x124},{0x8ef386,0x48}};
 std::array<unsigned char,0x124> bytes{};
 for(const auto& c:chunks)if(ok)ok=safe_copy(bytes.data(),reinterpret_cast<void*>(base+c.rva),c.size)&&BCryptHashData(hash,bytes.data(),c.size,0)>=0;
 unsigned char digest[32]{};char hex[65]{};
 if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
 for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
 ok=ok&&!strcmp(hex,"caaeccd655338a64bccd980b7844e39f05550bdbd33fd1d2e93f11cb3b220f59");
 fprintf(output,"resource_resize_identity pid=%lu module_base=%llx rva=2b59f4 bytes=364 sha256=%s match=%u\n",GetCurrentProcessId(),static_cast<unsigned long long>(base),hex,ok?1:0);
 return ok;
}
}
bool enable_resource_resize_probe(FILE* output){
 if(self_testing)return true;
 if(!continuous_session||!family_diagnostics||!resource_resize_probe::verify(output))return false;
 return install(reinterpret_cast<void*>(base+0x2b59f4),reinterpret_cast<void*>(resource_resize_probe::hook),reinterpret_cast<void**>(&resource_resize_probe::original));
}
void print_resource_resize_probe(FILE* output){
 auto& buffer=resource_resize_probe::buffer;
 const auto attempted=buffer.attempts.load();
 fprintf(output,"resource_resize_probe attempts=%llu capacity=%zu overflow=%llu rva=2b59f4 no_argument_changes=%u capacity_reuse=%u qpc_frequency=%lld\n",attempted,buffer.rows.size(),attempted>buffer.rows.size()?attempted-buffer.rows.size():0,resource_resize_probe::reuse?0:1,resource_resize_probe::reuse?1:0,qpc_frequency);
 for(size_t i=0;i<buffer.rows.size();++i){if(!buffer.ready[i].load(std::memory_order_acquire))continue;const auto& r=buffer.rows[i];
  fprintf(output,"resource_resize_call index=%zu thread=%lu object=%llx caller=%llx renderer=%llx ticket=%llu owned=%u nested=%u request=%u,%u begin=%llu end=%llu result=%llx before=%u,%u after=%u,%u valid=%u,%u config_before=",
   i,r.thread,r.object,r.caller,r.renderer,r.ticket,r.owned,r.nested,r.width,r.height,r.begin,r.end,r.result,r.before.width,r.before.height,r.after.width,r.after.height,r.before.valid,r.after.valid);
  for(unsigned j=0;j<7;++j)fprintf(output,"%s%u",j?",":"",r.before.config[j]);
  fprintf(output," forwarded=%u,%u policy=%u",r.forward_width,r.forward_height,r.policy);
  fprintf(output," config_after=");for(unsigned j=0;j<7;++j)fprintf(output,"%s%u",j?",":"",r.after.config[j]);
  fprintf(output," api_calls=");for(unsigned j=0;j<8;++j)fprintf(output,"%s%u",j?",":"",r.calls[j]);
  fprintf(output," api_failed=");for(unsigned j=0;j<8;++j)fprintf(output,"%s%u",j?",":"",r.failed[j]);fprintf(output,"\n");
 }
}
#else
bool enable_resource_resize_probe(FILE*){return true;}
void print_resource_resize_probe(FILE*){}
#endif
