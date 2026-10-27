#pragma once
#include "full_body_c0_planner.h"
#include "body_c0_arm_alpha_policy.h"
#include "../vr-common/weapon_hand_participant.h"
#include <atomic>
#include <cstdint>
#include <type_traits>

// Owned POD observations only. Never a source, writer permit or solver input.
namespace gears_ready_wrist_entry {
namespace body=gears_full_body_c0;
using U=std::uint64_t;
constexpr U unknown=UINT64_MAX;
constexpr unsigned capacity=64;
constexpr unsigned bones[3]={35,51,52};
enum Valid:U {source=1,capture=2,target=4,planned=8,basis=16,original_result=32,normal_scope_close=64};
enum class ReadStatus:U {okay=1,state=2,metadata=3,storage=4,local_read=5,space_read=6,
    local_changed=7,space_changed=8,metadata_changed=9,exception=10};
struct Key {
    U entry{},pid{},game_start{},native_started{},thread{},tick{},compose{},mesh{},cookie{},
      publication{},scene{},input_session{},stereo_sequence{},controller_sequence{},space_epoch{},
      profile_generation{},input_epoch{},reference_revision{},publisher_pid{},publisher_start{},
      stereo_received_ns{},controllers_received_ns{},sync_ns{},tracking_begin_ns{},tracking_end_ns{},capture_ns{};
    U input_source{unknown},grip_flags{unknown},grip_active{unknown},begin_qpc{};
};
struct Stage {
    U status{unknown},begin_qpc{},end_qpc{},pose_epoch{unknown};
    body::Atom local[3]{};
    body::Matrix64 space35{},space52{},component{};
    // Local is parent-local. Space/CTW are available at returned_end and raw_return.
    U space_valid{};
};
constexpr unsigned alpha_capacity=64,weight_capacity=8;
struct WeightBoundary {
    gears_body_c0_arm_alpha::WeightReturn input{};Stage snapshot{};
    U constant_read_mask{},full_threshold_bits{unknown},zero_threshold_bits{unknown};
};
// Independent passive samples: no Context, source clock or writer permit.
enum class OuterStatus:U {okay=1,scope=2,identity=3,layout=4,attachment=5,
    metadata=6,reference=7,payload=8,changed=9,exception=10,unprepared=11};
struct ComponentStage {
    U status{unknown},begin_qpc{},end_qpc{};
    gears_weapon_hand::ObjectIdentity weapon{},component{},asset{};
    body::Matrix64 matrix{};
    U attachment_data{},attachment_count{},attachment_capacity{};
    unsigned char attachment[52]{};
    U gun_pose_epoch{},proof_generation{};
};
struct OuterTick {
    U status{unknown},entry{},tick{},thread{},mesh{},original_called{},original_returned{},
      last_compose_matches{},begin_qpc{},end_qpc{},tick_epoch{},flag318{},flag32c{},flag350{},layout_step{};
    Stage body{},body_close{};ComponentStage gun{};
};
struct Record {
    U sequence{},valid{},selection{unknown};Key key{};
    U native_begin{unknown},capture_reason{unknown},capture_context_verdict{unknown},
      capture_epoch{unknown},avatar_asset{unknown},body_plan_status{unknown},hands_status{unknown},
      actual_mount{unknown},prepare_code{unknown},prepare_unit{unknown},commit_code{unknown},commit_unit{unknown},
      finish_code{unknown},original_called{unknown},original_returned{unknown};
    gears_vr::Pose raw_head{},raw_right{},head_relative_right{};
    double selector_position_error{},selector_angle_error{},squeeze{},trigger{},units_per_metre{};
    gears_vr::Pose xr_reference{};
    U native_reference_epoch{unknown},native_reference_flags{unknown};
    body::Pose target_wrist_world{};
    body::Matrix64 base_view{},capture_component{};
    body::Atom before[3]{},planned_local[3]{};
    Stage commit{},consumer_pre{},returned_end{};
    // Append-only v2: exact existing commit-stage Context source-age return.
    U commit_context_verdict{unknown},commit_context_causes{unknown};
    // Append-only v3: raw original returned, before all finally/end callbacks.
    Stage raw_return{};
    // Append-only v4: original POD events and final actual weight boundary.
    U alpha_count{},alpha_dropped{},weight_count{},weight_dropped{};
    gears_body_c0_arm_alpha::Diagnostic alpha[alpha_capacity]{};
    WeightBoundary weight[weight_capacity]{};
    // Append-only v5: normal compose-close gun and exact outer MeshTick end.
    ComponentStage compose_gun{};OuterTick outer{};
};
struct Header {
    char magic[8]{'R','W','E','N','T','0','0','5'};
    U version{5},header_bytes{sizeof(Header)},record_bytes{sizeof(Record)},count{},limit{capacity},
      pid{},native_started{},qpc_frequency{},selected{},dropped_capacity{},nested{},unmatched_end{},
      nonmatching_input{},unavailable_input{},window_cookie{};
};
static_assert(sizeof(body::Atom)==32&&sizeof(body::Matrix64)==64&&sizeof(body::Pose)==56);
static_assert(std::is_standard_layout_v<Record>&&std::is_trivially_copyable_v<Record>);
inline bool ready_pose(gears_vr::Pose const& head,gears_vr::Pose const& right,
                       gears_vr::Pose& relative,double& position_error,double& angle_error,bool* known=nullptr) noexcept {
    if(known)*known=false;
    try {
        if(!gears_vr::finite(head.position)||!gears_vr::finite(right.position))return false;
        const auto inverse=gears_vr::inverse(head.orientation);
        relative.position=gears_vr::transform(gears_vr::rotation_matrix(inverse),
            {right.position.x-head.position.x,right.position.y-head.position.y,right.position.z-head.position.z});
        relative.orientation=gears_vr::normalized(gears_vr::multiply(inverse,gears_vr::normalized(right.orientation)));
        const gears_vr::Quaternion expected{-std::sin(.15),0,0,std::cos(.15)};
        const auto& q=relative.orientation;
        const auto dot=std::abs(q.x*expected.x+q.y*expected.y+q.z*expected.z+q.w*expected.w);
        angle_error=2*std::acos((std::min)(1.0,dot));
        position_error=std::hypot(relative.position.x-.18,relative.position.y+.30,relative.position.z);
        if(known)*known=true;return position_error<=.001&&angle_error<=.001;
    } catch(...) {return false;}
}
struct Recorder {
    bool enabled{};std::atomic<bool> active{};std::atomic<unsigned> saved{};
    U selected{},dropped_capacity{},nested{},unmatched_end{},nonmatching_input{},unavailable_input{};
    bool window{},exhausted{};Key window_key{};U window_cookie{};
    Record* current{};std::array<Record,capacity> rows{};
    bool exact(U entry,U compose,U thread) const noexcept {
        return enabled&&active.load(std::memory_order_acquire)&&current&&current->key.entry==entry&&
            current->key.compose==compose&&current->key.thread==thread;
    }
    bool begin(Key const& key,gears_vr::Pose const& head,gears_vr::Pose const& right,
               double squeeze,double trigger,bool copied_fixture_input_valid,bool update_existing=false) noexcept {
        if(!enabled||exhausted)return false;
        const bool updating=active.load(std::memory_order_acquire);
        if(updating&&(!update_existing||!exact(key.entry,key.compose,key.thread))){++nested;return false;}
        if(!key.entry||!key.tick||!key.compose||!key.thread){++unavailable_input;return false;}
        const bool changed=window&&(key.pid!=window_key.pid||key.game_start!=window_key.game_start||key.native_started!=window_key.native_started||
           key.mesh!=window_key.mesh||!key.cookie||key.cookie!=window_cookie||
           (key.input_source!=unknown&&key.input_source!=window_key.input_source));
        if(changed)window=false;
        gears_vr::Pose relative{};double position{},angle{};bool known=false;
        const bool match=copied_fixture_input_valid&&key.cookie&&ready_pose(head,right,relative,position,angle,&known);
        if(updating){
            if(changed){current->selection=4;return true;}
            if(!known)return true;
            if(current->selection==2&&unavailable_input)--unavailable_input;
            current->selection=match?1u:3u;
            if(!match){++nonmatching_input;window=false;}
            const auto began=current->key.begin_qpc;current->key=key;current->key.begin_qpc=began;
            current->valid|=Valid::source;current->raw_head=head;current->raw_right=right;current->head_relative_right=relative;
            current->selector_position_error=position;current->selector_angle_error=angle;current->squeeze=squeeze;current->trigger=trigger;
            return true;
        }
        U selection=unknown;
        if(match){selection=1;if(!window){window=true;window_key=key;window_cookie=key.cookie;}}
        else if(known){++nonmatching_input;if(!window)return false;selection=3;window=false;}
        else{++unavailable_input;if(!window)return false;selection=2;}
        const auto seq=++selected;const auto index=saved.load(std::memory_order_relaxed);
        if(index>=capacity){++dropped_capacity;window=false;exhausted=true;return false;}
        auto& row=rows[index];row={};row.sequence=seq;row.key=key;row.selection=selection;
        if(known){row.valid=Valid::source;row.raw_head=head;row.raw_right=right;row.head_relative_right=relative;
            row.selector_position_error=position;row.selector_angle_error=angle;row.squeeze=squeeze;row.trigger=trigger;}
        current=&row;active.store(true,std::memory_order_release);return true;
    }
    void note_commit_source(U entry,U compose,U thread,U verdict,U causes) noexcept {
        if(!exact(entry,compose,thread)||current->commit_context_verdict!=unknown)return;
        current->commit_context_verdict=verdict;current->commit_context_causes=causes;
    }
    void finish(U entry,U compose,U thread,bool called,bool returned) noexcept {
        if(!enabled||!active.load(std::memory_order_acquire))return;
        if(!exact(entry,compose,thread)){++unmatched_end;return;}
        current->original_called=U(called);current->original_returned=U(returned);current->valid|=Valid::original_result;
        saved.fetch_add(1,std::memory_order_release);current=nullptr;active.store(false,std::memory_order_release);
    }
};
} // namespace gears_ready_wrist_entry
