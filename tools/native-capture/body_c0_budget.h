#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

// Observation only. No object reads, clock renewal, policy, locks or IO.
namespace gears_body_c0_budget {
enum class Stage:unsigned {compose_entry,compose_admitted,native_entry,capture_begin,capture_end,model_begin,model_end,
    goal_entry,goal_initial_end,goal_adapter_end,goal_final_end,apply_entry,apply_gate_end,body_plan_end,
    query_begin,query_end,post_current_end,post40_end,hands_end,final40_end,prepare_end,commit_end,finally_end,count};
enum class Op:unsigned {context,owner_actor,metadata_initial,metadata_repeat,read_tick,geometry,standing,count};
struct Record {
    std::uint64_t entry{},compose{},tick{},tick_begin_qpc{},scene{},publication{},cookie{},native_started{},pid{},thread{},
        published_ns{},scene_qpc{},scene_frequency{},stereo_received_ns{},controller_received_ns{},sync_ns{},
        query{},query_code{},query_branch{},no_query_age{},returned{},admitted{};
    std::array<std::int64_t,unsigned(Stage::count)> time{};
    std::array<std::uint64_t,unsigned(Op::count)> calls{},duration_ns{};
};
struct Header {
    char magic[8]{'C','0','B','U','D','G','0','1'};
    std::uint32_t version{1},header_bytes{sizeof(Header)},record_bytes{sizeof(Record)},query_count{},capacity{32},no_query_count{};
    std::uint64_t pid{},native_started{},clock_units_per_second{1000000000},attempts{},ignored{},nested{};
};
static_assert(std::is_trivially_copyable_v<Record> && sizeof(Record)==472 && sizeof(Header)==80);
using Clock=std::int64_t(*)() noexcept;using Cookie=std::uint64_t(*)() noexcept;using Thread=std::uint64_t(*)() noexcept;
struct Recorder {
    Clock clock{};Cookie cookie{};Thread thread{};std::atomic<bool> enabled{};
    Record current{},no_query{};std::array<Record,32> queries{};
    std::atomic<bool> active{},no_query_saved{};std::atomic<std::uint64_t> owner_thread{};
    std::atomic<unsigned> saved{};
    std::atomic<std::uint64_t> attempts{},ignored{},nested{};
    bool here() const noexcept {return enabled.load(std::memory_order_acquire)&&thread&&thread()==owner_thread.load(std::memory_order_acquire)&&active.load(std::memory_order_acquire);}
    std::int64_t now() const noexcept {return clock?clock():0;}
    void mark(Stage stage) noexcept {if(here())current.time[unsigned(stage)]=now();}
    void mark_entry(Stage stage,std::uintptr_t entry,std::uint64_t compose) noexcept {
        if(here()&&current.entry==entry&&current.compose==compose)mark(stage);
    }
    template<class Core> void begin(std::uintptr_t entry,std::uint64_t compose,std::uint64_t tick,std::int64_t tick_qpc,
        std::uint64_t actual_thread,Core const& source) noexcept {
        if(!enabled.load(std::memory_order_acquire)||!clock||!cookie||!thread||thread()!=actual_thread||!actual_thread||source.native.game_thread!=actual_thread)return;
        const auto parent=cookie();if(!parent)return;
        if(active.load(std::memory_order_acquire)){++nested;return;}
        if(saved.load(std::memory_order_acquire)>=queries.size()&&no_query_saved.load(std::memory_order_acquire))return;
        ++attempts;owner_thread.store(actual_thread,std::memory_order_release);current={};current.entry=entry;current.compose=compose;current.tick=tick;current.tick_begin_qpc=std::uint64_t(tick_qpc);
        current.cookie=parent;current.thread=actual_thread;current.scene=source.scene_invocation;current.publication=source.publication_sequence;
        current.native_started=source.native.native_started_qpc;current.pid=source.native.game_pid;current.published_ns=source.published_ns;
        current.scene_qpc=std::uint64_t(source.scene_qpc);current.scene_frequency=std::uint64_t(source.qpc_frequency);
        current.stereo_received_ns=std::uint64_t(source.stereo.received_ns);current.controller_received_ns=std::uint64_t(source.controllers.received_ns);
        current.sync_ns=std::uint64_t(source.controllers.sync_ns);active.store(true,std::memory_order_release);mark(Stage::compose_entry);
    }
    void admitted() noexcept {if(here()){current.admitted=1;mark(Stage::compose_admitted);}}
    void age_rejected() noexcept {if(here())current.no_query_age=1;}
    void query_result(std::uint64_t id,unsigned code,unsigned branch) noexcept {if(here()){current.query=id;current.query_code=code;current.query_branch=branch;}}
    void finish(std::uintptr_t entry,std::uint64_t compose,bool returned) noexcept {
        if(!here()||current.entry!=entry||current.compose!=compose)return;
        mark(Stage::finally_end);current.returned=returned?1:0;
        const auto count=saved.load(std::memory_order_relaxed);
        if(current.time[unsigned(Stage::query_begin)]&&count<queries.size()){
            queries[count]=current;saved.store(count+1,std::memory_order_release);
        }else if(!current.time[unsigned(Stage::query_begin)]&&current.no_query_age&&!no_query_saved.load(std::memory_order_relaxed)){
            no_query=current;no_query_saved.store(true,std::memory_order_release);
        }else ++ignored;
        active.store(false,std::memory_order_release);
    }
};
inline Recorder recorder{};
struct Span {
    Op op;std::uint64_t entry{},compose{},owner{};std::int64_t start{};
    explicit Span(Op value) noexcept:op(value){auto& r=recorder;if(r.here()){entry=r.current.entry;compose=r.current.compose;owner=r.current.thread;start=r.now();++r.current.calls[unsigned(op)];}}
    ~Span(){auto& r=recorder;if(start>0&&r.here()&&r.current.entry==entry&&r.current.compose==compose&&r.current.thread==owner){
        const auto end=r.now();if(end>=start)r.current.duration_ns[unsigned(op)]+=std::uint64_t(end-start);}}
};
template<class F> auto observe(Op op,F&& fn)->decltype(fn()){Span span(op);return fn();}
template<class F> auto bracket(Stage begin,Stage end,F&& fn)->decltype(fn()){
    recorder.mark(begin);const auto result=fn();recorder.mark(end);return result;
}
} // namespace gears_body_c0_budget
