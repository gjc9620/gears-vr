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
    assert(dump(first)=="ui_hud_first_fault schema=1 available=0\n");
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
