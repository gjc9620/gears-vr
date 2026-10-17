// The caller retains the existing frame lock across recording and original API
// calls. All resource queries use the engine's currently bound input object.
#include "final_transfer_policy.h"
#include "final_transfer_bracket.h"
#include <exception>
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
namespace bracket {
using Snapshot=gears_final_transfer_bracket::Snapshot;
gears_final_transfer_bracket::Budget budget;
struct Row {Snapshot entry{},draw{};gears_final_transfer_bracket::DrawCalls calls{};unsigned difference{};long long exit_qpc{};bool exited{},unwinding{};};
std::array<Row,16> rows{};
std::array<uint64_t,6> entry_reasons{};
thread_local Row* current{};
void environment(Snapshot& value){
    value.qpc=now();value.world=committed_world_generation.load();
    value.chain=final_eye_inventory.chain;value.inventory=final_eye_inventory.epoch;
    value.resize=swapchain_bindings.registered(value.chain).generation;
}
void pair(Snapshot& value){
    auto const& s=value.input.source;
    if(value.list.identity&&value.list.generation&&s.resource){
        auto result=frame_link.current_recorded_pair(value.list,{s.resource,s.width,s.height});
        value.ticket=result.ticket;value.pair_linked=result.linked;value.pair_rejection=result.rejection;
    }
}
// This is an independent, bounded entry observation. Do not reuse it in the
// actual draw's Input or in admission; the engine can bind lazily in original.
Snapshot entry(gears_final_transfer::Input input){
    Snapshot value{};value.input=input;environment(value);
    uintptr_t tls{},owner{},list{},table{},draw{};
    const bool context=safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
        safe_copy(&value.context,reinterpret_cast<void*>(tls+0x20),8)&&value.context;
    const bool list_valid=context&&safe_copy(&owner,reinterpret_cast<void*>(value.context+0x127c),8)&&owner&&
        safe_copy(&list,reinterpret_cast<void*>(owner+0x14),8)&&list&&
        safe_copy(&table,reinterpret_cast<void*>(list),8)&&table&&
        safe_copy(&draw,reinterpret_cast<void*>(table+12*8),8)&&system_resource_methods(list)&&
        viewport_targets[3]&&draw==reinterpret_cast<uintptr_t>(viewport_targets[3]);
    value.raw_list=list;
    if(list_valid){
        value.list=frame_link.current(canonical_identity(reinterpret_cast<IUnknown*>(list)));
        value.input.binding=frame_target_stats.binding(list);value.input.rectangles=frame_target_stats.draw_state(list);
        value.input.known_buffer=swapchain_bindings.known_buffer(value.input.binding.target.resource);
        value.input.descriptor_lost=frame_target_stats.descriptor_capacity_lost();
    }
    unsigned char count{};ResourceIdentity identity{};
    if(context&&safe_copy(&count,reinterpret_cast<void*>(value.context+0x65d),1)&&count==1&&
       safe_copy(&value.source_object,reinterpret_cast<void*>(value.context+0x768),8)&&value.source_object){
        query_bound_resource(value.source_object,identity);auto const& d=identity.desc;
        if(SUCCEEDED(identity.result)&&d.Width<=8192){value.input.slots=count;
            value.input.source={identity.canonical,unsigned(d.Dimension),unsigned(d.Width),d.Height,unsigned(d.DepthOrArraySize),
                unsigned(d.MipLevels),unsigned(d.Format),d.SampleDesc.Count};}
    }
    pair(value);
    if(!list_valid||!value.list.identity||!value.list.generation)value.reason|=1;
    if(value.input.binding.targets!=1||!value.input.binding.target.resource||value.input.descriptor_lost)value.reason|=2;
    if(!value.input.source.resource||!value.input.source.width||!value.input.source.height)value.reason|=4;
    if(!value.pair_linked||!value.ticket)value.reason|=8;
    if(!value.world||!final_eye_inventory.valid||!value.chain)value.reason|=16;
    if(!value.input.rectangles.tracked||value.input.rectangles.viewport_count!=1||value.input.rectangles.scissor_count!=1)value.reason|=32;
    return value;
}
struct Scope {
    Row* saved{};Row* row{};int exceptions{};
    Scope(uintptr_t caller,std::array<float,8> args,std::array<unsigned,4> dimensions):saved(current),exceptions(std::uncaught_exceptions()){
        current=nullptr; // Nested unrelated quads never become outer evidence.
        if(caller!=0x9a7377||!enabled()||!outage_diagnostics.load()||!recording.load())return;
        std::lock_guard<std::recursive_mutex> guard(frame_hook_mutex);
        if(!budget.preflight())return;
        gears_final_transfer::Input input{};input.quad_caller=caller;input.args=args;input.dimensions=dimensions;
        auto value=entry(input);
        for(unsigned bit=0;bit<entry_reasons.size();++bit)if(value.reason&(1u<<bit))++entry_reasons[bit];
        const int index=budget.retain(value.reason==0);if(index<0)return;
        row=&rows[unsigned(index)];row->entry=value;current=row;
    }
    ~Scope(){
        if(row){std::lock_guard<std::recursive_mutex> guard(frame_hook_mutex);
            row->exit_qpc=now();row->exited=true;row->unwinding=std::uncaught_exceptions()>exceptions;}
        current=saved;
    }
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
};
void draw_call(uintptr_t raw_list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances,bool bypass){
    if(!current)return;std::lock_guard<std::recursive_mutex> guard(frame_hook_mutex);
    current->calls.call({raw_list,caller,kind,elements,instances},bypass);
}
void actual(gears_final_transfer::Input const& input,gears_frame_link::ListRef ref,uintptr_t raw_list,uintptr_t context,uintptr_t object){
    // Existing Draw owner holds frame_hook_mutex. Preserve the independent
    // hook counter even when binding/other filters prevent observe() routing.
    if(!current)return;auto& row=*current;
    row.calls.match({raw_list,input.draw_caller,input.kind,input.elements,input.instances});
    if(row.calls.matched_draws!=1)return;
    auto& value=row.draw;value.input=input;value.list=ref;value.raw_list=raw_list;
    value.context=context;value.source_object=object;environment(value);pair(value);
    row.difference=gears_final_transfer_bracket::compare(row.entry,value);
}
void print(FILE* file){
    if(active.load())return; // Rows are emitted only after callback drain.
    fprintf(file,"final_transfer_bracket attempted=%u maximum_attempts=64 rows=%u rejected_rows=%u complete_rows=%u skipped=%u dropped=%u row_limit=16 first_draw_only=1 all_draws_scope=existing_draw_hooks matched_draws_scope=existing_transfer_observe bypass_separate=1 observe_only=1 entry_may_be_lazily_bound=1 gpu_proven=0\n",
        budget.attempted,budget.rejected+budget.complete,budget.rejected,budget.complete,budget.skipped,budget.dropped);
    fprintf(file,"final_transfer_bracket_entry_reasons list=%llu target=%llu source=%llu pair=%llu inventory=%llu rectangles=%llu\n",
        entry_reasons[0],entry_reasons[1],entry_reasons[2],entry_reasons[3],entry_reasons[4],entry_reasons[5]);
    for(unsigned i=0;i<rows.size();++i){auto const& row=rows[i];if(!row.entry.qpc)continue;
        auto const& first=row.calls.first;
        fprintf(file,"final_transfer_bracket_row row=%u entry_reason=%u all_draws=%llu matched_draws=%llu bypass_draws=%llu first_draw=%llx,%llx,%u,%u,%u exactly_one_routed_draw=%u difference=%u exited=%u unwinding=%u exit_qpc=%lld reason_bits=list:1,target:2,source:4,pair:8,inventory:16,rectangles:32 difference_bits=list:1,source:2,target:4,rectangles:8,world:16,inventory:32,pair:64,quad:128\n",
            i,row.entry.reason,row.calls.all_draws,row.calls.matched_draws,row.calls.bypass_draws,
            static_cast<unsigned long long>(first.raw_list),static_cast<unsigned long long>(first.caller),first.kind,first.elements,first.instances,
            unsigned(row.exited&&!row.unwinding&&row.calls.exactly_one_routed_draw()),row.difference,unsigned(row.exited),unsigned(row.unwinding),row.exit_qpc);
        auto snapshot=[&](const char* phase,Snapshot const& v){auto const& in=v.input;auto const& s=in.source;auto const& t=in.binding.target;auto const& r=in.rectangles;
            fprintf(file,"final_transfer_bracket_snapshot row=%u phase=%s qpc=%lld world=%llu chain=%llx inventory=%llu resize=%llu context=%llx raw_list=%llx list=%llx generation=%llu source_object=%llx source=%llx source_desc=%u,%u,%u,%u,%u,%u,%u slots=%u target=%llx target_desc=%llu,%u,%u,%u targets=%u known_buffer=%u descriptor_lost=%u pair_linked=%u ticket=%llu pair_rejection=%u draw=%llx,%u,%u,%u tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g depth=%u,%.9g,%.9g args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u\n",
                i,phase,v.qpc,v.world,static_cast<unsigned long long>(v.chain),v.inventory,v.resize,static_cast<unsigned long long>(v.context),static_cast<unsigned long long>(v.raw_list),static_cast<unsigned long long>(v.list.identity),v.list.generation,
                static_cast<unsigned long long>(v.source_object),static_cast<unsigned long long>(s.resource),s.dimension,s.width,s.height,s.depth,s.mips,s.format,s.samples,in.slots,
                static_cast<unsigned long long>(t.resource),t.width,t.height,t.format,t.mip,in.binding.targets,unsigned(in.known_buffer),unsigned(in.descriptor_lost),unsigned(v.pair_linked),v.ticket,v.pair_rejection,
                static_cast<unsigned long long>(in.draw_caller),in.kind,in.elements,in.instances,unsigned(r.tracked),r.viewport_count,r.scissor_count,
                r.viewport.x,r.viewport.y,r.viewport.width,r.viewport.height,r.scissor.x,r.scissor.y,r.scissor.width,r.scissor.height,unsigned(r.depth_known),r.min_depth,r.max_depth,
                in.args[0],in.args[1],in.args[2],in.args[3],in.args[4],in.args[5],in.args[6],in.args[7],in.dimensions[0],in.dimensions[1],in.dimensions[2],in.dimensions[3]);};
        snapshot("entry",row.entry);if(row.calls.matched_draws)snapshot("actual_draw",row.draw);
    }
}
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
        verify_range(file,0x9a6adc,0x31,"0d960a5c0790633f40f677d59975f25a72d40b36d63f2f0fd98dbcc4ab7a1a04")&&
        verify_range(file,0x2b0c04,0x651,"5088f5f7fb8c718769016b98194413e42aa466a30bfdfeab01d081c07075515d")&&
        verify_range(file,0x8ee78c,0x3ee,"024e4777801ff3236360d5ead3b10a2a02717438020dd634537a47723453e138");
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
    bracket::actual(input,ref,reinterpret_cast<uintptr_t>(list),context,object);
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
    bracket::print(file);
}
}
