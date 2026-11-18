#include "../vr-common/weapon_boltok_grasp_pose.h"
#include "../vr-common/weapon_boltok_owner_profiles.h"
#include "../vr-common/weapon_boltok_mechanism_geometry.h"
#include "../vr-common/weapon_boltok_motion_history.h"
#include "../vr-common/weapon_boltok_completed_grasp.h"
#include "../vr-common/weapon_boltok_feed_motion.h"
#include "../vr-common/weapon_boltok_perfect_window.h"
#include "../vr-common/weapon_boltok_insertion_window.h"
#include "../vr-common/weapon_boltok_proposal_projection.h"
#include "../vr-common/weapon_boltok_cylinder_insertion.h"
#include "../vr-common/weapon_boltok_group_mask.h"
#include "../vr-common/weapon_boltok_grip_close.h"
#include "../vr-common/weapon_boltok_ammo_avatar_profile.h"
#include "../vr-common/weapon_boltok_loading_snap.h"
#include "../vr-common/weapon_boltok_close_release.h"
// Single GT physical Boltok mechanism consumer. Ammo insertion/render resources
// join this owner; Native remains the only original-bone writer.
namespace boltok_runtime {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace api=gears_boltok_original;
using P=gears_vr::Pose;using V=gears_vr::Vector;
enum class Stage {idle,acquiring,opening,opened,closing,completing,recovering};
Stage stage{};std::atomic<bool> retained{},stop_requested{};
api::Control control{};api::Receipt native_receipt{};
api::Result last_update_result{api::Result::rejected};unsigned recovery_rows{};
unsigned read_rows{},last_read_code{~0u},last_read_difference{~0u};
inv::Identity identity{},proposal_identity{};
hand::Lease held{};std::uint64_t last_intent{},accepted_intent{},proposal_reference{};
bool proposal_known{},moving{},closing{},switch_requested{};
// An open endpoint ACK ends motion, not the still-held mechanism grasp. This
// stores only the original exact lease and identity; it grants no pose/action.
hand::Lease completed_lease{};
gears_boltok_completed_grasp::Identity completed_identity{};
bool completed_known{};
void clear_completed_grasp() noexcept {completed_known=false;completed_lease={};completed_identity={};}
// Metadata only: the exact admitted press, never a retained Frame pose or
// source timestamp. Current authority is reread by pre_tick/tick.
struct PendingStart {
    inv::Identity identity{};
    hand::FrameKey key{};
    hand::Lease lease{};
    std::uint64_t press{},sequence{},frame{},reference{};
    bool known{};
};
PendingStart pending_start{};
void clear_pending_start() noexcept {pending_start={};}
bool same_pending_domain(hand::FrameKey const& a,hand::FrameKey const& b) noexcept {
    // The admitted lease survives newer publications in the same domain.
    // Frame/sequence monotonicity is checked separately; no old input is used.
    return a.game_start==b.game_start&&a.native_started_qpc==b.native_started_qpc&&
        a.registration_cookie==b.registration_cookie&&a.lifecycle==b.lifecycle&&
        a.world_generation==b.world_generation&&a.game_pid==b.game_pid&&a.thread_id==b.thread_id;
}
gears_boltok_proposal_projection::Hint proposal_projection;
gears_vr::Quaternion closed_support{};
// Captured mechanism socket for this exact bound hold. Calibration data only:
// no Frame, input timestamp, lease or writer permission survives here.
struct CapturedMechanism {
    gears_boltok_mechanism_geometry::Output geometry{};
    inv::Identity identity{};
    P canonical_root{};std::array<unsigned char,68> root_reference{};
    hand::ObjectIdentity weapon{},component{},asset{};
    std::uint64_t hold_generation{},reference{},world_generation{},session{},space_epoch{},input_epoch{};
    double component_scale{},units{};
    bool known{};
};
CapturedMechanism captured{};
void clear_captured() noexcept {captured={};proposal_projection.clear();}
gears_weapon::HammerburstLatchGeometry hinge{};
gears_weapon::BoltokSwingStroke stroke;
gears_boltok_grip_close::Closure grip_close;
bool grip_closing{};
gears_vr::Pose cylinder_relative{};
inv::ObservedInventory inventory_observer;unsigned live_mask{};
inv::Transaction transaction;inv::InsertionWriter inventory_writer;inv::Inventory opening_inventory{};
inv::Request completion{};lancerreload_state::RewardReceipt reward{};
std::uint64_t reload_serial{},round_item{},round_sequence{};unsigned pending_group_mask{};
bool cycle{},mask_known{},native_exited{};std::int64_t perfect_ms{};
// Original native duration; the VR window starts at first feed-port alignment.
gears_boltok_perfect_window::ClosingWindow perfect_window;
gears_boltok_insertion_window::PendingClose auto_close;
unsigned preparation_gate{~0u},preparation_rows{},preparation_ready_rows{};
namespace motion {void clear() noexcept;}
namespace feed_motion {void reset() noexcept;}
namespace close_release {void reset() noexcept;}
// Local timing/progress only. Never a source or permission cache.
struct TickTiming {
    std::uint64_t frame{},sequence{},lifecycle{};std::int64_t began{},ended{},sync{};
    std::array<std::int64_t,9> ns{};unsigned mask{};
};
TickTiming last_tick_timing{};
struct TickCost {
    hand::Frame const& frame;weapon_part_assets::SourceCost before{weapon_part_assets::source_cost};
    TickTiming prior{last_tick_timing};
    std::int64_t began{weaponhand::clock_ns(nullptr)},last{began};std::array<std::int64_t,9> ns{};
    unsigned mask{},stage_before{unsigned(stage)};bool active_before{cycle||moving||stage!=Stage::idle},force{};
    explicit TickCost(hand::Frame const& f) noexcept:frame(f){}
    void mark(unsigned phase) noexcept {const auto now=weaponhand::clock_ns(nullptr);ns[phase]+=now-last;last=now;mask|=1u<<phase;}
    ~TickCost() noexcept {
        const auto end=weaponhand::clock_ns(nullptr);
        last_tick_timing={frame.key.frame,frame.controllers.sequence,frame.key.lifecycle,began,end,frame.controllers.sync_ns,ns,mask};
        if(!log_file||!(force||active_before||cycle||moving||stage!=Stage::idle)||(!force&&end-began<35000000))return;
        static unsigned slow_rows{},failed_rows{},late_rows{};static std::int64_t prior_log_ns{};
        const bool late=frame.controllers.sync_ns&&end-frame.controllers.sync_ns>=80000000;
        bool log_row{};
        if(force){if(failed_rows++<8)log_row=true;}
        else if(slow_rows++<8)log_row=true;
        if(!log_row&&late&&late_rows++<3)log_row=true;
        if(!log_row)return;
        const auto log_began=weaponhand::clock_ns(nullptr);
        fwprintf(log_file,L"boltok_tick_cost frame=%llu sequence=%llu force=%u stage_before=%u stage_after=%u phases=%u begin_age_ns=%lld end_age_ns=%lld elapsed_ns=%lld source_calls=%llu source_ns=%lld phase_ns=%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld tail_ns=%lld prior_log_ns=%lld sync_ns=%lld began_ns=%lld ended_ns=%lld prior_frame=%llu prior_sequence=%llu prior_lifecycle=%llu prior_sync_ns=%lld prior_began_ns=%lld prior_ended_ns=%lld prior_elapsed_ns=%lld between_ticks_ns=%lld prior_phases=%u prior_phase_ns=%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
            frame.key.frame,frame.controllers.sequence,unsigned(force),stage_before,unsigned(stage),mask,
            frame.controllers.sync_ns?began-frame.controllers.sync_ns:-1,frame.controllers.sync_ns?end-frame.controllers.sync_ns:-1,
            end-began,weapon_part_assets::source_cost.calls-before.calls,weapon_part_assets::source_cost.ns-before.ns,
            ns[0],ns[1],ns[2],ns[3],ns[4],ns[5],ns[6],ns[7],ns[8],end-last,prior_log_ns,
            frame.controllers.sync_ns,began,end,prior.frame,prior.sequence,prior.lifecycle,prior.sync,prior.began,prior.ended,
            prior.ended?prior.ended-prior.began:-1,prior.ended?began-prior.ended:-1,prior.mask,
            prior.ns[0],prior.ns[1],prior.ns[2],prior.ns[3],prior.ns[4],prior.ns[5],prior.ns[6],prior.ns[7],prior.ns[8]);fflush(log_file);
        prior_log_ns=weaponhand::clock_ns(nullptr)-log_began;
    }
};
// Bounded owner witness for diagnostic correlation only, never action authority.
void report_owner_witness(const wchar_t* phase,hand::Frame const& f) noexcept {
    const auto id=inv::identity(f);
    fwprintf(log_file,L"boltok_owner phase=%ls handle=%llu frame=%llu sequence=%llu reference=%llu game_start=%llu native_start=%llu registration=%llu lifecycle=%llu world_generation=%llu hold=%llu session=%llu space=%llu input=%llu",
        phase,control.handle,f.key.frame,f.controllers.sequence,f.reference_generation,id.game_start,id.native_start,
        id.registration,id.lifecycle,id.world_generation,id.hold_generation,id.session,id.space,id.input);
    const hand::ObjectIdentity objects[]{id.world,id.pawn,id.weapon};
    const wchar_t* labels[]{L"world",L"pawn",L"weapon"};
    for(unsigned i=0;i<3;++i){const auto& o=objects[i];
        fwprintf(log_file,L" %ls=%llx,%llx,%llx,%u,%u,%u,%u",labels[i],
            o.address,o.outer,o.klass,o.registry_index,o.name_index,o.name_number,o.known);
    }
    fwprintf(log_file,L"\n");fflush(log_file);
}
void report_preparation(unsigned gate,hand::Frame const* frame=nullptr) noexcept {
    if(gate==preparation_gate)return;preparation_gate=gate;
    // Eight resources can consume more than sixteen intermediate transitions.
    // Reserve bounded terminal receipts so a private reader cannot mistake a
    // truncated journal for failed creation. This grants no action/readiness.
    const bool completed=gate==0&&boltok_visual::ready;
    if(log_file&&(completed?preparation_ready_rows++<16:preparation_rows++<16)){fwprintf(log_file,L"boltok_preparation gate=%u resources=%u asset_gate=%u handle=%llu frame=%llu sequence=%llu age_ns=%lld\n",
        gate,unsigned(boltok_visual::ready),boltok_visual::failure,control.handle,
        frame?frame->key.frame:0ull,frame?frame->controllers.sequence:0ull,
        frame?weaponhand::clock_ns(nullptr)-frame->controllers.sync_ns:-1ll);
        if(completed&&frame)report_owner_witness(L"prepared",*frame);fflush(log_file);}
}
gears_weapon::BoltokCylinderInsertion insertion;P actual_left_world{};
bool recover_ammo() noexcept;
double angle{},component_scale{};
bool manual_enabled() noexcept {return weapon_part_inputs::original_source_enabled.load(std::memory_order_acquire);}
bool outstanding() noexcept {return retained.load(std::memory_order_acquire);}
void request_stop() noexcept {stop_requested.store(true,std::memory_order_release);}
void reset_after_drain() noexcept {if(!outstanding()){clear_pending_start();stop_requested.store(false,std::memory_order_release);}}
template<class Fn>Fn function(const char* label) noexcept {
    const auto m=weaponhand::native_module;const auto p=m?GetProcAddress(m,label):nullptr;
    return p&&weaponhand::executable(p,m)?reinterpret_cast<Fn>(p):nullptr;
}
#include "weapon_boltok_grasp.inl"
bool same_source(hand::Frame const& f) noexcept {
    const auto current_identity=inv::identity(f);
    const unsigned difference=(identity==current_identity?0u:1u)|
        (proposal_reference==f.reference_generation?0u:2u)|
        (nativelancerfollow::bound_key_known?0u:4u)|
        (hand::same(f.weapon,nativelancerfollow::bound_key.weapon)?0u:8u);
    if(difference&&outstanding()&&log_file){
        static unsigned rows{};static std::uint64_t last_frame{};
        if(rows<8&&last_frame!=f.key.frame){++rows;last_frame=f.key.frame;
            const auto& a=identity;const auto& z=current_identity;
            unsigned identity_mask=(!hand::same(a.world,z.world)?1u:0u)|(!hand::same(a.pawn,z.pawn)?2u:0u)|(!hand::same(a.weapon,z.weapon)?4u:0u);
            const std::uint64_t before[]{a.game_start,a.native_start,a.registration,a.lifecycle,a.world_generation,a.hold_generation,a.session,a.space,a.input};
            const std::uint64_t after[]{z.game_start,z.native_start,z.registration,z.lifecycle,z.world_generation,z.hold_generation,z.session,z.space,z.input};
            for(unsigned i=0;i<9;++i)if(before[i]!=after[i])identity_mask|=1u<<(i+3);
            fwprintf(log_file,L"boltok_identity_difference frame=%llu mask=%u lifecycle=%llu,%llu world_generation=%llu,%llu hold=%llu,%llu session=%llu,%llu space=%llu,%llu input=%llu,%llu\n",
                f.key.frame,identity_mask,a.lifecycle,z.lifecycle,a.world_generation,z.world_generation,
                a.hold_generation,z.hold_generation,a.session,z.session,a.space,z.space,a.input,z.input);
            fwprintf(log_file,L"boltok_source_difference mask=%u frame=%llu sequence=%llu stage=%u moving=%u cycle=%u token=%llu reference=%llu captured_reference=%llu weapon=%llx bound_weapon=%llx hold=%llu model_hold=%llu\n",
                difference,f.key.frame,f.controllers.sequence,unsigned(stage),unsigned(moving),unsigned(cycle),f.press_token,
                f.reference_generation,proposal_reference,f.weapon.address,nativelancerfollow::bound_key.weapon.address,
                nativelancerfollow::bound_key.hold_generation,vrequip::model.hold_generation());fflush(log_file);
        }
    }
    return difference==0;
}
void reconcile_completed_grasp(hand::Frame const& f) noexcept {
    if(completed_known&&(!same_source(f)||!f.left_tracked||!f.left_grip_known||!f.left_grip_down||
       !gears_boltok_completed_grasp::exact(f,completed_lease,completed_identity,
           nativelancerfollow::bound_key.component)))clear_completed_grasp();
}
enum class EligibilityFailure:unsigned {none,disabled,wrong_class,unbound,weapon_changed,hold_changed,current_rejected};
struct EligibilityEvidence {
    EligibilityFailure failure{};std::int64_t before_ns{},after_ns{};
    bool local_active_before{},local_active_after{};
};
bool eligible(hand::Frame const& f,EligibilityEvidence* evidence=nullptr) noexcept {
    if(evidence)*evidence={};
    const auto decline=[&](EligibilityFailure failure) noexcept {
        if(evidence)evidence->failure=failure;return false;
    };
    // Preserve the original order and one current-source call. Observations
    // never select permission or renew the immutable source input timestamp.
    if(!manual_enabled())return decline(EligibilityFailure::disabled);
    if(!playground::instance(f.weapon.address,L"Weap_LocustPistol"))return decline(EligibilityFailure::wrong_class);
    if(!nativelancerfollow::bound_key_known)return decline(EligibilityFailure::unbound);
    if(!hand::same(f.weapon,nativelancerfollow::bound_key.weapon))return decline(EligibilityFailure::weapon_changed);
    if(nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation())return decline(EligibilityFailure::hold_changed);
    if(!evidence)return nativeequipment::current(nullptr,f);
    evidence->before_ns=weaponhand::clock_ns(nullptr);
    evidence->local_active_before=hand::frame_active(f,evidence->before_ns);
    const bool current=nativeequipment::current(nullptr,f);
    evidence->after_ns=weaponhand::clock_ns(nullptr);
    evidence->local_active_after=hand::frame_active(f,evidence->after_ns);
    return current?true:decline(EligibilityFailure::current_rejected);
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    return outstanding()&&hand::same(weapon,identity.weapon)&&(angle>.001||stage!=Stage::idle);
}
void switch_dispatched(hand::Frame const& f) noexcept {if(outstanding()&&hand::same(identity.weapon,f.weapon)){switch_requested=true;clear_pending_start();clear_completed_grasp();}}
bool restore() noexcept {
    static unsigned restore_rows{},prior_stage{~0u},prior_result{~0u},prior_cpu{~0u};
    static std::uint64_t prior_handle{};
    const auto restore_stage=[&](unsigned receipt_stage,api::Receipt const* value=nullptr,
        api::Result call_result=api::Result::rejected) noexcept {
        const auto result=unsigned(call_result),cpu=value?unsigned(value->cpu_restored):0;
        const auto handle=value?value->handle:control.handle;
        if(prior_stage==receipt_stage&&prior_result==result&&prior_cpu==cpu&&prior_handle==handle)return;
        prior_stage=receipt_stage;prior_result=result;prior_cpu=cpu;prior_handle=handle;
        if(log_file&&restore_rows++<8){fwprintf(log_file,L"boltok_restore_stage stage=%u handle=%llu retained=%u control_hide=%u has_receipt=%u call_result=%u receipt_result=%u receipt_size=%u receipt_version=%u native_writes=%llu native_epoch=%u cpu_restored=%u\n",
            receipt_stage,handle,unsigned(retained.load(std::memory_order_acquire)),unsigned(control.hide_cylinder),unsigned(value!=nullptr),
            result,value?unsigned(value->result):0,value?value->size:0,value?value->version:0,
            static_cast<unsigned long long>(value?value->writes:0),value?value->epoch:0,cpu);fflush(log_file);}
    };
    clear_pending_start();perfect_window.reset();auto_close.reset();grip_close.reset();grip_closing=false;motion::clear();feed_motion::reset();close_release::reset();
    proposal_known=false;moving=false;held={};clear_completed_grasp();stroke.reset();
    if(!outstanding())return true;
    stage=Stage::recovering;
    if(!recover_ammo()){restore_stage(1);return false;}
    if(!boltok_visual::withdraw()){restore_stage(2);return false;}
    const auto call=function<api::Update>(api::restore_export);if(!call){restore_stage(3);return false;}
    api::Receipt r{};const auto result=call(&control,&r);
    if((result!=api::Result::restored&&result!=api::Result::inactive_restored)||r.size!=sizeof r||
       r.version!=1||r.handle!=control.handle||r.result!=result||!r.cpu_restored){restore_stage(4,&r,result);return false;}
    if(!boltok_visual::restore()){restore_stage(5,&r,result);return false;}
    control={};native_receipt={};identity=proposal_identity={};angle=0;stage=Stage::idle;
    inventory_observer={};live_mask=0;
    transaction={};inventory_writer={};cycle=mask_known=native_exited=false;reload_serial=round_item=round_sequence=0;perfect_ms=0;
    opening_inventory={};completion={};reward={};pending_group_mask=0;insertion.reset();
    switch_requested=false;clear_pending_start();clear_captured();retained.store(false,std::memory_order_release);
    restore_stage(6,&r,result);return true;
}
bool restore_at(unsigned site,hand::Frame const& f,hand::GripGameState const& game,EligibilityEvidence const* evidence=nullptr) noexcept {
    if(stage!=Stage::recovering&&log_file&&recovery_rows++<16){
        const unsigned flags=unsigned(game.known)|(unsigned(game.main_hand_held)<<1)|
            (unsigned(game.busy)<<2)|(unsigned(game.animation_owns_hands)<<3);
        fwprintf(log_file,L"boltok_recovery site=%u stage=%u frame=%llu sequence=%llu token=%llu age_ns=%lld game=%u left=%u,%u,%u angle=%.9g moving=%u cycle=%u hide=%u handle=%llu api=%u asset=%u eligibility=%u current_before_age_ns=%lld current_after_age_ns=%lld local_active_before=%u local_active_after=%u\n",
            site,unsigned(stage),f.key.frame,f.controllers.sequence,f.press_token,
            weaponhand::clock_ns(nullptr)-f.controllers.sync_ns,flags,unsigned(f.left_grip_known),
            unsigned(f.left_grip_down),unsigned(f.left_tracked),angle,unsigned(moving),unsigned(cycle),
            unsigned(control.hide_cylinder),control.handle,unsigned(last_update_result),weapon_part_assets::failure,
            evidence?unsigned(evidence->failure):0u,evidence&&evidence->before_ns?evidence->before_ns-f.controllers.sync_ns:-1,
            evidence&&evidence->after_ns?evidence->after_ns-f.controllers.sync_ns:-1,
            evidence?unsigned(evidence->local_active_before):0u,evidence?unsigned(evidence->local_active_after):0u);
        report_owner_witness(L"recovery",f);fflush(log_file);
    }
    return restore();
}
unsigned active_failure_rows{};
void report_active_failure(unsigned reason,hand::Frame const& f,hand::Decision const& d) noexcept {
    if((!cycle&&!moving&&stage==Stage::idle)||!log_file||active_failure_rows>=16)return;
    ++active_failure_rows;
    fwprintf(log_file,L"boltok_active_rejected reason=%u stage=%u frame=%llu sequence=%llu token=%llu age_ns=%lld prior_control_gap_ns=%lld moving=%u cycle=%u held=%llu decision=%u lease=%u lease_id=%llu release=%u angle=%.9g last_api=%u asset=%u\n",
        reason,unsigned(stage),f.key.frame,f.controllers.sequence,f.press_token,
        weaponhand::clock_ns(nullptr)-f.controllers.sync_ns,
        f.controllers.sync_ns-control.source.controllers.sync_ns,unsigned(moving),unsigned(cycle),held.id,
        unsigned(d.status),unsigned(d.lease.kind),d.lease.id,unsigned(d.release_required),angle,
        unsigned(last_update_result),weapon_part_assets::failure);fflush(log_file);
}
void drain_on_gt() noexcept {
    if(weaponhand::current_gt(nullptr)&&(stop_requested.load()||stage==Stage::recovering))restore();
}
#include "weapon_boltok_motion_observer.inl"
#include "weapon_boltok_motion.inl"
#include "weapon_boltok_close_release.inl"
#include "weapon_boltok_support.inl"
// Calibration is composed from one immutable mount and the authenticated
// asset reference root. A live world getter may be from another render phase.
bool derive_captured(hand::Frame const& f,boltok_parts::Mechanical const& parts,
    CapturedMechanism& out) noexcept {
    out={};CapturedMechanism c{};std::array<float,16> matrix{};P model{};
    V root{};double scale{};gears_hand_authored_mount::Mount left{};
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_boltok||
       !nativelancerfollow::mount.bound()||!hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       !hand::same(nativelancerfollow::bound_key.component,parts.source.component)||
       !hand::same(nativelancerfollow::bound_key.asset,parts.source.asset)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       nativelancerfollow::bound_reference!=f.reference_generation||
       nativelancerfollow::bound_world!=f.key.world_generation||
       nativelancerfollow::bound_session!=f.stereo.session||
       nativelancerfollow::bound_space!=f.stereo.space_epoch||
       nativelancerfollow::bound_input!=f.controllers.input_epoch||
       nativelancerfollow::bound_units!=parts.units||
       !motion::root_reference(f,parts.source.asset,c.root_reference,root)||
       !nativelancerfollow::mount.model_matrix({{},{}},matrix)||
       !hammerburst_runtime::matrix(matrix,model,scale)||
       std::abs(scale-parts.component_scale)>1e-5||
       !gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
           gears_hand_authored_mount::Side::left,1,100,left))return false;
    c.canonical_root.position=gears_weapon::scaled(gears_weapon::sum(model.position,
        gears_weapon::rotate(model.orientation,gears_weapon::scaled(root,scale))),1/parts.units);
    c.canonical_root.orientation=model.orientation;
    gears_boltok_mechanism_geometry::Inputs inputs{};
    inputs.model_in_right=model;inputs.root_in_right_metres=c.canonical_root;
    inputs.hinge_pivot_in_root_metres=parts.in_root[0].position;
    inputs.hinge_axis_in_root={0,1,0};inputs.model_scale=scale;inputs.units=parts.units;
    inputs.angle_radians=0;inputs.left_calibration=left.calibration;
    if(!gears_boltok_mechanism_geometry::derive(gears_boltok_owner_profiles::mechanism,inputs,c.geometry)||
       !nativeequipment::current(nullptr,f))return false;
    c.identity=inv::identity(f);c.weapon=f.weapon;c.component=parts.source.component;c.asset=parts.source.asset;
    c.hold_generation=vrequip::model.hold_generation();c.reference=f.reference_generation;
    c.world_generation=f.key.world_generation;c.session=f.stereo.session;
    c.space_epoch=f.stereo.space_epoch;c.input_epoch=f.controllers.input_epoch;
    c.component_scale=scale;c.units=parts.units;c.known=true;out=c;return true;
}

struct HandRead {
    gears_rig_tracking::Frame rig{};
    bool attempted{},available{},prepared{},eager{};
    unsigned read_reply{};
    std::int64_t read_ns{};
};
bool read_for_hand(hand::Frame const& f,HandRead& out) noexcept {
    out={};
    const auto source=static_cast<std::uint32_t>(f.stereo.source);
    out.prepared=nativelancerfollow::prepared_for(f,out.rig);
    out.read_ns=weaponhand::clock_ns(nullptr);
    if(!out.prepared){
        const auto reply=handrig::read(out.rig,0,source);
        out.read_reply=static_cast<unsigned>(reply.reply);
    }
    if(out.prepared)out.read_reply=static_cast<unsigned>(gears_rig_tracking::Reply::accepted);
    out.attempted=true;
    out.available=out.prepared||out.read_reply==static_cast<unsigned>(gears_rig_tracking::Reply::accepted);
    return out.available;
}
void hand_read_failure(unsigned gate,hand::Frame const& f,HandRead const& read,
    unsigned match_status=~0u,unsigned pose_mask=0) noexcept {
    static unsigned rows{};
    if(rows>=8||!log_file||!(moving||cycle||f.left_grip_down))return;
    ++rows;
    const auto now=weaponhand::clock_ns(nullptr);
    fwprintf(log_file,L"boltok_hand_read_failure gate=%u frame=%llu sequence=%llu reference=%llu token=%llu attempted=%u prepared=%u available=%u read_reply=%u read_ns=%lld early=%u rig_publication=%llu rig_scene=%llu rig_input=%llu rig_reference=%llu pose_mask=%u match=%u age_ns=%lld\n",
        gate,f.key.frame,f.controllers.sequence,f.reference_generation,f.press_token,
        unsigned(read.attempted),unsigned(read.prepared),unsigned(read.available),read.read_reply,read.read_ns,
        unsigned(read.eager),read.rig.publication_sequence,read.rig.scene_invocation,
        read.rig.controllers.sequence,read.rig.reference_generation,pose_mask,match_status,
        f.controllers.sync_ns&&now>=f.controllers.sync_ns?now-f.controllers.sync_ns:-1ll);
    fflush(log_file);
}
bool actual_hand(hand::Frame const& f,boltok_parts::Mechanical const& parts,V& local,HandRead const* early=nullptr) noexcept {
    HandRead own{};
    if(!early){
        (void)read_for_hand(f,own);
        early=&own;
    }
    const auto& rig=early->rig;
    const bool prepared=early->prepared;
    const auto source=static_cast<std::uint32_t>(f.stereo.source);
    // Reuse only the existing follower's exact same authenticated Frame.
    // prepared_for repeats current-source admission; match below and the
    // final current check still apply at the real clock. This is data reuse,
    // not a fresh timestamp or a retained action/lease.
    if(!early->available){hand_read_failure(1,f,*early);return false;}
    const auto match_status=gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),source);
    if(match_status!=gears_equipment_rig::Status::matched){
        hand_read_failure(2,f,*early,static_cast<unsigned>(match_status),rig.world_pose_mask);return false;
    }
    if((rig.world_pose_mask&(gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose))!=
       (gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose)){
        hand_read_failure(3,f,*early,static_cast<unsigned>(match_status),rig.world_pose_mask);return false;
    }
    const auto root=parts.root_metres;
    const auto left=gears_weapon::scaled(rig.world_grip[0].position,1/parts.units);
    actual_left_world=rig.world_grip[0];
    auto right=rig.world_grip[1];right.position=gears_weapon::scaled(right.position,1/parts.units);
    const auto inverse=gears_vr::inverse(right.orientation);
    const P relative{gears_weapon::rotate(inverse,gears_weapon::difference(root.position,right.position)),
        gears_vr::normalized(gears_vr::multiply(inverse,root.orientation))};
    local=gears_weapon::rotate(gears_vr::inverse(root.orientation),gears_weapon::difference(left,root.position));
    if(!gears_vr::finite(local)||!gears_vr::stereo_pose_valid(relative)){
        hand_read_failure(4,f,*early);return false;
    }
    if(!nativeequipment::current(nullptr,f)){hand_read_failure(5,f,*early);return false;}
    static unsigned prepared_rows{},fallback_rows{};auto& boltok_rig_rows=prepared?prepared_rows:fallback_rows;
    if(log_file&&boltok_rig_rows++<2){fwprintf(log_file,L"boltok_hand_rig prepared=%u frame=%llu sequence=%llu observation_only=1\n",unsigned(prepared),f.key.frame,f.controllers.sequence);fflush(log_file);}
    motion_observer::capture(f,rig);
    motion::capture(f,rig,parts,relative,local);
    (void)proposal_projection.capture(f,relative);proposal_identity=inv::identity(f);proposal_reference=f.reference_generation;proposal_known=true;return true;
}
void publish_mechanism_hover(hand::Frame const& f,
    hand::GripGameState const& game,hand::Decision const& d,
    boltok_parts::Mechanical const& parts,V local) noexcept {
    if(stage!=Stage::opened||!cycle||moving||round_item||completed_known||
       stop_requested.load()||switch_requested)return;
    if(!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       !f.left_tracked||!f.left_grip_known||f.left_grip_down||hand::index_pinch(f)||
       !hand::same(d.key,f.key)||d.lease.kind!=hand::LeaseKind::none||
       d.release_required)return;

    const auto& left=f.controllers.hands[0];
    if(left.squeeze_active!=1||left.trigger_active!=1||
       gears_weapon_hand::control_held(left,gears_vr::control_squeeze)||
       gears_weapon_hand::control_held(left,gears_vr::control_trigger))return;

    if(!captured.known||!(captured.identity==inv::identity(f))||
       !hand::same(captured.weapon,f.weapon)||
       captured.hold_generation!=vrequip::model.hold_generation()||
       captured.reference!=f.reference_generation||
       captured.world_generation!=f.key.world_generation||
       captured.session!=f.stereo.session||
       captured.space_epoch!=f.stereo.space_epoch||
       captured.input_epoch!=f.controllers.input_epoch)return;

    if(!hand::same(parts.source.key,f.key)||
       !hand::same(parts.source.component,captured.component)||
       !hand::same(parts.source.asset,captured.asset)||
       parts.units!=captured.units||
       std::abs(parts.component_scale-captured.component_scale)>1e-5||
       !std::isfinite(parts.units)||parts.units<10||parts.units>500||
       !parts.native_observed||parts.observed_ns<=0)return;

    const auto now=weaponhand::clock_ns(nullptr);
    if(now<parts.observed_ns||now-parts.observed_ns>100000000)return;
    if(!gears_vr::stereo_pose_valid(actual_left_world)||
       !gears_vr::stereo_pose_valid(parts.root_metres)||
       !gears_vr::stereo_pose_valid(parts.in_root[0]))return;

    const auto delta=gears_vr::normalized(gears_vr::multiply(
        parts.in_root[0].orientation,gears_vr::inverse(closed_support)));
    if(!gears_vr::stereo_pose_valid({{},delta}))return;

    const V pivot=hinge.pivot;
    const auto rotate_point=[&](V point) noexcept {
        return gears_weapon::sum(pivot,
            gears_weapon::rotate(delta,
                gears_weapon::difference(point,pivot)));
    };
    const V target_local=rotate_point(
        captured.geometry.closed_grip_in_root_metres.position);
    const V centre_local=rotate_point(
        captured.geometry.volume.center_in_root_metres);
    if(!gears_vr::finite(target_local)||!gears_vr::finite(centre_local))return;

    const auto& volume=captured.geometry.volume;
    const double radius=gears_weapon::length(
        gears_weapon::difference(local,centre_local));
    const double snap=gears_weapon::length(
        gears_weapon::difference(local,target_local));
    const double snap_limit=(std::min)(volume.snap_distance_metres,.1);
    if(!std::isfinite(radius)||!std::isfinite(snap)||
       !std::isfinite(snap_limit)||radius>volume.radius_metres+1e-5||
       snap>snap_limit+1e-5)return;

    const auto expected_orientation=gears_vr::normalized(
        gears_vr::multiply(gears_vr::multiply(
            parts.root_metres.orientation,delta),captured.geometry.closed_grip_in_root_metres.orientation));
    const double dot=std::abs(
        double(actual_left_world.orientation.x)*expected_orientation.x+
        double(actual_left_world.orientation.y)*expected_orientation.y+
        double(actual_left_world.orientation.z)*expected_orientation.z+
        double(actual_left_world.orientation.w)*expected_orientation.w);
    const double hover_angle=2.0*std::acos((std::min)(1.0,(std::max)(0.0,dot)));
    if(!std::isfinite(hover_angle)||hover_angle>volume.snap_angle_radians+1e-5)return;

    namespace authored=gears_hand_authored_grasp;
    authored::Request q{};
    q.source=f;
    q.geometry_signature=gears_hand_authored_mount::marcus115_signature;
    q.profile_id=gears_boltok_owner_profiles::mechanism.profile_id;
    q.side=authored::Side::left;
    q.mode=authored::Mode::left_part;
    q.kind=authored::object::Kind::magazine;
    q.proximity=1;
    q.grip.position=gears_weapon::scaled(gears_weapon::sum(
        parts.root_metres.position,
        gears_weapon::rotate(parts.root_metres.orientation,target_local)),
        parts.units);
    q.grip.orientation=expected_orientation;
    q.closed=gears_boltok_owner_profiles::mechanism.fingers_native;
    if(!gears_hand_marcus115_reference::read(q.geometry_signature,q.side,
           q.reference)||!authored::structural(q))return;

    const auto call=function<authored::Publish>(authored::publish_export);
    if(!call||!nativeequipment::current(nullptr,f))return;
    const bool accepted=call(&q);

    static unsigned rows{};
    static bool known{},prior_accepted{};
    if(log_file&&rows<12&&
       (!known||accepted!=prior_accepted)){
        ++rows;
        fwprintf(log_file,
            L"boltok_mechanism_hover accepted=%u profile=%llu frame=%llu "
            L"sequence=%llu proximity=1 observed_serial=%llu age_ns=%lld "
            L"radius_distance=%.9g snap_distance=%.9g angle=%.9g "
            L"capture=1794486092117-4\n",
            unsigned(accepted),q.profile_id,f.key.frame,f.controllers.sequence,
            parts.observed_serial,now-parts.observed_ns,radius,snap,hover_angle);
        fflush(log_file);
    }
    known=true;prior_accepted=accepted;
}

struct ProjectionTrace {unsigned gate{100};std::int64_t now{},hint_sync{},input_sync{};};
bool proposal_hand(hand::Frame const& f,V& local,ProjectionTrace* trace=nullptr) noexcept {
    if(trace){trace->hint_sync=proposal_projection.sync;trace->input_sync=f.controllers.sync_ns;}
    if(!proposal_known||!(proposal_identity==inv::identity(f))||
       proposal_reference!=f.reference_generation)return false;
    if(!captured.known||!(captured.identity==inv::identity(f))||
       !hand::same(captured.weapon,f.weapon)||
       captured.hold_generation!=vrequip::model.hold_generation()||
       captured.reference!=f.reference_generation||
       captured.world_generation!=f.key.world_generation||
       captured.session!=f.stereo.session||
       captured.space_epoch!=f.stereo.space_epoch||
       captured.input_epoch!=f.controllers.input_epoch)return false;
    const auto now=weaponhand::clock_ns(nullptr);
    if(trace)trace->now=now;
    return proposal_projection.project(f,now,local,trace?&trace->gate:nullptr);
}
V contact() noexcept {
    const V offset=gears_weapon::difference(hinge.closed_contact,hinge.pivot);
    const V axial=gears_weapon::scaled(hinge.axis,gears_weapon::dot(offset,hinge.axis));
    const V radial=gears_weapon::difference(offset,axial);
    return gears_weapon::sum(hinge.pivot,gears_weapon::sum(axial,
        gears_weapon::sum(gears_weapon::scaled(radial,std::cos(angle)),
            gears_weapon::scaled(gears_weapon::cross(hinge.axis,radial),std::sin(angle)))));
}
bool granted(hand::Frame const& f,hand::Decision const& d,hand::LeaseKind kind=hand::LeaseKind::magazine) noexcept {
    return hand::same(f.key,d.key)&&!d.release_required&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        d.lease.kind==kind&&hand::lease_valid(d.lease,f.key.lifecycle)&&
        d.lease.id==f.press_token&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
void committed(hand::Frame const& f,hand::Decision const& d) noexcept {
    reconcile_completed_grasp(f);
    if(completed_known&&(!granted(f,d)||!hand::same(completed_lease,d.lease)))clear_completed_grasp();
    if(outstanding()&&same_source(f)&&d.new_grant&&granted(f,d)){
        accepted_intent=f.press_token;
        pending_start={inv::identity(f),f.key,d.lease,f.press_token,f.controllers.sequence,
            f.key.frame,f.reference_generation,true};
    }
}
// Bounded numeric observations only; never query/log from inside the arbiter.
struct ProposalRecord {
    std::uint64_t frame{},sequence{},token{};unsigned gate{1};
    bool proposal{},captured{},request{};double angle{},radius{},snap{};
    V local{},centre{},target{};P canonical{};
    ProjectionTrace projection{};bool pending_before{},pending_after{};
};
std::array<ProposalRecord,32> proposal_records{};
unsigned proposal_record_count{},proposal_records_flushed{};
struct ProposalProbe {
    ProposalRecord* row{};
    explicit ProposalProbe(hand::Frame const& f) noexcept {
        if(stage!=Stage::opened||!cycle||!f.left_grip_down||!f.press_token||
           proposal_record_count>=proposal_records.size())return;
        unsigned count{},same_frame{};
        for(unsigned i=0;i<proposal_record_count;++i)if(proposal_records[i].token==f.press_token){
            if((proposal_records[i].frame==f.key.frame&&++same_frame>=4)||++count>=8)return;
        }
        row=&proposal_records[proposal_record_count++];
        row->frame=f.key.frame;row->sequence=f.controllers.sequence;row->token=f.press_token;
        row->proposal=proposal_known;row->captured=captured.known;row->angle=angle;row->canonical=captured.canonical_root;
    }
    void gate(unsigned value) noexcept {if(row)row->gate=value;}
};
void flush_proposal_records() noexcept {
    bool wrote=false;
    while(proposal_records_flushed<proposal_record_count){
        const auto& r=proposal_records[proposal_records_flushed++];if(!log_file)continue;
        fwprintf(log_file,L"boltok_proposal frame=%llu sequence=%llu token=%llu gate=%u proposal=%u captured=%u request=%u angle=%.9g radius_distance=%.9g snap_distance=%.9g local=%.9g,%.9g,%.9g centre=%.9g,%.9g,%.9g contact=%.9g,%.9g,%.9g canonical=%.9g,%.9g,%.9g canonical_q=%.9g,%.9g,%.9g,%.9g\n",
            r.frame,r.sequence,r.token,r.gate,unsigned(r.proposal),unsigned(r.captured),unsigned(r.request),r.angle,r.radius,r.snap,
            r.local.x,r.local.y,r.local.z,r.centre.x,r.centre.y,r.centre.z,r.target.x,r.target.y,r.target.z,
            r.canonical.position.x,r.canonical.position.y,r.canonical.position.z,
            r.canonical.orientation.x,r.canonical.orientation.y,r.canonical.orientation.z,r.canonical.orientation.w);wrote=true;
        fwprintf(log_file,L"boltok_projection_check frame=%llu sequence=%llu token=%llu gate=%u now_ns=%lld hint_sync_ns=%lld input_sync_ns=%lld hint_age_ns=%lld input_age_ns=%lld request=%u pending_before=%u pending_after=%u\n",
            r.frame,r.sequence,r.token,r.projection.gate,r.projection.now,r.projection.hint_sync,r.projection.input_sync,
            r.projection.now&&r.projection.hint_sync?r.projection.now-r.projection.hint_sync:-1ll,
            r.projection.now&&r.projection.input_sync?r.projection.now-r.projection.input_sync:-1ll,
            unsigned(r.request),unsigned(r.pending_before),unsigned(r.pending_after));
    }
    if(wrote)fflush(log_file);
}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    ProposalProbe probe(f);
    if(probe.row)probe.row->pending_before=probe.row->pending_after=game.magazine_observation_pending;
    // Inside the hand arbiter: no Native/HandClient reentry or bone getters.
    reconcile_completed_grasp(f);
    if(!manual_enabled()||!outstanding()||!same_source(f)||stage==Stage::recovering||
       !game.main_hand_held||game.busy||game.animation_owns_hands||!f.left_tracked||!f.left_grip_down||
       game.kind!=hand::WeaponKind::firearm||!playground::instance(f.weapon.address,L"Weap_LocustPistol"))return;
    probe.gate(2);
    if(completed_known){
        if(!gears_boltok_completed_grasp::project(game.object_incumbents,completed_lease)){
            clear_completed_grasp();return;
        }
        return;
    }
    probe.gate(3);
    if((moving||round_item||stage==Stage::acquiring)&&held.id==f.press_token&&hand::lease_valid(held,f.key.lifecycle)){
        for(auto& slot:game.object_incumbents){if(hand::same(slot,held))return;
            if(slot.kind==hand::LeaseKind::none){slot=held;return;}}return;
    }
    probe.gate(4);
    if(stage==Stage::opened&&cycle&&!round_item&&transaction.inserted()<transaction.target()&&boltok_visual::ready){
        const P body{f.stereo.head.position,f.reference.orientation};
        game.request_ammo=gears_weapon::body_slot(nativeequipment::calibration,body,
            f.controllers.hands[0].grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;
        if(game.request_ammo)return;
    }
    probe.gate(5);
    V local{};const bool proposal_ready=captured.known&&proposal_hand(f,local,probe.row?&probe.row->projection:nullptr);
    if(captured.known&&!proposal_ready){
        // Unknown geometry defers only an undispatched press through the
        // existing arbiter path. It never creates contact or steals a lease.
        // The normal tick can now measure the current root before selection.
        game.magazine_observation_pending=true;if(probe.row)probe.row->pending_after=true;return;
    }
    if(proposal_ready){
        const auto centre=gears_boltok_mechanism_geometry::rotate_root(
            captured.geometry.volume.center_in_root_metres,hinge.pivot,hinge.axis,angle);
        const double radius=gears_weapon::length(gears_weapon::difference(local,centre));
        const auto target=contact();
        const double snap=gears_weapon::length(gears_weapon::difference(local,target));
        const double tolerance=(std::min)(captured.geometry.volume.snap_distance_metres,.1)+1e-5;
        game.request_magazine=radius<=captured.geometry.volume.radius_metres+1e-5&&snap<=tolerance;
        probe.gate(game.request_magazine?7:6);
        if(probe.row){auto& r=*probe.row;r.request=game.request_magazine;r.local=local;
            r.centre=centre;r.target=target;r.radius=radius;r.snap=snap;}
    }
}
gears_weapon::BoltokReloadStamp stamp(hand::Frame const& f) noexcept {
    const auto id=inv::identity(f);float seconds{};
    if(!lancerreload_state::time_field(f.world.address,L"TimeSeconds",seconds)||!std::isfinite(seconds)||seconds<0)return {};
    const gears_weapon::BoltokReloadIdentity key{f.world.address,f.pawn.address,f.weapon.address,f.key.world_generation,
        id.hold_generation,f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation};
    return {key,f.controllers.sequence,f.controllers.sync_ns,std::int64_t(double(seconds)*1000),
        gears_vr::controller_pose_usable(f.controllers.hands[1].grip,f.controllers.hands[1].grip_flags,
            f.controllers.hands[1].grip_active),bool(f.left_tracked),true};
}
#include "weapon_boltok_feed_motion.inl"
#include "weapon_boltok_ammo.inl"
// Pure numeric diagnostic ring. It never stores a Frame, pointer, engine
// object or permission and performs no query. The bind token only partitions
// failures; normal motion clears do not erase the pre-failure history.
struct BoltokGapTrace {
    struct Row {
        std::uint64_t frame{},sequence{},source_sequence{},previous_sequence{};
        unsigned measure_failure{};
        bool parts_measured{},native_observed{},hand_known{},input_matched{};
        unsigned motion_gate{};
        unsigned motion_failure{},incoming_count{};
        bool transport_matched{},merge_attempted{},merge_matched{};
        std::uint64_t incoming_first{},incoming_last{};
    };
    std::array<Row,16> rows{};
    unsigned count{};
    std::uint64_t token{};
    bool token_known{};

    void record(hand::Frame const& f,std::uint64_t bind_token,bool motion_active,
        std::uint64_t source_sequence,std::uint64_t previous_sequence,
        unsigned measure_failure,bool parts_measured,bool native_observed,
        bool hand_known,unsigned motion_gate,bool input_matched) noexcept {
        if(!motion_active)return;
        if(!token_known||bind_token!=token){
            token=bind_token;token_known=true;count=0;
        }
        Row row{f.key.frame,f.controllers.sequence,source_sequence,
            previous_sequence,measure_failure,parts_measured,native_observed,
            hand_known,input_matched,motion_gate};
        row.motion_failure=unsigned(motion::input.failure);
        row.transport_matched=motion::input.transport_matched;
        row.merge_attempted=motion::input.merge_attempted;
        row.merge_matched=motion::input.merge_matched;
        row.incoming_count=motion::input.incoming_count;
        row.incoming_first=motion::input.incoming_first;
        row.incoming_last=motion::input.incoming_last;
        if(count<rows.size())rows[count++]=row;
        else{
            for(unsigned i=1;i<rows.size();++i)rows[i-1]=rows[i];
            rows.back()=row;
        }
    }

    bool dumped{};
    void dump(FILE* file) noexcept {
        if(!file||dumped)return;dumped=true;
        for(unsigned i=0;i<count;++i){
            const auto& r=rows[i];
            fwprintf(file,
                L"boltok_gap_trace row=%u/%u frame=%llu sequence=%llu "
                L"source_sequence=%llu previous_sequence=%llu "
                L"measure_failure=%u parts_measured=%u native_observed=%u "
                L"hand_known=%u motion_gate=%u input_matched=%u "
                L"motion_failure=%u transport_matched=%u merge_attempted=%u merge_matched=%u incoming_count=%u incoming_first=%llu incoming_last=%llu\n",
                i+1,count,r.frame,r.sequence,r.source_sequence,
                r.previous_sequence,r.measure_failure,
                unsigned(r.parts_measured),unsigned(r.native_observed),
                unsigned(r.hand_known),r.motion_gate,unsigned(r.input_matched),
                r.motion_failure,unsigned(r.transport_matched),unsigned(r.merge_attempted),unsigned(r.merge_matched),
                r.incoming_count,r.incoming_first,r.incoming_last);
        }
        fflush(file);
    }
} gap_trace;


bool tick(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    if(!manual_enabled()&&!outstanding()){publish_normal_support(pc);return false;}
    if(stage==Stage::completing){complete_ammo();return true;}
    if(stop_requested.load()||switch_requested||stage==Stage::recovering){if(outstanding())restore();return outstanding();}
    hand::Frame f{};hand::GripGameState game{};hand::Decision d{};
    TickCost cost(f);hand::FrameReadObservation read{};
    if(!weaponhand::client.copy_frame_on_gt(f,game,d,&read)||f.controller.address!=pc){
        cost.mark(0);cost.force=cost.active_before;const auto code=unsigned(read.failure);
        if((cycle||moving||stage!=Stage::idle)&&log_file&&read_rows<16&&
           (last_read_code!=code||last_read_difference!=read.game_difference)){
            ++read_rows;
            fwprintf(log_file,L"boltok_read_rejected stage=%u failure=%u difference=%u frame=%llu sequence=%llu token=%llu age_ns=%lld down=%u frame_difference=%u game_flags=%u candidate_flags=%u decision=%u lease=%u\n",
                unsigned(stage),code,read.game_difference,read.frame,read.sequence,read.press_token,
                read.input_age_ns,read.left_grip_down,read.frame_difference,read.game_flags,
                read.candidate.flags,read.candidate.decision,read.candidate.lease);fflush(log_file);
        }
        last_read_code=code;last_read_difference=read.game_difference;return outstanding();
    }
    cost.mark(0);last_read_code=last_read_difference=~0u;
    if(pending_start.known){
        if(!same_source(f)||
           !(pending_start.identity==inv::identity(f))||
           !same_pending_domain(pending_start.key,f.key)||
           !hand::same(pending_start.lease,d.lease)||
           pending_start.press!=f.press_token||
           f.key.frame<pending_start.frame||
           f.controllers.sequence<pending_start.sequence||
           d.release_required||
           f.reference_generation!=pending_start.reference||
           !game.known||!game.main_hand_held||game.busy||
           game.animation_owns_hands||!f.left_tracked||
           !f.left_grip_known||!f.left_grip_down||
           !granted(f,d)){
            clear_pending_start();
            return outstanding();
        }
    }
    // One caller-owned comparison record for this exact copied Frame only.
    // Every original current guard rereads all witnesses; writes/metadata
    // changes force a complete original observation. No input/lease refresh.
    std::optional<weaponhand::source_validation::Scope> source_read_scope;
    if(!weaponhand::source_validation::active)source_read_scope.emplace(f);
    HandRead early_hand{};
    // Public arbiter already selected left support. This routes unrelated
    // mechanism work away; it grants no action or authority.
    const bool support_only=d.lease.kind==hand::LeaseKind::support&&
        stage==Stage::idle&&!cycle&&!moving&&!round_item&&!reload_serial&&
        !pending_start.known&&!control.hide_cylinder&&angle==0&&
        !native_receipt.hidden&&native_receipt.angle==0;
    const bool early_hand_attempted=moving||(f.left_grip_down&&!support_only);
    if(early_hand_attempted){(void)read_for_hand(f,early_hand);early_hand.eager=true;}
    flush_proposal_records();
    reconcile_completed_grasp(f);
    if(completed_known&&(!granted(f,d)||!hand::same(completed_lease,d.lease)))clear_completed_grasp();
    main_skin::publish(f);
    // Only the closed idle weapon may offer passive support fitting. Active
    // mechanics, item transactions and all restoration obligations keep priority.
    if(stage==Stage::idle&&!cycle&&!moving&&!round_item&&!reload_serial&&
       !pending_start.known&&!control.hide_cylinder&&angle==0&&
       !native_receipt.hidden&&native_receipt.angle==0)
        support_skin::publish_hover(f,game,d);
    support_skin::publish(f,d);
    if(same_source(f)){
        if(round_item==f.press_token&&round_item)part_skin::publish(f,d,held,true);
        else if(moving||stage==Stage::acquiring)part_skin::publish(f,d,held,false);
        else if(completed_known)part_skin::publish(f,d,completed_lease,false);
    }
    motion::read_early(f);
    EligibilityEvidence eligibility{};
    if(!eligible(f,&eligibility)||!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands){
        proposal_known=false;if(outstanding())restore_at(__LINE__,f,game,&eligibility);return outstanding();
    }
    if(outstanding()&&!same_source(f)){restore_at(__LINE__,f,game);return true;}
    // Display-source maintenance only: same bound value, independently admitted current Frame.
    // This does not advance a stroke or borrow the observation as permission.
    const bool need_observation=outstanding()&&(control.hide_cylinder||native_receipt.hidden||
        native_receipt.angle!=0||cycle||moving||stage!=Stage::idle||f.left_grip_down||round_item||reload_serial);
    boltok_parts::ObservationQuery query{};
    if(need_observation){
        const auto update=function<api::Update>(api::update_export);
        if(!update||native_receipt.size!=sizeof native_receipt||native_receipt.version!=1||
           native_receipt.handle!=control.handle||!std::isfinite(native_receipt.angle)){
            restore_at(__LINE__,f,game);return true;
        }
        auto same=control;same.source=f;same.angle=native_receipt.angle;same.hide_cylinder=native_receipt.hidden;
        api::Receipt receipt{};const auto result=update(&same,&receipt);
        if((result!=api::Result::ready&&result!=api::Result::pending)||receipt.size!=sizeof receipt||receipt.version!=1||
           receipt.result!=result||receipt.handle!=same.handle||receipt.hidden!=same.hide_cylinder||
           !std::isfinite(receipt.angle)||std::abs(receipt.angle-same.angle)>1e-6){restore_at(__LINE__,f,game);return true;}
        // control.source is the existing action-consumer watermark. The Native
        // display has admitted same.source, but no motion was consumed here.
        control.angle=same.angle;control.hide_cylinder=same.hide_cylinder;
        native_receipt=receipt;last_update_result=result;
        query.read=function<api::ObservationRead>(api::observation_export);
        query.handle=control.handle;query.cookie=control.cookie;query.hidden=control.hide_cylinder;
    }
    // No visible replacement or action exists in this state. Current Frame,
    // eligibility, source identity and recovery checks above remain mandatory.
    // Skip hidden-part geometry/preparation only; the next interaction still
    // remeasures and fully admits its own current source before any mutation.
    if(outstanding()&&boltok_visual::ready&&captured.known&&
       captured.identity==inv::identity(f)&&captured.reference==f.reference_generation&&
       captured.hold_generation==vrequip::model.hold_generation()&&
       stage==Stage::idle&&!cycle&&!moving&&!control.hide_cylinder&&angle==0&&
       (!f.left_grip_down||support_only)&&!round_item&&!reload_serial&&
       !pending_start.known)return true;
    cost.mark(1);boltok_parts::Mechanical parts{};V local{};
    boltok_parts::MeasureFailure measure_failure{};
    const bool parts_measured=boltok_parts::measure(f,parts,outstanding()&&control.hide_cylinder?&cylinder_relative:nullptr,
        need_observation?&query:nullptr,&measure_failure);cost.mark(2);
    if(need_observation){
        static unsigned rows{},last_stage{~0u},last_status{~0u};const auto status=unsigned(measure_failure);
        if(log_file&&rows<8&&(last_stage!=unsigned(stage)||last_status!=status)){
            ++rows;last_stage=unsigned(stage);last_status=status;
            fwprintf(log_file,L"boltok_observation_adopt accepted=%u failure=%u stage=%u frame=%llu sequence=%llu handle=%llu epoch=%u serial=%llu observed_ns=%lld age_ns=%lld\n",
                unsigned(parts_measured&&parts.native_observed),status,unsigned(stage),f.key.frame,f.controllers.sequence,
                control.handle,parts.observed_epoch,parts.observed_serial,parts.observed_ns,
                parts.observed_ns?weaponhand::clock_ns(nullptr)-parts.observed_ns:0);fflush(log_file);
        }
    }
    if(!parts_measured){
        gap_trace.record(f,f.press_token,moving,
            control.source.controllers.sequence,motion::previous.raw.input.sequence,
            unsigned(measure_failure),parts_measured,parts.native_observed,false,
            motion::current.gate,motion::input.matched);
        if(measure_failure==boltok_parts::MeasureFailure::observation){report_preparation(8);return true;}
        cost.force=cost.active_before;report_active_failure(1,f,d);report_preparation(1);return true;
    }
    const bool hand_known=actual_hand(f,parts,local,early_hand_attempted?&early_hand:nullptr);cost.mark(3);
    if(!hand_known){
        gap_trace.record(f,f.press_token,moving,
            control.source.controllers.sequence,motion::previous.raw.input.sequence,
            unsigned(measure_failure),parts_measured,parts.native_observed,false,
            motion::current.gate,motion::input.matched);
        cost.force=cost.active_before;report_active_failure(2,f,d);report_preparation(2);return true;
    }
    if(captured.known&&(!hand::same(captured.component,parts.source.component)||
       !hand::same(captured.asset,parts.source.asset)||captured.units!=parts.units||
       std::abs(captured.component_scale-parts.component_scale)>1e-5)){
        restore_at(__LINE__,f,game);return true;
    }
    publish_mechanism_hover(f,game,d,parts,local);
    if(!outstanding()){
        // First bind is display ownership only; it cannot rotate a part.
        if(hammerburst_animation::engaged_for(f)||lancerreload_state::engaged_for(f))return true;
        CapturedMechanism candidate{};if(!derive_captured(f,parts,candidate)){report_preparation(6);return true;}
        hand::ObjectIdentity node{};const auto call=function<api::Begin>(api::bind_export);
        if(!call){report_preparation(3);return true;}
        if(!hammerburst_animation::selected_node(f,node)){report_preparation(4);return true;}
        api::Request q{};q.source=f;q.component=parts.source.component;q.asset=parts.source.asset;q.node=node;
        api::Receipt r{};const auto result=call(&q,&r);
        if((result!=api::Result::pending&&result!=api::Result::ready&&result!=api::Result::restoring)||
           r.size!=sizeof r||r.version!=1||!r.handle||r.result!=result){report_preparation(5);return true;}
        identity=inv::identity(f);control={};control.handle=r.handle;control.cookie=f.key.registration_cookie;
        native_receipt=r;component_scale=parts.component_scale;closed_support=parts.in_root[0].orientation;
        if(!gears_boltok_child_pose::relative(parts.in_root[0],parts.in_root[1],cylinder_relative)){
            restore_at(__LINE__,f,game);return true;
        }
        last_intent=accepted_intent=0;
        hinge={};hinge.pivot=parts.in_root[0].position;hinge.axis={0,1,0};hinge.closed_contact=parts.in_root[1].position;
        hinge.open_angle=api::maximum_angle;hinge.contact_radius=.045;hinge.current_instance_verified=true;
        // Assign only after display ownership succeeds. Failed calibration
        // allocates nothing and is retried with the next current Frame.
        captured=candidate;
        if(captured.known){
            hinge.closed_contact=captured.geometry.closed_grip_in_root_metres.position;
            hinge.contact_radius=(std::min)(captured.geometry.volume.snap_distance_metres,.1);
            if(log_file){const auto& g=captured.geometry;
                fwprintf(log_file,L"boltok_mechanism_geometry capture=%hs weapon=%llx hold=%llu units=%.9g closed_grip=%.9g,%.9g,%.9g closed_rotation=%.9g,%.9g,%.9g,%.9g volume=%.9g,%.9g,%.9g radius=%.9g snap=%.9g release=%.9g pivot=%.9g,%.9g,%.9g axis=%.9g,%.9g,%.9g closed_support=%.9g,%.9g,%.9g,%.9g observation_only=1\n",
                    g.capture_id,f.weapon.address,captured.hold_generation,parts.units,
                    g.closed_grip_in_root_metres.position.x,g.closed_grip_in_root_metres.position.y,g.closed_grip_in_root_metres.position.z,
                    g.closed_grip_in_root_metres.orientation.x,g.closed_grip_in_root_metres.orientation.y,g.closed_grip_in_root_metres.orientation.z,g.closed_grip_in_root_metres.orientation.w,
                    g.volume.center_in_root_metres.x,g.volume.center_in_root_metres.y,g.volume.center_in_root_metres.z,
                    g.volume.radius_metres,g.volume.snap_distance_metres,g.volume.release_distance_metres,
                    hinge.pivot.x,hinge.pivot.y,hinge.pivot.z,hinge.axis.x,hinge.axis.y,hinge.axis.z,
                    closed_support.x,closed_support.y,closed_support.z,closed_support.w);fflush(log_file);}
        }
        retained.store(true,std::memory_order_release);if(result==api::Result::restoring)stage=Stage::recovering;return true;
    }
    if(std::abs(parts.component_scale-component_scale)>1e-5){restore_at(__LINE__,f,game);return true;}
    boltok_visual::prepare(f,parts);
    report_preparation(boltok_visual::ready?0u:100u+boltok_visual::failure,&f);cost.mark(4);
    // Construction owns this frame's bounded engine work. Before any physical
    // action, retain its closed, hidden-resource progress instead of spending
    // the same sample again on inventory/native publication and then restoring
    // everything because that sample expired. Existing actions still fail shut.
    if(!boltok_visual::ready&&!control.hide_cylinder&&!cycle&&!moving&&stage==Stage::idle)return true;
    if(control.hide_cylinder&&!boltok_visual::ready){restore_at(__LINE__,f,game);return true;}
    // A closed original with no physical grab or transaction needs no new
    // inventory observation, mechanism command or render publication. Keep its
    // hidden prepared resources. Native independently declines an expired old
    // display source; the next real grab still uses the original current Frame,
    // lease and stamp below. No action or timestamp is retained by this return.
    if(stage==Stage::idle&&!cycle&&!moving&&!control.hide_cylinder&&angle==0&&
       !f.left_grip_down&&!round_item&&!reload_serial)return true;
    if(captured.known&&(!(captured.identity==inv::identity(f))||
       !hand::same(captured.weapon,f.weapon)||captured.hold_generation!=vrequip::model.hold_generation()||
       captured.reference!=f.reference_generation||captured.world_generation!=f.key.world_generation||
       captured.session!=f.stereo.session||captured.space_epoch!=f.stereo.space_epoch||
       captured.input_epoch!=f.controllers.input_epoch))clear_captured();
    inv::Inventory inventory{};
    const bool inventory_known=inventory_observer.sample(f,inventory)&&inventory.capacity==6;
    if(inventory_known){
        if(!mask_known){live_mask=(1u<<inventory.loaded)-1u;mask_known=true;}
        else if(cycle&&!inv::counts_equal(inventory,transaction.inventory())){restore_at(__LINE__,f,game);return true;}
        else if(!cycle){unsigned count=0;for(auto mask=live_mask;mask;mask>>=1)count+=mask&1u;
            if(inventory.loaded>count){restore_at(__LINE__,f,game);return true;}
            while(count>inventory.loaded){live_mask&=live_mask-1;--count;}}
    }
    else if(control.hide_cylinder){restore_at(__LINE__,f,game);return true;}
    const auto call=function<api::Update>(api::update_export);if(!call){restore_at(__LINE__,f,game);return true;}
    const auto s=stamp(f);const auto now=weaponhand::clock_ns(nullptr);
    cost.mark(5);if(!s.valid(now)){cost.force=true;restore_at(__LINE__,f,game);return true;}
    if(inventory_known&&stage==Stage::opened&&!round_item&&!inventory.infinite_reserve&&!inventory.reserve)
        boltok_sound::empty(f);
    if(round_item||stage==Stage::opened){if(ammo_step(f,d,parts,s,now))return true;}
    // A new Grip dispatches one finite close command. Releasing after that
    // dispatch cancels the bonus; only the already requested real zero-angle
    // endpoint can complete, with current source/identity and Native ACK.
    if(grip_closing&&!auto_close.armed&&!f.left_grip_down){
        const bool canceled=perfect_window.armed();perfect_window.reset();
        if(canceled&&log_file){fwprintf(log_file,L"boltok_perfect_cancel reason=grip_release frame=%llu sequence=%llu token=%llu serial=%llu dispatch_ns=%lld release_sync_ns=%lld now_ns=%lld\n",
            f.key.frame,f.controllers.sequence,f.press_token,reload_serial,grip_close.dispatch_ns(),
            f.controllers.sync_ns,weaponhand::clock_ns(nullptr));fflush(log_file);}
    }
    if(!f.left_grip_down&&!grip_closing){moving=false;held={};clear_completed_grasp();stroke.reset();}
    if(grip_closing&&(!grip_close.current(s,f.press_token,now)||
       (auto_close.armed&&auto_close.reload!=reload_serial)||
       (!auto_close.armed&&f.left_grip_down&&(!granted(f,d)||!hand::same(held,d.lease))))){restore_at(__LINE__,f,game);return true;}
    const bool captured_contact=captured.known&&([&]() noexcept {
        const auto centre=gears_boltok_mechanism_geometry::rotate_root(
            captured.geometry.volume.center_in_root_metres,hinge.pivot,hinge.axis,angle);
        const double radius=gears_weapon::length(gears_weapon::difference(local,centre));
        const double snap=gears_weapon::length(gears_weapon::difference(local,contact()));
        const double tolerance=(std::min)(captured.geometry.volume.snap_distance_metres,.1)+1e-5;
        return radius<=captured.geometry.volume.radius_metres+1e-5&&snap<=tolerance;
    })();
    if(!moving&&captured_contact&&granted(f,d)&&f.press_token>last_intent&&(d.new_grant||accepted_intent==f.press_token)){
        // Never start the original transaction with an unpaired world/root
        // observation. capture already enforces the actual current closure.
        if(!motion::current.valid||!hand::same(motion::current.frame.key,f.key)){
            motion::report(f,4,false);return true;
        }
        if(!cycle){
            if(!inventory_known||!boltok_visual::ready)return true;
            opening_inventory=inventory;cycle=true;held=d.lease;clear_pending_start();
            if(!transaction.bind(inventory)||!lancerreload_state::capture_reward_eligibility(inventory,reward)){restore_at(__LINE__,f,game);return true;}
            const auto target=(std::min)(inventory.capacity-inventory.loaded,
                inventory.infinite_reserve?inventory.capacity:inventory.reserve);
            auto start=f.controllers.sync_ns/1000000;
            if(target){
                const auto result=lancerreload_state::begin(f,reload_serial);
                if(result!=lancerreload_state::BeginResult::acquired){restore_at(__LINE__,f,game);return true;}
                if(!lancerreload_state::accepted_clock(reload_serial,start,perfect_ms)){restore_at(__LINE__,f,game);return true;}
            }
            if(!transaction.begin(inventory,f.press_token,gears_weapon::ReloadIntentSource::mechanism_open,
                reload_serial,gears_weapon::ReloadTxnMode::round,6,start)){restore_at(__LINE__,f,game);return true;}
            if(reload_serial&&!perfect_window.begin(reload_serial,start,600)){restore_at(__LINE__,f,game);return true;}
            if(reload_serial){stage=Stage::acquiring;return true;}
        }
        closing=stage==Stage::opened||stage==Stage::closing;
        if(closing){
            if(!grip_close.begin(s,f.press_token,now)){restore_at(__LINE__,f,game);return true;}
            // Negative-only original release metadata. Failure cannot create
            // a new action; the ordinary Frame/lease/ACK guards still own close.
            (void)close_release::begin(f);
            clear_pending_start();grip_closing=true;moving=true;held=d.lease;
            last_intent=f.press_token;stage=Stage::closing;angle=0;
            boltok_sound::event(f,3u,f.press_token);
        }else if(stroke.begin(hinge,s,local,f.press_token,true,angle,false,now)){
            if(!motion::bind(f,now)){restore_at(__LINE__,f,game);return true;}
            clear_pending_start();moving=true;held=d.lease;last_intent=f.press_token;stage=closing?Stage::closing:Stage::opening;
            boltok_sound::event(f,closing?3u:0u,f.press_token);
        }
    }else if(moving&&!grip_closing&&f.controllers.sequence>control.source.controllers.sequence){
        const bool has_grant=granted(f,d);
        const bool wait=has_grant&&hand::same(held,d.lease)&&motion::can_wait_for_pose(f);
        double echo_angle{};
        if(wait&&!gears_weapon::boltok_swing_angle(parts.in_root[0].orientation,closed_support,echo_angle)){
            restore_at(__LINE__,f,game);return true;
        }
        if(wait){
            const auto old_wanted=angle;
            auto echo=control;echo.source=f;echo.angle=echo_angle;echo.hide_cylinder=parts.original_cylinder_hidden;
            cost.mark(6);
            api::Receipt r{};const auto result=call(&echo,&r);cost.mark(7);last_update_result=result;
            if((result!=api::Result::ready&&result!=api::Result::pending)||r.size!=sizeof r||r.version!=1||
               r.handle!=control.handle||r.result!=result||r.hidden!=echo.hide_cylinder||!std::isfinite(r.angle)||
               std::abs(r.angle-echo_angle)>1e-6){restore_at(__LINE__,f,game);return true;}
            control.source=f;native_receipt=r;
            if(!parts.original_cylinder_hidden&&!boltok_visual::withdraw()){
                restore_at(__LINE__,f,game);return true;
            }
            if(log_file){static unsigned rows{};if(rows++<8){
                fwprintf(log_file,L"boltok_motion_wait frame=%llu sequence=%llu prior_sequence=%llu echo_angle=%.9g old_wanted=%.9g result=%u no_motion_advance=1\n",
                    f.key.frame,f.controllers.sequence,motion::previous.raw.input.sequence,echo_angle,old_wanted,unsigned(result));fflush(log_file);}}
            return true;
        }
        gap_trace.record(f,f.press_token,moving,
            control.source.controllers.sequence,motion::previous.raw.input.sequence,
            unsigned(measure_failure),parts_measured,parts.native_observed,true,
            motion::current.gate,motion::input.matched);
        const bool continued=has_grant&&(stroke.original_motion_bound()?motion::advance(f,s,local,now):
            stroke.move(s,local,f.press_token,true,now));
        if(!continued){if(closing)perfect_window.reset();cost.force=true;gap_trace.dump(log_file);report_active_failure(has_grant?4u:3u,f,d);moving=false;held={};clear_completed_grasp();stroke.reset();}
        else angle=stroke.wanted_angle();
    }
    // Publish the exact calibrated endpoint while the real grasp still owns
    // the existing stroke's endpoint tolerance. Completion waits for Native's
    // evaluated result; a residual angle must not leave a closed gun fire-locked.
    if(moving&&granted(f,d)){
        if(closing&&angle<=.02)angle=0;
        else if(!closing&&angle>=hinge.open_angle-.02)angle=hinge.open_angle;
    }
    cost.mark(6);control.source=f;control.angle=angle;
    // The owner's old cylinder remains the original native part during the
    // opening arc. Hide it at the open endpoint; no compound redraw per step.
    if(moving&&boltok_visual::ready&&inventory_known&&
       (closing||angle>=hinge.open_angle-.02))control.hide_cylinder=true;
    api::Receipt r{};const auto result=call(&control,&r);cost.mark(7);
    last_update_result=result;
    if((result!=api::Result::ready&&result!=api::Result::pending)||r.size!=sizeof r||r.version!=1||
       r.handle!=control.handle||r.result!=result||r.hidden!=control.hide_cylinder||!std::isfinite(r.angle)||
       std::abs(r.angle-control.angle)>1e-6){restore_at(__LINE__,f,game);return true;}
    native_receipt=r;
    if(grip_closing&&!grip_close.dispatched()){
        if((!auto_close.armed&&(!granted(f,d)||!hand::same(held,d.lease)))||control.angle!=0||
           !grip_close.dispatch(s,f.press_token,weaponhand::clock_ns(nullptr))){restore_at(__LINE__,f,game);return true;}
        if(log_file){fwprintf(log_file,L"boltok_grip_close policy=%ls dispatch=1 frame=%llu sequence=%llu token=%llu serial=%llu native_result=%u dispatch_ns=%lld actual_endpoint_pending=1\n",
            auto_close.armed?L"insert_ack":L"grip_once",f.key.frame,f.controllers.sequence,f.press_token,reload_serial,unsigned(result),grip_close.dispatch_ns());fflush(log_file);}
    }
    const bool shown=[&](){
        return boltok_visual::show(f,parts,live_mask,parts.original_cylinder_hidden&&r.hidden,
            transaction.inserted()>0||(!closing&&angle<hinge.open_angle-.02));
    }();cost.mark(8);
    if(!shown){cost.force=true;restore_at(__LINE__,f,game);return true;}
    if(moving&&result==api::Result::ready){
        double measured{};
        if(gears_weapon::boltok_swing_angle(parts.in_root[0].orientation,closed_support,measured)&&
           (grip_closing?grip_close.acknowledge(s,f.press_token,weaponhand::clock_ns(nullptr),
                parts.observed_ns,measured,parts.native_observed):stroke.acknowledge(s,measured,now))){
            if(grip_closing&&log_file){fwprintf(log_file,L"boltok_grip_close policy=grip_once actual_endpoint_ack=1 frame=%llu sequence=%llu token=%llu serial=%llu angle=%.9g observed_ns=%lld dispatch_ns=%lld\n",
                f.key.frame,f.controllers.sequence,f.press_token,reload_serial,measured,parts.observed_ns,grip_close.dispatch_ns());fflush(log_file);}
            if(grip_closing&&!auto_close.armed)(void)close_release::cancel(f,parts.observed_ns);
            close_release::reset();grip_closing=false;
            stage=closing?Stage::idle:Stage::opened;moving=false;
            completed_lease=held;completed_identity={f.reference_generation,f.key.world_generation,
                f.key.lifecycle,f.weapon,parts.source.component};
            completed_known=granted(f,d)&&hand::same(held,d.lease);
            held={};
            if(closing&&!finish_ammo(f)){restore_at(__LINE__,f,game);return true;}
            boltok_sound::event(f,closing?4u:1u,last_intent);
            // The six-hole layout belongs to this actual transaction, not
            // to the aggregate loaded count. Expose its existing mask only
            // after the original current Native/bone endpoint ACK so a later
            // input planner never guesses an empty hole from ammunition totals.
            if(log_file){fwprintf(log_file,L"boltok_mechanism endpoint=%u angle=%.9g actual=%.9g inventory_changed=0 physical_headset_pending=1 round_mask_known=%u loaded_mask=%u empty_mask=%u frame=%llu sequence=%llu token=%llu weapon=%llx handle=%llu\n",
                unsigned(stage),angle,measured,unsigned(mask_known),live_mask,(~live_mask)&63u,
                f.key.frame,f.controllers.sequence,f.press_token,f.weapon.address,control.handle);fflush(log_file);}
        }
    }
    return true;
}
bool pending_work() noexcept {return outstanding();}
// Scheduling only. tick performs the sole current Frame read and checks the
// exact pending lease there, followed by all existing C0/contact/write guards.
bool pre_tick(uptr pc) noexcept {
    const bool admitted=pending_start.known;
    const bool active=cycle||moving||round_item||stage!=Stage::idle;
    if(!admitted&&!active)return false;
    if(!manual_enabled()||!outstanding()||
       !weaponhand::current_gt(nullptr)||
       !nativeequipment::domain_ready()||
       !nativelancerfollow::bound_key_known||
       !playground::instance(nativelancerfollow::bound_key.weapon.address,
           L"Weap_LocustPistol")){
        if(pending_start.known)clear_pending_start();
        return false;
    }
    return tick(pc);
}

// Service an existing old owner's completion/restoration only. Its pending
// result must not select or block another weapon's current candidate.
void service_unselected(uptr pc,uptr selected) noexcept {
    if(outstanding()&&identity.weapon.address!=selected)tick(pc);
}
} // namespace boltok_runtime
