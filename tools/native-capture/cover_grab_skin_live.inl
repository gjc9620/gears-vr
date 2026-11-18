#include "cover_grab_current_adapter.h"
#include "cover_grab_pose_adapter.h"
#include "../vr-common/cover_grab_profiles.h"
#include "../vr-common/cover_l2_grab_contact.h"
// Cosmetic append to the one existing C0 candidate. No second writer/lease,
// engine call, tracking reacquisition or timestamp replacement is introduced.
namespace cover_grab_skin_live {
bool enabled=false; // Owner deferred matched point skin; basic body/hands delivery first.
namespace body=gears_full_body_c0;namespace vr=gears_vr;
namespace shared=cover_shared_input_live;namespace grab=gears_cover_grab_pose;
namespace skin=gears_cover_grab_pose_adapter;namespace l2=gears_cover_l2_contact;
namespace transition=gears_cover_grab_transition;
enum class Status {inactive,prepared,source,profile,tracking,pose};
struct Workspace {
    gears_cover_grab_receipt::Value owner{};l2::Observation points{};
    cover_l2_grab_live::PointReceipt point_receipt{};
    grab::Profile profile{};grab::Frame input{};grab::Contact contact{};
    gears_cover_grab_profiles::Joints reference{};skin::Workspace skin_work{};
};
bool read(gears_native_rig_tracking::CoreSnapshot const& s,Workspace& work){
    const auto thread=GetCurrentThreadId();
    if(!shared::grab_receipt.on_thread(thread)||s.native.game_thread!=thread||
       !(s.flags&gears_rig_tracking::cover_key_known)||!s.cover_type||s.cover_slot<0||
       !gears_weapon_hand::identity_valid(s.cover_link))return false;
    auto* broker=weapon_hand_runtime::current();if(!broker||broker->action_barrier())return false;
    const auto action=broker->action_generation(),cookie=broker->cookie(),revision=broker->lifecycle_revision();
    cover_neutral_eye_live::EyeSample admitted_eye{};
    if(!shared::grab_receipt.read_if(cover_input_live::participant,thread,[&](shared::GrabKey const& k){
        const auto& t=shared::tracking;
        if(!t.available||!t.matched||!shared::tracking_ticket_current(k.tracking_ticket)||
           k.invocation!=s.scene_invocation||k.thread!=thread||k.scene_qpc!=s.scene_qpc||
           k.lifecycle!=t.lifecycle||k.invocation!=t.invocation||k.tracking_ticket!=s.source_ticket||
           t.ticket!=s.source_ticket||k.action!=action||k.cookie!=cookie||k.revision!=revision||
           !shared::binding::same_key(k.actor,t.actor)||!shared::same_eye(k.eye,t.eye)||
           k.actor.scene_invocation!=s.scene_invocation||k.eye.scene_invocation!=s.scene_invocation||
           k.actor.world_generation!=s.world_generation||
           !gears_weapon_hand::same(shared::object(k.actor.world),s.world)||
           !gears_weapon_hand::same(shared::object(k.actor.controller),s.controller)||
           !gears_weapon_hand::same(shared::object(k.actor.pawn),s.pawn)||
           !gears_weapon_hand::same(shared::object(k.actor.mesh),s.mesh)||
           !gears_weapon_hand::same(shared::object(k.actor.weapon),s.weapon)||
           !gears_weapon_hand::same(shared::object(k.eye.link),s.cover_link)||
           k.eye.slot!=s.cover_slot||k.eye.native_ct!=s.cover_type||
           t.stereo.reference_generation!=s.reference_generation||
           !vr::stereo_same_pose(t.stereo.reference,s.reference)||
           std::memcmp(&t.stereo.sample,&s.stereo,sizeof s.stereo)||
           std::memcmp(&t.hands,&s.controllers,sizeof s.controllers))return false;
        admitted_eye=k.eye;return true;
    },work.owner))return false;
    auto const& o=work.owner.snapshot;
    if(!cover_l2_grab_live::read_points(o.frame,o.cover.epochs,work.point_receipt))return false;
    auto const& p=work.point_receipt;
    if(!shared::same_eye(admitted_eye,p.eye)||p.eye.scene_invocation!=s.scene_invocation||
       p.units_per_metre!=s.units_per_metre||p.references.cover_type!=s.cover_type||
       broker->action_barrier()||broker->action_generation()!=action||broker->cookie()!=cookie||
       broker->lifecycle_revision()!=revision)return false;
    work.points={p.frame,s.cover_link.address,p.observed_ns,p.epochs,p.references,p.wall_anchor,p.units_per_metre};
    return true;
}
Status compose(body::Model const& model,body::Frame const& frame,gears_body_tick_source::Input const& current,
    gears_free_hand_eye_targets::EyeFrame const& eye,std::uint64_t compose,
    std::array<std::uint64_t,2> const& desired,std::int64_t now,bool allow_sim,
    transition::State& next,Workspace& work,body::Output& inout){
    if(!read(current.observed.parent_scene,work)){next={};return Status::source;}
    auto const& o=work.owner.snapshot;grab::Mode mode{};std::uint64_t edge{};
    if(o.ownership.owner==vr::CoverLeftHandOwner::other){next={};return Status::inactive;}
    if(o.hand.hand.lgrip_down&&o.ownership.owner==vr::CoverLeftHandOwner::cover){mode=grab::Mode::held;edge=o.cover.edge.id;}
    else if(!o.hand.hand.lgrip_down&&o.ownership.owner==vr::CoverLeftHandOwner::free&&next.known){mode=grab::Mode::release;edge=next.key.edge;}
    else if(!o.hand.hand.lgrip_down&&o.ownership.owner==vr::CoverLeftHandOwner::free&&o.cover.edge.allowed){mode=grab::Mode::approach;edge=o.cover.edge.id;}
    else {next={};return Status::inactive;}
    if(edge<1||edge>3){next={};return Status::profile;}
    constexpr vr::CoverEdgeKind kinds[]={vr::CoverEdgeKind::left,vr::CoverEdgeKind::right,vr::CoverEdgeKind::top};
    for(unsigned n=0;n<15;++n){auto const& q=model.reference[12+n].q;work.reference[n]={q[0],q[1],q[2],q[3]};}
    if(!gears_cover_grab_profiles::prepare(kinds[edge-1],model.signature,work.reference,work.profile)){
        next={};return Status::profile;}
    const grab::Key key{work.points.epochs,edge,work.points.cover_link,work.points.epochs.cover,work.profile.id};
    if(mode==grab::Mode::release){
        if(!grab::same(key,next.key)){next={};return Status::inactive;}
        work.contact={};work.contact.key=key;work.contact.observed_ns=work.points.observed_ns;
    }else if(l2::prepare(work.points,o,work.profile,now,work.contact)!=l2::Status::prepared){next={};return Status::source;}
    if(gears_cover_grab_current::prepare(current,model.signature,frame.model_scale,o,work.owner.control,
        key,eye,compose,mode,now,allow_sim,work.input)!=gears_cover_grab_current::Status::prepared){
        next={};return Status::tracking;}
    const auto result=skin::compose_transition(model,frame,work.input,work.contact,work.profile,desired,next,work.skin_work,inout);
    if(result.status==skin::Status::prepared)return Status::prepared;
    if(result.status==skin::Status::inactive)return Status::inactive;
    next={};return Status::pose;
}
} // namespace cover_grab_skin_live

