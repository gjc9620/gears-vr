#pragma once
#include "outage_diagnostics.h"
#include <array>
#include <memory>
#include <new>

// Same render thread as the snapshot producer. Store values only: no shared
// memory reads, XR calls, GPU waits, or IO until dump() after the frame loop.
namespace image_ticket_diagnostics {
using Row=outage_diagnostics::Row;
class Recorder {
public:
    static constexpr unsigned capacity=4096;
private:
    using Storage=std::array<Row,capacity>;
    std::unique_ptr<Storage> rows_;
    unsigned count_{};
    std::uint64_t observed_{},total_{},dropped_{},submitted_{},invalid_submitted_{};
    std::uint64_t last_session_{},last_ticket_{};
    bool have_submission_{},dumped_{};
public:
    // No large stack allocation, including when diagnostics is disabled.
    bool enable() noexcept {
        if(!rows_)rows_.reset(new(std::nothrow) Storage{});
        return bool(rows_);
    }
    bool enabled() const noexcept {return bool(rows_);}
    unsigned count() const noexcept {return count_;}
    std::uint64_t total() const noexcept {return total_;}
    std::uint64_t dropped() const noexcept {return dropped_;}
    const Row* row(unsigned index) const noexcept {return index<count_?&(*rows_)[index]:nullptr;}
    void observe(Row const& r) noexcept {
        if(!rows_||dumped_)return;
        ++observed_;
        const bool fresh=r.new_image_valid&&r.new_image;
        const bool new_submission=r.submitted&&(!r.image.ticket_valid||!have_submission_||
            r.image.session!=last_session_||r.image.ticket_id!=last_ticket_);
        // A copy can precede its first successful cached submission. Updating
        // this key only on submission preserves that later receipt.
        if(r.submitted){
            ++submitted_;
            if(!r.image.ticket_valid)++invalid_submitted_;
            have_submission_=r.image.ticket_valid;
            last_session_=r.image.session;last_ticket_=r.image.ticket_id;
        }
        if(!fresh&&!new_submission)return;
        ++total_;
        if(count_<capacity)(*rows_)[count_++]=r;else ++dropped_;
    }
    static void print_row(FILE* out,Row const& r) noexcept {
        fprintf(out,"{\"image_ticket_row\":true,\"qpc_ns\":%lld,\"status\":\"%s\",\"submitted\":%s,\"actual_new_image_valid\":%s,\"actual_new_image\":%s,\"receiver_valid\":%s,\"ticket_valid\":%s,\"ticket_id\":%llu,\"session\":%llu,\"sequence\":%llu,\"received_ns\":%lld,\"source\":%u,\"head_flags\":%llu,\"view_flags\":%llu,\"acquired_serial\":%llu,\"copy_count\":%u,\"head_position\":[%.17g,%.17g,%.17g],\"head_orientation\":[%.17g,%.17g,%.17g,%.17g]}\n",
            static_cast<long long>(r.ns),r.status,r.submitted?"true":"false",
            r.new_image_valid?"true":"false",r.new_image?"true":"false",
            r.image.receiver_valid?"true":"false",r.image.ticket_valid?"true":"false",
            static_cast<unsigned long long>(r.image.ticket_id),static_cast<unsigned long long>(r.image.session),
            static_cast<unsigned long long>(r.image.sequence),static_cast<long long>(r.image.received_ns),r.image.source,
            static_cast<unsigned long long>(r.image.head_flags),static_cast<unsigned long long>(r.image.view_flags),
            static_cast<unsigned long long>(r.image.acquired_serial),r.image.copies,
            r.image.head_position[0],r.image.head_position[1],r.image.head_position[2],
            r.image.head_orientation[0],r.image.head_orientation[1],r.image.head_orientation[2],r.image.head_orientation[3]);
    }
    void dump(FILE* out=stdout) noexcept {
        if(!rows_||dumped_)return;dumped_=true;
        fprintf(out,"{\"image_ticket_diagnostics\":true,\"schema\":2,\"clock\":\"qpc_monotonic_ns\",\"timestamp_semantics\":\"post_xrEndFrame_observation\",\"pose_semantics\":\"frozen_native_image_ticket_pose_not_current_runtime_pose\",\"submission_semantics\":\"projection_layer_in_successful_xrEndFrame_not_display_confirmation\",\"acquired_serial_semantics\":\"acknowledged_producer_serial_not_gpu_fence\",\"selection\":\"actual_new_image_or_successful_submission_with_changed_session_ticket\",\"capacity\":%u,\"observed_rows\":%llu,\"total_rows\":%llu,\"retained_rows\":%u,\"dropped_rows\":%llu,\"submitted_frames\":%llu,\"invalid_submitted_ticket_rows\":%llu,\"retention_complete\":%s}\n",
            capacity,static_cast<unsigned long long>(observed_),static_cast<unsigned long long>(total_),count_,
            static_cast<unsigned long long>(dropped_),static_cast<unsigned long long>(submitted_),
            static_cast<unsigned long long>(invalid_submitted_),dropped_?"false":"true");
        for(unsigned i=0;i<count_;++i)print_row(out,(*rows_)[i]);
        fflush(out);
    }
};
static_assert(sizeof(Recorder)<256,"Diagnostic rows must remain on the heap");
}
