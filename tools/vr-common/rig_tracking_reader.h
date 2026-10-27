#pragma once
#include "rig_tracking_frame.h"

// Read-only client of the shared prepared-render snapshot. The caller keeps
// the exact Native module and registration alive throughout this call. This
// helper does not register, own a callback, manufacture a GHW1 certificate or
// grant a bone setter/restore/normal-frame scope.
namespace gears_rig_tracking {
using Clock=std::int64_t(*)() noexcept;
enum class ReadFailure:std::uint32_t {none,binding,provider,envelope,producer};
struct ReaderBinding {
    NativeKey native{};
    std::uint32_t expected_source{};
    Read read{};
    Clock clock_ns{};
};
struct ReadResult {
    Reply reply{Reply::unavailable};
    ReadFailure failure{ReadFailure::binding};
    std::int64_t observed_ns{};
};
// minimum_sequence is the Native Scene publication cursor. It is not the XR
// input cursor: a newer Scene may legitimately contain the same raw sample.
inline ReadResult read_frame(ReaderBinding const& binding,std::uint64_t minimum_sequence,
                             Frame& out,std::uint64_t maximum_age=maximum_age_ns,
                             bool require_producer=true) noexcept {
    out={};
    ReadResult result{};
    if(!binding.read||!binding.clock_ns||binding.expected_source>1||
       !binding.native.game_pid||!binding.native.game_thread||!binding.native.game_start||
       !binding.native.native_started_qpc||!binding.native.registration_cookie||
       !maximum_age||maximum_age>maximum_age_ns){result.reply=Reply::invalid;return result;}
    ReadRequest request{};
    request.registration_cookie=binding.native.registration_cookie;
    request.native_started_qpc=binding.native.native_started_qpc;
    request.minimum_publication_sequence=minimum_sequence;
    request.maximum_source_age_ns=maximum_age;
    Frame value{};
    result.reply=binding.read(&request,&value);
    if(result.reply!=Reply::accepted){result.failure=ReadFailure::provider;return result;}
    // Sample after the provider copy. A pre-call time could precede a valid
    // concurrent publication. Never replace either original source timestamp.
    result.observed_ns=binding.clock_ns();
    if(!same(value.native,binding.native)||value.publication_sequence<=minimum_sequence||
       static_cast<std::uint32_t>(value.stereo.source)!=binding.expected_source||
       !frame_valid(value,result.observed_ns,maximum_age,binding.expected_source==1)){
        result.reply=Reply::invalid;result.failure=ReadFailure::envelope;return result;
    }
    if((value.flags&controllers_matched)&&
       (result.observed_ns<value.controllers.received_ns||
        std::uint64_t(result.observed_ns-value.controllers.received_ns)>maximum_age)){
        result.reply=Reply::invalid;result.failure=ReadFailure::envelope;return result;
    }
    if(require_producer&&
       (value.flags&(controllers_matched|producer_continuity_available))!=
           (controllers_matched|producer_continuity_available)){
        result.reply=Reply::unavailable;result.failure=ReadFailure::producer;return result;
    }
    out=value;
    result.failure=ReadFailure::none;
    return result;
}
} // namespace gears_rig_tracking
