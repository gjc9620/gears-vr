#include "ui_hud_first_fault.h"
#include <cassert>
#include <string>
#include <thread>

using namespace gears_ui_fault;
static std::string dump(First const& first){
    FILE* file{};assert(tmpfile_s(&file)==0&&file);
    print(file,first);rewind(file);
    std::string text;char buffer[2048]{};
    while(fgets(buffer,sizeof(buffer),file))text+=buffer;
    fclose(file);return text;
}
int main(){
    First first;Row saved{};saved.qpc=99;
    assert(!first.snapshot(saved)&&saved.qpc==99);
    assert(dump(first)=="ui_hud_first_fault schema=2 available=0\n");
    Row row{};row.reason=Reason::finish;row.qpc=123;row.frequency=1000;row.thread=4;
    row.execute_sequence=8;row.candidate_generation=17;row.root_first=21;row.root_last=25;
    row.command=0x100;row.payload=0x200;row.cursor_expected=3;row.cursor_observed=2;
    row.cursor_poisoned=true;row.result_valid=true;row.result=48;
    row.expected_draw_valid=true;row.expected_draw={9,1,0,-2,0};
    row.observed_draw_valid=true;row.observed_draw={6,1,0,-2,0};row.ticket_valid=true;row.ticket=388;
    assert(first.record(row));row.qpc=456;row.candidate_generation=29;
    row.reason=Reason::callback;assert(!first.record(row));
    assert(first.snapshot(saved)&&saved.qpc==123&&saved.candidate_generation==17);
    assert(saved.reason==Reason::finish&&saved.root_first==21&&saved.root_last==25);
    assert(saved.ticket_valid&&saved.ticket==388&&saved.cursor_expected==3&&saved.cursor_observed==2);
    assert(saved.command==0x100&&saved.payload==0x200&&saved.expected_draw.base_vertex==-2);
    auto text=dump(first);
    assert(text.find("reason=finish qpc=123 qpc_frequency=1000")!=std::string::npos);
    assert(text.find("cursor_expected=3 cursor_observed=2 cursor_poisoned=1")!=std::string::npos);
    assert(text.find("result_valid=1 result=48")!=std::string::npos);
    assert(text.find("expected_draw=9,1,0,-2,0")!=std::string::npos);
    assert(text.find("ticket_valid=1 ticket=388 ticket_scope=last_completed_overlay_in_same_execute_not_rejected_draw")!=std::string::npos);
    assert(text.find('\n')==text.size()-1); // Exactly one existing-status row.
    First missing;row.ticket_valid=false;row.ticket=999;row.result_valid=false;
    assert(missing.record(row)&&missing.snapshot(saved)&&!saved.ticket_valid&&saved.ticket==0);
    assert(dump(missing).find("reason=callback")!=std::string::npos);
    First zero;row.ticket_valid=true;row.ticket=0;row.reason=Reason::return_value;row.result_valid=true;row.result=7;
    assert(zero.record(row)&&zero.snapshot(saved)&&!saved.ticket_valid&&saved.ticket==0);
    assert(dump(zero).find("reason=return")!=std::string::npos);
    // Real cursors can end with identical poisoned/count states for two
    // different histories: missing tail versus an earlier malformed callback.
    // A zero-index tail still fails finish; diagnostics never waive that guard.
    gears_ui_indices::Snapshot snapshot{};snapshot.payload=0x1000;
    snapshot.vertex_count=1;snapshot.segment_count=2;
    for(auto& segment:snapshot.segments){segment.minimum=0;segment.maximum=0;segment.expected={0,1,0,0,0};}
    gears_ui_indices::Cursor tail(snapshot),prepoison(snapshot);
    unsigned ordinal{};assert(tail.consume({0,1,0,0,0},ordinal));
    assert(prepoison.consume({0,1,0,0,0},ordinal));
    assert(!prepoison.consume({3,1,0,0,0},ordinal));
    Row tail_row{},poison_row{};
    tail_row.reason=poison_row.reason=Reason::finish;
    tail_row.finish={true,tail.poisoned(),tail.expected_count(),tail.observed(),true,false};
    poison_row.finish={true,prepoison.poisoned(),prepoison.expected_count(),prepoison.observed(),true,false};
    assert(!tail.finish()&&!prepoison.finish());
    tail_row.cursor_poisoned=tail.poisoned();poison_row.cursor_poisoned=prepoison.poisoned();
    tail_row.cursor_expected=tail.expected_count();poison_row.cursor_expected=prepoison.expected_count();
    tail_row.cursor_observed=tail.observed();poison_row.cursor_observed=prepoison.observed();
    assert(tail_row.cursor_poisoned&&poison_row.cursor_poisoned);
    assert(tail_row.cursor_expected==poison_row.cursor_expected&&tail_row.cursor_observed==poison_row.cursor_observed);
    First late_tail,early_poison;assert(late_tail.record(tail_row)&&early_poison.record(poison_row));
    assert(late_tail.snapshot(saved)&&saved.finish.before_finish_valid&&!saved.finish.before_finish_poisoned);
    assert(saved.finish.before_finish_expected==saved.cursor_expected&&saved.finish.before_finish_observed==saved.cursor_observed);
    assert(dump(late_tail).find("before_finish_valid=1 before_finish_poisoned=0 before_finish_expected=2 before_finish_observed=1")!=std::string::npos);
    assert(dump(early_poison).find("before_finish_valid=1 before_finish_poisoned=1")!=std::string::npos);
    // Binding failure is independently retained even when all callbacks finish.
    gears_ui_indices::Cursor good(snapshot);
    assert(good.consume({0,1,0,0,0},ordinal)&&good.consume({0,1,0,0,0},ordinal));
    Row binding_row{};binding_row.reason=Reason::return_value;binding_row.result_valid=true;binding_row.result=7;
    binding_row.finish={true,good.poisoned(),good.expected_count(),good.observed(),true,false};
    binding_row.sequence_complete=good.finish();binding_row.cursor_poisoned=good.poisoned();
    First binding_failed;assert(binding_failed.record(binding_row)&&binding_failed.snapshot(saved));
    assert(saved.sequence_complete&&!saved.cursor_poisoned&&saved.finish.binding_complete_valid&&!saved.finish.binding_complete);
    assert(dump(binding_failed).find("binding_complete_valid=1 binding_complete=0")!=std::string::npos);
    First binding_ok;binding_row.finish.binding_complete=true;assert(binding_ok.record(binding_row));
    assert(dump(binding_ok).find("binding_complete_valid=1 binding_complete=1")!=std::string::npos);
    // Callback wins the first-row race before end is known. A later finish
    // cannot fill in or overwrite those unknown values.
    First callback_first;Row callback{};callback.reason=Reason::callback;
    assert(callback_first.record(callback)&&!callback_first.record(binding_row)&&callback_first.snapshot(saved));
    assert(!saved.finish.before_finish_valid&&!saved.finish.binding_complete_valid);
    assert(dump(callback_first).find("before_finish_valid=0 before_finish_poisoned=0")!=std::string::npos);
    assert(dump(callback_first).find("binding_complete_valid=0 binding_complete=0")!=std::string::npos);
    First raced;std::array<std::thread,8> workers;
    for(unsigned i=0;i<workers.size();++i)workers[i]=std::thread([&,i]{
        Row value{};value.execute_sequence=i+1;value.candidate_generation=(i+1)*10;
        value.qpc=100+static_cast<long long>(i);raced.record(value);
    });
    for(auto& worker:workers)worker.join();
    assert(raced.snapshot(saved)&&saved.execute_sequence>=1&&saved.execute_sequence<=8);
    assert(saved.candidate_generation==saved.execute_sequence*10);
    assert(saved.qpc==99+static_cast<long long>(saved.execute_sequence));
    assert(!raced.record(row));
}
