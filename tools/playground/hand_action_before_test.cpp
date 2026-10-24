// The real implementation is included below. Host services control input and
// restoration effects; the fake Action destructor models a late finish only.
// This does not establish real reflection, engine execution or handback safety.
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>
#include "../vr-common/hand_ik_trial_policy.h"
#ifdef NDEBUG
#error HandActionBeforeTest requires active assertions
#endif
namespace {
using uptr=std::uintptr_t;
enum class Late {none,hand_dirty,hand_fault,component_dirty,component_fault,cleanup_failed,relinquished,clear_dirty};
struct Config {
    bool active{true},anchored{true},registered{true},related{true},named{true},same_thread{true},releasing{},ended{},immediate_pending{true};
    bool dirty{true},fault{},component_dirty{},component_fault{},relinquished{},cleanup_failed{},restored{true};
    bool after_dirty{},after_fault{},after_component_dirty{},after_component_fault{};
    unsigned phase{2},gt{7};Late late{Late::none};wchar_t const* label=L"StartMeleeAttack";
} config;
std::vector<std::string> calls;
std::atomic<unsigned> game_thread{7},status{};FILE* log_file{};
unsigned GetCurrentThreadId(){calls.push_back("thread");return config.same_thread?7u:8u;}
bool registered(uptr){calls.push_back("registered");return config.registered;}
bool name(uptr,wchar_t* out,std::size_t count){calls.push_back("name");if(!config.named)return false;
    const auto length=wcslen(config.label);assert(length<count);std::wmemcpy(out,config.label,length+1);return true;}
[[maybe_unused]] inline bool diagnostic_event(){return false;}
namespace handstream {
struct Lease {bool pending{},failed{};bool dirty()const noexcept{return pending;}bool faulted()const noexcept{return failed;}};
struct Trial {
    Lease lease;bool anchored{},releasing{},ended{},immediate_pending{},slot_play_seen{},melee_seen{},xr_ready{true};
    unsigned phase{},epoch{3},action_yields{},xr_equipment_phase{1};gears_hand_ik::StabilityGate stability;
} trial;
std::atomic<bool> active_visible{true};bool hand_writes_relinquished{},component_pending{},component_failed{};
inline bool journals_dirty(){calls.push_back("journals_dirty");return trial.lease.dirty()||component_pending;}
inline bool related(uptr){calls.push_back("related");return config.related;}
inline bool component_dirty(){return component_pending;}
inline bool component_faulted(){return component_failed;}
inline void component_relinquish(){calls.push_back("component_relinquish");component_failed=true;}
inline void publish_journals(){calls.push_back("publish_journals");}
inline void end(unsigned code,wchar_t const*){calls.push_back("end:"+std::to_string(code));trial.ended=true;}
inline void restoration_effect(){trial.lease.pending=config.after_dirty;trial.lease.failed=config.after_fault;
    component_pending=config.after_component_dirty;component_failed=config.after_component_fault;}
inline bool yield_for_equipment(wchar_t const*){calls.push_back("yield_for_equipment");restoration_effect();return config.restored;}
inline bool restore(wchar_t const*){calls.push_back("restore");restoration_effect();return config.restored;}
namespace component_continuous {
struct State {unsigned action_depth{},finished{};bool cleanup_failed{};} state;
inline void late_finish(){
    calls.push_back("Action.finish");++state.finished;
    switch(config.late){
    case Late::none:break;
    case Late::hand_dirty:trial.lease.pending=true;break;
    case Late::hand_fault:trial.lease.failed=true;break;
    case Late::component_dirty:component_pending=true;break;
    case Late::component_fault:component_failed=true;break;
    case Late::cleanup_failed:state.cleanup_failed=true;break;
    case Late::relinquished:hand_writes_relinquished=true;break;
    case Late::clear_dirty:trial.lease.pending=false;component_pending=false;break;
    }
}
struct Action {
    Action(){calls.push_back("Action.begin");++state.action_depth;}
    ~Action(){calls.push_back("Action.end");--state.action_depth;if(!state.action_depth){late_finish();publish_journals();}}
    Action(Action const&)=delete;Action& operator=(Action const&)=delete;
};
inline void suspend_interval(wchar_t const*,bool keep){assert(!keep);calls.push_back("suspend_interval");}
inline void request_terminal(unsigned,wchar_t const*){calls.push_back("request_terminal");}
}
#include "hand_action_before.inl"
}
using Outcome=handstream::BeforeActionOutcome;
void setup(Config const& input){config=input;calls.clear();handstream::trial={};auto& trial=handstream::trial;
    trial.anchored=config.anchored;trial.releasing=config.releasing;trial.ended=config.ended;trial.immediate_pending=config.immediate_pending;
    trial.phase=config.phase;trial.lease={config.dirty,config.fault};handstream::active_visible=config.active;
    handstream::component_pending=config.component_dirty;handstream::component_failed=config.component_fault;
    handstream::hand_writes_relinquished=config.relinquished;handstream::component_continuous::state={};
    handstream::component_continuous::state.cleanup_failed=config.cleanup_failed;status=0;log_file=nullptr;game_thread=config.gt;
}
struct Result {
    std::vector<std::string> calls;bool dirty{},fault{},component_dirty{},component_fault{},relinquished{},cleanup_failed{},ended{},slot{},melee{};
    unsigned epoch{},phase{},action_yields{},depth{},finished{},status{};
    bool operator==(Result const& b)const{return calls==b.calls&&dirty==b.dirty&&fault==b.fault&&component_dirty==b.component_dirty&&
        component_fault==b.component_fault&&relinquished==b.relinquished&&cleanup_failed==b.cleanup_failed&&ended==b.ended&&slot==b.slot&&
        melee==b.melee&&epoch==b.epoch&&phase==b.phase&&action_yields==b.action_yields&&depth==b.depth&&finished==b.finished&&status==b.status;}
};
Result result(){auto const& t=handstream::trial;auto const& s=handstream::component_continuous::state;
    return {calls,t.lease.dirty(),t.lease.faulted(),handstream::component_pending,handstream::component_failed,
        handstream::hand_writes_relinquished,s.cleanup_failed,t.ended,t.slot_play_seen,t.melee_seen,t.epoch,t.phase,t.action_yields,s.action_depth,s.finished,status.load()};}
unsigned call_count(char const* name){unsigned count{};for(auto const& call:calls)if(call==name)++count;return count;}
Outcome checked(bool owned){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    return handstream::before_action_checked(1,2,owned);
#else
    (void)owned;return handstream::before_action_checked(1,2);
#endif
}
void old_void(bool owned){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    handstream::before_action(1,2,owned);
#else
    (void)owned;handstream::before_action(1,2);
#endif
}
unsigned cases{},comparisons{};
Result verify(Config const& input,Outcome expected,char const* label,bool owned=false){
    setup(input);assert(checked(owned)==expected);const auto direct=result();
    setup(input);old_void(owned);assert(result()==direct);++comparisons;++cases;
    std::printf("%02u %s\n",cases,label);return direct;
}
}
int main(){
    {Config c;c.same_thread=false;verify(c,Outcome::ignored,"GT guard");assert(!call_count("registered")&&!call_count("name")&&!call_count("related"));}
    {Config c;c.gt=0;verify(c,Outcome::ignored,"unknown GT guard");assert(!call_count("registered")&&!call_count("name")&&!call_count("related"));}
    {Config c;c.active=false;c.dirty=false;verify(c,Outcome::ignored,"inactive clean guard");}
    {Config c;c.anchored=false;verify(c,Outcome::ignored,"unanchored guard");}
    {Config c;c.registered=false;verify(c,Outcome::ignored,"function registration guard");assert(!call_count("name"));}
    {Config c;c.named=false;verify(c,Outcome::ignored,"unknown name preserves related fallback");assert(call_count("related")==1);}
    {Config c;c.related=false;verify(c,Outcome::ignored,"object association guard");
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        assert(call_count("name")==1);
#else
        assert(!call_count("name"));
#endif
    }
    {Config c;c.label=L"GetBoneMatrix";verify(c,Outcome::ignored,"known passive name");
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        assert(!call_count("related"));
#else
        assert(call_count("related")==1);
#endif
    }
    {Config c;c.label=L"startmeleeattack";verify(c,Outcome::ignored,"case-sensitive action names");}
    {Config c;verify(c,Outcome::clean,"successful restoration with clean journals");}
    {Config c;c.restored=false;verify(c,Outcome::unresolved,"false restoration despite clean final state",true);}
    {Config c;c.after_dirty=true;verify(c,Outcome::unresolved,"hand dirty remains");}
    {Config c;c.after_fault=true;verify(c,Outcome::unresolved,"hand fault remains");}
    {Config c;c.dirty=false;verify(c,Outcome::clean,"clean recognized action");}
    {Config c;c.phase=3;c.label=L"PlayCustomAnimByDuration";const auto r=verify(c,Outcome::clean,"animation observation marker retained");assert(r.slot);}
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    {Config c;c.active=false;verify(c,Outcome::clean,"inactive dirty mesh still hands back");}
    {Config c;c.releasing=true;verify(c,Outcome::clean,"mesh releasing is not blanket exemption");}
    for(auto label:{L"SetAbsolute",L"SetTranslation",L"SetRotation",L"SetWeaponFromSlot",L"ForceReload"}){
        Config c;c.label=label;verify(c,Outcome::clean,"phase1 full action list uses production classifier");assert(handstream::component_action_name(label));}
    {Config c;c.after_component_dirty=true;verify(c,Outcome::unresolved,"component dirty remains");}
    {Config c;c.after_component_fault=true;verify(c,Outcome::unresolved,"component fault remains");}
    {Config c;c.relinquished=true;verify(c,Outcome::unresolved,"hand writes relinquished remains");}
    {Config c;c.cleanup_failed=true;verify(c,Outcome::unresolved,"existing cleanup failure remains");}
    {Config c;c.restored=false;const auto r=verify(c,Outcome::unresolved,"external failed restoration relinquishes before forwarding");
        assert(r.relinquished&&r.ended&&call_count("component_relinquish")==1&&call_count("end:8")==1);}
    {Config c;c.restored=false;const auto r=verify(c,Outcome::unresolved,"owned failed restoration still unresolved",true);
        assert(!r.relinquished&&!r.ended&&!call_count("component_relinquish")&&!call_count("end:8"));}
    {Config c;c.restored=false;c.ended=true;verify(c,Outcome::unresolved,"external failure preserves prior ended guard");assert(!call_count("end:8"));}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    for(auto late:{Late::hand_dirty,Late::hand_fault,Late::component_dirty,Late::component_fault,Late::cleanup_failed,Late::relinquished}){
        Config c;c.late=late;const auto r=verify(c,Outcome::unresolved,"checked observes late Action destructor failure");
        assert(r.finished==1&&!r.depth&&call_count("Action.finish")==1);}
    {Config c;c.after_dirty=true;c.after_component_dirty=true;c.late=Late::clear_dirty;
        const auto r=verify(c,Outcome::clean,"checked sees actual cleanup completed by destructor");assert(!r.dirty&&!r.component_dirty&&r.finished==1);}
    {Config c;c.restored=false;c.after_dirty=true;c.late=Late::clear_dirty;
        const auto r=verify(c,Outcome::unresolved,"late clean destructor cannot erase false restore receipt",true);
        assert(!r.dirty&&!r.component_dirty&&!r.cleanup_failed&&!r.relinquished&&r.finished==1);}
    {Config c;c.late=Late::cleanup_failed;setup(c);handstream::ActionAttempt attempt{};
        handstream::before_action_impl(1,2,attempt,true);assert(attempt.recognized&&attempt.restore_known&&attempt.restore_ok);
        assert(handstream::component_continuous::state.finished==1&&handstream::component_continuous::state.cleanup_failed);
        ++cases;std::printf("%02u real impl returns only after Action destructor\n",cases);}
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    {Config c;const auto r=verify(c,Outcome::clean,"XR melee observation marker retained");assert(r.melee);}
#endif
#else
    {Config c;c.active=false;verify(c,Outcome::ignored,"legacy inactive dirty guard");}
    {Config c;c.releasing=true;verify(c,Outcome::ignored,"legacy releasing guard");assert(!call_count("name")&&!call_count("related"));}
    for(auto label:{L"SetAbsolute",L"SetWeaponFromSlot",L"ForceReload"}){Config c;c.label=label;
        verify(c,Outcome::ignored,"legacy action list remains narrower");}
    {Config c;c.immediate_pending=false;const auto r=verify(c,Outcome::clean,"legacy unexpected action end remains ordered");assert(r.ended&&call_count("end:7")==1);}
    {Config c;c.restored=false;c.immediate_pending=false;verify(c,Outcome::unresolved,"legacy false restore returns before unexpected-action end");assert(!call_count("end:7"));}
    {Config c;c.dirty=false;setup(c);handstream::ActionAttempt attempt{};handstream::before_action_impl(1,2,attempt);
        assert(attempt.recognized&&!attempt.restore_known&&!call_count("restore"));++cases;std::printf("%02u legacy clean action has no invented restore receipt\n",cases);}
#endif
    std::printf("HandActionBeforeTest passed: %u cases, %u checked/void trace-state comparisons; production include, controlled host restoration/late Action effects only\n",cases,comparisons);
}
