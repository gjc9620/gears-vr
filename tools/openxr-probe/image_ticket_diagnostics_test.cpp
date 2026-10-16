#include "image_ticket_diagnostics.h"
#include <cassert>
#include <cstring>
#include <string>
using image_ticket_diagnostics::Recorder;
using image_ticket_diagnostics::Row;
static Row frame(std::uint64_t id,bool submitted,bool fresh,std::uint64_t session=7){
    Row r;r.ns=1000000000+static_cast<std::int64_t>(id);r.status=submitted?"submitted":"stale_ticket";
    r.submitted=submitted;r.new_image_valid=true;r.new_image=fresh;
    r.image.receiver_valid=r.image.ticket_valid=true;r.image.ticket_id=id;r.image.session=session;
    r.image.sequence=id+100;r.image.received_ns=r.ns-5;r.image.source=1;r.image.head_flags=r.image.view_flags=15;
    r.image.head_position={.125,-2.5,3.75};r.image.head_orientation={.5,-.25,.125,.8125};
    r.image.acquired_serial=id+200;r.image.copies=static_cast<unsigned>(id);return r;
}
static std::string dumped(Recorder& recorder){
    FILE* file{};assert(!tmpfile_s(&file)&&file);recorder.dump(file);recorder.dump(file);rewind(file);
    std::string text;char buf[2048];while(fgets(buf,sizeof(buf),file))text+=buf;fclose(file);return text;
}
int main(int argc,char** argv){
    Recorder disabled;disabled.observe(frame(1,true,true));assert(!disabled.enabled()&&!disabled.count()&&!disabled.total());
    assert(dumped(disabled).empty());
    Recorder receipts;assert(receipts.enable());
    auto first=frame(1,false,true);const auto frozen_position=first.image.head_position;
    const auto frozen_orientation=first.image.head_orientation;
    receipts.observe(first);first.image.ticket_id=999;first.image.head_position={99,98,97};first.image.head_orientation={1,0,0,0};
    assert(receipts.count()==1&&receipts.row(0)->image.ticket_id==1&&!receipts.row(0)->submitted);
    assert(receipts.row(0)->image.head_position==frozen_position&&receipts.row(0)->image.head_orientation==frozen_orientation);
    receipts.observe(frame(1,true,false));assert(receipts.count()==2&&receipts.row(1)->submitted);
    receipts.observe(frame(1,true,false));assert(receipts.count()==2); // Repeated cache submission.
    receipts.observe(frame(2,false,true));assert(receipts.count()==3);
    receipts.observe(frame(2,false,false));assert(receipts.count()==3); // Rejection does not commit key.
    receipts.observe(frame(2,true,false));assert(receipts.count()==4);
    receipts.observe(frame(2,true,true));assert(receipts.count()==5); // Every actual new copy retained.
    receipts.observe(frame(3,true,false));assert(receipts.count()==6); // New successful key without copy.
    receipts.observe(frame(3,true,false,8));assert(receipts.count()==7); // Session is part of identity.
    auto unobserved=frame(4,false,true);unobserved.new_image_valid=false;receipts.observe(unobserved);
    assert(receipts.count()==7);
    auto terminal=frame(3,false,false,8);terminal.terminal=true;terminal.status="producer_stopped";
    receipts.observe(terminal);assert(receipts.count()==7);
    auto bad=frame(0,true,false);bad.image.ticket_valid=false;receipts.observe(bad);
    assert(receipts.count()==8&&!receipts.row(7)->image.ticket_valid);
    receipts.observe(frame(3,true,false,8));assert(receipts.count()==9); // Invalid success never hides valid receipt.
    assert(receipts.enable()&&receipts.count()==9&&!receipts.row(9)); // Enabling twice never resets evidence.
    const auto text=dumped(receipts);
    assert(text.find("\"image_ticket_diagnostics\":true")!=std::string::npos);
    assert(text.find("\"schema\":2")!=std::string::npos);
    assert(text.find("\"pose_semantics\":\"frozen_native_image_ticket_pose_not_current_runtime_pose\"")!=std::string::npos);
    assert(text.find("\"head_position\":[0.125,-2.5,3.75],\"head_orientation\":[0.5,-0.25,0.125,0.8125]")!=std::string::npos);
    assert(text.find("\"total_rows\":9,\"retained_rows\":9,\"dropped_rows\":0")!=std::string::npos);
    assert(text.find("\"invalid_submitted_ticket_rows\":1")!=std::string::npos);
    assert(text.find("\"ticket_id\":1,\"session\":7,\"sequence\":101")!=std::string::npos);
    assert(text.find("\"acquired_serial\":201,\"copy_count\":1")!=std::string::npos);
    assert(text.find("outage_row")==std::string::npos&&text.find("\"retention_complete\":true")!=std::string::npos);
    receipts.observe(frame(10,true,true));assert(receipts.count()==9); // Dump is terminal and idempotent.
    Recorder failed_end;assert(failed_end.enable());
    auto failed=frame(10,false,true);failed.status="session_lost";
    failed_end.observe(failed);assert(failed_end.count()==1&&!failed_end.row(0)->submitted);
    failed_end.observe(frame(10,true,false));assert(failed_end.count()==2&&failed_end.row(1)->submitted);
    Recorder integers;assert(integers.enable());
    auto wide=frame(1,true,true);wide.image.ticket_id=18446744073709551614ULL;
    wide.image.session=9007199254740993ULL;wide.image.sequence=9007199254740995ULL;
    wide.ns=9223372036854775806LL;wide.image.received_ns=9223372036854775801LL;
    wide.image.head_position[0]=1.0000000000000002;
    integers.observe(wide);const auto integer_text=dumped(integers);
    assert(integer_text.find("\"ticket_id\":18446744073709551614,\"session\":9007199254740993,\"sequence\":9007199254740995")!=std::string::npos);
    assert(integer_text.find("\"qpc_ns\":9223372036854775806")!=std::string::npos);
    assert(integer_text.find("\"head_position\":[1.0000000000000002,")!=std::string::npos);
    Recorder overflow;assert(overflow.enable());
    for(unsigned i=1;i<=Recorder::capacity+3;++i)overflow.observe(frame(i,true,true));
    assert(overflow.count()==Recorder::capacity&&overflow.total()==Recorder::capacity+3&&overflow.dropped()==3);
    overflow.observe(frame(Recorder::capacity+3,true,false));assert(overflow.dropped()==3); // Key updates even if full.
    assert(overflow.row(0)->image.ticket_id==1&&overflow.row(Recorder::capacity-1)->image.ticket_id==Recorder::capacity);
    auto overflow_text=dumped(overflow);assert(overflow_text.find("\"retention_complete\":false")!=std::string::npos);
    if(argc==2&&!strcmp(argv[1],"--dump-fixtures")){fputs(text.c_str(),stdout);fputs(integer_text.c_str(),stdout);}
    printf("image ticket diagnostics disabled/copy/cache/session/invalid/overflow/output checks passed; no XR/GPU opened\n");
}
