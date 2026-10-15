// The caller retains the existing frame lock across recording and original API
// calls. All resource queries use the engine's currently bound input object.
#include "final_transfer_policy.h"
namespace final_transfer_live {
std::atomic<bool> attested{};
uint64_t attempts{},accepted{},policy_rejected{},pair_rejected{},source_draws{},copy_writes{};
bool enabled(){
#if defined(GEARS_FINAL_OUTPUT_TRANSFER) && GEARS_FINAL_OUTPUT_TRANSFER
    return attested.load()&&!self_testing&&!owned_bindings_enabled;
#else
    return false;
#endif
}
bool verify_range(FILE* file,uintptr_t rva,ULONG size,const char* expected){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::array<unsigned char,0x723> bytes{};
    bool ok=size<=bytes.size()&&BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=safe_copy(bytes.data(),reinterpret_cast<void*>(base+rva),size)&&BCryptHashData(hash,bytes.data(),size,0)>=0;
    unsigned char digest[32]{};char hex[65]{};
    if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    ok=ok&&!strcmp(hex,expected);
    fprintf(file,"final_transfer_identity rva=%llx size=%lu sha256=%s match=%u source=historical_runtime_artifact1 module_base=%llx\n",
        static_cast<unsigned long long>(rva),size,hex,unsigned(ok),static_cast<unsigned long long>(base));return ok;
}
bool enable(FILE* file){
#if defined(GEARS_FINAL_OUTPUT_TRANSFER) && GEARS_FINAL_OUTPUT_TRANSFER
    if(self_testing)return true;
    const bool ok=continuous_session&&frame_link_diagnostics&&!owned_bindings_enabled&&
        verify_range(file,0x9a6d88,0x723,"7a39267788e942bbb0b1a9700c9f8d55c0c11322f307dc525429b2272fdf025f")&&
        verify_range(file,0x9a6aa8,0x31,"db55c6cada06abe7a7dd87ee80d8eeaf6faedfa6b18f1562685163f1fd30e5d0")&&
        verify_range(file,0x9a6adc,0x31,"0d960a5c0790633f40f677d59975f25a72d40b36d63f2f0fd98dbcc4ab7a1a04");
    attested.store(ok);return ok;
#else
    (void)file;return true;
#endif
}
bool tracks(gears_frame_link::ListRef ref,uintptr_t resource){
    return enabled()&&(frame_link.has_recorded_target(ref,resource)||frame_link.has_transfer_endpoint(resource));
}
void record_source_draw(){++source_draws;}
bool observe(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances,
    gears_viewport::TargetDraw const& binding,gears_frame_link::ListRef ref){
    if(!enabled()||!late_quad_candidate.active||late_quad_candidate.caller!=0x9a7377)return false;
    ++attempts;gears_final_transfer::Input input{};
    input.quad_caller=late_quad_candidate.caller;input.draw_caller=caller-base;input.args=late_quad_candidate.args;
    input.dimensions=late_quad_candidate.dimensions;input.binding=binding;input.kind=kind;input.elements=elements;input.instances=instances;
    input.rectangles=frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list));
    input.known_buffer=swapchain_bindings.known_buffer(binding.target.resource);
    input.descriptor_lost=frame_target_stats.descriptor_capacity_lost();
    uintptr_t tls{},context{},object{};unsigned char count{};ResourceIdentity identity{};
    if(safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
       safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)&&context&&
       safe_copy(&count,reinterpret_cast<void*>(context+0x65d),1)&&count==1&&
       safe_copy(&object,reinterpret_cast<void*>(context+0x768),8)&&object){
        query_bound_resource(object,identity);auto const& d=identity.desc;
        if(SUCCEEDED(identity.result)&&d.Width<=8192){input.slots=count;
            input.source={identity.canonical,unsigned(d.Dimension),unsigned(d.Width),d.Height,unsigned(d.DepthOrArraySize),
                unsigned(d.MipLevels),unsigned(d.Format),d.SampleDesc.Count};}}
    if(!gears_final_transfer::eligible(input)){++policy_rejected;return false;}
    const gears_frame_link::Target source{input.source.resource,input.source.width,input.source.height};
    const gears_frame_link::Target destination{binding.target.resource,unsigned(binding.target.width),binding.target.height};
    if(!frame_link.draw_transfer(ref,source,destination)){++pair_rejected;return false;}
    ++accepted;return true;
}
void copy_destination(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination){
    if(!enabled()||!recording.load())return;
    std::lock_guard<std::recursive_mutex> guard(frame_hook_mutex);
    D3D12_TEXTURE_COPY_LOCATION value{};
    if(!safe_copy(&value,destination,sizeof(value))||!value.pResource){frame_link.invalidate();return;}
    const auto resource=canonical_identity(value.pResource);const auto ref=frame_link.current(canonical_identity(list));
    if(tracks(ref,resource)){frame_link.draw_unknown(ref,resource);++copy_writes;}
}
void print(FILE* file){
    fprintf(file,"final_transfer enabled=%u attested=%u attempts=%llu accepted=%llu policy_rejected=%llu pair_rejected=%llu source_draws=%llu observed_copy_writes=%llu same_list_only=1 geometry=same_width_height_delta_0_or_1 native_caller=9a7377 shader_bytecode_decoded=0 gpu_pixels_verified=0\n",
        unsigned(enabled()),unsigned(attested.load()),attempts,accepted,policy_rejected,pair_rejected,source_draws,copy_writes);
}
}
