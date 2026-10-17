// CPU-only actual-hook contract; dedicated DLL-loader process, never a game probe.
#include <vector>
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
namespace { namespace ui_material_bypass_check {
constexpr uint32_t sentinel=0x76543210;
constexpr uintptr_t upload_sentinel=0x12345678;
struct TestException {unsigned value;};
UiMaterialContext* parent{};void* expected_command{};
unsigned mode{},depth{},calls{},upload_calls{},draw_calls{};bool good=true;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
game_material::ConsumerScope* parent_scope{};
gears_game_material::UploadCertificate* parent_upload{};
#endif
bool tls(){bool ok=ui_material_context==parent;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
 ok=ok&&game_material::current==parent_scope&&game_material::uploading==parent_upload&&
     parent_scope->context==parent&&parent_scope->entered&&parent_scope->drew&&!parent_scope->bad&&parent_scope->ended;
#endif
 return ok;}
void STDMETHODCALLTYPE draw(ID3D12GraphicsCommandList* list,UINT vertices,UINT instances,UINT start,UINT first){
 ++draw_calls;good=good&&stereo_publish_bypass&&tls()&&list==reinterpret_cast<ID3D12GraphicsCommandList*>(0x1234)&&
  vertices==3&&instances==1&&start==0&&first==0;
}
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
uintptr_t __fastcall upload(void* context,unsigned primitive,unsigned primitives,void* vertices,unsigned stride){
 ++upload_calls;good=good&&stereo_publish_bypass&&tls()&&context==reinterpret_cast<void*>(0x111)&&
  primitive==1&&primitives==2&&vertices==reinterpret_cast<void*>(0x222)&&stride==32;return upload_sentinel;
}
#endif
uint32_t __fastcall execute(void* command){
 ++calls;good=good&&command==expected_command;
 if(!mode){good=good&&ui_observer_enabled&&recording.load()&&!stereo_publish_bypass&&
  world_thread.admitted&&world_thread.depth&&active.load()&&!ui_material_context;return 36;}
 good=good&&stereo_publish_bypass&&!world_thread.depth&&!active.load()&&tls();
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
 good=good&&game_material::hook_upload(reinterpret_cast<void*>(0x111),1,2,reinterpret_cast<void*>(0x222),32)==upload_sentinel&&tls();
#endif
 hook_draw(reinterpret_cast<ID3D12GraphicsCommandList*>(0x1234),3,1,0,0);
 if(mode==2&&!depth){++depth;const auto previous=stereo_publish_bypass;
  {StereoPublishBypass nested;good=good&&hook_ui_material_execute(command)==sentinel&&stereo_publish_bypass==previous+1;}
  good=good&&stereo_publish_bypass==previous;--depth;}
 if(mode==3)throw TestException{0x2468};return sentinel;
}
bool same_receipt(gears_ui_material::Receipt const& a,gears_ui_material::Receipt const& b){
 return a.canvas==b.canvas&&a.fcanvas==b.fcanvas&&a.item==b.item&&a.payload==b.payload&&a.proxy==b.proxy&&
  a.key==b.key&&a.kind==b.kind&&a.transform==b.transform&&a.record==b.record;
}
bool same_entry(UiMaterialEntry const& a,UiMaterialEntry const& b){return same_receipt(a.receipt,b.receipt)&&
 a.invocation==b.invocation&&a.occupied==b.occupied&&a.poisoned==b.poisoned&&a.owned_receipt==b.owned_receipt&&a.owned_ticket==b.owned_ticket;}
bool same_context(UiMaterialContext const& a,UiMaterialContext const& b){return
 same_receipt(a.consumer.receipt,b.consumer.receipt)&&a.consumer.command==b.consumer.command&&a.consumer.view==b.consumer.view&&
 a.consumer.family==b.consumer.family&&a.consumer.target==b.consumer.target&&a.invocation==b.invocation&&a.sequence==b.sequence&&
 a.gpu_calls==b.gpu_calls&&a.owned_receipt==b.owned_receipt&&a.owned_ticket==b.owned_ticket&&a.owned_resource==b.owned_resource&&
 a.owned_draws==b.owned_draws&&a.owned_failed==b.owned_failed;}
std::vector<uint64_t> counters(){const auto f=frame_link.snapshot();
 std::vector<uint64_t> v{active.load(),ui_material_sequence.load(),ui_material_published.load(),ui_material_rejected.load(),
  ui_material_execute_calls.load(),ui_material_claimed.load(),ui_material_content_rejected.load(),ui_material_bad_return.load(),
  ui_material_gpu_candidates.load(),ui_material_gpu_rejected.load(),ui_material_before_attempts.load(),ui_material_before_rejected.load(),
  ui_material_reject_count,uint64_t(ui_material_append_reject.recorded),uint64_t(ui_material_disabled),ui_semantic_consumer_pending,
  frame_counts.draws.load(),frame_counts.filtered.load(),frame_counts.unknown.load(),frame_counts.missing_target.load(),
  frame_counts.missing_ticket.load(),frame_counts.executes.load(),frame_counts.presents.load(),
  f.world_epoch,f.reset_calls,f.execute_calls,f.unknown_batches,f.duplicate_batches,f.invalid_batches,f.metadata_batches,
  f.active_lists,f.list_highwater,f.event_highwater,uint64_t(f.capacity_bad),f.overlay_recorded,f.overlay_executed,f.overlay_rejected};
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
 const uint64_t extra[]{game_material::consumers.load(),game_material::completed.load(),game_material::uploads.load(),
  game_material::draws.load(),game_material::ordinary_draws.load(),game_material::hud_draws.load(),game_material::rejected.load(),
  game_material::failed.load(),game_material::pending_presents.load(),game_material::completion.pending(),uint64_t(game_material::completion.blocks_present())};
 v.insert(v.end(),std::begin(extra),std::end(extra));for(auto const& r:game_material::reject_reasons)v.push_back(r.load());
#endif
#if defined(GEARS_OWNED_UI_MATERIAL) && GEARS_OWNED_UI_MATERIAL
 const uint64_t owned[]{owned_material::produced.load(),owned_material::executed.load(),owned_material::completed.load(),
  owned_material::uploads.load(),owned_material::draws.load(),owned_material::faults.load(),owned_material::first_fault.load()};
 v.insert(v.end(),std::begin(owned),std::end(owned));
#endif
 return v;
}
} }
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureUiMaterialBypassCheck(void*) {
 using namespace ui_material_bypass_check;
 if(recording.load()||active.load()||stereo_publish_bypass||ui_material_context||world_thread.depth||
    frame_link.snapshot().reset_calls)return 330;
 {Scope admitted;if(!admitted.world()||!world_thread.admitted||world_thread.depth!=1)return 331;}
 const auto saved_execute=ui_original_material_execute;const auto saved_draw=original_draw;
 const auto saved_base=base;const bool saved_observer=ui_observer_enabled,saved_recording=recording.load();
 const auto saved_entries=ui_material_entries;UiMaterialRestore material_restore;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
 const auto saved_upload=game_material::original_upload;const auto saved_current=game_material::current;
 const auto saved_uploading=game_material::uploading;const auto saved_completion=game_material::completion;
 game_material::ConsumerScope scope(nullptr);gears_game_material::UploadCertificate certificate{};
 scope.context=nullptr;scope.entered=true;scope.drew=true;scope.ended=true;
 parent_scope=&scope;parent_upload=&certificate;
#endif
 struct Restore {UiExecute execute;DrawFn draw;uintptr_t base_value;bool observer,recording_value;
  std::array<UiMaterialEntry,32> entries;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
  game_material::Upload upload;game_material::ConsumerScope* current;gears_game_material::UploadCertificate* uploading;
  gears_game_material::Completion completion;
#endif
  ~Restore(){ui_original_material_execute=execute;original_draw=draw;base=base_value;ui_observer_enabled=observer;
   recording=recording_value;ui_material_entries=entries;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
   game_material::original_upload=upload;game_material::current=current;game_material::uploading=uploading;game_material::completion=completion;
#endif
  }
 } restore{saved_execute,saved_draw,saved_base,saved_observer,saved_recording,saved_entries
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
  ,saved_upload,saved_current,saved_uploading,saved_completion
#endif
 };
 base=0;ui_observer_enabled=true;recording=true;ui_original_material_execute=execute;original_draw=draw;
 UiMaterialContext context{};context.consumer.receipt.payload=0x4000;context.consumer.command=0x3000;
 context.invocation=777;context.sequence=55;context.gpu_calls=3;parent=&context;ui_material_context=parent;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
 scope.context=parent;game_material::current=parent_scope;game_material::uploading=parent_upload;game_material::original_upload=upload;
 if(!game_material::completion.begin())return 332;
#endif
 UiMaterialEntry pending{};pending.receipt.payload=0x4000;pending.receipt.item=0x5000;pending.receipt.fcanvas=0x6000;
 pending.receipt.kind=3;pending.invocation=777;pending.occupied=true;ui_material_entries[0]=pending;
 const auto entries=ui_material_entries;
 const gears_frame_link::Target target{0x9000,640,480};const auto ref=frame_link.reset_success(0x7000);
 if(!ref.generation||!frame_link.draw_final(ref,77,0,target,{0,0,320,480})||
    !frame_link.draw_final(ref,77,1,target,{320,0,320,480})||!frame_link.execute(0x8000,&ref,1))return 333;
 alignas(16) std::array<uintptr_t,5> command{0,0,pending.receipt.payload,0,0};expected_command=command.data();
 const auto context_before=context;const auto before=counters();
 auto unchanged=[&]{if(!tls()||!same_context(context,context_before))return false;
  for(unsigned i=0;i<entries.size();++i)if(!same_entry(ui_material_entries[i],entries[i]))return false;return true;};
 for(mode=1;mode<=3;++mode){bool caught=false;
  try{StereoPublishBypass bypass;if(hook_ui_material_execute(command.data())!=sentinel)return 334;}
  catch(TestException const& e){if(mode!=3||e.value!=0x2468)return 335;caught=true;}
  if((mode==3)!=caught||!good||stereo_publish_bypass||!unchanged()||counters()!=before)return 336+mode;
 }
 if(calls!=4||draw_calls!=4)return 340;
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
 if(upload_calls!=4)return 341;
#endif
 // Observer-on non-bypass control must enter Scope and clear material TLS.
 alignas(16) std::array<uintptr_t,5> invalid{};expected_command=invalid.data();mode=0;
 const auto execute_before=ui_material_execute_calls.load();
 if(hook_ui_material_execute(invalid.data())!=36||!good||!unchanged()||active.load()||world_thread.depth||
    ui_material_execute_calls.load()!=execute_before+1)return 342;
 const auto result=frame_link.present(target.resource,0x8000);
 if(!result.linked||result.contaminated||result.ticket!=77)return 343;
 printf("UI Material bypass actual hooks: pending entry/TLS/completion/frame pair preserved; non36, nested, exception, observer-on control; game_material=%u\n",
#if defined(GEARS_GAME_UI_MATERIAL) && GEARS_GAME_UI_MATERIAL
 1u
#else
 0u
#endif
 );return 0;
}
#else
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureUiMaterialBypassCheck(void*) {
 printf("UI Material bypass check unsupported: material route compiled=0\n");return ERROR_NOT_SUPPORTED;
}
#endif
