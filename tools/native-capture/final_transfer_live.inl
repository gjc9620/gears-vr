// The caller retains the existing frame lock across recording and original API
// calls. All resource queries use the engine's currently bound input object.
#include "final_transfer_policy.h"
#include "final_transfer_bracket.h"
#include "final_transfer_normalize.h"
#include <exception>
namespace final_transfer_live {
std::atomic<bool> attested{};
std::atomic<bool> pending_code_attested{};
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
using Pending=gears_final_transfer_bracket::Pending;
struct Row {Snapshot entry{},draw{};gears_final_transfer_bracket::DrawCalls calls{};unsigned difference{};
    Pending pending_entry{},pending_draw{};gears_final_transfer_bracket::ConsumedBinds binds{},draw_binds{};
    unsigned pending_entry_reason{},pending_draw_reason{};long long exit_qpc{};bool exited{},unwinding{};};
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
Pending read_pending_once(uintptr_t context){
    Pending p{};if(!context)return p;
    unsigned char count{},vp{},sc{};
    auto read=[&](void* out,uintptr_t address,size_t size){return safe_copy(out,reinterpret_cast<void*>(address),size);};
    p.readable=read(&count,context+0x563,1)&&read(&vp,context+0x560,1)&&read(&sc,context+0x561,1)&&
        read(&p.dirty[0],context+0x9f,1)&&read(&p.dirty[1],context+0xa3,1)&&read(&p.dirty[2],context+0xa4,1);
    p.count=count;p.viewport_count=vp;p.scissor_count=sc;
    // One view/handle/rectangle only. MRT or array counts are not traversed.
    if(!p.readable||count!=1||vp!=1||sc!=1)return p;
    p.readable=read(&p.view,context+0x590,8)&&p.view&&read(&p.handle,p.view+0xc,8)&&p.handle&&
        read(p.viewport.data(),context+0x290,sizeof(p.viewport))&&read(p.scissor.data(),context+0x410,sizeof(p.scissor));
    if(!p.readable)return p;
    const auto descriptor=frame_target_stats.descriptor_snapshot(p.handle);
    p.target=descriptor.target;p.trace=descriptor.provenance.trace;
    p.known_buffer=swapchain_bindings.known_buffer(p.target.resource);p.descriptor_lost=frame_target_stats.descriptor_capacity_lost();
    return p;
}
Pending read_pending(uintptr_t context){
    const auto a=read_pending_once(context);auto b=read_pending_once(context);
    b.stable=gears_final_transfer_bracket::equal_pending(a,b);return b;
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
        value.provenance=frame_target_stats.binding_provenance(list);
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
        std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
        if(!budget.preflight())return;
        gears_final_transfer::Input input{};input.quad_caller=caller;input.args=args;input.dimensions=dimensions;
        auto value=entry(input);
        for(unsigned bit=0;bit<entry_reasons.size();++bit)if(value.reason&(1u<<bit))++entry_reasons[bit];
        const int index=budget.retain(value.reason==0);if(index<0)return;
        row=&rows[unsigned(index)];row->entry=value;current=row;
        if(!pending_code_attested.load())row->pending_entry_reason=gears_final_transfer_bracket::pending_unattested;
        else if(value.reason&(1u|16u))row->pending_entry_reason=gears_final_transfer_bracket::pending_invalid;
        else {row->pending_entry=read_pending(value.context);
            row->pending_entry_reason=gears_final_transfer_bracket::pending_reason(row->pending_entry,value);}
    }
    ~Scope(){
        if(row){std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
            row->exit_qpc=now();row->exited=true;row->unwinding=std::uncaught_exceptions()>exceptions;}
        current=saved;
    }
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
};
void draw_call(uintptr_t raw_list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances,bool bypass){
    if(!current)return;std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    current->calls.call({raw_list,caller,kind,elements,instances},bypass);
}
void consumed_target(uintptr_t list,unsigned count,const D3D12_CPU_DESCRIPTOR_HANDLE* handles,bool bypass){
    if(!current||!pending_code_attested.load())return;std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};const bool readable=count==1&&safe_copy(&handle,handles,sizeof(handle));
    current->binds.consume(0,list==current->entry.raw_list,readable&&handle.ptr==current->pending_entry.handle,bypass);
}
void consumed_viewport(uintptr_t list,unsigned count,const D3D12_VIEWPORT* values,bool bypass){
    if(!current||!pending_code_attested.load())return;std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    std::array<float,6> value{};const bool readable=count==1&&safe_copy(value.data(),values,sizeof(value));
    current->binds.consume(1,list==current->entry.raw_list,readable&&value==current->pending_entry.viewport,bypass);
}
void consumed_scissor(uintptr_t list,unsigned count,const D3D12_RECT* values,bool bypass){
    if(!current||!pending_code_attested.load())return;std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    std::array<int32_t,4> value{};const bool readable=count==1&&safe_copy(value.data(),values,sizeof(value));
    current->binds.consume(2,list==current->entry.raw_list,readable&&value==current->pending_entry.scissor,bypass);
}
void actual(gears_final_transfer::Input const& input,gears_frame_link::ListRef ref,uintptr_t raw_list,uintptr_t context,uintptr_t object){
    // Existing Draw owner holds frame_hook_mutex. Preserve the independent
    // hook counter even when binding/other filters prevent observe() routing.
    if(!current)return;auto& row=*current;
    row.calls.match({raw_list,input.draw_caller,input.kind,input.elements,input.instances});
    if(row.calls.matched_draws!=1)return;
    auto& value=row.draw;value.input=input;value.list=ref;value.raw_list=raw_list;
    value.context=context;value.source_object=object;environment(value);pair(value);
    value.provenance=frame_target_stats.binding_provenance(raw_list);
    row.difference=gears_final_transfer_bracket::compare(row.entry,value);
    row.draw_binds=row.binds;
    if(!pending_code_attested.load())row.pending_draw_reason=gears_final_transfer_bracket::pending_unattested;
    else if(!value.context||!ref.identity||!ref.generation)row.pending_draw_reason=gears_final_transfer_bracket::pending_invalid;
    else {row.pending_draw=read_pending(context);
        row.pending_draw_reason=row.pending_entry_reason|gears_final_transfer_bracket::compare_pending(row.pending_entry,row.pending_draw,value,row.draw_binds);}
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
                in.args[0],in.args[1],in.args[2],in.args[3],in.args[4],in.args[5],in.args[6],in.args[7],in.dimensions[0],in.dimensions[1],in.dimensions[2],in.dimensions[3]);
            auto const& p=v.provenance;auto const& trace=p.trace;
            fprintf(file,"final_transfer_bound_descriptor row=%u phase=%s handle=%llx trace=%llu,%llu,%llu,%llu,%llx,%llx,%llu\n",i,phase,
                static_cast<unsigned long long>(p.handle),trace.write_serial,trace.origin_create_serial,trace.create_epoch,trace.last_write_epoch,
                static_cast<unsigned long long>(trace.copy_source_handle),static_cast<unsigned long long>(trace.creation_live),trace.source_write_serial);};
        snapshot("entry",row.entry);if(row.calls.matched_draws)snapshot("actual_draw",row.draw);
        auto pending=[&](const char* phase,Pending const& p,unsigned reason){
            fprintf(file,"final_transfer_pending row=%u phase=%s code_attested=%u reason=%u readable=%u stable=%u counts=%u,%u,%u dirty=%u,%u,%u view=%llx handle=%llx target=%llx target_desc=%llu,%u,%u,%u known_buffer=%u descriptor_lost=%u trace=%llu,%llu,%llu,%llu,%llx,%llx,%llu viewport=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g scissor=%d,%d,%d,%d reason_bits=invalid:1,target:2,provenance:4,rectangles:8,not_dirty_disagree:16,changed:32,dirty_bind:64,actual_disagree:128,unattested:256\n",
                i,phase,unsigned(pending_code_attested.load()),reason,unsigned(p.readable),unsigned(p.stable),p.count,p.viewport_count,p.scissor_count,
                unsigned(p.dirty[0]),unsigned(p.dirty[1]),unsigned(p.dirty[2]),static_cast<unsigned long long>(p.view),static_cast<unsigned long long>(p.handle),static_cast<unsigned long long>(p.target.resource),p.target.width,p.target.height,p.target.format,p.target.mip,
                unsigned(p.known_buffer),unsigned(p.descriptor_lost),p.trace.write_serial,p.trace.origin_create_serial,p.trace.create_epoch,p.trace.last_write_epoch,
                static_cast<unsigned long long>(p.trace.copy_source_handle),static_cast<unsigned long long>(p.trace.creation_live),p.trace.source_write_serial,
                p.viewport[0],p.viewport[1],p.viewport[2],p.viewport[3],p.viewport[4],p.viewport[5],p.scissor[0],p.scissor[1],p.scissor[2],p.scissor[3]);};
        pending("entry",row.pending_entry,row.pending_entry_reason);if(row.calls.matched_draws)pending("actual_draw",row.pending_draw,row.pending_draw_reason);
        for(unsigned kind=0;kind<3;++kind)fprintf(file,"final_transfer_pending_binds row=%u kind=%u calls=%llu matched=%llu mismatched=%llu bypassed=%llu wrong_list=%llu wrong_args=%llu before_draw_calls=%llu before_draw_matched=%llu before_draw_mismatched=%llu before_draw_bypassed=%llu before_draw_wrong_list=%llu before_draw_wrong_args=%llu kind_names=om:0,viewport:1,scissor:2\n",
            i,kind,row.binds.calls[kind],row.binds.matched[kind],row.binds.mismatched[kind],row.binds.bypassed[kind],row.binds.wrong_list[kind],row.binds.wrong_args[kind],row.draw_binds.calls[kind],row.draw_binds.matched[kind],row.draw_binds.mismatched[kind],row.draw_binds.bypassed[kind],row.draw_binds.wrong_list[kind],row.draw_binds.wrong_args[kind]);
    }
}
}
namespace normalizer {
#if defined(GEARS_FINAL_TRANSFER_NORMALIZE) && GEARS_FINAL_TRANSFER_NORMALIZE
struct Scope;
thread_local Scope* current{};
std::atomic<bool> faulted{};
uint64_t considered{},preflight_rejected{},identity_passthrough{},planned{},linked_count{},completed_count{},failed_count{};
const char* first_failure="none";const char* first_preflight="none";
std::array<float,8> first_original{},first_effective{};
std::array<float,8> failed_original{},failed_effective{};
void fault(const char* stage);
struct Scope {
    Scope* saved{};std::unique_lock<std::recursive_mutex> guard;
    bracket::Snapshot entry{},draw{};bracket::Pending pending{};
    gears_final_transfer_normalize::Plan plan{};
    gears_final_transfer_bracket::DrawCalls calls{};
    gears_final_transfer_bracket::ConsumedBinds binds{},draw_binds{};
    uint64_t accepted{};int exceptions{};bool mutated{},nested{};
    Scope(uintptr_t caller,std::array<float,8> args,std::array<unsigned,4> dimensions,bool world):saved(current),exceptions(std::uncaught_exceptions()){
        if(saved){saved->nested=true;fault("nested_quad");current=nullptr;return;}
        if(caller!=0x9a7377||!gears_final_transfer_normalize::allowed_mode(enabled(),pending_code_attested.load(),
            recording.load(),world,self_testing,owned_bindings_enabled,faulted.load()))return;
        guard=std::unique_lock<FrameHookMutex>(frame_hook_mutex);
        if(faulted.load()){guard.unlock();return;}++considered;
        auto reject=[&](const char* stage){++preflight_rejected;if(!std::strcmp(first_preflight,"none"))first_preflight=stage;guard.unlock();};
        gears_final_transfer::Input input{};input.quad_caller=caller;input.args=args;input.dimensions=dimensions;
        entry=bracket::entry(input);
        if(entry.reason){reject("entry");return;}
        pending=bracket::read_pending(entry.context);
        if(gears_final_transfer_bracket::pending_reason(pending,entry)){reject("pending");return;}
        auto prospective=entry.input;prospective.binding.targets=1;prospective.binding.target=pending.target;
        prospective.known_buffer=pending.known_buffer;prospective.descriptor_lost=pending.descriptor_lost;
        prospective.rectangles={true,gears_final_transfer_bracket::pending_viewport(pending),gears_final_transfer_bracket::pending_scissor(pending),1,1,true,pending.viewport[4],pending.viewport[5]};
        plan=gears_final_transfer_normalize::plan(prospective);
        if(!plan.valid){reject("plan");return;}
        if(!plan.modified){++identity_passthrough;guard.unlock();return;}
        if(!planned){first_original=plan.original;first_effective=plan.effective;}
        ++planned;mutated=true;current=this;
    }
    ~Scope(){
        if(mutated){
            const bool exception=std::uncaught_exceptions()>exceptions;
            if(!faulted.load()&&!exception&&calls.exactly_one_routed_draw()&&accepted==1&&!nested){
                gears_final_transfer::Input input{};input.quad_caller=entry.input.quad_caller;
                input.args=plan.effective;input.dimensions=entry.input.dimensions;
                auto post=bracket::entry(input);
                const auto difference=gears_final_transfer_bracket::compare(draw,post);
                // The explicit transfer retires its source-local pair. All
                // other post-call fields must still match the certified draw.
                if((post.reason&(1u|2u|4u|16u|32u))||(difference&~unsigned(gears_final_transfer_bracket::pair)))fault("completion_identity");
                else if(gears_final_transfer_bracket::compare_pending(pending,bracket::read_pending(post.context),post,binds))fault("completion_pending");
                else if(binds.calls!=draw_binds.calls)fault("post_draw_bind");
            }
            if(!gears_final_transfer_normalize::completion(calls,accepted,exception,nested,faulted.load()?1u:0u))fault(exception?"exception":"completion");
            else ++completed_count;
        }
        current=saved;
    }
    void forward(float& u,float& v,float& uw,float& vh)const{if(mutated){u=plan.effective[4];v=plan.effective[5];uw=plan.effective[6];vh=plan.effective[7];}}
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
};
void fault(const char* stage){
    if(!faulted.exchange(true)){first_failure=stage;++failed_count;
        if(current){failed_original=current->plan.original;failed_effective=current->plan.effective;}}
    // Discard the uncertain current window; publication is latched off for
    // this session, including any already recorded transfer from this scope.
    frame_link.invalidate();
}
bool scoped(){return current&&current->mutated;}
bool publication_allowed(){return !faulted.load();}
void boundary(){if(scoped())fault("execute_or_present_inside_quad");}
void draw_call(uintptr_t list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances,bool bypass){
    if(!scoped())return;auto& s=*current;s.calls.call({list,caller,kind,elements,instances},bypass);
    if(bypass||s.calls.all_draws!=1||list!=s.entry.raw_list||caller!=0x1af92b||kind!=1||elements!=6||instances!=1)fault("draw_hook");
}
void bind(unsigned kind,uintptr_t list,unsigned count,const void* values,bool bypass){
    if(!scoped())return;auto& s=*current;bool match=false;
    if(kind==0){D3D12_CPU_DESCRIPTOR_HANDLE h{};match=count==1&&safe_copy(&h,values,sizeof(h))&&h.ptr==s.pending.handle;}
    if(kind==1){std::array<float,6> v{};match=count==1&&safe_copy(v.data(),values,sizeof(v))&&v==s.pending.viewport;}
    if(kind==2){std::array<int32_t,4> v{};match=count==1&&safe_copy(v.data(),values,sizeof(v))&&v==s.pending.scissor;}
    s.binds.consume(kind,list==s.entry.raw_list,match,bypass);
    if(bypass||list!=s.entry.raw_list||!match)fault("consumed_bind");
}
bool validate(gears_final_transfer::Input const& input,gears_frame_link::ListRef ref,uintptr_t list,uintptr_t context,uintptr_t object){
    if(!scoped()||faulted.load())return false;auto& s=*current;
    s.calls.match({list,input.draw_caller,input.kind,input.elements,input.instances});
    auto& draw=s.draw;draw.input=input;draw.list=ref;draw.raw_list=list;draw.context=context;draw.source_object=object;
    bracket::environment(draw);bracket::pair(draw);draw.provenance=frame_target_stats.binding_provenance(list);s.draw_binds=s.binds;
    if(!s.calls.exactly_one_routed_draw()){fault("draw_routing");return false;}
    if(gears_final_transfer_normalize::draw_proof(s.entry,draw,s.plan,s.pending,bracket::read_pending(context),s.binds)){
        fault("draw_proof");return false;}
    return true;
}
void linked(bool success){if(!scoped())return;if(!success){fault("transfer_record");return;}++current->accepted;++linked_count;}
void print(FILE* file){
    if(active.load())return;
    fprintf(file,"final_transfer_normalize compiled=1 faulted=%u considered=%llu preflight_rejected=%llu identity_passthrough=%llu planned=%llu linked=%llu completed=%llu failures=%llu first_failure=%s first_preflight_rejection=%s original=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g effective=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g gpu_proven=0\n",
        unsigned(faulted.load()),considered,preflight_rejected,identity_passthrough,planned,linked_count,completed_count,failed_count,first_failure,first_preflight,
        first_original[0],first_original[1],first_original[2],first_original[3],first_original[4],first_original[5],first_original[6],first_original[7],
        first_effective[0],first_effective[1],first_effective[2],first_effective[3],first_effective[4],first_effective[5],first_effective[6],first_effective[7]);
    if(faulted.load())fprintf(file,"final_transfer_normalize_first_failure stage=%s original=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g effective=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",first_failure,
        failed_original[0],failed_original[1],failed_original[2],failed_original[3],failed_original[4],failed_original[5],failed_original[6],failed_original[7],
        failed_effective[0],failed_effective[1],failed_effective[2],failed_effective[3],failed_effective[4],failed_effective[5],failed_effective[6],failed_effective[7]);
}
#else
struct Scope {Scope(uintptr_t,std::array<float,8>,std::array<unsigned,4>,bool){}void forward(float&,float&,float&,float&)const{}};
constexpr bool scoped(){return false;}inline bool publication_allowed(){return true;}
void boundary(){}void draw_call(uintptr_t,uintptr_t,unsigned,unsigned,unsigned,bool){}void bind(unsigned,uintptr_t,unsigned,const void*,bool){}
bool validate(gears_final_transfer::Input const&,gears_frame_link::ListRef,uintptr_t,uintptr_t,uintptr_t){return false;}
void linked(bool){}void print(FILE* file){fprintf(file,"final_transfer_normalize compiled=0\n");}
#endif
}
bool verify_range(FILE* file,uintptr_t rva,ULONG size,const char* expected,const char* label="final_transfer_identity"){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::array<unsigned char,0x1000> bytes{};
    bool ok=size<=bytes.size()&&BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=safe_copy(bytes.data(),reinterpret_cast<void*>(base+rva),size)&&BCryptHashData(hash,bytes.data(),size,0)>=0;
    unsigned char digest[32]{};char hex[65]{};
    if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    ok=ok&&!strcmp(hex,expected);
    fprintf(file,"%s rva=%llx size=%lu sha256=%s match=%u source=historical_runtime_artifact1 module_base=%llx\n",
        label,static_cast<unsigned long long>(rva),size,hex,unsigned(ok),static_cast<unsigned long long>(base));return ok;
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
    bool pending_ok=ok;
    // Historical artifact1 hashes from pending-target-historical-asm.json.
    // Independent diagnostic authority: mismatch does not alter admission.
    pending_ok=verify_range(file,0x1af8fe,9,"03f238db561fe2c123de0356c34e987d91d22502d4daa40179423655f5869366","pending_target_identity")&&pending_ok;
    pending_ok=verify_range(file,0x177e90,55,"06e1fefa4c97cd3caf9c54ccfd8484e4613f7efa14225c50c6644123e7d373a7","pending_target_identity")&&pending_ok;
    pending_ok=verify_range(file,0x17862a,82,"09a2cf8f9adf0b04079b75e8e0fdf5d90074bc74f89d69de810af61ee3057b40","pending_target_identity")&&pending_ok;
    pending_ok=verify_range(file,0x1786bf,100,"c7b97be2b6cf5ee14259c396ea0903c64ad83c98d1e4d3958372d22b2b2e3d86","pending_target_identity")&&pending_ok;
    pending_ok=verify_range(file,0x178d9c,39,"092f6eeed12732aaaf7daaf57b3595b7ec55623dc81a2e6b3fb638b9cbd11698","pending_target_identity")&&pending_ok;
    pending_ok=verify_range(file,0x1793a6,39,"4f7475ea72ee212e8517c09ff63583ca331e1bdeafdda1cd3b03e5b3eec91be6","pending_target_identity")&&pending_ok;
    pending_code_attested.store(pending_ok);
    fprintf(file,"pending_target_code attested=%u source_blob_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 observe_only=1\n",unsigned(pending_ok));
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
    const bool normalized=normalizer::scoped();
    if(normalized?!normalizer::validate(input,ref,reinterpret_cast<uintptr_t>(list),context,object):!gears_final_transfer::eligible(input)){
        ++policy_rejected;return false;}
    const gears_frame_link::Target source{input.source.resource,input.source.width,input.source.height};
    const gears_frame_link::Target destination{binding.target.resource,unsigned(binding.target.width),binding.target.height};
    if(!frame_link.draw_transfer(ref,source,destination)){if(normalized)normalizer::linked(false);++pair_rejected;return false;}
    if(normalized)normalizer::linked(true);
    ++accepted;return true;
}
void copy_destination(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination){
    if(!enabled()||!recording.load())return;
    std::lock_guard<FrameHookMutex> guard(frame_hook_mutex);
    D3D12_TEXTURE_COPY_LOCATION value{};
    if(!safe_copy(&value,destination,sizeof(value))||!value.pResource){frame_link.invalidate();return;}
    const auto resource=canonical_identity(value.pResource);const auto ref=frame_link.current(canonical_identity(list));
    if(tracks(ref,resource)){frame_link.draw_unknown(ref,resource);++copy_writes;}
}
void print(FILE* file){
    fprintf(file,"final_transfer enabled=%u attested=%u attempts=%llu accepted=%llu policy_rejected=%llu pair_rejected=%llu source_draws=%llu observed_copy_writes=%llu same_list_only=1 geometry=same_width_height_delta_0_or_1 native_caller=9a7377 shader_bytecode_decoded=0 gpu_pixels_verified=0\n",
        unsigned(enabled()),unsigned(attested.load()),attempts,accepted,policy_rejected,pair_rejected,source_draws,copy_writes);
    bracket::print(file);
    normalizer::print(file);
}
}
