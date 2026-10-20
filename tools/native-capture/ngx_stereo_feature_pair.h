#pragma once
#include "ngx_stereo_layout.h"

namespace gears_ngx_stereo {
struct EvalUse {
    Identity identity{};uint64_t ticket{},list{},generation{};unsigned eye{};
};
struct Submission {
    uint64_t device{},list{},generation{},queue{},fence{},value{};bool direct{};
};
struct Completion {uint64_t device{},queue{},fence{},value{};};
// Every post-call result explicitly assigns ownership, even malformed calls.
// caller_quarantine means the adapter retains this unadmitted private handle;
// it must not publish it, forget it, or treat the pair as its release owner.
enum class CreationOwnership {none,pair_retained,pair_already_owns,caller_quarantine};
struct [[nodiscard]] CreationResult {
    bool success{};CreationOwnership ownership{};uint64_t quarantine_handle{};
    explicit operator bool()const{return success;}
};
enum class Lifetime {empty,created_unused,recorded_unsubmitted,submitted,released};
struct HandleState {
    uint64_t handle{};Lifetime lifetime{};bool create_ok{},reset_pending{true};
    EvalUse last_use{};Submission last_submission{};
};
// Serialized adapter-owned state. No global handle swapping, COM ownership or
// vendor calls occur here. A returned release claim must be executed once by the
// adapter; uncertain vendor Release outcomes must never be retried blindly.
class FeaturePair {
    const Identity identity_;const Layout layout_;
    std::array<HandleState,2> handles_{};
    unsigned creates_{},next_eye_{};uint64_t ticket_{},completed_ticket_{};
    bool retired_{},committed_{},creation_failed_{};
    void fail(){retired_=true;committed_=false;}
    bool owned_index(uint64_t handle,unsigned& index)const{
        if(!handle)return false;
        for(unsigned i=0;i<2;++i)if(handles_[i].handle==handle){index=i;return true;}return false;
    }
public:
    FeaturePair(Identity id,Layout l):identity_(id),layout_(l){if(!valid(id)||!valid_create_layout(l))fail();}
    FeaturePair(FeaturePair const&)=delete;
    FeaturePair& operator=(FeaturePair const&)=delete;
    Identity identity()const{return identity_;}
    Layout const& layout()const{return layout_;}
    HandleState const& state(unsigned eye)const{return handles_.at(eye);}
    bool retired()const{return retired_;}
    bool usable()const{return committed_&&!retired_;}
    // Ordered attempts retain nonzero handles, including failed vendor results.
    // Malformed post-call results return an explicit ownership disposition.
    CreationResult created(unsigned eye,uint64_t handle,bool vendor_success){
        if(eye>1||eye!=creates_||creates_>=2||committed_){
            fail();unsigned index{};
            if(!handle)return {false,CreationOwnership::none,0};
            if(owned_index(handle,index)&&handles_[index].lifetime!=Lifetime::released)return {false,CreationOwnership::pair_already_owns,0};
            return {false,CreationOwnership::caller_quarantine,handle};
        }
        auto& h=handles_[eye];h.handle=handle;h.create_ok=vendor_success&&handle;
        h.lifetime=handle?Lifetime::created_unused:Lifetime::empty;++creates_;
        if(!h.create_ok||retired_||(eye&&handle==handles_[0].handle))creation_failed_=true;
        if(creation_failed_)fail();
        return {!creation_failed_,handle?CreationOwnership::pair_retained:CreationOwnership::none,0};
    }
    bool commit(){
        if(retired_||creation_failed_||creates_!=2||!handles_[0].create_ok||!handles_[1].create_ok||handles_[0].handle==handles_[1].handle){fail();return false;}
        committed_=true;return true;
    }
    // Mark recording BEFORE entering vendor Eval, even if Eval later fails.
    // CPU return does not alter lifetime or establish GPU completion.
    bool begin_eval(EvalUse use,uint64_t& handle,bool& reset){
        if(!usable()||!same(use.identity,identity_)||!use.ticket||!use.list||!use.generation||use.eye>1||use.eye!=next_eye_){fail();return false;}
        if((next_eye_==0&&use.ticket<=completed_ticket_)||(next_eye_==1&&use.ticket!=ticket_)){fail();return false;}
        auto& h=handles_[use.eye];
        if(h.lifetime==Lifetime::recorded_unsubmitted||h.lifetime==Lifetime::released){fail();return false;}
        if(!next_eye_)ticket_=use.ticket;
        h.last_use=use;h.lifetime=Lifetime::recorded_unsubmitted;
        handle=h.handle;reset=h.reset_pending;return true;
    }
    // Successful CPU return advances eye order only. Failure retires the pair;
    // its recorded work still needs submission evidence before GPU retirement.
    // Each eye may use its own list/generation. This policy does not prove
    // queue execution order or authorize final-output publication.
    bool end_eval(EvalUse use,bool vendor_success){
        if(use.eye>1||!same(use.identity,identity_)){fail();return false;}
        auto& h=handles_[use.eye];auto const& last=h.last_use;
        if(!usable()||use.eye!=next_eye_||h.lifetime!=Lifetime::recorded_unsubmitted||use.ticket!=last.ticket||use.list!=last.list||use.generation!=last.generation){fail();return false;}
        if(!vendor_success){fail();return false;}
        h.reset_pending=false;
        if(++next_eye_==2){completed_ticket_=ticket_;next_eye_=0;}return true;
    }
    // Legal after invalidation: this is how already-recorded uses acquire an
    // actual queue/fence retirement condition. A changed queue/fence quarantines.
    bool submitted(uint64_t handle,EvalUse use,Submission s){
        unsigned i{};if(!owned_index(handle,i))return false;auto& h=handles_[i];auto const& last=h.last_use;
        if(h.lifetime!=Lifetime::recorded_unsubmitted||i!=use.eye||!same(use.identity,identity_)||use.ticket!=last.ticket||use.list!=last.list||use.generation!=last.generation||
           !s.direct||s.device!=identity_.device||s.list!=last.list||s.generation!=last.generation||!s.queue||!s.fence||!s.value||s.value==UINT64_MAX)return false;
        auto const& previous=h.last_submission;
        if(previous.value&&(s.queue!=previous.queue||s.fence!=previous.fence||s.value<=previous.value))return false;
        h.last_submission=s;h.lifetime=Lifetime::submitted;return true;
    }
    void invalidate(){fail();}
    bool owns(uint64_t handle)const{unsigned i{};return owned_index(handle,i);}
    // Never-used cleanup is deliberately a different operation from completed
    // GPU work. Duplicate Create handles represent one owned release obligation.
    bool claim_unused_release(uint64_t handle,Identity owner,uint64_t& release){
        unsigned i{};if(!retired_||!same(owner,identity_)||!owned_index(handle,i))return false;
        if(handles_[i].lifetime!=Lifetime::created_unused)return false;
        for(auto const& h:handles_)if(h.handle==handle&&h.lifetime!=Lifetime::created_unused)return false;
        for(auto& h:handles_)if(h.handle==handle)h.lifetime=Lifetime::released;
        release=handle;return true;
    }
    bool claim_completed_release(uint64_t handle,Completion c,uint64_t& release){
        unsigned i{};if(!retired_||!owned_index(handle,i))return false;auto& h=handles_[i];auto const& s=h.last_submission;
        if(h.lifetime!=Lifetime::submitted||c.value==UINT64_MAX||c.device!=identity_.device||!c.queue||c.queue!=s.queue||!c.fence||c.fence!=s.fence||c.value<s.value)return false;
        h.lifetime=Lifetime::released;release=handle;return true;
    }
    bool drained()const{
        for(auto const& h:handles_)if(h.handle&&h.lifetime!=Lifetime::released)return false;return retired_;
    }
};
}
