// Normal PG GT preparation only. No Native registration/ABI is added here.
// Missing current C0 geometry is NOT replaced with an assumed Marcus/scale.
namespace nativeequipment_main_mount {
namespace policy=::gears_weapon_main_mount;
namespace hand=::gears_weapon_hand;
namespace source=::gears_weapon_main_mount_source;
struct Diagnostic {
    lancermain_asset::Status asset_status{lancermain_asset::Status::invalid_identity};
    policy::AssetBinding asset{};
    std::uint64_t hold_generation{};bool held{},asset_attempted{};
    std::int64_t observed_ns{},elapsed_ns{};
};
// Private synchronous continuation. game is the actual observation returned
// with this call's Frame, not a cached witness or a new source permission.
template<bool enclosed_attachment=false>
policy::Status prepare_observed(hand::Frame const& frame,policy::BodyBinding const& body,
    hand::GripGameState const& game,policy::Candidate& out,Diagnostic& diagnostic,
    std::int64_t started) noexcept {
    out={};diagnostic={};
    diagnostic.observed_ns=started;
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
       !weaponhand::client.armed()||!weaponhand::client.registration_alive(
           frame.key.registration_cookie,frame.key.native_started_qpc))return policy::Status::inactive;
    const auto generation=vrequip::model.hold_generation();
    diagnostic.hold_generation=generation;diagnostic.held=game.main_hand_held;
    const auto allowed=policy::eligibility(frame,game,generation,nativeequipment::clock_ns(nullptr));
    if(allowed!=policy::Status::ready)return allowed;
    lancermain_asset::Evidence evidence{};diagnostic.asset_attempted=true;
    diagnostic.asset_status=lancermain_asset::read(frame,evidence);
    if(diagnostic.asset_status!=lancermain_asset::Status::ready)return policy::Status::unsupported_asset;
    if constexpr(enclosed_attachment){
        // The actual copy-frame observation above and independent final
        // source_game below enclose all asset/attachment reads. Do not repeat
        // two complete Game observations inside this metadata-only read.
        const auto current=[&]() noexcept {
            return weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready()&&
                weaponhand::client.armed()&&weaponhand::client.registration_alive(
                    frame.key.registration_cookie,frame.key.native_started_qpc)&&
                hand::frame_active(frame,nativeequipment::clock_ns(nullptr));
        };
        if(!current()||!nativeequipment::read_attached_frame(frame,evidence.component.address)||!current())
            return policy::Status::not_attached;
    }else if(!nativeequipment::attached_frame(frame,evidence.component.address))return policy::Status::not_attached;
    hand::GripGameState after{};
    if(!weaponhand::client.source_game_on_gt(frame.key,after)||!hand::same_game(game,after)||
       vrequip::model.hold_generation()!=generation)return policy::Status::inactive;
    lancermain_asset::Evidence final_asset{};
    if(lancermain_asset::read(frame,final_asset)!=lancermain_asset::Status::ready||
       !hand::same(evidence.component,final_asset.component)||!hand::same(evidence.asset,final_asset.asset))
        return policy::Status::unsupported_asset;
    if(!weaponhand::client.armed()||!weaponhand::client.registration_alive(frame.key.registration_cookie,frame.key.native_started_qpc)||
       vrequip::model.hold_generation()!=generation)return policy::Status::inactive;
    const auto finished=nativeequipment::clock_ns(nullptr);
    if(policy::eligibility(frame,after,generation,finished)!=policy::Status::ready)return policy::Status::inactive;
    policy::AssetBinding asset{};asset.certified=asset.right_attached=true;asset.key=frame.key;
    asset.weapon=frame.weapon;asset.component=evidence.component;asset.asset=evidence.asset;
    diagnostic.asset=asset;diagnostic.observed_ns=finished;diagnostic.elapsed_ns=finished-started;
    return policy::prepare(frame,after,generation,asset,body,finished,out);
}
policy::Status prepare(hand::Frame const& frame,policy::BodyBinding const& body,
    policy::Candidate& out,Diagnostic& diagnostic) noexcept {
    out={};diagnostic={};const auto started=nativeequipment::clock_ns(nullptr);
    diagnostic.observed_ns=started;hand::GripGameState game{};
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
       !weaponhand::client.source_game_on_gt(frame.key,game))return policy::Status::inactive;
    return prepare_observed(frame,body,game,out,diagnostic,started);
}
struct RetainedDiagnostic {
    enum class Stage:unsigned {entry,copy_frame,initial_bind,prepare,final_bind,complete};
    source::SourceStatus binding{source::SourceStatus::source_mismatch};
    gears_equipment_rig::Status agreement{gears_equipment_rig::Status::inactive_hand};
    Diagnostic mount{};Stage stage{};
};
policy::Status source_rejection(source::SourceStatus value) noexcept {
    switch(value){
    case source::SourceStatus::body_unavailable:return policy::Status::body_unavailable;
    case source::SourceStatus::body_mismatch:return policy::Status::body_mismatch;
    case source::SourceStatus::unsupported_geometry:return policy::Status::unsupported_geometry;
    default:return policy::Status::inactive;
    }
}
// PG-local sender entry, not an export or new Native callback. The caller must
// hold the existing PG callback lifetime lease. A future Native invocation must
// first gain explicit admission/drain through the shared broker; a module pin
// or this payload alone is insufficient. Native alone certifies C0 geometry.
// The retained Scene source is data, never a C0 counter turned into a Frame.
[[maybe_unused]] policy::Status prepare_retained(gears_rig_tracking::Frame const& retained,
    source::BodyContext const& native_body,policy::Candidate& out,RetainedDiagnostic& diagnostic) noexcept {
    out={};diagnostic={};
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready())return policy::Status::inactive;
    // Freeze caller-owned data once; do not reread a mutable publication or
    // manufacture/retimestamp a hand Frame if this exact Scene is unavailable.
    const auto rig=retained;const auto context=native_body;hand::Frame frame{};hand::GripGameState game{};
    diagnostic.stage=RetainedDiagnostic::Stage::copy_frame;
    if(!weaponhand::client.copy_frame_on_gt(frame,game))return policy::Status::inactive;
    policy::BodyBinding body{};
    const auto expected=static_cast<std::uint32_t>(frame.stereo.source);
    diagnostic.stage=RetainedDiagnostic::Stage::initial_bind;
    diagnostic.binding=source::bind(frame,rig,context,nativeequipment::clock_ns(nullptr),expected,body,diagnostic.agreement);
    if(diagnostic.binding!=source::SourceStatus::ready)return source_rejection(diagnostic.binding);
    diagnostic.stage=RetainedDiagnostic::Stage::prepare;
    const auto result=prepare_observed<true>(frame,body,game,out,diagnostic.mount,nativeequipment::clock_ns(nullptr));
    if(result!=policy::Status::ready)return result;
    policy::BodyBinding final_body{};
    diagnostic.stage=RetainedDiagnostic::Stage::final_bind;
    diagnostic.binding=source::bind(frame,rig,context,nativeequipment::clock_ns(nullptr),expected,final_body,diagnostic.agreement);
    if(diagnostic.binding!=source::SourceStatus::ready){out={};return source_rejection(diagnostic.binding);}
    diagnostic.stage=RetainedDiagnostic::Stage::complete;return result;
}
const char* status_name(policy::Status value) noexcept {
    switch(value){
    case policy::Status::ready:return "ready";case policy::Status::inactive:return "inactive";
    case policy::Status::no_weapon:return "no_weapon";case policy::Status::unheld:return "unheld";
    case policy::Status::animation:return "animation";case policy::Status::busy:return "busy";
    case policy::Status::not_firearm:return "not_firearm";case policy::Status::unsupported_asset:return "unsupported_asset";
    case policy::Status::not_attached:return "not_attached";case policy::Status::body_unavailable:return "body_unavailable";
    case policy::Status::body_mismatch:return "body_mismatch";case policy::Status::unsupported_geometry:return "unsupported_geometry";
    }
    return "unknown";
}
unsigned logs{};bool have_status{};policy::Status last_status{};
std::uint64_t certified_count{},rejected_count{},last_weapon{};
// Diagnostic budget only. No result, geometry, calibration or pose survives
// a query. prepare() above remains a fresh full read on every actual request.
hand::FrameKey preview_key{};std::array<hand::ObjectIdentity,5> preview_objects{};
std::uint64_t preview_generation{};unsigned preview_attempts{},preview_flags{};
bool preview_known{},preview_complete{},preview_switching{},preview_animation{};
ULONGLONG preview_after{};
bool preview_admitted(hand::Frame const& frame) noexcept {
    const auto& k=frame.key;const auto& p=preview_key;
    const auto generation=vrequip::model.hold_generation();
    const bool switching=vrequip::model.switch_pending(),animation=vrequip::model.animation_owns_hands();
    const std::array<hand::ObjectIdentity,5> objects{frame.world,frame.controller,frame.pawn,frame.mesh,frame.weapon};
    bool same=preview_known&&k.game_start==p.game_start&&k.native_started_qpc==p.native_started_qpc&&
        k.registration_cookie==p.registration_cookie&&k.lifecycle==p.lifecycle&&k.world_generation==p.world_generation&&
        k.game_pid==p.game_pid&&k.thread_id==p.thread_id&&generation==preview_generation&&
        switching==preview_switching&&animation==preview_animation&&frame.flags==preview_flags;
    for(unsigned i=0;i<objects.size();++i)same=same&&hand::same(objects[i],preview_objects[i]);
    if(!same){
        preview_key=k;preview_objects=objects;preview_generation=generation;preview_switching=switching;
        preview_animation=animation;preview_flags=frame.flags;preview_attempts=0;preview_after=0;
        preview_known=true;preview_complete=false;
    }
    const auto now=GetTickCount64();
    if(preview_complete||preview_attempts>=8||now<preview_after)return false;
    ++preview_attempts;preview_after=now+100;return true;
}
void tick(hand::Frame const& frame) noexcept {
    // Preview deliberately supplies no C0 binding. The shared Native owner
    // will supply its actual geometry/basis in the agreed supplementary ABI.
    if(!preview_admitted(frame))return;
    policy::Candidate candidate{};Diagnostic diagnostic{};
    const auto status=prepare(frame,policy::BodyBinding{},candidate,diagnostic);
    preview_complete=diagnostic.asset.certified||status==policy::Status::unheld||status==policy::Status::animation||
        status==policy::Status::not_firearm||
        (status==policy::Status::unsupported_asset&&diagnostic.asset_status==lancermain_asset::Status::unsupported_weapon);
    if(diagnostic.asset.certified)++certified_count;else ++rejected_count;
    if(log_file&&logs<24&&(!have_status||status!=last_status||last_weapon!=frame.weapon.address||
        (diagnostic.asset.certified&&certified_count<=6))){
        fwprintf(log_file,L"native_equipment_main_mount status=%hs asset_status=%u asset_attempted=%u asset_certified=%u right_attached=%u held=%u hold_generation=%llu candidate=%u body_binding=0 required_basis=2 required_basis_revision=2 frame=%llu cookie=%llu native_started=%llu world_generation=%llu pid=%u game_start=%llu weapon=%llx component=%llx asset=%llx observed_ns=%lld input_received_ns=%lld input_sync_ns=%lld elapsed_ns=%lld\n",
            status_name(status),unsigned(diagnostic.asset_status),unsigned(diagnostic.asset_attempted),unsigned(diagnostic.asset.certified),
            unsigned(diagnostic.asset.right_attached),unsigned(diagnostic.held),diagnostic.hold_generation,unsigned(candidate.right.known),
            frame.key.frame,frame.key.registration_cookie,frame.key.native_started_qpc,frame.key.world_generation,
            frame.key.game_pid,frame.key.game_start,frame.weapon.address,diagnostic.asset.component.address,diagnostic.asset.asset.address,
            diagnostic.observed_ns,frame.controllers.received_ns,frame.controllers.sync_ns,diagnostic.elapsed_ns);
        fflush(log_file);++logs;
    }
    have_status=true;last_status=status;last_weapon=frame.weapon.address;
}
void reset_after_drain() noexcept {
    if(log_file&&(certified_count||rejected_count)){
        fwprintf(log_file,L"native_equipment_main_mount_summary certified=%llu rejected=%llu\n",certified_count,rejected_count);fflush(log_file);
    }
    logs=0;have_status=false;last_status={};certified_count=rejected_count=last_weapon=0;
    preview_key={};preview_objects={};preview_generation=0;preview_attempts=preview_flags=0;
    preview_known=preview_complete=preview_switching=preview_animation=false;preview_after=0;
}
} // namespace nativeequipment_main_mount
