// Opt-in diagnostic only. Include after ui_payload_observer.inl and before
// ui_observer_hooks.inl. Root integrates the existing Canvas/GPU wrappers.
#include "ui_material_batch.h"
UiExecute ui_original_material_execute{};void* ui_material_execute_target{};
struct UiMaterialEntry {gears_ui_material::Receipt receipt{};uint64_t invocation{};bool occupied{},poisoned{};};
std::array<UiMaterialEntry,32> ui_material_entries{};
std::mutex ui_material_mutex;
std::atomic<uint64_t> ui_material_sequence{},ui_material_published{},ui_material_rejected{},ui_material_execute_calls{},ui_material_claimed{},ui_material_content_rejected{},ui_material_bad_return{},ui_material_gpu_candidates{},ui_material_gpu_rejected{};
std::atomic<uint64_t> ui_material_before_attempts{},ui_material_before_rejected{};
struct UiMaterialRejectRow {uintptr_t command{},payload{};unsigned mode{},flags{},read_mask{};int count{},capacity{};};
std::array<UiMaterialRejectRow,4> ui_material_reject_rows{};unsigned ui_material_reject_count{};
struct UiMaterialAppendReject {uintptr_t item{};unsigned flags{},read_mask{};int count{},capacity{};bool recorded{};} ui_material_append_reject;
bool ui_material_disabled{};
struct UiMaterialContext {gears_ui_material::Consumer consumer{};uint64_t invocation{},sequence{};unsigned gpu_calls{};};
thread_local UiMaterialContext* ui_material_context{};
struct UiMaterialRestore {UiMaterialContext* saved=ui_material_context;~UiMaterialRestore(){ui_material_context=saved;}};
void complete_ui_material_rows(uint64_t sequence,bool complete);
bool ui_material_before(void* canvas,gears_ui_material::Before& out){
 ++ui_material_before_attempts;const bool ok=ui_observer_enabled&&recording.load()&&gears_ui_material::snapshot_before({base},safe_copy,reinterpret_cast<uintptr_t>(canvas),out);if(!ok)++ui_material_before_rejected;return ok;
}
void record_ui_material_content_reject(uintptr_t command){
 std::lock_guard<std::mutex> lock(ui_material_mutex);if(ui_material_reject_count>=ui_material_reject_rows.size())return;
 UiMaterialRejectRow row{};row.command=command;gears_ui_batch::detail::Reader<decltype(&safe_copy)> read{safe_copy};uintptr_t table{};
 if(read.field(table,command,0)&&table==base+0x136c5d8){row.read_mask|=1;
  if(read.field(row.mode,command,0x18))row.read_mask|=2;if(read.field(row.flags,command,0x20))row.read_mask|=4;
  if(read.field(row.payload,command,0x10)&&row.payload){row.read_mask|=8;gears_ui_batch::detail::Array records{};
   if(read.field(records,row.payload,0x60)){row.read_mask|=16;row.count=records.count;row.capacity=records.capacity;}}
 }
 ui_material_reject_rows[ui_material_reject_count++]=row;
}
void record_ui_material_append_reject(gears_ui_material::Before const& before){
 // Traversal uses the same bounded certified FCanvas/key selection as producer
 // admission. Only an exact Material vtable permits its payload interpretation.
 gears_ui_material::Before now{};if(!gears_ui_material::snapshot_before({base},safe_copy,before.canvas,now)||now.fcanvas!=before.fcanvas||now.key!=before.key||now.count!=before.count+1)return;
 for(unsigned i=0;i<before.count;++i)if(now.items[i]!=before.items[i])return;
 UiMaterialAppendReject row{};row.item=now.items[before.count];gears_ui_batch::detail::Reader<decltype(&safe_copy)> read{safe_copy};uintptr_t table{},payload{};
 if(!read.field(table,row.item,0)||table!=base+0x136c5f0)return;row.read_mask=1;
 if(read.field(row.flags,row.item,0x10))row.read_mask|=2;
 if(read.field(payload,row.item,8)&&payload){row.read_mask|=4;gears_ui_batch::detail::Array records{};
  if(read.field(records,payload,0x60)){row.read_mask|=8;row.count=records.count;row.capacity=records.capacity;}}
 row.recorded=true;std::lock_guard<std::mutex> lock(ui_material_mutex);if(!ui_material_append_reject.recorded)ui_material_append_reject=row;
}
void ui_material_after(gears_ui_material::Before const& before,unsigned kind,uint64_t invocation){
 gears_ui_material::Receipt receipt{};
 if(!gears_ui_material::snapshot_after({base},safe_copy,before,kind,receipt)){++ui_material_rejected;record_ui_material_append_reject(before);return;}
 std::lock_guard<std::mutex> lock(ui_material_mutex);
 if(ui_material_disabled){++ui_material_rejected;return;}
 UiMaterialEntry* vacant=nullptr;
 for(auto& entry:ui_material_entries){if(!entry.occupied){if(!vacant)vacant=&entry;continue;}
  if(entry.receipt.payload==receipt.payload){entry.poisoned=true;++ui_material_rejected;return;}}
 // No eviction: exhaustion disables later publication to prevent address ABA
 // from turning an abandoned producer entry into a fresh semantic claim.
 if(!vacant){ui_material_disabled=true;++ui_material_rejected;return;}
 *vacant={receipt,invocation,true,false};++ui_material_published;
}
uint32_t __fastcall hook_ui_material_execute(void* command){
 Scope active_scope;UiMaterialRestore restore;ui_material_context=nullptr;
 UiMaterialContext context{};UiMaterialEntry claimed{};bool have=false;
 if(ui_observer_enabled&&recording.load()){
  ++ui_material_execute_calls;uintptr_t payload{};
  if(command&&safe_copy(&payload,reinterpret_cast<unsigned char*>(command)+16,8)){
   std::lock_guard<std::mutex> lock(ui_material_mutex);
   for(auto& entry:ui_material_entries)if(entry.occupied&&entry.receipt.payload==payload){claimed=entry;entry={};have=!claimed.poisoned&&!ui_material_disabled;break;}
  }
  if(have){++ui_material_claimed;
   if(gears_ui_material::begin_consumer({base},safe_copy,claimed.receipt,reinterpret_cast<uintptr_t>(command),context.consumer)){
    context.invocation=claimed.invocation;context.sequence=++ui_material_sequence;ui_material_context=&context;
   }else {++ui_material_content_rejected;record_ui_material_content_reject(reinterpret_cast<uintptr_t>(command));}
  }
 }
 const auto result=ui_original_material_execute(command); // Exactly once.
 // Native payload/view may already be destroyed. Only the copied context is used.
 if(ui_material_context){const bool complete=gears_ui_material::finish_consumer(context.consumer,result);if(!complete)++ui_material_bad_return;complete_ui_material_rows(context.sequence,complete);}
 return result;
}
bool reserve_ui_material_execute(){
 ui_material_execute_target=reinterpret_cast<void*>(base+0x15a420);
 uintptr_t entry{};std::array<unsigned char,32> after{};
 return gears_ui_material::validate_binding({base},safe_copy)&&
  safe_copy(&entry,reinterpret_cast<void*>(base+0x136c5d8+8),8)&&entry==base+0x15a420&&
  MH_CreateHook(ui_material_execute_target,reinterpret_cast<void*>(hook_ui_material_execute),reinterpret_cast<void**>(&ui_original_material_execute))==MH_OK&&
  safe_copy(after.data(),ui_material_execute_target,after.size())&&after==gears_ui_material::execute_bytes;
}
struct UiMaterialGpuRow {uint64_t invocation{},ticket{},sequence{};uintptr_t caller{},payload{},native_target{},list{},resource{},source{},command{};unsigned kind{},calls{};bool indexed{},command_completed{};};
std::array<UiMaterialGpuRow,24> ui_material_rows{};unsigned ui_material_row_count{};
struct UiMaterialGpuRejectRow {
 uintptr_t command{},payload{},view{},family{},target{},caller{};
 gears_frame_link::ListRef list{};gears_frame_link::Target om{};
 gears_ui_material::DrawArgs args{};gears_ui_material::QuadDiagnostic quad{};bool quad_ok{};
 uintptr_t expected_proxy{};uint64_t sequence{};unsigned kind{};
};
std::array<UiMaterialGpuRejectRow,4> ui_material_gpu_reject_rows{};unsigned ui_material_gpu_reject_count{};
void complete_ui_material_rows(uint64_t sequence,bool complete){std::lock_guard<std::mutex> lock(ui_material_mutex);for(unsigned i=0;i<ui_material_row_count;++i)if(ui_material_rows[i].sequence==sequence)ui_material_rows[i].command_completed=complete;}
// Called before original GPU draw. ref/target must come from current DrawState;
// pair is a candidate frame-link lookup, not proof that the draw actually used
// these four CPU vertices. IA and material projection are explicitly unverified.
void observe_ui_material_gpu(uintptr_t caller,gears_frame_link::ListRef ref,gears_frame_link::Target target,gears_ui_material::DrawArgs args){
 if(!ui_material_context||!ui_observer_enabled||!recording.load())return;
 gears_ui_material::Quad quad{};gears_ui_material::Candidate candidate{};auto& context=*ui_material_context;
 gears_ui_material::QuadDiagnostic diagnostic{};
 const bool quad_ok=gears_ui_material::capture_quad({base},safe_copy,context.consumer,quad,&diagnostic);
 if(!quad_ok||!gears_ui_material::classify_draw({base},context.consumer,quad,caller,args,candidate)){
  ++ui_material_gpu_rejected;
  std::lock_guard<std::mutex> lock(ui_material_mutex);
  if(ui_material_gpu_reject_count<ui_material_gpu_reject_rows.size()){
   const auto& c=context.consumer;
   ui_material_gpu_reject_rows[ui_material_gpu_reject_count++]={c.command,c.receipt.payload,c.view,c.family,c.target,caller,ref,target,args,diagnostic,quad_ok,c.receipt.proxy,context.sequence,c.receipt.kind};
  }
  return;
 }
 ++ui_material_gpu_candidates;++context.gpu_calls;const auto pair=frame_link.overlay_pair(ref,target);
 std::lock_guard<std::mutex> lock(ui_material_mutex);if(ui_material_row_count>=ui_material_rows.size())return;
 ui_material_rows[ui_material_row_count++]={context.invocation,pair.ticket,context.sequence,caller,candidate.payload,candidate.target,ref.identity,target.resource,quad.source,context.consumer.command,candidate.kind,context.gpu_calls,args.indexed,false};
}
void print_ui_material_observer(FILE* status){
 std::lock_guard<std::mutex> lock(ui_material_mutex);
 fprintf(status,"ui_material_before attempts=%llu rejected=%llu\n",ui_material_before_attempts.load(),ui_material_before_rejected.load());
 // quad_step: 0=passed, 1=mesh_read, 2=metadata, 3=vertex_read,
 // 4=vertex_match, 5=mesh_recheck, 6=vertex_recheck. No native reads here.
 for(unsigned i=0;i<ui_material_gpu_reject_count;++i){const auto& r=ui_material_gpu_reject_rows[i];const auto& q=r.quad;const auto& a=r.args;
  fprintf(status,"ui_material_gpu_reject row=%u command=%p payload=%p view=%p family=%p logical_target=%p caller=%p list=%p list_generation=%llu om_resource=%p om_width=%u om_height=%u indexed=%u count=%u instances=%u start=%u first_instance=%u base_vertex=%d quad_ok=%u quad_step=%u stride=%u first=%u primitives=%u minimum=%u maximum=%u flags=%u proxy=%p index_data=%p source=%p expected_proxy=%p sequence=%llu kind=%u sampled_before_original=1 gpu_ia_verified=0 mutation_allowed=0\n",
   i,reinterpret_cast<void*>(r.command),reinterpret_cast<void*>(r.payload),reinterpret_cast<void*>(r.view),reinterpret_cast<void*>(r.family),reinterpret_cast<void*>(r.target),reinterpret_cast<void*>(r.caller),reinterpret_cast<void*>(r.list.identity),r.list.generation,reinterpret_cast<void*>(r.om.resource),r.om.width,r.om.height,
   a.indexed?1u:0u,a.count,a.instances,a.start,a.first_instance,a.base_vertex,r.quad_ok?1u:0u,q.step,q.stride,q.first,q.primitives,q.minimum,q.maximum,unsigned(q.flags),reinterpret_cast<void*>(q.proxy),reinterpret_cast<void*>(q.index_data),reinterpret_cast<void*>(q.source),reinterpret_cast<void*>(r.expected_proxy),r.sequence,r.kind);
 }
 for(unsigned i=0;i<ui_material_reject_count;++i){auto const& row=ui_material_reject_rows[i];fprintf(status,"ui_material_content_reject row=%u command=%p payload=%p read_mask=%u mode=%u commandflags=%u payloadcount=%d payloadcapacity=%d sampled_before_original=1\n",i,reinterpret_cast<void*>(row.command),reinterpret_cast<void*>(row.payload),row.read_mask,row.mode,row.flags,row.count,row.capacity);}
 if(ui_material_append_reject.recorded){auto const& row=ui_material_append_reject;fprintf(status,"ui_material_append_reject item=%p read_mask=%u itemflags=%u payloadcount=%d payloadcapacity=%d\n",reinterpret_cast<void*>(row.item),row.read_mask,row.flags,row.count,row.capacity);}
 fprintf(status,"ui_material published=%llu rejected=%llu executes=%llu claims=%llu content_rejected=%llu bad_return=%llu gpu_candidates=%llu gpu_rejected=%llu disabled=%u source_vertices_modified=0 gpu_ia_verified=0 projection_verified=0 mutation_allowed=0\n",ui_material_published.load(),ui_material_rejected.load(),ui_material_execute_calls.load(),ui_material_claimed.load(),ui_material_content_rejected.load(),ui_material_bad_return.load(),ui_material_gpu_candidates.load(),ui_material_gpu_rejected.load(),ui_material_disabled?1:0);
 for(unsigned i=0;i<ui_material_row_count;++i){auto const& row=ui_material_rows[i];
  fprintf(status,"ui_material_gpu_row row=%u root_invocation=%llu kind=%u payload=%p native_target=%p source=%p caller=%p indexed=%u ticket_candidate=%llu list=%p resource=%p command_completed=%u gpu_ia_verified=0 allocation_identity_proven=0 mutation_allowed=0\n",i,row.invocation,row.kind,reinterpret_cast<void*>(row.payload),reinterpret_cast<void*>(row.native_target),reinterpret_cast<void*>(row.source),reinterpret_cast<void*>(row.caller),row.indexed?1:0,row.ticket,reinterpret_cast<void*>(row.list),reinterpret_cast<void*>(row.resource),row.command_completed?1:0);
 }
}

