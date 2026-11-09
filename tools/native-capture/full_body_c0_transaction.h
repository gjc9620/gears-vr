#pragma once
#include "full_body_c0_planner.h"
#include <atomic>
#include <climits>

// Bounded private journal. Native authorization and memory access belong to
// its separately supplied Backend. CAS16 is not array atomicity or a reader fence.
namespace gears_full_body_c0_transaction {
namespace body=gears_full_body_c0;
constexpr unsigned max_units=body::bones+1; // Q for115 bones, plus Root T/scale.
struct alignas(16) Block {std::uint64_t low{},high{};};
static_assert(sizeof(Block)==16&&alignof(Block)==16);
struct Mask {std::array<std::uint64_t,4> words{};}; // bit2*bone+half
inline void include(Mask& m,unsigned bit) noexcept {m.words[bit/64]|=std::uint64_t(1)<<(bit%64);}
inline bool contains(Mask const& m,unsigned bit) noexcept {return (m.words[bit/64]&(std::uint64_t(1)<<(bit%64)))!=0;}
inline bool covers(Mask const& a,Mask const& b) noexcept {for(unsigned i=0;i<4;++i)if((a.words[i]&b.words[i])!=b.words[i])return false;return true;}
struct Binding {
    // These identify Root's private authenticated registrations, not permission
    // booleans. Backend must resolve ALL actual identities/code/layouts itself.
    std::uint64_t owner{},model{},metadata_generation{},session{},world_generation{};
    std::uintptr_t local{};std::uint32_t count{},capacity{};
};
struct Stamp {Binding binding{};std::uint64_t frame{},producer{},scope{};};
inline bool same(Binding const& a,Binding const& b) noexcept {
    return a.owner==b.owner&&a.model==b.model&&a.metadata_generation==b.metadata_generation&&a.session==b.session&&a.world_generation==b.world_generation&&a.local==b.local&&a.count==b.count&&a.capacity==b.capacity;
}
inline bool same(Stamp const& a,Stamp const& b) noexcept {return same(a.binding,b.binding)&&a.frame==b.frame&&a.producer==b.producer&&a.scope==b.scope;}
struct Desired {unsigned count{};std::array<unsigned char,body::bones> indices{};};
// Copied data from this kernel's immutable consumed plan, never a write or
// collision lease. The Native reader obtains this directly from the Kernel.
constexpr std::array<unsigned,7> completed_pose_bones{{body::root,body::pelvis,body::head,
    body::legs[0][2],body::legs[1][2],body::shoulders[0],body::shoulders[1]}};
struct CompletedPose {Stamp stamp{};std::uint64_t serial{};std::array<body::Pose,7> world{};};

enum class UnitState:unsigned char {unwritten,applied,uncertain,conflict,restored,fresh_retired};
struct Unit {std::uint16_t bone{};unsigned char half{};UnitState state{};unsigned char reserved[12]{};Block before{},ours{};};
static_assert(sizeof(Unit)==48&&offsetof(Unit,before)==16&&offsetof(Unit,ours)==32);
enum class Stage {prepare,commit,rollback,consumer_entry,scope_end};
enum class Verdict {verified,denied,fault,source_age_expired};
enum class Cas {exchanged,mismatch,fault_unknown,source_age_no_write};
enum class End {before_consumer,original_returned,original_threw};
enum class Failure {none,source_age_only,hard};
enum class Code {prepared,committed,rolled_back,consumer_marked,scope_closed,fresh_retired,
    invalid_plan,unavailable,busy,wrong_lease,wrong_thread,stopped,guard_rejected,
    compare_mismatch,cas_fault,rollback_pending,already_consumed,bad_receipt};
struct Result {Code code{};unsigned unit{},pending{};};
struct ReceiptRef {std::uint64_t event{};}; // opaque lookup key, no self-asserted bool
struct FreshEvidence {Stamp written{};Mask actual_written{};std::uint64_t event{};};
struct View {Stamp const& stamp;body::Frame const& source;body::Output const& plan;Desired const& desired;Unit const* units;unsigned count;};
class Backend {
public:
    virtual ~Backend()=default;
    virtual std::uint64_t thread_token() const noexcept=0;
    // Root must validate exact owner/frame/code/metadata/Local header/Desired/
    // age and the phase-specific exclusive interval, including mixed expected
    // source/ours units in View. verified is backend evidence, never a user flag.
    virtual Verdict validate(View const&,Stage,unsigned) noexcept=0;
    // Must be one aligned atomic128 comparison. Catch native faults here; an
    // escaping exception is unsupported. fault_unknown is never no-write proof.
    virtual Cas exchange(std::uintptr_t,Block const&,Block const&) noexcept=0;
    // Verify a real completed producer event, independent of value equality.
    // Scope-end completion and coverage of old storage must be authenticated.
    virtual Verdict fresh(View const&,ReceiptRef,FreshEvidence&) noexcept=0;
};
class NativeBackendUnavailable final:public Backend {
public:
    std::uint64_t thread_token() const noexcept override{return 0;}
    Verdict validate(View const&,Stage,unsigned) noexcept override{return Verdict::denied;}
    Cas exchange(std::uintptr_t,Block const&,Block const&) noexcept override{return Cas::fault_unknown;}
    Verdict fresh(View const&,ReceiptRef,FreshEvidence&) noexcept override{return Verdict::denied;}
};
class Kernel;
class Lease {
    friend class Kernel;
#if defined(GEARS_FULL_BODY_C0_TRANSACTION_HOST_TEST)
    friend struct HostLeaseSeam;
#endif
    std::uintptr_t self_{reinterpret_cast<std::uintptr_t>(this)};
    Kernel* kernel_{};std::uint64_t serial_{};bool closed_{};
public:
    Lease()=default;Lease(Lease const&)=delete;Lease& operator=(Lease const&)=delete;
    Lease(Lease&&)=delete;Lease& operator=(Lease&&)=delete;
    ~Lease()=default; // explicit finally/end: never an automatic restoration
};
static_assert(std::is_trivially_destructible<Lease>::value&&!std::is_copy_constructible<Lease>::value);
class Kernel {
    Stamp stamp_{};body::Frame source_{};body::Output plan_{};Desired desired_{};
    std::array<Unit,max_units> units_{};unsigned count_{};
    std::array<body::ik::Matrix,body::bones> check_before_{},check_after_{};
    Backend* backend_{};Lease* lease_{};std::uint64_t serial_{};
    std::atomic<std::uint64_t> thread_{};
    std::atomic<unsigned> outstanding_{},active_{},protocol_{};
    std::atomic<bool> stopping_{},failed_{},consumed_{},original_exception_{};
    std::atomic<Failure> failure_{};
    End ending_{};bool ended_{},commit_finished_{},consumer_verified_{};
    View view() const noexcept{return {stamp_,source_,plan_,desired_,units_.data(),count_};}
    static bool owes(UnitState s) noexcept {return s==UnitState::applied||s==UnitState::uncertain||s==UnitState::conflict;}
    void sync() noexcept {unsigned n{};for(unsigned i=0;i<count_;++i)n+=unsigned(owes(units_[i].state));outstanding_=n;}
    void fail(Failure why) noexcept {failed_=true;stopping_=true;
        if(why==Failure::hard)failure_=why;
        else {auto expected=Failure::none;failure_.compare_exchange_strong(expected,why);}}
    void debt() noexcept {fail(Failure::hard);unsigned x=protocol_.load();while(x!=UINT_MAX&&!protocol_.compare_exchange_weak(x,x+1)){};}
    bool installed() const noexcept {
        for(unsigned i=0;i<count_;++i)if(units_[i].state!=UnitState::applied)return false;
        return true;
    }
    Result result(Code code,unsigned unit=0) const noexcept{return {code,unit,pending()};}
    Code exact(Backend& b,Lease const& l,Stamp const& actual) noexcept {
        const auto t=b.thread_token();
        if(!t||t!=thread_.load()){debt();return Code::wrong_thread;}
        if(backend_!=&b||!active_.load()||lease_!=&l||l.self_!=reinterpret_cast<std::uintptr_t>(&l)||l.kernel_!=this||l.serial_!=serial_||l.closed_||!same(actual,stamp_)){debt();return Code::wrong_lease;}
        return Code::prepared;
    }
    bool shape(Stamp const& s,body::Frame const& f,body::Output const& p,Desired const& desired) noexcept {
        const auto& k=s.binding;
        if(!k.owner||!k.model||!k.metadata_generation||!k.session||!k.world_generation||!s.frame||!s.producer||!s.scope||
           k.count!=body::bones||k.capacity<k.count||k.capacity>512||!k.local||(k.local&15)||k.local>UINTPTR_MAX-std::uintptr_t(k.capacity)*32||
           f.count!=body::bones||f.model_scale!=1||!std::isfinite(f.units_per_metre)||f.units_per_metre<10||f.units_per_metre>500||desired.count>body::bones)return false;
        body::rig::CanonicalGeometry canonical{};if(!body::rig::canonical_geometry(f.reference.data(),body::bones,canonical))return false;
        body::ik::Matrix component{};for(unsigned i=0;i<16;++i)component.m[i]=f.component_to_world.values[i];
        if(!body::d::similarity(component)||std::abs(body::d::length(body::d::row(component,0))-1)>2e-5)return false;
        std::array<std::uint64_t,2> wanted{},changed{};
        for(unsigned i=0;i<desired.count;++i){const unsigned b=desired.indices[i];if(b>=body::bones||(wanted[b/64]&(std::uint64_t(1)<<(b%64))))return false;wanted[b/64]|=std::uint64_t(1)<<(b%64);}
        if(p.report.status!=body::Status::solved||!body::d::goal_pose(p.root_world)||!body::d::goal_pose(p.pelvis_world)||!body::d::goal_pose(p.head_world))return false;
        for(unsigned i=0;i<2;++i)if(!body::d::goal_pose(p.feet_world[i])||!body::d::goal_pose(p.shoulders_world[i]))return false;
        const double residuals[]={p.report.head_error,p.report.foot_error[0],p.report.foot_error[1],p.report.max_length_error};
        for(double x:residuals)if(!std::isfinite(x)||x<0||x>.002)return false;
        const double angles[]={p.report.head_angle_error,p.report.foot_angle_error[0],p.report.foot_angle_error[1]};
        for(double x:angles)if(!std::isfinite(x)||x<0||x>1e-4)return false;
        if(!std::isfinite(p.report.pelvis_projection)||p.report.pelvis_projection<0)return false;
        for(unsigned i=0;i<body::bones;++i){const auto& a=f.local[i];const auto& n=p.local[i];body::Quat q{};
            if(!body::d::normalized({a.q[0],a.q[1],a.q[2],a.q[3]},q)||!body::d::normalized({n.q[0],n.q[1],n.q[2],n.q[3]},q)||!body::d::unit_animation_scale(a.scale)||std::memcmp(&a.scale,&n.scale,4))return false;
            for(unsigned j=0;j<3;++j)if(!std::isfinite(a.translation[j])||!std::isfinite(n.translation[j]))return false;
            if(i&&std::memcmp(a.translation,n.translation,12))return false;
            if(std::memcmp(&a,&n,32))changed[i/64]|=std::uint64_t(1)<<(i%64);
        }
        if(changed!=p.changed_mask||(changed[0]&wanted[0])!=changed[0]||(changed[1]&wanted[1])!=changed[1])return false;
        // Validate the COMPLETE candidate before the first external write.
        // Output pose fields must actually describe these Local atoms, rather
        // than relying on its public report.status or caller-supplied mask.
        double max_length_error{};
        for(unsigned i=0;i<body::bones;++i){std::int32_t parent{};std::memcpy(&parent,f.reference[i].bytes+56,4);
            check_before_[i]=body::d::multiply(body::d::atom_matrix(f.local[i]),i?check_before_[unsigned(parent)]:component);
            check_after_[i]=body::d::multiply(body::d::atom_matrix(p.local[i]),i?check_after_[unsigned(parent)]:component);
            if(!body::d::finite(check_before_[i])||!body::d::finite(check_after_[i]))return false;
            if(i){const double a=body::d::length(body::d::sub(body::d::position(check_before_[i]),body::d::position(check_before_[unsigned(parent)]))),
                b=body::d::length(body::d::sub(body::d::position(check_after_[i]),body::d::position(check_after_[unsigned(parent)])));
                max_length_error=(std::max)(max_length_error,std::abs(a-b));}
        }
        if(max_length_error>.002||std::abs(max_length_error-p.report.max_length_error)>1e-6)return false;
        const unsigned selected[]={body::root,body::pelvis,body::head,body::legs[0][2],body::legs[1][2],body::shoulders[0],body::shoulders[1]};
        const body::Pose* reported[]={&p.root_world,&p.pelvis_world,&p.head_world,&p.feet_world[0],&p.feet_world[1],&p.shoulders_world[0],&p.shoulders_world[1]};
        for(unsigned i=0;i<7;++i){const auto& m=check_after_[selected[i]];
            if(!body::d::similarity(m)||body::d::length(body::d::sub(body::d::position(m),reported[i]->position))>.002||body::d::angle(body::d::rotation(m),reported[i]->orientation)>1e-4)return false;}
        return true;
    }
    Result rollback_impl() noexcept {
        if(consumed_.load())return result(Code::already_consumed);
        for(unsigned i=count_;i>0;--i){auto& u=units_[i-1];if(!owes(u.state))continue;
            if(backend_->validate(view(),Stage::rollback,i-1)!=Verdict::verified){fail(Failure::hard);sync();return result(Code::rollback_pending,i-1);}
            const auto c=backend_->exchange(stamp_.binding.local+std::uintptr_t(u.bone)*32+u.half*16,u.ours,u.before);
            if(c!=Cas::exchanged)fail(Failure::hard);
            u.state=c==Cas::exchanged?UnitState::restored:c==Cas::mismatch?UnitState::conflict:UnitState::uncertain;sync();
        }
        return result(outstanding_.load()?Code::rollback_pending:Code::rolled_back);
    }
public:
    explicit Kernel(std::uint64_t serial=0) noexcept:serial_(serial){};Kernel(Kernel const&)=delete;Kernel& operator=(Kernel const&)=delete;
    Kernel(Kernel&&)=delete;Kernel& operator=(Kernel&&)=delete;
    unsigned pending() const noexcept {const auto n=std::uint64_t(outstanding_.load())+active_.load()+protocol_.load();return n>UINT_MAX?UINT_MAX:unsigned(n);}
    Mask outstanding_mask() const noexcept {
        Mask mask{};for(unsigned i=0;i<count_;++i)if(owes(units_[i].state))include(mask,units_[i].bone*2+units_[i].half);
        return mask; // Owned diagnostic data, never a retirement receipt.
    }
    unsigned outstanding_fields() const noexcept{return outstanding_.load();}
    unsigned protocol_debt() const noexcept{return protocol_.load();}
    bool failed() const noexcept{return failed_.load();}
    bool stopping() const noexcept{return stopping_.load();} // read-only first-failure diagnostics
    bool consumed() const noexcept{return consumed_.load();}
    bool original_exception() const noexcept{return original_exception_.load();}
    Failure failure() const noexcept{return failure_.load();}
    std::uint64_t serial() const noexcept{return serial_;}
    // Read-only consumption identity. This cannot prepare/commit, refresh a
    // source, notify again, mutate debt or extend the lease past exact finish.
    bool retained_consumer(Backend& backend,Lease const& lease,Stamp const& actual) const noexcept {
        const auto thread=backend.thread_token();
        return thread&&thread==thread_.load()&&backend_==&backend&&active_.load()&&lease_==&lease&&
            lease.self_==reinterpret_cast<std::uintptr_t>(&lease)&&lease.kernel_==this&&lease.serial_==serial_&&!lease.closed_&&
            same(actual,stamp_)&&consumed_.load()&&consumer_verified_&&commit_finished_&&installed()&&!ended_&&!failed_.load()&&!stopping_.load()&&
            !original_exception_.load()&&!protocol_.load();
    }
    // Read-only proof of this kernel's exact, normally completed original
    // consumption. It is valid only until fresh retirement or another prepare;
    // it grants no move/write/occupancy permission and does not assert final
    // SpaceBases, physics or hit geometry. Caller must bind those separately.
    // Owed applied fields may remain: normal consumption is not restoration.
    bool completed_consumer(Backend& backend,Stamp const& actual,std::uint64_t serial) const noexcept {
        const auto thread=backend.thread_token();
        return thread&&thread==thread_.load()&&backend_==&backend&&serial&&serial==serial_&&
            same(actual,stamp_)&&!active_.load()&&!lease_&&ended_&&ending_==End::original_returned&&
            consumed_.load()&&consumer_verified_&&commit_finished_&&installed()&&!failed_.load()&&
            !stopping_.load()&&!original_exception_.load()&&!protocol_.load();
    }
    bool completed_pose(Backend& backend,Stamp const& actual,std::uint64_t serial,CompletedPose& out) const noexcept {
        const body::d::Range ranges[]={body::d::range(this,sizeof *this),body::d::range(&actual,sizeof actual),body::d::range(&out,sizeof out)};
        if(!body::d::disjoint(ranges,3)||!completed_consumer(backend,actual,serial))return false;
        CompletedPose next{};next.stamp=stamp_;next.serial=serial_;
        next.world={plan_.root_world,plan_.pelvis_world,plan_.head_world,plan_.feet_world[0],plan_.feet_world[1],
            plan_.shoulders_world[0],plan_.shoulders_world[1]};out=next;return true;
    }
    bool settled_source_age() const noexcept {return failure_.load()==Failure::source_age_only&&failed_.load()&&stopping_.load()&&
        ended_&&ending_!=End::original_threw&&!original_exception_.load()&&!pending();}
    void request_stop() noexcept {fail(Failure::hard);} // never clears obligations
    Result prepare(Backend& b,Stamp const& stamp,body::Frame const& source,body::Output const& plan,Desired const& desired,Lease& lease) noexcept {
        const auto t=b.thread_token();if(!t)return result(Code::unavailable);
        auto previous=thread_.load();if(!previous){thread_.compare_exchange_strong(previous,t);previous=thread_.load();}
        if(previous!=t){debt();return result(Code::wrong_thread);}
        if(pending())return result(Code::busy);
        if(stopping_.load())return result(Code::stopped);
        const body::d::Range ranges[]={body::d::range(this,sizeof *this),body::d::range(&source,sizeof source),body::d::range(&plan,sizeof plan),body::d::range(&stamp,sizeof stamp),body::d::range(&desired,sizeof desired),body::d::range(&lease,sizeof lease),{stamp.binding.local,body::bones*sizeof(body::Atom)}};
        if(!body::d::disjoint(ranges,7)||lease.kernel_||lease.closed_||lease.self_!=reinterpret_cast<std::uintptr_t>(&lease)||!shape(stamp,source,plan,desired))return result(Code::invalid_plan);
        stamp_=stamp;source_=source;plan_=plan;desired_=desired;count_=0;units_={};consumed_=false;commit_finished_=false;consumer_verified_=false;ended_=false;
        for(unsigned bone=0;bone<body::bones;++bone)for(unsigned half=0;half<2;++half){
            const auto* before=reinterpret_cast<unsigned char const*>(&source_.local[bone])+half*16;const auto* after=reinterpret_cast<unsigned char const*>(&plan_.local[bone])+half*16;
            if(!std::memcmp(before,after,16))continue;
            if(half&&bone)return result(Code::invalid_plan);
            auto& u=units_[count_++];u.bone=std::uint16_t(bone);u.half=static_cast<unsigned char>(half);std::memcpy(&u.before,before,16);std::memcpy(&u.ours,after,16);
        }
        if(b.validate(view(),Stage::prepare,0)!=Verdict::verified)return result(Code::guard_rejected);
        if(serial_==UINT64_MAX){debt();return result(Code::stopped);}
        backend_=&b;lease_=&lease;++serial_;lease.kernel_=this;lease.serial_=serial_;active_=1;return result(Code::prepared);
    }
    Result commit(Backend& b,Lease& lease,Stamp const& actual) noexcept {
        auto c=exact(b,lease,actual);if(c!=Code::prepared)return result(c);
        if(consumed_.load())return result(Code::already_consumed);
        if(commit_finished_){debt();return result(Code::wrong_lease);}
        for(unsigned i=0;i<count_;++i){
            if(stopping_.load()){fail(Failure::hard);rollback_impl();return result(Code::guard_rejected,i);}
            const auto verdict=b.validate(view(),Stage::commit,i);
            if(verdict!=Verdict::verified){fail(verdict==Verdict::source_age_expired?Failure::source_age_only:Failure::hard);
                rollback_impl();return result(Code::guard_rejected,i);}
            auto& u=units_[i];u.state=UnitState::uncertain;sync(); // BEFORE CAS/fault
            const auto r=b.exchange(stamp_.binding.local+std::uintptr_t(u.bone)*32+u.half*16,u.before,u.ours);
            if(r==Cas::exchanged){u.state=UnitState::applied;sync();continue;}
            if(r==Cas::mismatch||r==Cas::source_age_no_write)u.state=UnitState::unwritten;sync();
            fail(r==Cas::source_age_no_write?Failure::source_age_only:Failure::hard);rollback_impl();
            if(r==Cas::source_age_no_write)return result(Code::guard_rejected,i);
            return result(r==Cas::mismatch?Code::compare_mismatch:Code::cas_fault,i);
        }
        commit_finished_=true;return result(Code::committed);
    }
    Result rollback(Backend& b,Lease& lease,Stamp const& actual) noexcept {
        auto c=exact(b,lease,actual);if(c!=Code::prepared)return result(c);return rollback_impl();
    }
    // Mandatory notification immediately BEFORE original Compose invocation,
    // including its passthrough/failure path. This is NOT permission to invoke
    // an engine function. Once set, even an exception forbids all C0 rollback.
    Result notify_consumer_entry(Backend& b,Lease& lease,Stamp const& actual) noexcept {
        const bool already=consumed_.exchange(true); // fail closed BEFORE any rejection
        auto c=exact(b,lease,actual);if(c!=Code::prepared)return result(c);
        if(already){debt();return result(Code::already_consumed);}
        if(b.validate(view(),Stage::consumer_entry,0)!=Verdict::verified){fail(Failure::hard);return result(Code::guard_rejected);}
        consumer_verified_=true;
        return result(Code::consumer_marked);
    }
    Result finish(Backend& b,Lease& lease,Stamp const& actual,End ending) noexcept {
        // A finally that knows original returned/threw is also an irreversible
        // consumption notification, even if its scope/owner validation fails.
        if(ending!=End::before_consumer)consumed_=true;
        if(ending==End::original_threw){original_exception_=true;fail(Failure::hard);}
        auto c=exact(b,lease,actual);if(c!=Code::prepared)return result(c);
        if(ending!=End::before_consumer&&ending!=End::original_returned&&ending!=End::original_threw){debt();return result(Code::wrong_lease);}
        if(b.validate(view(),Stage::scope_end,0)!=Verdict::verified){debt();return result(Code::guard_rejected);}
        if(!consumed_.load())rollback_impl();
        lease.closed_=true;lease_=nullptr;active_=0;ending_=ending;ended_=true;sync();return result(Code::scope_closed);
    }
    Result retire_fresh(Backend& b,ReceiptRef receipt) noexcept {
        if(!thread_.load()||b.thread_token()!=thread_.load()){debt();return result(Code::wrong_thread);}
        if(backend_!=&b||active_.load()||!outstanding_.load())return result(Code::bad_receipt);
        FreshEvidence evidence{};
        if(!receipt.event||b.fresh(view(),receipt,evidence)!=Verdict::verified||evidence.event!=receipt.event||!same(evidence.written.binding,stamp_.binding)||
           evidence.written.frame<=stamp_.frame||evidence.written.producer<=stamp_.producer||!evidence.written.scope||evidence.written.scope==stamp_.scope||(evidence.actual_written.words[3]>>38))return result(Code::bad_receipt);
        Mask required{};for(unsigned i=0;i<count_;++i)if(owes(units_[i].state))include(required,unsigned(units_[i].bone)*2+units_[i].half);
        if(!covers(evidence.actual_written,required))return result(Code::bad_receipt);
        for(unsigned i=0;i<count_;++i)if(owes(units_[i].state))units_[i].state=UnitState::fresh_retired;
        sync();return result(Code::fresh_retired); // not "restored C0"
    }
};
} // namespace gears_full_body_c0_transaction
