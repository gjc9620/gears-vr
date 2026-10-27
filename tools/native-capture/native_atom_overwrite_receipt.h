#pragma once
#include "full_body_c0_transaction.h"
#include "cover_pose_phase.h"
#include <cstring>
// Field overwrite only. No BaseClean, FK, Space or physics claim follows.
namespace gears_atom_overwrite {
namespace tx=gears_full_body_c0_transaction;namespace pose=gears_cover_pose_phase;
struct Arguments {std::uintptr_t sequence{},out{},caller{};std::uint64_t track{},fifth{},sixth{};std::uint32_t time_bits{};};
struct Cohort {pose::ArrayHeader raw{},compressed{},offsets{};std::array<std::int32_t,4> offset{};
    std::array<unsigned char,48> raw_track{};std::uint32_t duration_bits{};unsigned char format{};unsigned branch{},read_mask{};};
inline bool equal(Cohort const&a,Cohort const&b) noexcept {return pose::header_equal(a.raw,b.raw)&&pose::header_equal(a.compressed,b.compressed)&&
    pose::header_equal(a.offsets,b.offsets)&&a.offset==b.offset&&a.raw_track==b.raw_track&&a.duration_bits==b.duration_bits&&a.format==b.format&&a.branch==b.branch&&a.read_mask==b.read_mask;}
inline bool overlaps(std::uintptr_t a,std::size_t an,std::uintptr_t b,std::size_t bn) noexcept {return pose::span(a,an)&&pose::span(b,bn)&&a<=b+bn-1&&b<=a+an-1;}
inline bool animation_array(pose::ArrayHeader const&h,unsigned stride,unsigned budget)noexcept {
    if(!stride||h.count<0||h.capacity<h.count||h.capacity<0||std::uint64_t(h.capacity)>budget)return false;
    if(!h.capacity)return h.count==0;const auto n=std::uint64_t(h.capacity)*stride;
    return n<=SIZE_MAX&&pose::span(h.data,std::size_t(n));
}
template<class Read>bool cohort(Read&& read,Arguments const&a,Cohort& c) {
    c={};if(!pose::span(a.sequence,0xe4)||!pose::span(a.out,32)||overlaps(a.out,32,a.sequence,0xe4)||a.track>0x1fffffffu)return false;
    auto get=[&](auto& value,std::uintptr_t at){return pose::span(at,sizeof value)&&read(&value,reinterpret_cast<void const*>(at),sizeof value);};
    if(!get(c.duration_bits,a.sequence+0x78)||!get(c.format,a.sequence+0xc1)||!get(c.raw,a.sequence+0x88)||
       !get(c.compressed,a.sequence+0xc4)||!get(c.offsets,a.sequence+0xd4))return false;c.read_mask=1;
    float time{},duration{};std::memcpy(&time,&a.time_bits,4);std::memcpy(&duration,&c.duration_bits,4);
    if(!std::isfinite(time)||!std::isfinite(duration)||duration<=0||c.format>5)return false;
    // The real branch uses only the low DWORD of arg6. Preserve all 64 input bits.
    if(std::uint32_t(a.sixth)||c.offsets.count<=0){c.branch=2;
        if(!animation_array(c.raw,48,65536))return false;
        if(!c.raw.count){c.branch=3;c.read_mask|=2;return true;}
        if(a.track>=std::uint64_t(c.raw.count))return false;const auto at=c.raw.data+std::uintptr_t(a.track)*48;
        if(overlaps(a.out,32,at,48)||!get(c.raw_track,at))return false;c.read_mask|=2;
        std::int32_t n[3]{};for(unsigned i=0;i<3;++i)std::memcpy(&n[i],c.raw_track.data()+i*16+8,4);
        for(auto count:n)if(count<0||count>65536)return false;
        if(!n[0]||!n[1]||!n[2])c.branch=3;return true;
    }
    c.branch=1;
    if(!animation_array(c.offsets,4,4*65536)||a.track*4+4>std::uint64_t(c.offsets.count)||
       !animation_array(c.compressed,1,64*1024*1024))return false;
    const auto at=c.offsets.data+std::uintptr_t(a.track)*16;
    if(overlaps(a.out,32,at,16)||!get(c.offset,at))return false;c.read_mask|=2;
    if(c.offset[1]<0||c.offset[1]>65536||c.offset[3]<0||c.offset[3]>65536)return false;
    if(!c.offset[1]||!c.offset[3]){c.branch=3;return true;}
    return c.offset[0]>=0&&c.offset[2]>=0&&c.offset[0]<c.compressed.count&&c.offset[2]<c.compressed.count;
    // Success + exact normal return proves stores, not source cleanliness.
    // Key payload values are deliberately not treated as coverage evidence.
}
struct Observation {pose::Metadata metadata{};std::uint64_t journal_version{},qpc{};};
class Context {public:virtual ~Context()=default;virtual std::uint64_t thread_token()const noexcept=0;
    virtual bool current(tx::Stamp const&,Observation&) noexcept=0;};
class ReceiptSource {public:virtual ~ReceiptSource()=default;virtual tx::Verdict verify(tx::View const&,tx::ReceiptRef,tx::FreshEvidence&)noexcept=0;};
struct Ticket {Ticket const*self{};std::uint64_t version{},serial{},thread{};unsigned bone{};Observation before{};Cohort source{};bool admitted{};};
class Registry final:public ReceiptSource {
    Context* context_{};tx::Stamp installed_{};std::uint32_t installed_epoch_{},coverage_epoch_{};
    std::uint64_t version_{},serial_{},event_{},return_qpc_{};std::uintptr_t caller_{};tx::Mask covered_{};Observation last_{};Ticket*active_{};
    std::atomic<std::uint64_t> thread_{};std::atomic<unsigned> protocol_{},in_call_{};bool bound_{};
    void debt()noexcept{auto x=protocol_.load();while(x!=UINT_MAX&&!protocol_.compare_exchange_weak(x,x+1)){};}
public:
    std::uint64_t version()const noexcept{return version_;}
    std::uint64_t event()const noexcept{return event_;}
    // Cheap same-GT selection only; this grants neither coverage nor write permission.
    // Recheck all source, epoch, version and identity requirements in begin/finish.
    bool may_match(std::uintptr_t out,std::uintptr_t caller,std::uint64_t thread)const noexcept {
        if(!thread||thread_.load()!=thread||!bound_||protocol_.load()||caller!=caller_||out<installed_.binding.local)return false;
        const auto offset=out-installed_.binding.local;return offset<=115*32-32&&offset%32==0;
    }
    bool can_install(std::uint64_t thread)const noexcept{return thread&&(!thread_.load()||thread_.load()==thread)&&!active_&&!bound_&&!protocol_.load()&&version_!=UINT64_MAX;}
    // Methods that mutate/read the journal run on the externally registered GT;
    // cleanup threads only call this atomic pending query.
    unsigned pending()const noexcept {const auto n=std::uint64_t(protocol_.load())+in_call_.load();return n>UINT_MAX?UINT_MAX:unsigned(n);}
    bool install(Context& context,tx::Stamp const&s,std::uint32_t epoch,std::uintptr_t caller)noexcept {
        const auto t=context.thread_token();auto expected=thread_.load();if(!expected)thread_.compare_exchange_strong(expected,t);
        if(!t||thread_.load()!=t||active_||bound_||protocol_.load()||!epoch||epoch==UINT32_MAX||!caller||version_==UINT64_MAX||!s.frame||!s.producer||!s.scope||
           s.binding.count!=115||s.binding.capacity<115||!s.binding.local||(s.binding.local&15)||!pose::span(s.binding.local,115*32))return false;
        context_=&context;installed_=s;installed_epoch_=epoch;caller_=caller;coverage_epoch_=0;++version_;covered_={};event_=return_qpc_=0;last_={};bound_=true;return true;
    }
    bool begin(Arguments const&a,Cohort const&source,Ticket&t)noexcept {
        t.self=&t;t.thread=context_?context_->thread_token():0;
        if(!bound_||!t.thread||thread_.load()!=t.thread||protocol_.load())return false;
        if(active_){debt();return false;}
        if(a.caller!=caller_||source.read_mask!=3||source.format>5||source.branch<1||source.branch>3||
           a.out<installed_.binding.local||a.out-installed_.binding.local>=115*32||((a.out-installed_.binding.local)%32)||serial_==UINT64_MAX)return false;
        Observation now{};if(!context_->current(installed_,now)||now.journal_version!=version_||now.metadata.pose_epoch<=installed_epoch_||now.qpc<=installed_.producer)return false;
        if(coverage_epoch_!=now.metadata.pose_epoch){covered_={};coverage_epoch_=now.metadata.pose_epoch;event_=return_qpc_=0;}
        t.version=version_;t.serial=++serial_;t.bone=unsigned((a.out-installed_.binding.local)/32);t.before=now;t.source=source;t.admitted=true;active_=&t;in_call_=1;return true;
    }
    bool finish(Ticket&t,Cohort const&source,bool returned,bool packet_preserved)noexcept {
        if(!t.admitted)return false;
        const bool exact=t.self==&t&&active_==&t&&t.version==version_&&t.thread==thread_.load()&&context_&&context_->thread_token()==t.thread;
        if(!exact){debt();return false;}active_=nullptr;in_call_=0;t.admitted=false;
        if(!returned||!packet_preserved||protocol_.load()||!equal(t.source,source))return false;
        Observation now{};if(!context_->current(installed_,now)||now.journal_version!=version_||now.metadata.pose_epoch!=coverage_epoch_||
           now.metadata.pose_epoch<=installed_epoch_||!pose::metadata_equal(now.metadata,t.before.metadata)||now.qpc<=t.before.qpc||now.qpc<=installed_.producer)return false;
        tx::include(covered_,t.bone*2);tx::include(covered_,t.bone*2+1);last_=now;return_qpc_=now.qpc;event_=t.serial;return true;
    }
    void abandon(Ticket&t)noexcept {if(t.admitted){if(t.self==&t&&active_==&t){active_=nullptr;in_call_=0;t.admitted=false;}debt();}}
    tx::Verdict verify(tx::View const&v,tx::ReceiptRef r,tx::FreshEvidence&out)noexcept override {
        if(!bound_||active_||protocol_.load()||!context_||context_->thread_token()!=thread_.load()||!r.event||r.event!=event_||!tx::same(v.stamp,installed_))return tx::Verdict::denied;
        tx::Mask need{};for(unsigned i=0;i<v.count;++i){const auto&u=v.units[i];if(u.bone>=115||u.half>1)return tx::Verdict::denied;
            if(u.state==tx::UnitState::applied||u.state==tx::UnitState::uncertain||u.state==tx::UnitState::conflict)tx::include(need,u.bone*2+u.half);}
        if(!tx::covers(covered_,need)||return_qpc_<=installed_.frame||return_qpc_<=installed_.producer)return tx::Verdict::denied;
        Observation now{};if(!context_->current(installed_,now)||now.journal_version!=version_||now.metadata.pose_epoch!=coverage_epoch_||
            now.metadata.pose_epoch<=installed_epoch_||!pose::metadata_equal(now.metadata,last_.metadata))return tx::Verdict::denied;
        out={};out.written=installed_;out.written.frame=out.written.producer=out.written.scope=return_qpc_;out.actual_written=covered_;out.event=event_;return tx::Verdict::verified;
    }
    bool retired(std::uint64_t version,std::uint64_t event)noexcept {if(!bound_||active_||version!=version_||event!=event_||protocol_.load())return false;
        bound_=false;covered_={};event_=return_qpc_=0;return true;}
};
}
