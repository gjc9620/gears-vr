// Private unpublished experiment. Buffers are retained by the pinned test
// context until process exit; there is no uncertain late COM destructor.
#if defined(GEARS_NGX_ISOLATED_STEREO_TRIAL) && GEARS_NGX_ISOLATED_STEREO_TRIAL
struct PackedBuffers {
    ID3D12Resource* right{};ID3D12Resource* left{};Extent extent{};
    bool ensure(TypedCopyBoundary& boundary,Extent output){
        if(right||left)return right&&left&&same(extent,output);
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        if(!boundary.active()||!bounded(output)||FAILED(boundary.target_resource()->GetDevice(IID_PPV_ARGS(&device))))return false;
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};
        desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=output.width;desc.Height=output.height;
        desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R11G11B10_FLOAT;
        StereoPublishBypass bypass;
        if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&right)))||
           FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&left))))return false;
        extent=output;return true;
    }
};
struct PackedBackend {
    PackedBuffers& buffers;TypedCopyBoundary& boundary;
    ID3D12Resource* native(PackedRole role)const{
        switch(role){case PackedRole::color:return boundary.target_resource();case PackedRole::output:return boundary.source_resource();
            case PackedRole::right_input:return buffers.right;case PackedRole::left_output:return buffers.left;}return nullptr;
    }
    Resource metadata(PackedRole role)const{Resource out;auto lease=TypedResourceLease::observe(native(role));if(!lease.materialize(out))return {};return out;}
    bool transition(PackedRole role,PackedState before,PackedState after){
        if(!boundary.active()||!native(role)||!original_resource_barrier)return false;
        auto state=[](PackedState v){return v==PackedState::source?D3D12_RESOURCE_STATE_COPY_SOURCE:D3D12_RESOURCE_STATE_COPY_DEST;};
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={native(role),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,state(before),state(after)};
        original_resource_barrier(boundary.command(),1,&b);return boundary.active();
    }
    bool copy(PackedRole from,Rect box,PackedRole to,Point at){
        auto source=metadata(from),target=metadata(to);
        if(!boundary.active()||!source.identity||!target.identity||source.device!=target.device||source.format!=26||target.format!=26||
            !contains(source.extent,box)||!contains(target.extent,{at.x,at.y,box.width,box.height}))return false;
        D3D12_TEXTURE_COPY_LOCATION s{},d{};s.pResource=native(from);d.pResource=native(to);s.Type=d.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_BOX b{box.x,box.y,0,box.x+box.width,box.y+box.height,1};boundary.original_copy()(boundary.command(),&d,at.x,at.y,0,&s,&b);return boundary.active();
    }
};
#endif
