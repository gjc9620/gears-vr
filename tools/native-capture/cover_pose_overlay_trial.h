#pragma once
#include "cover_pose_phase.h"
#include "cover_skeleton_ik.h"
#include "../vr-common/hand_two_bone_pose.h"
#include <atomic>

// Private automated trial ONLY. No export, hook, PG authority or engine call.
// Root must authenticate the real GT/outer meshTick and exclude Scene, stereo
// and callback consumers for EACH call. All IO functions must be protected,
// noexcept, and confined to that explicit scope. rollback_failed MUST abort
// before native consumers; a denied rollback cannot be disguised as recovery.
// write(false) MAY have written any prefix; immediate rollback handles it.
namespace gears_cover_pose_overlay_trial {
namespace pose=gears_cover_pose_phase;
namespace ik=gears_cover_skeleton_ik;
namespace arm=gears_hand_two_bone;
constexpr unsigned required_proofs=31; // controlled mesh, GT, outer tick, Scene/stereo idle, callbacks idle
struct Params {bool enabled{};ik::Vec delta{};double roll{};unsigned ticks{1};bool adaptive{};ik::Chain chain{8,10,11};};
struct Scope {
    pose::OwnerKey owner{};std::uint64_t nonce{},tick{},call{};
    unsigned thread{},ct{},sm{},proofs{};pose::Phase phase{};
    // Caller certifies SHA256 -> initial Ref bytes; kernel additionally checks
    // every Ref byte at all phases. This field is not a permission by itself.
    std::array<unsigned char,32> reference_sha256{};
};
struct Input {pose::Snapshot const* snapshot{};Scope scope{};};
enum class Status {disabled,okay,applied,restored,drained,rejected,sequence,context,conflict,write_failed,rollback_failed};
struct Stats {
    unsigned applied_ticks{},restored_fields{},rollback_fields{},faults{},native_c1{},native_l1{};
    unsigned native_replaced_locals{},native_replaced_space{},c0_c1_changed_atoms{},c1_l1_changed_atoms{},c1_l1_changed_space{};
    Status last_fault{Status::okay};
};
struct Record {std::uintptr_t address{};unsigned size{};bool local{},active{},known{true};unsigned char before[64]{},after[64]{};};
// Caller-owned scratch: never allocate these large buffers on the game stack.
struct Workspace {
    ik::Workspace math{};ik::Atom input[pose::max_bones]{},solved[pose::max_bones]{};
    ik::Matrix space[pose::max_bones]{},world[pose::max_bones]{},component{};
    std::int32_t parents[pose::max_bones]{};pose::Matrix packed[pose::max_bones]{};
    unsigned char buffer[64]{};
};
namespace detail {
inline bool identity(pose::Identity const& a,pose::Identity const& b) noexcept {
    return a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&a.internal_index==b.internal_index&&a.name_index==b.name_index&&a.number==b.number;
}
inline bool owner(pose::OwnerKey const& a,pose::OwnerKey const& b,bool scene=false) noexcept {
    return identity(a.world,b.world)&&identity(a.pawn,b.pawn)&&identity(a.mesh,b.mesh)&&a.session==b.session&&a.world_generation==b.world_generation&&
        (!scene||(a.scene==b.scene&&a.scene_qpc==b.scene_qpc));
}
inline bool layout(pose::Metadata const& a,pose::Metadata const& b) noexcept {
    return a.asset==b.asset&&a.master==b.master&&a.physics_asset==b.physics_asset&&a.physics_instance==b.physics_instance&&
        pose::header_equal(a.reference,b.reference)&&pose::header_equal(a.local,b.local)&&pose::header_equal(a.space,b.space)&&pose::header_equal(a.desired,b.desired);
}
template<class IO> struct Reader {IO& io;bool operator()(void* out,void const* in,std::size_t n) noexcept {return io.read(out,reinterpret_cast<std::uintptr_t>(in),n);}};
} // namespace detail

class Trial {
    enum class Stage {c0,c1,l1,blocked};
    Params params_{};std::atomic<bool> stop_{true};bool armed_{},bound_{},debt_{},native_c1_{},busy_{};
    Stage stage_{Stage::c0};std::uint64_t nonce_{},last_nonce_{},tick_{},call_{};std::int64_t qpc_{};
    Scope binding_{},tick_scope_{};pose::Metadata metadata_{};unsigned count_{},records_{},trial_ticks_{};
    std::array<pose::RefBone,pose::max_bones> reference_{};
    // Distinct facts: raw C0 receipt, native C1 baseline, and each record's
    // native-C1 before / verified overlay after. C0 never defines C1 baseline.
    std::array<pose::Atom,pose::max_bones> c0_input_{},native_local_{};
    std::array<pose::Matrix,pose::max_bones> native_space_{};
    std::array<Record,pose::max_bones+5> journal_{};
    Stats stats_{};
    Status fail(Status s,bool unsafe=false) noexcept {
        stop_.store(true);++stats_.faults;stats_.last_fault=s;
        if(unsafe||s==Status::context||s==Status::sequence||s==Status::conflict||s==Status::rollback_failed){
            stage_=Stage::blocked;return debt_||unsafe?Status::rollback_failed:s;}
        return s;
    }
    template<class IO> bool metadata_current(Input const& in,IO& io) noexcept {
        detail::Reader<IO> read{io};pose::Metadata current{};
        return pose::metadata(read,in.snapshot->request,current)&&pose::metadata_equal(in.snapshot->metadata,current)&&
            pose::repeat(read,in.snapshot->reference.data(),current.reference.data,count_*sizeof(pose::RefBone))==pose::Reject::none&&
            pose::repeat(read,in.snapshot->desired.data(),current.desired.data,static_cast<std::size_t>(current.desired.count))==pose::Reject::none;
    }
    template<class IO> bool proof(Input const& in,IO& io,Workspace const& w) noexcept {
        if(!in.snapshot)return false;const auto& s=*in.snapshot;const auto& p=in.scope;const auto& m=s.metadata;
        bool hash=false;for(auto b:p.reference_sha256)hash|=b!=0;
        if(!hash||p.nonce!=nonce_||!p.tick||!p.call||!p.thread||p.ct||p.sm||p.proofs!=required_proofs||
            !pose::request_valid(s.request)||p.call!=s.request.invocation||p.thread!=s.request.thread||p.phase!=s.request.phase||
            !detail::owner(p.owner,s.request.owner,true)||s.request.qpc<qpc_||
            (p.phase!=pose::Phase::C0&&p.phase!=pose::Phase::C1&&p.phase!=pose::Phase::L1)||
            (p.phase==pose::Phase::L1&&p.call!=p.tick))return false;
        if(!pose::header_valid(m.reference,68)||!pose::header_valid(m.local,32)||!pose::header_valid(m.space,64)||!pose::header_valid(m.desired,1,true)||
            m.local.count!=m.reference.count||m.space.count!=m.reference.count||m.desired.count>m.reference.count||m.local.data%4||m.space.data%4)return false;
        const auto n=static_cast<unsigned>(m.local.count);
        ik::detail::Range ranges[]={ik::detail::range(reinterpret_cast<void*>(m.local.data),n*32),ik::detail::range(reinterpret_cast<void*>(m.space.data),n*64),
            ik::detail::range(reinterpret_cast<void*>(m.reference.data),n*68),ik::detail::range(this,sizeof(*this)),ik::detail::range(in.snapshot,sizeof(s)),
            ik::detail::range(&w,sizeof(w)),ik::detail::range(&in,sizeof(in)),
            // Native metadata/control bytes already read by pose::metadata.
            ik::detail::range(reinterpret_cast<void*>(p.owner.mesh.address),0x354),ik::detail::range(reinterpret_cast<void*>(m.asset),0xb4),
            ik::detail::range(reinterpret_cast<void*>(m.desired.data),static_cast<std::size_t>(m.desired.count))};
        const auto& chain=params_.chain;
        if(!ik::detail::disjoint(ranges,m.desired.count?10:9)||pose::validate(s)!=pose::Reject::none||pose::parent(s.reference[0])!=0||
            chain.upper>=n||chain.lower>=n||chain.end>=n||pose::parent(s.reference[chain.lower])!=static_cast<int>(chain.upper)||
            pose::parent(s.reference[chain.end])!=static_cast<int>(chain.lower))return false;
        if(bound_&&(!detail::owner(binding_.owner,p.owner)||binding_.thread!=p.thread||!detail::layout(metadata_,m)||
            binding_.reference_sha256!=p.reference_sha256||std::memcmp(reference_.data(),s.reference.data(),n*68)))return false;
        count_=n;detail::Reader<IO> read{io};
        return metadata_current(in,io)&&pose::repeat(read,s.local.data(),m.local.data,n*32)==pose::Reject::none&&
            pose::repeat(read,s.space.data(),m.space.data,n*64)==pose::Reject::none&&metadata_current(in,io);
    }
    bool append(std::uintptr_t address,void const* before,void const* after,unsigned n,bool local) noexcept {
        if(!std::memcmp(before,after,n))return true;if(records_==journal_.size())return false;
        auto& r=journal_[records_++];r={};r.address=address;r.size=n;r.local=local;
        std::memcpy(r.before,before,n);std::memcpy(r.after,after,n);return true;
    }
    template<class IO> bool rollback(Input const& in,IO& io,Workspace& w) noexcept {
        bool okay=true;
        for(unsigned i=records_;i>0;--i){auto& r=journal_[i-1];if(!r.active)continue;
            if(!metadata_current(in,io)){okay=false;continue;}
            if(!io.read(w.buffer,r.address,r.size)){r.known=false;okay=false;continue;}
            if(!std::memcmp(w.buffer,r.before,r.size)){r.active=false;continue;}
            bool mixed=true;for(unsigned j=0;j<r.size;++j)mixed&=w.buffer[j]==r.before[j]||w.buffer[j]==r.after[j];
            if(!mixed){r.known=false;okay=false;continue;}
            if(io.writable(r.address,r.size))io.write(r.address,r.before,r.size);
            if(!io.read(w.buffer,r.address,r.size)){r.known=false;okay=false;continue;}
            if(!std::memcmp(w.buffer,r.before,r.size)){r.active=false;++stats_.rollback_fields;}
            else {std::memcpy(r.after,w.buffer,r.size);r.known=true;okay=false;}
        }detail::Reader<IO> read{io};const auto& s=*in.snapshot;
        return okay&&metadata_current(in,io)&&pose::repeat(read,s.local.data(),s.metadata.local.data,count_*32)==pose::Reject::none&&
            pose::repeat(read,s.space.data(),s.metadata.space.data,count_*64)==pose::Reject::none&&metadata_current(in,io);
    }
    template<class IO> bool restore_locals(Input const& in,IO& io,Workspace& w) noexcept {
        bool okay=true;
        for(unsigned i=0;i<records_;++i){auto& r=journal_[i];if(!r.local||!r.active)continue;
            if(!metadata_current(in,io)){okay=false;continue;}
            if(!io.read(w.buffer,r.address,r.size)){okay=false;continue;}
            if(!std::memcmp(w.buffer,r.before,r.size)){r.active=false;++stats_.native_replaced_locals;continue;}
            if(!r.known){okay=false;continue;}
            // Only a fully certified new Native C0 may relinquish a known
            // owned field replaced by fresh input; never overwrite that input.
            if(std::memcmp(w.buffer,r.after,r.size)){r.active=false;++stats_.native_replaced_locals;continue;}
            if(io.writable(r.address,r.size))io.write(r.address,r.before,r.size);
            if(!io.read(w.buffer,r.address,r.size)){r.known=false;okay=false;continue;}
            if(!std::memcmp(w.buffer,r.before,r.size)){r.active=false;++stats_.restored_fields;}
            else {std::memcpy(r.after,w.buffer,r.size);okay=false;}
        }return okay;
    }
    bool native_replaced_space(pose::Snapshot const& s) noexcept {
        bool okay=true;
        for(unsigned i=0;i<records_;++i){auto& r=journal_[i];if(!r.active)continue;
            if(r.local){okay=false;continue;}
            const auto offset=r.address-s.metadata.space.data;
            const auto* current=reinterpret_cast<unsigned char const*>(s.space.data())+offset;
            bool replaced=!std::memcmp(current,r.before,r.size);
            if(!replaced&&r.known){replaced=true;
                for(unsigned j=0;j<r.size;j+=4){
                    if(!std::memcmp(r.before+j,r.after+j,4))continue;
                    if(!std::memcmp(current+j,r.after+j,4)){replaced=false;break;}
                }}
            if(replaced){r.active=false;++stats_.native_replaced_space;}
            else okay=false; // Every changed float32 needs exact replacement; FK/one changed matrix byte is insufficient.
        }return okay;
    }
    bool prepare(pose::Snapshot const& s,Workspace& w) noexcept {
        std::memcpy(w.input,s.local.data(),count_*32);
        for(unsigned i=0;i<count_;++i)w.parents[i]=pose::parent(s.reference[i]);
        for(unsigned j=0;j<16;++j)w.component.m[j]=s.metadata.component.values[j];
        if(ik::forward(w.input,w.parents,count_,w.component,w.math,w.space,w.world)!=ik::Status::okay)return false;
        for(unsigned i=0;i<count_;++i)for(unsigned j=0;j<16;++j)
            if(std::abs(w.space[i].m[j]-s.space[i].values[j])>.001)return false;
        return true;
    }
    bool plan(pose::Snapshot const& s,Workspace& w) noexcept {
        const auto& chain=params_.chain;
        arm::Input input{};if(ik::world_pose(w.world[chain.upper],input.shoulder)!=ik::Status::okay||ik::world_pose(w.world[chain.lower],input.elbow)!=ik::Status::okay||
            ik::world_pose(w.world[chain.end],input.wrist)!=ik::Status::okay)return false;
        input.target=input.wrist;input.target.position.x+=params_.delta.x;input.target.position.y+=params_.delta.y;input.target.position.z+=params_.delta.z;
        input.target.orientation=gears_vr::multiply(input.wrist.orientation,{std::sin(params_.roll/2),0,0,std::cos(params_.roll/2)});
        input.lengths.mode=params_.adaptive?arm::LengthMode::adaptive:arm::LengthMode::preserve;arm::Solution solution{};
        if(arm::solve(input,solution)!=arm::Status::solved)return false;
        const ik::Pose goals[]={solution.shoulder,solution.elbow,solution.wrist};ik::AdaptResult result{};
        if(ik::world_poses_to_local(w.input,w.parents,count_,w.component,chain,goals,params_.adaptive?ik::LengthPolicy::adapt_translations:ik::LengthPolicy::fixed,
            w.math,w.solved,w.space,w.world,result)!=ik::Status::okay)return false;
        records_=0;
        for(const auto i:{chain.upper,chain.lower,chain.end}){if(!append(s.metadata.local.data+i*32,s.local[i].q,w.solved[i].q,16,true))return false;
            if(params_.adaptive&&i!=chain.upper&&!append(s.metadata.local.data+i*32+16,s.local[i].translation,w.solved[i].translation,12,true))return false;}
        std::memcpy(w.packed,s.space.data(),count_*64);
        for(unsigned i=static_cast<unsigned>(chain.upper);i<count_;++i){unsigned ancestor=i;while(ancestor>chain.upper)ancestor=static_cast<unsigned>(w.parents[ancestor]);if(ancestor!=chain.upper)continue;
            for(unsigned j=0;j<16;++j){const float v=static_cast<float>(w.space[i].m[j]);if(!std::isfinite(v))return false;w.packed[i].values[j]=v;}
            if(!append(s.metadata.space.data+i*64,&s.space[i],&w.packed[i],64,false))return false;}
        return true;
    }
public:
    bool start(Params const& p,std::uint64_t nonce) noexcept {
        if(busy_||armed_||debt_||!p.enabled||!nonce||nonce<=last_nonce_||!p.ticks||p.ticks>16||!ik::detail::finite_vector(p.delta)||
            ik::detail::length(p.delta)>5||!std::isfinite(p.roll)||std::abs(p.roll)>.8||!p.chain.upper||p.chain.upper>=pose::max_bones||
            p.chain.lower>=pose::max_bones||p.chain.end>=pose::max_bones||p.chain.upper==p.chain.lower||p.chain.upper==p.chain.end||p.chain.lower==p.chain.end)return false;
        params_=p;nonce_=last_nonce_=nonce;tick_=call_=0;qpc_=0;bound_=false;records_=0;count_=trial_ticks_=0;stats_={};stage_=Stage::c0;
        armed_=true;native_c1_=false;stop_.store(false);return true;
    }
    void request_stop() noexcept {stop_.store(true);}
    bool pending() const noexcept {return debt_;} // GT/owner-only; stop is the sole cross-thread method.
    Stats stats() const noexcept {return stats_;}
    static constexpr bool physics_outcome_proven=false;
    static constexpr bool global_exclusivity_proven=false;
private:
    template<class IO> Status step(Input const& in,IO& io,Workspace& w) noexcept {
        if(!armed_)return Status::disabled;
        if(!proof(in,io,w))return fail(Status::context);
        const auto& s=*in.snapshot;const auto& p=in.scope;
        if(p.phase==pose::Phase::C0){
            if((stage_!=Stage::c0&&stage_!=Stage::blocked)||p.tick<=tick_)return fail(Status::sequence);
            if(!bound_){binding_=p;metadata_=s.metadata;reference_=s.reference;bound_=true;}
            tick_=p.tick;call_=p.call;qpc_=s.request.qpc;
            std::memcpy(c0_input_.data(),s.local.data(),count_*32);tick_scope_=p;
            // A failed/partial C0 return may mix locals with old Space. It is
            // fatal+pinned, not ordinary conflict; only a fresh C0 may recover.
            if(!restore_locals(in,io,w))return fail(Status::conflict,true);
            if(!metadata_current(in,io))return fail(Status::context,debt_);
            metadata_=s.metadata;native_c1_=false;stage_=Stage::c1;
            return debt_?Status::restored:Status::okay;
        }
        if(p.tick!=tick_||!detail::owner(tick_scope_.owner,p.owner,true)||std::memcmp(&s.metadata.component,&metadata_.component,sizeof(pose::Matrix)))return fail(Status::context);
        if(p.phase==pose::Phase::C1){
            if(stage_!=Stage::c1||p.call!=call_)return fail(Status::sequence);
            if(!native_replaced_space(s)||!prepare(s,w))return fail(Status::context);
            for(unsigned i=0;i<count_;++i)if(std::memcmp(&c0_input_[i],&s.local[i],32))++stats_.c0_c1_changed_atoms;
            std::memcpy(native_local_.data(),s.local.data(),count_*32);std::memcpy(native_space_.data(),s.space.data(),count_*64);
            native_c1_=true;++stats_.native_c1;qpc_=s.request.qpc;stage_=Stage::l1;
            if(stop_.load())return Status::okay;
            if(!plan(s,w))return fail(Status::rejected);
            const bool last=++trial_ticks_>=params_.ticks;
            for(unsigned i=0;i<records_;++i)if(!io.writable(journal_[i].address,journal_[i].size))return fail(Status::write_failed);
            if(!records_){if(last)request_stop();return Status::okay;}
            bool okay=true;debt_=true;native_c1_=false;
            for(unsigned i=0;i<records_;++i){auto& r=journal_[i];r.active=true;
                if(!metadata_current(in,io)||!io.write(r.address,r.after,r.size)||!io.read(w.buffer,r.address,r.size)||std::memcmp(w.buffer,r.after,r.size)||stop_.load()){okay=false;break;}}
            detail::Reader<IO> read{io};okay=okay&&metadata_current(in,io)&&pose::repeat(read,w.solved,s.metadata.local.data,count_*32)==pose::Reject::none&&
                pose::repeat(read,w.packed,s.metadata.space.data,count_*64)==pose::Reject::none&&!stop_.load();
            if(!okay){request_stop();if(!rollback(in,io,w)){stage_=Stage::blocked;return fail(Status::rollback_failed);}
                native_c1_=metadata_current(in,io);stage_=Stage::l1;return fail(Status::write_failed);}
            ++stats_.applied_ticks;if(last)request_stop();return Status::applied;
        }
        if(stage_!=Stage::l1)return fail(Status::sequence);
        for(unsigned i=0;i<records_;++i){const auto& r=journal_[i];if(r.local&&r.active&&(!io.read(w.buffer,r.address,r.size)||std::memcmp(w.buffer,r.after,r.size)))return fail(Status::conflict);}
        qpc_=s.request.qpc;stage_=Stage::c0;
        if(stop_.load()&&native_c1_){
            for(unsigned i=0;i<records_;++i)if(journal_[i].active)return fail(Status::conflict);
            // L1 is the same authenticated native pipeline, not another C0.
            // Later native changes are recorded/preserved, never rewritten.
            for(unsigned i=0;i<count_;++i){if(std::memcmp(&native_local_[i],&s.local[i],32))++stats_.c1_l1_changed_atoms;
                if(std::memcmp(&native_space_[i],&s.space[i],64))++stats_.c1_l1_changed_space;}
            ++stats_.native_l1;debt_=false;armed_=false;return Status::drained;}
        return Status::okay;
    }
public:
    template<class IO> Status feed(Input const& in,IO& io,Workspace& w) noexcept {
        if(busy_)return fail(Status::sequence);
        busy_=true;const Status result=step(in,io,w);busy_=false;return result;
    }
};
} // namespace gears_cover_pose_overlay_trial
