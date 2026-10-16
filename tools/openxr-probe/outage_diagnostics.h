#pragma once
#include <array>
#include <cstdint>
#include <cstdio>

// Render-thread observations only. Fixed storage, no IO until dump(). These
// timestamps describe post-xrEndFrame observations, not GPU/publisher causality.
// age_ns is observation age, NOT the earlier render_projection gate age.
namespace outage_diagnostics {
enum class CaptureResult : unsigned {not_attempted,no_frame,pre_epoch,future,expired,resize,invalid_crop,capture_error,accepted,window_unavailable};
enum class MenuRenderReason : unsigned {not_attempted,in_progress,window_unavailable,generation_changed,capture_not_fresh,no_uploaded_image,image_expired,rendered};
inline const char* capture_result_name(CaptureResult value) noexcept {
    switch(value){
    case CaptureResult::no_frame:return "no_frame";case CaptureResult::pre_epoch:return "pre_epoch";
    case CaptureResult::future:return "future";case CaptureResult::expired:return "expired";
    case CaptureResult::resize:return "resize";case CaptureResult::invalid_crop:return "invalid_crop";
    case CaptureResult::capture_error:return "capture_error";case CaptureResult::accepted:return "accepted";
    case CaptureResult::window_unavailable:return "window_unavailable";default:return "not_attempted";}
}
inline const char* menu_render_reason_name(MenuRenderReason value) noexcept {
    switch(value){
    case MenuRenderReason::in_progress:return "in_progress";case MenuRenderReason::window_unavailable:return "window_unavailable";
    case MenuRenderReason::generation_changed:return "generation_changed";case MenuRenderReason::capture_not_fresh:return "capture_not_fresh";
    case MenuRenderReason::no_uploaded_image:return "no_uploaded_image";case MenuRenderReason::image_expired:return "image_expired";
    case MenuRenderReason::rendered:return "rendered";default:return "not_attempted";}
}
struct MenuCaptureSnapshot {
    CaptureResult result=CaptureResult::not_attempted;
    std::uint64_t attempts=0,no_frames=0,accepted=0,generation=0,hwnd=0;
    std::int64_t last_taken_100ns=0,epoch_100ns=0,accepted_100ns=0;
    std::int64_t future_ahead_last_100ns=0,future_ahead_max_100ns=0;
    std::uint64_t pending_reused=0,pending_dropped=0;
};
struct MenuSnapshot {
    MenuCaptureSnapshot capture{};
    bool cached_only=true,render_attempted=false,update_attempted=false,updated=false;
    bool available_checked=false,available=false,current=false,fresh_checked=false,fresh=false,generation_changed=false;
    std::int64_t fresh_now_100ns=0;
    MenuRenderReason reason=MenuRenderReason::not_attempted;
};
// A skipped render retains labelled historical capture data, never an old
// render reason as the explanation for the current frame.
inline MenuSnapshot menu_snapshot_for_frame(MenuSnapshot const& observed,bool attempted) noexcept {
    if(attempted)return observed;
    MenuSnapshot result;result.capture=observed.capture;return result;
}
struct ImageSnapshot {
    bool receiver_valid=false,ticket_valid=false;
    std::uint64_t acquired_serial=0, ticket_id=0, session=0, sequence=0;
    std::int64_t received_ns=0;
    unsigned copies=0,source=0;
    std::uint64_t head_flags=0,view_flags=0;
    std::array<double,3> head_position{};
    std::array<double,4> head_orientation{}; // x,y,z,w; frozen image-ticket pose.
};
struct Row {
    std::int64_t ns=0;
    const char* status="not_observed";
    bool submitted=false,new_image=false,new_image_valid=false,terminal=false;
    ImageSnapshot image{};
    unsigned layer_count=0;
    std::array<char,32> layer_kind{{'n','o','n','e',0}};
    MenuSnapshot menu{};
    unsigned menu_mode=0;int session_state=0;
    bool should_render=false,panel_requested=false,render_attempted=false,render_result=false,authorization_rejected=false;
};
// Own the composition label and ordinary diagnostic values, never XR layer
// pointers. The input retains its projection-only submitted meaning for image
// ticket diagnostics; this copy describes successful nonempty xrEndFrame.
inline Row composition_row(Row const& projection,bool end_succeeded,unsigned layer_count,const char* layer_kind) noexcept {
    Row result=projection;result.submitted=end_succeeded&&layer_count>0;result.layer_count=layer_count;
    result.layer_kind.fill(0);
    const char* kind=layer_kind&&*layer_kind?layer_kind:"none";
    for(unsigned i=0;i+1<result.layer_kind.size()&&kind[i];++i)result.layer_kind[i]=kind[i];
    return result;
}
struct Boundary {
    Row start{},first_missing{},threshold{},last_missing{},end{};
    bool first_missing_valid=false,threshold_valid=false,last_missing_valid=false,end_valid=false;
    std::int64_t duration_ns=0;
};
class Recorder {
    static constexpr std::int64_t threshold_ns=100000000;
    std::array<Row,8> pre{};
    unsigned pre_count=0,pre_next=0;
    Row last_success{};
    bool have_success=false,in_gap=false,first_open=false,first_complete=false;
    Boundary current{},longest{};
    void append(Row const& r) noexcept {
        if(count<rows.size())rows[count++]=r;else ++dropped;
    }
    void remember(Row const& r) noexcept {pre[pre_next]=r;pre_next=(pre_next+1)%pre.size();if(pre_count<pre.size())++pre_count;}
public:
    bool enabled=false,dumped=false;
    // Reserve first-start plus six boundary snapshots: <=256 emitted items.
    std::array<Row,249> rows{};
    unsigned count=0;
    std::uint64_t dropped=0,later_windows=0;
    void observe(Row const& r) noexcept {
        if(!enabled)return;
        // producer_stopped is a termination boundary, not another elapsed
        // outage sample. Match the existing submission evidence policy.
        if(r.terminal){terminal_row=r;terminal_valid=true;return;}
        if(!have_success){if(r.submitted){last_success=r;have_success=true;}remember(r);return;}
        if(r.ns<=last_success.ns){++invalid_clock;return;}
        if(!in_gap){current={};current.start=last_success;in_gap=true;}
        current.duration_ns=r.ns-last_success.ns;
        if(!r.submitted){
            if(!current.first_missing_valid){current.first_missing=r;current.first_missing_valid=true;}
            current.last_missing=r;current.last_missing_valid=true;
        }
        if(current.duration_ns>threshold_ns&&!current.threshold_valid){
            current.threshold=r;current.threshold_valid=true;
            if(!first_started){
                first_started=true;first_open=true;first_start=last_success;
                for(unsigned i=0;i<pre_count;++i){const auto& before=pre[(pre_next+pre.size()-pre_count+i)%pre.size()];
                    if(before.ns!=first_start.ns)append(before); }
            }else ++later_windows;
        }
        if(first_open)append(r);
        if(r.submitted){current.end=r;current.end_valid=true;}
        if(current.duration_ns>longest.duration_ns)longest=current;
        if(r.submitted){
            if(first_open){first_open=false;first_complete=true;}
            last_success=r;in_gap=false;
        }
        remember(r);
    }
    bool first_started=false;
    Row first_start{},terminal_row{};
    bool terminal_valid=false;
    std::uint64_t invalid_clock=0;
    Boundary const& longest_gap() const noexcept {return longest;}
    bool complete() const noexcept {return (!first_started||first_complete)&&!dropped&&!later_windows&&!invalid_clock;}
    static void print_row(FILE* out,const char* scope,const char* edge,Row const& r) noexcept {
        fprintf(out,"{\"outage_row\":true,\"scope\":\"%s\",\"edge\":\"%s\",\"qpc_ns\":%lld,\"status\":\"%s\",\"submitted\":%s,\"layer_count\":%u,\"layer_kind\":\"%s\",\"actual_new_image_valid\":%s,\"actual_new_image\":%s,\"cache_reused_for_submission\":%s,\"receiver_valid\":%s,\"copy_count\":%u,\"acquired_serial_valid\":%s,\"acquired_serial\":%llu,\"ticket_valid\":%s",
            scope,edge,(long long)r.ns,r.status,r.submitted?"true":"false",r.layer_count,r.layer_kind.data(),r.new_image_valid?"true":"false",r.new_image?"true":"false",r.submitted&&r.new_image_valid&&!r.new_image?"true":"false",r.image.receiver_valid?"true":"false",r.image.copies,r.image.receiver_valid&&r.image.acquired_serial?"true":"false",(unsigned long long)r.image.acquired_serial,r.image.ticket_valid?"true":"false");
        if(r.image.ticket_valid){
            fprintf(out,",\"ticket_id\":%llu,\"session\":%llu,\"sequence\":%llu,\"received_ns\":%lld,\"age_ns\":%lld,\"source\":%u,\"head_flags\":%llu,\"view_flags\":%llu",
                (unsigned long long)r.image.ticket_id,(unsigned long long)r.image.session,(unsigned long long)r.image.sequence,(long long)r.image.received_ns,(long long)(r.ns-r.image.received_ns),r.image.source,(unsigned long long)r.image.head_flags,(unsigned long long)r.image.view_flags);
        }
        const auto& m=r.menu;const auto& c=m.capture;
        fprintf(out,",\"menu_mode\":%u,\"session_state\":%d,\"shouldRender\":%s,\"panel_requested\":%s,\"menu_render_attempted\":%s,\"menu_render_result\":%s,\"menu_authorization_rejected\":%s,\"menu_snapshot\":{\"cached_only\":%s,\"render_attempted\":%s,\"reason\":\"%s\",\"update_attempted\":%s,\"updated\":%s,\"available_checked\":%s,\"available\":%s,\"current\":%s,\"fresh_checked\":%s,\"fresh\":%s,\"fresh_now_100ns\":%lld,\"generation_changed\":%s,\"capture_result\":\"%s\",\"capture_attempts\":%llu,\"capture_no_frames\":%llu,\"capture_accepted\":%llu,\"last_taken_100ns\":%lld,\"generation\":%llu,\"hwnd\":%llu,\"epoch_100ns\":%lld,\"accepted_100ns\":%lld}}\n",
            r.menu_mode,r.session_state,r.should_render?"true":"false",r.panel_requested?"true":"false",r.render_attempted?"true":"false",r.render_result?"true":"false",r.authorization_rejected?"true":"false",
            m.cached_only?"true":"false",m.render_attempted?"true":"false",menu_render_reason_name(m.reason),m.update_attempted?"true":"false",m.updated?"true":"false",m.available_checked?"true":"false",m.available?"true":"false",m.current?"true":"false",m.fresh_checked?"true":"false",m.fresh?"true":"false",(long long)m.fresh_now_100ns,m.generation_changed?"true":"false",capture_result_name(c.result),
            (unsigned long long)c.attempts,(unsigned long long)c.no_frames,(unsigned long long)c.accepted,(long long)c.last_taken_100ns,(unsigned long long)c.generation,(unsigned long long)c.hwnd,(long long)c.epoch_100ns,(long long)c.accepted_100ns);
    }
    void dump(FILE* out=stdout) noexcept {
        if(!enabled||dumped)return;dumped=true;
        fprintf(out,"{\"outage_diagnostics\":true,\"schema\":3,\"submission_semantics\":\"successful_nonempty_xrEndFrame_not_pixel_validation\",\"clock\":\"qpc_monotonic_ns\",\"timestamp_semantics\":\"post_xrEndFrame_observation\",\"age_semantics\":\"observation_time_minus_received_ns_not_exact_projection_gate_age\",\"threshold_ns\":100000000,\"first_window_started\":%s,\"first_window_recovered\":%s,\"first_window_rows\":%u,\"first_window_capacity\":249,\"total_item_capacity\":256,\"dropped_rows\":%llu,\"later_windows_not_recorded\":%llu,\"invalid_clock_rows\":%llu,\"complete\":%s,\"longest_gap_ns\":%lld,\"longest_gap_recovered\":%s,\"observed_submission\":%s}\n",
            first_started?"true":"false",first_complete?"true":"false",count,(unsigned long long)dropped,(unsigned long long)later_windows,(unsigned long long)invalid_clock,complete()?"true":"false",(long long)longest.duration_ns,longest.end_valid?"true":"false",have_success?"true":"false");
        if(first_started)print_row(out,"first_window","last_successful_submission",first_start);
        for(unsigned i=0;i<count;++i)print_row(out,"first_window","frame",rows[i]);
        if(terminal_valid)print_row(out,"termination_excluded_from_gap","terminal",terminal_row);
        if(longest.duration_ns){
            print_row(out,"longest_gap","last_successful_submission",longest.start);
            if(longest.first_missing_valid)print_row(out,"longest_gap","first_missing",longest.first_missing);
            if(longest.threshold_valid)print_row(out,"longest_gap","threshold_crossing",longest.threshold);
            if(longest.last_missing_valid)print_row(out,"longest_gap","last_missing",longest.last_missing);
            if(longest.end_valid)print_row(out,"longest_gap","recovery",longest.end);
        }
    }
};
}
