// The single already-reserved original presentation quad. The owned startup
// lease is retired before host preflight; normal Native later reuses that same
// trampoline. No window/engine-object writes or additional renderer.
#include "startup_menu_geometry.h"
namespace startup_menu_geometry_live {
struct Quad {uintptr_t caller{};std::array<float,8> args{};std::array<unsigned,4> dimensions{};};
enum Phase : unsigned {absent,prepared,early,retiring,retired};
std::atomic<unsigned> phase{},callbacks{};
std::atomic<bool> enabled{};
std::atomic<uint64_t> attempts{},repaired{};
std::array<std::atomic<uint64_t>,8> rejected{};
std::array<unsigned char,32> original_bytes{};
struct Activity {
    bool entered{};
    Activity(){if(enabled.load()&&phase.load()!=retiring){++callbacks;entered=true;
        if(phase.load()==retiring){--callbacks;entered=false;}}}
    ~Activity(){if(entered)--callbacks;}
};
bool arm(FILE* status){
    if(!prime_owner.handle||!reservation_ready||self_testing)return true;
    const std::array<unsigned char,15> prefix{0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x10,0x48,0x89,0x78,0x18};
    if(!original_quad||quad_target!=reinterpret_cast<void*>(base+0x1af42c)||
       !safe_copy(original_bytes.data(),quad_target,original_bytes.size())||
       !std::equal(prefix.begin(),prefix.end(),original_bytes.begin()))return false;
    std::array<unsigned char,5> call{};
    bool code=safe_copy(call.data(),reinterpret_cast<void*>(base+0x9a7372),call.size())&&
        call==std::array<unsigned char,5>{0xe8,0xb5,0x80,0x80,0xff};
    // Reuse the established byte certificates, not the old transfer feature's
    // world/pair admission or compile-time enable switch. Those stay unchanged.
    const auto verify=[&](uintptr_t rva,ULONG size,const char* hash){
        code=final_transfer_live::verify_range(status,rva,size,hash,"startup_menu_geometry_identity")&&code;};
    verify(0x9a6d88,0x723,"7a39267788e942bbb0b1a9700c9f8d55c0c11322f307dc525429b2272fdf025f");
    verify(0x1af8fe,9,"03f238db561fe2c123de0356c34e987d91d22502d4daa40179423655f5869366");
    verify(0x177e90,55,"06e1fefa4c97cd3caf9c54ccfd8484e4613f7efa14225c50c6644123e7d373a7");
    verify(0x17862a,82,"09a2cf8f9adf0b04079b75e8e0fdf5d90074bc74f89d69de810af61ee3057b40");
    verify(0x1786bf,100,"c7b97be2b6cf5ee14259c396ea0903c64ad83c98d1e4d3958372d22b2b2e3d86");
    verify(0x178d9c,39,"092f6eeed12732aaaf7daaf57b3595b7ec55623dc81a2e6b3fb638b9cbd11698");
    verify(0x1793a6,39,"4f7475ea72ee212e8517c09ff63583ca331e1bdeafdda1cd3b03e5b3eec91be6");
    enabled.store(code);if(code)phase=prepared;
    fprintf(status,"startup_menu_geometry_arm code_attested=%u owned_startup_lease=1\n",unsigned(code));fflush(status);return code;
}
bool queue(){
    if(!enabled.load())return true;
    if(phase.load()!=prepared||MH_QueueEnableHook(quad_target)!=MH_OK)return false;
    phase=early;return true;
}
bool disable(){
    if(!enabled.load())return true;
    phase=retiring;
    const auto queued=MH_QueueDisableHook(quad_target),disabled=MH_DisableHook(quad_target);
    return queued==MH_OK&&(disabled==MH_OK||disabled==MH_ERROR_DISABLED);
}
bool restored(){
    if(!enabled.load())return true;
    std::array<unsigned char,32> bytes{};
    return !callbacks.load()&&safe_copy(bytes.data(),quad_target,bytes.size())&&bytes==original_bytes;
}
DWORD retire(){
    if(!enabled.load())return 0;
    if(phase.load()==retired)return restored()?0:ERROR_INVALID_STATE;
    if(phase.load()!=early||!disable())return ERROR_INVALID_STATE;
    const auto deadline=GetTickCount64()+2000;
    while(callbacks.load()&&GetTickCount64()<deadline)Sleep(1);
    if(!restored())return ERROR_BUSY;
    phase=retired;return 0;
}
bool repair(Quad& quad){
    // Valid presentation is overwhelmingly common during gameplay. Reject its
    // shape before any TLS, texture, window, descriptor or COM query.
    if(!enabled.load()||quad.caller!=base+0x9a7377||phase.load()==retiring||
       (!recording.load()&&phase.load()!=early)||quad.dimensions[0]||quad.dimensions[1])return false;
    for(unsigned i=0;i<4;++i)if(quad.args[i]!=0)return false;
    for(unsigned i=4;i<8;++i)if(!std::isnan(quad.args[i]))return false;
    ++attempts;auto reject=[](unsigned reason){++rejected[reason];return false;};
    if(prime_owner.state()!=gears_session::OwnerState::alive)return reject(7);
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    uintptr_t tls{},context{},object{};unsigned char count{};
    if(!safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)||!tls||
       !safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)||!context||
       !safe_copy(&count,reinterpret_cast<void*>(context+0x65d),1)||count!=1||
       !safe_copy(&object,reinterpret_cast<void*>(context+0x768),8)||!object)return reject(0);
    ResourceIdentity identity{};query_bound_resource(object,identity);
    const auto& d=identity.desc;
    if(FAILED(identity.result)||!identity.canonical||d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.DepthOrArraySize!=1||
       d.MipLevels!=1||d.SampleDesc.Count!=1||d.Format!=DXGI_FORMAT_R10G10B10A2_UNORM||d.Width>8192||d.Height>8192)return reject(1);
    const auto pending=final_transfer_live::bracket::read_pending(context);
    if(!pending.readable||!pending.stable||pending.descriptor_lost||pending.count!=1||pending.viewport_count!=1||pending.scissor_count!=1||
       !pending.target.resource||pending.target.resource==identity.canonical||!pending.trace.write_serial||!pending.trace.origin_create_serial||
       !pending.trace.creation_live||pending.target.mip||pending.target.format!=24||pending.target.width!=d.Width||pending.target.height!=d.Height)return reject(2);
    const bool rectangles=pending.viewport==std::array<float,6>{0,0,float(d.Width),float(d.Height),0,1}&&
        pending.scissor[0]<=0&&pending.scissor[1]<=0&&pending.scissor[2]>=int64_t(d.Width)&&pending.scissor[3]>=int64_t(d.Height);
    if(!rectangles)return reject(3);
    unsigned matches{};
    {
        std::lock_guard<std::mutex> owners(binding_owner_mutex);
        for(auto const& owner:binding_owners){
            if(!owner.chain||!owner.queue)continue;
            gears_vr::ComPtr<IDXGISwapChain3> chain;DXGI_SWAP_CHAIN_DESC desc{};DWORD pid{};
            if(FAILED(owner.chain->QueryInterface(IID_PPV_ARGS(&chain)))||FAILED(chain->GetDesc(&desc))||
               !desc.OutputWindow||!GetWindowThreadProcessId(desc.OutputWindow,&pid)||pid!=GetCurrentProcessId()||
               desc.BufferDesc.Width!=d.Width||desc.BufferDesc.Height!=d.Height||desc.BufferDesc.Format!=d.Format)continue;
            const auto binding=swapchain_bindings.lookup(canonical_identity(chain.Get()));
            if(!binding.chain||!binding.queue||binding.resizing||binding.rejected)continue;
            gears_vr::ComPtr<ID3D12Resource> buffer;
            if(FAILED(chain->GetBuffer(chain->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&buffer))))continue;
            const auto actual=buffer->GetDesc();
            if(canonical_identity(buffer.Get())==pending.target.resource&&actual.Width==d.Width&&actual.Height==d.Height&&actual.Format==d.Format)++matches;
        }
    }
    if(matches!=1)return reject(4);
    const auto after=final_transfer_live::bracket::read_pending(context);ResourceIdentity source_after{};
    query_bound_resource(object,source_after);
    if(!after.stable||!gears_final_transfer_bracket::equal_pending(pending,after)||FAILED(source_after.result)||
       source_after.canonical!=identity.canonical||source_after.native!=identity.native)return reject(5);
    const gears_startup_menu_geometry::Input input{quad.caller-base,quad.args,quad.dimensions,unsigned(d.Width),d.Height,true,true,true,true,true};
    const auto plan=gears_startup_menu_geometry::plan(input);if(!plan.valid)return reject(6);
    quad.args=plan.args;quad.dimensions=plan.dimensions;++repaired;return true;
}
void print(FILE* status){
    fprintf(status,"startup_menu_geometry enabled=%u phase=%u callbacks=%u attempts=%llu repaired=%llu rejects=",unsigned(enabled.load()),phase.load(),callbacks.load(),attempts.load(),repaired.load());
    for(unsigned i=0;i<rejected.size();++i)fprintf(status,"%s%llu",i?",":"",rejected[i].load());fprintf(status,"\n");
}
}
