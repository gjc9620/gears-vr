#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

// Passive, owned POD only. No game reads, guards, permission, locks or GT IO.
namespace gears_body_held_outcome {
using U=std::uint64_t;
constexpr U unknown=UINT64_MAX;
constexpr unsigned capacity=128;
enum class SourceGate:U {okay,policy,thread,query_scope,captured,prepared,notified,
    accepted_parent,queried_parent,input_bytes,scene_ticket,cache_revision,compose,
    capture,component,view,geometry,model_scale,units,context,exception};
struct Guard {U begin_ns{},end_ns{},gate{unknown},context_verdict{unknown};};
struct Admission {U now_ns{},source_verdict{unknown},passed{},capture_ns{};};
enum class NativeBegin:U {captured,policy,thread,already_captured,kernel_pending,tick_entry,
    body_source_copy,body_input_copy,backend_capture,prepare_model,recovery_defer,recovery_hard,exception};
enum class PreQueryGate:U {passed=0,marker_missing=1,marker_basis=2,eye_status=3,main_cookie=4,
    body_policy=10,body_cookie=11,parent_cookie=12,registration_current=13,context_prepare=14,
    lifecycle_revision=15,action_generation=16,recorder_cookie_missing=20};
struct Record {
    U entry{},tick{},compose{},thread{},cookie{},pid{},native_started{},game_start{},world_generation{},source_ticket{},publication{},scene{},
      begin_ns{},end_ns{},original_published_ns{},original_scene_qpc{},original_scene_frequency{},
      cache_revision{},tracking_begin_ns{},tracking_end_ns{},stereo_received_ns{},
      controller_received_ns{},sync_ns{},capture_ns{},input_recorded{},
      query_begin_ns{},query_end_ns{},query{},query_code{unknown},query_branch{unknown},callback_status{unknown},
      equipment_begin_ns{},equipment_end_ns{},mount{unknown},prepare_code{unknown},commit_code{unknown},
      held_counter{},trace_sequence{},normal_return{},guard_calls{},admission_calls{};
    // Existing-entry return observations only. UNKNOWN means unobserved.
    U native_begin{unknown},goal_reason{unknown},goal_source_available{},goal_key_available{},
      goal_initialized{},goal_poisoned{},goal_key_bits{unknown},goal_adapter{unknown};
    // Exact capture invocation only; no call leaves UNKNOWN, never zero-success.
    U capture_reason{unknown},capture_context_verdict{unknown};
    std::array<Guard,2> guards{};
    std::array<Admission,4> admission{};
    U prequery_gate{unknown},prequery_detail{unknown},prequery_cookie{unknown},prequery_parent_cookie{unknown},
      prequery_revision{unknown},prequery_generation{unknown};
};
struct Header {
    char magic[8]{'B','H','E','L','D','0','0','4'};
    U version{4},header_bytes{sizeof(Header)},record_bytes{sizeof(Record)},count{},limit{capacity},
      pid{},native_started{},clock_units_per_second{1000000000},armed_ns{},deadline_ns{},
      ignored_before_arm{},dropped_capacity{},nested{},foreign_thread{},arm_kind{};
};
static_assert(std::is_trivially_copyable_v<Record> && sizeof(Record)<1024);
using Clock=std::int64_t(*)() noexcept;
using Thread=U(*)() noexcept;
using Cookie=U(*)() noexcept;
struct Recorder {
    Clock clock{};Thread thread{};Cookie cookie{};
    std::atomic<bool> enabled{},active{};
    std::atomic<unsigned> saved{};
    std::atomic<U> owner_thread{},foreign_thread{};
    U armed_ns{},deadline_ns{},ignored_before_arm{},dropped_capacity{},nested{};
    bool prequery_enabled{};std::atomic<U> arm_kind{};std::atomic<bool> cookie_missing_saved{};
    Record current{};
    std::array<Record,capacity> rows{};
    bool here() const noexcept {return enabled.load(std::memory_order_acquire)&&thread&&
        active.load(std::memory_order_acquire)&&thread()==owner_thread.load(std::memory_order_acquire);}
    U now() const noexcept {const auto value=clock?clock():0;return value>0?U(value):0;}
    template<class Core> void begin(U entry,U compose,U tick,U actual_thread,Core const& source) noexcept {
        if(!enabled.load(std::memory_order_acquire)||!clock||!thread||!cookie||
           !actual_thread||thread()!=actual_thread||source.native.game_thread!=actual_thread)return;
        U unbound{};owner_thread.compare_exchange_strong(unbound,actual_thread,std::memory_order_acq_rel);
        if(owner_thread.load(std::memory_order_acquire)!=actual_thread){foreign_thread.fetch_add(1,std::memory_order_relaxed);return;}
        bool inactive=false;if(!active.compare_exchange_strong(inactive,true,std::memory_order_acq_rel)){++nested;return;}
        const auto parent=cookie();
        if(!parent){
            // First diagnostic-only pre-row failure. No active row, clock or arm.
            if(prequery_enabled&&!cookie_missing_saved.exchange(true,std::memory_order_acq_rel)){
                const auto count=saved.load(std::memory_order_relaxed);
                if(count<rows.size()){auto& row=rows[count];row={};
                    row.entry=entry;row.compose=compose;row.tick=tick;row.thread=actual_thread;row.cookie=0;
                    row.pid=source.native.game_pid;row.native_started=source.native.native_started_qpc;row.game_start=source.native.game_start;
                    row.world_generation=source.world_generation;row.source_ticket=source.source_ticket;
                    row.publication=source.publication_sequence;row.scene=source.scene_invocation;
                    row.original_published_ns=source.published_ns;row.original_scene_qpc=U(source.scene_qpc);row.original_scene_frequency=U(source.qpc_frequency);
                    row.prequery_gate=U(PreQueryGate::recorder_cookie_missing);row.prequery_detail=0;row.prequery_cookie=0;
                    saved.store(count+1,std::memory_order_release);
                }else ++dropped_capacity;
            }
            active.store(false,std::memory_order_release);return;
        }
        const auto stamp=now();if(!stamp||(deadline_ns&&stamp>deadline_ns)){active.store(false,std::memory_order_release);return;}
        current={};current.entry=entry;current.compose=compose;current.tick=tick;current.thread=actual_thread;
        current.cookie=parent;current.pid=source.native.game_pid;current.native_started=source.native.native_started_qpc;
        current.game_start=source.native.game_start;current.world_generation=source.world_generation;current.source_ticket=source.source_ticket;
        current.publication=source.publication_sequence;current.scene=source.scene_invocation;current.begin_ns=stamp;
        current.original_published_ns=source.published_ns;current.original_scene_qpc=U(source.scene_qpc);
        current.original_scene_frequency=U(source.qpc_frequency);
        // The actual GT owns the busy slot until its matching finish.
    }
    template<class Input> void input(Input const& source,std::int64_t capture) noexcept {
        if(!here())return;auto const& observed=source.observed;
        current.cache_revision=source.scene_cache_revision;current.tracking_begin_ns=U(observed.begin_ns);
        current.tracking_end_ns=U(observed.end_ns);current.stereo_received_ns=U(observed.stereo.received_ns);
        current.controller_received_ns=U(observed.controllers.received_ns);current.sync_ns=U(observed.controllers.sync_ns);
        current.capture_ns=U(capture);current.input_recorded=1;
    }
    bool exact(U entry,U compose) const noexcept {return here()&&current.entry==entry&&current.compose==compose;}
    void prequery_result(U entry,U compose,U gate,U detail,U observed_cookie,U parent_cookie,U revision,U generation) noexcept {
        if(!exact(entry,compose)||current.prequery_gate!=unknown)return;
        current.prequery_gate=gate;current.prequery_detail=detail;current.prequery_cookie=observed_cookie;
        current.prequery_parent_cookie=parent_cookie;current.prequery_revision=revision;current.prequery_generation=generation;
        // Entry begin is an existing observation, not a new branch/input timestamp.
        if(prequery_enabled&&gate!=0&&gate!=unknown&&!armed_ns&&current.begin_ns&&current.begin_ns<=UINT64_MAX-7000000000ULL){
            armed_ns=current.begin_ns;deadline_ns=armed_ns+7000000000ULL;arm_kind.store(2,std::memory_order_release);
        }
    }
    void native_result(U entry,U compose,NativeBegin reason) noexcept {
        if(exact(entry,compose))current.native_begin=U(reason);
    }
    void goal_result(U entry,U compose,U reason,bool source,bool key,bool initialized,bool poisoned,U bits,U adapter) noexcept {
        if(!exact(entry,compose)||current.goal_reason!=unknown)return;
        current.goal_reason=reason;current.goal_source_available=U(source);current.goal_key_available=U(key);
        current.goal_initialized=U(initialized);current.goal_poisoned=U(poisoned);current.goal_key_bits=bits;current.goal_adapter=adapter;
    }
    void capture_result(U entry,U compose,U reason,U context_verdict) noexcept {
        if(!exact(entry,compose)||current.capture_reason!=unknown)return;
        current.capture_reason=reason;current.capture_context_verdict=context_verdict;
    }
    U guard_begin() noexcept {
        if(!here())return unknown;
        const auto ordinal=current.guard_calls++;if(ordinal>=current.guards.size())return unknown;
        current.guards[unsigned(ordinal)].begin_ns=now();return ordinal;
    }
    void guard_end(U ordinal,SourceGate gate,U context_verdict=unknown) noexcept {
        if(!here()||ordinal>=current.guards.size())return;
        auto& result=current.guards[unsigned(ordinal)];result.gate=U(gate);result.context_verdict=context_verdict;result.end_ns=now();
    }
    void query_begin() noexcept {if(here())current.query_begin_ns=now();}
    void query_end(U id,U code,U branch,U callback_status,std::int64_t equipment_begin,std::int64_t equipment_end) noexcept {
        if(!here())return;
        current.query_end_ns=now();current.query=id;current.query_code=code;current.query_branch=branch;
        current.callback_status=callback_status;current.equipment_begin_ns=U(equipment_begin);current.equipment_end_ns=U(equipment_end);
        // PG ready is status 0. This arms observation only, never a write. Keep
        // the current row too, including a source-after rejection of that reply.
        if(!armed_ns&&callback_status==0&&current.query_end_ns>=current.begin_ns&&current.query_end_ns>0&&current.query_end_ns<=UINT64_MAX-7000000000ULL){
            armed_ns=current.query_end_ns;deadline_ns=armed_ns+7000000000ULL;arm_kind.store(1,std::memory_order_release);
        }
    }
    void admission(U verdict,bool passed,std::int64_t capture) noexcept {
        if(!here())return;
        const auto ordinal=current.admission_calls++;if(ordinal>=current.admission.size())return;
        current.admission[unsigned(ordinal)]={now(),verdict,U(passed),U(capture)};
    }
    void selected_mount(U value) noexcept {if(here())current.mount=value;}
    void prepared(U value) noexcept {if(here())current.prepare_code=value;}
    void committed(U value) noexcept {if(here())current.commit_code=value;}
    void held(U counter,U trace) noexcept {if(here()){current.held_counter=counter;current.trace_sequence=trace;}}
    void finish(U entry,U compose,bool returned) noexcept {
        if(!here()||current.entry!=entry||current.compose!=compose)return;
        current.end_ns=now();current.normal_return=U(returned);
        if(!armed_ns)++ignored_before_arm;
        else {const auto count=saved.load(std::memory_order_relaxed);
            if(count<rows.size()){rows[count]=current;saved.store(count+1,std::memory_order_release);}
            else ++dropped_capacity;
        }
        active.store(false,std::memory_order_release);
    }
};
inline Recorder recorder{};
} // namespace gears_body_held_outcome
