#include "death_source_control.h"
namespace death_source_live {
struct Prepared {gears_death_source::Decision decision{};gears_death_source::Receipt receipt{};};
#if defined(GEARS_DEATH_SOURCE_CONTROL) && GEARS_DEATH_SOURCE_CONTROL
constexpr bool configured=true;
std::atomic<bool> attested{};
gears_death_source::Recorder receipts;
std::array<uint64_t,gears_death_source::reject_count> rejected{};
bool other_controls(){
    return quad_source_control_enabled()||first_view_mask_control||submit_duplicate_control||
        submit_mono_control||submit_mono_independent_state||projection_control||origin_trial_enabled;
}
bool enable(FILE* file){
    const bool eligible=!self_testing&&continuous_session&&frame_link_diagnostics&&!owned_bindings_enabled&&!other_controls();
    const bool ok=eligible&&final_transfer_live::verify_range(file,0xd80a1b,128,
        "d276fe37daadd6dfeadd4a0e3154cf6fb7c053bf17757a3da5b9e0e0d54ff6dc");
    attested.store(ok);
    fprintf(file,"death_source_identity enabled=1 eligible_configuration=%u runtime_range_match=%u rva=d80a1b size=128 historical_code_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6\n",
        unsigned(eligible),unsigned(ok));
    return ok; // Reject hook activation when runtime bytes or scope disagree.
}
struct Pointers {
    uintptr_t tls{},context{},current{},views{},object{};int count{},capacity{};unsigned char slots{};
    bool operator==(Pointers const& p)const{return tls==p.tls&&context==p.context&&current==p.current&&views==p.views&&
        object==p.object&&count==p.count&&capacity==p.capacity&&slots==p.slots;}
};
bool read_pointers(uintptr_t renderer,Pointers& p){
    struct Header {uintptr_t views;int count,capacity;} h{};
    const bool ok=renderer&&renderer%8==0&&safe_copy(&p.tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&p.tls&&p.tls%8==0&&
        safe_copy(&p.context,reinterpret_cast<void*>(p.tls+0x20),8)&&p.context&&p.context%8==0&&
        safe_copy(&p.current,reinterpret_cast<void*>(p.context+0x14c0),8)&&
        safe_copy(&h,reinterpret_cast<void*>(renderer+0x68),sizeof(h))&&
        safe_copy(&p.slots,reinterpret_cast<void*>(p.context+0x65d),1)&&p.slots>=1&&p.slots<=32&&
        safe_copy(&p.object,reinterpret_cast<void*>(p.context+0x768),8)&&p.object&&p.object%8==0;
    p.views=h.views;p.count=h.count;p.capacity=h.capacity;return ok;
}
Prepared prepare(gears_death_source::Args const& args,uintptr_t caller){
    using namespace gears_death_source;
    Prepared p;p.decision.forwarded=args;
    if(caller!=0xd80a7b)return p;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    Gate g{};g.enabled=true;g.attested=attested.load();g.recording=recording.load();g.diagnostics=frame_link_diagnostics;
    g.frozen_enabled=consuming_frozen_frame.enabled;g.unpossessed=consuming_frozen_frame.unpossessed;
    g.ticket=consuming_frame_ticket;g.frozen_ticket=consuming_frozen_frame.id;g.renderer=consuming_renderer!=nullptr;
    g.other_control=other_controls();g.caller=caller;
    Pointers before{},after{};ResourceIdentity identity{};
    // Only the exact unpossessed right-call scope may inspect engine pointers.
    if(g.attested&&g.recording&&g.diagnostics&&g.frozen_enabled&&g.unpossessed&&g.renderer&&!g.other_control&&
       g.ticket&&g.ticket==g.frozen_ticket){
        const auto renderer=reinterpret_cast<uintptr_t>(consuming_renderer);
        g.native_read=read_pointers(renderer,before);g.current=before.current;g.views=before.views;g.count=before.count;g.capacity=before.capacity;
        auto unused_origin=receipts.origin;
        // This preliminary policy cannot start the clock: source_valid is false.
        if(decide(args,g,now(),qpc_frequency,unused_origin).rejection==source){
            query_bound_resource(before.object,identity);
            uintptr_t object_table{},wrapper{},wrapper_table{},native{};
            g.stable=read_pointers(renderer,after)&&before==after&&
                safe_copy(&object_table,reinterpret_cast<void*>(before.object),8)&&object_table==base+0x1667bb8&&
                safe_copy(&wrapper,reinterpret_cast<void*>(before.object+0x60),8)&&wrapper==identity.engine_wrapper&&wrapper&&
                safe_copy(&wrapper_table,reinterpret_cast<void*>(wrapper),8)&&wrapper_table==base+0x133ecf8&&
                safe_copy(&native,reinterpret_cast<void*>(wrapper+0xc),8)&&native==identity.native;
            g.source_valid=SUCCEEDED(identity.result)&&identity.native&&identity.resource&&identity.canonical;
            const auto& d=identity.desc;g.source_canonical=identity.canonical;g.source_width=d.Width;g.source_height=d.Height;
            g.source_format=unsigned(d.Format);g.source_dimension=unsigned(d.Dimension);g.source_array=d.DepthOrArraySize;
            g.source_mips=d.MipLevels;g.source_samples=d.SampleDesc.Count;
        }
    }
    p.decision=decide(args,g,now(),qpc_frequency,receipts.origin);
    if(p.decision.rejection!=accepted){++rejected[p.decision.rejection];return p;}
    p.receipt={g.ticket,committed_world_generation.load(),p.decision.qpc,p.decision.phase,
        args.rectangle[4],p.decision.forwarded.rectangle[4],identity.canonical};
    return p;
}
void complete(Prepared const& p){
    if(p.decision.rejection!=gears_death_source::accepted)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    receipts.completed(p.receipt); // Original CPU quad returned; no GPU claim.
}
void progress(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"death_source_enabled=1 death_source_attested=%u death_source_phase=%u death_source_a1_count=%llu death_source_b_count=%llu death_source_a2_count=%llu death_source_last_ticket=%llu death_source_last_qpc=%lld death_source_dropped=%llu ",
        unsigned(attested.load()),gears_death_source::phase_at(receipts.origin,now(),qpc_frequency),
        receipts.counts[0],receipts.counts[1],receipts.counts[2],receipts.last.ticket,receipts.last.qpc,receipts.dropped);
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"death_source_control enabled=1 attested=%u diagnostic_only=1 product_fix=0 caller=d80a7b right_source_u=half_to_zero sole_modified_argument=u destination_unchanged=1 source_format=26 source_extent=full_sbs first_eligible_origin_qpc=%lld qpc_frequency=%lld a1_seconds=1 b_seconds=1.5 a2_after_seconds=2.5 clock_never_reset=1 completed_cpu_quad_only=1 gpu_completed=0 capacity=512 rows=%u dropped=%llu modified=%llu a1_count=%llu b_count=%llu a2_count=%llu\n",
        unsigned(attested.load()),receipts.origin,qpc_frequency,receipts.size,receipts.dropped,receipts.modified,receipts.counts[0],receipts.counts[1],receipts.counts[2]);
    for(unsigned i=0;i<receipts.size;++i){const auto& r=receipts.rows[i];
        fprintf(file,"death_source_receipt row=%u ticket=%llu world=%llu qpc=%lld phase=%u original_u=%.9g effective_u=%.9g source_canonical=%llx\n",
            i,r.ticket,r.world,r.qpc,r.phase,r.original_u,r.effective_u,static_cast<unsigned long long>(r.source_canonical));}
    for(unsigned i=1;i<rejected.size();++i)if(rejected[i])fprintf(file,"death_source_rejected reason=%u count=%llu\n",i,rejected[i]);
}
#else
constexpr bool configured=false;
bool enable(FILE*){return true;}
Prepared prepare(gears_death_source::Args const& args,uintptr_t){Prepared p;p.decision.forwarded=args;return p;}
void complete(Prepared const&){}
void progress(FILE* file){fprintf(file,"death_source_enabled=0 death_source_phase=3 death_source_a1_count=0 death_source_b_count=0 death_source_a2_count=0 ");}
void print(FILE* file){fprintf(file,"death_source_control enabled=0 diagnostic_only=1 product_fix=0\n");}
#endif
unsigned status_interval_ms(){return configured?100u:1000u;}
}
