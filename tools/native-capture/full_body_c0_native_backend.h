#pragma once
#include "full_body_c0_transaction.h"
#include "cover_pose_phase.h"
#include "native_atom_overwrite_receipt.h"
#include <windows.h>
#include <intrin.h>

// Actual in-process reads and atomic writes. Context authenticates the EXISTING
// C0 owner/latch/caller; it does not assert a whole-program reader/physics fence.
namespace gears_full_body_c0_native {
namespace tx=gears_full_body_c0_transaction;namespace body=gears_full_body_c0;
namespace pose=gears_cover_pose_phase;
class Context {
public:
    virtual ~Context()=default;
    virtual bool current(tx::Stage) noexcept=0;
    // Only the exact last failed current() may classify a pure source deadline.
    virtual tx::Verdict rejection() const noexcept{return tx::Verdict::denied;}
};
inline bool read_memory(void* out,std::uintptr_t address,std::size_t size) noexcept {
    if(!pose::span(address,size)||!out)return false;
    __try{std::memcpy(out,reinterpret_cast<void const*>(address),size);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline tx::Cas atomic_exchange(std::uintptr_t address,tx::Block const& expected,tx::Block const& replacement) noexcept {
    if(!address||(address&15))return tx::Cas::fault_unknown;
    alignas(16) __int64 comparand[2];std::memcpy(comparand,&expected,16);
    __try{return _InterlockedCompareExchange128(reinterpret_cast<volatile __int64*>(address),
        static_cast<__int64>(replacement.high),static_cast<__int64>(replacement.low),comparand)?tx::Cas::exchanged:tx::Cas::mismatch;}
    __except(EXCEPTION_EXECUTE_HANDLER){return tx::Cas::fault_unknown;}
}
// Diagnostic output only; no extra Backend members or game reads.
enum class CaptureReject:std::uint64_t {captured,stopped,already_captured,seed_owner,seed_model,
    seed_metadata_generation,seed_frame,seed_producer,seed_scope,owner_session,owner_world,
    initial_current,metadata_read,metadata_shape,reference_read,local_read,desired_read,
    desired_index_range,desired_index_duplicate,immutable_readback,repeated_local_read,
    local_changed,final_metadata_read,metadata_changed,final_current};
struct CaptureObservation {
    std::uint64_t reason{UINT64_MAX},context_verdict{UINT64_MAX};
};
class Backend final:public tx::Backend {
    gears_atom_overwrite::ReceiptSource* receipts_{};
    Context* context_{};pose::Request request_{};pose::Metadata metadata_{};tx::Stamp stamp_{};
    body::Frame source_{};tx::Desired desired_{};DWORD thread_{};bool captured_{};std::atomic<bool> stopped_{};
    struct Permit {tx::Block expected{},replacement{};std::uintptr_t address{};tx::Stage stage{};bool armed{};unsigned char reserved[3]{};} permit_{};
    // Large readback storage belongs to the backend, never to the game stack.
    body::Frame repeated_{};std::array<unsigned char,115> desired_repeat_{};
    bool read_metadata(pose::Metadata& m) noexcept {
        auto reader=[](void* o,void const* p,std::size_t n){return read_memory(o,reinterpret_cast<std::uintptr_t>(p),n);};
        return pose::metadata(reader,request_,m);
    }
    bool shape(pose::Metadata const& m) const noexcept {
        if(m.master||m.reference.count!=115||m.local.count!=115||m.desired.count!=115||m.space.count!=115||
           !pose::header_valid(m.reference,68)||!pose::header_valid(m.local,32)||!pose::header_valid(m.desired,1)||!pose::header_valid(m.space,64)||
           (m.local.data&15))return false;
        const body::d::Range ranges[]={{m.local.data,sizeof source_.local},{m.reference.data,sizeof source_.reference},
            {m.desired.data,115},{m.space.data,115*64},body::d::range(this,sizeof *this),
            {request_.owner.mesh.address+0x27c,16},{request_.owner.mesh.address+0x28c,16},{m.asset+0xa4,16}};
        return body::d::disjoint(ranges,8);
    }
    bool source_equal(body::Frame const& a) const noexcept {
        return a.count==source_.count&&a.model_scale==source_.model_scale&&a.units_per_metre==source_.units_per_metre&&
            !std::memcmp(a.reference.data(),source_.reference.data(),sizeof a.reference)&&
            !std::memcmp(a.local.data(),source_.local.data(),sizeof a.local)&&
            !std::memcmp(&a.component_to_world,&source_.component_to_world,sizeof a.component_to_world);
    }
    bool immutable_readback() noexcept {
        return read_memory(repeated_.reference.data(),metadata_.reference.data,sizeof repeated_.reference)&&
            !std::memcmp(repeated_.reference.data(),source_.reference.data(),sizeof repeated_.reference)&&
            read_memory(desired_repeat_.data(),metadata_.desired.data,115)&&desired_repeat_==desired_.indices;
    }
public:
    Backend()=default;Backend(Backend const&)=delete;Backend& operator=(Backend const&)=delete;
    std::uint64_t thread_token() const noexcept override{return GetCurrentThreadId();}
    tx::Stamp const& stamp() const noexcept{return stamp_;}
    pose::Metadata const& metadata() const noexcept{return metadata_;}
    void request_stop() noexcept {stopped_=true;}
    void invalidate() noexcept {permit_.armed=false;context_=nullptr;}
    // Seed IDs are registrations supplied only by the actual C0 adapter; Local
    // binding/header values are read here, never accepted from the seed.
    bool capture(Context& context,pose::OwnerKey const& owner,tx::Stamp seed,body::Frame& out,tx::Desired& desired,
                 CaptureObservation* observation=nullptr) noexcept {
        if(observation)*observation={};
        const auto reject=[&](CaptureReject reason) noexcept {
            if(observation)observation->reason=std::uint64_t(reason);return true;
        };
        const auto reject_current=[&](CaptureReject reason) noexcept {
            if(observation){observation->reason=std::uint64_t(reason);
                observation->context_verdict=std::uint64_t(context.rejection());}return true;
        };
        if((stopped_&&reject(CaptureReject::stopped))||
           (captured_&&reject(CaptureReject::already_captured))||
           (!seed.binding.owner&&reject(CaptureReject::seed_owner))||
           (!seed.binding.model&&reject(CaptureReject::seed_model))||
           (!seed.binding.metadata_generation&&reject(CaptureReject::seed_metadata_generation))||
           (!seed.frame&&reject(CaptureReject::seed_frame))||
           (!seed.producer&&reject(CaptureReject::seed_producer))||
           (!seed.scope&&reject(CaptureReject::seed_scope))||
           (owner.session!=seed.binding.session&&reject(CaptureReject::owner_session))||
           (owner.world_generation!=seed.binding.world_generation&&reject(CaptureReject::owner_world))||
           (!context.current(tx::Stage::prepare)&&reject_current(CaptureReject::initial_current)))return false;
        request_={};request_.owner=owner;request_.kind=pose::Kind::compose;request_.phase=pose::Phase::C0;
        request_.invocation=seed.scope;request_.thread=GetCurrentThreadId();request_.caller_rva=pose::compose_caller;
        context_=&context;thread_=GetCurrentThreadId();pose::Metadata after{};
        if((!read_metadata(metadata_)&&reject(CaptureReject::metadata_read))||
           (!shape(metadata_)&&reject(CaptureReject::metadata_shape)))return false;
        source_={};source_.count=115;source_.model_scale=1;source_.units_per_metre=100;
        std::memcpy(&source_.component_to_world,&metadata_.component,64);
        desired_={};desired_.count=115;
        if((!read_memory(source_.reference.data(),metadata_.reference.data,sizeof source_.reference)&&reject(CaptureReject::reference_read))||
           (!read_memory(source_.local.data(),metadata_.local.data,sizeof source_.local)&&reject(CaptureReject::local_read))||
           (!read_memory(desired_.indices.data(),metadata_.desired.data,115)&&reject(CaptureReject::desired_read)))return false;
        std::array<bool,115> seen{};for(auto index:desired_.indices){
            if((index>=115&&reject(CaptureReject::desired_index_range))||
               (seen[index]&&reject(CaptureReject::desired_index_duplicate)))return false;seen[index]=true;}
        if((!immutable_readback()&&reject(CaptureReject::immutable_readback))||
           (!read_memory(repeated_.local.data(),metadata_.local.data,sizeof repeated_.local)&&reject(CaptureReject::repeated_local_read))||
           (std::memcmp(repeated_.local.data(),source_.local.data(),sizeof source_.local)&&reject(CaptureReject::local_changed))||
           (!read_metadata(after)&&reject(CaptureReject::final_metadata_read))||
           (!pose::metadata_equal(after,metadata_)&&reject(CaptureReject::metadata_changed))||
           (!context.current(tx::Stage::prepare)&&reject_current(CaptureReject::final_current)))return false;
        seed.binding.local=metadata_.local.data;seed.binding.count=115;seed.binding.capacity=unsigned(metadata_.local.capacity);
        stamp_=seed;out=source_;desired=desired_;captured_=true;
        if(observation)observation->reason=std::uint64_t(CaptureReject::captured);return true;
    }
    tx::Verdict validate(tx::View const& v,tx::Stage stage,unsigned unit) noexcept override {
        permit_.armed=false;
        if(!captured_||!context_||GetCurrentThreadId()!=thread_||!tx::same(v.stamp,stamp_)||
           !source_equal(v.source)||v.desired.count!=115||v.desired.indices!=desired_.indices||v.count>tx::max_units||
           ((stage==tx::Stage::prepare||stage==tx::Stage::commit)&&stopped_))return tx::Verdict::denied;
        if(stage==tx::Stage::commit&&(!metadata_.pose_epoch||metadata_.pose_epoch==UINT32_MAX))return tx::Verdict::denied;
        if((stage==tx::Stage::commit||stage==tx::Stage::rollback)&&unit>=v.count)return tx::Verdict::denied;
        pose::Metadata now{},again{};
        if(!read_metadata(now)||!pose::metadata_equal(now,metadata_)||!immutable_readback())return tx::Verdict::denied;
        // Original Compose may legitimately mutate Local. Scope-end authenticates
        // the same owner/header/metadata, but can never arm a memory-write permit.
        if(stage!=tx::Stage::scope_end){
            if(!read_memory(repeated_.local.data(),metadata_.local.data,sizeof repeated_.local))return tx::Verdict::fault;
            for(unsigned bone=0;bone<115;++bone)for(unsigned half=0;half<2;++half){
                auto const* expected=reinterpret_cast<unsigned char const*>(&source_.local[bone])+half*16;
                bool owned=false,skip=false;
                for(unsigned i=0;i<v.count;++i){auto const& u=v.units[i];
                    if(u.bone>=115||u.half>1||(u.half&&u.bone))return tx::Verdict::denied;
                    if(u.bone!=bone||u.half!=half)continue;
                    if(owned)return tx::Verdict::denied;owned=true;
                    const auto* planned=reinterpret_cast<unsigned char const*>(&v.plan.local[bone])+half*16;
                    if(std::memcmp(&u.before,expected,16)||std::memcmp(&u.ours,planned,16))return tx::Verdict::denied;
                    // Reverse recovery examines each owned value with CAS itself;
                    // all NON-owned fields must retain their original bytes.
                    if(stage==tx::Stage::rollback)skip=true;
                    else if(u.state==tx::UnitState::applied)expected=reinterpret_cast<unsigned char const*>(&u.ours);
                    else if(u.state==tx::UnitState::uncertain||u.state==tx::UnitState::conflict)return tx::Verdict::denied;
                }
                if(!skip&&std::memcmp(reinterpret_cast<unsigned char const*>(&repeated_.local[bone])+half*16,expected,16))return tx::Verdict::denied;
            }
        }
        if(!immutable_readback()||!read_metadata(again)||!pose::metadata_equal(now,again))return tx::Verdict::denied;
        // Readback cannot grant permission. Check the complete current context
        // once at the final boundary, after every non-age readback check. An
        // age failure burns its source latch; do not call it again and erase
        // the exact reason with the now-invalid latch's generic rejection.
        if(!context_->current(stage))return context_->rejection();
        if(stage==tx::Stage::commit||stage==tx::Stage::rollback){
            auto const& u=v.units[unit];
            permit_.address=metadata_.local.data+std::uintptr_t(u.bone)*32+u.half*16;permit_.stage=stage;
            permit_.expected=stage==tx::Stage::commit?u.before:u.ours;permit_.replacement=stage==tx::Stage::commit?u.ours:u.before;permit_.armed=true;
        }
        return tx::Verdict::verified;
    }
    tx::Cas exchange(std::uintptr_t address,tx::Block const& expected,tx::Block const& replacement) noexcept override {
        const auto ticket=permit_;permit_.armed=false;
        if(!ticket.armed||address!=ticket.address||std::memcmp(&expected,&ticket.expected,16)||std::memcmp(&replacement,&ticket.replacement,16)||
           !context_||GetCurrentThreadId()!=thread_||(ticket.stage==tx::Stage::commit&&stopped_))return tx::Cas::fault_unknown;
        if(!context_->current(ticket.stage))return ticket.stage==tx::Stage::commit&&context_->rejection()==tx::Verdict::source_age_expired?
            tx::Cas::source_age_no_write:tx::Cas::fault_unknown;
        return atomic_exchange(address,expected,replacement);
    }
    void receipt_source(gears_atom_overwrite::ReceiptSource* p)noexcept{receipts_=p;}
    std::uint32_t installation_epoch()const noexcept{return metadata_.pose_epoch;}
    bool retirement_metadata(pose::Metadata& now)noexcept {
        if(!captured_||GetCurrentThreadId()!=thread_||!read_metadata(now))return false;
        auto storage_same=[&](pose::Metadata const&x){return x.asset==metadata_.asset&&x.master==metadata_.master&&
            x.physics_asset==metadata_.physics_asset&&x.physics_instance==metadata_.physics_instance&&
            pose::header_equal(x.reference,metadata_.reference)&&pose::header_equal(x.local,metadata_.local)&&
            pose::header_equal(x.desired,metadata_.desired)&&pose::header_equal(x.space,metadata_.space);};
        pose::Metadata again{};return storage_same(now)&&immutable_readback()&&read_metadata(again)&&
            pose::metadata_equal(now,again)&&storage_same(again)&&immutable_readback();
    }
    tx::Verdict fresh(tx::View const&v,tx::ReceiptRef r,tx::FreshEvidence&e) noexcept override {
        if(!captured_||!receipts_||GetCurrentThreadId()!=thread_||!tx::same(v.stamp,stamp_)||!source_equal(v.source)||
            v.desired.count!=115||v.desired.indices!=desired_.indices)return tx::Verdict::denied;
        return receipts_->verify(v,r,e);
    }
};
} // namespace gears_full_body_c0_native
