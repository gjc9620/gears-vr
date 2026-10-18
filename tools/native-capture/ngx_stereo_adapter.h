#pragma once
#include "ngx_stereo_feature_pair.h"
#include <atomic>
#include <cstring>
#include <memory>

// Unwired production adapter. No hook installation, environment switch, global
// handle write, COM cast, barrier, Execute or guessed NGX result code lives here.
// The host holds Entry across wrapper setters AND its SDK tail dispatcher.
namespace gears_ngx_stereo {
// Historical artifact1 native contracts; these constants do not authenticate
// the current process. Current-byte gates remain mandatory before activation.
using NativeResult=uint32_t;
constexpr NativeResult dispatch_unavailable=0xbad00007u;
constexpr NativeResult named_result_success=1u;
// Native callers accept every non-BAD value. This is broader than the named
// NVSDK_NGX_Result_Success value1; preserve unknown accepted raw values.
constexpr bool native_result_accepted(NativeResult raw){return (raw&0xfff00000u)!=0xbad00000u;}
constexpr size_t create_consumed_span=0x19; // Consumer-complete, not sizeof.
constexpr size_t evaluate_initialized_span=0x168;
struct alignas(8) CreateShadow {
    std::array<unsigned char,create_consumed_span> bytes{};
    std::array<unsigned char,7> owner_alignment_padding{}; // Not native fields.
};
struct alignas(8) EvaluateShadow {std::array<unsigned char,evaluate_initialized_span> bytes{};};
struct AdapterPrerequisites {
    bool enabled{}; // Explicit opt-in; default construction never calls SDK.
    bool current_wrappers{},current_dispatchers{},current_release{},current_builder{};
    bool consumed_structure_spans{},result_semantics{},serial_parameter_consumption{};
    // Host-attested capabilities/context, not observations of this ticket.
    // Isolation must prevent candidate output reaching normal publication;
    // execution_dependencies covers actual resource barriers/queue ordering.
    // Release context remains mandatory even for a one-ticket trial.
    bool typed_resource_domains{},release_context{},candidate_isolation{},execution_dependencies{};
    bool ready()const{return enabled&&current_wrappers&&current_dispatchers&&current_release&&current_builder&&
        consumed_structure_spans&&result_semantics&&serial_parameter_consumption&&
        typed_resource_domains&&release_context&&candidate_isolation&&execution_dependencies;}
};
struct Command {
    uint64_t raw{},list{},generation{},device{};bool direct{};
};
inline bool valid(Command c,Identity id){return c.raw&&c.list&&c.generation&&c.device==id.device&&c.direct;}
// Derive the logical eye only from the attested producer's matching Color,
// Depth and MV domains. TLS current-view eye is intentionally not an input.
inline bool select_input_eye(Layout const& l,Extent rendered,std::array<Point,3> const& bases,unsigned& eye){
    if(!valid_create_geometry(l)||!same(rendered,l.input)||!same(bases[0],bases[1])||!same(bases[0],bases[2]))return false;
    for(unsigned i=0;i<2;++i)if(same(bases[0],l.input_base[i])){eye=i;return true;}return false;
}
struct ExecuteReceipt {
    Submission submission{};uint64_t order{};unsigned member{};
};
enum class RecordingKind {creation,evaluate,copy};
struct PendingRecording {RecordingKind kind{};unsigned eye{};uint64_t list{},generation{};};
enum class AcceptancePhase {inactive,candidate,accepted,quarantined};
enum class TransportCompletion {incomplete,pending,accepted,rejected};
enum class TicketAdmission {admitted,hold,rejected};
// The host captures these from actual named Color/Depth/MV producers and typed
// resource bindings BEFORE Eval. Patched Evaluate arguments are not evidence of
// rendering. Order of resources is Color, Depth, MV, Output, matching Layout.
struct ProducerObservation {
    EvalUse use{};uint64_t view{};
    std::array<Resource,4> resources{};
    std::array<Rect,3> rendered{};
};
// Host observation from this eye's GPU-completed output capture at its copy
// boundary, before scratch reuse. SDK nonfailure alone cannot construct it.
// This is retained-resource/produced-region acceptance, not temporal/image QA.
struct RetainedOutputObservation {
    EvalUse use{};uint64_t feature{};Resource output{};Rect produced{};
    Completion completion{};
};
inline bool same_resource(Resource const& a,Resource const& b){
    return a.identity==b.identity&&a.device==b.device&&same(a.extent,b.extent)&&a.format==b.format&&
        a.mip==b.mip&&a.samples==b.samples&&a.subresource==b.subresource;
}
inline bool same_use(EvalUse const& a,EvalUse const& b){return same(a.identity,b.identity)&&
    a.ticket==b.ticket&&a.list==b.list&&a.generation==b.generation&&a.eye==b.eye;}
// These callbacks are the already resolved, unfiltered originals. Host maps
// copy() to the original typed CopyTextureRegion arguments plus this one box.
struct AdapterBackend {
    virtual ~AdapterBackend()=default;
    virtual NativeResult create(uint64_t command,uint32_t feature,uint64_t parameters,uint64_t* handle)=0;
    // Dispatcher7c8440 forwards R9 as a callback through helper7c7bd0.
    // The admitted be5c58 wrapper explicitly supplies zero at be6188.
    virtual NativeResult evaluate(uint64_t command,uint64_t handle,uint64_t parameters,uint64_t callback)=0;
    virtual NativeResult release(uint64_t handle)=0;
    virtual void copy(CopyPlan const&)=0;
};
struct [[nodiscard]] DispatchResult {
    // SDK call admission only. A candidate handle is usable for its bounded
    // trial while all normal output-publication receipts remain unavailable.
    bool admitted{},raw_valid{};NativeResult raw{};uint64_t public_handle{};
};
struct RecordedOutput {
    Identity identity{};uint64_t ticket{},color{};
    Layout layout{};std::array<CopyPlan,2> copies{};
};
// Consumes an adapter receipt after both boxed originals returned. This is a
// geometry proof only: the host must separately join the actual final Color SRV
// and draw/list generation, and reject publication on failed execution proof.
inline bool output_geometry(RecordedOutput const& r,Identity id,uint64_t ticket,uint64_t color,
    unsigned eye,Rect rendered,Extent display,Rect& source,Rect& destination){
    if(!valid(r.layout)||!same(r.identity,id)||r.ticket!=ticket||!ticket||!color||r.color!=color||eye>1||
        !bounded(display)||display.width%2)return false;
    auto const& l=r.layout;auto p=l.input_base[eye];
    if(rendered.x!=p.x||rendered.y!=p.y||rendered.width!=l.input.width||rendered.height!=l.input.height)return false;
    for(unsigned i=0;i<2;++i){auto const& c=r.copies[i];
        if(c.ticket!=ticket||c.ordinal!=i||c.target!=color||!c.source||c.source==color||!c.list||!c.generation||
            c.source_box!=std::array<uint32_t,6>{0,0,0,l.eye_output.width,l.eye_output.height,1}||
            c.destination!=std::array<uint32_t,3>{i*l.stride,0,0})return false;
    }
    if(r.copies[0].source!=r.copies[1].source)return false;
    source={eye*l.stride,0,l.eye_output.width,l.eye_output.height};
    destination={eye*(display.width/2),0,display.width/2,display.height};return true;
}
// Create's authenticated complete wrapper consumes only six u32 and byte+18;
// its structure pointer does not escape. 0x19 is a consumer-complete shadow,
// not an inferred native sizeof. Evaluate requires its FULL caller-initialized
// 0x168 span: be65cc initializes it, and wrapper be5c58 reads through +160.
// Scalar patches preserve every other byte, including opaque fields after90.
template<size_t N> inline bool patch_create(std::array<unsigned char,N> const& original,
    std::array<unsigned char,N>& owned,Layout const& l){
    static_assert(N>=create_consumed_span,"consumer-complete Create shadow required");
    if(!valid_create_layout(l))return false;owned=original;
    uint32_t dimensions[]{l.input.width,l.input.height,l.eye_output.width,l.eye_output.height};
    std::memcpy(owned.data(),dimensions,sizeof dimensions);return true;
}
template<size_t N> inline bool patch_evaluate(std::array<unsigned char,N> const& original,
    std::array<unsigned char,N>& owned,Layout const& l,unsigned eye,bool reset){
    static_assert(N>=evaluate_initialized_span,"full 0x168 initialized Evaluate span required");
    if(!valid(l)||eye>1)return false;owned=original;
    auto put=[&](size_t offset,uint32_t v){std::memcpy(owned.data()+offset,&v,sizeof v);};
    put(0x30,l.input.width);put(0x34,l.input.height);put(0x38,reset?1u:0u);
    for(size_t offset:{size_t(0x60),size_t(0x68),size_t(0x70)}){
        put(offset,l.input_base[eye].x);put(offset+4,l.input_base[eye].y);}
    put(0x88,0);put(0x8c,0);return true;
}
class StereoAdapter {
    AdapterBackend& backend_;AdapterPrerequisites prerequisites_;
    std::atomic_flag gate_=ATOMIC_FLAG_INIT;std::atomic<bool> poisoned_{false};
    std::unique_ptr<FeaturePair> pair_;std::unique_ptr<CopySequence> sequence_;
    Layout layout_{};bool resources_bound_{};std::array<Resource,4> epoch_resources_{};
    Command creation_command_{};bool creation_recorded_{},creation_submitted_{},creation_retired_{},release_uncertain_{};
    ExecuteReceipt creation_execution_{},last_execution_{};
    std::array<EvalUse,2> uses_{};std::array<CopyPlan,2> copies_{};
    std::array<bool,2> recorded_{},evaluated_{},copied_{},submitted_{},history_reset_{true,true};
    AcceptancePhase phase_{AcceptancePhase::inactive};uint64_t candidate_ticket_{},producer_ticket_{};
    std::array<ProducerObservation,2> producers_{};std::array<bool,2> producer_observed_{};
    std::array<Resource,4> accepted_resources_{};
    std::array<ExecuteReceipt,2> executions_{};
    std::array<ExecuteReceipt,2> copy_executions_{};
    std::array<bool,2> copy_recorded_{},copy_submitted_{};
    EvalUse pending_{};Command pending_command_{};bool prepared_{},dispatched_{},reset_{};
    uint64_t selected_handle_{},ticket_{},reset_ticket_{};unsigned next_copy_{};
    void fail(){if(pair_){pair_->invalidate();phase_=AcceptancePhase::quarantined;}}
    bool stream_valid(ExecuteReceipt const& e)const{
        if(!pair_)return false;auto const& s=e.submission;auto const& p=last_execution_.submission;
        if(!e.order||!s.direct||s.device!=pair_->identity().device||!s.list||!s.generation||!s.queue||!s.fence||!s.value||s.value==UINT64_MAX)return false;
        if(!last_execution_.order)return true;
        if(s.queue!=p.queue||s.fence!=p.fence||s.value<p.value||e.order<last_execution_.order)return false;
        if(e.order==last_execution_.order){
            if(e.member<last_execution_.member)return false;
            if(e.member==last_execution_.member&&(s.list!=p.list||s.generation!=p.generation||s.value!=p.value))return false;
        }
        return true;
    }
public:
    class Entry {
        friend class StereoAdapter;StereoAdapter* owner_{};bool held_{};
        explicit Entry(StereoAdapter& a):owner_(&a),held_(!a.gate_.test_and_set(std::memory_order_acquire)){
            if(!held_)a.poisoned_.store(true,std::memory_order_release);
        }
    public:
        Entry(Entry const&)=delete;Entry& operator=(Entry const&)=delete;
        ~Entry(){if(held_){const bool poisoned=owner_->poisoned_.exchange(false,std::memory_order_acq_rel);
            if(owner_->prepared_||poisoned)owner_->fail();
            owner_->gate_.clear(std::memory_order_release);}}
        explicit operator bool()const{return held_;}
    };
private:
    bool allowed(Entry const& e){
        if(e.owner_!=this||!e.held_)return false;
        if(poisoned_.load(std::memory_order_acquire)){fail();return false;}return true;
    }
public:
    explicit StereoAdapter(AdapterBackend& backend,AdapterPrerequisites p={}):backend_(backend),prerequisites_(p){}
    StereoAdapter(StereoAdapter const&)=delete;StereoAdapter& operator=(StereoAdapter const&)=delete;
    Entry enter(){return Entry(*this);}
    AcceptancePhase phase(Entry const& e){return allowed(e)?phase_:AcceptancePhase::quarantined;}
    // Restart requires all old owned handles released. Destruction never calls
    // vendor Release: host must retain/quarantine this owner until drained.
    bool begin_epoch(Entry const& e,Identity id,Layout layout){
        if(!allowed(e)||!prerequisites_.ready()||!valid(id)||!valid_create_layout(layout)||release_uncertain_||
           (creation_recorded_&&!creation_retired_)||(pair_&&!pair_->drained()))return false;
        pair_=std::make_unique<FeaturePair>(id,layout);sequence_.reset();creation_command_={};
        layout_=layout;resources_bound_=false;epoch_resources_={};
        creation_recorded_=creation_submitted_=creation_retired_=false;creation_execution_=last_execution_={};
        uses_={};copies_={};recorded_={};evaluated_={};copied_={};submitted_={};history_reset_={true,true};
        phase_=AcceptancePhase::candidate;candidate_ticket_=producer_ticket_=0;producers_={};producer_observed_={};accepted_resources_={};executions_={};
        copy_executions_={};copy_recorded_={};copy_submitted_={};
        prepared_=dispatched_=false;ticket_=reset_ticket_=0;next_copy_=0;return true;
    }
    // Bind one actual candidate ticket. There is no all-ready shortcut to the
    // accepted phase, and another ticket cannot run until this one is accepted.
    // After acceptance every new ticket still needs fresh producer observations.
    bool observe_producer(Entry const& e,ProducerObservation const& observed){
        auto const& use=observed.use;
        if(!allowed(e)||!pair_||!pair_->usable()||prepared_||!same(use.identity,pair_->identity())||
           !use.ticket||!use.list||!use.generation||use.eye>1||!observed.view){fail();return false;}
        auto l=layout_;unsigned eye{};std::array<Point,3> bases{};
        if(!resources_bound_)for(unsigned i=0;i<4;++i){
            if(!l.physical[i].width&&!l.physical[i].height)l.physical[i]=observed.resources[i].extent;
        }
        if(!valid(l)){fail();return false;}
        for(unsigned i=0;i<3;++i){auto const& r=observed.rendered[i];bases[i]={r.x,r.y};
            if(!same(Extent{r.width,r.height},l.input)){fail();return false;}}
        if(!select_input_eye(l,l.input,bases,eye)||eye!=use.eye){fail();return false;}
        for(unsigned i=0;i<4;++i){auto const& r=observed.resources[i];
            if(!r.identity||r.device!=use.identity.device||!same(r.extent,l.physical[i])||!r.format||
               ((i==0||i==3)&&r.format!=26)||r.mip||r.samples!=1||r.subresource){fail();return false;}
            for(unsigned j=0;j<i;++j)if(r.identity==observed.resources[j].identity){fail();return false;}}
        if(resources_bound_)for(unsigned i=0;i<4;++i)
            if(!same_resource(observed.resources[i],epoch_resources_[i])){fail();return false;}
        if(phase_==AcceptancePhase::candidate&&candidate_ticket_&&candidate_ticket_!=use.ticket){fail();return false;}
        if(!eye){
            if(producer_ticket_==use.ticket||(ticket_&&(use.ticket<=ticket_||!copied_[0]||!copied_[1]||!submitted_[0]||!submitted_[1]||!copy_submitted_[0]||!copy_submitted_[1]))){fail();return false;}
            // Acceptance is for these retained resources. Replacement needs a
            // fresh epoch/trial even when the new allocations have equal size.
            if(phase_==AcceptancePhase::accepted)for(unsigned i=0;i<4;++i)
                if(!same_resource(observed.resources[i],accepted_resources_[i])){fail();return false;}
            producer_ticket_=use.ticket;producer_observed_={};producers_={};
            if(phase_==AcceptancePhase::candidate)candidate_ticket_=use.ticket;
        }else if(producer_ticket_!=use.ticket||!producer_observed_[0]||producer_observed_[1]||
            ticket_!=use.ticket||!copied_[0]||observed.view==producers_[0].view){fail();return false;}
        if(eye)for(unsigned i=0;i<4;++i)if(!same_resource(observed.resources[i],producers_[0].resources[i])){fail();return false;}
        // Bind only after the full first-eye identity/domain/ticket checks.
        // Create dimensions remain immutable; physical identity is fixed for
        // this epoch even before the candidate reaches acceptance.
        if(!resources_bound_){if(eye){fail();return false;}layout_=l;epoch_resources_=observed.resources;resources_bound_=true;}
        producers_[eye]=observed;producer_observed_[eye]=true;return true;
    }
    bool bound_layout(Entry const& e,Layout& out){out={};if(!allowed(e)||!resources_bound_)return false;out=layout_;return true;}
    // Called at Create SDK dispatcher after the owned wrapper set eye-sized
    // parameters. The engine handle slot is written by host ONLY if admitted.
    DispatchResult create(Entry const& e,Command command,uint32_t feature,uint64_t parameters){
        if(!allowed(e)||!pair_||pair_->retired()||phase_!=AcceptancePhase::candidate||creation_recorded_||!valid(command,pair_->identity())||!feature||parameters!=pair_->identity().parameters){fail();return {};}
        creation_command_=command;creation_recorded_=true;DispatchResult result;
        for(unsigned eye=0;eye<2;++eye){uint64_t handle{};NativeResult raw{};
            try{raw=backend_.create(command.raw,feature,parameters,&handle);}
            catch(...){(void)pair_->created(eye,handle,false);fail();throw;}
            result.raw_valid=true;result.raw=raw;
            const auto ownership=pair_->created(eye,handle,native_result_accepted(raw));
            // Ordered local output slots make caller_quarantine unreachable.
            // A failed Create can still own a nonzero handle and GPU commands.
            if(!ownership||!allowed(e)){fail();return result;}
        }
        if(!pair_->commit()){fail();return result;}
        result.admitted=true;result.public_handle=pair_->state(0).handle;return result;
    }
    // Host uses reset_out to patch its owned Evaluate structure, retaining
    // native Reset (camera cut) as well as each independent history's first use.
    bool prepare_evaluate(Entry const& e,EvalUse use,Command command,uint64_t public_handle,
        uint64_t parameters,bool native_reset,bool& reset_out){
        if(!allowed(e)||!pair_||!pair_->usable()||prepared_||!valid(command,pair_->identity())||
            command.list!=use.list||command.generation!=use.generation||parameters!=pair_->identity().parameters||
            public_handle!=pair_->state(0).handle||!same(use.identity,pair_->identity())||!use.ticket||use.eye>1||
            !producer_observed_[use.eye]||!same_use(producers_[use.eye].use,use)){fail();return false;}
        if(use.eye==0){
            if(ticket_&&(use.ticket<=ticket_||!copied_[0]||!copied_[1]||!submitted_[0]||!submitted_[1]||!copy_submitted_[0]||!copy_submitted_[1])){fail();return false;}
            sequence_=std::make_unique<CopySequence>(layout_,pair_->identity());
            recorded_={};evaluated_={};copied_={};submitted_={};copies_={};executions_={};next_copy_=0;ticket_=use.ticket;
            copy_executions_={};copy_recorded_={};copy_submitted_={};
        }else if(use.eye!=1||!copied_[0]||ticket_!=use.ticket){fail();return false;}
        // Setters are CPU work. Mark the handle GPU-recorded only immediately
        // before entering the original SDK dispatcher, so an abandoned wrapper
        // preparation can still retire its prior actual uses safely.
        reset_=pair_->state(use.eye).reset_pending;
        pending_=use;pending_command_=command;prepared_=true;dispatched_=false;
        if(native_reset&&reset_ticket_!=use.ticket){history_reset_={true,true};reset_ticket_=use.ticket;}
        reset_out=reset_||history_reset_[use.eye];return true;
    }
    DispatchResult evaluate(Entry const& e,uint64_t command,uint64_t engine_handle,uint64_t parameters,uint64_t callback){
        if(!allowed(e)||!pair_||!prepared_||dispatched_||command!=pending_command_.raw||
            engine_handle!=pair_->state(0).handle||parameters!=pair_->identity().parameters||callback!=0){fail();return {};}
        bool feature_reset{};
        if(!pair_->begin_eval(pending_,selected_handle_,feature_reset)||feature_reset!=reset_){fail();prepared_=false;return {};}
        dispatched_=true;DispatchResult result;uses_[pending_.eye]=pending_;recorded_[pending_.eye]=true;
        try{result.raw=backend_.evaluate(command,selected_handle_,parameters,callback);result.raw_valid=true;}
        catch(...){fail();throw;}
        result.admitted=allowed(e)&&pair_->end_eval(pending_,native_result_accepted(result.raw));
        evaluated_[pending_.eye]=result.admitted;if(result.admitted)history_reset_[pending_.eye]=false;prepared_=false;
        if(!result.admitted)fail();return result;
    }
    // One original boxed copy per completed eye. No fallback full copy after
    // a mutation; rejection invalidates this ticket's publication at the host.
    bool copy(Entry const& e,CopyFacts const& facts,unsigned eye){
        if(!allowed(e)||!pair_||!pair_->usable()||prepared_||eye>1||eye!=next_copy_||!evaluated_[eye]||
            facts.ticket!=ticket_||facts.ticket!=uses_[eye].ticket||!sequence_){fail();return false;}
        for(unsigned i=0;i<4;++i)if(!same_resource(facts.resources[i],producers_[eye].resources[i])){fail();return false;}
        CopyPlan plan;if(!sequence_->next(facts,eye,plan)){fail();return false;}
        // Native93/95 Copy uses a DIFFERENT list from its Eval. Retain the
        // actual copy recording even if the original throws after recording.
        copies_[eye]=plan;copy_recorded_[eye]=true;
        try{backend_.copy(plan);}catch(...){fail();throw;}
        if(!allowed(e)){fail();return false;}copies_[eye]=plan;copied_[eye]=true;++next_copy_;return true;
    }
    // Actual SDK/copy calls, including failed calls, own these recording
    // obligations. Never reconstruct them from diagnostic rows or TLS eye.
    unsigned pending_recordings(Entry const& e,std::array<PendingRecording,5>& out){
        out={};if(!allowed(e))return 0;unsigned n{};
        if(creation_recorded_&&!creation_submitted_)out[n++]={RecordingKind::creation,0,creation_command_.list,creation_command_.generation};
        for(unsigned eye=0;eye<2;++eye){
            if(recorded_[eye]&&!submitted_[eye])out[n++]={RecordingKind::evaluate,eye,uses_[eye].list,uses_[eye].generation};
            if(copy_recorded_[eye]&&!copy_submitted_[eye])out[n++]={RecordingKind::copy,eye,copies_[eye].list,copies_[eye].generation};
        }
        return n;
    }
    // Call only AFTER real ExecuteCommandLists and successful Signal covering
    // that batch. Receipt order/member comes from the actual submission hook.
    bool submitted_creation(Entry const& e,ExecuteReceipt receipt){
        auto const& s=receipt.submission;
        if(!allowed(e)||!creation_recorded_||creation_submitted_||s.list!=creation_command_.list||
            s.generation!=creation_command_.generation||!stream_valid(receipt)){fail();return false;}
        creation_execution_=last_execution_=receipt;creation_submitted_=true;return true;
    }
    bool submitted_eye(Entry const& e,unsigned eye,ExecuteReceipt receipt){
        if(!allowed(e)||!pair_||eye>1||!creation_submitted_||!recorded_[eye]||submitted_[eye]||
            (eye==1&&!submitted_[0])||!stream_valid(receipt)||
            !pair_->submitted(pair_->state(eye).handle,uses_[eye],receipt.submission)){fail();return false;}
        submitted_[eye]=true;executions_[eye]=last_execution_=receipt;
        if(copy_recorded_[eye]&&copies_[eye].list==receipt.submission.list&&copies_[eye].generation==receipt.submission.generation){
            copy_submitted_[eye]=true;copy_executions_[eye]=receipt;}
        return true;
    }
    // A distinct Copy list needs its own real Execute+Signal receipt. Legal
    // after invalidation so failed recordings retain a retirement obligation.
    bool submitted_copy(Entry const& e,unsigned eye,ExecuteReceipt receipt){
        auto const& s=receipt.submission;
        if(!allowed(e)||!pair_||eye>1||!copy_recorded_[eye]||copy_submitted_[eye]||!submitted_[eye]||
           s.list!=copies_[eye].list||s.generation!=copies_[eye].generation||!stream_valid(receipt))return false;
        copy_executions_[eye]=last_execution_=receipt;copy_submitted_[eye]=true;return true;
    }
    // Completion of already validated producer/SDK/boxed-Copy recordings.
    // No output readback or pixel/temporal QA is asserted by this transport path.
    // An incomplete or pending candidate remains intact for a later poll.
    TransportCompletion complete_candidate_transport(Entry const& e,Completion c){
        if(!allowed(e)||phase_==AcceptancePhase::quarantined)return TransportCompletion::rejected;
        if(!pair_)return TransportCompletion::incomplete;
        if(!pair_->usable())return TransportCompletion::rejected;
        if(phase_==AcceptancePhase::accepted)return TransportCompletion::accepted;
        if(phase_!=AcceptancePhase::candidate||!candidate_ticket_||ticket_!=candidate_ticket_||
           !creation_submitted_||!sequence_||!sequence_->complete())return TransportCompletion::incomplete;
        for(unsigned eye=0;eye<2;++eye)
            if(!producer_observed_[eye]||!recorded_[eye]||!evaluated_[eye]||!copied_[eye]||!submitted_[eye]||!copy_submitted_[eye])
                return TransportCompletion::incomplete;
        uint64_t required{};
        const Submission obligations[]{creation_execution_.submission,executions_[0].submission,executions_[1].submission,
            copy_executions_[0].submission,copy_executions_[1].submission};
        for(auto const& s:obligations){
            if(c.value==UINT64_MAX||!s.value||c.device!=s.device||c.queue!=s.queue||c.fence!=s.fence){
                fail();return TransportCompletion::rejected;}
            if(s.value>required)required=s.value;
        }
        if(c.value<required)return TransportCompletion::pending;
        // CopySequence certified exact disjoint Color boxes; copied_ records
        // returned originals, and these five receipts cover their real queue work.
        accepted_resources_=producers_[0].resources;phase_=AcceptancePhase::accepted;
        return TransportCompletion::accepted;
    }
    TicketAdmission admit_ticket(Entry const& e,uint64_t world,uint64_t ticket,Completion c){
        if(!allowed(e)||!pair_||!pair_->usable()||world!=pair_->identity().world||!ticket||
           (candidate_ticket_&&ticket<candidate_ticket_)){fail();return TicketAdmission::rejected;}
        const auto state=complete_candidate_transport(e,c);
        if(state==TransportCompletion::rejected)return TicketAdmission::rejected;
        if(state==TransportCompletion::accepted||!candidate_ticket_||ticket==candidate_ticket_)return TicketAdmission::admitted;
        if(state==TransportCompletion::pending)return TicketAdmission::hold;
        // Never wait for recordings which have not actually been submitted.
        fail();return TicketAdmission::rejected;
    }
    bool accept_candidate(Entry const& e,std::array<RetainedOutputObservation,2> const& observed){
        if(!allowed(e)||!pair_||!pair_->usable()||phase_!=AcceptancePhase::candidate||!candidate_ticket_||
           ticket_!=candidate_ticket_||!creation_submitted_||!sequence_||!sequence_->complete()){fail();return false;}
        for(unsigned eye=0;eye<2;++eye){auto const& o=observed[eye];auto const& c=o.completion;
            auto const& submitted=executions_[eye].submission;auto const& created=creation_execution_.submission;
            auto const& copied=copy_executions_[eye].submission;
            auto const extent=layout_.eye_output;
            if(!producer_observed_[eye]||!recorded_[eye]||!evaluated_[eye]||!copied_[eye]||!submitted_[eye]||!copy_submitted_[eye]||
               !same_use(o.use,uses_[eye])||o.feature!=pair_->state(eye).handle||
               !same_resource(o.output,producers_[eye].resources[3])||o.output.identity!=copies_[eye].source||
               o.produced.x||o.produced.y||o.produced.width!=extent.width||o.produced.height!=extent.height||
               c.value==UINT64_MAX||c.device!=submitted.device||c.queue!=submitted.queue||c.fence!=submitted.fence||c.value<submitted.value||
               c.device!=copied.device||c.queue!=copied.queue||c.fence!=copied.fence||c.value<copied.value||
               c.device!=created.device||c.queue!=created.queue||c.fence!=created.fence||c.value<created.value){fail();return false;}
        }
        accepted_resources_=producers_[0].resources;phase_=AcceptancePhase::accepted;return true;
    }
    bool recorded_output(Entry const& e,RecordedOutput& out){
        if(!allowed(e)||!pair_||!pair_->usable()||phase_!=AcceptancePhase::accepted||producer_ticket_!=ticket_||
           !producer_observed_[0]||!producer_observed_[1]||!sequence_||!sequence_->complete()||!copied_[0]||!copied_[1])return false;
        out={pair_->identity(),ticket_,copies_[0].target,layout_,copies_};return true;
    }
    // This establishes actual ordered submissions, not completed pixels or
    // final-SRV association. Host still joins its actual Color/list draw proof.
    bool ordered_output(Entry const& e,RecordedOutput& out){
        return allowed(e)&&submitted_[0]&&submitted_[1]&&copy_submitted_[0]&&copy_submitted_[1]&&recorded_output(e,out);
    }
    void invalidate(Entry const& e){if(allowed(e))fail();}
    // Unknown handles are forwarded unchanged at the original release hook.
    // Owned handles are deferred; a raw SDK success is never fabricated here.
    bool owns(Entry const& e,uint64_t handle){return allowed(e)&&pair_&&pair_->owns(handle);}
    // Read under the same wrapper/dispatcher gate. A snapshot does not transfer
    // ownership; failed partial Creates also remain visible to host retirement.
    std::array<uint64_t,2> owned_handles(Entry const& e){
        if(!allowed(e)||!pair_)return {};return {pair_->state(0).handle,pair_->state(1).handle};
    }
    // Preflight the entire owned pair before the native Release interception
    // consumes either obligation. Completion must cover creation and every
    // recorded use; an unsubmitted list can never be retired by a large value.
    bool pair_release_ready(Entry const& e,Completion c){
        if(!allowed(e)||!pair_||!pair_->retired()||release_uncertain_||!creation_submitted_||
           !complete_creation(e,c))return false;
        for(unsigned eye=0;eye<2;++eye){auto const& h=pair_->state(eye);if(!h.handle)continue;
            if(copy_recorded_[eye]){auto const& s=copy_executions_[eye].submission;
                if(!copy_submitted_[eye]||c.device!=s.device||c.queue!=s.queue||c.fence!=s.fence||c.value<s.value)return false;}
            if(h.lifetime==Lifetime::created_unused)continue;
            auto const& s=h.last_submission;
            if(h.lifetime!=Lifetime::submitted||c.device!=s.device||c.queue!=s.queue||c.fence!=s.fence||c.value<s.value)return false;
        }
        return true;
    }
    // Create may record GPU work even on failure, exception or zero handles.
    // Its retirement is independent of FeaturePair's handle obligations. Only
    // a completed real submission can discharge it; CPU return cannot do so.
    bool complete_creation(Entry const& e,Completion c){
        if(!allowed(e)||!pair_||!creation_recorded_||!creation_submitted_)return false;
        auto const& s=creation_execution_.submission;
        if(c.value==UINT64_MAX||c.device!=s.device||c.queue!=s.queue||c.fence!=s.fence||c.value<s.value)return false;
        creation_retired_=true;return true;
    }
    bool release_completed(Entry const& e,uint64_t handle,Completion c,NativeResult& raw_result){
        if(!allowed(e)||!pair_||!pair_->retired()||!creation_submitted_)return false;
        if(!complete_creation(e,c))return false;
        for(unsigned eye=0;eye<2;++eye)if(copy_recorded_[eye]){auto const& s=copy_executions_[eye].submission;
            if(!copy_submitted_[eye]||c.device!=s.device||c.queue!=s.queue||c.fence!=s.fence||c.value<s.value)return false;}
        uint64_t release{};
        if(!pair_->claim_unused_release(handle,pair_->identity(),release)&&!pair_->claim_completed_release(handle,c,release))return false;
        // Claim before entering SDK; exceptions/uncertain failures aren't retried.
        try{raw_result=backend_.release(release);}
        catch(...){release_uncertain_=true;throw;}
        if(!native_result_accepted(raw_result))release_uncertain_=true;return true;
    }
    bool drained(Entry const& e){return allowed(e)&&pair_&&pair_->drained()&&
        (!creation_recorded_||creation_retired_)&&!release_uncertain_;}
};
}
