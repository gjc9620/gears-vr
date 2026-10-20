// D3D12 pose, test-pattern and HWND game-panel diagnostics; no game mutation.
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <limits>
#include <share.h>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <string>
#include "../vr-common/pose_bridge.h"
#include "reference_space_epoch.h"
#include "../vr-common/stereo_bridge.h"
#include "../vr-common/render_size_bridge.h"
#include "continuous_session.h"
#include "shutdown_diagnostics.h"
#include "session_exit_state.h"
#include "image_ticket_diagnostics.h"
#include "startup_panel_policy.h"
#include "window_fallback_policy.h"
#include "menu_display_reader.h"
#include "xr_wait_diagnostics.h"

using Microsoft::WRL::ComPtr;
XrInstance instance = XR_NULL_HANDLE;
XrSession session = XR_NULL_HANDLE;
XrSpace local = XR_NULL_HANDLE;
XrSpace view_space = XR_NULL_HANDLE;
bool xr_gpu_teardown_unsafe=false;
void xr_check(XrResult result, const char* action) {
    if (XR_FAILED(result)) {
        char text[XR_MAX_RESULT_STRING_SIZE]{};
        if(instance) xrResultToString(instance,result,text);
        throw std::runtime_error(std::string(action)+": "+text+" ("+std::to_string(result)+")");
    }
}
void dx_check(HRESULT result, const char* action) {
    if (FAILED(result)) throw std::runtime_error(std::string(action)+": "+std::to_string(result));
}
#include "test_pattern.h"
#include "game_screen.h"
#include "game_gpu_screen.h"
#include "transition_card.h"
// Cumulative, render-thread-only diagnostics: one row per second plus final.
// Cached packet age is evidence, never a second display-authority decision.
struct MovieFallbackDiagnostics {
    uint64_t eligible{},render_true{},render_false{},post_context_rejected{},post_fresh_rejected{};
    uint64_t mode_changes{},movie_entries{},movie_exits{},expired_lease_frames{},submitted{},end_failed{};
    uint64_t render_reasons[8]{},capture_results[10]{};
    unsigned last_mode{};
    std::int64_t next_report{},maximum_render_ns{},last_packet_age_ns{-1};
    outage_diagnostics::MenuSnapshot last_render{};
    GameScreen::WindowSnapshot last_window{};
    void mode(gears_menu::Mode value){
        const auto current=unsigned(value);if(current==last_mode)return;
        ++mode_changes;if(current==7)++movie_entries;if(last_mode==7)++movie_exits;last_mode=current;
    }
    void rendered(bool result,std::int64_t elapsed,GameScreen const& screen){
        if(result)++render_true;else ++render_false;
        if(elapsed>maximum_render_ns)maximum_render_ns=elapsed;
        last_render=screen.diagnostic_snapshot();last_window=screen.window_snapshot();
        const auto reason=unsigned(last_render.reason);if(reason<8)++render_reasons[reason];
        const auto capture=unsigned(last_render.capture.result);
        if(last_render.update_attempted&&capture<10)++capture_results[capture];
    }
    void report(std::int64_t now,bool final=false){
        if(!final&&now<next_report)return;next_report=now+1000000000;
        printf("{\"movie_fallback_diagnostics\":true,\"final\":%s,\"observed_ns\":%lld,\"cumulative\":true,"
            "\"eligible\":%llu,\"render_true\":%llu,\"render_false\":%llu,\"post_context_rejected\":%llu,\"post_fresh_rejected\":%llu,"
            "\"mode_changes\":%llu,\"movie_entries\":%llu,\"movie_exits\":%llu,\"expired_lease_frames\":%llu,"
            "\"submitted\":%llu,\"end_failed\":%llu,\"mode\":%u,\"packet_age_ns\":%lld,\"maximum_render_ns\":%lld,"
            "\"last_render_current\":%s,\"last_render_fresh_checked\":%s,\"last_render_fresh\":%s,\"last_render_fresh_now_100ns\":%lld,"
            "\"capture_generation\":%llu,\"capture_epoch_100ns\":%lld,\"captured_100ns\":%lld,\"width\":%u,\"height\":%u,\"uploads\":%u,\"render_reasons\":{",
            final?"true":"false",now,eligible,render_true,render_false,post_context_rejected,post_fresh_rejected,
            mode_changes,movie_entries,movie_exits,expired_lease_frames,submitted,end_failed,last_mode,last_packet_age_ns,maximum_render_ns,
            last_render.current?"true":"false",last_render.fresh_checked?"true":"false",last_render.fresh?"true":"false",last_render.fresh_now_100ns,
            last_window.generation,last_window.epoch_100ns,last_window.captured_100ns,last_window.width,last_window.height,last_window.uploads);
        for(unsigned i=0;i<8;++i)printf("%s\"%s\":%llu",i?",":"",outage_diagnostics::menu_render_reason_name(outage_diagnostics::MenuRenderReason(i)),render_reasons[i]);
        printf("},\"future_ahead_last_100ns\":%lld,\"future_ahead_max_100ns\":%lld,\"pending_reused\":%llu,\"pending_dropped\":%llu,\"capture_results_on_update\":{",
            last_render.capture.future_ahead_last_100ns,last_render.capture.future_ahead_max_100ns,
            last_render.capture.pending_reused,last_render.capture.pending_dropped);
        for(unsigned i=0;i<10;++i)printf("%s\"%s\":%llu",i?",":"",outage_diagnostics::capture_result_name(outage_diagnostics::CaptureResult(i)),capture_results[i]);
        printf("}}\n");fflush(stdout);
    }
};
bool transition_panel_wait(ProjectionStatus status){
    switch(status){
    case ProjectionStatus::producer_unavailable:case ProjectionStatus::waiting_image:
    case ProjectionStatus::menu_recovery_pending:case ProjectionStatus::stale_ticket:
    case ProjectionStatus::source_reconfiguration:case ProjectionStatus::reference_epoch_pending:return true;
    default:return false;
    }
}
bool startup_panel_wait(ProjectionStatus status){
    return transition_panel_wait(status)||status==ProjectionStatus::menu_requested;
}
bool recovery_panel_wait(ProjectionStatus status,StartupPanelPolicy const& policy){
    return transition_panel_wait(status)||(status==ProjectionStatus::menu_requested&&policy.transition_menu_requested_wait());
}
bool world_handoff_panel_wait(ProjectionStatus status){
    return status==ProjectionStatus::source_reconfiguration||status==ProjectionStatus::waiting_image;
}
bool tracking_recovery_panel_wait(ProjectionStatus status,unsigned queued_layers){
    return queued_layers==0&&status==ProjectionStatus::current_untracked;
}
bool projection_end_succeeded(bool queued,XrResult result,unsigned layer_count,bool projection_layers){
    return queued&&result==XR_SUCCESS&&layer_count==1&&projection_layers;
}
bool cleanup() {
    shutdown_diagnostics::marker("cleanup_enter");
    if(xr_gpu_teardown_unsafe){fprintf(stderr,"Unsafe XR teardown: submitted image GPU work is unconfirmed; XR objects retained until process exit\n");
        printf("{\"unsafe_xr_teardown\":true,\"xr_objects_destroyed\":false}\n");fflush(stdout);shutdown_diagnostics::marker("cleanup_retained_unsafe");return false;}
    bool ok=true;
    if(view_space){shutdown_diagnostics::marker("destroy_view_space_enter");auto result=xrDestroySpace(view_space);
        shutdown_diagnostics::marker("destroy_view_space_returned",true,result);ok=XR_SUCCEEDED(result)&&ok;view_space=XR_NULL_HANDLE;}
    if(local){shutdown_diagnostics::marker("destroy_local_space_enter");auto result=xrDestroySpace(local);
        shutdown_diagnostics::marker("destroy_local_space_returned",true,result);ok=XR_SUCCEEDED(result)&&ok;local=XR_NULL_HANDLE;}
    if(session){shutdown_diagnostics::marker("destroy_session_enter");auto result=xrDestroySession(session);
        shutdown_diagnostics::marker("destroy_session_returned",true,result);ok=XR_SUCCEEDED(result)&&ok;session=XR_NULL_HANDLE;}
    if(instance){shutdown_diagnostics::marker("destroy_instance_enter");auto result=xrDestroyInstance(instance);
        shutdown_diagnostics::marker("destroy_instance_returned",true,result);ok=XR_SUCCEEDED(result)&&ok;instance=XR_NULL_HANDLE;}
    shutdown_diagnostics::marker("cleanup_return",true,ok?1:0);
    return ok;
}
struct ProjectionEvidence {
    std::array<uint64_t,static_cast<unsigned>(ProjectionStatus::count)> counts{};
    std::array<int64_t,static_cast<unsigned>(ProjectionStatus::count)> first{},last{};
    int64_t maximum_gap{};bool structural_failure{};
    unsigned producer_seconds{30};
    void record(ProjectionStatus status,int64_t now){auto i=static_cast<unsigned>(status);
        if(status==ProjectionStatus::submitted && counts[i])maximum_gap=std::max(maximum_gap,now-last[i]);
        if(status!=ProjectionStatus::submitted && status!=ProjectionStatus::producer_stopped && counts[0])maximum_gap=std::max(maximum_gap,now-last[0]);
        if(!counts[i])first[i]=now;last[i]=now;++counts[i];
        if(status==ProjectionStatus::reference_space_changed||status==ProjectionStatus::empty_ticket||status==ProjectionStatus::session_mismatch||
           status==ProjectionStatus::ticket_flags||status==ProjectionStatus::ticket_geometry||status==ProjectionStatus::image_rectangle||status==ProjectionStatus::session_lost||status==ProjectionStatus::producer_structure_invalid||status==ProjectionStatus::probe_failure)structural_failure=true;
    }
    double span_seconds() const{return counts[0]?double(last[0]-first[0])/1e9:0.;}
    bool sustained() const{return (producer_seconds==30||producer_seconds==120) && counts[0]>1 && span_seconds()>=double(producer_seconds-1) && maximum_gap<=100000000 && !structural_failure;}
    void print_rejections() const {
        for(unsigned i=1;i<counts.size();++i)printf("{\"stereo_projection_rejection\":\"%s\",\"count\":%llu,\"first_ns\":%lld,\"last_ns\":%lld,\"expected_stop\":%s}\n",
            projection_status_name(static_cast<ProjectionStatus>(i)),counts[i],first[i],last[i],i==unsigned(ProjectionStatus::producer_stopped)?"true":"false");
        fflush(stdout);
    }
    void print() const {
        printf("{\"stereo_submission_evidence\":true,\"first_submission_ns\":%lld,\"last_submission_ns\":%lld,\"submission_span_seconds\":%.9g,\"maximum_submission_gap_ms\":%.9g,\"required_span_seconds\":%u,\"producer_seconds\":%u,\"maximum_allowed_gap_ms\":100,\"sustained_submission_passed\":%s,\"structural_failure\":%s,\"policy\":\"bounded producer coverage with one second startup allowance; not continuous-mode validation\"}\n",
            first[0],last[0],span_seconds(),double(maximum_gap)/1e6,producer_seconds-1,producer_seconds,sustained()?"true":"false",structural_failure?"true":"false");
        print_rejections();
    }
};
int projection_receiver_self_test(){
    if(!projection_end_succeeded(true,XR_SUCCESS,1,true)||
       projection_end_succeeded(true,XR_SESSION_LOSS_PENDING,1,true)||
       projection_end_succeeded(true,XR_ERROR_SESSION_LOST,1,true)||
       projection_end_succeeded(false,XR_SUCCESS,1,true)||projection_end_succeeded(true,XR_SUCCESS,0,true)||
       projection_end_succeeded(true,XR_SUCCESS,1,false))return 43;
    for(unsigned i=0;i<unsigned(ProjectionStatus::count);++i){const auto s=ProjectionStatus(i);
        const bool expected=s==ProjectionStatus::producer_unavailable||s==ProjectionStatus::waiting_image||
            s==ProjectionStatus::menu_recovery_pending||s==ProjectionStatus::stale_ticket||
            s==ProjectionStatus::source_reconfiguration||s==ProjectionStatus::reference_epoch_pending;
        if(transition_panel_wait(s)!=expected)return 42;
        if(startup_panel_wait(s)!=(expected||s==ProjectionStatus::menu_requested))return 44;
        if(world_handoff_panel_wait(s)!=(s==ProjectionStatus::source_reconfiguration||s==ProjectionStatus::waiting_image))return 47;
        // Tracking may disappear before the first game projection. Eligibility
        // is independent of stereo history, but never replaces a queued layer.
        if(tracking_recovery_panel_wait(s,0)!=(s==ProjectionStatus::current_untracked)||
           tracking_recovery_panel_wait(s,1)||tracking_recovery_panel_wait(s,2))return 48;
        for(auto mode:{gears_menu::Mode::unknown,gears_menu::Mode::gameplay,gears_menu::Mode::loading_display,
                      gears_menu::Mode::main_menu,gears_menu::Mode::acquisition_wait}){
            StartupPanelPolicy witnessed(true,true);witnessed.projection_rendered(true);
            witnessed.observe(gears_menu::Mode::pause_menu,100);witnessed.menu_rendered();witnessed.observe(mode,200);
            const bool narrow=mode==gears_menu::Mode::main_menu||mode==gears_menu::Mode::acquisition_wait;
            if(recovery_panel_wait(s,witnessed)!=(expected||(s==ProjectionStatus::menu_requested&&narrow)))return 45;
            StartupPanelPolicy unwitnessed(true,true);unwitnessed.observe(mode,200);
            if(recovery_panel_wait(s,unwitnessed)!=expected)return 46;
        }
    }
    if(!xr_image_wait_succeeded(XR_SUCCESS)||xr_image_wait_succeeded(XR_TIMEOUT_EXPIRED)||xr_image_wait_succeeded(XR_SESSION_LOSS_PENDING)||
       !xr_image_work_complete(3,2)||xr_image_work_complete(1,2)||xr_image_work_complete(~uint64_t(0),2)||xr_image_work_complete(0,0))return 9;
    auto good=ProjectionStatus::submitted;
    if(projection_current_status(true,true,2,15,15)!=good||projection_current_status(true,true,1,15,15)!=ProjectionStatus::current_untracked||
       projection_current_status(true,true,2,3,15)!=ProjectionStatus::current_untracked||projection_current_status(true,true,2,15,3)!=ProjectionStatus::current_untracked||
       projection_current_status(false,true,2,15,15)!=ProjectionStatus::current_inactive||projection_current_status(true,false,2,15,15)!=ProjectionStatus::current_inactive)return 10;
    gears_vr::StereoImageTicket t{};t.id=1;t.sample.session=7;t.sample.sequence=9;t.sample.received_ns=1000000000;
    t.sample.focused=t.sample.should_render=1;t.sample.head_flags=t.sample.view_flags=15;t.sample.head.orientation.w=1;
    for(unsigned i=0;i<2;++i){t.sample.eyes[i].pose.orientation.w=1;t.sample.eyes[i].pose.position.x=i?.032:-.032;
        t.sample.eyes[i].fov={-.8,.7,.75,-.65};t.rect[i]={int32_t(i*960),0,960,1080};}
    if(projection_ticket_status(t,1920,1080,7,1100000000)!=good||projection_ticket_status(t,1920,1080,7,1100000001)!=ProjectionStatus::stale_ticket||
       projection_ticket_status(t,1920,1080,8,1000000000)!=ProjectionStatus::session_mismatch)return 11;
    auto changed=t;changed.id=0;if(projection_ticket_status(changed,1920,1080,7,1000000000)!=ProjectionStatus::empty_ticket)return 12;
    changed=t;changed.sample.head_flags=3;if(projection_ticket_status(changed,1920,1080,7,1000000000)!=ProjectionStatus::ticket_flags)return 13;
    changed=t;changed.sample.eyes[0].fov.left=NAN;if(projection_ticket_status(changed,1920,1080,7,1000000000)!=ProjectionStatus::ticket_geometry)return 14;
    changed=t;changed.rect[1].x=0;if(projection_ticket_status(changed,1920,1080,7,1000000000)!=ProjectionStatus::image_rectangle)return 15;
    ProjectionEvidence evidence;evidence.record(good,1);if(evidence.sustained())return 16;
    for(int64_t i=1;i<=290;++i)evidence.record(good,1+i*100000000);
    if(!evidence.sustained())return 17;
    evidence.record(ProjectionStatus::producer_stopped,31000000001);if(!evidence.sustained())return 18;
    evidence.record(ProjectionStatus::stale_ticket,31000000001);if(evidence.sustained())return 19;
    ProjectionEvidence structural;for(int64_t i=0;i<=290;++i)structural.record(good,1+i*100000000);
    structural.record(ProjectionStatus::session_mismatch,29000000002);if(structural.sustained()||!structural.structural_failure)return 20;
    ProjectionEvidence extended;extended.producer_seconds=120;
    for(int64_t i=0;i<=1189;++i)extended.record(good,1+i*100000000);
    if(extended.sustained())return 21;
    extended.record(good,119000000001);if(!extended.sustained())return 22;
    extended.record(ProjectionStatus::producer_stopped,121000000001);if(!extended.sustained())return 23;
    extended.record(good,119100000002);if(extended.sustained())return 24;
    extended.producer_seconds=31;if(extended.sustained())return 25;
    changed=t;changed.sample.source=gears_vr::StereoSource::MetaOperatorSimulated;changed.sample.head_flags=3;
    if(projection_ticket_status(changed,1920,1080,7,1000000000)!=ProjectionStatus::ticket_flags ||
       projection_ticket_status(changed,1920,1080,7,1000000000,true)!=good ||
       projection_ticket_status(t,1920,1080,7,1000000000,true)!=ProjectionStatus::ticket_flags)return 26;
    if(projection_current_status(true,true,2,3,15,true)!=good ||
       projection_current_status(true,true,2,1,15,true)!=ProjectionStatus::current_untracked ||
       projection_current_status(true,true,2,3,3,true)!=ProjectionStatus::current_untracked)return 27;
    changed.sample.source=static_cast<gears_vr::StereoSource>(2);
    if(projection_ticket_status(changed,1920,1080,7,1000000000,true)!=ProjectionStatus::ticket_flags)return 28;
    changed=t;changed.sample.predicted_time=100;
    if(projection_ticket_status(changed,1920,1080,7,1000000000,false,2,100)!=ProjectionStatus::reference_epoch_pending)return 29;
    changed.sample.space_epoch=2;
    if(projection_ticket_status(changed,1920,1080,7,1000000000,false,2,101)!=ProjectionStatus::reference_epoch_pending)return 30;
    if(projection_ticket_status(changed,1920,1080,7,1000000000,false,2,100)!=good)return 31;
    ProjectionEvidence transition;transition.record(ProjectionStatus::reference_epoch_pending,1);transition.record(good,2);
    if(transition.structural_failure)return 32;
    // Exercise the live readiness mapping too: gate-only tests cannot detect
    // a receiver still checking the preceding GPU packet ABI version.
    auto pid=GetCurrentProcessId();
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(gears_vr::GpuPacket),gears_vr::gpu_name(pid,L"metadata").c_str());
    if(!mapping||GetLastError()==ERROR_ALREADY_EXISTS)return 33;
    auto* packet=static_cast<gears_vr::GpuPacket*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(gears_vr::GpuPacket)));
    if(!packet){CloseHandle(mapping);return 34;}
    *packet={};packet->magic=0x47564731;packet->version=gears_vr::gpu_packet_version;packet->size=sizeof(*packet);packet->pid=pid;packet->ready=1;
    auto readiness=ProjectionStatus::producer_unavailable;
    bool ready=GameGpuScreen::producer_ready(pid,&readiness);
    packet->version=gears_vr::gpu_packet_version-1;
    bool old_rejected=!GameGpuScreen::producer_ready(pid,&readiness)&&readiness==ProjectionStatus::producer_structure_invalid;
    UnmapViewOfFile(packet);CloseHandle(mapping);
    if(!ready||!old_rejected)return 35;
    if(!projection_session_visible(XR_SESSION_STATE_VISIBLE)||!projection_session_visible(XR_SESSION_STATE_FOCUSED)||
       projection_session_visible(XR_SESSION_STATE_SYNCHRONIZED)||projection_session_visible(XR_SESSION_STATE_STOPPING))return 36;
    changed=t;changed.sample.focused=0;changed.sample.visible=1;
    if(projection_ticket_status(changed,1920,1080,7,1000000000)!=good)return 37;
    changed.sample.visible=0;
    if(projection_ticket_status(changed,1920,1080,7,1000000000)!=ProjectionStatus::ticket_flags)return 38;
    // Continuous cached images preserve their rendered poses; only their age
    // budget differs. Invalid session/origin/rects never gain a grace period.
    auto cached=t;
    if(projection_ticket_status(cached,1920,1080,7,1600000000,false,1,0,projection_retained_image_max_age)!=good||
       projection_ticket_status(cached,1920,1080,7,3000000000,false,1,0,projection_retained_image_max_age)!=good||
       projection_ticket_status(cached,1920,1080,7,3000000001,false,1,0,projection_retained_image_max_age)!=ProjectionStatus::stale_ticket)return 39;
    if(projection_ticket_status(cached,1920,1080,8,1600000000,false,1,0,projection_retained_image_max_age)!=ProjectionStatus::session_mismatch||
       projection_ticket_status(cached,1920,1080,7,1600000000,false,2,0,projection_retained_image_max_age)!=ProjectionStatus::reference_epoch_pending)return 40;
    cached.rect[1].x=0;
    if(projection_ticket_status(cached,1920,1080,7,1600000000,false,1,0,projection_retained_image_max_age)!=ProjectionStatus::image_rectangle)return 41;
    printf("projection receiver CPU gate/age/rect/epoch/coverage/stop/structural checks passed; no XR session opened\n");return 0;
}
int runtime_info(){
    try {
        XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};
        strcpy_s(create.applicationInfo.applicationName,"GearsVR Runtime Identity");
        create.applicationInfo.apiVersion=XR_API_VERSION_1_0;
        xr_check(xrCreateInstance(&create,&instance),"runtime identity instance");
        XrInstanceProperties properties{XR_TYPE_INSTANCE_PROPERTIES};
        xr_check(xrGetInstanceProperties(instance,&properties),"runtime identity properties");
        printf("runtime_name=%s\nruntime_version=%llu\n",properties.runtimeName,properties.runtimeVersion);
        xr_check(xrDestroyInstance(instance),"runtime identity cleanup");instance=XR_NULL_HANDLE;
        return 0;
    } catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());cleanup();return 1;}
}
gears_vr::RenderSizeLimits query_render_size_limits(XrSystemId system) {
    gears_vr::RenderSizeLimits limits{};XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    xr_check(xrGetSystemProperties(instance,system,&properties),"render size system limits");
    limits.system_width=properties.graphicsProperties.maxSwapchainImageWidth;
    limits.system_height=properties.graphicsProperties.maxSwapchainImageHeight;
    uint32_t count{};
    xr_check(xrEnumerateViewConfigurationViews(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr),"render size view count");
    if(count!=2)throw std::runtime_error("Render size requires exactly two runtime views");
    std::array<XrViewConfigurationView,2> views{{{XR_TYPE_VIEW_CONFIGURATION_VIEW},{XR_TYPE_VIEW_CONFIGURATION_VIEW}}};
    xr_check(xrEnumerateViewConfigurationViews(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,views.data()),"render size views");
    if(count!=2)throw std::runtime_error("Render size runtime view count changed");
    for(unsigned eye=0;eye<2;++eye){limits.recommended_width[eye]=views[eye].recommendedImageRectWidth;
        limits.recommended_height[eye]=views[eye].recommendedImageRectHeight;
        limits.maximum_width[eye]=views[eye].maxImageRectWidth;limits.maximum_height[eye]=views[eye].maxImageRectHeight;}
    return limits;
}
void print_render_size_plan(gears_vr::RenderSizePlan const& p) {
    printf("{\"runtime_render_size_plan\":true,\"recommended\":[%u,%u],\"render_scale\":%.9g,\"eye\":[%u,%u],\"source\":[%u,%u],\"engine_sbs_limit\":8192,\"dimensions_are_plan_not_gpu_proof\":true}\n",
        p.recommended_width,p.recommended_height,p.scale,p.eye_width,p.eye_height,p.source_width,p.source_height);fflush(stdout);
}
bool parse_render_scale(const char* text,double& out) {
    if(!text||!*text)return false;
    char* end{};errno=0;const double value=strtod(text,&end);
    if(end==text||*end||errno==ERANGE||!std::isfinite(value)||value<.25||value>2.)return false;
    out=value;return true;
}
int render_size_info(double scale) {
    try{XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};
        strcpy_s(create.applicationInfo.applicationName,"GearsVR Render Size");create.applicationInfo.apiVersion=XR_API_VERSION_1_0;
        xr_check(xrCreateInstance(&create,&instance),"render size instance");
        XrSystemGetInfo info{XR_TYPE_SYSTEM_GET_INFO};info.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;XrSystemId system{};
        xr_check(xrGetSystem(instance,&info,&system),"render size system");
        gears_vr::RenderSizePlan plan{};auto error=gears_vr::make_render_size_plan(query_render_size_limits(system),scale,plan);
        if(error!=gears_vr::RenderSizeError::none)throw std::runtime_error(gears_vr::render_size_error(error));
        print_render_size_plan(plan);xr_check(xrDestroyInstance(instance),"render size cleanup");instance=XR_NULL_HANDLE;return 0;
    }catch(std::exception const& error){fprintf(stderr,"%s\n",error.what());cleanup();return 1;}
}
int main(int argc, char** argv) {
    xr_wait_diagnostics::SessionDump wait_diagnostics_dump;
    if(argc>=2&&!strcmp(argv[1],"--render-size-info")){
        double scale=1.;
        if(argc!=2&&(argc!=4||strcmp(argv[2],"--render-scale")||!parse_render_scale(argv[3],scale)))return 2;
        return render_size_info(scale);
    }
    if(argc==3&&!strcmp(argv[1],"--stop-session")){
        char* end{};errno=0;const auto pid=strtoull(argv[2],&end,10);
        if(*argv[2]<'0'||*argv[2]>'9'||*end||errno==ERANGE||!pid||pid>std::numeric_limits<DWORD>::max())return 2;
        return ContinuousSessionControl::request(static_cast<DWORD>(pid))?0:1;
    }
    if(argc==2&&!strcmp(argv[1],"--continuous-control-self-test")){
        const auto pid=GetCurrentProcessId();
        const auto created=ContinuousSessionControl::creation_time(GetCurrentProcess());
        for(const auto stamp:{std::uint64_t(0),created+1}){
            bool rejected=false;try{ContinuousSessionControl wrong;wrong.open(pid,pid,stamp);}catch(const std::exception&){rejected=true;}
            if(!rejected||ContinuousSessionControl::request(pid))return 6;
        }
        {ContinuousSessionControl owner;owner.open(pid,pid,created);if(owner.requested())return 1;
            bool rejected=false;try{ContinuousSessionControl duplicate;duplicate.open(pid,pid,created);}catch(const std::exception&){rejected=true;}
            if(!rejected||owner.requested()||!ContinuousSessionControl::request(pid)||strcmp(owner.requested(),"stop_event"))return 2;}
        if(ContinuousSessionControl::request(pid))return 3;
        {ContinuousSessionControl reopened;reopened.open(pid,pid,created);if(reopened.requested())return 4;}
        if(!continuous_orderly_reason("producer_stopped")||continuous_orderly_reason("control_error")||continuous_orderly_reason("owner_exited"))return 5;
        if(ContinuousSessionControl::owner_wait_reason(WAIT_TIMEOUT)||strcmp(ContinuousSessionControl::owner_wait_reason(WAIT_OBJECT_0),"owner_exited")||
           strcmp(ContinuousSessionControl::owner_wait_reason(WAIT_FAILED),"owner_error"))return 7;
        printf("continuous control owner identity/alive/death/error/duplicate/stop/reopen checks passed; no XR session opened\n");return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--projection-receiver-self-test"))return projection_receiver_self_test();
    if(argc==2&&!strcmp(argv[1],"--runtime-info"))return runtime_info();
    if(argc<3) {fprintf(stderr,"Usage: OpenXRPoseProbe SECONDS OUTPUT.csv [--render] [--bridge-pid PID]\n");return 2;}
    bool test_render=false;DWORD bridge_pid=0,stereo_pid=0,stereo_game_pid=0,game_pid=0,gpu_pid=0;HWND game_window=nullptr;const char* gpu_snapshot=nullptr;
    HWND startup_window=nullptr;DWORD startup_game_pid=0;
    const char* menu_state_file=nullptr;std::uint64_t menu_game_creation=0;
    unsigned stereo_producer_seconds=30;bool explicit_stereo_duration=false;
    double render_scale{};bool explicit_render_scale=false;
    bool simulator_operator=false,continuous_session=false,diagnostic_shutdown=false,diagnostic_outage=false,diagnostic_image_tickets=false,movie_display=false;
    DWORD continuous_owner_pid=0;std::uint64_t continuous_owner_creation=0;
    for(int i=3;i<argc;++i) {
        if(!strcmp(argv[i],"--render"))test_render=true;
        else if(!strcmp(argv[i],"--render-scale")&&i+1<argc){
            if(explicit_render_scale||!parse_render_scale(argv[++i],render_scale))return 2;explicit_render_scale=true;
        }
        else if(!strcmp(argv[i],"--menu-state-file")&&i+1<argc){if(menu_state_file)return 2;menu_state_file=argv[++i];if(!*menu_state_file)return 2;}
        else if(!strcmp(argv[i],"--movie-display")){if(movie_display)return 2;movie_display=true;}
        else if(!strcmp(argv[i],"--menu-game-creation")&&i+1<argc){
            if(menu_game_creation)return 2;const char* value=argv[++i];char* end{};errno=0;
            menu_game_creation=strtoull(value,&end,10);
            if(*value<'0'||*value>'9'||*end||errno==ERANGE||!menu_game_creation)return 2;
        }
        else if(!strcmp(argv[i],"--simulator-operator")){if(simulator_operator)return 2;simulator_operator=true;}
        else if(!strcmp(argv[i],"--continuous-session")){if(continuous_session)return 2;continuous_session=true;}
        else if(!strcmp(argv[i],"--shutdown-diagnostics")){if(diagnostic_shutdown)return 2;diagnostic_shutdown=true;}
        else if(!strcmp(argv[i],"--outage-diagnostics")){if(diagnostic_outage)return 2;diagnostic_outage=true;}
        else if(!strcmp(argv[i],"--image-ticket-diagnostics")){if(diagnostic_image_tickets)return 2;diagnostic_image_tickets=true;}
        else if((!strcmp(argv[i],"--owner-pid")||!strcmp(argv[i],"--owner-creation-time"))&&i+1<argc){
            const bool is_pid=!strcmp(argv[i],"--owner-pid");
            if(is_pid?continuous_owner_pid!=0:continuous_owner_creation!=0)return 2;
            const char* value=argv[++i];char* end{};errno=0;const auto parsed=strtoull(value,&end,10);
            if(*value<'0'||*value>'9'||*end||errno==ERANGE||!parsed||(is_pid&&parsed>std::numeric_limits<DWORD>::max()))return 2;
            if(is_pid)continuous_owner_pid=static_cast<DWORD>(parsed);else continuous_owner_creation=parsed;
        }
        else if(!strcmp(argv[i],"--bridge-pid") && i+1<argc)bridge_pid=strtoul(argv[++i],nullptr,10);
        else if((!strcmp(argv[i],"--stereo-bridge-pid")||!strcmp(argv[i],"--stereo-game-pid")) && i+1<argc){
            bool game=!strcmp(argv[i],"--stereo-game-pid");
            const char* value=argv[++i];char* end{};errno=0;
            auto parsed=strtoull(value,&end,10);
            if(*value<'0'||*value>'9'||*end||errno==ERANGE||!parsed||parsed>std::numeric_limits<DWORD>::max())return 2;
            stereo_pid=static_cast<DWORD>(parsed);
            if(game){stereo_game_pid=stereo_pid;test_render=true;}
        }
        else if(!strcmp(argv[i],"--stereo-producer-seconds") && i+1<argc){
            if(explicit_stereo_duration)return 2;
            const char* value=argv[++i];
            if(strcmp(value,"30")&&strcmp(value,"120"))return 2;
            stereo_producer_seconds=unsigned(atoi(value));explicit_stereo_duration=true;
        }
        else if(!strcmp(argv[i],"--gpu-game-pid") && i+1<argc)gpu_pid=strtoul(argv[++i],nullptr,10);
        else if(!strcmp(argv[i],"--gpu-snapshot") && i+1<argc)gpu_snapshot=argv[++i];
        else if(!strcmp(argv[i],"--game-window") && i+2<argc){game_window=reinterpret_cast<HWND>(_strtoui64(argv[++i],nullptr,0));game_pid=strtoul(argv[++i],nullptr,10);}
        else if(!strcmp(argv[i],"--startup-menu-window") && i+2<argc){
            if(startup_window)return 2;
            char* end{};errno=0;const char* value=argv[++i];
            const auto hwnd_value=_strtoui64(value,&end,0);
            if(*value<'0'||*value>'9'||*end||errno==ERANGE||!hwnd_value||hwnd_value>UINTPTR_MAX)return 2;
            startup_window=reinterpret_cast<HWND>(static_cast<uintptr_t>(hwnd_value));
            value=argv[++i];errno=0;const auto pid_value=strtoull(value,&end,10);
            if(*value<'0'||*value>'9'||*end||errno==ERANGE||!pid_value||pid_value>MAXDWORD)return 2;
            startup_game_pid=static_cast<DWORD>(pid_value);
        }
        else return 2;
    }
    int seconds=atoi(argv[1]);
    if(continuous_session){if(strcmp(argv[1],"0")||!stereo_game_pid||explicit_stereo_duration||simulator_operator||!continuous_owner_pid||!continuous_owner_creation)return 2;}
    else if(seconds<3 || seconds>((bridge_pid||(stereo_game_pid&&stereo_producer_seconds==120))?180:60) || (bridge_pid && !test_render)) return 2;
    if(!continuous_session&&(continuous_owner_pid||continuous_owner_creation))return 2;
    if(explicit_stereo_duration&&(!stereo_game_pid||seconds<int(stereo_producer_seconds+10)))return 2;
    if(simulator_operator&&!stereo_game_pid)return 2;
    if(explicit_render_scale&&!stereo_game_pid)return 2;
    if(diagnostic_outage&&!stereo_game_pid)return 2;
    if(diagnostic_image_tickets&&!stereo_game_pid)return 2;
    if(game_window && (test_render || bridge_pid || !game_pid))return 2;
    if(gpu_pid && (test_render || bridge_pid || game_window))return 2;
    if(gpu_snapshot && !gpu_pid&&!stereo_game_pid)return 2;
    // Initial stereo integration publishes real paired views while displaying
    // the existing XR test pattern. Game projection-layer output is separate.
    if(stereo_pid && (!test_render || bridge_pid || gpu_pid || game_window))return 2;
    if(stereo_game_pid&&stereo_game_pid!=stereo_pid)return 2;
    if(startup_window&&(!continuous_session||!stereo_game_pid||startup_game_pid!=stereo_game_pid||game_window||gpu_pid||bridge_pid))return 2;
    if((menu_state_file!=nullptr)!=(menu_game_creation!=0)||(menu_state_file&&!startup_window))return 2;
    if(movie_display&&(!continuous_session||!menu_state_file))return 2;
    if(diagnostic_shutdown)shutdown_diagnostics::enable();
    ContinuousSessionControl continuous_control;
    if(continuous_session){try{continuous_control.open(stereo_game_pid,continuous_owner_pid,continuous_owner_creation);}catch(const std::exception& error){fprintf(stderr,"%s\n",error.what());return 2;}}
    if(game_window||startup_window){SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);winrt::init_apartment(winrt::apartment_type::multi_threaded);}
    gears_vr::PoseBridge bridge;
    if(bridge_pid && !bridge.open(bridge_pid,true)){fprintf(stderr,"Pose bridge creation rejected\n");return 2;}
    gears_vr::RenderSizeBridge render_size_bridge;
    gears_vr::StereoBridge stereo_bridge;
    if(stereo_pid && !stereo_bridge.open(stereo_pid,true)){fprintf(stderr,"Stereo bridge creation rejected\n");return 2;}
    const uint64_t stereo_session=static_cast<uint64_t>(gears_vr::monotonic_ns());
    uint64_t stereo_published=0,stereo_publish_skipped=0;
    bool stereo_space_changed=false;
    gears_xr::ReferenceSpaceEpoch space_epoch;
    std::uint64_t last_submitted_space_epoch{};
    std::uint64_t last_submitted_resource_generation{};
    // Permit the bounded runner to read flushed readiness rows while sampling.
    FILE* csv=_fsopen(argv[2],"w",_SH_DENYWR);
    if(!csv) return 2;
    fprintf(csv,"frame,predicted_time,session_state,should_render,view_flags,head_flags,head_x,head_y,head_z,qx,qy,qz,qw,left_x,left_y,left_z,right_x,right_y,right_z,left_angle_left,left_angle_right,left_angle_up,left_angle_down,left_qx,left_qy,left_qz,left_qw,right_qx,right_qy,right_qz,right_qw,right_angle_left,right_angle_right,right_angle_up,right_angle_down,stereo_session,stereo_sequence,published_ns,stereo_published,stereo_source\n");
    bool running=false;
    gears_vr::SessionExitState session_exit;
    bool simulated_runtime=false;
    std::string stop_reason;
    ProjectionEvidence projection_evidence;
    outage_diagnostics::Recorder outage;
    outage.enabled=diagnostic_outage;
    image_ticket_diagnostics::Recorder image_tickets;
    projection_evidence.producer_seconds=stereo_producer_seconds;
    try {
        if(diagnostic_image_tickets&&!image_tickets.enable())throw std::runtime_error("Image ticket diagnostics allocation failed");
        const char* extensions[]={XR_KHR_D3D12_ENABLE_EXTENSION_NAME};
        XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};
        strcpy_s(create.applicationInfo.applicationName,"GearsVR Pose Probe");
        strcpy_s(create.applicationInfo.engineName,"GearsVR Research");
        create.applicationInfo.apiVersion=XR_API_VERSION_1_0;
        create.enabledExtensionCount=1;create.enabledExtensionNames=extensions;
        shutdown_diagnostics::initialization("xr_create_instance_enter");
        xr_check(xrCreateInstance(&create,&instance),"xrCreateInstance");
        shutdown_diagnostics::initialization("xr_create_instance_completed");
        XrInstanceProperties properties{XR_TYPE_INSTANCE_PROPERTIES};
        xr_check(xrGetInstanceProperties(instance,&properties),"runtime properties");
        simulated_runtime=!strcmp(properties.runtimeName,"OpenXR Simulator Runtime")||!strcmp(properties.runtimeName,"Meta XR Simulator");
        fprintf(stderr,"runtime_name=%s runtime_version=%llu simulator_operator=%u\n",properties.runtimeName,properties.runtimeVersion,unsigned(simulator_operator));
        if(simulator_operator && (strcmp(properties.runtimeName,"Meta XR Simulator") ||
            !GetModuleHandleW(L"SIMULATOR.dll") || !GetModuleHandleW(L"XrApiLayer_METAX_operator.dll")))
            throw std::runtime_error("Synthetic input requires verified Meta simulator runtime and loaded Operator layer");
        XrSystemGetInfo system_info{XR_TYPE_SYSTEM_GET_INFO};system_info.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        XrSystemId system{};xr_check(xrGetSystem(instance,&system_info,&system),"xrGetSystem");
        gears_eye_chain::Limits stereo_limits{};
        if(stereo_game_pid) {
            XrSystemProperties system_properties{XR_TYPE_SYSTEM_PROPERTIES};
            xr_check(xrGetSystemProperties(instance,system,&system_properties),"stereo system size limits");
            stereo_limits.system_width=system_properties.graphicsProperties.maxSwapchainImageWidth;
            stereo_limits.system_height=system_properties.graphicsProperties.maxSwapchainImageHeight;
            uint32_t count{};
            xr_check(xrEnumerateViewConfigurationViews(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr),"stereo view size count");
            if(count!=2)throw std::runtime_error("Stereo eye chains require exactly two runtime views");
            std::array<XrViewConfigurationView,2> configurations{{{XR_TYPE_VIEW_CONFIGURATION_VIEW},{XR_TYPE_VIEW_CONFIGURATION_VIEW}}};
            xr_check(xrEnumerateViewConfigurationViews(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,configurations.data()),"stereo view size limits");
            if(count!=2)throw std::runtime_error("Stereo runtime view count changed");
            for(unsigned eye=0;eye<2;++eye) {
                stereo_limits.view_width[eye]=configurations[eye].maxImageRectWidth;
                stereo_limits.view_height[eye]=configurations[eye].maxImageRectHeight;
                stereo_limits.view_samples[eye]=configurations[eye].maxSwapchainSampleCount;
                printf("{\"xr_eye_limits\":%u,\"recommended\":[%u,%u],\"maximum\":[%u,%u],\"system_maximum\":[%u,%u]}\n",eye,
                       configurations[eye].recommendedImageRectWidth,configurations[eye].recommendedImageRectHeight,
                       stereo_limits.view_width[eye],stereo_limits.view_height[eye],stereo_limits.system_width,stereo_limits.system_height);
            }
            if(explicit_render_scale){
                auto error=gears_vr::make_render_size_plan(query_render_size_limits(system),render_scale,stereo_limits.render_size);
                if(error!=gears_vr::RenderSizeError::none)throw std::runtime_error(gears_vr::render_size_error(error));
                if(!render_size_bridge.publish(stereo_game_pid,stereo_session,stereo_limits.render_size))
                    throw std::runtime_error("Render size control publication rejected");
                stereo_limits.independent_render_size=true;print_render_size_plan(stereo_limits.render_size);
            }
            fflush(stdout);
        }
        PFN_xrGetD3D12GraphicsRequirementsKHR requirements_fn{};
        xr_check(xrGetInstanceProcAddr(instance,"xrGetD3D12GraphicsRequirementsKHR",reinterpret_cast<PFN_xrVoidFunction*>(&requirements_fn)),"requirements function");
        XrGraphicsRequirementsD3D12KHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
        xr_check(requirements_fn(instance,system,&requirements),"D3D12 requirements");
        ComPtr<IDXGIFactory4> factory;dx_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");
        ComPtr<IDXGIAdapter1> adapter;dx_check(factory->EnumAdapterByLuid(requirements.adapterLuid,IID_PPV_ARGS(&adapter)),"runtime adapter");
        ComPtr<ID3D12Device> device;
        shutdown_diagnostics::initialization("d3d12_create_device_enter");
        dx_check(D3D12CreateDevice(adapter.Get(),requirements.minFeatureLevel,IID_PPV_ARGS(&device)),"D3D12 device");
        shutdown_diagnostics::initialization("d3d12_create_device_completed");
        D3D12_COMMAND_QUEUE_DESC desc{};ComPtr<ID3D12CommandQueue> queue;
        dx_check(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)),"D3D12 queue");
        XrGraphicsBindingD3D12KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};binding.device=device.Get();binding.queue=queue.Get();
        XrSessionCreateInfo session_create{XR_TYPE_SESSION_CREATE_INFO};session_create.next=&binding;session_create.systemId=system;
        shutdown_diagnostics::initialization("xr_create_session_enter");
        xr_check(xrCreateSession(instance,&session_create,&session),"xrCreateSession");
        shutdown_diagnostics::initialization("xr_create_session_completed");
        uint32_t format_count{};xr_check(xrEnumerateSwapchainFormats(session,0,&format_count,nullptr),"diagnostic format count");
        std::vector<int64_t> session_formats(format_count);xr_check(xrEnumerateSwapchainFormats(session,format_count,&format_count,session_formats.data()),"diagnostic formats");
        printf("{\"swapchain_formats\":[");for(uint32_t i=0;i<format_count;++i)printf("%s%lld",i?",":"",session_formats[i]);printf("]}\n");fflush(stdout);
        XrReferenceSpaceCreateInfo space{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};space.poseInReferenceSpace.orientation.w=1;
        space.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;xr_check(xrCreateReferenceSpace(session,&space,&local),"LOCAL space");
        space.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;xr_check(xrCreateReferenceSpace(session,&space,&view_space),"VIEW space");
        std::unique_ptr<TestPattern,shutdown_diagnostics::DeleteMarker<TestPattern>> pattern{nullptr,{"pattern_reset_enter","pattern_reset_completed"}};
        if(test_render&&!continuous_session)pattern.reset(new TestPattern(device.Get(),queue.Get(),argv[2]));
        std::unique_ptr<GameScreen,shutdown_diagnostics::DeleteMarker<GameScreen>> screen{nullptr,{"screen_reset_enter","screen_reset_completed"}};
        if(game_window)screen.reset(new GameScreen(device.Get(),queue.Get(),game_window,game_pid,argv[2]));
        std::unique_ptr<GameScreen,shutdown_diagnostics::DeleteMarker<GameScreen>> startup_screen{nullptr,{"startup_screen_reset_enter","startup_screen_reset_completed"}};
        if(startup_window){
            shutdown_diagnostics::initialization("startup_game_screen_enter");
            startup_screen.reset(new GameScreen(device.Get(),queue.Get(),startup_window,startup_game_pid,argv[2],true,menu_game_creation));
            if(diagnostic_outage)startup_screen->enable_performance_diagnostics();
            shutdown_diagnostics::initialization("startup_game_screen_completed");
        }
        StartupPanelPolicy startup_panel(startup_window!=nullptr,menu_state_file!=nullptr);
        WindowFallbackPolicy window_fallback;
        MovieFallbackDiagnostics movie_fallback_diagnostics;
        bool window_fallback_shown=false;
        const bool window_fallback_enabled=movie_display&&continuous_session&&startup_screen&&menu_game_creation;
        std::unique_ptr<TransitionCard,shutdown_diagnostics::DeleteMarker<TransitionCard>> transition_card{nullptr,{"transition_card_reset_enter","transition_card_reset_completed"}};
        if(menu_state_file&&startup_window)transition_card.reset(new TransitionCard(device.Get(),queue.Get()));
        unsigned transition_panel_layers{};bool transition_panel_active=false,transition_preview_saved=false;
        unsigned world_handoff_panel_layers{};bool world_handoff_panel_active=false;
        WorldHandoffDisplay::State last_world_handoff_panel{};
        std::unique_ptr<TransitionCard,shutdown_diagnostics::DeleteMarker<TransitionCard>> startup_wait_card{
            nullptr,{"startup_wait_card_reset_enter","startup_wait_card_reset_completed"}};
        if(startup_window)startup_wait_card.reset(new TransitionCard(device.Get(),queue.Get(),transition_card_pixels::Variant::starting_vr));
        unsigned startup_wait_card_layers{};
        std::unique_ptr<TransitionCard,shutdown_diagnostics::DeleteMarker<TransitionCard>> tracking_recovery_card{
            nullptr,{"tracking_recovery_card_reset_enter","tracking_recovery_card_reset_completed"}};
        if(stereo_game_pid)tracking_recovery_card.reset(new TransitionCard(device.Get(),queue.Get(),transition_card_pixels::Variant::tracking_lost));
        unsigned tracking_recovery_card_layers{};
        MenuDisplayReader menu_display(menu_state_file,stereo_game_pid,menu_game_creation,continuous_owner_pid,continuous_owner_creation,movie_display);
        unsigned menu_quad_layers=0,menu_entries=0,menu_recoveries=0;
        unsigned main_menu_quad_layers=0,last_main_menu_entry=0,main_menu_snapshots=0;
        unsigned pause_menu_projection_layers=0,last_pause_menu_entry=0;
        unsigned startup_quad_layers=0;
        std::unique_ptr<GameGpuScreen,shutdown_diagnostics::DeleteMarker<GameGpuScreen>> gpu_screen{nullptr,{"gpu_screen_reset_enter","gpu_screen_reset_completed"}};
        if(gpu_pid)gpu_screen.reset(new GameGpuScreen(device.Get(),queue.Get(),gpu_pid,gpu_snapshot));
        std::unique_ptr<GameGpuScreen,shutdown_diagnostics::DeleteMarker<GameGpuScreen>> stereo_screen{nullptr,{"stereo_screen_reset_enter","stereo_screen_reset_completed"}};
        bool stereo_image_shown=false;
        auto stereo_retry=std::chrono::steady_clock::now();
        unsigned retired_stereo_copies{},retired_stereo_projections{},source_rebuilds{};
        unsigned retired_retained_projections{},retired_late_images{};
        unsigned frames=0,valid=0,tracked=0,render=0,focused=0;XrSessionState state=XR_SESSION_STATE_UNKNOWN;
        auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
        auto next_csv=std::chrono::steady_clock::now();
        bool exiting=false;
        shutdown_diagnostics::marker("frame_loop");
        while(!exiting && (continuous_session||std::chrono::steady_clock::now()<deadline)) {
            if(continuous_session){if(const auto* reason=continuous_control.requested()){startup_panel.cancel();stop_reason=reason;break;}}
            XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
            XrResult poll;
            while((poll=xrPollEvent(instance,&event))==XR_SUCCESS) {
                if(event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                    auto change=reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                    if(change->session!=session)continue;
                    state=change->state;session_exit.observe(state);
                    fprintf(stderr,"session_state=%d\n",int(state));
                    if(state==XR_SESSION_STATE_READY) {
                        XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO};begin.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                        xr_check(xrBeginSession(session,&begin),"xrBeginSession");running=true;
                    } else if(state==XR_SESSION_STATE_STOPPING) {xr_check(xrEndSession(session),"xrEndSession");session_exit.mark_end_success();running=false;
                        if(continuous_session){stop_reason="runtime_stopping";exiting=true;}}
                    else if(state==XR_SESSION_STATE_EXITING || state==XR_SESSION_STATE_LOSS_PENDING){exiting=true;
                        stop_reason=state==XR_SESSION_STATE_LOSS_PENDING?"session_lost":"runtime_exiting";
                        if(stereo_game_pid&&state==XR_SESSION_STATE_LOSS_PENDING)projection_evidence.record(ProjectionStatus::session_lost,gears_vr::monotonic_ns());}
                } else if(event.type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING){exiting=true;
                    session_exit.loss_observed=true;
                    stop_reason="instance_lost";
                    if(stereo_game_pid)projection_evidence.record(ProjectionStatus::session_lost,gears_vr::monotonic_ns());}
                else if(event.type==XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING && stereo_pid){
                    const auto* change=reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(&event);
                    if(change->session==session&&change->referenceSpaceType==XR_REFERENCE_SPACE_TYPE_LOCAL){
                        // Apply at changeTime and wait for an image rendered
                        // using that explicit reference-space epoch.
                        stereo_space_changed=true;
                        if(continuous_session){
                            if(!space_epoch.enqueue(change->changeTime))throw std::runtime_error("Reference-space event queue rejected");
                            fprintf(stderr,"reference_space_pending change_time=%lld pose_valid=%u\n",change->changeTime,change->poseValid);
                        }else {stereo_bridge.publish({});fprintf(stderr,"stereo_reference_space_changed=1 stream_invalidated=1\n");}
                    }
                }
                event={XR_TYPE_EVENT_DATA_BUFFER};
            }
            if(poll!=XR_EVENT_UNAVAILABLE) xr_check(poll,"xrPollEvent");
            if(exiting)break;
            if(!running) {std::this_thread::sleep_for(std::chrono::milliseconds(10));continue;}
            XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};XrFrameState frame{XR_TYPE_FRAME_STATE};
            XrResult frame_wait_result=XR_SUCCESS;
            const auto waited_frame=xr_wait_diagnostics::recorder.measure(xr_wait_diagnostics::Operation::wait_frame,
                [&]{return frame_wait_result=xrWaitFrame(session,&wait,&frame);},
                [&]{return frame_wait_result==XR_SUCCESS?xr_wait_diagnostics::context(int(state),frame.shouldRender!=0):xr_wait_diagnostics::Context::unknown;},
                [](XrResult r){return r==XR_TIMEOUT_EXPIRED?xr_wait_diagnostics::Outcome::timeout:XR_FAILED(r)?xr_wait_diagnostics::Outcome::error:xr_wait_diagnostics::Outcome::success;});
            xr_check(waited_frame,"xrWaitFrame");
            xr_wait_diagnostics::current_context=xr_wait_diagnostics::context(int(state),frame.shouldRender!=0);
            if(continuous_session&&stereo_pid){
                if(space_epoch.advance(frame.predictedDisplayTime)){
                    printf("{\"reference_space_rebased\":true,\"space_epoch\":%llu,\"change_time\":%lld}\n",space_epoch.epoch(),space_epoch.last_change_time());fflush(stdout);
                }
                if(space_epoch.advance_rejected())throw std::runtime_error("Reference-space frame time rejected");
            }
            XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};xr_check(xrBeginFrame(session,&begin),"xrBeginFrame");
            XrViewLocateInfo locate{XR_TYPE_VIEW_LOCATE_INFO};locate.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            locate.displayTime=frame.predictedDisplayTime;locate.space=local;
            XrViewState view_state{XR_TYPE_VIEW_STATE};XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};uint32_t view_count=0;
            xr_check(xrLocateViews(session,&locate,&view_state,2,&view_count,views),"xrLocateViews");
            XrSpaceLocation head{XR_TYPE_SPACE_LOCATION};xr_check(xrLocateSpace(view_space,local,frame.predictedDisplayTime,&head),"xrLocateSpace");
            if(bridge_pid) {
                const auto& p=head.pose;
                gears_vr::PoseSample sample{{{p.position.x,p.position.y,p.position.z},
                    {p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w}},
                    frames+1,gears_vr::monotonic_ns(),head.locationFlags,state==XR_SESSION_STATE_FOCUSED};
                bridge.publish(sample);
            }
            gears_vr::StereoSample stereo_sample{};bool stereo_sent=false;
            if(stereo_pid && (!stereo_space_changed||continuous_session)){
                auto pose=[](const XrPosef& p){return gears_vr::Pose{{p.position.x,p.position.y,p.position.z},
                    {p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w}};};
                stereo_sample.session=stereo_session;stereo_sample.sequence=frames+1;
                stereo_sample.space_epoch=space_epoch.epoch();
                stereo_sample.source=simulator_operator?gears_vr::StereoSource::MetaOperatorSimulated:gears_vr::StereoSource::RuntimeTracked;
                stereo_sample.received_ns=gears_vr::monotonic_ns();stereo_sample.predicted_time=frame.predictedDisplayTime;
                stereo_sample.head_flags=head.locationFlags;stereo_sample.view_flags=view_count==2?view_state.viewStateFlags:0;
                stereo_sample.focused=state==XR_SESSION_STATE_FOCUSED;
                stereo_sample.visible=projection_session_visible(state);stereo_sample.should_render=frame.shouldRender;
                stereo_sample.head=pose(head.pose);
                for(unsigned i=0;i<2;++i){const auto& f=views[i].fov;
                    stereo_sample.eyes[i]={pose(views[i].pose),{f.angleLeft,f.angleRight,f.angleUp,f.angleDown}};}
                stereo_sent=stereo_bridge.publish(stereo_sample);
                if(stereo_sent)++stereo_published;else ++stereo_publish_skipped;
            }
            XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};end.displayTime=frame.predictedDisplayTime;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            XrCompositionLayerProjectionView projection_views[2];
            XrCompositionLayerProjection projection{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
            const XrCompositionLayerBaseHeader* layers[]={reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projection)};
            XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};
            const XrCompositionLayerBaseHeader* game_layers[]={reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quad)};
            ProjectionStatus projection_frame_status=ProjectionStatus::producer_unavailable;
            bool outage_render_attempted=false,startup_quad_queued=false,startup_handoff_queued=false,pause_menu_queued=false,transition_panel_queued=false,startup_wait_card_queued=false,world_handoff_panel_queued=false,window_fallback_queued=false,tracking_recovery_card_queued=false;
            WorldHandoffDisplay::State world_handoff_panel_state{};
            const auto menu_now=gears_vr::monotonic_ns();
            const auto menu_mode=menu_display.poll(menu_now);
            const auto observe_menu=[&](gears_menu::Mode mode,std::int64_t observed){
                if(!startup_panel.observe(mode,observed))return;
                if((mode==gears_menu::Mode::death_menu||mode==gears_menu::Mode::main_menu)&&startup_screen)
                    startup_screen->begin_menu_epoch();
                ++menu_entries;
                printf("{\"menu_display_enter\":true,\"epoch\":%u,\"mode\":\"%s\",\"observed_ns\":%lld}\n",
                    menu_entries,mode==gears_menu::Mode::pause_menu?"pause_menu":
                        (mode==gears_menu::Mode::main_menu?"main_menu":"death_menu"),observed);fflush(stdout);
            };
            observe_menu(menu_mode,menu_now);
            // Capture suspension happens after successful native submission;
            // a waiting frame can resume the same session-owned capture below.
            unsigned outage_copies_before=(diagnostic_outage||diagnostic_image_tickets)&&stereo_screen?stereo_screen->copies:0;
            if(screen && frame.shouldRender && screen->render(quad)){end.layerCount=1;end.layers=game_layers;}
            const bool menu_panel_requested=startup_panel.show_panel();
            const bool menu_render_attempted=menu_panel_requested&&startup_screen&&frame.shouldRender;
            const bool menu_render_result=menu_render_attempted&&startup_screen->render(quad);
            bool menu_authorization_rejected=false;
            if(menu_render_result){
                // WGC upload may wait for a GPU fence. Authorization must still
                // be fresh at submission; never extend an owner's display lease.
                const auto after_capture=gears_vr::monotonic_ns();
                const auto authorized=menu_display.poll(after_capture);
                if(!menu_state_file||menu_mode==gears_menu::Mode::unknown||authorized==menu_mode){
                    startup_quad_queued=true;end.layerCount=1;end.layers=game_layers;
                }else {menu_authorization_rejected=true;observe_menu(authorized,after_capture);}
            }
            if(gpu_screen && frame.shouldRender && gpu_screen->render(quad)){end.layerCount=1;end.layers=game_layers;}
            if(pattern && !stereo_image_shown && frame.shouldRender && view_count==2 && (view_state.viewStateFlags&3)==3) {
                pattern->render(views,projection_views);projection.space=local;projection.viewCount=2;projection.views=projection_views;
                end.layerCount=1;end.layers=layers;
            }
            if(stereo_game_pid){
                const auto current=std::chrono::steady_clock::now();
                if(stereo_screen&&stereo_screen->reconfiguring()){
                    const auto generation=stereo_screen->resource_generation();
                    if(stereo_screen->retire_for_resize()){
                        retired_stereo_copies+=stereo_screen->copies;
                        retired_stereo_projections+=stereo_screen->projection_rendered;
                        retired_retained_projections+=stereo_screen->retained_projections;
                        retired_late_images+=stereo_screen->rejected_late_images;
                        stereo_screen.reset();stereo_retry=current;++source_rebuilds;
                        printf("{\"stereo_source_retired\":true,\"resource_generation\":%llu,\"rebuilds\":%u}\n",generation,source_rebuilds);fflush(stdout);
                    }
                }
                if(!stereo_screen&&current>=stereo_retry){
                    stereo_retry=current+std::chrono::milliseconds(200);
                    ProjectionStatus readiness=ProjectionStatus::producer_unavailable;
                    const bool ready=GameGpuScreen::producer_ready(stereo_game_pid,&readiness,simulator_operator);
                    if(readiness==ProjectionStatus::producer_structure_invalid)projection_evidence.record(readiness,gears_vr::monotonic_ns());
                    if(ready){
                        try{stereo_screen.reset(new GameGpuScreen(device.Get(),queue.Get(),stereo_game_pid,gpu_snapshot,simulator_operator,&stereo_limits,continuous_session));}
                        catch(gears_vr::GpuOpeningDeferred const&){} // Resize won the consumer-claim race; retry after retirement.
                    }
                }
                if(stereo_screen)stereo_screen->poll_world_handoff();
                if(stereo_image_shown&&!startup_quad_queued)end.layerCount=0;
                ProjectionStatus result;
                if(stereo_space_changed&&!continuous_session)result=ProjectionStatus::reference_space_changed;
                else if(stereo_screen&&stereo_screen->reconfiguring())result=ProjectionStatus::source_reconfiguration;
                else if(stereo_screen&&stereo_screen->producer_stopped())result=ProjectionStatus::producer_stopped;
                else if(!startup_panel.allow_projection(window_fallback_enabled)&&!startup_panel.show_pause_projection())result=(menu_mode==gears_menu::Mode::death_menu||menu_mode==gears_menu::Mode::main_menu)?ProjectionStatus::menu_requested:ProjectionStatus::menu_recovery_pending;
                else result=projection_current_status(frame.shouldRender,projection_session_visible(state),view_count,head.locationFlags,view_state.viewStateFlags,simulator_operator);
                bool submitted=false;
                if(result==ProjectionStatus::submitted){
                    if(!stereo_screen)result=ProjectionStatus::producer_unavailable;
                    else {outage_render_attempted=true;outage_copies_before=stereo_screen->copies;
                        if(startup_panel.show_pause_projection()){
                            const auto menu_boundary=startup_panel.minimum_sample_ns();
                            pause_menu_queued=stereo_screen->render_pause_menu(projection,projection_views,stereo_session,result,menu_boundary,space_epoch.epoch(),space_epoch.last_change_time());
                            if(pause_menu_queued){
                                // A GPU copy can wait. Reject a changed/expired menu
                                // lease and any new menu epoch before submission.
                                const auto after_copy=gears_vr::monotonic_ns();
                                const auto authorized=menu_display.poll(after_copy);
                                observe_menu(authorized,after_copy);
                                pause_menu_queued=startup_panel.show_pause_projection()&&startup_panel.minimum_sample_ns()==menu_boundary;
                                if(pause_menu_queued){end.layerCount=1;end.layers=layers;}
                                else result=ProjectionStatus::menu_recovery_pending;
                            }
                        }else submitted=stereo_screen->render_projection(projection,projection_views,stereo_session,result,startup_panel.minimum_sample_ns(),space_epoch.epoch(),space_epoch.last_change_time());}
                }
                projection_frame_status=result;
                if(submitted){
                    end.layerCount=1;end.layers=layers;startup_handoff_queued=true;
                }
            }
            const auto fallback_context=[&](gears_menu::Mode mode){
                return WindowFallbackPolicy::Context{window_fallback_enabled,
                    running&&!exiting&&stop_reason.empty()&&!session_exit.loss_observed&&!session_exit.terminal_exiting&&
                    !projection_evidence.structural_failure&&(!stereo_screen||!stereo_screen->producer_stopped()),
                    projection_session_visible(state),frame.shouldRender!=0,transition_panel_wait(projection_frame_status),
                    startup_handoff_queued,end.layerCount!=0,mode};
            };
            auto fallback=fallback_context(menu_mode);
            if(movie_display){
                movie_fallback_diagnostics.mode(menu_mode);
                const auto observed=menu_display.observed_ns(),now=uint64_t(gears_vr::monotonic_ns());
                movie_fallback_diagnostics.last_packet_age_ns=observed&&now>=observed?std::int64_t(now-observed):-1;
                if(menu_display.cached_mode()==7&&observed&&now>=observed&&now-observed>250000000)
                    ++movie_fallback_diagnostics.expired_lease_frames;
            }
            window_fallback.observe(fallback);
            if(WindowFallbackPolicy::eligible(fallback)){
                if(window_fallback.enter(fallback))startup_screen->begin_menu_epoch();
                ++movie_fallback_diagnostics.eligible;
                const auto render_started=gears_vr::monotonic_ns();
                const bool rendered=startup_screen->render(quad,WindowFallbackPolicy::maximum_age_100ns);
                movie_fallback_diagnostics.rendered(rendered,gears_vr::monotonic_ns()-render_started,*startup_screen);
                if(rendered){
                    // A GPU upload can wait: recheck owner/stop and current menu
                    // authority before submitting pixels from the game window.
                    if(const auto* reason=continuous_control.requested()){
                        startup_panel.cancel();window_fallback.cancel();stop_reason=reason;exiting=true;
                    }
                    const auto after_window=gears_vr::monotonic_ns();
                    const auto fresh_mode=menu_display.poll(after_window);observe_menu(fresh_mode,after_window);
                    window_fallback.observe(fallback_context(fresh_mode));
                    movie_fallback_diagnostics.mode(fresh_mode);
                    if(WindowFallbackPolicy::eligible(fallback_context(fresh_mode))){
                        if(startup_screen->window_fresh(WindowFallbackPolicy::maximum_age_100ns)){
                            window_fallback_queued=true;end.layerCount=1;end.layers=game_layers;
                        }else ++movie_fallback_diagnostics.post_fresh_rejected;
                    }else ++movie_fallback_diagnostics.post_context_rejected;
                }
            }
            if(stereo_game_pid&&end.layerCount==0&&transition_card&&startup_panel.show_transition_panel()&&
               frame.shouldRender&&projection_session_visible(state)&&!projection_evidence.structural_failure&&
               recovery_panel_wait(projection_frame_status,startup_panel)&&transition_card->render(quad)){
                // Upload can wait for a fence. Recheck the independent menu
                // lease afterwards, without extending its acquisition time.
                const auto authorized=menu_display.poll(gears_vr::monotonic_ns());
                if(authorized!=menu_mode)observe_menu(authorized,gears_vr::monotonic_ns());
                if(startup_panel.transition_authorized(authorized)&&recovery_panel_wait(projection_frame_status,startup_panel)&&
                   running&&!exiting&&!projection_evidence.structural_failure&&
                   (!stereo_screen||!stereo_screen->producer_stopped())){
                    transition_panel_queued=true;end.layerCount=1;end.layers=game_layers;
                }
            }
            // A positively observed world handoff has its own display wait;
            // menu witnesses and projection ticket gates remain unchanged.
            if(continuous_session&&stereo_game_pid&&end.layerCount==0&&transition_card&&stereo_screen&&
               frame.shouldRender&&projection_session_visible(state)&&running&&!exiting&&
               !session_exit.loss_observed&&!session_exit.terminal_exiting&&stop_reason.empty()&&
               !projection_evidence.structural_failure&&world_handoff_panel_wait(projection_frame_status)&&
               menu_mode==gears_menu::Mode::gameplay&&stereo_screen->handoff_display_eligible(true)){
                bool stop=false;
                if(continuous_session){if(const auto* reason=continuous_control.requested()){
                    startup_panel.cancel();stop_reason=reason;exiting=true;stop=true;
                }}
                if(!stop&&transition_card->render(quad)){
                    // Upload may wait: reread world, owner/stop and menu leases.
                    const bool world_authorized=stereo_screen->handoff_display_eligible(true);
                    if(continuous_session){if(const auto* reason=continuous_control.requested()){
                        startup_panel.cancel();stop_reason=reason;exiting=true;stop=true;
                    }}
                    const auto after_card=gears_vr::monotonic_ns();
                    const auto authorized=menu_display.poll(after_card);observe_menu(authorized,after_card);
                    if(!stop&&world_authorized&&authorized==gears_menu::Mode::gameplay&&running&&!exiting&&
                       !session_exit.loss_observed&&!session_exit.terminal_exiting&&!projection_evidence.structural_failure){
                        world_handoff_panel_state=stereo_screen->handoff_display_state();
                        world_handoff_panel_queued=true;end.layerCount=1;end.layers=game_layers;
                    }
                }
            }
            // Ordinary WGC/game layers win. This independent generic card is
            // eligible only before the first successfully ended projection.
            const auto startup_wait_context=[&]{
                return StartupPanelPolicy::StartupWaitContext{end.layerCount,frame.shouldRender!=0,
                    projection_session_visible(state),running,exiting||!stop_reason.empty(),
                    stereo_screen&&stereo_screen->producer_stopped(),
                    session_exit.loss_observed||session_exit.terminal_exiting,
                    projection_evidence.structural_failure,startup_panel_wait(projection_frame_status)};
            };
            const auto startup_wait_stop=[&]{
                if(continuous_session){if(const auto* reason=continuous_control.requested()){
                    startup_panel.cancel();stop_reason=reason;exiting=true;end.layerCount=0;
                }}
            };
            if(startup_wait_card&&startup_panel.show_startup_wait(startup_wait_context())){
                startup_wait_stop();
                if(startup_panel.show_startup_wait(startup_wait_context())&&startup_wait_card->render(quad)){
                    // First upload may wait. Recheck owner/stop and menu using
                    // the existing controls, without extending any image lease.
                    startup_wait_stop();
                    const auto after_card=gears_vr::monotonic_ns();
                    const auto authorized=menu_display.poll(after_card);
                    observe_menu(authorized,after_card);
                    if(startup_panel.show_startup_wait(startup_wait_context())){
                        startup_wait_card_queued=true;end.layerCount=1;end.layers=game_layers;
                    }
                }
            }
            // A VIEW-space, session-owned card needs no game image or tracked
            // world pose. Keep current_untracked out of all game-pixel fallbacks.
            const auto tracking_card_eligible=[&]{
                return tracking_recovery_card&&
                    tracking_recovery_panel_wait(projection_frame_status,end.layerCount)&&
                    frame.shouldRender&&projection_session_visible(state)&&running&&!exiting&&stop_reason.empty()&&
                    !session_exit.loss_observed&&!session_exit.terminal_exiting&&!projection_evidence.structural_failure&&
                    (!stereo_screen||!stereo_screen->producer_stopped());
            };
            if(tracking_card_eligible()&&tracking_recovery_card->render(quad)&&tracking_card_eligible()){
                tracking_recovery_card_queued=true;end.layerCount=1;end.layers=game_layers;
            }
            // A first upload can wait. Honor a stop which arrived during it,
            // still balancing the already begun XR frame with zero layers.
            if(continuous_session){if(const auto* reason=continuous_control.requested()){
                startup_panel.cancel();stop_reason=reason;exiting=true;end.layerCount=0;
                tracking_recovery_card_queued=world_handoff_panel_queued=startup_wait_card_queued=transition_panel_queued=startup_handoff_queued=startup_quad_queued=pause_menu_queued=false;
            }}
            const auto ended=xr_wait_diagnostics::recorder.measure(xr_wait_diagnostics::Operation::end_frame,
                [&]{return xrEndFrame(session,&end);},[]{return xr_wait_diagnostics::current_context;},
                [](XrResult r){return r==XR_TIMEOUT_EXPIRED?xr_wait_diagnostics::Outcome::timeout:XR_FAILED(r)?xr_wait_diagnostics::Outcome::error:xr_wait_diagnostics::Outcome::success;});
            xr_check(ended,"xrEndFrame");
#if defined(GEARS_SIXDOF_TICKET_DIAGNOSTICS) && GEARS_SIXDOF_TICKET_DIAGNOSTICS
            if(stereo_screen&&projection_frame_status==ProjectionStatus::submitted)
                stereo_screen->observe_sixdof_submission(ended,end,projection,space_epoch.epoch());
#endif
            if(window_fallback_queued&&ended==XR_SUCCESS&&end.layerCount==1&&end.layers==game_layers){
                if(!window_fallback_shown){const auto image=startup_screen->window_snapshot();
                    printf("{\"game_window_fallback_first_frame\":true,\"both_eyes_common_panel\":true,\"panel_counts_as_stereo\":false,\"capture_generation\":%llu,\"capture_epoch_100ns\":%lld,\"captured_100ns\":%lld,\"maximum_age_ms\":250,\"reason\":\"%s\"}\n",
                        image.generation,image.epoch_100ns,image.captured_100ns,projection_status_name(projection_frame_status));fflush(stdout);}
                window_fallback_shown=true;
            }
            const bool world_handoff_projection=stereo_screen&&projection_frame_status==ProjectionStatus::submitted&&
                projection_end_succeeded(startup_handoff_queued,ended,end.layerCount,end.layers==layers)&&
                stereo_screen->handoff_projection_completed();
            if(world_handoff_projection){
                // A fast transition need not display a waiting card. Record
                // the same admitted world/image and real xrEndFrame proof even
                // when there was no card to retire.
                const auto world=stereo_screen->handoff_display_state();
                const auto image=stereo_screen->diagnostic_snapshot();
                printf("{\"world_handoff_projection\":true,\"observed_ns\":%lld,\"generation\":%llu,\"world_valid\":%s,\"quiescing\":%s,\"ticket\":%llu,\"sample_ns\":%lld,\"end_frame_succeeded\":true,\"replacement_projection\":true}\n",
                    gears_vr::monotonic_ns(),world.generation,world.valid?"true":"false",world.quiescing?"true":"false",
                    image.ticket_id,image.received_ns);fflush(stdout);
            }
            const bool world_handoff_shown=world_handoff_panel_queued&&ended==XR_SUCCESS&&end.layerCount==1&&end.layers==game_layers;
            if(world_handoff_shown){
                ++world_handoff_panel_layers;last_world_handoff_panel=world_handoff_panel_state;
                if(!world_handoff_panel_active){
                    printf("{\"world_handoff_card_first_frame\":true,\"observed_ns\":%lld,\"generation\":%llu,\"quiescing\":%s,\"cause\":\"%s\",\"panel_counts_as_stereo\":false}\n",
                        gears_vr::monotonic_ns(),world_handoff_panel_state.generation,world_handoff_panel_state.quiescing?"true":"false",projection_status_name(projection_frame_status));fflush(stdout);
                }
            }else if(world_handoff_panel_active){
                const auto world=stereo_screen?stereo_screen->handoff_display_state():WorldHandoffDisplay::State{};
                printf("{\"world_handoff_card_ended\":true,\"observed_ns\":%lld,\"generation\":%llu,\"world_valid\":%s,\"quiescing\":%s,\"last_card_generation\":%llu,\"replacement_projection\":%s}\n",
                    gears_vr::monotonic_ns(),world.generation,world.valid?"true":"false",world.quiescing?"true":"false",last_world_handoff_panel.generation,world_handoff_projection?"true":"false");fflush(stdout);
            }
            world_handoff_panel_active=world_handoff_shown;
            const bool startup_wait_shown=startup_wait_card_queued&&ended==XR_SUCCESS&&end.layerCount==1&&end.layers==game_layers;
            if(startup_wait_shown){
                if(++startup_wait_card_layers==1){
                    printf("{\"startup_wait_card_first_frame\":true,\"observed_ns\":%lld,\"cause\":\"%s\",\"source\":\"session_owned_card\",\"panel_counts_as_stereo\":false}\n",
                        gears_vr::monotonic_ns(),projection_status_name(projection_frame_status));fflush(stdout);
                }
            }
            if(tracking_recovery_card_queued&&ended==XR_SUCCESS&&end.layerCount==1&&end.layers==game_layers){
                if(++tracking_recovery_card_layers==1){
                    printf("{\"tracking_recovery_card_first_frame\":true,\"cause\":\"current_untracked\",\"source\":\"session_owned_card\",\"panel_counts_as_stereo\":false}\n");fflush(stdout);
                }
            }
            const bool transition_shown=transition_panel_queued&&ended==XR_SUCCESS&&end.layerCount==1&&end.layers==game_layers;
            if(transition_shown){
                ++transition_panel_layers;
                if(!transition_panel_active){
                    printf("{\"transition_panel_first_frame\":true,\"observed_ns\":%lld,\"cause\":\"%s\",\"source\":\"session_owned_card\",\"panel_counts_as_stereo\":false}\n",
                        gears_vr::monotonic_ns(),projection_status_name(projection_frame_status));fflush(stdout);
                    if(gpu_snapshot&&!transition_preview_saved){
                        transition_preview_saved=transition_card->save_source_preview(std::string(argv[2])+"-transition-card.ppm");
                        printf("{\"transition_card_source_preview\":true,\"saved\":%s,\"gpu_readback\":false}\n",transition_preview_saved?"true":"false");fflush(stdout);
                    }
                }
            }else if(transition_panel_active){
                printf("{\"transition_panel_ended\":true,\"observed_ns\":%lld,\"replacement_projection\":%s}\n",
                    gears_vr::monotonic_ns(),ended==XR_SUCCESS&&end.layerCount==1&&end.layers==layers?"true":"false");fflush(stdout);
            }
            transition_panel_active=transition_shown;
            if(pause_menu_queued&&ended==XR_SUCCESS&&end.layerCount==1&&end.layers==layers){
                startup_panel.menu_rendered();
                ++pause_menu_projection_layers;
                if(last_pause_menu_entry!=menu_entries){
                    last_pause_menu_entry=menu_entries;const auto snapshot=stereo_screen->diagnostic_snapshot();
                    printf("{\"pause_menu_first_frame\":true,\"epoch\":%u,\"end_frame_succeeded\":true,\"layer_type\":\"projection\",\"view_count\":%u,\"both_eyes_common_panel\":false,\"ticket\":%llu,\"sample_ns\":%lld,\"minimum_sample_ns\":%lld,\"eye_extent\":[%d,%d]}\n",
                        menu_entries,projection.viewCount,snapshot.ticket_id,snapshot.received_ns,startup_panel.minimum_sample_ns(),
                        projection_views[0].subImage.imageRect.extent.width,projection_views[0].subImage.imageRect.extent.height);fflush(stdout);
                }
            }
            if(stereo_game_pid&&ended==XR_SUCCESS&&projection_frame_status==ProjectionStatus::submitted&&
               end.layerCount==1&&end.layers==layers&&stereo_screen&&
               (last_submitted_space_epoch!=space_epoch.epoch()||last_submitted_resource_generation!=stereo_screen->resource_generation())){
                last_submitted_space_epoch=space_epoch.epoch();
                last_submitted_resource_generation=stereo_screen->resource_generation();
                printf("{\"reference_epoch_first_projection\":true,\"space_epoch\":%llu,\"resource_generation\":%llu,\"end_frame_succeeded\":true,\"monotonic_ns\":%lld}\n",
                    last_submitted_space_epoch,last_submitted_resource_generation,gears_vr::monotonic_ns());fflush(stdout);
            }
            if(startup_handoff_queued&&!projection_end_succeeded(true,ended,end.layerCount,end.layers==layers))startup_handoff_queued=false;
            if(startup_handoff_queued){
                const bool recovering=startup_panel.minimum_sample_ns()!=0;
                stereo_image_shown=true;
                if(window_fallback_shown){printf("{\"game_window_fallback_recovered\":true,\"native_projection_end_succeeded\":true}\n");fflush(stdout);}
                window_fallback_shown=false;window_fallback.projection_completed(true);
                startup_handoff_queued=startup_panel.projection_rendered(true,window_fallback_enabled);
                if(startup_screen&&!window_fallback.active()&&startup_panel.suspend_window_capture())startup_screen->suspend_capture();
                if(startup_handoff_queued&&!menu_state_file)startup_screen.reset();
                if(startup_handoff_queued&&recovering){++menu_recoveries;
                    printf("{\"menu_display_recovered\":true,\"epoch\":%u,\"end_frame_succeeded\":true}\n",menu_entries);fflush(stdout);}
            }
            if(startup_quad_queued&&ended==XR_SUCCESS&&end.layerCount==1&&end.layers==game_layers){
                startup_panel.menu_rendered();
                ++startup_quad_layers;
                if(menu_mode==gears_menu::Mode::main_menu){
                    ++main_menu_quad_layers;
                    if(last_main_menu_entry!=menu_entries){
                        last_main_menu_entry=menu_entries;const auto image=startup_screen->window_snapshot();
                        printf("{\"main_menu_first_frame\":true,\"epoch\":%u,\"source\":\"wgc_full_client\",\"owner_sequence\":%llu,\"capture_epoch_100ns\":%lld,\"captured_100ns\":%lld,\"source_generation\":%llu,\"hwnd\":%llu,\"extent\":[%u,%u],\"source_hash_algorithm\":\"pixel_fnv4\",\"source_hash\":%llu,\"end_frame_succeeded\":true,\"both_eyes_common_panel\":true,\"panel_counts_as_stereo\":false}\n",
                            menu_entries,menu_display.sequence(),image.epoch_100ns,image.captured_100ns,
                            image.generation,image.hwnd,image.width,image.height,image.hash);fflush(stdout);
                        if(gpu_snapshot&&main_menu_snapshots<2){
                            ++main_menu_snapshots;
                            startup_screen->save_source_preview(std::string(argv[2])+"-main-menu-"+std::to_string(menu_entries)+".ppm");
                        }
                    }
                }
                if(menu_mode==gears_menu::Mode::death_menu){++menu_quad_layers;
                    if(menu_quad_layers==1){printf("{\"death_menu_first_frame\":true,\"end_frame_succeeded\":true,\"panel_counts_as_stereo\":false}\n");fflush(stdout);}}
                if(startup_quad_layers==1){printf("{\"startup_menu_first_frame\":true,\"end_frame_succeeded\":true,\"panel_counts_as_stereo\":false}\n");fflush(stdout);}
            }
            if(startup_handoff_queued){
                printf("{\"startup_menu_handoff\":true,\"startup_quad_layers\":%u,\"ticketed_projection_rendered\":true,\"end_frame_succeeded\":true,\"one_way\":%s}\n",startup_quad_layers,menu_state_file?"false":"true");fflush(stdout);
            }
            if(movie_display){
                if(window_fallback_queued){if(ended==XR_SUCCESS)++movie_fallback_diagnostics.submitted;else ++movie_fallback_diagnostics.end_failed;}
                movie_fallback_diagnostics.report(gears_vr::monotonic_ns());
            }
            if(stereo_game_pid)projection_evidence.record(ended==XR_SUCCESS?projection_frame_status:ProjectionStatus::session_lost,gears_vr::monotonic_ns());
            if(diagnostic_outage||diagnostic_image_tickets){
                outage_diagnostics::Row row;
                // Observation follows xrEndFrame. Ticket validation used an earlier
                // timestamp inside render_projection; do not infer its gate age.
                row.ns=gears_vr::monotonic_ns();
                const auto status=ended==XR_SUCCESS?projection_frame_status:ProjectionStatus::session_lost;
                row.status=projection_status_name(status);row.submitted=status==ProjectionStatus::submitted;
                row.terminal=status==ProjectionStatus::producer_stopped;
                if(stereo_screen){row.image=stereo_screen->diagnostic_snapshot();row.image.copies+=retired_stereo_copies;}
                row.new_image_valid=outage_render_attempted;
                row.new_image=outage_render_attempted&&stereo_screen&&stereo_screen->copies>outage_copies_before;
                // Menu quads are real composition, even though they are not
                // stereo projection evidence. Keep the ticket recorder below
                // on its original projection-only contract.
                const char* layer_kind=end.layerCount==0?"none":
                    end.layers==layers?(pause_menu_queued?"pause_menu_projection":"projection"):
                    end.layers==game_layers?(tracking_recovery_card_queued?"tracking_recovery_card":world_handoff_panel_queued?"world_handoff_card":startup_wait_card_queued?"startup_wait_card":transition_panel_queued?"transition_panel":
                        (startup_quad_queued?"startup_panel":"quad")):"other";
                row.menu_mode=static_cast<unsigned>(menu_mode);row.session_state=static_cast<int>(state);
                row.should_render=frame.shouldRender!=0;row.panel_requested=menu_panel_requested;
                row.render_attempted=menu_render_attempted;row.render_result=menu_render_result;
                row.authorization_rejected=menu_authorization_rejected;
                if(startup_screen)row.menu=outage_diagnostics::menu_snapshot_for_frame(startup_screen->diagnostic_snapshot(),menu_render_attempted);
                outage.observe(outage_diagnostics::composition_row(row,ended==XR_SUCCESS,end.layerCount,layer_kind));
                image_tickets.observe(row);
            }
            if(continuous_session&&projection_frame_status==ProjectionStatus::producer_stopped){stop_reason="producer_stopped";exiting=true;}
            if(continuous_session&&projection_evidence.structural_failure){stop_reason="projection_structure_failure";exiting=true;}
            ++frames;if(frame.shouldRender)++render;if(state==XR_SESSION_STATE_FOCUSED)++focused;
            if((head.locationFlags&3)==3)++valid;if((head.locationFlags&12)==12)++tracked;
            const auto& p=head.pose;const auto& left=views[0];const auto& right=views[1];
            // Full-rate readiness rows for three seconds, then one per second.
            // Rendering and all safety/evidence gates still run every frame.
            const auto csv_now=std::chrono::steady_clock::now();
            if(!continuous_session||frames<=240||csv_now>=next_csv){
            fprintf(csv,"%u,%lld,%d,%u,%llu,%llu,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g",
                frames,frame.predictedDisplayTime,int(state),frame.shouldRender,view_state.viewStateFlags,head.locationFlags,
                p.position.x,p.position.y,p.position.z,p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w,
                left.pose.position.x,left.pose.position.y,left.pose.position.z,right.pose.position.x,right.pose.position.y,right.pose.position.z,
                left.fov.angleLeft,left.fov.angleRight,left.fov.angleUp,left.fov.angleDown);
            fprintf(csv,",%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%llu,%llu,%lld,%u,%u\n",
                left.pose.orientation.x,left.pose.orientation.y,left.pose.orientation.z,left.pose.orientation.w,
                right.pose.orientation.x,right.pose.orientation.y,right.pose.orientation.z,right.pose.orientation.w,
                right.fov.angleLeft,right.fov.angleRight,right.fov.angleUp,right.fov.angleDown,
                stereo_sample.session,stereo_sample.sequence,stereo_sample.received_ns,unsigned(stereo_sent),unsigned(stereo_sample.source));
            if(continuous_session||frames%60==0)fflush(csv);
            next_csv=csv_now+std::chrono::seconds(1);
            }
        }
        if(continuous_session){if(const auto* reason=continuous_control.owner_reason())stop_reason=reason;}
        if(bridge_pid)bridge.publish({});
        if(stereo_pid)stereo_bridge.publish({});
        shutdown_diagnostics::marker("frame_loop_finished");
#if defined(GEARS_SIXDOF_TICKET_DIAGNOSTICS) && GEARS_SIXDOF_TICKET_DIAGNOSTICS
        if(stereo_screen)stereo_screen->dump_sixdof();
#endif
        outage.dump();
        image_tickets.dump();
        if(running&&!session_exit.terminal_exiting&&stop_reason!="session_lost"&&stop_reason!="instance_lost") {
            shutdown_diagnostics::marker("request_exit_session_enter");
            xr_check(xrRequestExitSession(session),"xrRequestExitSession");
            shutdown_diagnostics::marker("request_exit_session_returned");
            auto stop_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
            while(running && !session_exit.terminal_exiting && !session_exit.loss_observed && std::chrono::steady_clock::now()<stop_deadline) {
                XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
                const auto result=xrPollEvent(instance,&event);xr_check(result,"poll session exit");
                if(result==XR_SUCCESS && event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                    auto change=reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                    if(change->session!=session)continue;
                    state=change->state;session_exit.observe(state);
                    shutdown_diagnostics::marker("exit_session_state",true,int(state));
                    if(change->state==XR_SESSION_STATE_STOPPING) {shutdown_diagnostics::marker("end_session_enter");xr_check(xrEndSession(session),"end stopped session");
                        shutdown_diagnostics::marker("end_session_returned");session_exit.mark_end_success();running=false;}
                } else if(result==XR_SUCCESS && event.type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
                    session_exit.loss_observed=true;
                } else std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        printf("{\"frames\":%u,\"valid_pose_frames\":%u,\"tracked_pose_frames\":%u,\"should_render_frames\":%u,\"focused_frames\":%u,\"last_session_state\":%d,\"submitted_image_layers\":%u,\"left_checksum\":%llu,\"right_checksum\":%llu}\n",
               frames,valid,tracked,render,focused,int(state),pattern?pattern->rendered:0,
               pattern?pattern->checksums[0]:0,pattern?pattern->checksums[1]:0);
        if(screen)printf("{\"game_quad_layers\":%u,\"game_texture_uploads\":%u,\"game_image_changes\":%u,\"first_source_hash\":%llu,\"last_source_hash\":%llu,\"gpu_pixels_verified\":%s,\"stereo_scene_rendering\":false}\n",screen->rendered,screen->uploads,screen->changes,screen->first_hash,screen->last_hash,screen->gpu_pixels_verified?"true":"false");
        if(gpu_screen)printf("{\"gpu_game_quad_layers\":%u,\"gpu_game_copies\":%u,\"per_frame_cpu_readback\":false,\"stereo_scene_rendering\":false}\n",gpu_screen->rendered,gpu_screen->copies);
        if(stereo_pid)printf("{\"stereo_bridge_session\":%llu,\"published\":%llu,\"publish_skipped\":%llu,\"reference_space_changed\":%s,\"game_projection_layer\":%s}\n",
            stereo_session,stereo_published,stereo_publish_skipped,stereo_space_changed?"true":"false",stereo_screen&&stereo_screen->projection_rendered?"true":"false");
        if(stereo_game_pid)printf("{\"stereo_game_projection_layers\":%u,\"stereo_game_copies\":%u,\"matching_render_ticket_required\":true}\n",
            retired_stereo_projections+(stereo_screen?stereo_screen->projection_rendered:0),retired_stereo_copies+(stereo_screen?stereo_screen->copies:0));
        if(stereo_game_pid)printf("{\"image_continuity_evidence\":true,\"continuous_mode\":%s,\"retained_over_100ms\":%u,\"late_images_rejected\":%u,\"new_image_max_age_ns\":%lld,\"retained_max_age_ns\":%lld,\"original_render_pose_preserved\":true}\n",
            continuous_session?"true":"false",retired_retained_projections+(stereo_screen?stereo_screen->retained_projections:0),
            retired_late_images+(stereo_screen?stereo_screen->rejected_late_images:0),
            continuous_session?projection_new_image_max_age:100000000,continuous_session?projection_retained_image_max_age:100000000);
        if(stereo_game_pid)printf("{\"reference_epoch_evidence\":true,\"current_epoch\":%llu,\"last_submitted_epoch\":%llu,\"current_epoch_submitted\":%s}\n",
            space_epoch.epoch(),last_submitted_space_epoch,last_submitted_space_epoch==space_epoch.epoch()?"true":"false");
        if(stereo_game_pid)printf("{\"source_rebuild_evidence\":true,\"rebuilds\":%u,\"last_submitted_generation\":%llu,\"current_generation\":%llu,\"current_generation_submitted\":%s}\n",
            source_rebuilds,last_submitted_resource_generation,stereo_screen?stereo_screen->resource_generation():0,
            stereo_screen&&last_submitted_resource_generation==stereo_screen->resource_generation()?"true":"false");
        if(stereo_game_pid&&!continuous_session)projection_evidence.print();
        bool display_ok=!screen || (screen->rendered && screen->gpu_pixels_verified);
        if(movie_display)movie_fallback_diagnostics.report(gears_vr::monotonic_ns(),true);
        if(startup_window)printf("{\"startup_menu_evidence\":true,\"startup_quad_layers\":%u,\"stereo_handoff\":%s,\"panel_counts_as_stereo\":false}\n",startup_quad_layers,stereo_image_shown?"true":"false");
        if(menu_state_file)printf("{\"menu_recovery_evidence\":true,\"entries\":%u,\"quad_layers\":%u,\"recoveries\":%u,\"panel_counts_as_stereo\":false}\n",menu_entries,menu_quad_layers,menu_recoveries);
        if(menu_state_file)printf("{\"main_menu_evidence\":true,\"quad_layers\":%u,\"source\":\"wgc_full_client\",\"panel_counts_as_stereo\":false}\n",main_menu_quad_layers);
        if(menu_state_file)printf("{\"pause_menu_evidence\":true,\"projection_layers\":%u,\"view_count\":2,\"both_eyes_common_panel\":false}\n",pause_menu_projection_layers);
        if(startup_window)printf("{\"startup_wait_card_evidence\":true,\"quad_layers\":%u,\"uploads\":%u,\"source\":\"session_owned_card\",\"panel_counts_as_stereo\":false}\n",
            startup_wait_card_layers,startup_wait_card?startup_wait_card->uploads:0);
        if(menu_state_file)printf("{\"transition_panel_evidence\":true,\"quad_layers\":%u,\"uploads\":%u,\"source\":\"session_owned_card\",\"panel_counts_as_stereo\":false}\n",transition_panel_layers,transition_card?transition_card->uploads:0);
        if(menu_state_file)printf("{\"world_handoff_card_evidence\":true,\"quad_layers\":%u,\"source\":\"session_owned_card\",\"panel_counts_as_stereo\":false}\n",world_handoff_panel_layers);
        display_ok=display_ok && (!gpu_screen || (gpu_screen->rendered && gpu_screen->copies));
        const auto scene_copies=retired_stereo_copies+(stereo_screen?stereo_screen->copies:0);
        display_ok=display_ok && (!stereo_game_pid || (stereo_screen && stereo_screen->projection_rendered && scene_copies &&
            last_submitted_space_epoch==space_epoch.epoch()&&
            last_submitted_resource_generation==stereo_screen->resource_generation()&&
            (continuous_session?!projection_evidence.structural_failure:projection_evidence.sustained()&&!stereo_space_changed)));
        stereo_screen.reset();
        tracking_recovery_card.reset();
        startup_wait_card.reset();
        transition_card.reset();
        startup_screen.reset();
        gpu_screen.reset();
        screen.reset();
        pattern.reset();
        const bool objects_destroyed=cleanup();
        const bool shutdown_completed=session_exit.complete(objects_destroyed,running,xr_gpu_teardown_unsafe);
        printf("{\"session_exit_evidence\":true,\"last_state\":%d,\"stopping_observed\":%s,\"end_session_succeeded\":%s,\"terminal_exiting_observed\":%s,\"terminal_exit_without_end\":%s,\"loss_observed\":%s,\"objects_destroyed\":%s,\"shutdown_completed\":%s}\n",
            int(session_exit.last_state),session_exit.stopping_observed?"true":"false",session_exit.end_succeeded?"true":"false",
            session_exit.terminal_exiting?"true":"false",session_exit.exited_without_end()?"true":"false",session_exit.loss_observed?"true":"false",
            objects_destroyed?"true":"false",shutdown_completed?"true":"false");
        if(continuous_session){
            if(const auto* reason=continuous_control.owner_reason())stop_reason=reason;
            printf("{\"continuous_owner_evidence\":true,\"owner_pid\":%lu,\"owner_creation_time\":%llu,\"owner_lost\":%s}\n",continuous_owner_pid,continuous_owner_creation,continuous_control.owner_reason()?"true":"false");
            printf("{\"continuous_session_evidence\":true,\"stop_reason\":\"%s\",\"orderly_stop\":%s,\"structural_failure\":%s,\"submitted_frames\":%llu,\"scene_copies\":%u,\"span_seconds\":%.9g,\"maximum_submission_gap_ms\":%.9g,\"shutdown_completed\":%s,\"csv_sampled\":true,\"physical_tracking_verified\":false,\"simulation_not_physical_tracking\":%s}\n",
                stop_reason.c_str(),continuous_orderly_reason(stop_reason)?"true":"false",projection_evidence.structural_failure?"true":"false",
                projection_evidence.counts[0],scene_copies,projection_evidence.span_seconds(),double(projection_evidence.maximum_gap)/1e6,
                shutdown_completed?"true":"false",(simulated_runtime||simulator_operator)?"true":"false");projection_evidence.print_rejections();fclose(csv);
            const int code=frames&&display_ok&&continuous_orderly_reason(stop_reason)&&shutdown_completed?0:1;
            shutdown_diagnostics::marker("main_return_pending",true,code);return code;
        }
        fclose(csv);const int code=frames && display_ok && shutdown_completed?0:1;
        shutdown_diagnostics::marker("main_return_pending",true,code);return code;
    }catch(std::exception const& error) {fprintf(stderr,"Pose probe error: %s\n",error.what());
        outage.dump();
        image_tickets.dump();
        shutdown_diagnostics::marker("main_exception_handler");
        if(bridge_pid)bridge.publish({});
        if(stereo_pid)stereo_bridge.publish({});
        if(stereo_game_pid){
        const bool malformed=strstr(error.what(),"metadata")||strstr(error.what(),"ticket rejected");
        projection_evidence.record(malformed?ProjectionStatus::producer_structure_invalid:ProjectionStatus::probe_failure,gears_vr::monotonic_ns());
        if(!continuous_session)projection_evidence.print();}
        const bool destroyed=cleanup();
        if(continuous_session){
            if(const auto* reason=continuous_control.owner_reason())stop_reason=reason;
            if(stop_reason.empty())stop_reason="probe_failure";
            printf("{\"continuous_owner_evidence\":true,\"owner_pid\":%lu,\"owner_creation_time\":%llu,\"owner_lost\":%s}\n",continuous_owner_pid,continuous_owner_creation,continuous_control.owner_reason()?"true":"false");
            printf("{\"continuous_session_evidence\":true,\"stop_reason\":\"%s\",\"orderly_stop\":false,\"structural_failure\":true,\"submitted_frames\":%llu,\"span_seconds\":%.9g,\"maximum_submission_gap_ms\":%.9g,\"shutdown_completed\":false,\"xr_objects_destroyed\":%s,\"error_reported\":true,\"physical_tracking_verified\":false}\n",
                stop_reason.c_str(),projection_evidence.counts[0],projection_evidence.span_seconds(),double(projection_evidence.maximum_gap)/1e6,destroyed?"true":"false");
            projection_evidence.print_rejections();
        }
        fclose(csv);shutdown_diagnostics::marker("main_return_pending",true,1);return 1;}
}
