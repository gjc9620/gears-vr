// Reuse the real D3D12 fixture and encoded-pixel reader, with the legacy suite
// still linked under another entry name. No production hook/lock is modified.
#define main legacy_owned_gpu_suite_main
#include "owned_gpu_share_test.cpp"
#undef main

struct QueuedPaint {
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12DescriptorHeap> rtv;
    QueuedPaint(GPU& gpu,ID3D12Resource* source,unsigned id){
        gpu_check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"queued paint allocator");
        gpu_check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"queued paint list");
        D3D12_DESCRIPTOR_HEAP_DESC d{};d.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;d.NumDescriptors=1;
        gpu_check(gpu.device->CreateDescriptorHeap(&d,IID_PPV_ARGS(&rtv)),"queued paint RTV");
        gpu.device->CreateRenderTargetView(source,nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
        gpu_barrier(list.Get(),source,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_RENDER_TARGET);
        const float a[]{float(id)/8,0,0,1},b[]{0,float(id)/8,1,1};const D3D12_RECT l{0,0,32,32},r{32,0,64,32};
        list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),a,1,&l);
        list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),b,1,&r);
        gpu_barrier(list.Get(),source,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COMMON);
        gpu_check(list->Close(),"queued paint close");ID3D12CommandList* lists[]{list.Get()};gpu.queue->ExecuteCommandLists(1,lists);
    }
    void retire(){gpu_check(allocator->Reset(),"queued paint retire allocator");gpu_check(list->Reset(allocator.Get(),nullptr),"queued paint retire list");gpu_check(list->Close(),"queued paint retire close");}
};
static void ready_fifo_pixels(){
    GPU producer,consumer;auto source=texture(producer.device.Get());QueuedPaint paint1(producer,source.Get(),1);require(producer.finish(),"paint1 completion");
    GpuPublisher publisher;require(publisher.bind_resource(producer.queue.Get(),source.Get(),7,D3D12_RESOURCE_STATE_COMMON)&&publisher.prepare(),"ready prepare");
    GpuReceiver receiver;receiver.open(consumer.device.Get(),GetCurrentProcessId());Metadata metadata(true);
    auto* original=receiver.texture.Get();SlotReadback pixels(consumer.device.Get(),original);
    {
        QueueHold blocked(producer.device.Get(),producer.queue.Get());
        require(publisher.capture_resource(source.Get(),7,D3D12_RESOURCE_STATE_COMMON,ticket(1)),"blocked ready first publish");
        QueuedPaint paint2(producer,source.Get(),2);
        require(publisher.capture_resource(source.Get(),7,D3D12_RESOURCE_STATE_COMMON,ticket(2)),"blocked ready second publish");
        for(unsigned i=0;i<16;++i)require(!receiver.acquire(consumer.queue.Get(),true),"unready head acquired");
        require(receiver.ready_deferred_count==16&&receiver.acquired_count==0&&receiver.acknowledged_count==0&&receiver.sequence()==0&&
            receiver.acquired_slot()==gears_gpu_slots::none&&receiver.texture.Get()==original&&stereo_image_ticket_empty(receiver.ticket()),"defer changed lease or prior image");
        require(consumer.finish(),"ready-only enqueued hidden producer Wait");
        blocked.release();require(producer.finish(),"blocked ready producer completion");
        consumer.begin();
        {
            QueueHold ack_hold(consumer.device.Get(),consumer.queue.Get());
            require(receiver.acquire(consumer.queue.Get(),true)&&same_ticket(receiver.ticket(),ticket(1)),"ready FIFO ticket1");
            pixels.submit(consumer,receiver.texture.Get());receiver.acknowledge(consumer.queue.Get());
            GpuCaptureTrace busy;require(!publisher.capture_resource(source.Get(),7,D3D12_RESOURCE_STATE_COMMON,ticket(3),&busy)&&busy.reason==GpuCaptureReason::both_slots_busy,
                "ready CPU ACK recycled blocked GPU slot");ack_hold.release();
        }
        require(consumer.finish(),"ready first reader completion");pixels.verify(1);
        consumer.begin();require(receiver.acquire(consumer.queue.Get(),true)&&same_ticket(receiver.ticket(),ticket(2)),"ready FIFO ticket2");
        pixels.submit(consumer,receiver.texture.Get());receiver.acknowledge(consumer.queue.Get());require(consumer.finish(),"ready second reader completion");pixels.verify(2);
    }
    require(receiver.acquired_count==2&&receiver.acknowledged_count==2&&receiver.ready_deferred_count==16,"ready counters changed");
    publisher.mark_stopped();require(publisher.drain(1000),"ready pixel drain");receiver.close();require(publisher.close(),"ready pixel close");
    producer.verify();consumer.verify();puts("{\"ready_only_fifo_pixels\":true,\"no_hidden_queue_wait\":true,\"deferred_calls\":16,\"encoded_stereo_frames\":2,\"delayed_gpu_ack\":true}");
}
static void ready_quiesce_or_stop(bool stop){
    GPU producer,consumer;auto source=texture(producer.device.Get());GpuPublisher publisher;
    require(publisher.bind_resource(producer.queue.Get(),source.Get(),7,D3D12_RESOURCE_STATE_COMMON)&&publisher.prepare(),"ready lifecycle prepare");
    GpuReceiver receiver;receiver.open(consumer.device.Get(),GetCurrentProcessId());
    {
        QueueHold blocked(producer.device.Get(),producer.queue.Get());
        for(unsigned id=1;id<=2;++id)require(publisher.capture_resource(source.Get(),7,D3D12_RESOURCE_STATE_COMMON,ticket(id)),"ready lifecycle capture");
        require(!receiver.acquire(consumer.queue.Get(),true)&&consumer.finish(),"ready lifecycle hidden wait");
        if(stop){publisher.mark_stopped();require(!receiver.acquire(consumer.queue.Get(),true)&&!publisher.drain(5),"stop retired unfinished producer");}
        else{
            require(publisher.begin_world_rebind(1),"ready lifecycle quiesce");
            require(!gpu_world_image_current(receiver.world_state(),1,1000000001)&&!publisher.commit_world_rebind(1,2,1000000003),"quiesce displayed or retired old GPU work");
            for(unsigned id=1;id<=2;++id){require(receiver.acquire(consumer.queue.Get())&&receiver.ticket().id==id,"quiesce FIFO queued acquire");receiver.discard(consumer.queue.Get());}
            require(!publisher.commit_world_rebind(1,2,1000000003),"queued ACK fabricated GPU completion");
        }
        blocked.release();require(producer.finish()&&consumer.finish(),"ready lifecycle GPU drain");
    }
    if(!stop){publisher.poll();require(publisher.retire_owned_source(1,7)&&publisher.commit_world_rebind(1,2,1000000003),"ready lifecycle world commit");
        require(!gpu_world_image_current(receiver.world_state(),1,1000000001)&&receiver.discarded_count==2,"old world displayed after commit");publisher.mark_stopped();}
    require(publisher.drain(1000),"ready lifecycle producer drain");receiver.close();require(publisher.close(),"ready lifecycle close");
    require(stop?receiver.acknowledged_count==0:receiver.acknowledged_count==2,"lifecycle fabricated ACK");
    producer.verify();consumer.verify();printf("{\"ready_only_lifecycle\":\"%s\",\"blocked_producer_retained\":true,\"no_fabricated_ack\":true}\n",stop?"stop":"quiesce");
}
static void ready_resize(){
    GPU producer,consumer;ComPtr<IDXGIFactory4> factory;gpu_check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"ready resize factory");
    DXGI_SWAP_CHAIN_DESC1 d{};d.Width=64;d.Height=32;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.BufferCount=2;
    d.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;d.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;d.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
    ComPtr<IDXGISwapChain1> chain;gpu_check(factory->CreateSwapChainForComposition(producer.queue.Get(),&d,nullptr,&chain),"ready resize chain");
    ComPtr<ID3D12Resource> source;gpu_check(chain->GetBuffer(0,IID_PPV_ARGS(&source)),"ready resize source");QueuedPaint paint(producer,source.Get(),1);require(producer.finish(),"ready resize paint");
    GpuPublisher publisher;require(publisher.bind(producer.queue.Get(),chain.Get())&&publisher.prepare(),"ready resize prepare");
    GpuReceiver receiver;receiver.open(consumer.device.Get(),GetCurrentProcessId());const auto generation=receiver.generation();
    {
        QueueHold blocked(producer.device.Get(),producer.queue.Get());
        for(unsigned id=1;id<=2;++id){auto t=ticket(id);require(publisher.capture(chain.Get(),&t),"ready resize capture");}
        require(!receiver.acquire(consumer.queue.Get(),true)&&consumer.finish(),"ready resize hidden wait");
        require(publisher.begin_resize(chain.Get())&&receiver.reconfiguring()&&!receiver.acquire(consumer.queue.Get(),true),"ready resize admission");
        require(!publisher.drain(5)&&!publisher.prepare(),"ready resize retired producer before ready");
        require(receiver.release_for_resize(),"never-acquired resize consumer could not retire");
        publisher.poll();require(publisher.state==5,"resize ignored unfinished producer");
        blocked.release();require(producer.finish()&&publisher.drain(1000),"ready resize source drain");
    }
    paint.retire();source.Reset();gpu_check(chain->ResizeBuffers(2,128,32,d.Format,0),"ready resize buffers");publisher.end_resize();publisher.poll();
    require(publisher.state==1&&publisher.prepare(),"ready resize rebuild");receiver.open(consumer.device.Get(),GetCurrentProcessId());
    require(receiver.generation()==generation+1&&receiver.width==128&&receiver.sequence()==0&&stereo_image_ticket_empty(receiver.ticket()),"ready resize stale identity");
    require(!receiver.acquire(consumer.queue.Get(),true)&&receiver.acknowledged_count==0,"ready resize fabricated image/ACK");
    publisher.mark_stopped();require(publisher.drain(1000),"ready resize final drain");receiver.close();require(publisher.close(),"ready resize close");
    producer.verify();consumer.verify();puts("{\"ready_only_blocked_resize\":true,\"producer_completion_required\":true,\"generation_reset\":true}");
}
int main(){try{ComPtr<ID3D12Debug> debug;const bool enabled=SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));if(enabled)debug->EnableDebugLayer();
    printf("{\"debug_layer\":%s}\n",enabled?"true":"false");ready_fifo_pixels();ready_quiesce_or_stop(false);ready_quiesce_or_stop(true);ready_resize();
    puts("ReadyOnlyGpuTest passed");return 0;
}catch(std::exception const& e){fprintf(stderr,"ReadyOnlyGpuTest failed: %s\n",e.what());return 1;}}
