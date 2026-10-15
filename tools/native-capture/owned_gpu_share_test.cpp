#include "../vr-common/gpu_share.h"
#include <d3d12sdklayers.h>
#include <vector>
#include <cstdio>
#include <memory>
using namespace gears_vr;
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct GPU {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12Fence> fence;
    UINT64 serial{};
    GPU(){gpu_check(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"device");
        D3D12_COMMAND_QUEUE_DESC d{};gpu_check(device->CreateCommandQueue(&d,IID_PPV_ARGS(&queue)),"queue");
        gpu_check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
        gpu_check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");
        gpu_check(list->Close(),"initial close");gpu_check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");}
    bool finish(){if(FAILED(queue->Signal(fence.Get(),++serial)))return false;const auto until=GetTickCount64()+5000;
        do{const auto n=fence->GetCompletedValue();if(n==~UINT64(0))return false;if(n>=serial)return true;Sleep(1);}while(GetTickCount64()<until);return false;}
    void begin(){require(finish(),"GPU drain before reuse");gpu_check(allocator->Reset(),"reset allocator");gpu_check(list->Reset(allocator.Get(),nullptr),"reset list");}
    void submit(){gpu_check(list->Close(),"close list");ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);require(finish(),"GPU completion");}
    void verify(){ComPtr<ID3D12InfoQueue> info;if(FAILED(device.As(&info)))return;
        for(UINT64 i=0;i<info->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T bytes{};gpu_check(info->GetMessage(i,nullptr,&bytes),"debug size");
            std::vector<unsigned char> memory(bytes);auto* m=reinterpret_cast<D3D12_MESSAGE*>(memory.data());gpu_check(info->GetMessage(i,m,&bytes),"debug message");
            if(m->Severity==D3D12_MESSAGE_SEVERITY_ERROR||m->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION)throw std::runtime_error(m->pDescription);}}
    ~GPU(){if(queue&&fence&&!finish()){list.Detach();allocator.Detach();fence.Detach();queue.Detach();device.Detach();}}
};
static ComPtr<ID3D12Resource> texture(ID3D12Device* device,unsigned width=64,bool rt=true,D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON){
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=32;d.DepthOrArraySize=1;d.MipLevels=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count=1;d.Flags=rt?D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;ComPtr<ID3D12Resource> result;
    gpu_check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&result)),"texture");return result;
}
static StereoImageTicket ticket(unsigned id){StereoImageTicket t{};t.id=id;auto&s=t.sample;
    s.session=17;s.sequence=id;s.received_ns=1000000000+id;s.predicted_time=2000000000+id;
    s.head_flags=s.view_flags=15;s.focused=s.should_render=1;s.head.orientation={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.orientation={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.8,.7,.75,-.65};}
    t.rect[0]={0,0,32,32};t.rect[1]={32,0,32,32};return t;}
static bool same_ticket(StereoImageTicket const&a,StereoImageTicket const&b){if(a.id!=b.id||!stereo_same(a.sample,b.sample))return false;
    for(unsigned i=0;i<2;++i)if(a.rect[i].x!=b.rect[i].x||a.rect[i].y!=b.rect[i].y||a.rect[i].width!=b.rect[i].width||a.rect[i].height!=b.rect[i].height)return false;return true;}
struct Metadata {HANDLE map{};GpuPacket* packet{};
    Metadata(){map=OpenFileMappingW(FILE_MAP_READ,FALSE,gpu_name(GetCurrentProcessId(),L"metadata").c_str());
        require(map!=nullptr,"metadata map");packet=static_cast<GpuPacket*>(MapViewOfFile(map,FILE_MAP_READ,0,0,sizeof(GpuPacket)));require(packet!=nullptr,"metadata view");}
    ~Metadata(){if(packet)UnmapViewOfFile(packet);if(map)CloseHandle(map);}};
static void rejects(GPU& gpu){auto source=texture(gpu.device.Get());GpuPublisher p;
    require(!p.bind_resource(nullptr,source.Get(),1,D3D12_RESOURCE_STATE_COMMON),"null queue accepted");
    require(!p.bind_resource(gpu.queue.Get(),nullptr,1,D3D12_RESOURCE_STATE_COMMON),"null resource accepted");
    require(!p.bind_resource(gpu.queue.Get(),source.Get(),0,D3D12_RESOURCE_STATE_COMMON),"zero generation accepted");
    require(!p.bind_resource(gpu.queue.Get(),source.Get(),1,D3D12_RESOURCE_STATE_COPY_SOURCE),"unsupported state accepted");
    require(!p.bind_resource(gpu.queue.Get(),source.Get(),1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),"combined state accepted");
    auto odd=texture(gpu.device.Get(),63);require(!p.bind_resource(gpu.queue.Get(),odd.Get(),1,D3D12_RESOURCE_STATE_COMMON),"odd SBS accepted");
    auto no_rt=texture(gpu.device.Get(),64,false);require(!p.bind_resource(gpu.queue.Get(),no_rt.Get(),1,D3D12_RESOURCE_STATE_RENDER_TARGET),"missing RTV flag accepted");
    D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_COPY;ComPtr<ID3D12CommandQueue> copy;
    gpu_check(gpu.device->CreateCommandQueue(&q,IID_PPV_ARGS(&copy)),"copy queue");require(!p.bind_resource(copy.Get(),source.Get(),1,D3D12_RESOURCE_STATE_COMMON),"copy queue accepted");
    // D3D12CreateDevice is a per-adapter singleton within one process; calling
    // it twice on the default adapter is not a foreign-device counterexample.
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;ComPtr<ID3D12Device> other_device;
    gpu_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"factory");gpu_check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)),"WARP adapter");
    gpu_check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&other_device)),"other adapter device");
    auto foreign=texture(other_device.Get());require(!p.bind_resource(gpu.queue.Get(),foreign.Get(),1,D3D12_RESOURCE_STATE_COMMON),"foreign device accepted");
    require(p.state==0,"rejected bind mutated publisher");gpu.verify();
}
static void exercise(GPU& gpu,D3D12_RESOURCE_STATES idle){
    auto source=texture(gpu.device.Get(),64,true,idle),other=texture(gpu.device.Get());
    GpuPublisher publisher;require(publisher.bind_resource(gpu.queue.Get(),source.Get(),7,idle),"owned bind");
    require(!publisher.bind_resource(gpu.queue.Get(),other.Get(),8,idle),"binding replaced");
    auto* retained_source=source.Get();source.Reset(); // Publisher must own the sole remaining resource references.
    require(publisher.prepare(),"owned prepare");
    GpuReceiver receiver;receiver.open(gpu.device.Get(),GetCurrentProcessId());Metadata metadata;
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;ComPtr<ID3D12DescriptorHeap> rtv;
    gpu_check(gpu.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv)),"RTV");gpu.device->CreateRenderTargetView(retained_source,nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    auto td=receiver.texture->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes{};
    gpu.device->GetCopyableFootprints(&td,0,1,0,&fp,nullptr,nullptr,&bytes);
    D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=bytes;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;ComPtr<ID3D12Resource> readback;
    gpu_check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)),"readback");
    const auto invalid=[&](ID3D12Resource* resource,UINT64 generation,D3D12_RESOURCE_STATES state){
        const auto sequence=metadata.packet->sequence;const auto before=metadata.packet->ticket;GpuCaptureTrace trace;
        require(!publisher.capture_resource(resource,generation,state,ticket(99),&trace),"invalid capture accepted");
        require(trace.reason==GpuCaptureReason::owned_binding_mismatch&&publisher.state==2&&metadata.packet->sequence==sequence&&same_ticket(metadata.packet->ticket,before),"rejection changed frozen slot");};
    invalid(other.Get(),7,idle);invalid(retained_source,8,idle);invalid(retained_source,7,D3D12_RESOURCE_STATE_COPY_DEST);invalid(nullptr,7,idle);
    auto malformed=ticket(1);malformed.rect[1].x=31;GpuCaptureTrace invalid_ticket_trace;
    require(!publisher.capture_resource(retained_source,7,idle,malformed,&invalid_ticket_trace)&&
        invalid_ticket_trace.reason==GpuCaptureReason::ticket_invalid&&metadata.packet->sequence==0,"invalid eye rectangles published");
    require(!publisher.capture(nullptr),"owned binding accepted swapchain capture");
    for(unsigned frame=1;frame<=2;++frame){
        gpu.begin();if(idle!=D3D12_RESOURCE_STATE_RENDER_TARGET)gpu_barrier(gpu.list.Get(),retained_source,idle,D3D12_RESOURCE_STATE_RENDER_TARGET);
        const float red[]={1,0,0,1},blue[]={0,0,1,1},white[]={1,1,1,1};
        const D3D12_RECT l{0,0,32,32},r{32,0,64,32},corner{0,0,2,2};
        gpu.list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),frame==1?red:blue,1,&l);
        gpu.list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),frame==1?blue:red,1,&r);
        gpu.list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),white,1,&corner);
        if(idle!=D3D12_RESOURCE_STATE_RENDER_TARGET)gpu_barrier(gpu.list.Get(),retained_source,D3D12_RESOURCE_STATE_RENDER_TARGET,idle);gpu.submit();
        auto supplied=ticket(frame),expected=supplied;GpuCaptureTrace trace;
        require(publisher.capture_resource(retained_source,7,idle,supplied,&trace),"owned publication");
        require(trace.reason==GpuCaptureReason::published,"capture trace");supplied.sample.head.position.x=900;supplied.id=999;
        require(gpu.finish(),"producer copy complete");publisher.poll();
        GpuIdleSnapshot idle_before{};
        require(!publisher.try_idle(idle_before)&&idle_before.valid&&idle_before.serial==frame&&
            idle_before.ready_completed>=frame&&idle_before.done_completed<frame,"producer drain falsely proved receiver idle");
        require(receiver.acquire(gpu.queue.Get())&&same_ticket(receiver.ticket(),expected),"private copied ticket");
        auto busy_ticket=ticket(frame+10);require(!publisher.capture_resource(retained_source,7,idle,busy_ticket,&trace)&&
            trace.reason==GpuCaptureReason::consumer_not_done&&same_ticket(receiver.ticket(),expected)&&same_ticket(metadata.packet->ticket,expected),"busy overwrote ticket");
        gpu.begin();gpu_barrier(gpu.list.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=fp;
        src.pResource=receiver.texture.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;gpu.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        gpu_barrier(gpu.list.Get(),receiver.texture.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);gpu.submit();
        void* mapped{};D3D12_RANGE range{0,SIZE_T(bytes)};gpu_check(readback->Map(0,&range,&mapped),"map pixels");
        bool pixels=true;for(unsigned y=0;y<32;++y)for(unsigned x=0;x<64;++x){auto* p=static_cast<unsigned char*>(mapped)+size_t(y)*fp.Footprint.RowPitch+x*4;
            const bool white_pixel=x<2&&y<2,red_pixel=(x<32)==(frame==1);
            if(p[0]!=(white_pixel||red_pixel?255:0)||p[1]!=(white_pixel?255:0)||p[2]!=(white_pixel||!red_pixel?255:0)||p[3]!=255)pixels=false;}
        D3D12_RANGE empty{};readback->Unmap(0,&empty);require(pixels,"full two-eye native pixels differ");
        receiver.acknowledge(gpu.queue.Get());require(gpu.finish(),"consumer ACK");require(same_ticket(receiver.ticket(),expected),"ACK changed private ticket");
        GpuIdleSnapshot idle_after{};require(publisher.try_idle(idle_after)&&idle_after.serial==frame&&
            idle_after.generation==idle_before.generation,"acknowledged slot not observed idle");
        auto removed=idle_after;removed.ready_completed=~std::uint64_t(0);require(!gpu_idle_complete(removed),"removed ready fence accepted");
        removed=idle_after;removed.done_completed=~std::uint64_t(0);require(!gpu_idle_complete(removed),"removed done fence accepted");
        auto changing=idle_after;changing.reconfiguring=true;require(!gpu_idle_complete(changing),"resizing slot accepted idle");
        changing=idle_after;changing.in_flight=true;require(!gpu_idle_complete(changing),"in-flight slot accepted idle");
    }
    const auto resource_generation=receiver.generation();
    require(receiver.world_state().valid&&receiver.world_state().generation==1&&!receiver.world_state().quiescing,"initial world");
    require(!publisher.begin_world_rebind(0)&&!publisher.begin_world_rebind(2)&&!publisher.commit_world_rebind(1,2,100),"invalid world begin/commit");
    require(publisher.capture_resource(retained_source,7,idle,ticket(3)),"final old-world publication");
    require(gpu.finish(),"old-world producer completion");publisher.poll();
    require(publisher.begin_world_rebind(1),"world begin");
    const auto old_sequence=metadata.packet->sequence;const auto old_ticket=metadata.packet->ticket;GpuCaptureTrace quiesced{};
    require(!publisher.capture_resource(retained_source,7,idle,ticket(4),&quiesced)&&quiesced.reason==GpuCaptureReason::world_quiescing&&
        metadata.packet->sequence==old_sequence&&same_ticket(metadata.packet->ticket,old_ticket),"quiescence mutated slot");
    require(!publisher.commit_world_rebind(1,2,1000000003),"unacquired old slot committed");
    require(receiver.acquire(gpu.queue.Get())&&receiver.acquired_world_generation()==1,"old slot acquire during quiescence");
    require(!gpu_world_image_current(receiver.world_state(),receiver.acquired_world_generation(),receiver.ticket().sample.received_ns),"quiescing slot displayable");
    require(!publisher.commit_world_rebind(1,2,1000000003),"pending old slot committed");
    receiver.acknowledge(gpu.queue.Get());require(gpu.finish(),"old-world ACK");
    require(!publisher.commit_world_rebind(1,3,1000000003)&&!publisher.commit_world_rebind(1,2,0)&&!publisher.commit_world_rebind(2,3,1000000003)&&
        receiver.world_state().quiescing,"invalid commit reopened world");
    bool invoked=false;
    require(!publisher.commit_world_rebind(1,2,1000000003,[&](){invoked=true;return false;})&&invoked&&
        receiver.world_state().quiescing&&receiver.world_state().generation==1,"failed native commit became visible");
    require(publisher.commit_world_rebind(1,2,1000000003),"drained world commit");
    auto world=receiver.world_state();require(world.valid&&world.generation==2&&!world.quiescing&&world.minimum_sample_ns==1000000003,"committed world metadata");
    require(receiver.generation()==resource_generation&&metadata.packet->sequence==old_sequence&&receiver.sequence()==UINT64(old_sequence),"commit reset GPU identity or serial");
    require(!receiver.acquire(gpu.queue.Get())&&!gpu_world_image_current(world,receiver.acquired_world_generation(),receiver.ticket().sample.received_ns),"old ACK became new-world image");
    require(!gpu_world_image_current(world,2,1000000003)&&!gpu_world_image_current({},2,1000000004),"minimum or invalid world accepted");
    require(publisher.capture_resource(retained_source,7,idle,ticket(4)),"new-world publication");
    require(gpu.finish(),"new-world producer completion");publisher.poll();
    require(receiver.acquire(gpu.queue.Get())&&receiver.acquired_world_generation()==2&&
        gpu_world_image_current(receiver.world_state(),receiver.acquired_world_generation(),receiver.ticket().sample.received_ns),"new-world image rejected");
    receiver.acknowledge(gpu.queue.Get());require(gpu.finish(),"new-world ACK");
    const auto sequence=metadata.packet->sequence;const auto frozen=metadata.packet->ticket;
    publisher.mark_stopped();require(!publisher.capture_resource(retained_source,7,idle,ticket(3))&&metadata.packet->sequence==sequence&&same_ticket(metadata.packet->ticket,frozen),"stop mutated image slot");
    GpuIdleSnapshot stopped{};require(!publisher.try_idle(stopped),"stopped publisher accepted as resumable idle");
    require(publisher.drain(1000),"producer drain");receiver.close();require(publisher.close(),"producer close");gpu.verify();
    printf("{\"owned_idle_state\":%u,\"frames\":4,\"world_handoff_verified\":true,\"two_eye_pixels_verified\":true,\"private_ticket_verified\":true}\n",unsigned(idle));
}
static void world_epoch_policy(){
    GpuPacket packet{};packet.magic=0x47564731;packet.version=gpu_packet_version;packet.size=sizeof(packet);packet.world_generation=1;
    require(!gpu_world_state(nullptr).valid,"null world valid");
    auto world=gpu_world_state(&packet);require(world.valid&&world.generation==1&&!world.quiescing&&world.minimum_sample_ns==0,"initial epoch snapshot");
    require(!gpu_world_image_current(world,0,1)&&!gpu_world_image_current(world,2,1)&&!gpu_world_image_current(world,1,0),"wrong epoch or nonpositive sample accepted");
    packet.world_revision=1;require(!gpu_world_state(&packet).valid,"in-progress epoch snapshot accepted");packet.world_revision=2;
    packet.world_quiescing=2;require(!gpu_world_state(&packet).valid,"invalid quiescing state accepted");packet.world_quiescing=1;
    world=gpu_world_state(&packet);require(world.valid&&world.quiescing&&!gpu_world_image_current(world,1,1),"quiescing epoch displayable");
    packet.world_quiescing=0;packet.world_generation=0;require(!gpu_world_state(&packet).valid,"zero world epoch valid");packet.world_generation=2;
    packet.world_minimum_sample_ns=-1;require(!gpu_world_state(&packet).valid,"negative world minimum valid");packet.world_minimum_sample_ns=10;
    world=gpu_world_state(&packet);require(!gpu_world_image_current(world,2,10)&&gpu_world_image_current(world,2,11),"world boundary not exclusive");
    packet.version=gpu_packet_version-1;require(!gpu_world_state(&packet).valid,"old world ABI accepted");
}
int main(){try{
    world_epoch_policy();
    ComPtr<ID3D12Debug> debug;const bool enabled=SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));if(enabled)debug->EnableDebugLayer();
    printf("{\"debug_layer\":%s}\n",enabled?"true":"false");GPU gpu;rejects(gpu);
    for(auto state:{D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE})exercise(gpu,state);
    puts("OwnedGpuShareTest passed");return 0;
}catch(std::exception const& error){fprintf(stderr,"OwnedGpuShareTest failed: %s\n",error.what());return 1;}}
