#include "ngx_output_pixels_live.h"
#include "ngx_stereo_host_live.h"
namespace ngx_route_observer {
#if defined(GEARS_NGX_ROUTE_OBSERVER) && GEARS_NGX_ROUTE_OBSERVER
using namespace gears_ngx_route;
using CreateFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,const void*);
using EvaluateFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,const void*);
CreateFn original_create{};EvaluateFn original_evaluate{};void* create_target{};void* evaluate_target{};
bool attested{},printed{};Recorder recorder;thread_local Row* current{};thread_local unsigned depth{};
template<class T> bool read(T& out,uintptr_t address,uintptr_t offset=0){return address&&offset<=UINTPTR_MAX-address&&
    sizeof(T)-1<=UINTPTR_MAX-(address+offset)&&safe_copy(&out,reinterpret_cast<void const*>(address+offset),sizeof(T));}
Ownership ownership(bool world){
    Ownership out;out.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);uintptr_t slot{};
    if(read(slot,__readgsqword(0x58))&&read(out.context,slot,0x20)&&read(out.view,out.context,0x14c0))out.mask|=1;
    if(read(out.views,out.renderer,0x68))out.mask|=2;if(read(out.count,out.renderer,0x70))out.mask|=4;
    unsigned eye{};const auto generation=committed_world_generation.load();
    if(world&&recording.load()&&frame_link_diagnostics&&out.mask==7&&out.context&&consuming_frozen_frame.enabled&&
       consuming_frame_ticket&&consuming_frame_ticket==consuming_frozen_frame.id&&generation&&generation==consuming_frozen_frame.world_generation&&
       gears_unpossessed_quad::native_eye_index(out.view,out.views,out.count,eye)){
        out.associated=true;out.world=generation;out.ticket=consuming_frame_ticket;out.eye=eye;}
    return out;
}
Sample sample(Row const& row,bool world){
    Sample out;out.ownership=ownership(world);
    if(read(out.handle,base,handle_cell_rva))out.slots_mask|=1;if(read(out.parameters,base,parameter_cell_rva))out.slots_mask|=2;
    if(!row.evaluate){for(unsigned i=0;i<6;++i)if(read(out.create[i],row.structure,i*4))out.fields_mask|=1u<<i;
        uint8_t raw{};if(read(raw,row.structure,0x18)){out.create[6]=raw;out.fields_mask|=64;}return out;}
    for(unsigned i=0;i<resource_offsets.size();++i)if(read(out.resources[i],row.structure,resource_offsets[i]))out.fields_mask|=1u<<i;
    for(unsigned i=0;i<value_offsets.size();++i)if(read(out.values[i],row.structure,value_offsets[i]))out.fields_mask|=1u<<(i+4);
    return out;
}
struct Call {
    Row* row{};Row* parent{};unsigned previous_depth{};bool world{};int exceptions{};
    Call(bool eval,bool owned,uintptr_t caller,std::array<uintptr_t,5> args,const void* structure)
        :parent(current),previous_depth(depth),world(owned),exceptions(std::uncaught_exceptions()){
        current=nullptr;++depth;
        {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
            if(attested&&recording.load()){row=recorder.reserve(eval);if(previous_depth)++recorder.nested;}
            else ++recorder.bypassed;}
        // Reserved rows never move/reuse. Only their TLS owner writes them.
        if(parent)parent->nested=true;
        if(!row)return;current=row;row->evaluate=eval;row->depth=depth;row->nested=previous_depth!=0;row->thread=GetCurrentThreadId();
        row->caller=caller;row->args=args;row->structure=reinterpret_cast<uintptr_t>(structure);row->entry=now();row->before=sample(*row,world);
    }
    ~Call(){if(row){row->after=sample(*row,world);row->exit=now();row->exited=true;row->unwinding=std::uncaught_exceptions()>exceptions;}
        current=parent;depth=previous_depth;}
    Call(Call const&)=delete;Call& operator=(Call const&)=delete;
};
uint64_t __fastcall hook_create(uintptr_t first,uintptr_t second,uintptr_t third,uintptr_t fourth,uintptr_t fifth,const void* structure){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;ngx_stereo_host::WrapperScope host(false,scope.world(),caller,first,structure);ngx_output_pixels::CreateScope pixels(scope.world(),caller,first,structure);Call call(false,scope.world(),caller,{first,second,third,fourth,fifth},structure);
    const auto result=ngx_stereo_host::create_passthrough(first,second,third,fourth,fifth,structure,original_create);host.normal_return(result);pixels.normal_return();if(call.row){call.row->result=result;call.row->returned=true;}return result;
}
uint64_t __fastcall hook_evaluate(uintptr_t first,uintptr_t second,uintptr_t third,const void* structure){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope scope;ngx_stereo_host::WrapperScope host(true,scope.world(),caller,first,structure);ngx_output_pixels::EvalScope pixels(scope.world(),caller,first,structure);Call call(true,scope.world(),caller,{first,second,third,0,0},structure);
    const auto result=ngx_stereo_host::evaluate_passthrough(scope.world(),first,second,third,structure,original_evaluate);host.normal_return(result);pixels.normal_return(result);if(call.row){call.row->result=result;call.row->returned=true;}return result;
}
bool verify(FILE* file,uintptr_t rva,unsigned size,const char* expected){
    // Host contracts include complete compute helpers larger than the NGX
    // Eval wrapper. Keep a bounded buffer, but do not truncate their hashes.
    constexpr size_t contract_capacity=4096;
    static_assert(evaluate_size<=contract_capacity,"NGX wrapper must fit contract verification");
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::array<unsigned char,contract_capacity> bytes{};
    bool ok=size&&size<=bytes.size()&&BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=safe_copy(bytes.data(),reinterpret_cast<void*>(base+rva),size)&&BCryptHashData(hash,bytes.data(),size,0)>=0;
    unsigned char digest[32]{};char hex[65]{};if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    ok=ok&&!strcmp(hex,expected);
    fprintf(file,"ngx_route_identity rva=%llx size=%u sha256=%s match=%u module_base=%llx historical_module_base=140000000 source=historical_runtime_artifact1 snapshot_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 handle_cell_rva=1c20bd0 parameter_cell_rva=1c20bd8 cells_source=authenticated_wrapper_rip_operands\n",
        static_cast<unsigned long long>(rva),size,hex,unsigned(ok),static_cast<unsigned long long>(base));return ok;
}
bool enable(FILE* file){
    attested=false;printed=false;recorder={};create_target=reinterpret_cast<void*>(base+create_rva);evaluate_target=reinterpret_cast<void*>(base+evaluate_rva);
    if(self_testing||!continuous_session||!frame_link_diagnostics)return false;
    const bool create=verify(file,create_rva,create_size,create_sha256),eval=verify(file,evaluate_rva,evaluate_size,evaluate_sha256);
    attested=create&&eval;return attested;
}
void print_sample(FILE* file,Row const& row,unsigned index,const char* stage,Sample const& s){
    auto const& o=s.ownership;
    fprintf(file,"ngx_route_sample kind=%s row=%u stage=%s slots_mask=%u handle=%llx parameters=%llx fields_mask=%x missing_reads=%u tls_associated=%u world=%llu ticket=%llu tls_eye=%u logical_eye_verified=0 renderer=%llx context=%llx current_view=%llx renderer_views=%llx renderer_count=%d ownership_mask=%u\n",
        row.evaluate?"evaluate":"create",index,stage,s.slots_mask,static_cast<unsigned long long>(s.handle),static_cast<unsigned long long>(s.parameters),s.fields_mask,unsigned(s.slots_mask!=3||s.fields_mask!=(row.evaluate?0x7ffffu:0x7fu)),
        unsigned(o.associated),o.world,o.ticket,o.eye,static_cast<unsigned long long>(o.renderer),static_cast<unsigned long long>(o.context),static_cast<unsigned long long>(o.view),static_cast<unsigned long long>(o.views),o.count,o.mask);
    if(!row.evaluate){fprintf(file,"ngx_route_create_fields row=%u stage=%s width=%u height=%u out_width=%u out_height=%u quality_raw=%u flags_raw=%u output_subrects_byte_raw=%u\n",
        index,stage,s.create[0],s.create[1],s.create[2],s.create[3],s.create[4],s.create[5],s.create[6]);return;}
    auto const& v=s.values;
    fprintf(file,"ngx_route_evaluate_fields row=%u stage=%s color_raw=%llx output_raw=%llx depth_raw=%llx mv_raw=%llx render_width=%u render_height=%u reset_raw=%u jitter_x_bits=%08x jitter_y_bits=%08x mv_scale_x_bits=%08x mv_scale_y_bits=%08x color_base=%u,%u depth_base=%u,%u mv_base=%u,%u output_base=%u,%u resource_types_verified=0\n",
        index,stage,static_cast<unsigned long long>(s.resources[0]),static_cast<unsigned long long>(s.resources[1]),static_cast<unsigned long long>(s.resources[2]),static_cast<unsigned long long>(s.resources[3]),
        v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10],v[11],v[12],v[13],v[14]);
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(printed)return;printed=true;
    // Cleanup has a deadline: only a real active drain with recording closed
    // publishes all TLS owner writes. Timeout prints counters, never row fields.
    const bool available=!recording.load()&&active.load()==0;
    fprintf(file,"ngx_route_observer enabled=1 attested=%u creates=%u evaluations=%u create_limit=4 evaluate_limit=8 limited_create=%llu limited_evaluate=%llu nested=%llu bypassed=%llu rows_unavailable=%u cpu_evidence_only=1 consumed_handle_verified=0 sdk_result_semantics_verified=0 gpu_verified=0 temporal_history_verified=0 sr_verified=0\n",
        unsigned(attested),recorder.create_count,recorder.evaluate_count,recorder.limited_create,recorder.limited_evaluate,recorder.nested,recorder.bypassed,unsigned(!available));
    if(!available)return;
    auto rows=[&](auto const& values,unsigned count){for(unsigned i=0;i<count;++i){auto const& row=values[i];
        fprintf(file,"ngx_route_call kind=%s row=%u thread=%u caller=%llx depth=%u nested=%u entry=%lld exit=%lld exited=%u unwinding=%u result_valid=%u result_rax=%016llx structure=%llx args=%llx,%llx,%llx,%llx,%llx slots_changed=%u ownership_changed=%u\n",
            row.evaluate?"evaluate":"create",i,row.thread,static_cast<unsigned long long>(row.caller),row.depth,unsigned(row.nested),row.entry,row.exit,unsigned(row.exited),unsigned(row.unwinding),unsigned(row.returned),row.result,
            static_cast<unsigned long long>(row.structure),static_cast<unsigned long long>(row.args[0]),static_cast<unsigned long long>(row.args[1]),static_cast<unsigned long long>(row.args[2]),static_cast<unsigned long long>(row.args[3]),static_cast<unsigned long long>(row.args[4]),
            unsigned(row.before.handle!=row.after.handle||row.before.parameters!=row.after.parameters||row.before.slots_mask!=row.after.slots_mask),
            unsigned(row.before.ownership.associated!=row.after.ownership.associated||row.before.ownership.world!=row.after.ownership.world||row.before.ownership.ticket!=row.after.ownership.ticket||row.before.ownership.view!=row.after.ownership.view));
        print_sample(file,row,i,"before",row.before);print_sample(file,row,i,"after",row.after);}};
    rows(recorder.creates,recorder.create_count);rows(recorder.evaluations,recorder.evaluate_count);
}
#else
bool enable(FILE*){return true;}void print(FILE*){}
#endif
}
