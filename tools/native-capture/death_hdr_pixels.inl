#include "death_hdr_pixels.h"
#include "../vr-common/death_hdr_match.h"
namespace death_hdr_pixels {
#if defined(GEARS_DEATH_HDR_PIXELS) && GEARS_DEATH_HDR_PIXELS
using namespace gears_death_hdr_pixels;
bool attested{},attempted{},prepared{},released{},stopped{};HRESULT result=S_OK;
Trigger trigger;Lifetime life;
gears_death_hdr_match::Publisher matched_ticket;
bool match_mapping_created{};
HRESULT reset_hr=E_PENDING;
long long reset_qpc{};
ID3D12Resource *source_hold{},*destination_hold{},*readback{};
ID3D12GraphicsCommandList* list_hold{};ID3D12CommandQueue* queue_hold{};ID3D12Device* device{};ID3D12Fence* fence{};
D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT rows{};UINT64 row_bytes{},bytes{};
D3D12_TEXTURE_COPY_LOCATION original_source{},original_destination{};
uint64_t ticket{},world{},copy_seq{};long long prepared_qpc{};wchar_t raw_path[MAX_PATH]{},metadata_path[MAX_PATH]{};
template<class T> void release(T*& value){if(value){value->Release();value=nullptr;}}
void release_safe(){release(source_hold);release(destination_hold);release(readback);release(list_hold);release(queue_hold);release(fence);release(device);released=true;}
// Raw COM holders intentionally have no destructor. Ambiguous submitted objects
// remain referenced until process exit, including after hook teardown.
uintptr_t identity(IUnknown* object){__try{return object?canonical_identity(object):0;}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}}
Texture texture(ResourceIdentity const* i){
    if(!i||FAILED(i->result))return {};
    auto const& d=i->desc;return {i->canonical,d.Width,d.Height,unsigned(d.Format),unsigned(d.Dimension),unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),d.SampleDesc.Count,d.SampleDesc.Quality};
}
Texture texture(ID3D12Resource* resource){auto d=resource->GetDesc();return {identity(resource),d.Width,d.Height,unsigned(d.Format),unsigned(d.Dimension),unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),d.SampleDesc.Count,d.SampleDesc.Quality};}
bool enable(FILE* file){
    const bool runtime_match=!self_testing&&continuous_session&&frame_link_diagnostics&&color_copy_diagnostics&&viewport_diagnostics&&
        final_transfer_live::verify_range(file,0x1c6033,128,"9a2b3a678a07074110f5209b21e01a66b6a93a34872b76c01eee70f2d30b2c86");
    attested=runtime_match;
    if(attested){match_mapping_created=matched_ticket.create(GetCurrentProcessId());attested=match_mapping_created;}
    fprintf(file,"death_hdr_pixels_identity enabled=1 runtime_range_match=%u match_mapping_created=%u probe_enabled=%u rva=1c6033 size=128 historical_code_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 whole_function_attested=0\n",unsigned(runtime_match),unsigned(match_mapping_created),unsigned(attested));
    return attested;
}
void sample(gears_death_hdr::Event const& event,uintptr_t canonical){
    if(!attested||attempted)return;
    // Called under the timeline's frame lock, after its guarded identity query.
    auto* selected=current_unpossessed_quad_bank();auto* bank=death_hdr_timeline::current();
    if(!selected||!bank||selected->ordinal!=49)return;
    auto const* cached=bank->identities.find(event.target.resource);auto t=texture(cached);
    if(t.canonical!=canonical)return;
    trigger.sample(selected->ticket.id,selected->ticket.world,selected->ordinal,bank->sequence,event,t);
}
bool exact_locations(D3D12_TEXTURE_COPY_LOCATION const& src,D3D12_TEXTURE_COPY_LOCATION const& dst){
    return src.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&dst.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&
        src.SubresourceIndex==0&&dst.SubresourceIndex==0&&src.pResource&&dst.pResource;
}
bool make_paths(){
    const auto length=GetModuleFileNameW(own_module,raw_path,MAX_PATH);
    if(!length||length>=MAX_PATH)return false;
    auto* separator=wcsrchr(raw_path,L'\\');if(!separator)return false;*separator=0;
    wchar_t parent[MAX_PATH]{};wcscpy_s(parent,raw_path);
    return swprintf_s(raw_path,L"%s\\death-hdr-source-%lu-%llu-%lld.raw",parent,GetCurrentProcessId(),ticket,prepared_qpc)>0&&
        swprintf_s(metadata_path,L"%s.txt",raw_path)>0;
}
bool prepare_copy(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,UINT x,UINT y,UINT z,
    const D3D12_TEXTURE_COPY_LOCATION* source,const D3D12_BOX* box,uintptr_t caller){
    if(!attested||!recording.load()||!consuming_frozen_frame.unpossessed)return false;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(attempted||stopped)return false;
    auto* selected=current_unpossessed_quad_bank();auto* bank=death_hdr_timeline::current();
    if(!selected||!bank||selected->ordinal!=49||bank->dropped[gears_death_hdr::sample]||bank->dropped[gears_death_hdr::copy]||
       bank->dropped[gears_death_hdr::hdr_draw]||bank->identities.dropped||bank->read_failures)return false;
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};
    if(!safe_copy(&src,source,sizeof(src))||!safe_copy(&dst,destination,sizeof(dst))||!exact_locations(src,dst))return false;
    uint64_t original_seq{};
    // Other threads may have appended Execute rows since this API call's
    // timeline observation. Bind to its retained copy row, never the live counter.
    for(unsigned i=bank->size;i>0;--i){auto const& r=bank->rows[i-1];auto const& e=r.event;
        if(e.kind==gears_death_hdr::copy&&e.list.raw==reinterpret_cast<uintptr_t>(list)&&e.caller==caller&&e.read_mask==7&&
           e.resources[0]==reinterpret_cast<uintptr_t>(src.pResource)&&e.resources[1]==reinterpret_cast<uintptr_t>(dst.pResource)&&
           e.source_type==unsigned(src.Type)&&e.destination_type==unsigned(dst.Type)&&!e.source_subresource&&!e.destination_subresource&&
           e.xyz==std::array<unsigned,3>{x,y,z}&&e.has_box==(box!=nullptr)){
            if(box)continue;original_seq=r.first;break;}}
    if(!original_seq)return false;
    auto st=texture(bank->identities.find(reinterpret_cast<uintptr_t>(src.pResource)));
    auto dt=texture(bank->identities.find(reinterpret_cast<uintptr_t>(dst.pResource)));
    if(!trigger.copy(selected->ticket.id,selected->ticket.world,selected->ordinal,original_seq,caller,st,dt,!x&&!y&&!z&&!box))return false;
    attempted=true;ticket=selected->ticket.id;world=selected->ticket.world;copy_seq=original_seq;prepared_qpc=now();
    original_source=src;original_destination=dst;
    const auto ref=frame_link.current(identity(list));life.list={reinterpret_cast<uintptr_t>(list),ref.identity,ref.generation};
    auto fail=[&](){life.fail(prepare_failed);release_safe();return false;};
    if(!ref.identity||!ref.generation||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||!make_paths()||
       !system_resource_methods(reinterpret_cast<uintptr_t>(src.pResource))||!system_resource_methods(reinterpret_cast<uintptr_t>(dst.pResource)))return fail();
    result=src.pResource->QueryInterface(IID_PPV_ARGS(&source_hold));if(FAILED(result))return fail();
    result=dst.pResource->QueryInterface(IID_PPV_ARGS(&destination_hold));if(FAILED(result))return fail();
    auto fresh_src=texture(source_hold),fresh_dst=texture(destination_hold);
    if(!supported(fresh_src)||!supported(fresh_dst)||fresh_src.canonical!=st.canonical||fresh_dst.canonical!=dt.canonical||
       fresh_src.width!=st.width||fresh_src.height!=st.height||fresh_dst.width!=st.width||fresh_dst.height!=st.height)return fail();
    result=source_hold->GetDevice(IID_PPV_ARGS(&device));if(FAILED(result))return fail();
    ID3D12Device *destination_device{},*list_device{};
    const bool same_device=SUCCEEDED(destination_hold->GetDevice(IID_PPV_ARGS(&destination_device)))&&
        SUCCEEDED(list->GetDevice(IID_PPV_ARGS(&list_device)))&&identity(device)&&identity(device)==identity(destination_device)&&identity(device)==identity(list_device);
    release(destination_device);release(list_device);if(!same_device)return fail();
    const auto desc=source_hold->GetDesc();device->GetCopyableFootprints(&desc,0,1,0,&footprint,&rows,&row_bytes,&bytes);
    if(!footprint_valid(st,bytes,footprint.Offset,footprint.Footprint.RowPitch,rows,row_bytes,footprint.Footprint.Width,
        footprint.Footprint.Height,footprint.Footprint.Depth,unsigned(footprint.Footprint.Format)))return fail();
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=bytes;buffer.Height=1;
    buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    result=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback));
    if(FAILED(result))return fail();result=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));if(FAILED(result))return fail();
    list_hold=list;list_hold->AddRef();life.held_identity=identity(list_hold);
    if(!life.held_identity||life.held_identity!=life.list.identity)return fail();
    prepared=true;return true;
}
void append_after_original(bool token,ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,const D3D12_TEXTURE_COPY_LOCATION* source){
    if(!token)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};
    if(stopped||!prepared||life.appended||list!=list_hold||!safe_copy(&src,source,sizeof(src))||!safe_copy(&dst,destination,sizeof(dst))||
       !exact_locations(src,dst)||src.pResource!=original_source.pResource||dst.pResource!=original_destination.pResource){
        life.fail(prepare_failed);if(!life.appended)release_safe();return;}
    D3D12_TEXTURE_COPY_LOCATION own{};own.pResource=readback;own.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;own.PlacedFootprint=footprint;
    life.appended=true;prepared=false;
    original_color_copy(list,&own,0,0,0,&original_source,nullptr); // Same source contract, immediately adjacent; no transitions.
    matched_ticket.publish(ticket,world,copy_seq); // One immutable request; ticket is the atomic LAST commit.
}
std::unique_lock<FrameHookMutex> execution_lock(){return std::unique_lock<FrameHookMutex>(frame_hook_mutex);}
bool prepare_execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* members){
    if(!life.appended||life.failure||life.saved)return false;
    std::array<ID3D12CommandList*,gears_frame_link::Tracker::list_capacity> copied{};
    bool readable=count<=copied.size()&&(!count||safe_copy(copied.data(),members,size_t(count)*sizeof(copied[0])));
    unsigned occurrences{};bool exact=true;
    if(readable)for(unsigned i=0;i<count;++i){auto id=identity(copied[i]);if(!id){readable=false;break;}
        if(id==life.list.identity){++occurrences;const auto ref=frame_link.current(id);exact=exact&&ref.generation==life.list.generation;}}
    uintptr_t queue_id=occurrences?identity(queue):0;
    if(occurrences&&(!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)){life.fail(cross_queue);return false;}
    if(!life.before(readable,occurrences,queue_id,exact))return false;
    ID3D12Device* queue_device{};const bool same=SUCCEEDED(queue->GetDevice(IID_PPV_ARGS(&queue_device)))&&identity(queue_device)==identity(device);
    release(queue_device);if(!same){life.fail(cross_queue);return false;}
    queue_hold=queue;queue_hold->AddRef();return true;
}
void after_execute(bool token,ID3D12CommandQueue* queue){if(!token||life.failure)return;result=queue->Signal(fence,1);life.after(SUCCEEDED(result));}
void execute_passthrough(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* members){
    auto lock=execution_lock();const bool token=prepare_execute(queue,count,members);original_execute(queue,count,members);after_execute(token,queue);
}
void reset_after_original(ID3D12GraphicsCommandList* list,HRESULT hr){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!life.appended||life.failure||life.saved)return;
    const auto id=identity(list);
    // A known unrelated object cannot retire the held list. An unidentified
    // callback, or a changed identity at its held raw address, fails closed.
    if(id&&id!=life.held_identity&&reinterpret_cast<uintptr_t>(list)!=life.list.raw)return;
    if(life.retired)return;
    const auto tracker=frame_link.current(life.held_identity);reset_hr=hr;reset_qpc=now();
    life.reset(SUCCEEDED(hr),id,tracker.generation,reinterpret_cast<uintptr_t>(list),tracker.identity);
}
HRESULT reset_passthrough(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* state){
    auto lock=execution_lock();auto hr=original_reset_list(list,allocator,state);reset_after_original(list,hr);return hr;
}
void poll(){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!life.appended||life.failure||life.saved||!life.signalled)return;
    result=device->GetDeviceRemovedReason();if(FAILED(result)){life.fail(device_removed);return;}
    life.completion(fence->GetCompletedValue());if(!life.may_map())return;
    void* mapped{};D3D12_RANGE range{0,static_cast<SIZE_T>(bytes)};result=readback->Map(0,&range,&mapped);
    if(FAILED(result)||!mapped){life.fail(map_failed);return;}
    HANDLE file=CreateFileW(raw_path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);bool ok=file!=INVALID_HANDLE_VALUE;
    if(ok){DWORD written{};ok=WriteFile(file,mapped,static_cast<DWORD>(bytes),&written,nullptr)&&written==bytes&&FlushFileBuffers(file);
        if(!CloseHandle(file))ok=false;}
    D3D12_RANGE none_written{0,0};readback->Unmap(0,&none_written);
    HANDLE meta=CreateFileW(metadata_path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(meta==INVALID_HANDLE_VALUE)ok=false;else{
        char text[2048]{};const int n=sprintf_s(text,"ticket=%llu world=%llu original_copy_seq=%llu caller=1c6093 qpc=%lld source_canonical=%llx destination_canonical=%llx list_identity=%llx list_generation=%llu queue=%llx executions=1 retired=1 gpu_complete=1 raw_bytes=%llu footprint_offset=%llu row_pitch=%u rows=%u row_bytes=%llu width=%u height=%u depth=%u format=%u source_subresource=0 destination_subresource=0 xyz=0,0,0 box_null=1 conversion=none retirement_source=observed_successful_original_Reset_on_held_canonical held_identity=%llx reset_raw=%llx reset_identity=%llx reset_hr=%08lx reset_tracker_generation=%llu reset_tracker_identity=%llx reset_qpc=%lld\n",
            ticket,world,copy_seq,prepared_qpc,static_cast<unsigned long long>(trigger.main.canonical),static_cast<unsigned long long>(trigger.destination.canonical),static_cast<unsigned long long>(life.list.identity),life.list.generation,static_cast<unsigned long long>(life.queue),bytes,footprint.Offset,footprint.Footprint.RowPitch,rows,row_bytes,footprint.Footprint.Width,footprint.Footprint.Height,footprint.Footprint.Depth,unsigned(footprint.Footprint.Format),
            static_cast<unsigned long long>(life.held_identity),static_cast<unsigned long long>(life.reset_receipt.raw),static_cast<unsigned long long>(life.reset_receipt.identity),static_cast<unsigned long>(reset_hr),life.reset_receipt.tracker_generation,static_cast<unsigned long long>(life.reset_receipt.tracker_identity),reset_qpc);
        DWORD written{};if(n<=0||!WriteFile(meta,text,n,&written,nullptr)||written!=unsigned(n)||!FlushFileBuffers(meta))ok=false;
        if(!CloseHandle(meta))ok=false;}
    if(ok){life.saved=true;release_safe();}else life.fail(file_failed);
}
void stop(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);stopped=true;life.stop();matched_ticket.close();}
void progress(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"death_hdr_pixels_saved=%u death_hdr_pixels_appended=%u death_hdr_pixels_failure=%u ",unsigned(life.saved),unsigned(life.appended),life.failure);}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"death_hdr_pixels enabled=1 attested=%u attempted=%u eyes=%u matching_copies=%u ticket=%llu world=%llu copy_seq=%llu appended=%u executed=%u signalled=%u retired=%u gpu_complete=%u saved=%u failure=%u hr=%08lx held_until_process=%u original_forwarded_once=1 appended_source_copy_count=%u separate_list=0 separate_queue=0 engine_barriers_added=0 source_state_source=adjacent_original_copy_contract all_execute_reset_branches=1 unknown_execution_quarantines=1 raw_path=%ls metadata_path=%ls\n",
        unsigned(attested),unsigned(attempted),trigger.eyes,trigger.copies,ticket,world,copy_seq,unsigned(life.appended),unsigned(life.executed),unsigned(life.signalled),unsigned(life.retired),unsigned(life.gpu_complete),unsigned(life.saved),life.failure,static_cast<unsigned long>(result),unsigned(life.appended&&!released),unsigned(life.appended),raw_path,metadata_path);
    const auto& r=life.reset_receipt;
    const char* reason=!r.observed?"not_observed":!r.success?"original_Reset_failed":!r.identity?"canonical_unidentified":
        r.identity!=life.held_identity?"held_identity_mismatch":life.failure==reset_before_execute?"reset_before_execute":"successful_physical_Reset";
    fprintf(file,"death_hdr_pixels_reset observed=%u retired=%u reason=%s held_canonical=%llx captured_tracker_identity=%llx captured_tracker_generation=%llu reset_raw=%llx observed_reset_canonical=%llx original_reset_hr=%08lx observed_tracker_identity=%llx observed_tracker_generation=%llu qpc=%lld retirement_source=observed_successful_original_Reset_on_held_canonical\n",
        unsigned(r.observed),unsigned(life.retired),reason,static_cast<unsigned long long>(life.held_identity),static_cast<unsigned long long>(life.list.identity),life.list.generation,
        static_cast<unsigned long long>(r.raw),static_cast<unsigned long long>(r.identity),static_cast<unsigned long>(reset_hr),static_cast<unsigned long long>(r.tracker_identity),r.tracker_generation,reset_qpc);
    fprintf(file,"death_hdr_match_mapping created=%u published=%u game_pid=%lu ticket=%llu world=%llu copy_seq=%llu independent_bridge_abi=1 publish_after_append=1\n",
        unsigned(match_mapping_created),unsigned(matched_ticket.published()),GetCurrentProcessId(),ticket,world,copy_seq);
}
#else
bool enable(FILE*){return true;}
void sample(gears_death_hdr::Event const&,uintptr_t){}
bool prepare_copy(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*,uintptr_t){return false;}
void append_after_original(bool,ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_TEXTURE_COPY_LOCATION*){}
std::unique_lock<FrameHookMutex> execution_lock(){return std::unique_lock<FrameHookMutex>(frame_hook_mutex,std::defer_lock);}
bool prepare_execute(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){return false;}
void after_execute(bool,ID3D12CommandQueue*){}
void execute_passthrough(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* members){original_execute(queue,count,members);}
void reset_after_original(ID3D12GraphicsCommandList*,HRESULT){}
HRESULT reset_passthrough(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator* allocator,ID3D12PipelineState* state){return original_reset_list(list,allocator,state);}
void poll(){}void stop(){}void progress(FILE*){}
void print(FILE* file){fprintf(file,"death_hdr_pixels enabled=0\n");}
#endif
}
