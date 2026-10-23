// Real independent GPU queues; deterministic stalls exercise the same shared
// packet, textures and per-slot fences used by the game and XR receiver.
struct SlotReadback {
    ComPtr<ID3D12Resource> buffer;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};UINT64 bytes{};
    SlotReadback(ID3D12Device* device,ID3D12Resource* image){auto desc=image->GetDesc();
        device->GetCopyableFootprints(&desc,0,1,0,&layout,nullptr,nullptr,&bytes);
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;
        d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_READBACK;
        gpu_check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&buffer)),"slot readback");}
    void submit(GPU& gpu,ID3D12Resource* image){
        gpu_barrier(gpu.list.Get(),image,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=buffer.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=layout;
        from.pResource=image;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        gpu_barrier(gpu.list.Get(),image,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
        gpu_check(gpu.list->Close(),"slot readback close");ID3D12CommandList* lists[]{gpu.list.Get()};gpu.queue->ExecuteCommandLists(1,lists);
    }
    void verify(unsigned id){void* data{};D3D12_RANGE r{0,SIZE_T(bytes)};gpu_check(buffer->Map(0,&r,&data),"slot readback map");
        bool correct=true;const unsigned encoded=(id*255+4)/8;
        for(unsigned y=0;y<32;++y)for(unsigned x=0;x<64;++x){auto* p=static_cast<unsigned char*>(data)+layout.Offset+size_t(y)*layout.Footprint.RowPitch+x*4;
            const int expected[4]={x<32?int(encoded):0,x<32?0:int(encoded),x<32?0:255,255};
            for(unsigned c=0;c<4;++c)if(abs(int(p[c])-expected[c])>1)correct=false;}
        D3D12_RANGE empty{};buffer->Unmap(0,&empty);require(correct,"two-slot pixels and encoded frame ID differ");}
};
static void two_slot_ownership(GPU& producer){
    GPU reader;auto source=texture(producer.device.Get());GpuPublisher publisher;
    require(publisher.bind_resource(producer.queue.Get(),source.Get(),7,D3D12_RESOURCE_STATE_COMMON)&&publisher.prepare(),"two-slot prepare");
    GpuReceiver receiver;receiver.open(reader.device.Get(),GetCurrentProcessId());Metadata metadata(true);
    require(metadata.packet->slot_count==2&&metadata.packet->version==8,"two-slot ABI");
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;ComPtr<ID3D12DescriptorHeap> rtv;
    gpu_check(producer.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv)),"slot RTV");
    producer.device->CreateRenderTargetView(source.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    auto publish=[&](unsigned id){producer.begin();gpu_barrier(producer.list.Get(),source.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_RENDER_TARGET);
        const float left[]{float(id)/8,0,0,1},right[]{0,float(id)/8,1,1};const D3D12_RECT a{0,0,32,32},b{32,0,64,32};
        producer.list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),left,1,&a);
        producer.list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),right,1,&b);
        gpu_barrier(producer.list.Get(),source.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COMMON);producer.submit();
        GpuCaptureTrace trace;require(publisher.capture_resource(source.Get(),7,D3D12_RESOURCE_STATE_COMMON,ticket(id),&trace),"two-slot publication");
        require(trace.slot_index<2&&trace.slot_serial_after==id&&trace.serial_after==id,"two-slot capture lease trace");
        require(producer.finish(),"slot producer fence");publisher.poll();return trace.slot_index;};
    const auto first_slot=publish(1);
    reader.begin(); // Reset the local allocator before installing the stall.
    SlotReadback pixels(reader.device.Get(),receiver.texture.Get());
    {
        QueueHold stall(reader.device.Get(),reader.queue.Get());
        require(receiver.acquire(reader.queue.Get())&&receiver.acquired_slot()==first_slot&&same_ticket(receiver.ticket(),ticket(1)),"first slot acquire");
        auto* first_texture=receiver.texture.Get();
        pixels.submit(reader,first_texture);
        const auto second_slot=publish(2);require(second_slot!=first_slot,"second publication reused acquired slot");
        require(receiver.texture.Get()==first_texture&&same_ticket(receiver.ticket(),ticket(1)),"other-slot publish changed acquired image identity");
        require(!receiver.acquire(reader.queue.Get()),"two simultaneous CPU acquisitions accepted");
        auto before0=metadata.packet->slots[0].ticket,before1=metadata.packet->slots[1].ticket;
        const auto seq0=metadata.packet->slots[0].sequence,seq1=metadata.packet->slots[1].sequence;
        GpuCaptureTrace busy;
        require(!publisher.capture_resource(source.Get(),7,D3D12_RESOURCE_STATE_COMMON,ticket(3),&busy)&&busy.reason==GpuCaptureReason::both_slots_busy,"third capture with two occupied slots succeeded");
        require(metadata.packet->slots[0].sequence==seq0&&metadata.packet->slots[1].sequence==seq1&&
            same_ticket(metadata.packet->slots[0].ticket,before0)&&same_ticket(metadata.packet->slots[1].ticket,before1),"busy capture changed either immutable slot");
        receiver.acknowledge(reader.queue.Get()); // CPU ACK exists, GPU ACK remains blocked.
        require(!publisher.capture_resource(source.Get(),7,D3D12_RESOURCE_STATE_COMMON,ticket(3),&busy)&&busy.reason==GpuCaptureReason::both_slots_busy,"CPU ACK reused a GPU-owned slot");
        GpuIdleSnapshot waiting;require(!publisher.try_idle(waiting)&&waiting.slots_observed==2&&
            waiting.slots[first_slot].done<waiting.slots[first_slot].sequence,"blocked consumer observed drained");
        stall.release();
    }
    require(reader.finish(),"first consumer completion");pixels.verify(1);
    require(publish(3)==first_slot,"acknowledged slot did not become reusable");
    require(publisher.begin_world_rebind(1),"two-slot world quiescence");
    require(!publisher.commit_world_rebind(1,2,1000000003),"two unacquired slots skipped world drain");
    for(unsigned id=2;id<=3;++id){
        reader.begin();require(receiver.acquire(reader.queue.Get())&&same_ticket(receiver.ticket(),ticket(id)),"FIFO acquired wrong ticket");
        const auto index=receiver.acquired_slot();require(index==(id==2?1-first_slot:first_slot),"FIFO read wrong texture slot");
        require(receiver.acquired_world_generation()==1&&!gpu_world_image_current(receiver.world_state(),1,receiver.ticket().sample.received_ns),"old-world slot display admitted");
        pixels.submit(reader,receiver.texture.Get());
        if(id==2){
            const auto generation=metadata.packet->generation;metadata.packet->generation=generation+1;
            bool rejected=false;try{receiver.acknowledge(reader.queue.Get());}catch(...){rejected=true;}
            metadata.packet->generation=generation;require(rejected&&receiver.acknowledged_count==1,"stale-generation ACK accepted");
            const auto sequence=metadata.packet->slots[index].sequence;InterlockedExchange64(&metadata.packet->slots[index].sequence,sequence-1);
            rejected=false;try{receiver.acknowledge(reader.queue.Get());}catch(...){rejected=true;}
            InterlockedExchange64(&metadata.packet->slots[index].sequence,sequence);require(rejected&&receiver.acknowledged_count==1,"stale-sequence ACK accepted");
        }
        receiver.discard(reader.queue.Get());require(reader.finish(),"FIFO consumer completion");pixels.verify(id);
        if(id==2)require(!publisher.commit_world_rebind(1,2,1000000003),"ACK for slot 1 retired slot 0");
    }
    GpuIdleSnapshot drained;require(publisher.try_idle(drained)&&gears_gpu_slots::drained(drained.slots),"two-slot final idle");
    require(publisher.retire_owned_source(1,7)&&publisher.commit_world_rebind(1,2,1000000003),"two-slot handoff commit");
    require(receiver.acquired_count==3&&receiver.acknowledged_count==3&&receiver.discarded_count==2,"slot counters conflated discard and acquired images");
    publisher.mark_stopped();require(publisher.drain(1000),"two-slot stop drain");receiver.close();require(publisher.close(),"two-slot close");
    producer.verify();reader.verify();
    puts("{\"two_slot_ownership\":true,\"fifo_pixels_tickets\":3,\"delayed_gpu_ack\":true,\"stale_ack_rejected\":true,\"world_drain_all_slots\":true}");
}
