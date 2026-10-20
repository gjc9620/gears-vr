// Included after Material sidecar/context declarations. No native objects are
// rewritten; the original producer/consumer/upload each execute exactly once.
namespace owned_material {
using Produce=uint32_t(__fastcall*)(void*,void*);
using Upload=uintptr_t(__fastcall*)(void*,unsigned,unsigned,void*,unsigned);
Produce original_produce{};Upload original_upload{};
void* produce_target{};void* upload_target{};
std::atomic<uint64_t> produced{},executed{},completed{},uploads{},draws{},faults{};
std::atomic<unsigned> first_fault{};
struct DrawFailure {unsigned reason{},targets{},width{},height{},stride{},topology{};uintptr_t actual{},expected{},list{},native_list{};
 uint64_t ticket{},pair_ticket{};bool linked{};std::array<float,4> rect{};std::array<float,16> projection{};};
DrawFailure first_draw_failure{};std::mutex failure_mutex;
struct UploadContext {UiMaterialContext* material{};uintptr_t native_context{};gears_ui_material::Quad quad{};};
thread_local UploadContext* uploading{};
void fault(uint64_t ticket,unsigned reason){++faults;unsigned empty{};first_fault.compare_exchange_strong(empty,reason);owned_ui_live::fault(ticket,reason);}
void fail(UiMaterialContext& c,unsigned reason){c.owned_failed=true;fault(c.owned_ticket,reason);}
uint32_t __fastcall hook_produce(void* item,void* canvas){
 Scope scope;if(!scope.world())return original_produce(item,canvas);
 uint64_t receipt{},ticket{};
 const auto f=owned_ui_live::flushing;
 if(ui_observer_enabled&&recording.load()&&f.ticket){
  ticket=f.ticket;uintptr_t target{},secondary{},table{},payload{};unsigned flags{};
  const bool valid=f.canvas==reinterpret_cast<uintptr_t>(canvas)&&f.thread==GetCurrentThreadId()&&
   owned_ui_live::target_of(canvas,target,secondary)&&target==f.target&&!secondary&&item&&
   safe_copy(&table,item,8)&&table==base+0x136c5f0&&
   safe_copy(&payload,static_cast<unsigned char*>(item)+8,8)&&payload&&
   safe_copy(&flags,static_cast<unsigned char*>(item)+16,4)&&!flags;
  if(valid){std::lock_guard<std::mutex> lock(ui_material_mutex);
   for(auto& e:ui_material_entries)if(e.occupied&&!e.poisoned&&!ui_material_disabled&&
      !e.owned_receipt&&e.receipt.item==reinterpret_cast<uintptr_t>(item)&&e.receipt.payload==payload&&e.receipt.fcanvas==f.canvas){
    receipt=owned_ui_live::ledger.produce(ticket,payload,now(),gears_owned_ui::Kind::Material,target);
    if(receipt){e.owned_receipt=receipt;e.owned_ticket=ticket;++produced;}break;
   }
  }
  if(!receipt)fault(ticket,20);
 }
 const auto result=original_produce(item,canvas);
 // The native Material producer leaves item+8 intact; payload may already be
 // destroyed by a synchronous consumer. Do not dereference it after return.
 if(receipt&&!owned_ui_live::ledger.transferred(receipt,result==1?gears_owned_ui::TransferProof::MaterialDispatched:gears_owned_ui::TransferProof::Rejected))fault(ticket,21);
 return result;
}
void begin(UiMaterialContext& c,UiMaterialEntry const& entry){
 if(!entry.owned_receipt)return;
 c.owned_receipt=entry.owned_receipt;c.owned_ticket=entry.owned_ticket;
 const auto& s=c.consumer;
 const auto id=s.command?owned_ui_live::ledger.execute(s.receipt.payload,s.target,now(),gears_owned_ui::Kind::Material):0;
 gears_owned_ui::Execution e{};
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 const auto& trial=gears_owned_scene_live::trial;
 if(id!=c.owned_receipt||!owned_ui_live::ledger.execution(id,e)||e.ticket!=c.owned_ticket||
    e.kind!=gears_owned_ui::Kind::Material||!gears_owned_scene_live::healthy()||
    trial.ticket!=c.owned_ticket||!trial.resource||!owned_scene_gpu::watching()) {fail(c,22);return;}
 c.owned_resource=trial.resource;++executed;
}
void end(UiMaterialContext& c,uint32_t result){
 if(!c.owned_receipt)return;
 const bool ok=result==36&&c.owned_resource&&c.owned_draws&&!c.owned_failed;
 if(!owned_ui_live::ledger.complete(c.owned_receipt,ok,now(),gears_owned_ui::Kind::Material)||!ok)fail(c,23);
 else ++completed;
}
uintptr_t __fastcall hook_upload(void* native_context,unsigned primitive,unsigned primitives,void* vertices,unsigned stride){
 Scope scope;if(!scope.world())return original_upload(native_context,primitive,primitives,vertices,stride);
 auto saved=uploading;uploading=nullptr;UploadContext u{};
 if(ui_material_context&&ui_material_context->owned_ticket&&recording.load()){
  auto& c=*ui_material_context;
  if(primitive==1&&primitives==2&&stride==32&&
     gears_ui_material::capture_quad({base},safe_copy,c.consumer,u.quad)&&
     !u.quad.index_data&&u.quad.source==reinterpret_cast<uintptr_t>(vertices)){
   u.material=&c;u.native_context=reinterpret_cast<uintptr_t>(native_context);uploading=&u;++uploads;
  }else fail(c,24);
 }
 struct Restore {UploadContext* saved;~Restore(){uploading=saved;}} restore{saved};
 // Verified body copies count*stride from this exact pointer, establishes its
 // buffer/stride/topology, flushes native state, then calls DrawInstanced.
 return original_upload(native_context,primitive,primitives,vertices,stride);
}
bool bytes_match(uintptr_t rva,ULONG length,const char* expected){
 BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::array<unsigned char,1024> bytes{};
 unsigned char digest[32]{};char hex[65]{};
 bool ok=length<=bytes.size()&&safe_copy(bytes.data(),reinterpret_cast<void*>(base+rva),length)&&
  BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
 if(ok)ok=BCryptHashData(hash,bytes.data(),length,0)>=0&&BCryptFinishHash(hash,digest,32,0)>=0;
 for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
 return ok&&!strcmp(hex,expected);
}
bool reserve(){
 if(!bytes_match(0x3a514c,0x3fc,"193ddb21a13443746e258f12f799968a47ca9d21fe8badf3cd5c1884a462931b")||
    !bytes_match(0x1af054,0x2fb,"4cf2c5b249b2abb2decb11cae8adf0eacc3a08833345691e63fcbaf7eae7c41a"))return false;
 produce_target=reinterpret_cast<void*>(base+0x3a514c);upload_target=reinterpret_cast<void*>(base+0x1af054);
 return MH_CreateHook(produce_target,reinterpret_cast<void*>(hook_produce),reinterpret_cast<void**>(&original_produce))==MH_OK&&
  MH_CreateHook(upload_target,reinterpret_cast<void*>(hook_upload),reinterpret_cast<void**>(&original_upload))==MH_OK;
}
void progress(FILE* file){
 DrawFailure d{};{std::lock_guard<std::mutex> lock(failure_mutex);d=first_draw_failure;}
 fprintf(file,"owned_material_faults=%llu owned_material_first_fault=%u owned_material_draw_failure_reason=%u owned_material_draw_failure_ticket=%llu owned_material_draw_failure_pair=%llu owned_material_draw_failure_linked=%u owned_material_draw_failure_targets=%u owned_material_draw_failure_actual=%llx owned_material_draw_failure_expected=%llx owned_material_draw_failure_width=%u owned_material_draw_failure_height=%u owned_material_draw_failure_list=%llx owned_material_draw_failure_native_list=%llx owned_material_draw_failure_stride=%u owned_material_draw_failure_topology=%u ",
  faults.load(),first_fault.load(),d.reason,d.ticket,d.pair_ticket,unsigned(d.linked),d.targets,d.actual,d.expected,d.width,d.height,d.list,d.native_list,d.stride,d.topology);
}
void print(FILE* file){fprintf(file,"owned_material produced=%llu executed=%llu completed=%llu uploads=%llu draws=%llu faults=%llu first_fault=%u\n",
 produced.load(),executed.load(),completed.load(),uploads.load(),draws.load(),faults.load(),first_fault.load());
 std::lock_guard<std::mutex> lock(failure_mutex);const auto& d=first_draw_failure;if(!d.reason)return;
 fprintf(file,"owned_material_draw_failure reason=%u ticket=%llu pair=%llu linked=%u targets=%u actual=%llx expected=%llx width=%u height=%u list=%llx native_list=%llx stride=%u topology=%u rect=%.9g,%.9g,%.9g,%.9g projection=",
  d.reason,d.ticket,d.pair_ticket,d.linked,d.targets,d.actual,d.expected,d.width,d.height,d.list,d.native_list,d.stride,d.topology,d.rect[0],d.rect[1],d.rect[2],d.rect[3]);
 for(unsigned i=0;i<16;++i)fprintf(file,"%s%.9g",i?",":"",d.projection[i]);fprintf(file,"\n");
}
}
