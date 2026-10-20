#pragma once
#include "ngx_stereo_adapter.h"
#include <utility>

// The complete native wrapper owns the shared parameter object until its tail
// dispatcher returns. This bridge must enclose the ORIGINAL wrapper, not just
// the SDK call. The host supplies authenticated ABI arguments and safe-copied
// structures. No source/renderer mutation or GPU fact is manufactured here.
namespace gears_ngx_stereo {
struct WrapperOutcome {
    uint64_t raw{dispatch_unavailable};
    bool forwarded{},admitted{},dispatcher_seen{};
};
struct EvaluationBinding {
    ProducerObservation producer{};
    Command command{};
    uint64_t public_handle{};
    // Raw pointers whose association with producer.resources was established
    // through typed native resource bindings, ordered Color, Depth, MV, Output.
    std::array<uint64_t,4> raw_resources{};
};
class NativeWrapperBridge {
    StereoAdapter& adapter_;bool enabled_{};Layout layout_{};
    enum class Kind {create,evaluate};
    struct Scope {
        NativeWrapperBridge* bridge{};StereoAdapter::Entry* entry{};Kind kind{};
        Command command{};uint64_t parameters{},public_handle{};uint64_t* public_slot{};
        unsigned dispatches{};bool admitted{};
        Scope* previous{};
        Scope(NativeWrapperBridge& b,StereoAdapter::Entry& e,Kind k,Command c,uint64_t p,
              uint64_t h=0,uint64_t* slot=nullptr):bridge(&b),entry(&e),kind(k),command(c),
              parameters(p),public_handle(h),public_slot(slot),previous(current_){current_=this;}
        ~Scope(){current_=previous;}
        Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
    };
    inline static thread_local Scope* current_{};
    NativeResult reject(Scope* s){
        if(s&&s->bridge==this)adapter_.invalidate(*s->entry);
        else {auto entry=adapter_.enter();if(entry)adapter_.invalidate(entry);}
        return dispatch_unavailable;
    }
    static uint32_t word(EvaluateShadow const& s,size_t offset){uint32_t v{};std::memcpy(&v,s.bytes.data()+offset,sizeof v);return v;}
    static uint64_t pointer(EvaluateShadow const& s,size_t offset){uint64_t v{};std::memcpy(&v,s.bytes.data()+offset,sizeof v);return v;}
public:
    explicit NativeWrapperBridge(StereoAdapter& adapter,bool enabled=false):adapter_(adapter),enabled_(enabled){}
    NativeWrapperBridge(NativeWrapperBridge const&)=delete;
    NativeWrapperBridge& operator=(NativeWrapperBridge const&)=delete;
    bool in_wrapper()const{return current_&&current_->bridge==this;}
    // The disabled path forwards the exact original pointer and uint64 return.
    // When enabled, the host must copy source safely before calling, and pass an
    // authenticated writable handle cell whose lifetime encloses this call.
    template<class Forward> WrapperOutcome create_wrapper(Identity id,Layout const& layout,Command command,
        uint64_t* public_slot,CreateShadow const& source,void const* original_pointer,Forward&& original){
        if(!enabled_)return {original(original_pointer),true,true,false};
        auto entry=adapter_.enter();if(!entry)return {};
        if(current_||!public_slot||*public_slot||!valid(command,id)||!adapter_.begin_epoch(entry,id,layout)){
            adapter_.invalidate(entry);return {};}
        CreateShadow owned;if(!patch_create(source.bytes,owned.bytes,layout)){adapter_.invalidate(entry);return {};}
        layout_=layout;Scope scope(*this,entry,Kind::create,command,id.parameters,0,public_slot);
        uint64_t raw{};
        try{raw=original(owned.bytes.data());}catch(...){adapter_.invalidate(entry);throw;}
        const bool ok=scope.dispatches==1&&scope.admitted&&native_result_accepted(NativeResult(raw))&&
            adapter_.phase(entry)==AcceptancePhase::candidate;
        if(!ok)adapter_.invalidate(entry);
        return {ok||!native_result_accepted(NativeResult(raw))?raw:uint64_t(dispatch_unavailable),true,ok,scope.dispatches!=0};
    }
    template<class Forward> WrapperOutcome evaluate_wrapper(EvaluationBinding const& binding,
        EvaluateShadow const& source,void const* original_pointer,Forward&& original){
        if(!enabled_)return {original(original_pointer),true,true,false};
        auto entry=adapter_.enter();if(!entry)return {};
        auto const& use=binding.producer.use;unsigned eye{};
        std::array<Point,3> bases{{{word(source,0x60),word(source,0x64)},
            {word(source,0x68),word(source,0x6c)},{word(source,0x70),word(source,0x74)}}};
        // Width/height may carry the original family-wide clamp. The actual
        // producer observation supplies the replacement render dimensions.
        bool resources=true;constexpr std::array<size_t,4> offsets{0,0x18,0x20,8};
        for(unsigned i=0;i<4;++i)resources=resources&&binding.raw_resources[i]&&pointer(source,offsets[i])==binding.raw_resources[i];
        if(current_||!resources||!select_input_eye(layout_,layout_.input,bases,eye)||eye!=use.eye||
           !adapter_.observe_producer(entry,binding.producer)){adapter_.invalidate(entry);return {};}
        // First actual Evaluate binds physical resources; Create deliberately
        // had no such arguments. Never patch/call Eval with an unbound layout.
        if(!adapter_.bound_layout(entry,layout_)){adapter_.invalidate(entry);return {};}
        bool reset{};
        if(!adapter_.prepare_evaluate(entry,use,binding.command,binding.public_handle,use.identity.parameters,
                                     word(source,0x38)!=0,reset))return {};
        EvaluateShadow owned;
        if(!patch_evaluate(source.bytes,owned.bytes,layout_,eye,reset)){adapter_.invalidate(entry);return {};}
        Scope scope(*this,entry,Kind::evaluate,binding.command,use.identity.parameters,binding.public_handle);
        uint64_t raw{};
        try{raw=original(owned.bytes.data());}catch(...){adapter_.invalidate(entry);throw;}
        const bool ok=scope.dispatches==1&&scope.admitted&&native_result_accepted(NativeResult(raw))&&
            adapter_.phase(entry)!=AcceptancePhase::quarantined;
        if(!ok)adapter_.invalidate(entry);
        return {ok||!native_result_accepted(NativeResult(raw))?raw:uint64_t(dispatch_unavailable),true,ok,scope.dispatches!=0};
    }
    // Both authenticated wrappers tail-jump. Their return address is the
    // wrapper's caller, not be5c50/be61b4. Match this owning TLS scope and exact
    // arguments, never a fictitious dispatcher return-site whitelist.
    template<class Forward> NativeResult create_dispatch(uint64_t command,uint32_t feature,uint64_t parameters,
        uint64_t* slot,Forward&& original){
        if(!enabled_)return original(command,feature,parameters,slot);
        auto* s=current_;
        if(!s||s->bridge!=this||s->kind!=Kind::create||s->dispatches++||command!=s->command.raw||
           parameters!=s->parameters||slot!=s->public_slot||feature!=1)return reject(s);
        auto result=adapter_.create(*s->entry,s->command,feature,parameters);
        s->admitted=result.admitted;
        if(result.admitted)*slot=result.public_handle;
        // A non-BAD result with a missing/duplicate handle was NOT admitted.
        // Preserve raw SDK failure; return a real native failure for policy
        // rejection instead of exposing a false successful engine handle.
        if(!result.admitted&&(!result.raw_valid||native_result_accepted(result.raw)))return dispatch_unavailable;
        return result.raw;
    }
    template<class Forward> NativeResult evaluate_dispatch(uint64_t command,uint64_t handle,uint64_t parameters,
        uint64_t callback,Forward&& original){
        if(!enabled_)return original(command,handle,parameters,callback);
        auto* s=current_;
        if(!s||s->bridge!=this||s->kind!=Kind::evaluate||s->dispatches++||command!=s->command.raw||
           parameters!=s->parameters||handle!=s->public_handle||callback)return reject(s);
        auto result=adapter_.evaluate(*s->entry,command,handle,parameters,callback);s->admitted=result.admitted;
        if(!result.admitted&&(!result.raw_valid||native_result_accepted(result.raw)))return dispatch_unavailable;
        return result.raw;
    }
    // Called in the authenticated native release context, with an actual fence
    // completion. Never fabricate success for deferred work. The host must keep
    // this owner alive if rejected, even if native code clears its public cell.
    // Foreign handles keep their original release semantics unchanged.
    template<class Forward> NativeResult release_dispatch(uint64_t handle,Completion completed,Forward&& original){
        if(!enabled_)return original(handle);
        auto entry=adapter_.enter();if(!entry)return dispatch_unavailable;
        if(!adapter_.owns(entry,handle))return original(handle);
        adapter_.invalidate(entry);
        if(current_||!adapter_.pair_release_ready(entry,completed))return dispatch_unavailable;
        auto handles=adapter_.owned_handles(entry);NativeResult first{},last{};bool called{};
        for(unsigned i=0;i<2;++i){if(!handles[i]||(i&&handles[i]==handles[0]))continue;
            if(!adapter_.release_completed(entry,handles[i],completed,last))return dispatch_unavailable;
            if(!called)first=last;called=true;
            if(!native_result_accepted(last))return last;
        }
        return called?first:dispatch_unavailable;
    }
};
}
