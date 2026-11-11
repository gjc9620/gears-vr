#pragma once
#include "stereo_bridge.h"
#include "controller_input_bridge.h"
#include "controller_continuity_bridge.h"

// One GT owns these READ-ONLY mapping/process handles. Their lifetime is
// independent of Scene/cover readers and diagnostic stereo_mutex ownership.
// No producer, cached tracking fallback, blocking wait or timestamp renewal.
namespace gears_current_tracking {
enum class Failure : unsigned { none=0, stereo_read=4, stereo_geometry=5, stereo_active=6,
    stereo_focus=7, stereo_tracking=8, controller_read=10, controller_publisher=11,
    continuity_read=13, continuity_publisher=14, open=16, wrong_owner=17, cohort=18, clock=19,
    stereo_busy=20,controller_busy=21,continuity_busy=22 };
// Retry only a proven WAIT_TIMEOUT on a zero-timeout transport lock, before
// any cohort is accepted. Each attempt rereads the complete bundle and keeps
// the caller's original acquisition deadline. Invalid data is never retried.
template<class Read>
inline Failure bounded_transport_read(Read read) noexcept {
    for(unsigned attempt=0;attempt<3;++attempt){
        bool busy=false;const auto result=read(busy);
        if(result==Failure::none||!busy)return result;
        if(attempt==2){
            switch(result){case Failure::stereo_read:return Failure::stereo_busy;
            case Failure::controller_read:return Failure::controller_busy;
            case Failure::continuity_read:return Failure::continuity_busy;
            default:return result;}
        }
    }
    return Failure::cohort;
}
class Reader {
    gears_vr::StereoBridge stereo_;
    gears_vr::ControllerInputBridge controllers_;
    gears_hand_continuity::InputBridge continuity_;
    DWORD pid_{},thread_{};bool open_{};
    void close_handles() noexcept {continuity_.close();controllers_.close();stereo_.close();open_=false;}
public:
    gears_hand_continuity::MotionReadObservation motion_observation() const noexcept {return continuity_.motion_observation();}
    Reader()=default;Reader(Reader const&)=delete;Reader& operator=(Reader const&)=delete;
    template<class Clock>
    Failure read_matching(DWORD pid,Clock clock,bool allow_sim,gears_vr::StereoSample const& expected_stereo,
        gears_vr::ControllerInputFrame const& expected_input,gears_hand_continuity::TrackedInput& continuity,
        gears_hand_continuity::Publisher& publisher,
        gears_hand_continuity::TrackedInput* right_press=nullptr,
        gears_hand_continuity::MotionHistory* right_motion=nullptr) noexcept {
        continuity={};publisher={};if(right_press)*right_press={};if(right_motion)*right_motion={};const auto now=clock();
        if(!pid||pid!=GetCurrentProcessId()||now<=0||(pid_&&pid_!=pid)||
           (thread_&&thread_!=GetCurrentThreadId()))return Failure::wrong_owner;
        pid_=pid;thread_=GetCurrentThreadId();
        if(!open_){
            if(!stereo_.open(pid,false)||!controllers_.open(pid,false)||!continuity_.open(pid,false)){
                close_handles();return Failure::open;}
            open_=true;
        }
        const auto fail=[&](Failure reason){continuity={};publisher={};if(right_press)*right_press={};if(right_motion)*right_motion={};return reason;};
        if(!gears_vr::stereo_geometry_valid(expected_stereo))return fail(Failure::stereo_geometry);
        if(!gears_vr::stereo_render_active(expected_stereo))return fail(Failure::stereo_active);
        if(!gears_vr::stereo_tracking_valid(expected_stereo,allow_sim))return fail(Failure::stereo_tracking);
        // Match the exact caller packet on every attempt. Like read(), retry
        // only proven zero-timeout lock contention, without a wait, newer
        // source substitution or clock/deadline renewal. Invalid data fails.
        return bounded_transport_read([&](bool& busy) noexcept -> Failure {
        std::uint64_t before_pid{},before_start{},after_pid{},after_start{};
        gears_vr::ControllerInputFrame actual{};
        if(!controllers_.publisher_identity(before_pid,before_start)){
            close_handles();return fail(Failure::controller_publisher);}
        if(!controllers_.read_matching(expected_stereo,now,actual,allow_sim,&busy))return fail(Failure::controller_read);
        if(!gears_hand_continuity::same_input(actual,expected_input))return fail(Failure::cohort);
        if(!continuity_.read_matching(expected_stereo,now,continuity,publisher,allow_sim,right_press,&busy,right_motion)){
            if(!busy)close_handles();return fail(Failure::continuity_read);}
        if(!controllers_.publisher_identity(after_pid,after_start)||before_pid!=after_pid||before_start!=after_start||
           publisher.pid!=before_pid||publisher.start!=before_start){
            close_handles();return fail(Failure::continuity_publisher);}
        if(!gears_hand_continuity::same_input(continuity.input,actual))return fail(Failure::cohort);
        // Match the already-authenticated caller's exact cohort. Do not replace
        // its source with the latest packet or grant authority from this POD.
        return Failure::none;
        });
    }
    template<class Clock>
    Failure read(DWORD pid,Clock clock,bool allow_sim,gears_vr::StereoSample& stereo,
        gears_vr::ControllerInputFrame& controllers,gears_hand_continuity::TrackedInput& continuity,
        gears_hand_continuity::Publisher& publisher,
        gears_hand_continuity::MotionHistory* right_motion=nullptr) noexcept {
        stereo={};controllers={};continuity={};publisher={};if(right_motion)*right_motion={};
        const auto begin=clock();
        if(!pid||pid!=GetCurrentProcessId()||begin<=0||(pid_&&pid_!=pid)||
           (thread_&&thread_!=GetCurrentThreadId()))return Failure::wrong_owner;
        pid_=pid;thread_=GetCurrentThreadId();
        if(!open_){
            // Open only existing authenticated publishers for this exact game.
            if(!stereo_.open(pid,false)||!controllers_.open(pid,false)||!continuity_.open(pid,false)){
                close_handles();return Failure::open;
            }
            open_=true;
        }
        const auto fail=[&](Failure reason){stereo={};controllers={};continuity={};publisher={};if(right_motion)*right_motion={};return reason;};
        return bounded_transport_read([&](bool& busy) noexcept -> Failure {
        std::uint64_t producer_pid{},producer_start{};
        if(!controllers_.publisher_identity(producer_pid,producer_start)){
            close_handles();return fail(Failure::controller_publisher);
        }
        if(!stereo_.read(stereo,&busy))return fail(Failure::stereo_read);
        if(!gears_vr::stereo_geometry_valid(stereo))return fail(Failure::stereo_geometry);
        if(!gears_vr::stereo_render_active(stereo))return fail(Failure::stereo_active);
        if(stereo.focused!=1)return fail(Failure::stereo_focus);
        if(!gears_vr::stereo_tracking_valid(stereo,allow_sim))return fail(Failure::stereo_tracking);
        // The publisher may have completed a NEW sample during the reads.
        // Compare it with the actual post-read clock, not an earlier time at
        // which that new sample did not yet exist. Publisher times stay intact;
        // the caller still applies its40ms bound at the end of acquisition.
        const auto now=clock();if(now<begin)return fail(Failure::clock);
        if(!controllers_.read_matching(stereo,now,controllers,allow_sim,&busy))return fail(Failure::controller_read);
        if(!continuity_.read_matching(stereo,now,continuity,publisher,allow_sim,nullptr,&busy,right_motion)){
            // A replaced/invalid publisher requires reopening next Tick.
            // Pure lock contention retains these same authenticated handles.
            if(!busy)close_handles();return fail(Failure::continuity_read);
        }
        std::uint64_t after_pid{},after_start{};
        if(!controllers_.publisher_identity(after_pid,after_start)||producer_pid!=after_pid||producer_start!=after_start||
           publisher.pid!=producer_pid||publisher.start!=producer_start){
            close_handles();return fail(Failure::continuity_publisher);
        }
        if(!gears_hand_continuity::same_input(controllers,continuity.input))return fail(Failure::cohort);
        return Failure::none;
        });
    }
    // Caller must first stop admission and drain all owned callbacks/Ticks.
    // No concurrent read/open/close is permitted, even during shutdown.
    void close_after_drain() noexcept {close_handles();pid_=thread_=0;}
};
}
