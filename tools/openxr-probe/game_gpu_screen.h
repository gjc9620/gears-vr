#pragma once
#include "../vr-common/gpu_share.h"
#include "../vr-common/gpu_blit.h"
#include "outage_diagnostics.h"
#include "eye_chain_policy.h"
#include "menu_eye_panel.h"

enum class ProjectionStatus {submitted,producer_stopped,producer_unavailable,waiting_image,current_inactive,current_untracked,
    reference_space_changed,empty_ticket,session_mismatch,ticket_flags,ticket_geometry,stale_ticket,image_rectangle,session_lost,producer_structure_invalid,probe_failure,menu_requested,menu_recovery_pending,reference_epoch_pending,source_reconfiguration,count};
inline const char* projection_status_name(ProjectionStatus status){
    static const char* names[]={"submitted","producer_stopped","producer_unavailable","waiting_image","current_inactive","current_untracked",
        "reference_space_changed","empty_ticket","session_mismatch","ticket_flags","ticket_geometry","stale_ticket","image_rectangle","session_lost","producer_structure_invalid","probe_failure","menu_requested","menu_recovery_pending","reference_epoch_pending","source_reconfiguration"};
    return names[static_cast<unsigned>(status)];
}
inline bool xr_image_work_complete(uint64_t completed,uint64_t target){return target && completed!=~uint64_t(0) && completed>=target;}
inline bool xr_image_wait_succeeded(XrResult result){return result==XR_SUCCESS;}
inline bool projection_session_visible(XrSessionState state){
    return state==XR_SESSION_STATE_VISIBLE||state==XR_SESSION_STATE_FOCUSED;
}
inline ProjectionStatus projection_current_status(bool should_render,bool visible,unsigned count,uint64_t head_flags,uint64_t view_flags,bool simulated=false){
    if(!should_render||!visible)return ProjectionStatus::current_inactive;
    const uint64_t required_head=simulated?3:15;
    if(count!=2||(head_flags&required_head)!=required_head||(view_flags&15)!=15)return ProjectionStatus::current_untracked;
    return ProjectionStatus::submitted;
}
inline constexpr int64_t projection_new_image_max_age=1000000000;
inline constexpr int64_t projection_retained_image_max_age=2000000000;
inline ProjectionStatus projection_ticket_status(const gears_vr::StereoImageTicket& ticket,unsigned width,unsigned height,uint64_t session_id,int64_t now,bool simulated=false,uint64_t space_epoch=0,int64_t change_time=0,int64_t maximum_age=100000000){
    auto const& sample=ticket.sample;
    if(!ticket.id||!sample.sequence||!sample.space_epoch)return ProjectionStatus::empty_ticket;
    if(sample.session!=session_id)return ProjectionStatus::session_mismatch;
    const auto expected_source=simulated?gears_vr::StereoSource::MetaOperatorSimulated:gears_vr::StereoSource::RuntimeTracked;
    if(!gears_vr::stereo_render_active(sample)||sample.source!=expected_source||!gears_vr::stereo_tracking_valid(sample,simulated))return ProjectionStatus::ticket_flags;
    if(!gears_vr::stereo_geometry_valid(sample))return ProjectionStatus::ticket_geometry;
    if(maximum_age<=0||now<=0||sample.received_ns<=0||sample.received_ns>now||now-sample.received_ns>maximum_age)return ProjectionStatus::stale_ticket;
    if(space_epoch&&(sample.space_epoch!=space_epoch||sample.predicted_time<change_time))return ProjectionStatus::reference_epoch_pending;
    if(!width||!height||width%2||width>8192||height>8192)return ProjectionStatus::image_rectangle;
    for(unsigned i=0;i<2;++i){auto r=ticket.rect[i];
        if(r.x!=int32_t(i*width/2)||r.y!=0||r.width!=int32_t(width/2)||r.height!=int32_t(height))return ProjectionStatus::image_rectangle;}
    return ProjectionStatus::submitted;
}

// GPU-only transfer on the same adapter. Legacy mono quad or two projection
// views from a validated SBS image and its immutable render-time XR ticket.
class GameGpuScreen {
    bool allow_simulated{};
    std::unique_ptr<gears_vr::GpuReceiver> receiver=std::make_unique<gears_vr::GpuReceiver>();
    struct Eye {
        XrSwapchain chain{};
        std::vector<XrSwapchainImageD3D12KHR> images;
        std::unique_ptr<gears_vr::GpuBlit> blit;
        bool acquired{},waited{};
    };
    std::array<Eye,2> eyes;
    bool stereo{};
    gears_eye_chain::Plan eye_plan{};
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
    ComPtr<ID3D12Fence> fence;
    HANDLE event{};UINT64 serial=0;
    std::chrono::steady_clock::time_point last_frame{};
    bool current=false,in_flight=false;
    bool retain_stereo_image{};
    // Only commit after both eye copies complete and their XR images release.
    // A newer receiver ticket can be rejected without changing these pixels.
    gears_vr::StereoImageTicket copied_ticket{};
    uint64_t copied_sequence{},copied_world_generation{},observed_world_generation{};
    ComPtr<ID3D12Resource> snapshot_buffer;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT snapshot_layout{};
    UINT64 snapshot_bytes=0;
    std::string snapshot_path;
    HANDLE snapshot_event{};
    static constexpr unsigned snapshot_limit=8; // Explicit diagnostic only, bounded disk usage.
    unsigned snapshot_count{};
    bool snapshot_auto_taken{};
    uint64_t snapshot_ticket{},snapshot_source_serial{};
    std::chrono::steady_clock::time_point snapshot_after=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    void require_eye_dimensions(unsigned eye_index,unsigned image_index,const char* stage) const {
        const auto& eye=eyes.at(eye_index);
        auto* texture=eye.images.at(image_index).texture;
        const unsigned width=stereo?eye_plan.eye_width:receiver->width;
        if(!texture)throw std::runtime_error("XR returned a null D3D12 swapchain image");
        const auto desc=texture->GetDesc();
        if(desc.Width==width&&desc.Height==receiver->height)return;
        // Log the actual resource, not only the requested swapchain dimensions.
        // A runtime may erroneously alias another live chain after a menu resize.
        fprintf(stderr,"{\"xr_image_dimension_mismatch\":true,\"stage\":\"%s\",\"eye\":%u,\"image_index\":%u,\"chain\":\"%p\",\"expected\":[%u,%u],\"actual\":[%llu,%u],\"format\":%u,\"array_size\":%u,\"samples\":%u}\n",
                stage,eye_index,image_index,reinterpret_cast<void*>(eye.chain),width,receiver->height,
                static_cast<unsigned long long>(desc.Width),desc.Height,unsigned(desc.Format),unsigned(desc.DepthOrArraySize),desc.SampleDesc.Count);
        fflush(stderr);
        throw std::runtime_error("XR image dimensions differ from exact crop");
    }
    void save_snapshot(){
        void* mapped{};D3D12_RANGE range{0,size_t(snapshot_bytes)};
        dx_check(snapshot_buffer->Map(0,&range,&mapped),"GPU snapshot map");
        const auto* data=static_cast<const unsigned char*>(mapped)+snapshot_layout.Offset;
        auto format=snapshot_layout.Footprint.Format;
        // Preserve original pixels for UI placement/legibility checks. The GPU
        // copy already contains the complete image; a 640-wide thumbnail hid
        // glyphs and made it impossible to distinguish missing from tiny HUD.
        const unsigned width=receiver->width,height=receiver->height;
        std::vector<unsigned char> rgb(size_t(width)*height*3);
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
            const auto* p=data+size_t(y)*snapshot_layout.Footprint.RowPitch+size_t(x)*4;
            auto* out=rgb.data()+(size_t(y)*width+x)*3;
            if(format==DXGI_FORMAT_R10G10B10A2_UNORM){uint32_t value{};memcpy(&value,p,4);for(unsigned c=0;c<3;++c)out[c]=static_cast<unsigned char>(((value>>(c*10))&1023)*255/1023);}
            else {bool bgra=format==DXGI_FORMAT_B8G8R8A8_UNORM || format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;out[0]=p[bgra?2:0];out[1]=p[1];out[2]=p[bgra?0:2];}
        }
        D3D12_RANGE written{0,0};snapshot_buffer->Unmap(0,&written);
        const auto path=snapshot_count?snapshot_path+"."+std::to_string(snapshot_count)+".ppm":snapshot_path;
        // CREATE_NEW is atomic: never truncate a previous diagnostic, even if
        // another writer races this save. A partial failed file is retained.
        HANDLE file=CreateFileA(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("GPU snapshot file unavailable or already exists");
        char header[96]{};const int length=sprintf_s(header,"P6\n%u %u\n255\n",width,height);
        DWORD header_written{},pixels_written{};
        const bool complete=length>0&&WriteFile(file,header,DWORD(length),&header_written,nullptr)&&header_written==DWORD(length)&&
            rgb.size()<=MAXDWORD&&WriteFile(file,rgb.data(),DWORD(rgb.size()),&pixels_written,nullptr)&&pixels_written==rgb.size();
        const bool closed=CloseHandle(file)!=FALSE;
        if(!complete||!closed)throw std::runtime_error("GPU snapshot write incomplete");
        const auto saved_ns=gears_vr::monotonic_ns();
        printf("{\"gpu_snapshot_size\":[%u,%u],\"source_format\":%u,\"snapshot_index\":%u,\"snapshot_limit\":%u,\"ticket\":%llu,\"source_serial\":%llu,\"saved_monotonic_ns\":%lld,\"resized\":false}\n",
               width,height,unsigned(format),snapshot_count,snapshot_limit,static_cast<unsigned long long>(snapshot_ticket),
               static_cast<unsigned long long>(snapshot_source_serial),static_cast<long long>(saved_ns));fflush(stdout);
        ++snapshot_count;
        snapshot_saved=true;snapshot_buffer.Reset();
    }
public:
    unsigned rendered=0,copies=0;
    unsigned projection_rendered=0,retained_projections=0,rejected_late_images=0;
    bool snapshot_saved=false;
    GameGpuScreen(ID3D12Device* d,ID3D12CommandQueue* q,DWORD pid,const char* snapshot=nullptr,bool simulated=false,
                  gears_eye_chain::Limits const* stereo_limits=nullptr,bool retain_image=false):allow_simulated(simulated),stereo(stereo_limits!=nullptr),device(d),queue(q),retain_stereo_image(retain_image),snapshot_path(snapshot?snapshot:""){
        try{
        receiver->open(d,pid,allow_simulated);
        if(receiver->generation()>1&&!snapshot_path.empty())snapshot_path+=".g"+std::to_string(receiver->generation());
        if(stereo&&!gears_eye_chain::make_plan(receiver->width,receiver->height,*stereo_limits,eye_plan))
            throw std::runtime_error("Stereo source exceeds advertised XR per-eye/system size or sample limits");
        uint32_t count{};xr_check(xrEnumerateSwapchainFormats(session,0,&count,nullptr),"GPU game formats");
        std::vector<int64_t> formats(count);xr_check(xrEnumerateSwapchainFormats(session,count,&count,formats.data()),"GPU game formats read");
        DXGI_FORMAT format=receiver->format;
        if(std::find(formats.begin(),formats.end(),int64_t(format))==formats.end()){
            format=DXGI_FORMAT_UNKNOWN;
            for(auto candidate:{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM})
                if(std::find(formats.begin(),formats.end(),int64_t(candidate))!=formats.end()){format=candidate;break;}
            if(!format)throw std::runtime_error("No supported SDR GPU conversion target");
        }
        for(unsigned i=0;i<(stereo?2u:1u);++i) {
            auto& eye=eyes[i];
            // Each conversion has immutable descriptor storage for this submit.
            if(format!=receiver->format)eye.blit=std::make_unique<gears_vr::GpuBlit>(d,format);
            XrSwapchainCreateInfo create{XR_TYPE_SWAPCHAIN_CREATE_INFO};create.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
            create.format=format;create.width=stereo?eye_plan.eye_width:receiver->width;
            create.height=receiver->height;create.sampleCount=1;create.faceCount=1;create.arraySize=1;create.mipCount=1;
            xr_check(xrCreateSwapchain(session,&create,&eye.chain),"GPU game swapchain");
            if(i && eye.chain==eyes[0].chain) {
                // A broken runtime can return the first live handle again.
                // Leave ownership with eye zero so constructor cleanup destroys
                // it once; never acquire the same chain as two separate eyes.
                eye.chain=XR_NULL_HANDLE;
                throw std::runtime_error("XR returned the same swapchain handle for both eyes");
            }
            xr_check(xrEnumerateSwapchainImages(eye.chain,0,&count,nullptr),"GPU game images");
            if(!count)throw std::runtime_error("Empty GPU game swapchain");
            eye.images.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
            xr_check(xrEnumerateSwapchainImages(eye.chain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(eye.images.data())),"GPU game images read");
            if(count!=eye.images.size())throw std::runtime_error("GPU game swapchain image count changed during enumeration");
            for(unsigned image_index=0;image_index<count;++image_index)require_eye_dimensions(i,image_index,"enumeration");
        }
        printf("{\"gpu_source_size\":[%u,%u],\"xr_image_size\":[%u,%u],\"xr_eye_chains\":%u,\"pixel_exact_crop\":%s,\"native_size_decoupled\":false}\n",
               receiver->width,receiver->height,stereo?eye_plan.eye_width:receiver->width,receiver->height,stereo?2u:1u,stereo?"true":"false");fflush(stdout);
        dx_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"GPU game allocator");
        dx_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&commands)),"GPU game commands");dx_check(commands->Close(),"GPU game initial close");
        dx_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"GPU game fence");event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if(!event)throw std::runtime_error("GPU game fence event unavailable");
        if(!snapshot_path.empty()){
            wchar_t name[96]{};swprintf_s(name,L"Local\\GearsVR.XR.Snapshot.%lu",GetCurrentProcessId());
            SetLastError(0);snapshot_event=CreateEventW(nullptr,FALSE,FALSE,name);
            if(!snapshot_event||GetLastError()==ERROR_ALREADY_EXISTS)throw std::runtime_error("GPU snapshot request event unavailable or already exists");
        }
        }catch(...){for(auto& eye:eyes)if(eye.chain){xrDestroySwapchain(eye.chain);eye.chain=XR_NULL_HANDLE;}if(event){CloseHandle(event);event=nullptr;}if(snapshot_event){CloseHandle(snapshot_event);snapshot_event=nullptr;}throw;}
    }
    ~GameGpuScreen(){
        // Request event has no GPU/fence dependency; close even on unsafe GPU teardown.
        if(snapshot_event){CloseHandle(snapshot_event);snapshot_event=nullptr;}
        if(in_flight && fence && xr_image_work_complete(fence->GetCompletedValue(),serial))in_flight=false;
        bool unsafe=in_flight;
        if(!unsafe)for(auto& eye:eyes)if(eye.acquired) {
            // Never release an image whose wait failed/timed out, or one whose
            // GPU work is not complete. Retain the session's ownership instead.
            XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            if(!eye.waited||XR_FAILED(xrReleaseSwapchainImage(eye.chain,&release)))unsafe=true;
            else eye.acquired=eye.waited=false;
        }
        if(unsafe){xr_gpu_teardown_unsafe=true;receiver.release();commands.Detach();allocator.Detach();fence.Detach();device.Detach();queue.Detach();snapshot_buffer.Detach();for(auto& eye:eyes)eye.blit.release();}
        else {
            for(auto& eye:eyes)if(eye.chain)xrDestroySwapchain(eye.chain);
            if(event)CloseHandle(event);
        }
    }
    static bool producer_ready(DWORD pid,ProjectionStatus* status=nullptr,bool simulated=false){
        HANDLE mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,gears_vr::gpu_name(pid,L"metadata").c_str());
        if(!mapping)return false;
        auto* packet=static_cast<const gears_vr::GpuPacket*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(gears_vr::GpuPacket)));
        if(status && packet && packet->ready==1 && (packet->magic!=0x47564731||packet->version!=gears_vr::gpu_packet_version||packet->size!=sizeof(*packet)||packet->pid!=pid||packet->allow_simulated!=unsigned(simulated)))*status=ProjectionStatus::producer_structure_invalid;
        bool ready=packet&&packet->magic==0x47564731&&packet->version==gears_vr::gpu_packet_version&&packet->size==sizeof(*packet)&&packet->pid==pid&&packet->allow_simulated==unsigned(simulated)&&packet->ready==1&&!packet->stopped;
        if(packet)UnmapViewOfFile(packet);CloseHandle(mapping);return ready;
    }
    // Poll on every XR frame, even when policy disables projection. Use this
    // receiver to drain the old slot without acquiring an XR image or copying.
    bool poll_world_handoff(){
        const auto world=receiver->world_state();
        const bool changed=world.valid&&observed_world_generation!=world.generation;
        if(changed)observed_world_generation=world.generation;
        if(!world.valid||world.quiescing||changed){current=false;copied_ticket={};copied_sequence=0;copied_world_generation=0;}
        if(!world.valid||world.quiescing){
            // Submitted work retains its fence, resources and XR ownership.
            // Only the existing completion path may retire that work.
            if(world.valid&&!in_flight){
                bool owned=false;for(auto const& eye:eyes)owned|=eye.acquired;
                if(!owned&&receiver->acquire(queue.Get()))receiver->acknowledge(queue.Get());
            }
            return false;
        }
        return true;
    }
    bool update_image(uint64_t expected_session=0,ProjectionStatus* status=nullptr,uint64_t space_epoch=0,int64_t change_time=0,int64_t minimum_sample_ns=0){
        if(!poll_world_handoff()){if(status)*status=ProjectionStatus::source_reconfiguration;return false;}
        if(receiver->stopped()||in_flight)return false;
        auto now=std::chrono::steady_clock::now();
        if(receiver->acquire(queue.Get())){
            const auto world=receiver->world_state();
            if(!world.valid||world.quiescing||receiver->acquired_world_generation()!=world.generation||
               ((stereo||world.minimum_sample_ns>0)&&!gears_vr::gpu_world_image_current(world,receiver->acquired_world_generation(),receiver->ticket().sample.received_ns))){
                receiver->acknowledge(queue.Get());current=false;if(status)*status=ProjectionStatus::source_reconfiguration;return false;
            }
            if(stereo) {
                const auto& ticket=receiver->ticket();
                auto validity=projection_ticket_status(ticket,receiver->width,receiver->height,expected_session,gears_vr::monotonic_ns(),allow_simulated,space_epoch,change_time,
                    retain_stereo_image?projection_new_image_max_age:100000000);
                if(validity==ProjectionStatus::submitted&&!gears_eye_chain::ticket_crops_match(eye_plan,ticket))validity=ProjectionStatus::image_rectangle;
                if(validity==ProjectionStatus::submitted&&ticket.sample.received_ns<=minimum_sample_ns)validity=ProjectionStatus::menu_recovery_pending;
                if(validity!=ProjectionStatus::submitted) {
                    receiver->acknowledge(queue.Get());if(status)*status=validity;
                    if(retain_stereo_image&&validity==ProjectionStatus::stale_ticket){
                        ++rejected_late_images;
                        // render_projection must still validate the original
                        // copied ticket's age, session, origin and menu epoch.
                        return current;
                    }
                    current=false;return false;
                }
            }
            std::array<ID3D12Resource*,2> destinations{};
            const unsigned count=stereo?2u:1u;
            for(unsigned i=0;i<count;++i) {
                auto& eye=eyes[i];uint32_t index{};
                XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                xr_check(xrAcquireSwapchainImage(eye.chain,&acquire,&index),"GPU game acquire");eye.acquired=true;
                XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=1000000000;
                const auto waited=xrWaitSwapchainImage(eye.chain,&wait);xr_check(waited,"GPU game wait");
                if(!xr_image_wait_succeeded(waited))throw std::runtime_error(waited==XR_TIMEOUT_EXPIRED?
                    "GPU game swapchain wait timeout; image not written or released":"GPU game swapchain wait did not succeed; image not written or released");
                eye.waited=true;destinations[i]=eye.images.at(index).texture;
                require_eye_dimensions(i,index,"acquire");
            }
            dx_check(allocator->Reset(),"GPU game reset");dx_check(commands->Reset(allocator.Get(),nullptr),"GPU game commands reset");
            const auto source_state=eyes[0].blit?D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATE_COPY_SOURCE;
            gears_vr::gpu_barrier(commands.Get(),receiver->texture.Get(),D3D12_RESOURCE_STATE_COMMON,source_state);
            for(unsigned i=0;i<count;++i) {
                auto* image=destinations[i];auto& eye=eyes[i];
                if(eye.blit) {
                    if(stereo)eye.blit->draw_crop(commands.Get(),receiver->texture.Get(),image,unsigned(eye_plan.crops[i].x),0);
                    else eye.blit->draw(commands.Get(),receiver->texture.Get(),image);
                }else{
                    gears_vr::gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);
                    if(stereo) {
                        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=image;to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=receiver->texture.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                        const auto r=eye_plan.crops[i];D3D12_BOX box{unsigned(r.x),0,0,unsigned(r.x+r.width),unsigned(r.height),1};
                        commands->CopyTextureRegion(&to,0,0,0,&from,&box);
                    }else commands->CopyResource(image,receiver->texture.Get());
                    gears_vr::gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);
                }
            }
            gears_vr::gpu_barrier(commands.Get(),receiver->texture.Get(),source_state,D3D12_RESOURCE_STATE_COMMON);
            bool requested=false;
            if(snapshot_event){
                // Poll only after admitting/acquiring a valid new image. Auto
                // reset coalesces requests; no cached-image or per-frame readback.
                const auto request=WaitForSingleObject(snapshot_event,0);
                if(request!=WAIT_OBJECT_0&&request!=WAIT_TIMEOUT)throw std::runtime_error("GPU snapshot request wait failed");
                requested=request==WAIT_OBJECT_0;
            }
            const bool automatic=!snapshot_path.empty()&&!snapshot_auto_taken&&now>=snapshot_after;
            if((requested||automatic)&&snapshot_count>=snapshot_limit){
                if(requested){printf("{\"gpu_snapshot_request_rejected\":true,\"snapshot_limit\":%u}\n",snapshot_limit);fflush(stdout);}
                snapshot_auto_taken=true;
            }
            if((requested||automatic)&&snapshot_count<snapshot_limit){
                if(automatic)snapshot_auto_taken=true;
                snapshot_ticket=receiver->ticket().id;
                // Diagnostic output remains full SBS, independent of eye chain layout.
                auto* image=stereo?receiver->texture.Get():destinations[0];auto desc=image->GetDesc();device->GetCopyableFootprints(&desc,0,1,0,&snapshot_layout,nullptr,nullptr,&snapshot_bytes);
                D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
                D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=snapshot_bytes;buffer.Height=1;buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                dx_check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&snapshot_buffer)),"GPU snapshot buffer");
                const auto snapshot_state=stereo?D3D12_RESOURCE_STATE_COMMON:D3D12_RESOURCE_STATE_RENDER_TARGET;
                gears_vr::gpu_barrier(commands.Get(),image,snapshot_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
                D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=snapshot_buffer.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=snapshot_layout;
                D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=image;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                commands->CopyTextureRegion(&to,0,0,0,&from,nullptr);
                gears_vr::gpu_barrier(commands.Get(),image,D3D12_RESOURCE_STATE_COPY_SOURCE,snapshot_state);
            }
            dx_check(commands->Close(),"GPU game close");ID3D12CommandList* lists[]={commands.Get()};++serial;in_flight=true;queue->ExecuteCommandLists(1,lists);
            receiver->acknowledge(queue.Get());dx_check(queue->Signal(fence.Get(),serial),"GPU game local fence");
            if(fence->GetCompletedValue()==~UINT64(0))throw std::runtime_error("GPU game device removed");
            if(fence->GetCompletedValue()<serial){dx_check(fence->SetEventOnCompletion(serial,event),"GPU game fence event");if(WaitForSingleObject(event,3000)!=WAIT_OBJECT_0)throw std::runtime_error("GPU game fence timeout");}
            if(!xr_image_work_complete(fence->GetCompletedValue(),serial))throw std::runtime_error("GPU game completion unconfirmed after fence wait");
            in_flight=false;
            if(snapshot_buffer){
                // sequence() is the last acknowledged producer serial. Read it
                // only after this image's successful acknowledge; no next
                // acquire can intervene on this render thread. Ticket remains
                // the value frozen when this readback copy was recorded.
                snapshot_source_serial=receiver->sequence();save_snapshot();
            }
            for(unsigned i=0;i<count;++i) {
                auto& eye=eyes[i];XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                xr_check(xrReleaseSwapchainImage(eye.chain,&release),"GPU game release");eye.acquired=eye.waited=false;
            }
            copied_ticket=receiver->ticket();copied_sequence=receiver->sequence();copied_world_generation=receiver->acquired_world_generation();
            ++copies;current=true;last_frame=now;
        }
        const auto world=receiver->world_state();
        if(!world.valid||world.quiescing||copied_world_generation!=world.generation||(stereo&&copied_ticket.sample.received_ns<=world.minimum_sample_ns))current=false;
        if(!current || now-last_frame>std::chrono::seconds(retain_stereo_image?2:1))return false;
        return true;
    }
    bool render(XrCompositionLayerQuad& quad){
        if(stereo)throw std::runtime_error("Stereo eye chains cannot render a mono quad");
        if(!update_image())return false;
        quad={XR_TYPE_COMPOSITION_LAYER_QUAD};quad.space=view_space;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;
        quad.pose.orientation.w=1;quad.pose.position.z=-2;quad.size={1.6f,1.6f*receiver->height/receiver->width};
        quad.subImage.swapchain=eyes[0].chain;quad.subImage.imageRect.extent={int32_t(receiver->width),int32_t(receiver->height)};++rendered;return true;
    }
    bool producer_stopped() const{return receiver->stopped();}
    bool reconfiguring()const{return receiver->reconfiguring();}
    std::uint64_t resource_generation()const{return receiver->generation();}
    bool retire_for_resize(){
        if(!reconfiguring()||in_flight)return false;
        if(serial&&!xr_image_work_complete(fence->GetCompletedValue(),serial))return false;
        for(auto const& eye:eyes)if(eye.acquired)return false;
        // Called on the XR render thread before the next submission. Destroying
        // each retired eye chain also lets the runtime finish its own use.
        for(auto& eye:eyes)if(eye.chain){
            xr_check(xrDestroySwapchain(eye.chain),"retire resized eye chain");
            eye.chain=XR_NULL_HANDLE;eye.images.clear();eye.blit.reset();
        }
        current=false;return receiver->release_for_resize();
    }
    // Same render thread only; local retained snapshot, no shared-memory/GPU
    // operation. sequence() is the acknowledged producer serial, not our fence.
    outage_diagnostics::ImageSnapshot diagnostic_snapshot() const noexcept {
        outage_diagnostics::ImageSnapshot out;
        out.receiver_valid=true;out.copies=copies;out.acquired_serial=copied_sequence;
        const auto& ticket=copied_ticket;const auto& sample=ticket.sample;
        out.ticket_valid=ticket.id!=0&&sample.sequence!=0;
        if(out.ticket_valid){out.ticket_id=ticket.id;out.session=sample.session;out.sequence=sample.sequence;
            out.received_ns=sample.received_ns;out.source=unsigned(sample.source);
            out.head_flags=sample.head_flags;out.view_flags=sample.view_flags;
            out.head_position={sample.head.position.x,sample.head.position.y,sample.head.position.z};
            out.head_orientation={sample.head.orientation.x,sample.head.orientation.y,sample.head.orientation.z,sample.head.orientation.w};}
        return out;
    }
private:
    bool prepare_ticketed_image(uint64_t expected_session,ProjectionStatus& status,int64_t minimum_sample_ns,uint64_t space_epoch,int64_t space_change_time){
        if(reconfiguring()){status=ProjectionStatus::source_reconfiguration;return false;}
        if(!stereo)throw std::runtime_error("Ticketed display requires admitted stereo eye chains");
        if(receiver->stopped()){status=ProjectionStatus::producer_stopped;return false;}
        status=ProjectionStatus::waiting_image;
        if(!update_image(expected_session,&status,space_epoch,space_change_time,minimum_sample_ns))return false;
        if(reconfiguring()){status=ProjectionStatus::source_reconfiguration;return false;}
        const auto& ticket=copied_ticket;const auto& sample=ticket.sample;
        if(!gears_vr::gpu_world_image_current(receiver->world_state(),copied_world_generation,sample.received_ns)){
            current=false;status=ProjectionStatus::source_reconfiguration;return false;
        }
        const auto now=gears_vr::monotonic_ns();
        status=projection_ticket_status(ticket,receiver->width,receiver->height,expected_session,now,allow_simulated,space_epoch,space_change_time,
            retain_stereo_image?projection_retained_image_max_age:100000000);
        if(status!=ProjectionStatus::submitted)return false;
        if(sample.received_ns<=minimum_sample_ns){status=ProjectionStatus::menu_recovery_pending;return false;}
        return true;
    }
public:
    bool render_eye_menu(XrCompositionLayerQuad& quad,uint64_t expected_session,ProjectionStatus& status,int64_t menu_enter_ns,uint64_t space_epoch,int64_t space_change_time){
        // Only the explicit pause-menu branch calls this. It is a common 2D
        // panel from one completed eye, never a projection or a second consumer.
        if(menu_enter_ns<=0){status=ProjectionStatus::menu_recovery_pending;return false;}
        if(!prepare_ticketed_image(expected_session,status,menu_enter_ns,space_epoch,space_change_time))return false;
        gears_menu_panel::Plan plan{};
        if(!gears_menu_panel::make_plan(receiver->width,receiver->height,plan)){
            status=ProjectionStatus::image_rectangle;return false;}
        quad={XR_TYPE_COMPOSITION_LAYER_QUAD};quad.space=view_space;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;
        quad.pose.orientation.w=1;quad.pose.position.z=-2;
        quad.size={plan.metres_width,plan.metres_height};
        quad.subImage.swapchain=eyes[0].chain;
        quad.subImage.imageRect={{plan.x,plan.y},{plan.width,plan.height}};
        return true;
    }
    bool render_projection(XrCompositionLayerProjection& layer,XrCompositionLayerProjectionView (&views)[2],uint64_t expected_session,ProjectionStatus& status,int64_t minimum_sample_ns=0,uint64_t space_epoch=1,int64_t space_change_time=0){
        const auto copies_before=copies;
        if(!prepare_ticketed_image(expected_session,status,minimum_sample_ns,space_epoch,space_change_time))return false;
        const auto& ticket=copied_ticket;const auto& sample=ticket.sample;
        const auto now=gears_vr::monotonic_ns();
        for(unsigned i=0;i<2;++i){
            const auto& rect=ticket.rect[i];const auto& eye=sample.eyes[i];
            auto& view=views[i];view={XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            view.pose.position={float(eye.pose.position.x),float(eye.pose.position.y),float(eye.pose.position.z)};
            view.pose.orientation={float(eye.pose.orientation.x),float(eye.pose.orientation.y),float(eye.pose.orientation.z),float(eye.pose.orientation.w)};
            view.fov={float(eye.fov.left),float(eye.fov.right),float(eye.fov.up),float(eye.fov.down)};
            view.subImage.swapchain=eyes[i].chain;view.subImage.imageRect={{0,0},{rect.width,rect.height}};
        }
        layer={XR_TYPE_COMPOSITION_LAYER_PROJECTION};layer.space=local;layer.viewCount=2;layer.views=views;
        if(copies==copies_before&&now-sample.received_ns>100000000)++retained_projections;
        ++projection_rendered;return true;
    }
};
