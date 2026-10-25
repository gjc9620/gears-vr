#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <intrin.h>
#define GEARS_FULL_BODY_C0_TRANSACTION_HOST_TEST 1
#include "full_body_c0_transaction.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
namespace gears_full_body_c0_transaction {struct HostLeaseSeam {
static void copied_record(Lease& copy,Lease const& real) noexcept {copy.self_=real.self_;copy.kernel_=real.kernel_;copy.serial_=real.serial_;copy.closed_=real.closed_;}
};}
namespace tx=gears_full_body_c0_transaction;namespace body=tx::body;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#x);}}while(0)
bool hash(void const* bytes,std::size_t size,body::rig::Signature& out){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE h{};DWORD needed{},got{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    bool good=BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&needed),sizeof needed,&got,0)>=0;
    std::vector<unsigned char> object(needed);if(good)good=BCryptCreateHash(algorithm,&h,object.data(),needed,nullptr,0,0)>=0;
    if(good)good=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;
    if(good)good=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(algorithm,0);return good;
}
bool read(std::string const& path,void* data,std::size_t size){std::ifstream f(path,std::ios::binary);return bool(f.read(static_cast<char*>(data),std::streamsize(size)))&&f.peek()==EOF;}
__declspec(noinline) tx::Cas actual_cas(std::uintptr_t address,tx::Block const& before,tx::Block const& after) noexcept {
    alignas(16) __int64 comparand[2];std::memcpy(comparand,&before,16);
    __try{return _InterlockedCompareExchange128(reinterpret_cast<volatile __int64*>(address),__int64(after.high),__int64(after.low),comparand)?tx::Cas::exchanged:tx::Cas::mismatch;}
    __except(EXCEPTION_EXECUTE_HANDLER){return tx::Cas::fault_unknown;}
}
enum class Inject {none,mismatch,fault_before,fault_after,real_access_fault,source_age};
struct Host final:tx::Backend {
    body::Frame const& source;body::Output const& planned;
    body::Atom* memory{};tx::Stamp current{};tx::Desired desired{};
    tx::Stage stage{};unsigned index{},forward{},backward{},cas_calls{},guard_calls{};
    int fail_forward{-1},fail_backward{-1},metadata_at{-1},stop_after{-1},age_validate_at{-1};
    Inject forward_fault{Inject::none},backward_fault{Inject::none};
    tx::Kernel* stop_kernel{};bool deny{},deny_rollback{},deny_end{};unsigned actual_calls{};
    std::vector<unsigned> forward_bits,rollback_bits;tx::FreshEvidence produced{};bool have_receipt{};
    Host(body::Frame const& f,body::Output const& p):source(f),planned(p){
        memory=static_cast<body::Atom*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!memory)std::terminate();
        std::memcpy(memory,source.local.data(),sizeof source.local);
        current.binding={11,12,13,14,15,reinterpret_cast<std::uintptr_t>(memory),115,115};current.frame=21;current.producer=31;current.scope=41;
        desired.count=115;for(unsigned i=0;i<115;++i)desired.indices[i]=static_cast<unsigned char>(i);
    }
    ~Host(){VirtualFree(memory,0,MEM_RELEASE);}
    std::uint64_t thread_token() const noexcept override{return GetCurrentThreadId();}
    tx::Verdict validate(tx::View const& v,tx::Stage s,unsigned i) noexcept override {
        stage=s;index=i;++guard_calls;
        if(s==tx::Stage::commit&&int(i)==metadata_at)++current.binding.metadata_generation;
        if(deny||(s==tx::Stage::rollback&&deny_rollback)||(s==tx::Stage::scope_end&&deny_end)||!tx::same(v.stamp,current)||v.desired.count!=desired.count||v.desired.indices!=desired.indices)return tx::Verdict::denied;
        if(std::memcmp(v.source.local.data(),source.local.data(),sizeof source.local)||std::memcmp(v.source.reference.data(),source.reference.data(),sizeof source.reference)||
           std::memcmp(v.plan.local.data(),planned.local.data(),sizeof planned.local))return tx::Verdict::denied;
        // Rollback permits unknown data elsewhere but retains exact scope/header
        // validation. CAS itself is the only authority to replace each ours.
        // Scope end must not demand pre-Compose data after native consumption.
        if(s==tx::Stage::rollback||s==tx::Stage::scope_end)return tx::Verdict::verified;
        for(unsigned bone=0;bone<115;++bone)for(unsigned half=0;half<2;++half){
            void const* expected=reinterpret_cast<unsigned char const*>(&source.local[bone])+half*16;bool unknown=false;
            for(unsigned n=0;n<v.count;++n)if(v.units[n].bone==bone&&v.units[n].half==half){
                if(v.units[n].state==tx::UnitState::applied)expected=&v.units[n].ours;
                unknown=v.units[n].state==tx::UnitState::uncertain||v.units[n].state==tx::UnitState::conflict;break;}
            if(!unknown&&std::memcmp(reinterpret_cast<unsigned char const*>(&memory[bone])+half*16,expected,16))return tx::Verdict::denied;
        }
        if(s==tx::Stage::commit&&int(i)==age_validate_at)return tx::Verdict::source_age_expired;
        return tx::Verdict::verified;
    }
    tx::Cas exchange(std::uintptr_t address,tx::Block const& before,tx::Block const& after) noexcept override {
        ++cas_calls;const unsigned bit=unsigned((address-current.binding.local)/16);Inject injection=Inject::none;
        if(stage==tx::Stage::rollback){rollback_bits.push_back(bit);if(int(backward)==fail_backward)injection=backward_fault;++backward;}
        else{forward_bits.push_back(bit);if(int(forward)==fail_forward)injection=forward_fault;++forward;}
        if(injection==Inject::mismatch){tx::Block foreign=before;foreign.low^=0x55aa55aa55aa55aaULL;C(actual_cas(address,before,foreign)==tx::Cas::exchanged);}
        if(injection==Inject::source_age)return tx::Cas::source_age_no_write;
        if(injection==Inject::fault_before)return tx::Cas::fault_unknown;
        if(injection==Inject::real_access_fault){DWORD prior{},ignored{};C(VirtualProtect(memory,4096,PAGE_NOACCESS,&prior)!=FALSE);auto r=actual_cas(address,before,after);C(VirtualProtect(memory,4096,prior,&ignored)!=FALSE);C(r==tx::Cas::fault_unknown);return r;}
        ++actual_calls;const auto answer=actual_cas(address,before,after);
        if(stage==tx::Stage::commit&&int(forward-1)==stop_after&&stop_kernel)stop_kernel->request_stop();
        return injection==Inject::fault_after?tx::Cas::fault_unknown:answer;
    }
    tx::Verdict fresh(tx::View const&,tx::ReceiptRef receipt,tx::FreshEvidence& out) noexcept override {
        if(!have_receipt||!receipt.event||receipt.event!=produced.event)return tx::Verdict::denied;
        out=produced;return tx::Verdict::verified;
    }
    tx::ReceiptRef producer(tx::Mask mask){
        // Synthetic Host producer, real stores to THIS PROCESS buffer. The
        // event is minted only after all selected physical stores complete.
        produced={};produced.written=current;++produced.written.frame;++produced.written.producer;++produced.written.scope;
        for(unsigned bit=0;bit<230;++bit)if(tx::contains(mask,bit)){tx::Block old{},native{};const auto at=reinterpret_cast<std::uintptr_t>(memory)+bit*16;
            std::memcpy(&old,reinterpret_cast<void const*>(at),16);std::memcpy(&native,reinterpret_cast<unsigned char const*>(source.local.data())+bit*16,16);
            C(actual_cas(at,old,native)==tx::Cas::exchanged);tx::include(produced.actual_written,bit);}
        static std::uint64_t next=100;produced.event=++next;have_receipt=true;return {produced.event};
    }
    bool original() const{return std::memcmp(memory,source.local.data(),sizeof source.local)==0;}
};
tx::Mask whole(){tx::Mask m{};for(unsigned i=0;i<230;++i)tx::include(m,i);return m;}
std::vector<unsigned> changed(body::Frame const& f,body::Output const& p){std::vector<unsigned> result;
    for(unsigned bit=0;bit<230;++bit)if(std::memcmp(reinterpret_cast<unsigned char const*>(f.local.data())+bit*16,reinterpret_cast<unsigned char const*>(p.local.data())+bit*16,16))result.push_back(bit);return result;}
void reverse_is(std::vector<unsigned> const& actual,std::vector<unsigned> const& bits,unsigned count){C(actual.size()==count);for(unsigned i=0;i<count&&i<actual.size();++i)C(actual[i]==bits[count-i-1]);}
int main(int argc,char** argv){
    if(argc!=2)return 2;const std::string folder=argv[1];auto f=std::make_unique<body::Frame>();auto workspace=std::make_unique<body::Workspace>();body::Model model{};body::Output plan{};
    C(read(folder+"/reference.bin",f->reference.data(),sizeof f->reference));C(read(folder+"/C0-local.bin",f->local.data(),sizeof f->local));C(read(folder+"/C0-component.bin",&f->component_to_world,sizeof f->component_to_world));C(body::prepare_model(f->reference,hash,model));
    for(unsigned i=0;i<16;++i)workspace->component.m[i]=f->component_to_world.values[i];std::copy(f->local.begin(),f->local.end(),workspace->pose.local);C(body::d::rebuild(model,*workspace));
    body::Goal goal{};goal.scope=body::Scope::fixed_cover;goal.body_yaw_delta=-.1;goal.physical_world={1,2,-3};goal.committed_grip_world={2,1,-2};
    goal.head_world=body::d::pose(workspace->pose.world[55]);goal.head_world.position=body::d::add(goal.head_world.position,{3,3,-5});for(unsigned i=0;i<2;++i)goal.feet_world[i]=body::d::pose(workspace->pose.world[body::legs[i][2]]);
    C(body::plan(model,*f,goal,*workspace,plan).status==body::Status::solved);const auto bits=changed(*f,plan);C(bits.size()>8&&bits.size()<=tx::max_units&&bits[0]==0&&bits[1]==1);const unsigned count=unsigned(bits.size());
    std::array<unsigned char,sizeof(body::Output)> old_plan{};std::memcpy(old_plan.data(),&plan,sizeof plan);const auto old_source=f->local;
    {tx::Kernel k;tx::Lease l;tx::NativeBackendUnavailable native;tx::Stamp key{};tx::Desired d{};C(k.prepare(native,key,*f,plan,d,l).code==tx::Code::unavailable);C(!k.pending());}
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);C(h.cas_calls==0);C(k.commit(h,l,h.current).code==tx::Code::committed);C(std::memcmp(h.memory,plan.local.data(),sizeof plan.local)==0);C(k.outstanding_fields()==count);C(k.rollback(h,l,h.current).code==tx::Code::rolled_back);C(h.original());reverse_is(h.rollback_bits,bits,count);C(k.finish(h,l,h.current,tx::End::before_consumer).pending==0);}
    for(unsigned n=0;n<count;++n)for(auto injection:{Inject::mismatch,Inject::fault_before,Inject::fault_after}){
        Host h(*f,plan);tx::Kernel k;tx::Lease l;h.fail_forward=int(n);h.forward_fault=injection;
        C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);auto r=k.commit(h,l,h.current);C(r.code==(injection==Inject::mismatch?tx::Code::compare_mismatch:tx::Code::cas_fault));
        C(k.failure()==tx::Failure::hard&&!k.settled_source_age());
        reverse_is(h.rollback_bits,bits,injection==Inject::mismatch?n:n+1);
        C(k.outstanding_fields()==unsigned(injection==Inject::fault_before));
        for(unsigned bit=0;bit<230;++bit){const bool equal=std::memcmp(reinterpret_cast<unsigned char const*>(h.memory)+bit*16,reinterpret_cast<unsigned char const*>(f->local.data())+bit*16,16)==0;C(equal==(injection!=Inject::mismatch||bit!=bits[n]));}
        k.finish(h,l,h.current,tx::End::before_consumer);C(k.pending()==unsigned(injection==Inject::fault_before));
        if(injection==Inject::fault_before){C(h.original());C(k.retire_fresh(h,{999999}).code==tx::Code::bad_receipt);C(k.pending()==1);auto receipt=h.producer(whole());C(k.retire_fresh(h,receipt).code==tx::Code::fresh_retired);C(!k.pending());}
    }
    // Deadline rejection is recoverable only once the exact scope is closed
    // and all prior writes have actually been restored. The failing unit is
    // known unwritten only for the dedicated no-CAS exchange result.
    for(unsigned at:{0u,3u})for(bool exchange:{false,true}){
        Host h(*f,plan);tx::Kernel k(70);tx::Lease l;
        if(exchange){h.fail_forward=int(at);h.forward_fault=Inject::source_age;}else h.age_validate_at=int(at);
        C(k.serial()==70);C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);C(k.serial()==71);
        const auto r=k.commit(h,l,h.current);C(r.code==tx::Code::guard_rejected&&r.unit==at);
        C(k.failure()==tx::Failure::source_age_only&&k.failed()&&k.stopping());
        C(!k.settled_source_age()&&k.outstanding_fields()==0&&k.pending()==1&&h.original());
        C(h.forward==at+unsigned(exchange)&&h.actual_calls==at*2);reverse_is(h.rollback_bits,bits,at);
        C(k.finish(h,l,h.current,at?tx::End::original_returned:tx::End::before_consumer).code==tx::Code::scope_closed);
        C(k.pending()==0&&k.settled_source_age()&&k.failure()==tx::Failure::source_age_only&&h.original());
        tx::Lease fresh;C(k.prepare(h,h.current,*f,plan,h.desired,fresh).code==tx::Code::stopped);
        k.request_stop();C(k.failure()==tx::Failure::hard&&!k.settled_source_age());
    }
    // A source deadline cannot excuse later hard refusal, CAS uncertainty,
    // wrong scope, an explicit stop, or an original-consumer exception.
    for(unsigned hard=0;hard<7;++hard){
        Host h(*f,plan);tx::Kernel k;tx::Lease l;h.fail_forward=3;h.forward_fault=Inject::source_age;
        if(hard==0)h.deny_rollback=true;
        if(hard==1){h.fail_backward=0;h.backward_fault=Inject::mismatch;}
        if(hard==2){h.fail_backward=0;h.backward_fault=Inject::fault_before;}
        C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);
        C(k.commit(h,l,h.current).code==tx::Code::guard_rejected);C(!k.settled_source_age());
        if(hard<3){C(k.failure()==tx::Failure::hard&&k.outstanding_fields()>0);}
        else C(k.failure()==tx::Failure::source_age_only&&h.original());
        auto key=h.current;auto ending=tx::End::before_consumer;
        if(hard==3)h.deny_end=true;
        if(hard==4)k.request_stop();
        if(hard==5)++key.scope;
        if(hard==6)ending=tx::End::original_threw;
        k.finish(h,l,key,ending);C(k.failure()==tx::Failure::hard&&!k.settled_source_age());
    }
    std::printf("forward every-unit mismatch/fault-before/fault-after: %u units\n",count);
    for(unsigned n=0;n<count;++n)for(auto injection:{Inject::mismatch,Inject::fault_before,Inject::fault_after}){
        Host h(*f,plan);tx::Kernel k;tx::Lease l;h.fail_backward=int(n);h.backward_fault=injection;
        C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);C(k.commit(h,l,h.current).code==tx::Code::committed);C(k.rollback(h,l,h.current).code==tx::Code::rollback_pending);reverse_is(h.rollback_bits,bits,count);C(k.outstanding_fields()==1);
        // Preserve the unresolved unit through scope close; no guessed success
        // from equal before data (fault-after) or unknown concurrent data.
        h.deny=true;C(k.finish(h,l,h.current,tx::End::before_consumer).code==tx::Code::guard_rejected);C(k.pending()>=2);h.deny=false;
        C(k.notify_consumer_entry(h,l,h.current).code==tx::Code::consumer_marked||k.consumed());k.finish(h,l,h.current,tx::End::before_consumer);C(k.outstanding_fields()==1);
        const unsigned before=h.cas_calls;C(k.retire_fresh(h,{888}).code==tx::Code::bad_receipt);C(h.cas_calls==before);
        auto receipt=h.producer(whole());C(k.retire_fresh(h,receipt).code==tx::Code::fresh_retired);C(k.outstanding_fields()==0);C(k.pending()>0); // exact bad-end protocol debt is retained
    }
    std::printf("reverse every-unit mismatch/fault-before/fault-after: %u units\n",count);
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;h.fail_forward=2;h.forward_fault=Inject::real_access_fault;C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);C(k.commit(h,l,h.current).code==tx::Code::cas_fault);C(k.outstanding_fields()==1&&h.original());k.notify_consumer_entry(h,l,h.current);k.finish(h,l,h.current,tx::End::original_threw);C(k.original_exception());auto receipt=h.producer(whole());C(k.retire_fresh(h,receipt).code==tx::Code::fresh_retired);C(!k.pending());}
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;const auto original=h.current;C(k.prepare(h,original,*f,plan,h.desired,l).code==tx::Code::prepared);C(k.commit(h,l,original).code==tx::Code::committed);C(k.notify_consumer_entry(h,l,original).code==tx::Code::consumer_marked);const auto calls=h.cas_calls;C(k.rollback(h,l,original).code==tx::Code::already_consumed);C(h.cas_calls==calls);k.request_stop();C(k.finish(h,l,original,tx::End::original_threw).code==tx::Code::scope_closed);C(k.original_exception()&&k.pending()==count);
        // Mere equality plus a newer clock/frame does not retire anything.
        std::memcpy(h.memory,f->local.data(),sizeof f->local);C(k.retire_fresh(h,{1}).code==tx::Code::bad_receipt);C(k.pending()==count);
        auto partial=whole();partial.words[0]&=~std::uint64_t(1);auto ref=h.producer(partial);C(k.retire_fresh(h,ref).code==tx::Code::bad_receipt);C(k.pending()==count);
        ref=h.producer(whole());const auto proper=h.produced;++h.produced.written.binding.owner;C(k.retire_fresh(h,ref).code==tx::Code::bad_receipt);h.produced=proper;++h.produced.written.binding.metadata_generation;C(k.retire_fresh(h,ref).code==tx::Code::bad_receipt);h.produced=proper;h.produced.written.producer=original.producer;C(k.retire_fresh(h,ref).code==tx::Code::bad_receipt);h.produced=proper;
        C(k.retire_fresh(h,ref).code==tx::Code::fresh_retired);C(!k.pending());C(k.retire_fresh(h,ref).code==tx::Code::bad_receipt);C(h.cas_calls==calls);
    }
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);C(k.commit(h,l,h.current).code==tx::Code::committed);k.notify_consumer_entry(h,l,h.current);k.finish(h,l,h.current,tx::End::before_consumer);auto receipt=h.producer(whole());C(k.retire_fresh(h,receipt).code==tx::Code::fresh_retired);h.current=h.produced.written;tx::Lease next;C(k.prepare(h,h.current,*f,plan,h.desired,next).code==tx::Code::prepared);C(k.commit(h,next,h.current).code==tx::Code::committed);k.notify_consumer_entry(h,next,h.current);k.finish(h,next,h.current,tx::End::before_consumer);C(k.retire_fresh(h,receipt).code==tx::Code::bad_receipt);C(k.pending()==count);receipt=h.producer(whole());C(k.retire_fresh(h,receipt).code==tx::Code::fresh_retired);}
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;h.stop_after=0;h.stop_kernel=&k;C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);C(k.commit(h,l,h.current).code==tx::Code::guard_rejected);C(h.original());C(k.finish(h,l,h.current,tx::End::before_consumer).pending==0);tx::Lease next;C(k.prepare(h,h.current,*f,plan,h.desired,next).code==tx::Code::stopped);}
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;const auto exact=h.current;h.metadata_at=2;C(k.prepare(h,exact,*f,plan,h.desired,l).code==tx::Code::prepared);C(k.commit(h,l,exact).code==tx::Code::guard_rejected);C(k.outstanding_fields()==2);C(k.finish(h,l,exact,tx::End::before_consumer).code==tx::Code::guard_rejected);auto receipt=h.producer(whole());C(k.retire_fresh(h,receipt).code==tx::Code::bad_receipt);C(k.pending()>0);}
    {Host h(*f,plan);tx::Kernel k;tx::Lease real,copy;static_assert(!std::is_copy_constructible<tx::Lease>::value);C(k.prepare(h,h.current,*f,plan,h.desired,real).code==tx::Code::prepared);C(k.commit(h,real,h.current).code==tx::Code::committed);tx::HostLeaseSeam::copied_record(copy,real);C(k.finish(h,copy,h.current,tx::End::before_consumer).code==tx::Code::wrong_lease);C(k.outstanding_fields()==count);C(k.finish(h,real,h.current,tx::End::before_consumer).code==tx::Code::scope_closed);C(h.original()&&k.pending()==1);C(k.finish(h,real,h.current,tx::End::before_consumer).code==tx::Code::wrong_lease);C(k.pending()==2);}
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);auto wrong=h.current;++wrong.binding.owner;C(k.finish(h,l,wrong,tx::End::before_consumer).code==tx::Code::wrong_lease);C(k.pending()==2);k.finish(h,l,h.current,tx::End::before_consumer);C(k.pending()==1);}
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);tx::Result r{};std::thread other([&]{r=k.finish(h,l,h.current,tx::End::before_consumer);});other.join();C(r.code==tx::Code::wrong_thread);C(k.pending()==2);k.finish(h,l,h.current,tx::End::before_consumer);C(k.pending()==1);}
    // P1 regression: even a rejected notification must latch consumption before
    // the unconditional original-call path and the later REAL scope finally.
    for(unsigned wrong=0;wrong<2;++wrong){Host h(*f,plan);tx::Kernel k;tx::Lease real,other;const auto exact=h.current;C(k.prepare(h,exact,*f,plan,h.desired,real).code==tx::Code::prepared);C(k.commit(h,real,exact).code==tx::Code::committed);
        auto key=exact;if(wrong==0)++key.binding.owner;const auto r=k.notify_consumer_entry(h,wrong?other:real,key);C(r.code==tx::Code::wrong_lease&&k.consumed());
        std::array<body::Atom,115> original_consumed{};std::memcpy(original_consumed.data(),h.memory,sizeof original_consumed);const unsigned calls=h.cas_calls;
        C(k.finish(h,real,exact,tx::End::original_returned).code==tx::Code::scope_closed);C(h.cas_calls==calls&&h.rollback_bits.empty());C(!std::memcmp(original_consumed.data(),h.memory,sizeof original_consumed));C(k.outstanding_fields()==count&&k.protocol_debt()==1);
        auto receipt=h.producer(whole());C(k.retire_fresh(h,receipt).code==tx::Code::fresh_retired);C(k.pending()==1);}
    // A real exception finally must also fail closed if entry notification was
    // accidentally omitted. This fallback does not replace the entry contract.
    {Host h(*f,plan);tx::Kernel k;tx::Lease l;C(k.prepare(h,h.current,*f,plan,h.desired,l).code==tx::Code::prepared);C(k.commit(h,l,h.current).code==tx::Code::committed);const auto calls=h.cas_calls;
        try{throw 7;}catch(...){C(k.finish(h,l,h.current,tx::End::original_threw).code==tx::Code::scope_closed);}C(k.consumed()&&k.original_exception()&&h.cas_calls==calls&&k.outstanding_fields()==count);auto receipt=h.producer(whole());C(k.retire_fresh(h,receipt).code==tx::Code::fresh_retired);C(!k.pending());}
    for(unsigned bad=0;bad<12;++bad){auto modified=plan;auto frame=*f;Host h(*f,plan);auto key=h.current;auto desired=h.desired;tx::Kernel k;tx::Lease l;
        if(bad==0)modified.changed_mask[0]^=1;if(bad==1)modified.local[2].translation[0]+=.1f;if(bad==2)modified.local[0].scale=2;if(bad==3)modified.local[4].q[0]=NAN;if(bad==4)key.binding.local+=8;if(bad==5){desired.count=114;for(unsigned i=0;i<114;++i)desired.indices[i]=static_cast<unsigned char>(i+1);}if(bad==6)modified.report.status=body::Status::residual;if(bad==7)frame.component_to_world.values[15]=0;
        if(bad==8)modified.local[0].translation[0]+=100;if(bad==9)modified.head_world.position.x+=1;if(bad==10)modified.report.max_length_error=0;
        if(bad==11)key.binding.local=reinterpret_cast<std::uintptr_t>(&k);
        C(k.prepare(h,key,frame,modified,desired,l).code==tx::Code::invalid_plan);C(h.cas_calls==0&&h.original()&&!k.pending());}
    C(!std::memcmp(old_plan.data(),&plan,sizeof plan));C(!std::memcmp(old_source.data(),f->local.data(),sizeof f->local));
    std::printf("{\"Block\":%zu,\"BlockAlign\":%zu,\"Unit\":%zu,\"Binding\":%zu,\"Stamp\":%zu,\"Desired\":%zu,\"FreshEvidence\":%zu,\"Lease\":%zu,\"Kernel\":%zu,\"actual_changed_units\":%u}\n",sizeof(tx::Block),alignof(tx::Block),sizeof(tx::Unit),sizeof(tx::Binding),sizeof(tx::Stamp),sizeof(tx::Desired),sizeof(tx::FreshEvidence),sizeof(tx::Lease),sizeof(tx::Kernel),count);
    std::printf("full_body_c0_transaction: %u checks, %u failures; real_CAS128=1 actual_SEH=1 Host_only=1 native_gate=unimplemented\n",checks,failures);return failures?1:0;
}
