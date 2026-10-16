#include "outage_diagnostics.h"
#include <cassert>
#include <cstring>
#include <string>
using namespace outage_diagnostics;
static Row frame(std::int64_t ns,bool submitted=false,bool fresh=false){
    Row r;r.ns=ns;r.submitted=submitted;r.new_image=fresh;r.new_image_valid=true;r.status=submitted?"submitted":"stale_ticket";return r;
}
static void composition_tests(){
    Row projection=frame(1,true,true);projection.terminal=true;
    projection.image={true,true,7,8,9,10,11,2,3,15,31};
    char kind[]="eye_menu";
    auto copied=composition_row(projection,true,2,kind);kind[0]='X';
    assert(copied.submitted&&copied.layer_count==2&&!strcmp(copied.layer_kind.data(),"eye_menu"));
    assert(copied.ns==projection.ns&&copied.status==projection.status&&copied.terminal==projection.terminal&&
        copied.new_image==projection.new_image&&copied.new_image_valid==projection.new_image_valid);
    assert(copied.image.receiver_valid&&copied.image.ticket_valid&&copied.image.acquired_serial==7&&
        copied.image.ticket_id==8&&copied.image.session==9&&copied.image.sequence==10&&
        copied.image.received_ns==11&&copied.image.copies==2&&copied.image.source==3&&
        copied.image.head_flags==15&&copied.image.view_flags==31);
    assert(projection.submitted&&projection.layer_count==0&&!strcmp(projection.layer_kind.data(),"none"));
    const auto failed=composition_row(projection,false,2,"projection");
    assert(!failed.submitted&&failed.layer_count==2&&!strcmp(failed.layer_kind.data(),"projection"));
    assert(!composition_row(projection,true,0,"none").submitted);
    assert(!composition_row(projection,false,0,nullptr).submitted);
    assert(!strcmp(composition_row(projection,true,1,nullptr).layer_kind.data(),"none"));
    char long_kind[80]{};for(unsigned i=0;i<79;++i)long_kind[i]='a';
    const auto bounded=composition_row(projection,true,1,long_kind);
    assert(bounded.layer_kind.back()==0&&strlen(bounded.layer_kind.data())==31);
    Recorder composed;composed.enabled=true;
    const char* kinds[]{"projection","eye_menu","startup_quad"};
    // Normal menu composition can persist far beyond the first-window capacity
    // without consuming it: every successful nonempty EndFrame counts.
    for(unsigned i=0;i<300;++i){const unsigned mode=i%3;
        composed.observe(composition_row(frame(1+std::int64_t(i)*50000000,mode==0,mode==0),true,mode==1?2:1,kinds[mode]));}
    assert(!composed.first_started&&composed.count==0&&composed.dropped==0);
    const auto last=1+std::int64_t(299)*50000000;
    composed.observe(composition_row(frame(last+50000000),true,0,"none"));
    composed.observe(composition_row(frame(last+100000001),true,0,"none"));
    assert(composed.first_started&&!composed.complete());
    composed.observe(composition_row(frame(last+150000000),true,2,"eye_menu"));
    assert(composed.complete()&&composed.longest_gap().end_valid);
    assert(composed.longest_gap().first_missing.layer_count==0&&!composed.longest_gap().first_missing.submitted);
    assert(composed.longest_gap().end.layer_count==2&&!strcmp(composed.longest_gap().end.layer_kind.data(),"eye_menu"));
    Recorder failed_end;failed_end.enabled=true;
    failed_end.observe(composition_row(frame(1,true),true,1,"projection"));
    failed_end.observe(composition_row(frame(100000002,true),false,1,"startup_quad"));
    assert(failed_end.first_started&&!failed_end.longest_gap().end_valid);
    failed_end.observe(composition_row(frame(120000001),true,1,"startup_quad"));
    assert(failed_end.complete()&&failed_end.longest_gap().end_valid);
    FILE* file{};assert(tmpfile_s(&file)==0&&file);composed.dump(file);rewind(file);
    std::string text;char buffer[2048];while(fgets(buffer,sizeof(buffer),file))text+=buffer;fclose(file);
    assert(text.find("\"schema\":3")!=std::string::npos);
    assert(text.find("\"submission_semantics\":\"successful_nonempty_xrEndFrame_not_pixel_validation\"")!=std::string::npos);
    assert(text.find("\"layer_count\":0,\"layer_kind\":\"none\"")!=std::string::npos);
    assert(text.find("\"layer_count\":2,\"layer_kind\":\"eye_menu\"")!=std::string::npos);
}
static void menu_snapshot_tests(){
    MenuSnapshot observed;observed.cached_only=false;observed.render_attempted=true;
    observed.update_attempted=true;observed.updated=true;observed.available_checked=true;
    observed.available=true;observed.current=true;observed.fresh_checked=true;observed.fresh=true;
    observed.fresh_now_100ns=1234;observed.generation_changed=true;
    observed.reason=MenuRenderReason::rendered;
    observed.capture={CaptureResult::accepted,10,3,7,42,99,101,100,101};
    auto skipped=menu_snapshot_for_frame(observed,false);
    assert(skipped.cached_only&&!skipped.render_attempted&&!skipped.update_attempted&&!skipped.updated);
    assert(!skipped.available_checked&&!skipped.available&&!skipped.current&&!skipped.fresh_checked&&!skipped.fresh);
    assert(skipped.fresh_now_100ns==0&&!skipped.generation_changed&&skipped.reason==MenuRenderReason::not_attempted);
    assert(skipped.capture.result==CaptureResult::accepted&&skipped.capture.attempts==10&&skipped.capture.no_frames==3);
    assert(skipped.capture.accepted==7&&skipped.capture.generation==42&&skipped.capture.hwnd==99);
    assert(skipped.capture.last_taken_100ns==101&&skipped.capture.epoch_100ns==100&&skipped.capture.accepted_100ns==101);
    Row row=frame(100,true);row.menu=menu_snapshot_for_frame(observed,true);
    row.menu_mode=3;row.session_state=5;row.should_render=true;row.panel_requested=true;
    row.render_attempted=true;row.render_result=true;row.authorization_rejected=true;
    auto copied=composition_row(row,true,1,"startup_panel");
    observed.capture.attempts=1000;observed.reason=MenuRenderReason::window_unavailable;
    assert(copied.menu.capture.attempts==10&&copied.menu.fresh_now_100ns==1234);
    assert(copied.menu.reason==MenuRenderReason::rendered&&copied.menu.generation_changed);
    assert(copied.menu_mode==3&&copied.session_state==5&&copied.should_render&&copied.panel_requested);
    assert(copied.render_attempted&&copied.render_result&&copied.authorization_rejected);
    FILE* file{};assert(tmpfile_s(&file)==0&&file);
    Recorder::print_row(file,"test","render",copied);
    row.menu=skipped;row.render_attempted=false;row.render_result=false;row.authorization_rejected=false;
    Recorder::print_row(file,"test","skipped",row);rewind(file);
    std::string text;char buffer[2048];while(fgets(buffer,sizeof(buffer),file))text+=buffer;fclose(file);
    assert(text.find("\"fresh_now_100ns\":1234")!=std::string::npos);
    assert(text.find("\"reason\":\"rendered\"")!=std::string::npos);
    assert(text.find("\"reason\":\"not_attempted\"")!=std::string::npos);
    assert(text.find("\"cached_only\":true,\"render_attempted\":false")!=std::string::npos);
    assert(text.find("\"capture_result\":\"accepted\",\"capture_attempts\":10")!=std::string::npos);
    assert(text.find("\"menu_authorization_rejected\":true")!=std::string::npos);
    for(unsigned i=0;i<=static_cast<unsigned>(CaptureResult::window_unavailable);++i)
        assert(capture_result_name(static_cast<CaptureResult>(i))[0]);
    for(unsigned i=0;i<=static_cast<unsigned>(MenuRenderReason::rendered);++i)
        assert(menu_render_reason_name(static_cast<MenuRenderReason>(i))[0]);
}
int main(int argc,char** argv){
    composition_tests();menu_snapshot_tests();
    Recorder disabled;disabled.observe(frame(1,true));assert(disabled.count==0&&disabled.longest_gap().duration_ns==0);
    Recorder before;before.enabled=true;before.observe(frame(1));before.observe(frame(900000001));assert(!before.first_started);
    Recorder exact;exact.enabled=true;exact.observe(frame(1,true,true));exact.observe(frame(100000001));assert(!exact.first_started);
    exact.observe(frame(100000002));assert(exact.first_started&&exact.count==2&&!exact.complete());
    exact.observe(frame(110000001,true,true));assert(exact.complete()&&exact.longest_gap().end_valid);
    Recorder cache;cache.enabled=true;auto cached=frame(1,true,false);cached.image.receiver_valid=true;cached.image.ticket_valid=true;cached.image.ticket_id=9;
    cached.image.received_ns=1;cached.image.copies=2;cached.image.acquired_serial=7;
    cache.observe(cached);auto second=cached;second.ns=50000001;cache.observe(second);cache.observe(frame(160000002));
    assert(cache.rows[0].submitted&&!cache.rows[0].new_image&&cache.rows[0].image.acquired_serial==7);
    Recorder overflow;overflow.enabled=true;overflow.observe(frame(1,true));
    for(int i=0;i<400;++i)overflow.observe(frame(100000002+i));
    overflow.observe(frame(200000001,true,true));assert(overflow.count==249&&overflow.dropped>0&&!overflow.complete());
    assert(overflow.longest_gap().end_valid&&overflow.longest_gap().end.new_image);
    overflow.observe(frame(400000002));overflow.observe(frame(410000002,true));assert(overflow.later_windows==1&&!overflow.complete());
    assert(overflow.first_start.ns==1&&overflow.longest_gap().start.ns==200000001);
    Recorder front;front.enabled=true;front.observe(frame(1,true));
    for(int i=1;i<=20;++i)front.observe(frame(1+i*6000000));
    assert(front.first_start.ns==1&&front.rows[0].ns>1);
    Recorder ongoing;ongoing.enabled=true;ongoing.observe(frame(1,true));ongoing.observe(frame(100000002));
    const auto gap=ongoing.longest_gap().duration_ns;auto stop=frame(900000001);stop.status="producer_stopped";stop.terminal=true;stop.new_image_valid=false;
    ongoing.observe(stop);assert(ongoing.longest_gap().duration_ns==gap&&!ongoing.longest_gap().end_valid&&!ongoing.complete());
    Recorder leap;leap.enabled=true;leap.observe(frame(1,true));leap.observe(frame(150000001,true,true));
    assert(leap.complete()&&leap.first_started&&!leap.longest_gap().first_missing_valid);
    FILE* file{};assert(tmpfile_s(&file)==0&&file);ongoing.dump(file);ongoing.dump(file);rewind(file);std::string text;char buf[2048];
    while(fgets(buf,sizeof(buf),file))text+=buf;fclose(file);
    assert(text.find("\"ticket_valid\":false")!=std::string::npos);
    assert(text.find("\"ticket_id\"")==std::string::npos);
    assert(text.find("termination_excluded_from_gap")!=std::string::npos);
    assert(text.find("\"complete\":false")!=std::string::npos);
    if(argc==2&&!strcmp(argv[1],"--dump-fixtures")){cache.dump();overflow.dump();leap.dump();}
    printf("outage diagnostics boundary/cache/overflow/recovery/ongoing/termination tests passed; no XR/GPU opened\n");
}
