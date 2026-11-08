#include "weapon_hand_broker.h"
#include <cstdio>
#include <thread>
namespace w=gears_weapon_hand;
namespace {
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
void configure_main_test_backend(w::BrokerBackend&) noexcept;
void configure_visual_test_backend(w::BrokerBackend&) noexcept;
void configure_body_mount_test_backend(w::BrokerBackend&) noexcept;
thread_local std::uint32_t test_thread=1;
constexpr std::uint32_t worker=1,gt=2;
enum class Mutate {none,frame,proposal,decision,key,nonce,candidate,cover,duplicate,reserved,size,version,reenter,fault};
struct Fake {
    w::Broker* broker{};
    std::atomic<std::int64_t> stamp{1000000000};
    std::atomic<unsigned> pins{},unpins{},prepares{},commits{},invalidations{},violations{};
    std::atomic<bool> publishing{},staged{},block_invalidation{};
    bool pin_ok=true,pin_on_failure{},unpin_ok=true,unpin_reenter{};
    w::Reply prepare_reply=w::Reply::accepted,commit_reply=w::Reply::accepted;
    Mutate prepare_mutation{},commit_mutation{},invalidate_mutation{};
    w::Lease incumbent{};
    bool magazine_candidate{},magazine_pending{};
    w::Reply reentered=w::Reply::invalid;
    w::Frame observed_frame{};w::Proposal observed_proposal{};w::Decision observed_decision{};
    w::Invalidation observed_invalidation{};
    HANDLE entered{},resume{},invalidate_entered{},invalidate_resume{};unsigned block{};
    void barrier(unsigned phase) noexcept {
        if(block==phase){SetEvent(entered);if(WaitForSingleObject(resume,3000)!=WAIT_OBJECT_0)++violations;}
    }
    static bool pin(void* context,w::Registration const&,w::ModulePin& out) noexcept {
        auto& f=*static_cast<Fake*>(context);++f.pins;f.barrier(3);
        if(f.pin_ok||f.pin_on_failure)out={0x8000,0x8000};return f.pin_ok;
    }
    static bool unpin(void* context,w::ModulePin pin_value) noexcept {
        auto& f=*static_cast<Fake*>(context);++f.unpins;
        if(pin_value.handle!=0x8000||test_thread!=worker||f.broker->pending_callbacks()||f.broker->pending_transactions()||f.publishing||f.staged)++f.violations;
        if(f.unpin_reenter){w::Registration r{};r.context=&f;r.prepare=prepare;r.commit=commit;r.invalidate=invalidate;
            w::RegistrationResult result{};f.reentered=f.broker->register_participant(&r,&result);}
        return f.unpin_ok;
    }
    static std::int64_t clock(void* context) noexcept {return static_cast<Fake*>(context)->stamp.load();}
    static std::uint32_t thread(void*) noexcept {return test_thread;}
    static std::uint32_t prepare(void* context,w::Frame const* frame,w::Proposal* proposal) noexcept {
        auto& f=*static_cast<Fake*>(context);++f.prepares;f.observed_frame=*frame;
        if(!f.broker->pending_callbacks()||!f.broker->pending_transactions())++f.violations;
        *proposal={};proposal->key=frame->key;proposal->nonce=frame->key.frame*17+1;proposal->support=1;proposal->incumbents[0]=f.incumbent;
        proposal->magazine=f.magazine_candidate?1u:0u;
        if(f.magazine_pending){proposal->support=0;proposal->magazine_observation_pending=1;}
        switch(f.prepare_mutation){
        case Mutate::frame:const_cast<w::Frame*>(frame)->pawn.address+=8;break;
        case Mutate::key:++proposal->key.frame;break;
        case Mutate::nonce:proposal->nonce=0;break;
        case Mutate::candidate:proposal->support=2;break;
        case Mutate::cover:proposal->incumbents[0]={w::LeaseKind::cover,0,frame->key.lifecycle,9};break;
        case Mutate::duplicate:proposal->incumbents[0]=proposal->incumbents[1]={w::LeaseKind::support,0,frame->key.lifecycle,9};break;
        case Mutate::reserved:proposal->magazine_observation_pending=2;break;
        case Mutate::size:--proposal->size;break;
        case Mutate::version:++proposal->abi_version;break;
        case Mutate::reenter:f.reentered=f.broker->unregister(frame->key.registration_cookie);break;
        case Mutate::fault:RaiseException(0xe042c020,0,0,nullptr);break;
        default:break;
        }
        f.observed_proposal=*proposal;f.barrier(1);f.staged=true;return unsigned(f.prepare_reply);
    }
    static std::uint32_t commit(void* context,w::Frame const* frame,w::Proposal const* proposal,w::Decision const* decision) noexcept {
        auto& f=*static_cast<Fake*>(context);++f.commits;f.observed_frame=*frame;f.observed_proposal=*proposal;f.observed_decision=*decision;
        if(!f.broker->pending_callbacks()||!f.broker->pending_transactions())++f.violations;
        if(f.commit_mutation==Mutate::frame)++const_cast<w::Frame*>(frame)->press_token;
        if(f.commit_mutation==Mutate::proposal)++const_cast<w::Proposal*>(proposal)->nonce;
        if(f.commit_mutation==Mutate::decision)++const_cast<w::Decision*>(decision)->press_token;
        if(f.commit_mutation==Mutate::fault)RaiseException(0xe042c021,0,0,nullptr);
        if(f.commit_mutation==Mutate::reenter)f.reentered=f.broker->collect_on_owner_worker();
        f.barrier(2);f.publishing=decision->lease.kind!=w::LeaseKind::none;f.staged=false;return unsigned(f.commit_reply);
    }
    static void invalidate(void* context,w::Invalidation const* value) noexcept {
        auto& f=*static_cast<Fake*>(context);++f.invalidations;f.observed_invalidation=*value;
        if(f.broker->authorized()||!f.broker->pending_callbacks())++f.violations;
        f.publishing=false;f.staged=false;
        if(f.block_invalidation.exchange(false)){SetEvent(f.invalidate_entered);
            if(WaitForSingleObject(f.invalidate_resume,3000)!=WAIT_OBJECT_0)++f.violations;}
        if(f.invalidate_mutation==Mutate::frame)++const_cast<w::Invalidation*>(value)->registration_cookie;
        if(f.invalidate_mutation==Mutate::fault)RaiseException(0xe042c022,0,0,nullptr);
        if(f.invalidate_mutation==Mutate::reenter)f.reentered=f.broker->unregister(value->registration_cookie);
    }
    w::BrokerBackend backend() noexcept {
        w::BrokerBackend b{this,pin,unpin,clock,thread,w::weapon_broker_win32::prepare,w::weapon_broker_win32::commit,w::weapon_broker_win32::invalidate};
        configure_main_test_backend(b);configure_visual_test_backend(b);configure_body_mount_test_backend(b);return b;
    }
    void start_barrier(unsigned phase){entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);block=phase;C(entered&&resume);}
    void end_barrier(){CloseHandle(entered);CloseHandle(resume);entered=resume=nullptr;block=0;}
};
struct Fixture {
    Fake fake;w::Broker broker{{42,worker,400,500},fake.backend()};w::Registration registration{};w::RegistrationResult result{};
    Fixture(){test_thread=worker;fake.broker=&broker;registration.context=&fake;registration.prepare=Fake::prepare;registration.commit=Fake::commit;registration.invalidate=Fake::invalidate;}
    void attach(){C(broker.register_participant(&registration,&result)==w::Reply::accepted);C(result.cookie&&result.native_started_qpc==500);}
    w::Frame frame(std::uint64_t number=1,std::uint64_t lifecycle=1){
        test_thread=gt;w::Frame f{};f.flags=w::scene_owned|w::input_matched|w::gameplay_interactable;
        f.key={400,500,result.cookie,lifecycle,number,6,42,gt};f.now_ns=fake.stamp.load();
        f.world={0x1000,0,0x9000,1,2,0,1};f.controller={0x2000,0x1000,0x9000,2,3,0,1};
        f.pawn={0x3000,0x1000,0x9000,3,4,0,1};f.mesh={0x4000,0x3000,0x9000,4,5,0,1};
        f.reference_generation=1;f.reference={{0,1.6,0},{}};f.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
        auto& s=f.stereo;s.session=4;s.sequence=number;s.space_epoch=1;s.received_ns=f.now_ns;s.predicted_time=f.now_ns+10000;
        s.focused=s.visible=s.should_render=1;s.head_flags=s.view_flags=15;s.head={{0,1.6,0},{}};
        s.eyes[0].pose={{-.032,1.6,0},{}};s.eyes[1].pose={{.032,1.6,0},{}};
        for(auto& eye:s.eyes)eye.fov={-.7,.7,.7,-.7};
        auto& h=f.controllers;h.session=s.session;h.sequence=s.sequence;h.space_epoch=s.space_epoch;h.predicted_time=s.predicted_time;
        h.received_ns=f.now_ns;h.sync_ns=f.now_ns-1000;h.focused=1;
        f.press_token=7;f.left_grip_down=f.left_grip_known=f.left_tracked=1;
        C(w::frame_active(f,f.now_ns));return f;
    }
    w::Decision decision(w::Frame const& frame,w::Proposal const& proposal,bool grant=true){
        w::Decision d{};d.key=frame.key;d.proposal_nonce=proposal.nonce;d.press_token=frame.press_token;d.now_ns=fake.stamp.load();
        if(grant){d.lease={w::LeaseKind::support,0,frame.key.lifecycle,frame.press_token};d.status=w::DecisionStatus::granted;d.new_grant=1;d.release_required=0;}
        else {d.status=w::DecisionStatus::released;d.release_required=0;}
        return d;
    }
    void finish(){
        test_thread=worker;broker.invalidate(w::InvalidateReason::native_stopping);
        C(!broker.pending_transactions());C(broker.quiescent());C(broker.collect_on_owner_worker()==w::Reply::quiescent);
        C(broker.availability()==w::Availability::absent_quiescent&&!broker.pin_retained());C(fake.violations.load()==0);
    }
};

void registration_tests(){
    Fixture f;C(f.broker.availability()==w::Availability::absent_quiescent&&!f.broker.pending());
    C(f.broker.register_participant(nullptr,&f.result)==w::Reply::invalid);
    auto bad=f.registration;bad.frame_bytes=1;C(f.broker.register_participant(&bad,&f.result)==w::Reply::invalid&&f.fake.pins==0);
    bad=f.registration;bad.abi_version=3;C(f.broker.register_participant(&bad,&f.result)==w::Reply::invalid&&f.fake.pins==0);
    bad=f.registration;bad.commit=nullptr;C(f.broker.register_participant(&bad,&f.result)==w::Reply::invalid);
    auto result=f.result;--result.size;C(f.broker.register_participant(&f.registration,&result)==w::Reply::invalid);
    f.fake.pin_ok=false;C(f.broker.register_participant(&f.registration,&f.result)==w::Reply::invalid);
    C(f.broker.availability()==w::Availability::absent_quiescent&&!f.broker.pin_retained());
    f.fake.pin_on_failure=true;C(f.broker.register_participant(&f.registration,&f.result)==w::Reply::invalid);
    C(f.broker.pin_retained()&&f.broker.quiescent()&&f.broker.availability()==w::Availability::unavailable&&f.fake.unpins==0);
    test_thread=gt;C(f.broker.collect_on_owner_worker()==w::Reply::invalid);
    test_thread=worker;C(f.broker.collect_on_owner_worker()==w::Reply::quiescent&&f.fake.unpins==1);
    f.fake.pin_ok=true;f.attach();const auto revision=f.broker.lifecycle_revision(),cookie=f.result.cookie;
    C(f.broker.register_participant(&f.registration,&f.result)==w::Reply::busy);
    C(f.broker.unregister(cookie+99)==w::Reply::wrong_cookie&&f.broker.lifecycle_revision()==revision);
    f.registration.prepare=nullptr; // The registered private copy is immutable.
    auto frame=f.frame();w::Proposal p{};C(f.broker.prepare(frame,p)==w::Reply::accepted&&f.fake.prepares==1);
    C(f.broker.abort_frame(frame.key,p.nonce)==w::Reply::accepted);f.finish();
}

void pending_magazine_tests(){
    for(unsigned defect=0;defect<7;++defect){
        Fixture f;f.attach();f.fake.magazine_pending=true;auto frame=f.frame();
        if(defect==1)f.fake.magazine_candidate=true;
        if(defect==2)f.fake.incumbent={w::LeaseKind::magazine,0,frame.key.lifecycle,frame.press_token};
        if(defect==3)frame.left_grip_down=0;
        if(defect==4)frame.left_grip_known=0;
        if(defect==5)frame.left_tracked=0;
        if(defect==6)f.fake.prepare_mutation=Mutate::version;
        w::Proposal p{};const auto reply=f.broker.prepare(frame,p);
        if(!defect){
            C(reply==w::Reply::accepted&&p.magazine_observation_pending&&!p.support&&!p.magazine&&!p.ammo);
            C(f.broker.abort_frame(frame.key,p.nonce)==w::Reply::accepted);
        }else C(reply==w::Reply::invalid&&!f.broker.authorized()&&!f.broker.pending_transactions());
        f.finish();
    }
}

void normal_and_retry_tests(){
    Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};
    const auto raw=frame;C(f.broker.prepare(frame,p)==w::Reply::accepted);C(std::memcmp(&frame,&raw,sizeof(raw))==0);
    C(f.broker.pending_transactions()==1&&f.broker.pending_callbacks()==0&&f.broker.pin_retained());
    const auto revision=f.broker.lifecycle_revision();auto output=p;f.fake.stamp+=1000;
    C(f.broker.prepare(frame,p)==w::Reply::busy&&f.fake.prepares==1&&f.broker.lifecycle_revision()==revision);
    C(std::memcmp(&p,&output,sizeof(p))==0);
    auto d=f.decision(frame,p);C(f.broker.commit(frame,p,d)==w::Reply::accepted&&f.broker.authorized());
    C(!f.broker.pending_transactions()&&f.fake.commits==1);
    C(f.broker.commit(frame,p,d)==w::Reply::busy&&f.fake.commits==1&&f.broker.lifecycle_revision()==revision);
    auto next=f.frame(2);f.fake.incumbent=d.lease;w::Proposal next_p{};C(f.broker.prepare(next,next_p)==w::Reply::accepted);
    auto retained=f.decision(next,next_p);retained.status=w::DecisionStatus::retained;retained.new_grant=0;
    C(f.broker.commit(next,next_p,retained)==w::Reply::accepted&&f.broker.authorized());
    f.broker.invalidate(w::InvalidateReason::input_lost);const auto loss=f.broker.lifecycle_revision();
    C(loss==revision+1&&!f.broker.authorized()&&!f.broker.draining()&&f.broker.availability()==w::Availability::unavailable);
    f.broker.invalidate(w::InvalidateReason::context_lost);C(f.broker.lifecycle_revision()==loss&&f.fake.invalidations==1);
    next=f.frame(3);next.left_grip_down=0;f.fake.incumbent={};C(f.broker.prepare(next,next_p)==w::Reply::accepted);
    C(f.broker.availability()==w::Availability::ready&&f.broker.lifecycle_revision()==loss);
    auto released=f.decision(next,next_p,false);C(f.broker.commit(next,next_p,released)==w::Reply::accepted&&!f.broker.authorized());f.finish();
}

void temporary_loss_tests(){
    for(auto reply:{w::Reply::unavailable,w::Reply::busy}){
        Fixture f;f.attach();const auto attached=f.broker.lifecycle_revision();f.fake.prepare_reply=reply;
        auto frame=f.frame();frame.weapon={};w::Proposal p{};p.nonce=999;const auto untouched=p;
        C(f.broker.prepare(frame,p)==reply&&f.broker.availability()==w::Availability::unavailable&&!f.broker.draining());
        C(std::memcmp(&p,&untouched,sizeof(p))==0&&f.broker.lifecycle_revision()==attached+1&&!f.broker.pending_transactions());
        C(f.broker.prepare(frame,p)==w::Reply::busy&&f.fake.prepares==1);
        frame=f.frame(2);C(f.broker.prepare(frame,p)==reply&&f.broker.lifecycle_revision()==attached+1&&f.fake.invalidations==1);
        f.fake.prepare_reply=w::Reply::accepted;frame=f.frame(3);frame.left_grip_down=0;
        C(f.broker.prepare(frame,p)==w::Reply::accepted&&f.broker.availability()==w::Availability::ready);
        auto d=f.decision(frame,p,false);C(f.broker.commit(frame,p,d)==w::Reply::accepted);f.finish();
    }
    for(auto reply:{w::Reply::unavailable,w::Reply::busy}){
        Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};C(f.broker.prepare(frame,p)==w::Reply::accepted);
        f.fake.commit_reply=reply;auto d=f.decision(frame,p);const auto revision=f.broker.lifecycle_revision();
        C(f.broker.commit(frame,p,d)==reply&&!f.broker.authorized()&&!f.broker.pending_transactions());
        C(f.broker.availability()==w::Availability::unavailable&&f.broker.lifecycle_revision()==revision+1&&!f.broker.draining());
        C(f.broker.commit(frame,p,d)==w::Reply::busy&&f.fake.commits==1);
        f.fake.commit_reply=w::Reply::accepted;frame=f.frame(2);frame.left_grip_down=0;
        C(f.broker.prepare(frame,p)==w::Reply::accepted);d=f.decision(frame,p,false);C(f.broker.commit(frame,p,d)==w::Reply::accepted);f.finish();
    }
    Fixture untracked;untracked.attach();auto frame=untracked.frame();frame.controllers.focused=0;w::Proposal p{};
    C(untracked.broker.prepare(frame,p)==w::Reply::unavailable&&untracked.fake.prepares==0&&!untracked.broker.draining());
    untracked.finish();
}

void bad_callback_tests(){
    for(auto mutation:{Mutate::frame,Mutate::key,Mutate::nonce,Mutate::candidate,Mutate::cover,Mutate::duplicate,
                      Mutate::reserved,Mutate::size,Mutate::version,Mutate::reenter,Mutate::fault}){
        Fixture f;f.attach();f.fake.prepare_mutation=mutation;auto frame=f.frame();const auto original=frame;w::Proposal p{};
        C(f.broker.prepare(frame,p)==w::Reply::invalid&&f.broker.draining()&&!f.broker.authorized());
        C(!f.broker.pending_transactions()&&f.broker.quiescent()&&f.fake.invalidations==1);
        C(std::memcmp(&frame,&original,sizeof(frame))==0&&p.nonce==0);
        if(mutation==Mutate::reenter)C(f.fake.reentered==w::Reply::reentrant);
        f.finish();
    }
    for(auto reply:{w::Reply::invalid,w::Reply::draining,w::Reply::wrong_cookie,w::Reply::quiescent,w::Reply::reentrant,static_cast<w::Reply>(999)}){
        Fixture f;f.attach();f.fake.prepare_reply=reply;auto frame=f.frame();w::Proposal p{};
        C(f.broker.prepare(frame,p)==w::Reply::invalid&&f.broker.draining());f.finish();
    }
    for(auto mutation:{Mutate::frame,Mutate::proposal,Mutate::decision,Mutate::fault,Mutate::reenter}){
        Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};C(f.broker.prepare(frame,p)==w::Reply::accepted);
        auto d=f.decision(frame,p);const auto original_frame=frame;const auto original_p=p;const auto original_d=d;f.fake.commit_mutation=mutation;
        C(f.broker.commit(frame,p,d)==w::Reply::invalid&&f.broker.draining()&&!f.broker.authorized()&&!f.broker.pending_transactions());
        C(std::memcmp(&frame,&original_frame,sizeof(frame))==0&&std::memcmp(&p,&original_p,sizeof(p))==0&&std::memcmp(&d,&original_d,sizeof(d))==0);
        if(mutation==Mutate::reenter)C(f.fake.reentered==w::Reply::reentrant);
        f.finish();
    }
    for(auto reply:{w::Reply::invalid,w::Reply::draining,w::Reply::wrong_cookie,w::Reply::quiescent,w::Reply::reentrant,static_cast<w::Reply>(999)}){
        Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};C(f.broker.prepare(frame,p)==w::Reply::accepted);
        auto d=f.decision(frame,p);f.fake.commit_reply=reply;
        C(f.broker.commit(frame,p,d)==w::Reply::invalid&&f.broker.draining()&&!f.broker.pending_transactions());f.finish();
    }
    for(auto mutation:{Mutate::frame,Mutate::fault,Mutate::reenter}){
        Fixture f;f.attach();f.fake.invalidate_mutation=mutation;
        C(f.broker.invalidate(w::InvalidateReason::input_lost)==w::Reply::draining);
        C(f.broker.draining()&&!f.broker.quiescent()&&f.broker.pending()&&f.broker.pin_retained()&&f.broker.pending_callbacks()==0);
        test_thread=worker;C(f.broker.collect_on_owner_worker()==w::Reply::draining&&f.fake.unpins==0);
        f.fake.invalidate_mutation=Mutate::none;C(f.broker.unregister(f.result.cookie)==w::Reply::quiescent&&f.fake.invalidations==2);f.finish();
    }
}

void bad_frame_and_decision_tests(){
    for(unsigned bad=0;bad<8;++bad){
        Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};
        if(bad==0)++frame.key.native_started_qpc;if(bad==1)++frame.key.game_start;if(bad==2)++frame.key.game_pid;
        if(bad==3)frame.key.thread_id=3;if(bad==4)frame.reserved=1;if(bad==5)--frame.size;
        if(bad==6)frame.mesh.known=0;if(bad==7)frame.left_grip_down=2;
        C(f.broker.prepare(frame,p)==w::Reply::invalid&&f.fake.prepares==0&&f.broker.draining());f.finish();
    }
    for(unsigned bad=0;bad<12;++bad){
        Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};C(f.broker.prepare(frame,p)==w::Reply::accepted);
        auto d=f.decision(frame,p);auto changed_frame=frame;auto changed_p=p;
        switch(bad){
        case 0:++d.proposal_nonce;break;case 1:++d.press_token;break;case 2:++d.key.frame;break;
        case 3:d.reserved=1;break;case 4:++d.abi_version;break;case 5:--d.size;break;
        case 6:d.new_grant=0;break;case 7:d.lease.id=99;break;case 8:d.lease.kind=w::LeaseKind::ammo;break;
        case 9:++changed_frame.now_ns;break;case 10:++changed_p.nonce;break;case 11:d.release_required=1;break;
        }
        C(f.broker.commit(changed_frame,changed_p,d)==w::Reply::invalid&&f.fake.commits==0);
        C(f.broker.pending_transactions()==1&&f.broker.draining()&&!f.broker.quiescent());
        C(f.broker.abort_frame(frame.key,p.nonce)==w::Reply::quiescent);f.finish();
    }
    Fixture retry;retry.attach();auto frame=retry.frame();w::Proposal p{};C(retry.broker.prepare(frame,p)==w::Reply::accepted);
    auto old=frame;++old.key.registration_cookie;const auto revision=retry.broker.lifecycle_revision();auto d=retry.decision(old,p);
    C(retry.broker.prepare(old,p)==w::Reply::wrong_cookie);C(retry.broker.commit(old,p,d)==w::Reply::wrong_cookie);
    C(retry.broker.abort_frame(old.key,p.nonce)==w::Reply::wrong_cookie&&retry.broker.lifecycle_revision()==revision);
    auto changed=frame;++changed.now_ns;C(retry.broker.prepare(changed,p)==w::Reply::invalid&&retry.broker.pending_transactions()==1);
    C(retry.broker.abort_frame(frame.key,p.nonce)==w::Reply::quiescent);retry.finish();
}

void transaction_drain_tests(){
    Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};C(f.broker.prepare(frame,p)==w::Reply::accepted);
    test_thread=worker;C(f.broker.unregister(f.result.cookie)==w::Reply::draining);
    C(f.broker.pending_transactions()==1&&f.broker.pending_callbacks()==0&&!f.broker.quiescent());
    for(unsigned n=0;n<3;++n)C(f.broker.collect_on_owner_worker()==w::Reply::draining&&f.fake.unpins==0);
    C(f.broker.unregister(f.result.cookie)==w::Reply::draining&&f.fake.invalidations==1);
    test_thread=gt;auto d=f.decision(frame,p);C(f.broker.commit(frame,p,d)==w::Reply::draining&&f.fake.commits==0);
    C(f.broker.pending_transactions()==1);auto bad=frame.key;++bad.frame;
    C(f.broker.abort_frame(bad,p.nonce)==w::Reply::invalid);C(f.broker.abort_frame(frame.key,p.nonce+1)==w::Reply::invalid);
    test_thread=3;C(f.broker.abort_frame(frame.key,p.nonce)==w::Reply::invalid&&f.broker.pending_transactions()==1);
    test_thread=gt;C(f.broker.abort_frame(frame.key,p.nonce)==w::Reply::quiescent);
    C(f.broker.collect_on_owner_worker()==w::Reply::invalid&&f.fake.unpins==0);
    test_thread=worker;f.fake.unpin_ok=false;C(f.broker.collect_on_owner_worker()==w::Reply::unavailable&&f.broker.pin_retained());
    f.fake.unpin_ok=true;f.fake.unpin_reenter=true;C(f.broker.collect_on_owner_worker()==w::Reply::quiescent);
    C(f.fake.reentered==w::Reply::busy&&f.fake.unpins==2&&f.fake.violations==0);
    const auto retired_cookie=f.result.cookie;f.fake.unpin_reenter=false;f.attach();C(f.result.cookie!=retired_cookie);
    const auto revision=f.broker.lifecycle_revision();C(f.broker.unregister(retired_cookie)==w::Reply::quiescent&&f.broker.lifecycle_revision()==revision);
    C(f.broker.cookie()==f.result.cookie&&f.broker.availability()==w::Availability::ready);f.finish();

    Fixture soft;soft.attach();frame=soft.frame();C(soft.broker.prepare(frame,p)==w::Reply::accepted);
    test_thread=worker;soft.broker.invalidate(w::InvalidateReason::context_lost);
    C(soft.broker.pending_transactions()==1&&!soft.broker.draining()&&soft.broker.availability()==w::Availability::unavailable);
    test_thread=gt;d=soft.decision(frame,p);C(soft.broker.commit(frame,p,d)==w::Reply::unavailable&&soft.fake.commits==0);
    C(soft.broker.abort_frame(frame.key,p.nonce)==w::Reply::accepted);frame=soft.frame(2);frame.left_grip_down=0;
    C(soft.broker.prepare(frame,p)==w::Reply::accepted);d=soft.decision(frame,p,false);C(soft.broker.commit(frame,p,d)==w::Reply::accepted);soft.finish();
}

void concurrent_tests(){
    for(unsigned phase:{1u,2u}){
        Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};
        if(phase==2)C(f.broker.prepare(frame,p)==w::Reply::accepted);
        auto d=f.decision(frame,p);f.fake.start_barrier(phase);std::atomic<w::Reply> response{w::Reply::invalid};
        std::thread caller([&](){test_thread=gt;response=phase==1?f.broker.prepare(frame,p):f.broker.commit(frame,p,d);});
        C(WaitForSingleObject(f.fake.entered,3000)==WAIT_OBJECT_0);test_thread=worker;
        C(f.broker.unregister(f.result.cookie)==w::Reply::draining);
        C(f.broker.pending_callbacks()==1&&f.broker.pending_transactions()==1&&f.broker.pin_retained());
        C(f.broker.collect_on_owner_worker()==w::Reply::draining&&f.fake.unpins==0);
        SetEvent(f.fake.resume);caller.join();f.fake.end_barrier();
        C(response==w::Reply::draining&&!f.broker.authorized()&&!f.broker.pending_transactions()&&f.broker.quiescent());
        C(f.fake.invalidations==2&&!f.fake.publishing&&!f.fake.staged);f.finish();
    }
    // A recovery prepare starts inside an unavailable phase. A second context
    // loss must cancel its result even though that phase bumps revision once.
    Fixture recovery;recovery.attach();auto frame=recovery.frame();w::Proposal proposal{};recovery.fake.prepare_reply=w::Reply::unavailable;
    C(recovery.broker.prepare(frame,proposal)==w::Reply::unavailable);const auto revision=recovery.broker.lifecycle_revision();
    recovery.fake.prepare_reply=w::Reply::accepted;frame=recovery.frame(2);recovery.fake.start_barrier(1);
    std::atomic<w::Reply> recovered{w::Reply::invalid};
    std::thread recovering([&](){test_thread=gt;recovered=recovery.broker.prepare(frame,proposal);});
    C(WaitForSingleObject(recovery.fake.entered,3000)==WAIT_OBJECT_0);test_thread=worker;
    recovery.broker.invalidate(w::InvalidateReason::context_lost);C(recovery.broker.lifecycle_revision()==revision);
    SetEvent(recovery.fake.resume);recovering.join();recovery.fake.end_barrier();
    C(recovered==w::Reply::unavailable&&recovery.broker.availability()==w::Availability::unavailable&&!recovery.broker.pending_transactions());
    C(recovery.fake.invalidations==3&&!recovery.fake.staged);
    frame=recovery.frame(3);frame.left_grip_down=0;C(recovery.broker.prepare(frame,proposal)==w::Reply::accepted);
    auto release=recovery.decision(frame,proposal,false);C(recovery.broker.commit(frame,proposal,release)==w::Reply::accepted);recovery.finish();

    // Exercise the opposite completion order: invalidate has disarmed and is
    // still in flight while the old commit publishes. Only its final repeat
    // may make the registration quiescent; no Native lock is held by either.
    Fixture late;late.attach();frame=late.frame();C(late.broker.prepare(frame,proposal)==w::Reply::accepted);
    auto decision=late.decision(frame,proposal);late.fake.start_barrier(2);
    late.fake.invalidate_entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);late.fake.invalidate_resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    C(late.fake.invalidate_entered&&late.fake.invalidate_resume);late.fake.block_invalidation=true;
    std::atomic<w::Reply> commit_response{w::Reply::invalid},stop_response{w::Reply::invalid};
    std::thread committing([&](){test_thread=gt;commit_response=late.broker.commit(frame,proposal,decision);});
    C(WaitForSingleObject(late.fake.entered,3000)==WAIT_OBJECT_0);
    std::thread stopping([&](){test_thread=worker;stop_response=late.broker.unregister(late.result.cookie);});
    C(WaitForSingleObject(late.fake.invalidate_entered,3000)==WAIT_OBJECT_0);SetEvent(late.fake.resume);committing.join();
    test_thread=worker;C(commit_response==w::Reply::draining&&late.fake.publishing&&late.broker.pending_callbacks()==1);
    C(!late.broker.quiescent()&&late.broker.collect_on_owner_worker()==w::Reply::draining&&late.fake.unpins==0);
    SetEvent(late.fake.invalidate_resume);stopping.join();late.fake.end_barrier();
    CloseHandle(late.fake.invalidate_entered);CloseHandle(late.fake.invalidate_resume);
    C(stop_response==w::Reply::quiescent&&late.fake.invalidations==2&&!late.fake.publishing);late.finish();

    Fixture registration;registration.fake.start_barrier(3);std::atomic<w::Reply> response{w::Reply::invalid};
    std::thread caller([&](){test_thread=3;response=registration.broker.register_participant(&registration.registration,&registration.result);});
    C(WaitForSingleObject(registration.fake.entered,3000)==WAIT_OBJECT_0);test_thread=worker;
    registration.broker.invalidate(w::InvalidateReason::native_stopping);
    C(registration.broker.collect_on_owner_worker()==w::Reply::draining&&registration.fake.unpins==0);
    SetEvent(registration.fake.resume);caller.join();registration.fake.end_barrier();
    C(response==w::Reply::draining&&registration.broker.quiescent()&&registration.fake.invalidations==0);
    C(registration.broker.collect_on_owner_worker()==w::Reply::quiescent);
    C(registration.broker.register_participant(&registration.registration,&registration.result)==w::Reply::draining);
}

void clock_thread_and_pin_tests(){
    Fixture stale;stale.attach();auto frame=stale.frame();stale.fake.stamp+=100000001;w::Proposal p{};
    C(stale.broker.prepare(frame,p)==w::Reply::unavailable&&!stale.broker.draining()&&stale.fake.prepares==0);stale.finish();
    Fixture commit_stale;commit_stale.attach();frame=commit_stale.frame();C(commit_stale.broker.prepare(frame,p)==w::Reply::accepted);
    commit_stale.fake.stamp+=100000001;auto d=commit_stale.decision(frame,p);
    C(commit_stale.broker.commit(frame,p,d)==w::Reply::unavailable&&commit_stale.broker.pending_transactions()==1);
    C(commit_stale.broker.abort_frame(frame.key,p.nonce)==w::Reply::accepted);commit_stale.finish();
    Fixture unknown_clock;unknown_clock.attach();frame=unknown_clock.frame();C(unknown_clock.broker.prepare(frame,p)==w::Reply::accepted);
    d=unknown_clock.decision(frame,p);unknown_clock.fake.stamp=0;
    C(unknown_clock.broker.commit(frame,p,d)==w::Reply::unavailable&&!unknown_clock.broker.draining());
    C(unknown_clock.broker.pending_transactions()==1&&unknown_clock.fake.commits==0);
    C(unknown_clock.broker.abort_frame(frame.key,p.nonce)==w::Reply::accepted);unknown_clock.finish();
    Fixture thread;thread.attach();frame=thread.frame();C(thread.broker.prepare(frame,p)==w::Reply::accepted);
    C(thread.broker.abort_frame(frame.key,p.nonce)==w::Reply::accepted);
    test_thread=worker;C(thread.broker.unregister(thread.result.cookie)==w::Reply::quiescent);C(thread.broker.collect_on_owner_worker()==w::Reply::quiescent);
    thread.attach();frame=thread.frame(2);frame.key.thread_id=3;test_thread=3;
    C(thread.broker.prepare(frame,p)==w::Reply::invalid&&thread.broker.draining());thread.finish();
    std::uintptr_t first{},second{},third{};
    C(w::weapon_broker_win32::executable_image(reinterpret_cast<std::uintptr_t>(&Fake::prepare),first));
    C(w::weapon_broker_win32::executable_image(reinterpret_cast<std::uintptr_t>(&Fake::commit),second)&&first==second);
    C(w::weapon_broker_win32::executable_image(reinterpret_cast<std::uintptr_t>(&Fake::invalidate),third)&&first==third);
    void* private_code=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);C(private_code!=nullptr);
    C(!w::weapon_broker_win32::executable_image(reinterpret_cast<std::uintptr_t>(private_code),first));
    Fixture mixed;auto bad=mixed.registration;std::memcpy(&bad.commit,&private_code,sizeof(private_code));w::ModulePin held{};
    C(!w::weapon_broker_win32::pin(nullptr,bad,held)&&!held.handle);C(VirtualFree(private_code,0,MEM_RELEASE)!=FALSE);
    auto other=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetCurrentThreadId");C(other!=nullptr);
    std::memcpy(&bad.commit,&other,sizeof(other));C(!w::weapon_broker_win32::pin(nullptr,bad,held)&&!held.handle);
    C(w::weapon_broker_win32::filter(0xe06d7363ul)==EXCEPTION_CONTINUE_SEARCH);
    C(w::weapon_broker_win32::filter(EXCEPTION_STACK_OVERFLOW)==EXCEPTION_CONTINUE_SEARCH);
    C(w::weapon_broker_win32::filter(EXCEPTION_ACCESS_VIOLATION)==EXCEPTION_EXECUTE_HANDLER);
    C(w::weapon_broker_win32::counter_ns(12345678,10000000)==1234567800);
    C(w::weapon_broker_win32::counter_ns(9,4)==2250000000ll);
    C(w::weapon_broker_win32::counter_ns(10,0)==0&&w::weapon_broker_win32::counter_ns(0,1)==0&&w::weapon_broker_win32::counter_ns(-1,1)==0);
    C(w::weapon_broker_win32::counter_ns((std::numeric_limits<std::int64_t>::max)(),1)==0);
    C(w::weapon_broker_win32::clock_ns(nullptr)>0);
}

void retired_cookie_tests(){
    Fixture f;std::array<std::uint64_t,17> cookies{};
    for(auto& value:cookies){
        f.attach();value=f.result.cookie;test_thread=worker;
        C(f.broker.unregister(value)==w::Reply::quiescent);
        C(f.broker.collect_on_owner_worker()==w::Reply::quiescent&&f.broker.cookie()==0);
        // Native's worker has already collected; PG can still obtain its exact
        // transport-quiescent receipt on its later unregister retry.
        C(f.broker.unregister(value)==w::Reply::quiescent);
    }
    C(f.broker.unregister(cookies.front())==w::Reply::wrong_cookie); // Bounded ring rotated it out.
    for(std::size_t n=1;n<cookies.size();++n)C(f.broker.unregister(cookies[n])==w::Reply::quiescent);
    C(f.broker.unregister(0)==w::Reply::wrong_cookie);
    C(f.broker.unregister(cookies.back()+1)==w::Reply::wrong_cookie);
    C(f.broker.unregister((std::numeric_limits<std::uint64_t>::max)())==w::Reply::wrong_cookie);
    f.attach();const auto current=f.result.cookie,revision=f.broker.lifecycle_revision();
    const auto invalidations=f.fake.invalidations.load();
    C(f.broker.unregister(cookies.back())==w::Reply::quiescent);
    C(f.broker.cookie()==current&&f.broker.lifecycle_revision()==revision&&f.fake.invalidations==invalidations);
    C(f.broker.availability()==w::Availability::ready&&!f.broker.draining());
    auto frame=f.frame();w::Proposal p{};C(f.broker.prepare(frame,p)==w::Reply::accepted);
    C(f.broker.unregister(cookies.back())==w::Reply::quiescent&&f.broker.pending_transactions()==1);
    auto d=f.decision(frame,p);C(f.broker.commit(frame,p,d)==w::Reply::accepted&&f.broker.authorized());
    C(f.broker.unregister(cookies.back())==w::Reply::quiescent&&f.broker.authorized());f.finish();

    Fixture failed;failed.attach();const auto held_cookie=failed.result.cookie;
    C(failed.broker.unregister(held_cookie)==w::Reply::quiescent);failed.fake.unpin_ok=false;
    C(failed.broker.collect_on_owner_worker()==w::Reply::unavailable);
    C(failed.broker.cookie()==held_cookie&&failed.broker.pin_retained()&&failed.broker.availability()==w::Availability::unavailable);
    C(failed.broker.register_participant(&failed.registration,&failed.result)==w::Reply::busy);
    C(failed.broker.unregister(held_cookie+1)==w::Reply::wrong_cookie);
    failed.fake.unpin_ok=true;C(failed.broker.collect_on_owner_worker()==w::Reply::quiescent&&failed.broker.cookie()==0);
    C(failed.broker.unregister(held_cookie)==w::Reply::quiescent);failed.finish();

    Fixture unpublished;unpublished.fake.pin_ok=false;unpublished.fake.pin_on_failure=true;
    C(unpublished.broker.register_participant(&unpublished.registration,&unpublished.result)==w::Reply::invalid);
    const auto reserved_cookie=unpublished.broker.cookie();C(reserved_cookie&&unpublished.result.cookie==0);
    C(unpublished.broker.collect_on_owner_worker()==w::Reply::quiescent);
    C(unpublished.broker.unregister(reserved_cookie)==w::Reply::wrong_cookie);unpublished.finish();

    // Receipts are owned by this exact Broker/session. A different Native
    // incarnation (or game lifetime) inherits neither the ring nor its proofs.
    for(auto session:{w::BrokerSession{42,worker,400,501},w::BrokerSession{42,worker,401,500},w::BrokerSession{43,worker,400,500}}){
        Fake other;w::Broker fresh(session,other.backend());other.broker=&fresh;
        C(fresh.unregister(held_cookie)==w::Reply::wrong_cookie&&fresh.cookie()==0);
        C(fresh.availability()==w::Availability::absent_quiescent&&other.pins==0&&other.invalidations==0);
    }
}
#include "weapon_hand_call_broker_test.inl"
#include "main_mount_broker_test.inl"
#include "body_main_mount_broker_test.inl"
#include "committed_hand_consumer_test.inl"
void part_visibility_admission_tests(){
    Fixture f;f.attach();f.fake.magazine_candidate=true;
    auto frame=f.frame();w::Proposal proposal{};
    C(!f.broker.admit_part_visibility(frame));
    C(f.broker.prepare(frame,proposal)==w::Reply::accepted);
    auto decision=f.decision(frame,proposal);decision.lease.kind=w::LeaseKind::magazine;
    C(f.broker.commit(frame,proposal,decision)==w::Reply::accepted);
    C(f.broker.admit_part_visibility(frame));
    auto changed=frame;changed.press_token++;
    C(!f.broker.admit_part_visibility(changed));
    changed=frame;changed.key.registration_cookie++;
    C(!f.broker.admit_part_visibility(changed));
    changed=frame;changed.left_grip_down=0;C(!f.broker.admit_part_visibility(changed));
    test_thread=worker;C(!f.broker.admit_part_visibility(frame));test_thread=gt;
    f.fake.stamp+=100000001;C(!f.broker.admit_part_visibility(frame));
    auto next=f.frame(2);C(f.broker.prepare(next,proposal)==w::Reply::accepted);
    decision=f.decision(next,proposal,false);C(f.broker.commit(next,proposal,decision)==w::Reply::accepted);
    C(!f.broker.admit_part_visibility(next));f.finish();
    Fixture support;support.attach();frame=support.frame();C(support.broker.prepare(frame,proposal)==w::Reply::accepted);
    decision=support.decision(frame,proposal);C(support.broker.commit(frame,proposal,decision)==w::Reply::accepted);
    C(!support.broker.admit_part_visibility(frame));C(support.broker.admit_support_visibility(frame));
    auto pinch=frame;pinch.interaction_control=1;C(!support.broker.admit_support_visibility(pinch));
    support.fake.stamp+=100000001;C(!support.broker.admit_support_visibility(frame));support.finish();
}
void weapon_render_admission_tests(){
    Fixture f;f.attach();auto frame=f.frame();w::Proposal proposal{};
    C(!f.broker.admit_weapon_render(frame));C(f.broker.prepare(frame,proposal)==w::Reply::accepted);
    auto decision=f.decision(frame,proposal);C(f.broker.commit(frame,proposal,decision)==w::Reply::accepted);
    C(f.broker.admit_weapon_render(frame));C(!f.broker.admit_part_visibility(frame));
    auto changed=frame;changed.key.registration_cookie++;C(!f.broker.admit_weapon_render(changed));
    changed=frame;changed.controllers.sequence++;C(!f.broker.admit_weapon_render(changed));
    test_thread=worker;C(!f.broker.admit_weapon_render(frame));test_thread=gt;
    f.fake.stamp+=100000001;C(!f.broker.admit_weapon_render(frame));f.finish();
}
void proximity_socket_admission_tests(){
    Fixture f;f.attach();auto frame=f.frame();w::Proposal proposal{};
    frame.left_grip_down=0;frame.press_token=0;
    frame.controllers.hands[0].squeeze_active=1;frame.controllers.hands[0].squeeze=0;
    C(!f.broker.admit_socket_visibility(frame));
    C(f.broker.prepare(frame,proposal)==w::Reply::accepted);
    auto decision=f.decision(frame,proposal,false);
    C(f.broker.commit(frame,proposal,decision)==w::Reply::accepted);
    C(f.broker.admit_socket_visibility(frame));
    C(!f.broker.admit_part_visibility(frame)&&!f.broker.admit_support_visibility(frame));
    auto changed=frame;changed.controllers.hands[0].squeeze=.8f;
    C(!f.broker.admit_socket_visibility(changed));
    changed=frame;changed.controllers.hands[0].squeeze_active=0;C(!f.broker.admit_socket_visibility(changed));
    changed=frame;changed.left_tracked=0;C(!f.broker.admit_socket_visibility(changed));
    changed=frame;changed.reference_generation++;C(!f.broker.admit_socket_visibility(changed));
    test_thread=worker;C(!f.broker.admit_socket_visibility(frame));test_thread=gt;
    f.fake.stamp+=100000001;C(!f.broker.admit_socket_visibility(frame));f.finish();
    Fixture held;held.attach();frame=held.frame();frame.controllers.hands[0].squeeze_active=1;
    C(held.broker.prepare(frame,proposal)==w::Reply::accepted);decision=held.decision(frame,proposal);
    C(held.broker.commit(frame,proposal,decision)==w::Reply::accepted);
    C(!held.broker.admit_socket_visibility(frame));held.finish();
}
} // namespace
int main(){
    registration_tests();pending_magazine_tests();normal_and_retry_tests();temporary_loss_tests();bad_callback_tests();
    bad_frame_and_decision_tests();transaction_drain_tests();concurrent_tests();clock_thread_and_pin_tests();retired_cookie_tests();
    auxiliary_tests();main_mount_tests();body_mount_tests();hand_visual_tests();
    part_visibility_admission_tests();
    weapon_render_admission_tests();
    proximity_socket_admission_tests();
    std::printf("weapon_hand_broker: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
