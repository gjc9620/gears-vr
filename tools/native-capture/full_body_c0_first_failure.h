#pragma once
#include "full_body_c0_transaction.h"
#include "cover_pose_phase.h"

// Private immutable first-failure input, not a public wire or authorization.
namespace gears_full_body_c0_first_failure {
namespace body=gears_full_body_c0;namespace tx=gears_full_body_c0_transaction;
enum class Stage:std::uint32_t {gate,plan,prepare,commit,exception,hands};
enum class Gate:std::uint32_t {okay,disabled,stopping,thread,capture,prepared,notified,entry,source,context,receipt,count};
constexpr unsigned statuses=unsigned(body::Status::pelvis_iterations)+1,codes=unsigned(tx::Code::bad_receipt)+1;
struct Timing {std::uint64_t begin{},gate{},plan{},prepare{},commit{},end{},frequency{};};
inline std::int64_t ns(std::uint64_t qpc,std::uint64_t frequency)noexcept {
    constexpr std::uint64_t billion=1000000000,limit=INT64_MAX;if(!frequency||!qpc)return 0;
    const auto whole=qpc/frequency,remainder=qpc%frequency;if(whole>limit/billion||remainder>limit/billion)return 0;
    const auto first=whole*billion,last=remainder*billion/frequency;return first>limit-last?0:std::int64_t(first+last);
}
inline std::int64_t age(std::uint64_t stamp,std::int64_t now)noexcept{return !stamp||stamp>INT64_MAX||now<=0||stamp>std::uint64_t(now)?-1:now-std::int64_t(stamp);}
struct Header {
    char magic[8]{'C','0','F','A','I','L','0','1'};
    std::uint32_t version{1},bytes{},header_bytes{},frame_bytes{},goal_bytes{},report_bytes{},output_bytes{},desired_bytes{};
    std::uint32_t stage{},detail{},unit{UINT_MAX},valid_mask{},thread{},kernel_pending{},kernel_fields{},kernel_protocol{},kernel_flags{};
    std::uint64_t session{},tick{},compose{},qpc{},ra{},rcx{},rdx{},r8{},registry_version{};
    Timing timing{};std::uint64_t core_published_ns{},stereo_received_ns{},controllers_received_ns{};
    std::int64_t core_age_begin{-1},core_age_end{-1},stereo_age_begin{-1},stereo_age_end{-1},controllers_age_begin{-1},controllers_age_end{-1};
};
struct Record {Header header{};tx::Stamp stamp{};gears_cover_pose_phase::Metadata metadata{};
    body::Frame source{};body::Goal goal{};body::Report report{};body::Output output{};tx::Desired desired{};};
static_assert(sizeof(body::Goal)==296&&sizeof(Record)<32768);
static_assert(std::is_trivially_copyable<Record>::value);
struct Counters {std::atomic<std::uint64_t> calls{},exceptions{};
    std::array<std::atomic<std::uint64_t>,unsigned(Gate::count)> gates{};
    std::array<std::atomic<std::uint64_t>,statuses> plans{};
    std::array<std::atomic<std::uint64_t>,codes> prepares{},commits{};};
class First {
    std::atomic<unsigned> state_{};Record record_{};
public:
    // The registered GT owns the source buffers. This routine copies only
    // owned memory once, without allocation, locks, file I/O or engine reads.
    bool save(Header header,tx::Stamp const&stamp,gears_cover_pose_phase::Metadata const&metadata,
              body::Frame const&source,body::Goal const&goal,body::Report const*report,
              body::Output const*output,tx::Desired const&desired)noexcept {
        unsigned empty{};if(!state_.compare_exchange_strong(empty,1))return false;
        header.bytes=sizeof(Record);header.header_bytes=sizeof(Header);header.frame_bytes=sizeof(body::Frame);
        header.goal_bytes=sizeof(body::Goal);header.report_bytes=sizeof(body::Report);header.output_bytes=sizeof(body::Output);header.desired_bytes=sizeof(tx::Desired);
        header.valid_mask=1|2|16|32;if(report)header.valid_mask|=4;if(output)header.valid_mask|=8;
        record_.header=header;record_.stamp=stamp;record_.metadata=metadata;record_.source=source;record_.goal=goal;record_.desired=desired;
        if(report)record_.report=*report;if(output)record_.output=*output;
        state_.store(2,std::memory_order_release);return true;
    }
    unsigned state()const noexcept{return state_.load(std::memory_order_acquire);}
    Record const* published()const noexcept{return state()==2?&record_:nullptr;}
};
}
