#pragma once
#include "owned_scene_trial.h"
#include "owned_scene_pipeline.h"
#include "native_render_size.h"
#include "owned_scene_retirement.h"
#if defined(GEARS_NATIVE_OWNED_SCENE_PROBE) && GEARS_NATIVE_OWNED_SCENE_PROBE
#include "native_owned_surface.inl"
void owned_ui_discard_completed(uint64_t ticket);
namespace gears_owned_scene_live {
gears_owned_scene::Pipeline pipeline;
gears_owned_scene::Trial& trial=pipeline.active;
#if defined(GEARS_OWNED_PENDING_PIPELINE) && GEARS_OWNED_PENDING_PIPELINE
constexpr bool pending_pipeline=true;
#else
constexpr bool pending_pipeline=false;
#endif
bool pipeline_failed{};unsigned pipeline_failure_reason{};
uint64_t pending_selections{},pending_promotions{},pending_rejections{};
gears_native_owned::Owner<gears_native_owned::NativeOps> owner;
std::unique_ptr<gears_native_owned::NativeOps> ops;
bool image_verified{},attempted{};
gears_owned_retirement::Gate shutdown;
gears_owned_retirement::Handoff handoff;
uint64_t resource_world{};
bool shutdown_conflict{};
uint64_t pending_fallbacks{},pending_fallback_ticket{};unsigned pending_fallback_mask{};
std::array<uint64_t,6> pending_conditions{};
struct FallbackConsume {
 uintptr_t destination{},result{};uint64_t constructor_ordinal{},old_ticket{},world{},born{};
 long long constructor_qpc{},return_qpc{},consume_qpc{};unsigned constructor_thread{},consume_thread{},row{},pointer_occurrence{};
 bool return_seen{},return_verified{},consumed{},ambiguous{},return_verified_at_consume{};
 uint64_t consume_sequence{},input_ticket{},active_ticket{},active_world{},active_born{},committed_world{},tracker_world{},last_consumed{};
 bool complete{},can_advance{},ui_ready{},identity_match{};unsigned candidate_matches{},outcome{};
};
std::array<FallbackConsume,64> fallback_rows{};unsigned fallback_row_count{};
uint64_t fallback_overflow{},fallback_consume_sequence{},fallback_ready{},fallback_notready{},fallback_mismatch{},fallback_unverified{},fallback_repeat{},fallback_return_mismatch{};
std::atomic<DWORD> consumer_thread{};std::atomic<bool> consumer_moved{};
std::atomic<uint64_t> pacing_waits{},pacing_ready{},pacing_timeouts{},pacing_changed{},pacing_elapsed_ns{},pacing_max_ns{},pacing_overshoots{};
std::atomic<unsigned> pacing_consecutive_timeouts{};
std::atomic<bool> shutdown_rhi_ready{};
gears_native_owned::Snapshot created{};
bool healthy(){const auto live=owner.snapshot();
 const bool released=shutdown.phase()==gears_owned_retirement::Phase::released&&live.phase==gears_native_owned::Phase::released;
 if(trial.resource&&(live.adapter_fault||(!released&&live.phase!=gears_native_owned::Phase::live)))trial.error=10;
 return !trial.error;
}
}
void initialize_native_owned_scene(bool image_verified){gears_owned_scene_live::image_verified=image_verified&&gears_native_render_size::initialize();
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
 owned_scene_gpu::enabled=gears_owned_scene_live::image_verified;
#endif
}
bool native_owned_scene_tick(uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> native_guard(session_native_mutex);
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(handoff.armed()){
 if(shutdown_conflict||pipeline.pending.ticket||!handoff.can_recreate(committed_world_generation.load(),ticket,submitted_lifetime.stopping()))return false;
  owned_scene_gpu::CreateScope scope;
  const bool ok=owner.recreate_on_rhi(*ops,gears_native_render_size::width(),gears_native_render_size::height(),ticket);created=owner.snapshot();
  if(!ok){trial.error=8;return false;}
  owned_scene_gpu::accept(owner.borrow_resource(),ticket);
  if(!pipeline.reset_resource()){trial.error=9;return false;}
  if(!trial.ready(canonical_identity(owner.borrow_resource()),reinterpret_cast<uintptr_t>(owner.borrow_adapter()),ticket)||
     !handoff.recreated(shutdown,committed_world_generation.load(),ticket)){trial.error=9;return false;}
  resource_world=committed_world_generation.load();owned_scene_gpu::handoff_recreated();return true;
 }
 if(attempted)return !trial.error;attempted=true;
 if(!image_verified){trial.error=13;return false;}
 ops=std::make_unique<gears_native_owned::NativeOps>(base,safe_copy,image_verified);
 owned_scene_gpu::CreateScope scope;
 const bool ok=owner.create_on_rhi(*ops,gears_native_render_size::width(),gears_native_render_size::height(),ticket);created=owner.snapshot();
 if(!ok){trial.error=8;return false;}resource_world=committed_world_generation.load();
 owned_scene_gpu::accept(owner.borrow_resource(),ticket);
 if(!trial.ready(canonical_identity(owner.borrow_resource()),reinterpret_cast<uintptr_t>(owner.borrow_adapter()),ticket)){
  trial.error=9;return false;}return true;
}
uintptr_t native_owned_scene_candidate(){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(shutdown.stopping()||submitted_lifetime.stopping()||pipeline_failed)return 0;
 if(!pipeline.pending.ticket&&trial.complete()&&healthy()&&owned_scene_gpu::can_advance(trial.ticket)){
  const auto completed_ticket=trial.ticket;
  pipeline.output_boundary(completed_ticket); // CPU and exact output proofs now both hold.
  if(!trial.advance(completed_ticket,true)){trial.error=12;return 0;}
  // The queued pipeline and active proof must advance together when overlap
  // is disabled after a consumer-thread change.
  if(pipeline.active.ticket&&
     (pipeline.active.ticket!=completed_ticket||!pipeline.active.advance(completed_ticket,true))){
   native_owned_pipeline_failure(5);return 0;}
 }
 if(!healthy())return 0;
 if(trial.candidate())return trial.adapter;
 return pending_pipeline&&pipeline.output_observed&&trial.ticket&&!pipeline.pending.ticket&&!handoff.requested()&&consumer_thread.load()&&!consumer_moved.load()&&
   resource_world==committed_world_generation.load()&&owned_ui_can_bind_frame()?trial.adapter:0;}
void native_owned_scene_pending_fallback(void* destination){using namespace gears_owned_scene_live;
 // Called under frame_hook_mutex at the exact pending-ticket fallback branch.
 unsigned mask=(!trial.returned?1u:0u)|(!trial.destroyed?2u:0u)|
    (trial.draw_mask!=3?4u:0u)|(trial.submitted_mask!=3?8u:0u)|
    (!owned_scene_gpu::can_advance(trial.ticket)?16u:0u)|(!owned_ui_ready(trial.ticket)?32u:0u);
 ++pending_fallbacks;pending_fallback_ticket=trial.ticket;pending_fallback_mask=mask;
 for(unsigned i=0;i<pending_conditions.size();++i)if(mask&(1u<<i))++pending_conditions[i];
 const auto pointer=reinterpret_cast<uintptr_t>(destination);unsigned occurrence=1;bool ambiguous=false;
 for(unsigned i=0;i<fallback_row_count;++i){auto& previous=fallback_rows[i];if(previous.destination!=pointer)continue;
  ++occurrence;if(!previous.consumed){previous.ambiguous=true;ambiguous=true;}}
 if(fallback_row_count==fallback_rows.size()){++fallback_overflow;return;}
 auto& row=fallback_rows[fallback_row_count];row.row=++fallback_row_count;
 row.destination=pointer;row.constructor_ordinal=pending_fallbacks;row.old_ticket=trial.ticket;
 row.world=resource_world;row.born=trial.born;row.constructor_qpc=now();row.constructor_thread=GetCurrentThreadId();
 row.pointer_occurrence=occurrence;row.ambiguous=ambiguous;
}
void native_owned_scene_fallback_built(void* destination,void* result){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 FallbackConsume* matched{};unsigned matches{};const auto pointer=reinterpret_cast<uintptr_t>(destination);
 const auto thread=GetCurrentThreadId();
 for(unsigned i=0;i<fallback_row_count;++i){auto& row=fallback_rows[i];
  if(row.destination==pointer&&row.constructor_thread==thread&&!row.return_seen){matched=&row;++matches;}}
 if(!matches)return;if(matches!=1){++fallback_return_mismatch;return;}
 matched->return_seen=true;matched->return_qpc=now();matched->result=reinterpret_cast<uintptr_t>(result);
 matched->return_verified=pointer&&result==destination;
 if(!matched->return_verified)++fallback_return_mismatch;
}
void native_owned_scene_fallback_consume(void* renderer,uint64_t input_ticket){using namespace gears_owned_scene_live;
 // Called under frame_hook_mutex, before trial.begin can change its state.
 if(!renderer||input_ticket||!fallback_row_count)return;
 const auto pointer=reinterpret_cast<uintptr_t>(renderer);unsigned matches{};bool repeated=false;
 for(unsigned i=0;i<fallback_row_count;++i)if(fallback_rows[i].destination==pointer){
  if(!fallback_rows[i].consumed)++matches;else repeated=true;}
 if(!matches){if(repeated)++fallback_repeat;return;}
 const auto sequence=++fallback_consume_sequence;const auto stamp=now();const auto thread=GetCurrentThreadId();
 const bool complete=trial.complete(),can_advance=owned_scene_gpu::can_advance(trial.ticket),ui_ready=owned_ui_ready(trial.ticket);
 for(unsigned i=0;i<fallback_row_count;++i){auto& row=fallback_rows[i];
  if(row.destination!=pointer||row.consumed)continue;
  row.consumed=true;row.consume_sequence=sequence;row.consume_qpc=stamp;row.consume_thread=thread;
  row.input_ticket=input_ticket;row.active_ticket=trial.ticket;row.active_world=resource_world;row.active_born=trial.born;
  row.committed_world=committed_world_generation.load();row.tracker_world=frame_link.world_epoch();
  row.last_consumed=owned_scene_gpu::last_consumed_ticket;row.complete=complete;row.can_advance=can_advance;row.ui_ready=ui_ready;
  row.candidate_matches=matches;row.return_verified_at_consume=row.return_seen&&row.return_verified;
  row.identity_match=matches==1&&!row.ambiguous&&row.old_ticket==trial.ticket&&row.world==resource_world&&row.born==trial.born&&
   row.world==row.committed_world&&row.world==row.tracker_world;
  if(!row.return_verified_at_consume){row.outcome=4;++fallback_unverified;}
  else if(!row.identity_match){row.outcome=3;++fallback_mismatch;}
  else if(complete&&can_advance){row.outcome=1;++fallback_ready;}
  else {row.outcome=2;++fallback_notready;}
 }
}
void native_owned_scene_progress(FILE* output){using namespace gears_owned_scene_live;
 uint64_t count{},ticket{};unsigned mask{};std::array<uint64_t,6> conditions{};
 unsigned rows{};uint64_t overflow{},ready{},notready{},mismatch{},unverified{},repeat{},return_mismatch{};
 {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
  count=pending_fallbacks;ticket=pending_fallback_ticket;mask=pending_fallback_mask;conditions=pending_conditions;
  rows=fallback_row_count;overflow=fallback_overflow;ready=fallback_ready;notready=fallback_notready;mismatch=fallback_mismatch;
  unverified=fallback_unverified;repeat=fallback_repeat;return_mismatch=fallback_return_mismatch;}
 fprintf(output,"owned_pending_fallbacks=%llu owned_pending_ticket=%llu owned_pending_mask=%u owned_pending_return=%llu owned_pending_destroy=%llu owned_pending_draw=%llu owned_pending_submit=%llu owned_pending_consume=%llu owned_pending_ui=%llu ",
  count,ticket,mask,conditions[0],conditions[1],conditions[2],conditions[3],conditions[4],conditions[5]);
 uint64_t selected{},promoted{},rejected{},pending{};bool failed{};unsigned failure{};
 {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);selected=pending_selections;promoted=pending_promotions;
  rejected=pending_rejections;pending=pipeline.pending.ticket;failed=pipeline_failed;failure=pipeline_failure_reason;}
 fprintf(output,"owned_pipeline_enabled=%u owned_pipeline_selected=%llu owned_pipeline_promoted=%llu owned_pipeline_rejected=%llu owned_pipeline_pending_ticket=%llu owned_pipeline_failed=%u owned_pipeline_failure_reason=%u ",
  pending_pipeline?1u:0u,selected,promoted,rejected,pending,failed?1u:0u,failure);
 fprintf(output,"owned_fallback_rows=%u owned_fallback_overflow=%llu owned_fallback_consume_count=%llu owned_fallback_consume_ready=%llu owned_fallback_consume_notready=%llu owned_fallback_consume_mismatch=%llu owned_fallback_consume_unverified=%llu owned_fallback_consume_repeat=%llu owned_fallback_return_mismatch=%llu ",
  rows,overflow,ready+notready+mismatch+unverified,ready,notready,mismatch,unverified,repeat,return_mismatch);
 fprintf(output,"owned_pacing_waits=%llu owned_pacing_ready=%llu owned_pacing_timeouts=%llu owned_pacing_changed=%llu owned_pacing_elapsed_ns=%llu owned_pacing_max_ns=%llu owned_pacing_overshoots=%llu owned_pacing_consecutive_timeouts=%u ",
  pacing_waits.load(),pacing_ready.load(),pacing_timeouts.load(),pacing_changed.load(),pacing_elapsed_ns.load(),pacing_max_ns.load(),pacing_overshoots.load(),pacing_consecutive_timeouts.load());
}
void native_owned_scene_pace(){
#if defined(GEARS_OWNED_FRAME_PACING) && GEARS_OWNED_FRAME_PACING
 using namespace gears_owned_scene_live;
 if(self_testing||!continuous_session||!stereo_xr||!stereo_gpu_publish||!recording.load()||
    session_build_depth||consuming_renderer||consuming_frame_ticket||!consumer_thread.load()||consumer_thread.load()==GetCurrentThreadId()||
    consumer_moved.load()||pacing_consecutive_timeouts.load()>=3)return;
 uint64_t ticket{},world{},born{};bool waiting=false;
 const auto start=gears_vr::monotonic_ns();constexpr int64_t budget=16000000;
 unsigned outcome=0; // 1 ready, 2 timeout, 3 changed/cancelled.
 for(;;){
  if(submitted_lifetime.stopping()||world_quiesce_requested.load()||consumer_moved.load()||
     !session_activation_gate.ready()){outcome=3;break;}
  {
   // Never block on a mod lock while waiting for the RHI thread. In particular
   // the RHI create/destroy paths must be able to enter session_native_mutex.
   std::unique_lock<std::recursive_mutex> session(session_native_mutex,std::try_to_lock);
   std::unique_lock<FrameHookMutex> frame(frame_hook_mutex,std::defer_lock);
   if(session.owns_lock())frame.try_lock();
   if(frame.owns_lock()){
    if(!trial.ticket||trial.error||shutdown.stopping()||handoff.requested()){outcome=3;break;}
    if(!waiting){ticket=trial.ticket;world=resource_world;born=trial.born;
      if(trial.complete()&&owned_scene_gpu::can_advance(ticket))return;
      waiting=true;++pacing_waits;
    }
    if(ticket!=trial.ticket||world!=resource_world||world!=committed_world_generation.load()||
       world!=frame_link.world_epoch()||born!=trial.born){outcome=3;break;}
    if(trial.complete()&&owned_scene_gpu::can_advance(ticket)){outcome=1;break;}
   }
  }
  if(!waiting)return; // No admitted ticket: never wait on initial lock contention.
  if(gears_vr::monotonic_ns()-start>=budget){outcome=2;break;}
  Sleep(1); // Scheduling can overshoot; actual duration is reported below.
 }
 if(!waiting)return;
 const auto elapsed=uint64_t((std::max)(int64_t(0),gears_vr::monotonic_ns()-start));
 pacing_elapsed_ns+=elapsed;auto maximum=pacing_max_ns.load();
 while(elapsed>maximum&&!pacing_max_ns.compare_exchange_weak(maximum,elapsed)){}
 if(elapsed>uint64_t(budget))++pacing_overshoots;
 if(outcome==1){++pacing_ready;pacing_consecutive_timeouts=0;}
 else if(outcome==2){++pacing_timeouts;++pacing_consecutive_timeouts;}
 else ++pacing_changed;
#endif
}
bool native_owned_scene_try_bootstrap(){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
 const auto world=committed_world_generation.load();const auto life=submitted_lifetime.snapshot();
 if(shutdown_conflict||pipeline.pending.ticket||trial.error||!handoff.armed()||submitted_lifetime.stopping()||
    world!=handoff.next_world()||frame_link.world_epoch()!=world||life.generation!=world||life.closed||
    life.stop_requested||life.permanent_closed||life.quiescing||life.pending||
    owner.snapshot().phase!=gears_native_owned::Phase::released||owner.retained()||
    !owned_scene_gpu::handoff_rearmed||owned_scene_gpu::handoff_failed()||owned_scene_gpu::tracker.world_epoch()!=world||
    owned_scene_gpu::tracker.owned_status().enabled)return false;
 gears_owned_retirement::Handoff::Receipt receipt;
 if(!handoff.receipt(shutdown,false,owned_scene_gpu::handoff_retired(handoff.identity()),receipt)||!handoff.current(shutdown,receipt))return false;
 return handoff.claim_bootstrap(shutdown,world,false);
#else
 return false;
#endif
}
bool native_owned_scene_select(void* renderer,uint64_t ticket,uintptr_t adapter){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 const bool ok=pipeline.select(reinterpret_cast<uintptr_t>(renderer),ticket,adapter,
   pending_pipeline&&consumer_thread.load()&&!consumer_moved.load()&&owned_ui_can_bind_frame());
 if(!ok&&pending_pipeline&&trial.ticket){++pending_rejections;native_owned_pipeline_failure(3);}
 if(ok){if(pipeline.pending_match(reinterpret_cast<uintptr_t>(renderer),ticket))++pending_selections;owned_scene_gpu::selected(ticket);}return ok;}
bool native_owned_scene_built(void* renderer,uint64_t ticket,bool verified){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);uintptr_t target{};
 const bool read=safe_copy(&target,static_cast<unsigned char*>(renderer)+0x18,8);
 owned_scene_gpu::built(ticket);
 return pipeline.construction(reinterpret_cast<uintptr_t>(renderer),ticket,verified&&read&&healthy(),target);}
bool native_owned_scene_begin(void* renderer,uint64_t ticket,uint64_t world,bool nested){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 native_owned_scene_fallback_consume(renderer,ticket);
 const auto address=reinterpret_cast<uintptr_t>(renderer);
 if(pending_pipeline&&((pipeline.pending.ticket&&pipeline.pending.renderer==address&&pipeline.pending.ticket!=ticket)||
    (trial.ticket&&!trial.destroyed&&trial.renderer==address&&trial.ticket!=ticket))){
  ++pending_rejections;native_owned_pipeline_failure(4);return false;
 }
 if(pending_pipeline&&pipeline.pending_match(reinterpret_cast<uintptr_t>(renderer),ticket)){
  // A pending renderer may already exist when the game's consumer moves to a
  // different RHI thread. The old output/order proof remains required; retire
  // this one pending renderer and disable further overlap after the move.
  const auto previous_consumer=consumer_thread.load();
  const bool moved=previous_consumer&&previous_consumer!=GetCurrentThreadId();
  const bool identity=!nested&&previous_consumer&&
    world==resource_world&&world==committed_world_generation.load()&&world==frame_link.world_epoch()&&healthy()&&
    owner.snapshot().phase==gears_native_owned::Phase::live&&!shutdown.stopping()&&!handoff.requested();
  const auto result=identity?pipeline.promote(reinterpret_cast<uintptr_t>(renderer),ticket,
    owned_scene_gpu::can_advance(trial.ticket)):gears_owned_scene::Promotion::identity_mismatch;
  if(result!=gears_owned_scene::Promotion::promoted){++pending_rejections;
    native_owned_pipeline_failure(result==gears_owned_scene::Promotion::identity_mismatch?2u:1u);return false;}
  ++pending_promotions;
  if(moved){consumer_moved=true;consumer_thread=GetCurrentThreadId();}
 }
 const bool ok=healthy()&&trial.begin(reinterpret_cast<uintptr_t>(renderer),ticket);
 if(ok&&trial.renderer==reinterpret_cast<uintptr_t>(renderer)&&trial.ticket==ticket){
  DWORD empty{};const auto thread=GetCurrentThreadId();
  if(!consumer_thread.compare_exchange_strong(empty,thread)&&empty!=thread)consumer_moved=true;
  owned_scene_gpu::began(ticket);owned_scene_gpu::activate();}return ok;}
void native_owned_scene_copy(){using namespace gears_owned_scene_live;
 if(shutdown.stopping())return;
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
 const auto consumed_before=owned_scene_gpu::consumed_count,published_before=owned_scene_gpu::published_count;
#endif
 owned_scene_gpu::copy(healthy()&&trial.returned&&trial.destroyed&&trial.submitted_mask==3&&!trial.unknown_draws&&owned_ui_ready(trial.ticket),trial.ticket);
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
 if(owned_scene_gpu::consumed_count>consumed_before){
  pipeline.output_boundary(trial.ticket); // An incomplete CPU tail keeps overlap disabled.
  if(owned_scene_gpu::published_count>published_before&&owned_scene_gpu::latest_publication.ticket==trial.ticket)
   owned_ui_published(trial.ticket);
  else owned_ui_discard_completed(trial.ticket);
 }
#endif
}
bool native_owned_scene_consuming(void* renderer,uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 return trial.resource&&!trial.error&&trial.entered&&!trial.returned&&ticket&&
  trial.ticket==ticket&&trial.renderer==reinterpret_cast<uintptr_t>(renderer);}
void native_owned_scene_end(void* renderer,uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(auto* row=pipeline.find(reinterpret_cast<uintptr_t>(renderer),ticket))row->end(reinterpret_cast<uintptr_t>(renderer),ticket);
 if(trial.renderer==reinterpret_cast<uintptr_t>(renderer)&&trial.ticket==ticket)owned_scene_gpu::returned(ticket);native_owned_scene_copy();}
void native_owned_scene_destroyed(void* renderer,uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(auto* row=pipeline.find(reinterpret_cast<uintptr_t>(renderer),ticket))row->retire(reinterpret_cast<uintptr_t>(renderer));
 if(trial.renderer==reinterpret_cast<uintptr_t>(renderer)&&trial.ticket==ticket)owned_scene_gpu::destroyed(trial.ticket);native_owned_scene_copy();}
void native_owned_scene_draw(ID3D12GraphicsCommandList* list,gears_viewport::TargetDraw const& binding,
 uintptr_t caller,unsigned kind,unsigned elements,unsigned instances){using namespace gears_owned_scene_live;
 if(shutdown.phase()==gears_owned_retirement::Phase::released)return;
 if(!trial.resource||binding.target.resource!=trial.resource)return;
 if(shutdown.stopping()){shutdown_conflict=true;return;}
 const auto state=owned_bindings_enabled?owned_target_stats.draw_state(canonical_identity(list)):
  frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list));
 const bool exact=final_eye.valid&&binding.targets==1&&!binding.target.mip&&final_eye.width==gears_native_render_size::width()&&final_eye.height==gears_native_render_size::height()&&
  binding.target.width==gears_native_render_size::width()&&binding.target.height==gears_native_render_size::height()&&verified_stereo_draw(
   state,final_eye,caller,kind,elements,instances);
 trial.draw(binding.target.resource,owned_scene_gpu::list_ref(canonical_identity(list)),consuming_frame_ticket,final_eye.eye,exact);
 if(exact){owned_scene_gpu::retain_image();owned_scene_gpu::drew(consuming_frame_ticket,final_eye.eye);}
 owned_scene_gpu::draw(list,binding.target.resource,exact,final_eye.eye,final_eye.rect,consuming_frame_ticket);
}
gears_owned_scene::Receipt native_owned_scene_prepare(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* members){
 using namespace gears_owned_scene_live;if(!trial.ticket||trial.error)return {};
 std::array<ID3D12CommandList*,gears_frame_link::Tracker::list_capacity> lists{};
 std::array<gears_frame_link::ListRef,gears_frame_link::Tracker::list_capacity> refs{};
 if(!count||count>lists.size()||!safe_copy(lists.data(),members,count*sizeof(lists[0])))return {};
 for(unsigned i=0;i<count;++i)refs[i]=owned_scene_gpu::list_ref(canonical_identity(lists[i]));
 return trial.prepare(canonical_identity(queue),queue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_DIRECT,refs.data(),count);
}
void native_owned_scene_commit(gears_owned_scene::Receipt const& receipt){using namespace gears_owned_scene_live;
 trial.commit(receipt,[](uintptr_t identity){return owned_scene_gpu::list_ref(identity);});
 if(trial.submitted_mask==3)owned_scene_gpu::submitted(trial.ticket);native_owned_scene_copy();}
bool native_owned_scene_retained(){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 return attempted&&(owner.retained()||(handoff.requested()&&!owned_scene_gpu::handoff_retired(handoff.identity())));}
// Caller holds session_native_mutex: native_cpu_drained includes closed entry
// admission, no pending submitted renderers/state, and no producer build scope.
// Canvas is independently derived from every active owned UI ledger receipt.
bool native_owned_scene_shutdown_worker(bool native_cpu_drained,bool canvas_drained,bool bridges_closed){
 using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(!attempted||(!owner.retained()&&(!handoff.requested()||owned_scene_gpu::handoff_retired(handoff.identity()))))return true;
 if(shutdown_conflict)return false;
 if(pipeline.pending.ticket)return false; // Native queued renderer must drain before resource retirement.
 if(handoff.requested()){
  if(shutdown.phase()==gears_owned_retirement::Phase::cpu_drained&&
     owned_scene_gpu::prepare_handoff(handoff.identity().world,handoff.identity().ticket_cutoff)&&owned_scene_gpu::drain_handoff(bridges_closed))
   if(shutdown.gpu_drained(created.generation,true,bridges_closed,true))shutdown_rhi_ready.store(true);
  return shutdown.phase()==gears_owned_retirement::Phase::released&&!owner.retained();
 }
 if(!shutdown.stopping()){
  if(!native_cpu_drained||!canvas_drained||(trial.ticket&&(!trial.returned||!trial.destroyed)))return false;
  const gears_native_owned::DrainProof proof{created.generation,true,true,true,true,false};
  if(!shutdown.request(proof))return false;
 }
 if(shutdown.phase()==gears_owned_retirement::Phase::cpu_drained&&owned_scene_gpu::begin_shutdown()&&
    owned_scene_gpu::drain_shutdown(bridges_closed))
  if(shutdown.gpu_drained(created.generation,true,bridges_closed,true))shutdown_rhi_ready.store(true);
 return shutdown.phase()==gears_owned_retirement::Phase::released&&!owner.retained();
}
void native_owned_scene_shutdown_rhi(){using namespace gears_owned_scene_live;
 thread_local bool servicing{};
 if(servicing||!shutdown_rhi_ready.load())return;
 std::lock_guard<std::recursive_mutex> native_guard(session_native_mutex);
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(shutdown.phase()!=gears_owned_retirement::Phase::gpu_drained||shutdown_conflict||GetCurrentThreadId()!=created.creation_thread)return;
 if(handoff.requested()&&!owned_scene_gpu::handoff_current()){shutdown_conflict=true;return;}
 servicing=true;if(shutdown.retire_on_rhi(owner)){
  if(handoff.requested()&&!owned_scene_gpu::finish_handoff(handoff.identity()))shutdown_conflict=true;
  shutdown_rhi_ready.store(false);
 }servicing=false;
}
uint64_t native_owned_scene_resource_generation(){
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);return gears_owned_scene_live::created.generation;
}
bool native_owned_scene_handoff_prepare(uint64_t old_world,uint64_t cutoff,bool native_cpu_drained,bool canvas_drained){
 using namespace gears_owned_scene_live;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(submitted_lifetime.stopping()||shutdown_conflict||pipeline.pending.ticket||!attempted||resource_world!=old_world||
    old_world!=committed_world_generation.load()||!native_cpu_drained||!canvas_drained||
    cutoff<created.generation||cutoff<trial.ticket||cutoff<trial.last_ticket||
    (trial.ticket&&(!trial.returned||!trial.destroyed)))return false;
 if(handoff.requested())return handoff.identity().world==old_world&&handoff.identity().ticket_cutoff==cutoff&&
  owned_scene_gpu::prepare_handoff(old_world,cutoff);
 if(!owner.retained())return false;
 const gears_owned_retirement::ResourceIdentity identity{old_world,created.generation,cutoff,trial.resource,owned_scene_gpu::handoff_queue_identity()};
 if(!handoff.begin(shutdown,identity,{created.generation,true,true,true,true,false}))return false;
 return owned_scene_gpu::prepare_handoff(old_world,cutoff);
}
bool native_owned_scene_handoff_step(bool bridge_sources_retired){
 using namespace gears_owned_scene_live;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(!handoff.requested()||shutdown_conflict)return false;
 if(shutdown.phase()==gears_owned_retirement::Phase::cpu_drained&&
    owned_scene_gpu::prepare_handoff(handoff.identity().world,handoff.identity().ticket_cutoff)&&owned_scene_gpu::drain_handoff(bridge_sources_retired))
  if(shutdown.gpu_drained(created.generation,true,bridge_sources_retired,true))shutdown_rhi_ready.store(true);
 return shutdown.phase()==gears_owned_retirement::Phase::released&&!owner.retained()&&owned_scene_gpu::handoff_retired(handoff.identity());
}
bool native_owned_scene_handoff_failed(){
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 return gears_owned_scene_live::shutdown_conflict||owned_scene_gpu::handoff_failed()||
  (gears_owned_scene_live::handoff.requested()&&gears_owned_scene_live::trial.error);
}
bool native_owned_scene_handoff_receipt(gears_owned_retirement::Handoff::Receipt& receipt){
 using namespace gears_owned_scene_live;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 return !shutdown_conflict&&handoff.receipt(shutdown,owner.retained(),owned_scene_gpu::handoff_retired(handoff.identity()),receipt);
}
bool native_owned_scene_handoff_current(gears_owned_retirement::Handoff::Receipt const& receipt){
 using namespace gears_owned_scene_live;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 return !shutdown_conflict&&!owner.retained()&&handoff.current(shutdown,receipt)&&owned_scene_gpu::handoff_retired(receipt.identity());
}
bool native_owned_scene_handoff_arm(gears_owned_retirement::Handoff::Receipt const& receipt,uint64_t next_world){
 using namespace gears_owned_scene_live;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 if(submitted_lifetime.stopping()||frame_link.world_epoch()!=next_world||!native_owned_scene_handoff_current(receipt))return false;
 if(!handoff.arm(shutdown,receipt,next_world,false)||!owned_scene_gpu::rearm_handoff(receipt.identity(),next_world))return false;
 shutdown_rhi_ready.store(false);return true;
}
void print_native_owned_scene(FILE* file){using namespace gears_owned_scene_live;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 owned_scene_gpu::poll();owned_scene_gpu::print(file);
 fprintf(file,"owned_fallback_consume_policy capacity=64 append_only=1 association=unique_destination_and_constructor_return constructor_qpc_scope=pending_fallback_before_original outcome=ready:1,notready:2,identity_mismatch:3,return_unverified:4 ready_requires=complete_and_can_advance ui_ready_is_independent_after_publication=1\n");
 for(unsigned i=0;i<fallback_row_count;++i){const auto& row=fallback_rows[i];
  fprintf(file,"owned_fallback_consume row=%u constructor_ordinal=%llu destination=%llx constructor_qpc=%lld constructor_thread=%u pointer_occurrence=%u ambiguous=%u old_ticket=%llu old_world=%llu old_born=%llu return_seen=%u return_qpc=%lld result=%llx constructor_return_verified=%u consumed=%u matched_row=%u candidate_matches=%u consume_sequence=%llu consume_qpc=%lld consume_thread=%u qpc_frequency=%lld return_verified_at_consume=%u input_ticket=%llu active_ticket=%llu active_world=%llu active_born=%llu committed_world=%llu tracker_world=%llu complete=%u can_advance=%u ui_ready=%u last_consumed=%llu identity_match=%u outcome=%u lifetime_proven=0\n",
   row.row,row.constructor_ordinal,row.destination,row.constructor_qpc,row.constructor_thread,row.pointer_occurrence,unsigned(row.ambiguous),row.old_ticket,row.world,row.born,
   unsigned(row.return_seen),row.return_qpc,row.result,unsigned(row.return_verified),unsigned(row.consumed),row.consumed?row.row:0,row.candidate_matches,
   row.consume_sequence,row.consume_qpc,row.consume_thread,qpc_frequency,unsigned(row.return_verified_at_consume),row.input_ticket,row.active_ticket,row.active_world,row.active_born,row.committed_world,row.tracker_world,
   unsigned(row.complete),unsigned(row.can_advance),unsigned(row.ui_ready),row.last_consumed,unsigned(row.identity_match),row.outcome);
 }
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
 const auto& size=gears_native_render_size::packet;
 fprintf(file,"runtime_render_size loaded=%u session=%llu scale=%.6g recommended=%u,%u eye=%u,%u source=%u,%u game_resolution_independent=1 dimensions_are_plan_not_gpu_proof=1\n",
  unsigned(gears_native_render_size::loaded),size.session,size.plan.scale,size.plan.recommended_width,size.plan.recommended_height,
  size.plan.eye_width,size.plan.eye_height,size.plan.source_width,size.plan.source_height);
#endif
 const auto bindings=owned_target_stats.snapshot();
 fprintf(file,"owned_bindings strict=1 resets=%llu overflow=%llu descriptor_evictions=%llu descriptor_lost=%u\n",
  bindings.resets,bindings.overflow,bindings.descriptor_evictions,owned_target_stats.descriptor_capacity_lost()?1:0);
 healthy();const auto live=owner.snapshot();
 fprintf(file,"native_owned_bootstrap armed=%u claimed=%u next_world=%llu minimum_generation=%llu\n",
  unsigned(handoff.armed()),unsigned(handoff.bootstrap_claimed()),handoff.next_world(),handoff.minimum_generation());
 fprintf(file,"native_owned_scene attempted=%u resource=%llx adapter=%llx born=%llu ticket=%llu renderer=%llx width=%u height=%u built=%u entered=%u returned=%u destroyed=%u draw_mask=%u submitted_mask=%u draws=%llu unknown_draws=%llu submissions=%llu queue=%llx error=%u create_error=%u live_phase=%u live_error=%u adapter_fault=%u retained=%u retention_until_process_exit=%u shutdown_phase=%u shutdown_conflict=%u canvas_redirected=0 gpu_state_certified=%u pixels_read_back=%u pixels_verified=0\n",
  attempted?1:0,trial.resource,trial.adapter,trial.born,trial.ticket,trial.renderer,created.width,created.height,
  trial.built?1:0,trial.entered?1:0,trial.returned?1:0,trial.destroyed?1:0,trial.draw_mask,trial.submitted_mask,
  trial.draws,trial.unknown_draws,trial.submissions,trial.queue,trial.error,unsigned(created.error),unsigned(live.phase),unsigned(live.error),live.adapter_fault?1:0,owner.retained()?1:0,
  owner.retained()?1:0,unsigned(shutdown.phase()),shutdown_conflict?1:0,
  owned_scene_gpu::state_certified()?1:0,owned_scene_gpu::pixels_read_back()?1:0);
}
#else
void initialize_native_owned_scene(bool){}
bool native_owned_scene_tick(uint64_t){return true;}
uintptr_t native_owned_scene_candidate(){return 0;}
void native_owned_scene_pending_fallback(void*){}
void native_owned_scene_fallback_built(void*,void*){}
void native_owned_scene_progress(FILE*){}
void native_owned_scene_pace(){}
bool native_owned_scene_try_bootstrap(){return false;}
bool native_owned_scene_select(void*,uint64_t,uintptr_t){return false;}
bool native_owned_scene_built(void*,uint64_t,bool){return true;}
bool native_owned_scene_begin(void*,uint64_t,uint64_t,bool){return true;}
bool native_owned_scene_consuming(void*,uint64_t){return false;}
void native_owned_scene_end(void*,uint64_t){}
void native_owned_scene_destroyed(void*,uint64_t){}
void native_owned_scene_draw(ID3D12GraphicsCommandList*,gears_viewport::TargetDraw const&,uintptr_t,unsigned,unsigned,unsigned){}
gears_owned_scene::Receipt native_owned_scene_prepare(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){return {};}
void native_owned_scene_commit(gears_owned_scene::Receipt const&){}
bool native_owned_scene_retained(){return false;}
bool native_owned_scene_shutdown_worker(bool,bool,bool){return true;}
void native_owned_scene_shutdown_rhi(){}
uint64_t native_owned_scene_resource_generation(){return 0;}
bool native_owned_scene_handoff_prepare(uint64_t,uint64_t,bool,bool){return false;}
bool native_owned_scene_handoff_step(bool){return false;}
bool native_owned_scene_handoff_failed(){return false;}
bool native_owned_scene_handoff_receipt(gears_owned_retirement::Handoff::Receipt&){return false;}
bool native_owned_scene_handoff_current(gears_owned_retirement::Handoff::Receipt const&){return false;}
bool native_owned_scene_handoff_arm(gears_owned_retirement::Handoff::Receipt const&,uint64_t){return false;}
void print_native_owned_scene(FILE*){}
#endif
