#pragma once
#include <array>
#include <cstdint>
#include <cstdio>

// Render-thread observations only. Fixed storage, no IO until dump(). These
// timestamps describe post-xrEndFrame observations, not GPU/publisher causality.
// age_ns is observation age, NOT the earlier render_projection gate age.
namespace outage_diagnostics {
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
        fprintf(out,"}\n");
    }
    void dump(FILE* out=stdout) noexcept {
        if(!enabled||dumped)return;dumped=true;
        fprintf(out,"{\"outage_diagnostics\":true,\"schema\":2,\"submission_semantics\":\"successful_nonempty_xrEndFrame_not_pixel_validation\",\"clock\":\"qpc_monotonic_ns\",\"timestamp_semantics\":\"post_xrEndFrame_observation\",\"age_semantics\":\"observation_time_minus_received_ns_not_exact_projection_gate_age\",\"threshold_ns\":100000000,\"first_window_started\":%s,\"first_window_recovered\":%s,\"first_window_rows\":%u,\"first_window_capacity\":249,\"total_item_capacity\":256,\"dropped_rows\":%llu,\"later_windows_not_recorded\":%llu,\"invalid_clock_rows\":%llu,\"complete\":%s,\"longest_gap_ns\":%lld,\"longest_gap_recovered\":%s,\"observed_submission\":%s}\n",
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
