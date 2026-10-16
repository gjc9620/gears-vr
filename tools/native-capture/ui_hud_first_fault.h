#pragma once
#include "ui_index_membership.h"
#include <atomic>
#include <cstdio>
#include <mutex>

namespace gears_ui_fault {
enum class Reason : unsigned { finish, callback, return_value };
inline char const* reason_name(Reason reason){
    switch(reason){case Reason::finish:return "finish";case Reason::callback:return "callback";
        case Reason::return_value:return "return";}return "unknown";
}
struct FinishEvidence {
    bool before_finish_valid{},before_finish_poisoned{};
    unsigned before_finish_expected{},before_finish_observed{};
    // This is semantic_scope.end's result, not an independent binding probe:
    // end(false) short-circuits its final binding read.
    bool binding_complete_valid{},binding_complete{};
};
// Owning diagnostics only. No native command/payload or cursor pointers survive.
struct Row {
    Reason reason{};
    long long qpc{},frequency{};
    unsigned thread{};
    std::uint64_t execute_sequence{},candidate_generation{},root_first{},root_last{};
    std::uintptr_t command{},payload{};
    unsigned cursor_expected{},cursor_observed{};
    bool cursor_poisoned{},sequence_complete{},result_valid{};
    std::uint32_t result{};
    bool expected_draw_valid{},observed_draw_valid{};
    gears_ui_indices::DrawArgs expected_draw{},observed_draw{};
    std::uint64_t ticket{};
    bool ticket_valid{};
    FinishEvidence finish{};
};
class First {
    std::atomic<bool> published_{};
    mutable std::mutex mutex_;
    Row row_{};
public:
    bool published() const {return published_.load(std::memory_order_acquire);}
    bool record(Row const& row){
        if(published())return false;
        std::lock_guard<std::mutex> lock(mutex_);
        if(published())return false;
        row_=row;
        if(!row_.ticket_valid||!row_.ticket){row_.ticket=0;row_.ticket_valid=false;}
        published_.store(true,std::memory_order_release);return true;
    }
    bool snapshot(Row& out) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!published())return false;
        out=row_;return true;
    }
};
inline void print(FILE* file,First const& first){
    Row row{};
    if(!first.snapshot(row)){std::fprintf(file,"ui_hud_first_fault schema=2 available=0\n");return;}
    std::fprintf(file,"ui_hud_first_fault schema=2 available=1 reason=%s qpc=%lld qpc_frequency=%lld thread=%u execute_sequence=%llu candidate_generation=%llu root_first=%llu root_last=%llu command=%llx payload=%llx cursor_expected=%u cursor_observed=%u cursor_poisoned=%u sequence_complete=%u result_valid=%u result=%u expected_draw_valid=%u expected_draw=%u,%u,%u,%d,%u observed_draw_valid=%u observed_draw=%u,%u,%u,%d,%u ticket_valid=%u ticket=%llu ticket_scope=last_completed_overlay_in_same_execute_not_rejected_draw before_finish_valid=%u before_finish_poisoned=%u before_finish_expected=%u before_finish_observed=%u binding_complete_valid=%u binding_complete=%u binding_complete_semantics=semantic_scope_end_result_not_independent_probe diagnostic_only=1\n",
        reason_name(row.reason),row.qpc,row.frequency,row.thread,
        static_cast<unsigned long long>(row.execute_sequence),static_cast<unsigned long long>(row.candidate_generation),
        static_cast<unsigned long long>(row.root_first),static_cast<unsigned long long>(row.root_last),
        static_cast<unsigned long long>(row.command),static_cast<unsigned long long>(row.payload),
        row.cursor_expected,row.cursor_observed,unsigned(row.cursor_poisoned),unsigned(row.sequence_complete),
        unsigned(row.result_valid),row.result,unsigned(row.expected_draw_valid),row.expected_draw.index_count,
        row.expected_draw.instance_count,row.expected_draw.start_index,row.expected_draw.base_vertex,row.expected_draw.first_instance,
        unsigned(row.observed_draw_valid),row.observed_draw.index_count,row.observed_draw.instance_count,
        row.observed_draw.start_index,row.observed_draw.base_vertex,row.observed_draw.first_instance,
        unsigned(row.ticket_valid),static_cast<unsigned long long>(row.ticket),unsigned(row.finish.before_finish_valid),
        unsigned(row.finish.before_finish_poisoned),row.finish.before_finish_expected,row.finish.before_finish_observed,
        unsigned(row.finish.binding_complete_valid),unsigned(row.finish.binding_complete));
}
}
