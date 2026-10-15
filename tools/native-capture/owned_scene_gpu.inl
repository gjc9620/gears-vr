#pragma once
// Diagnostic only. All calls are serialized by frame_hook_mutex. The tracker
// follows actual Reset generations; it never manufactures a command-list Reset.
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
#include "owned_scene_readback.h"
namespace owned_scene_gpu {
gears_frame_link::Tracker tracker;
struct Creation {uintptr_t resource{},device{};D3D12_RESOURCE_DESC desc{};unsigned state{},api{},uses{};bool legacy{true};unsigned layout{};};
std::array<Creation,8> creations{};
unsigned creation_count{},error{},prebirth_faults{},barriers{},unsupported{},birth_attempts{};
std::array<uint64_t,8> api_calls{},api_scoped{};unsigned device_dispatch_available{},device_dispatch_match{};
enum class Unknown : unsigned { unspecified,descriptor,clear,indirect,bundle,enhanced,render_pass,alias,uav,unreadable,copy,mrt };
std::array<uint64_t,12> unknown_reasons{};
thread_local unsigned creating{};
bool enabled{},finished{},sealed{},birth_ok{};
uintptr_t resource{};uint64_t generation{};
ID3D12Resource* texture{}; // Native Owner retains this typed interface.
ID3D12CommandQueue* queue{}; // Retained for this process-lifetime diagnostic.
Creation creation{};
gears_frame_link::Result proof{};
owned_scene_readback::Probe readback;
#if defined(GEARS_NATIVE_OWNED_CONTINUOUS_PROBE) && GEARS_NATIVE_OWNED_CONTINUOUS_PROBE
constexpr bool continuous_probe=true;
#else
constexpr bool continuous_probe=false;
#endif
constexpr unsigned frame_limit=owned_xr_probe?120:16;
owned_scene_readback::Probe last_readback;
struct Frame {uint64_t ticket{};unsigned state{};bool copied{};};
std::array<Frame,frame_limit> consumed_frames{};unsigned consumed_count{};
gears_vr::StereoTicketStore image_tickets; // Independent of Present's legacy store and retirement.
struct Publication {uint64_t ticket{},pose{},session{},serial{};unsigned reason{};};
std::array<Publication,frame_limit> publications{};unsigned publication_count{},published_count{};
uint64_t retained_images{},duplicate_images{};
struct Timing {uint64_t ticket{};int64_t selected{},built{},begin{},draw[2]{},submitted{},returned{},destroyed{},ready{},publish_begin{},publish_end{},received{};
 gears_render_cost::Record cost{};};
std::array<Timing,frame_limit> timings{};unsigned timing_count{};
Timing* timing(uint64_t ticket){for(unsigned i=0;i<timing_count;++i)if(timings[i].ticket==ticket)return &timings[i];return nullptr;}
gears_render_cost::Record* render_cost(uint64_t ticket){
#if defined(GEARS_NATIVE_RENDER_COST_PROBE) && GEARS_NATIVE_RENDER_COST_PROBE
 // The game builder appends timings concurrently with RHI consumption. Only
 // lookup needs the lock: array storage stays fixed through callback drain.
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 auto t=timing(ticket);return t?&t->cost:nullptr;
#else
 return nullptr;
#endif
}
void selected(uint64_t ticket){
 if(!owned_xr_probe||!stereo_gpu_publish)return;
 if(timing_count<timings.size())timings[timing_count++]={ticket,gears_vr::monotonic_ns()};
}
void began(uint64_t ticket){if(auto t=timing(ticket))t->begin=gears_vr::monotonic_ns();}
void built(uint64_t ticket){if(auto t=timing(ticket))t->built=gears_vr::monotonic_ns();}
void drew(uint64_t ticket,unsigned eye){if(eye<2)if(auto t=timing(ticket))t->draw[eye]=gears_vr::monotonic_ns();}
void submitted(uint64_t ticket){if(auto t=timing(ticket))if(!t->submitted)t->submitted=gears_vr::monotonic_ns();}
void returned(uint64_t ticket){if(auto t=timing(ticket))t->returned=gears_vr::monotonic_ns();}
void destroyed(uint64_t ticket){if(auto t=timing(ticket))t->destroyed=gears_vr::monotonic_ns();}
struct ExecuteFailure {
 uintptr_t caller{},module{},queue{},missing_list{},missing_reset{},missing_device{};unsigned count{},inside_present{},present_depth{},missing_type{};
 bool previously_certified{};gears_frame_link::Snapshot stats{};
 wchar_t module_path[MAX_PATH]{};std::array<void*,12> stack{};unsigned stack_count{};
} execute_failure;
std::array<IUnknown*,gears_frame_link::Tracker::list_capacity> created_lists{};
unsigned list_creation_count{},list_creation_closed{};
void fault(unsigned reason){if(!error)error=reason;tracker.invalidate();}
bool watching(){return enabled&&!finished&&!error;}
// Scene submission and GPU state proof must name the same observed generation.
// Present retires the legacy tracker independently of this owned target. Once
// observation stops, retained generations are historical, not current evidence.
gears_frame_link::ListRef list_ref(uintptr_t identity){
 return watching()?tracker.current(identity):gears_frame_link::ListRef{};
}
void list_created(HRESULT hr,void** out,bool open){
 if(!continuous_probe||!watching()||FAILED(hr))return;
 IUnknown* object{};IUnknown* identity{};
 if(!safe_copy(&object,out,sizeof(object))||!object||FAILED(object->QueryInterface(IID_PPV_ARGS(&identity)))){fault(14);return;}
 for(auto* previous:created_lists)if(previous==identity){identity->Release();fault(14);return;}
 if(list_creation_count==created_lists.size()){identity->Release();fault(14);return;}
 created_lists[list_creation_count++]=identity; // Diagnostic lease until process exit; no pointer reuse.
 if(!open)++list_creation_closed;
 if(!tracker.create_success(reinterpret_cast<uintptr_t>(identity),open).identity)fault(14);
}
struct CreateScope {CreateScope(){++creating;}~CreateScope(){--creating;}};
void created(ID3D12Device* device,HRESULT hr,void** out,D3D12_RESOURCE_STATES state,unsigned api,bool legacy=true,unsigned layout=0){
 gears_resize_probe::created(api,hr);
 if(!enabled)return;if(api<api_calls.size()){++api_calls[api];if(creating)++api_scoped[api];}
 if(!creating||FAILED(hr))return;
 IUnknown* object{};ID3D12Resource* value{};
 if(!safe_copy(&object,out,sizeof(object))||!object||FAILED(object->QueryInterface(IID_PPV_ARGS(&value)))){fault(1);return;}
 if(creation_count==creations.size()){value->Release();fault(2);return;}
 creations[creation_count++]={canonical_identity(value),canonical_identity(device),value->GetDesc(),unsigned(state),api,0,legacy,layout};value->Release();
}
// Before activation, any observed GPU reference to the new allocation rejects
// the creation-state seed. CPU adapter metadata queries do not change GPU state.
bool touches(uintptr_t value){
 if(!value)return false;
 if(resource)return value==resource;
 for(unsigned i=0;i<creation_count;++i)if(creations[i].resource==value)return true;
 return false;
}
void prebirth_use(uintptr_t value){
 for(unsigned i=0;i<creation_count;++i)if(creations[i].resource==value)++creations[i].uses;
 if(resource==value)++creation.uses;
}
void accept(ID3D12Resource* value,uint64_t ticket){
 if(!enabled)return;texture=value;resource=canonical_identity(value);generation=ticket;
 ID3D12Device* device{};if(FAILED(value->GetDevice(IID_PPV_ARGS(&device)))){fault(4);return;}
 const auto identity=canonical_identity(device);
 auto compare=[&](void* object,unsigned slot,unsigned installed,unsigned bit){device_dispatch_available|=bit;
  if((*reinterpret_cast<void***>(object))[slot]==viewport_targets[installed])device_dispatch_match|=bit;};
 compare(device,27,11,1);compare(device,29,12,2);
 ID3D12Device4* d4{};if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&d4)))){compare(d4,53,24,4);d4->Release();}
 ID3D12Device8* d8{};if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&d8)))){compare(d8,69,25,8);compare(d8,70,22,16);d8->Release();}
 ID3D12Device10* d10{};if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&d10)))){compare(d10,76,26,32);compare(d10,77,23,64);d10->Release();}
 device->Release();
 unsigned matches{};for(unsigned i=0;i<creation_count;++i)if(creations[i].resource==resource){creation=creations[i];++matches;}
 const auto& d=creation.desc;
 if(matches!=1||!resource||!creation.legacy||d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.Width!=4096||d.Height!=2048||
  d.DepthOrArraySize!=1||d.MipLevels!=1||d.SampleDesc.Count!=1||d.SampleDesc.Quality||d.Format!=DXGI_FORMAT_R8G8B8A8_UNORM||
  d.Flags!=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET||(creation.state!=4&&creation.state!=128)){fault(3);return;}
 if(identity!=creation.device){fault(4);return;}
}
void reset(ID3D12GraphicsCommandList* list,HRESULT hr){
 if(!watching())return;auto identity=canonical_identity(list);
 if(SUCCEEDED(hr))tracker.reset_success(identity);else tracker.invalidate_list(identity);
}
void present(){if(watching()&&!birth_ok){
 if(continuous_probe)tracker.retire_before_owned_birth();else tracker.retire_present_interval();
}}
void activate(){
 if(!watching()||birth_attempts++)return;
 if(!resource||creation.uses||prebirth_faults){fault(5);return;}
 {
  std::lock_guard<std::mutex> lock(binding_owner_mutex);
  uintptr_t found{};
  for(auto const& owner:binding_owners)if(owner.queue){
   ID3D12CommandQueue* candidate{};ID3D12Device* device{};
   if(FAILED(owner.queue->QueryInterface(IID_PPV_ARGS(&candidate)))){fault(6);return;}
   const bool match=candidate->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_DIRECT&&
    SUCCEEDED(candidate->GetDevice(IID_PPV_ARGS(&device)))&&canonical_identity(device)==creation.device;
   if(device)device->Release();
   if(match){auto id=canonical_identity(candidate);if(found&&found!=id){candidate->Release();fault(6);return;}
    if(!found){queue=candidate;found=id;candidate=nullptr;}}
   if(candidate)candidate->Release();
  }
 }
 if(!queue){fault(6);return;}
 birth_ok=tracker.birth_owned_output({resource,4096,2048},canonical_identity(queue),generation,creation.state,true,true);
 if(!birth_ok)fault(7);
 if(birth_ok&&owned_xr_probe&&stereo_gpu_publish){
  StereoPublishBypass bypass;
  if(stereo_publish_counts.stopped.load()||!stereo_publisher.bind_resource(queue,texture,generation,
       D3D12_RESOURCE_STATES(creation.state),simulator_operator)){fault(16);return;}
  ++stereo_publish_counts.bind_requests;
 }
}
void retain_image(){
 if(!owned_xr_probe||!stereo_gpu_publish||!watching())return;
 auto const& frame=consuming_frozen_frame;
 if(!birth_ok||!frame.enabled||!frame.id||frame.id!=consuming_frame_ticket){fault(17);return;}
 auto result=image_tickets.retain(frame.id,frame.sample,simulator_operator);
 if(!gears_vr::StereoTicketStore::accepted(result)){fault(17);return;}
 if(result==gears_vr::TicketRetainResult::retained)++retained_images;else ++duplicate_images;
}
bool publish_image(){
 if(!owned_xr_probe||!stereo_gpu_publish)return true;
 gears_vr::StereoImageTicket image{};image.id=proof.ticket;
 if(!image_tickets.lookup(image.id,image.sample,simulator_operator)||proof.owned_generation!=generation||
    proof.target.resource!=resource||proof.final_state!=creation.state){fault(18);return false;}
 for(unsigned eye=0;eye<2;++eye){auto r=proof.eyes[eye];
  image.rect[eye]={static_cast<int>(r.x),static_cast<int>(r.y),static_cast<int>(r.width),static_cast<int>(r.height)};}
 if(!gears_vr::stereo_image_ticket_valid(image,4096,2048,simulator_operator)||publication_count==publications.size()){
  fault(18);return false;
 }
 gears_vr::GpuCaptureTrace trace{};bool published=false;
 auto stamp=timing(image.id);if(stamp){stamp->received=image.sample.received_ns;stamp->publish_begin=gears_vr::monotonic_ns();}
 const auto deadline=stereo_publish_deadline.load();
 ++stereo_publish_counts.capture_attempts;
 if(stereo_publish_counts.stopped.load())trace.reason=gears_vr::GpuCaptureReason::native_stopped;
 else if(deadline&&now()>=deadline)trace.reason=gears_vr::GpuCaptureReason::native_deadline;
 else {StereoPublishBypass bypass;published=stereo_publisher.capture_resource(texture,generation,
       D3D12_RESOURCE_STATES(proof.final_state),image,&trace);}
 if(stamp)stamp->publish_end=gears_vr::monotonic_ns();
 publications[publication_count++]={image.id,image.sample.sequence,image.sample.session,trace.serial_after,unsigned(trace.reason)};
 if(published){++published_count;stereo_publish_counts.last_ticket=image.id;}
 else {
  ++stereo_publish_counts.capture_skipped;
  using R=gears_vr::GpuCaptureReason;
  const bool skip=trace.reason==R::lock_busy||trace.reason==R::no_consumer||trace.reason==R::producer_in_flight||
   trace.reason==R::consumer_not_done||trace.reason==R::native_stopped||trace.reason==R::native_deadline||
   (trace.reason==R::not_ready&&(trace.state==1||(trace.state==4&&stereo_publish_counts.stopped.load())));
  if(!skip){fault(19);return false;}
 }
 // capture_resource copied the value or made an explicit no-submit decision.
 // It owns all GPU copy references and fences independently of this CPU store.
 if(!image_tickets.retire_through(image.id)){fault(20);return false;}
 return true;
}
void draw(ID3D12GraphicsCommandList* list,uintptr_t target,bool exact,unsigned eye,gears_frame_link::Rect rect,uint64_t ticket){
 if(!watching()||!touches(target))return;
 if(!birth_ok){prebirth_use(target);return;}
 auto ref=tracker.current(canonical_identity(list));
 if(exact)tracker.draw_final(ref,ticket,eye,{resource,4096,2048},rect);
 else tracker.draw_unknown(ref,resource);
}
void uncertain(Unknown reason=Unknown::unspecified){if(!watching()||(!resource&&!creation_count))return;++unsupported;
 ++unknown_reasons[unsigned(reason)];
 if(!birth_ok)++prebirth_faults;else fault(8);}
void use(ID3D12GraphicsCommandList* list,uintptr_t target,bool state_uncertain){
 if(!watching()||!touches(target))return;
 if(!birth_ok){prebirth_use(target);return;}
 if(state_uncertain){uncertain(Unknown::copy);return;}
 tracker.draw_unknown(tracker.current(canonical_identity(list)),resource);
}
void barrier(ID3D12GraphicsCommandList* list,D3D12_RESOURCE_BARRIER const& b){
 if(!watching())return;
 if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_TRANSITION){
  auto target=canonical_identity(b.Transition.pResource);if(!touches(target))return;++barriers;
  if(!birth_ok){prebirth_use(target);return;}
  tracker.record_transition(tracker.current(canonical_identity(list)),resource,generation,
   b.Transition.StateBefore,b.Transition.StateAfter,b.Transition.Subresource,b.Flags);
 }else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING){
  if(!b.Aliasing.pResourceBefore||!b.Aliasing.pResourceAfter||touches(canonical_identity(b.Aliasing.pResourceBefore))||
   touches(canonical_identity(b.Aliasing.pResourceAfter)))uncertain(Unknown::alias);
 }else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_UAV){
  if(!b.UAV.pResource||touches(canonical_identity(b.UAV.pResource)))uncertain(Unknown::uav);
 }else uncertain();
}
struct Submission {
 gears_frame_link::ExecutionToken token{};
 std::array<gears_frame_link::ListRef,gears_frame_link::Tracker::list_capacity> refs{};
 unsigned count{};bool prebirth{};
};
Submission prepare(ID3D12CommandQueue* q,UINT count,ID3D12CommandList* const* members,uintptr_t caller=0){
 Submission receipt{};if(!watching())return receipt;auto& token=receipt.token;
 std::array<ID3D12CommandList*,gears_frame_link::Tracker::list_capacity> lists{};
 auto& refs=receipt.refs;
 if(!count||count>lists.size()||!safe_copy(lists.data(),members,count*sizeof(lists[0]))){
  // The original is still forwarded. Missing its membership would lose actual
  // submission history, so a later Present must never recover this probe.
  if(continuous_probe)fault(15);else tracker.invalidate();return receipt;
 }
 for(unsigned i=0;i<count;++i)refs[i]=tracker.current(canonical_identity(lists[i]));
 if(birth_ok){
  const bool was_certified=tracker.owned_status().certified;
  if(!tracker.prepare_execute(canonical_identity(q),refs.data(),count,token)){
   auto& f=execute_failure;f.caller=caller;f.queue=canonical_identity(q);f.count=count;f.previously_certified=was_certified;
   f.inside_present=frame_inside_original_present;f.present_depth=frame_present_depth;f.stats=tracker.snapshot();
   for(unsigned i=0;i<count;++i)if(!refs[i].identity){
    f.missing_list=canonical_identity(lists[i]);f.missing_type=lists[i]->GetType();
    ID3D12GraphicsCommandList* missing{};if(SUCCEEDED(lists[i]->QueryInterface(IID_PPV_ARGS(&missing)))){
     f.missing_reset=reinterpret_cast<uintptr_t>((*reinterpret_cast<void***>(missing))[10]);missing->Release();}
    ID3D12Device* d{};if(SUCCEEDED(lists[i]->GetDevice(IID_PPV_ARGS(&d)))){f.missing_device=canonical_identity(d);d->Release();}
    break;
   }
   HMODULE module{};if(caller&&GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    reinterpret_cast<LPCWSTR>(caller),&module)){f.module=reinterpret_cast<uintptr_t>(module);GetModuleFileNameW(module,f.module_path,MAX_PATH);}
   f.stack_count=CaptureStackBackTrace(0,static_cast<DWORD>(f.stack.size()),f.stack.data(),nullptr);fault(12);
  }
 }
 else {tracker.execute(canonical_identity(q),refs.data(),count);receipt.prebirth=continuous_probe;receipt.count=count;}
 return receipt;
}
void commit(const Submission& receipt){
 if(!watching())return;
 if(receipt.prebirth&&!tracker.submitted_before_owned_birth(receipt.refs.data(),receipt.count))fault(13);
 if(receipt.token.value&&!tracker.commit_execute(receipt.token))fault(13);
}
void copy(bool ready,uint64_t ticket){
 if(!watching()||!birth_ok||!ready||(!continuous_probe&&sealed)||
    (continuous_probe&&consumed_count&&ticket==consumed_frames[consumed_count-1].ticket))return;
 sealed=true;
 if(auto t=timing(ticket))t->ready=gears_vr::monotonic_ns();
 proof=continuous_probe?tracker.consume_owned_output(resource,generation,canonical_identity(queue),ticket):
  tracker.seal_owned_output(resource,generation,canonical_identity(queue));
 if(!proof.linked||!proof.state_certified||proof.ticket!=ticket){fault(9);return;}
 const bool last=continuous_probe&&consumed_count+1==frame_limit;
 const bool sample=!continuous_probe||!consumed_count||last;
 if(sample){
  wchar_t path[MAX_PATH]{};
  if(last)swprintf_s(path,L"%s\\owned-scene-%lu-last.ppm",directory,GetCurrentProcessId());
  else swprintf_s(path,L"%s\\owned-scene-%lu.ppm",directory,GetCurrentProcessId());
  StereoPublishBypass bypass;
  if(!(last?last_readback:readback).queue(texture,queue,D3D12_RESOURCE_STATES(proof.final_state),true,path)){fault(10);return;}
 }
 if(!publish_image())return;
 if(continuous_probe){
  if(consumed_count==consumed_frames.size()){fault(11);return;}
  consumed_frames[consumed_count++]={ticket,proof.final_state,sample};
 }
 finished=!continuous_probe||last;
}
void poll(){if(enabled){readback.poll();last_readback.poll();}}
bool can_advance(uint64_t ticket){return continuous_probe&&watching()&&birth_ok&&consumed_count&&
 consumed_count<frame_limit&&consumed_frames[consumed_count-1].ticket==ticket&&tracker.owned_status().certified;}
bool state_certified(){return proof.state_certified&&tracker.owned_status().certified;}
bool pixels_read_back(){return readback.result().completed&&readback.result().saved;}
bool publication_failed(){return owned_xr_probe&&stereo_gpu_publish&&error;}
void print(FILE* file){
 auto const& r=readback.result();const auto s=tracker.snapshot();
 fprintf(file,"native_owned_gpu enabled=%u creation_count=%u api=%u initial_state=%u creation_uses=%u prebirth_faults=%u birth_attempts=%u birth_ok=%u barriers=%u unsupported=%u error=%u sealed=%u rejection=%u state_certified=%u final_state=%u copied=%u completed=%u saved=%u read_error=%u read_hr=%08lx resets=%llu executes=%llu unknown_batches=%llu capacity=%u\n",
 enabled,creation_count,creation.api,creation.state,creation.uses,prebirth_faults,birth_attempts,birth_ok,barriers,unsupported,error,sealed,
 proof.rejection,state_certified(),proof.final_state,r.queued,r.completed,r.saved,unsigned(r.error),r.hr,s.reset_calls,s.execute_calls,s.unknown_batches,s.capacity_bad);
 fprintf(file,"native_owned_gpu_creation legacy=%u layout=%u resource=%llx device=%llx format=%u flags=%u\n",
  creation.legacy,creation.layout,creation.resource,creation.device,unsigned(creation.desc.Format),unsigned(creation.desc.Flags));
 fprintf(file,"native_owned_pixels width=%u height=%u left_nonblack=%llu right_nonblack=%llu different=%llu left_hash=%llx right_hash=%llx\n",
  r.width,r.height,r.eyes[0].nonblack,r.eyes[1].nonblack,r.differing_pixels,r.eyes[0].hash,r.eyes[1].hash);
 fprintf(file,"native_owned_gpu_dispatch available=%u match=%u\n",device_dispatch_available,device_dispatch_match);
 if(continuous_probe)fprintf(file,"native_owned_list_creation count=%u closed=%u tracker_creates=%llu retained_until_exit=1\n",list_creation_count,list_creation_closed,s.list_creates);
 if(execute_failure.count){auto const& f=execute_failure;
  fprintf(file,"native_owned_execute_failure caller=%llx module=%llx rva=%llx queue=%llx count=%u missing_list=%llx previously_certified=%u inside_present=%u present_depth=%u unknown=%llu duplicate=%llu metadata=%llu\n",
   f.caller,f.module,f.caller-f.module,f.queue,f.count,f.missing_list,f.previously_certified,f.inside_present,f.present_depth,
   f.stats.unknown_batches,f.stats.duplicate_batches,f.stats.metadata_batches);
  fprintf(file,"native_owned_execute_module path=%ls\n",f.module_path);
  fprintf(file,"native_owned_execute_missing type=%u device=%llx reset=%llx installed_reset=%p reset_dispatch_matches=%u\n",
   f.missing_type,f.missing_device,f.missing_reset,viewport_targets[0],f.missing_reset==reinterpret_cast<uintptr_t>(viewport_targets[0]));
  for(unsigned i=0;i<f.stack_count;++i)fprintf(file,"native_owned_execute_stack index=%u address=%p\n",i,f.stack[i]);
 }
 if(continuous_probe){const auto& last=last_readback.result();
  fprintf(file,"native_owned_continuous limit=%u consumed=%u finished=%u last_completed=%u last_saved=%u last_error=%u last_left_nonblack=%llu last_right_nonblack=%llu last_different=%llu\n",
   frame_limit,consumed_count,finished,last.completed,last.saved,unsigned(last.error),last.eyes[0].nonblack,last.eyes[1].nonblack,last.differing_pixels);
  for(unsigned i=0;i<consumed_count;++i)fprintf(file,"native_owned_frame index=%u ticket=%llu state=%u sampled=%u\n",i,consumed_frames[i].ticket,consumed_frames[i].state,consumed_frames[i].copied);
 }
 if(owned_xr_probe){
  fprintf(file,"native_owned_xr enabled=%u attempts=%u published=%u retained=%llu duplicates=%llu ticket_store_size=%zu retired_ticket=%llu\n",
   stereo_gpu_publish,publication_count,published_count,retained_images,duplicate_images,image_tickets.size(),image_tickets.retired_id());
  for(unsigned i=0;i<publication_count;++i){auto const& p=publications[i];
   fprintf(file,"native_owned_publication ticket=%llu pose=%llu session=%llu serial=%llu reason=%s\n",p.ticket,p.pose,p.session,p.serial,
    gears_vr::gpu_capture_reason_name(gears_vr::GpuCaptureReason(p.reason)));}
  for(unsigned i=0;i<timing_count;++i){auto const& t=timings[i];
   fprintf(file,"native_owned_timing ticket=%llu received_ns=%lld selected_ns=%lld built_ns=%lld begin_ns=%lld left_ns=%lld right_ns=%lld submitted_ns=%lld returned_ns=%lld destroyed_ns=%lld ready_ns=%lld publish_begin_ns=%lld publish_end_ns=%lld\n",
    t.ticket,t.received,t.selected,t.built,t.begin,t.draw[0],t.draw[1],t.submitted,t.returned,t.destroyed,t.ready,t.publish_begin,t.publish_end);
#if defined(GEARS_NATIVE_RENDER_COST_PROBE) && GEARS_NATIVE_RENDER_COST_PROBE
   fprintf(file,"native_owned_cost ticket=%llu total_ticks=%llu",t.ticket,t.cost.total_ticks);
   const char* names[]={"safe_read","engine_view","output_stack","viewport_draw","frame_draw","reset_tracking","barrier_observation","frame_lock","draw_forward"};
   for(unsigned k=0;k<gears_render_cost::count;++k)fprintf(file," %s_calls=%llu %s_ticks=%llu",names[k],t.cost.items[k].calls,names[k],t.cost.items[k].ticks);
   fprintf(file," kernel_100ns=%llu user_100ns=%llu cpu_samples=%llu cpu_failures=%llu thread=%lu thread_changes=%u\n",
    t.cost.kernel_100ns,t.cost.user_100ns,t.cost.cpu_samples,t.cost.cpu_failures,t.cost.thread,t.cost.thread_changes);
#endif
  }
 }
 for(unsigned i=1;i<api_calls.size();++i)fprintf(file,"native_owned_gpu_api api=%u calls=%llu scoped=%llu\n",i,api_calls[i],api_scoped[i]);
 const char* labels[]={"unspecified","descriptor","clear","indirect","bundle","enhanced","render_pass","alias","uav","unreadable","copy","mrt"};
 for(unsigned i=0;i<unknown_reasons.size();++i)if(unknown_reasons[i])fprintf(file,"native_owned_gpu_unknown reason=%s count=%llu\n",labels[i],unknown_reasons[i]);
}
}
#else
namespace owned_scene_gpu {
struct CreateScope{};
gears_frame_link::ListRef list_ref(uintptr_t identity){return frame_link.current(identity);}
void accept(ID3D12Resource*,uint64_t){}
void activate(){}
void reset(ID3D12GraphicsCommandList*,HRESULT){}
void present(){}
void draw(ID3D12GraphicsCommandList*,uintptr_t,bool,unsigned,gears_frame_link::Rect,uint64_t){}
void retain_image(){}
void selected(uint64_t){}
void began(uint64_t){}
void built(uint64_t){}
gears_render_cost::Record* render_cost(uint64_t){return nullptr;}
void drew(uint64_t,unsigned){}
void submitted(uint64_t){}
void returned(uint64_t){}
void destroyed(uint64_t){}
void commit(gears_frame_link::ExecutionToken){}
gears_frame_link::ExecutionToken prepare(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*,uintptr_t=0){return {};}
void copy(bool,uint64_t){}
bool can_advance(uint64_t){return false;}
void poll(){}
bool state_certified(){return false;}
bool pixels_read_back(){return false;}
bool publication_failed(){return false;}
void print(FILE*){}
}
#endif
