// Included inside capture after the NGX wrapper and established native-resource
// adapters. Never interpret an NGX raw pointer as COM: only typed copy arguments
// enter this backend, while their original API caller owns them.
namespace ngx_output_pixels {
#if (defined(GEARS_NGX_OUTPUT_PIXELS) && GEARS_NGX_OUTPUT_PIXELS) || (defined(GEARS_OWNED_SOURCE_PIXELS) && GEARS_OWNED_SOURCE_PIXELS)
struct NativeData {
    ID3D12Resource *source{},*destination{},*readback{};ID3D12GraphicsCommandList* list{};
    ID3D12CommandQueue* queue{};ID3D12Device* device{};ID3D12Fence* fence{};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT rows{};UINT64 row_bytes{},bytes{};
#if defined(GEARS_OWNED_SOURCE_PIXELS) && GEARS_OWNED_SOURCE_PIXELS
    UINT64 completed_value{};
#endif
};
// Deliberately no COM destructors: uncertain submitted objects survive teardown.
std::array<NativeData,2> native_data;gears_death_hdr_match::Publisher match_publisher;
template<class T> void release_native(T*& p){if(p){p->Release();p=nullptr;}}
uintptr_t native_identity(IUnknown* p){__try{return p?canonical_identity(p):0;}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}}
gears_death_hdr_pixels::Texture native_texture(ID3D12Resource* p){auto d=p->GetDesc();return {native_identity(p),d.Width,d.Height,unsigned(d.Format),unsigned(d.Dimension),unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),d.SampleDesc.Count,d.SampleDesc.Quality};}
struct NativeBackend final:Backend {
    bool start()override{return match_publisher.create(GetCurrentProcessId());}
    bool prepare(unsigned index,Slot& s)override{
        auto& n=native_data[index];auto* src=s.source.pResource;auto* dst=s.destination.pResource;
        if(!system_resource_methods(reinterpret_cast<uintptr_t>(src))||!system_resource_methods(reinterpret_cast<uintptr_t>(dst))||
           !system_resource_methods(reinterpret_cast<uintptr_t>(s.raw_list)))return false;
        s.hr=src->QueryInterface(IID_PPV_ARGS(&n.source));if(FAILED(s.hr))return false;
        s.hr=dst->QueryInterface(IID_PPV_ARGS(&n.destination));if(FAILED(s.hr))return false;
        s.source_desc=native_texture(n.source);s.destination_desc=native_texture(n.destination);
        auto const& a=s.source_desc;auto const& b=s.destination_desc;
        if(!textures_supported(a,b))return false;
        D3D12_HEAP_PROPERTIES source_heap{},destination_heap{};D3D12_HEAP_FLAGS source_flags{},destination_flags{};
        s.hr=n.source->GetHeapProperties(&source_heap,&source_flags);if(FAILED(s.hr))return false;
        s.hr=n.destination->GetHeapProperties(&destination_heap,&destination_flags);if(FAILED(s.hr))return false;
        // Reserved/unknown heaps and protected resources have no supported
        // readback contract in this bounded probe.
        if(source_heap.Type!=D3D12_HEAP_TYPE_DEFAULT||destination_heap.Type!=D3D12_HEAP_TYPE_DEFAULT||
           ((source_flags|destination_flags)&D3D12_HEAP_FLAG_HARDWARE_PROTECTED))return false;
        s.hr=n.source->GetDevice(IID_PPV_ARGS(&n.device));if(FAILED(s.hr))return false;
        ID3D12Device *dst_device{},*list_device{};
        const bool same=SUCCEEDED(n.destination->GetDevice(IID_PPV_ARGS(&dst_device)))&&SUCCEEDED(s.raw_list->GetDevice(IID_PPV_ARGS(&list_device)))&&
            native_identity(n.device)&&native_identity(n.device)==native_identity(dst_device)&&native_identity(n.device)==native_identity(list_device);
        release_native(dst_device);release_native(list_device);if(!same||s.raw_list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;
        const auto id=native_identity(s.raw_list);const auto ref=frame_link.current(id);if(!id||ref.identity!=id||!ref.generation)return false;
        s.life.list={reinterpret_cast<uintptr_t>(s.raw_list),ref.identity,ref.generation};
        const auto desc=n.source->GetDesc();n.device->GetCopyableFootprints(&desc,0,1,0,&n.footprint,&n.rows,&n.row_bytes,&n.bytes);
        if(!gears_death_hdr_pixels::footprint_valid(a,n.bytes,n.footprint.Offset,n.footprint.Footprint.RowPitch,n.rows,n.row_bytes,
           n.footprint.Footprint.Width,n.footprint.Footprint.Height,n.footprint.Footprint.Depth,unsigned(n.footprint.Footprint.Format)))return false;
        const auto length=GetModuleFileNameW(own_module,s.path,MAX_PATH);if(!length||length>=MAX_PATH)return false;
        auto* separator=wcsrchr(s.path,L'\\');if(!separator)return false;*separator=0;wchar_t parent[MAX_PATH]{};wcscpy_s(parent,s.path);
#if defined(GEARS_OWNED_SOURCE_PIXELS) && GEARS_OWNED_SOURCE_PIXELS
        if(swprintf_s(s.path,L"%s\\owned-hdr-source-%lu-%llu-copy%llu-%lld.raw",parent,GetCurrentProcessId(),s.before.ownership.ticket,s.sequence,s.copy_qpc)<=0)return false;
#else
        if(swprintf_s(s.path,L"%s\\ngx-output-copy-%lu-%llu-eval%llu-%lld.raw",parent,GetCurrentProcessId(),s.before.ownership.ticket,s.sequence,s.copy_qpc)<=0)return false;
#endif
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC buffer{};
        buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=n.bytes;buffer.Height=1;buffer.DepthOrArraySize=1;buffer.MipLevels=1;
        buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        {StereoPublishBypass bypass;
            s.hr=n.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&n.readback));
            if(FAILED(s.hr))return false;s.hr=n.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&n.fence));if(FAILED(s.hr))return false;}
        s.hr=s.raw_list->QueryInterface(IID_PPV_ARGS(&n.list));if(FAILED(s.hr))return false;
        s.life.held_identity=native_identity(n.list);const auto after=frame_link.current(s.life.held_identity);
        return s.life.held_identity==id&&after.identity==ref.identity&&after.generation==ref.generation;
    }
    void append(unsigned index,Slot& s,CopyFn forward)override{
        D3D12_TEXTURE_COPY_LOCATION own{};own.pResource=native_data[index].readback;own.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;own.PlacedFootprint=native_data[index].footprint;
        forward(s.raw_list,&own,0,0,0,&s.source,nullptr);
    }
    uintptr_t list_identity(ID3D12CommandList* list)override{return native_identity(list);}
    gears_frame_link::ListRef list_ref(uintptr_t id)override{return frame_link.current(id);}
    bool queue_valid(unsigned index,ID3D12CommandQueue* q,uintptr_t& id)override{
        auto& n=native_data[index];if(!q||!system_resource_methods(reinterpret_cast<uintptr_t>(q))||q->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;
        ID3D12Device* d{};const bool same=SUCCEEDED(q->GetDevice(IID_PPV_ARGS(&d)))&&native_identity(d)==native_identity(n.device);release_native(d);
        id=native_identity(q);if(!same||!id)return false;if(!n.queue){n.queue=q;q->AddRef();}return true;
    }
    bool signal(unsigned index,ID3D12CommandQueue* q)override{return SUCCEEDED(q->Signal(native_data[index].fence,1));}
    bool poll(unsigned index,Slot& s)override{
        auto& n=native_data[index];if(!s.life.signalled)return false;
        s.hr=n.device->GetDeviceRemovedReason();if(FAILED(s.hr)){s.life.fail(gears_death_hdr_pixels::device_removed);return false;}
#if defined(GEARS_OWNED_SOURCE_PIXELS) && GEARS_OWNED_SOURCE_PIXELS
        n.completed_value=n.fence->GetCompletedValue();s.life.completion(n.completed_value);
#else
        s.life.completion(n.fence->GetCompletedValue());
#endif
        if(!s.life.may_map())return false;
        void* data{};D3D12_RANGE range{0,SIZE_T(n.bytes)};s.hr=n.readback->Map(0,&range,&data);
        if(FAILED(s.hr)||!data){s.life.fail(gears_death_hdr_pixels::map_failed);return false;}
        HANDLE raw=CreateFileW(s.path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);bool ok=raw!=INVALID_HANDLE_VALUE;
        if(ok){DWORD written{};ok=WriteFile(raw,data,DWORD(n.bytes),&written,nullptr)&&written==n.bytes&&FlushFileBuffers(raw);if(!CloseHandle(raw))ok=false;}
        D3D12_RANGE none_written{};n.readback->Unmap(0,&none_written);
        wchar_t meta[MAX_PATH]{};if(swprintf_s(meta,L"%s.txt",s.path)<=0)ok=false;
        HANDLE output=ok?CreateFileW(meta,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr):INVALID_HANDLE_VALUE;
        if(output==INVALID_HANDLE_VALUE)ok=false;else{char text[2048]{};
#if !defined(GEARS_OWNED_SOURCE_PIXELS) || !GEARS_OWNED_SOURCE_PIXELS
            auto const& v=s.before.values;
#endif
#if defined(GEARS_OWNED_SOURCE_PIXELS) && GEARS_OWNED_SOURCE_PIXELS
            const int length=sprintf_s(text,"world=%llu ticket=%llu copy_order=%llu eye=%u view=%llx renderer=%llx thread=%u caller=1c6093 source_raw=%llx destination_raw=%llx source_canonical=%llx destination_canonical=%llx list_raw=%llx list_identity=%llx list_generation=%llu execute_order=%lld member=%u queue=%llx copy_qpc=%lld raw_bytes=%llu footprint_offset=%llu row_pitch=%u rows=%u row_bytes=%llu width=%u height=%u depth=%u format=%u samples=1 mip=0 source_subresource=0 destination_subresource=0 xyz=0,0,0 box_null=1 conversion=none executions=1 retired=1 gpu_complete=1 fence=%llx fence_target=1 fence_completed=%llu reset_raw=%llx reset_identity=%llx reset_hr=%08lx reset_tracker_generation=%llu reset_tracker_identity=%llx reset_qpc=%lld source_pixel_only=1 final_match_verified=0 content_eye_verified=0 barriers_added=0\n",
                s.before.ownership.world,s.before.ownership.ticket,s.sequence,s.before.ownership.eye,s.before.ownership.view,s.before.ownership.renderer,s.thread,
                s.before.resources[1],s.before.resources[0],s.source_desc.canonical,s.destination_desc.canonical,s.life.list.raw,s.life.list.identity,s.life.list.generation,
                s.execute_order,s.execute_member,s.life.queue,s.copy_qpc,n.bytes,n.footprint.Offset,n.footprint.Footprint.RowPitch,n.rows,n.row_bytes,n.footprint.Footprint.Width,n.footprint.Footprint.Height,n.footprint.Footprint.Depth,unsigned(n.footprint.Footprint.Format),
                reinterpret_cast<uintptr_t>(n.fence),n.completed_value,s.life.reset_receipt.raw,s.life.reset_receipt.identity,static_cast<unsigned long>(s.reset_hr),s.life.reset_receipt.tracker_generation,s.life.reset_receipt.tracker_identity,s.reset_qpc);
#else
            const int length=sprintf_s(text,"world=%llu ticket=%llu eval_sequence=%llu copy_sequence=%llu source_base=%u,%u output_base=%u,%u render_dimensions=%u,%u reset_raw=%u inside_eval=%u source_raw=%llx color_raw=%llx source_canonical=%llx color_canonical=%llx list=%llx generation=%llu execute_order=%lld member=%u queue=%llx copy_qpc=%lld raw_bytes=%llu footprint_offset=%llu row_pitch=%u rows=%u row_bytes=%llu width=%u height=%u format=%u source_subresource=0 destination_subresource=0 original_caller=1c6093 copy_point_only=1 logical_eye_verified=0 gpu_complete=1 physical_reset_retired=1 barriers_added=0\n",
                s.before.ownership.world,s.before.ownership.ticket,s.sequence,s.copy_sequence,v[7],v[8],v[13],v[14],v[0],v[1],v[2],unsigned(s.inside_eval),
                static_cast<unsigned long long>(s.before.resources[1]),static_cast<unsigned long long>(s.before.resources[0]),static_cast<unsigned long long>(s.source_desc.canonical),static_cast<unsigned long long>(s.destination_desc.canonical),
                static_cast<unsigned long long>(s.life.list.identity),s.life.list.generation,s.execute_order,s.execute_member,static_cast<unsigned long long>(s.life.queue),s.copy_qpc,n.bytes,n.footprint.Offset,n.footprint.Footprint.RowPitch,n.rows,n.row_bytes,n.footprint.Footprint.Width,n.footprint.Footprint.Height,unsigned(n.footprint.Footprint.Format));
#endif
            DWORD written{};if(length<=0||!WriteFile(output,text,DWORD(length),&written,nullptr)||written!=unsigned(length)||!FlushFileBuffers(output))ok=false;if(!CloseHandle(output))ok=false;}
        if(!ok){s.life.fail(gears_death_hdr_pixels::file_failed);return false;}s.life.saved=true;return true;
    }
    void release(unsigned index)override{auto& n=native_data[index];release_native(n.source);release_native(n.destination);release_native(n.readback);release_native(n.list);release_native(n.queue);release_native(n.fence);release_native(n.device);}
    bool publish(uint64_t selected_ticket,uint64_t selected_world,uint64_t seq)override{return match_publisher.publish(selected_ticket,selected_world,seq);}
    void close()override{match_publisher.close();}
};
NativeBackend native_backend;Backend& backend(){return native_backend;}
#endif
}
