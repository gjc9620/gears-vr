// Private, explicitly opted-in preview. Included after cover_neutral_eye_live.
// prepare/print run under stereo_mutex; progress takes that same mutex. The
// Scene caller already owns its one-use Receipt and admitted game-thread Scope.
#include "../vr-common/cover_view.h"

namespace cover_camera_live {
namespace neutral=cover_neutral_eye_live;
enum class Reason:unsigned {disabled,applied,neutral_provider,facing_wait,facing_guard,matrix,exception,count};
const char* reason_name(Reason reason){static constexpr const char* names[]={"disabled","applied","neutral_provider","facing_wait","facing_guard","matrix","exception"};
    const auto i=unsigned(reason);return i<std::size(names)?names[i]:"unknown";}
constexpr unsigned neutral_reason_count=unsigned(neutral::Reason::exception)+1;
bool enabled{};
struct Facing {
    bool ready{};neutral::EyeSample key{};gears_vr::Vector forward{},right{};
    std::array<int32_t,3> rotation{};float original_eye_height{};
} facing;
struct CalibrationRow {
    uint64_t invocation{},world{},calibrated_ms{};uintptr_t pawn{},mesh{},link{};int slot{},head_index{};
    gears_vr::Vector neutral_eye{},neutral_offset{},forward{};int32_t pawn_yaw{};float original_eye_height{};
};
struct RejectionRow {Reason reason{};neutral::Reason neutral_reason{};uint64_t invocation{};
    const char* stage="none";const wchar_t* field=L"none";};
struct Diagnostics {
    unsigned long long attempts{},provider_ready{},calibrating{},rejected{},applied{},bindings{},calibration_dropped{};
    std::array<unsigned long long,neutral_reason_count> neutral_counts{};
    std::array<bool,unsigned(Reason::count)> seen{};std::array<bool,neutral_reason_count> neutral_seen{};
    CalibrationRow latest_calibration{};
    std::array<CalibrationRow,8> calibrations{};unsigned calibration_count{};
    std::array<RejectionRow,unsigned(Reason::count)+neutral_reason_count> rejections{};unsigned rejection_count{};
    Reason last=Reason::disabled;neutral::Reason last_neutral=neutral::Reason::body_unavailable;
    const char* stage="not_sampled";const wchar_t* field=L"none";
    gears_vr::Vector latest_base_eye{},latest_matrix_eye{};uint64_t latest_invocation{};
} diagnostics;
bool same_key(neutral::EyeSample const& a,neutral::EyeSample const& b){return
    gears_anchor::equal_identity(a.pawn,b.pawn)&&gears_anchor::equal_identity(a.mesh,b.mesh)&&gears_anchor::equal_identity(a.link,b.link)&&
    a.current_world==b.current_world&&a.world_generation==b.world_generation&&a.slot==b.slot&&a.calibrated_ms==b.calibrated_ms&&
    a.head_name==b.head_name&&a.head_index==b.head_index&&a.neutral_offset.x==b.neutral_offset.x&&
    a.neutral_offset.y==b.neutral_offset.y&&a.neutral_offset.z==b.neutral_offset.z;}
bool reject(Reason reason,gears_anchor::Receipt const* scene){
    diagnostics.last=reason;++diagnostics.rejected;
    bool first=!diagnostics.seen[unsigned(reason)];diagnostics.seen[unsigned(reason)]=true;
    if(reason==Reason::neutral_provider){const auto i=unsigned(diagnostics.last_neutral);
        if(i<neutral_reason_count){first=first||!diagnostics.neutral_seen[i];diagnostics.neutral_seen[i]=true;}}
    if(first&&diagnostics.rejection_count<diagnostics.rejections.size())diagnostics.rejections[diagnostics.rejection_count++]={
        reason,diagnostics.last_neutral,scene?scene->invocation:0,diagnostics.stage,diagnostics.field};
    return false;
}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
bool matches(neutral::Context const& context,neutral::EyeSample const& eye){return
    gears_anchor::equal_identity(context.binding.pawn,eye.pawn)&&gears_anchor::equal_identity(context.mesh,eye.mesh)&&
    gears_anchor::equal_identity(context.link,eye.link)&&context.world.address==eye.current_world&&context.generation==eye.world_generation&&
    context.slot==eye.slot&&context.ct==eye.native_ct&&context.ca==eye.native_ca&&context.sm==eye.special_move&&context.last==eye.last_cover_action&&
    ((context.transition_word^eye.transition_word)&eye.transition_mask)==0&&context.location.x==eye.pawn_location.x&&
    context.location.y==eye.pawn_location.y&&context.location.z==eye.pawn_location.z;}
Reason initial_facing(gears_anchor::Receipt const* scene,neutral::EyeSample const& eye,Facing& out){
    namespace body=cover_body_live;
    std::lock_guard<std::mutex> lock(body::mutex);
    if(!scene||scene->thread!=GetCurrentThreadId()||eye.scene_invocation!=scene->invocation||!neutral::body_ready()||!neutral::proof_current())return Reason::facing_guard;
    neutral::Context before{},after{};
    if(!neutral::context(*scene,before)||!matches(before,eye))return Reason::facing_guard;
    // A cached neutral position is not sufficient to take a new facing while
    // native LT/special-move animation is already moving the character.
    if(!neutral::neutral(before))return Reason::facing_wait;
    const unsigned offset=anchor_config.pawn_rotation;
    if(!offset||offset>0x10000-sizeof(std::array<int32_t,3>)||offset!=body::metadata.pawn_rotation||
       anchor_config.objects_header!=body::metadata.objects_header||anchor_config.names_header!=body::metadata.names_header)return Reason::facing_guard;
    std::array<int32_t,3> rotation{},again{};
    if(!body::field(rotation,eye.pawn.address,offset)||!neutral::context(*scene,after)||!neutral::same_values(before,after)||
       !neutral::body_ready()||!neutral::proof_current()||!body::field(again,eye.pawn.address,offset)||rotation!=again||
       anchor_config.pawn_rotation!=offset||!neutral::neutral(after))return Reason::facing_guard;
    // Preview hypothesis only: retail neutral Pawn faces away from the cover.
    // No claim is made about mesh-body facing, reach/collision or weapon aim.
    constexpr double tau=6.283185307179586476925286766559;
    const double yaw=std::remainder(double(rotation[1])*tau/65536.+tau/2,tau);
    Facing candidate{};candidate.ready=true;candidate.key=eye;candidate.rotation=rotation;
    candidate.forward={std::cos(yaw),std::sin(yaw),0};candidate.right={-candidate.forward.y,candidate.forward.x,0};
    candidate.original_eye_height=scene->anchor.eye_height;out=candidate;return Reason::applied;
}
#else
Reason initial_facing(gears_anchor::Receipt const*,neutral::EyeSample const&,Facing&){return Reason::facing_guard;}
#endif
template<class Sample> bool prepare_with(gears_anchor::Receipt const* scene,gears_vr::StereoMatrix& view,Sample sample){
    if(!enabled)return false;++diagnostics.attempts;
    try{
        neutral::EyeSample eye{};
        if(!sample(scene,eye)){
            diagnostics.last_neutral=neutral::last_reason;diagnostics.stage=neutral::context_stage;diagnostics.field=neutral::rejected_field;
            const auto i=unsigned(diagnostics.last_neutral);if(i<neutral_reason_count)++diagnostics.neutral_counts[i];
            if(diagnostics.last_neutral==neutral::Reason::neutral_wait||diagnostics.last_neutral==neutral::Reason::layout_wait||
               diagnostics.last_neutral==neutral::Reason::query_wait)++diagnostics.calibrating;
            // Do not borrow an old view on a rejected/unknown world context.
            return reject(Reason::neutral_provider,scene);
        }
        ++diagnostics.provider_ready;diagnostics.last_neutral=neutral::Reason::ready;diagnostics.stage=neutral::context_stage;diagnostics.field=neutral::rejected_field;
        if(!facing.ready||!same_key(facing.key,eye)){
            facing.ready=false;Facing candidate{};const auto reason=initial_facing(scene,eye,candidate);
            if(reason!=Reason::applied)return reject(reason,scene);
            facing=candidate;++diagnostics.bindings;
            CalibrationRow row{eye.scene_invocation,eye.world_generation,eye.calibrated_ms,eye.pawn.address,eye.mesh.address,eye.link.address,eye.slot,eye.head_index,
                eye.neutral_eye_world,eye.neutral_offset,facing.forward,facing.rotation[1],facing.original_eye_height};
            diagnostics.latest_calibration=row;
            if(diagnostics.calibration_count<diagnostics.calibrations.size())diagnostics.calibrations[diagnostics.calibration_count++]=row;
            else ++diagnostics.calibration_dropped;
        }
        gears_vr::CoverViewResult result{};
        // Physical tracking is deliberately absent here. The one frozen stereo
        // ticket applies the reference/head/eye mapping after this base view.
        if(!gears_vr::prepare_cover_view({eye.neutral_eye_world,facing.forward,facing.right},{0,0},stereo_units_per_metre,result))return reject(Reason::matrix,scene);
        view=result.base_view;diagnostics.latest_base_eye=result.base_eye_world;
        diagnostics.latest_matrix_eye=gears_vr::stereo_camera_position(view);diagnostics.latest_invocation=eye.scene_invocation;
        ++diagnostics.applied;diagnostics.last=Reason::applied;return true;
    }catch(...){return reject(Reason::exception,scene);}
}
bool prepare(gears_anchor::Receipt const* scene,gears_vr::StereoMatrix& view){return prepare_with(scene,view,neutral::sample);}
bool configure(const wchar_t* ini,FILE* file){
    const unsigned request=GetPrivateProfileIntW(L"CoverCameraPreview",L"enabled",0,ini);
    if(!request)return true;
    if(request!=1||enabled||!continuous_session||!stereo_xr||!cover_body_live::enabled)return false;
#if !defined(GEARS_COVER_BODY_PROTOTYPE) || !GEARS_COVER_BODY_PROTOTYPE
    (void)file;return false;
#else
    enabled=true;
    fprintf(file,"cover_camera_config enabled=1 experimental=1 neutral_wait_ms=100 virtual_offset_right=0 virtual_offset_up=0 facing_policy=neutral_pawn_yaw_plus_pi mesh_body_facing_verified=0 collision_verified=0 aim_modified=0 automatic_lt_eye=0 hmd_application=stereo_ticket_only entry_transition_acceptance=0 gameplay_acceptance=0\n");
    return true;
#endif
}
void summary(FILE* file){
    fprintf(file,"cover_camera_enabled=%u cover_camera_ready=%u cover_camera_attempts=%llu cover_camera_provider_ready=%llu cover_camera_calibrating=%llu cover_camera_rejected=%llu cover_camera_applied=%llu cover_camera_bindings=%llu cover_camera_last_reason=%s cover_camera_neutral_reason=%s cover_camera_context_stage=%s cover_camera_rejected_field=%ls ",
        unsigned(enabled),unsigned(facing.ready&&diagnostics.last==Reason::applied),diagnostics.attempts,diagnostics.provider_ready,diagnostics.calibrating,diagnostics.rejected,diagnostics.applied,diagnostics.bindings,
        reason_name(diagnostics.last),neutral::reason_name(diagnostics.last_neutral),diagnostics.stage,diagnostics.field);
    if(diagnostics.applied)fprintf(file,"cover_camera_base_origin=%.9g,%.9g,%.9g cover_camera_matrix_origin=%.9g,%.9g,%.9g cover_camera_applied_invocation=%llu ",
        diagnostics.latest_base_eye.x,diagnostics.latest_base_eye.y,diagnostics.latest_base_eye.z,
        diagnostics.latest_matrix_eye.x,diagnostics.latest_matrix_eye.y,diagnostics.latest_matrix_eye.z,
        static_cast<unsigned long long>(diagnostics.latest_invocation));
    if(diagnostics.bindings){auto const& row=diagnostics.latest_calibration;
        fprintf(file,"cover_camera_neutral_origin=%.9g,%.9g,%.9g cover_camera_fixed_forward=%.9g,%.9g,%.9g cover_camera_original_eye_height=%.9g cover_camera_head_index=%d cover_camera_world=%llu cover_camera_slot=%d ",
            row.neutral_eye.x,row.neutral_eye.y,row.neutral_eye.z,row.forward.x,row.forward.y,row.forward.z,row.original_eye_height,row.head_index,static_cast<unsigned long long>(row.world),row.slot);}
}
void progress(FILE* file){std::lock_guard<std::mutex> lock(stereo_mutex);summary(file);}
void print(FILE* file){
    summary(file);fprintf(file,"\ncover_camera_policy experimental=1 facing_policy=neutral_pawn_yaw_plus_pi mesh_body_facing_verified=0 collision_verified=0 aim_modified=0 virtual_offset_right=0 virtual_offset_up=0 hmd_application=stereo_ticket_only entry_transition_acceptance=0 gameplay_acceptance=0 calibrations_retained=%u calibration_dropped=%llu\n",diagnostics.calibration_count,diagnostics.calibration_dropped);
    for(unsigned i=0;i<diagnostics.calibration_count;++i){auto const& row=diagnostics.calibrations[i];
        fprintf(file,"cover_camera_neutral invocation=%llu world=%llu calibrated_ms=%llu pawn=0x%llx mesh=0x%llx link=0x%llx slot=%d head_index=%d origin=%.17g,%.17g,%.17g offset=%.17g,%.17g,%.17g forward=%.17g,%.17g,%.17g pawn_yaw=%d original_eye_height=%.9g\n",
            static_cast<unsigned long long>(row.invocation),static_cast<unsigned long long>(row.world),static_cast<unsigned long long>(row.calibrated_ms),
            static_cast<unsigned long long>(row.pawn),static_cast<unsigned long long>(row.mesh),static_cast<unsigned long long>(row.link),row.slot,row.head_index,
            row.neutral_eye.x,row.neutral_eye.y,row.neutral_eye.z,row.neutral_offset.x,row.neutral_offset.y,row.neutral_offset.z,row.forward.x,row.forward.y,row.forward.z,row.pawn_yaw,row.original_eye_height);}
    for(unsigned i=0;i<diagnostics.rejection_count;++i){auto const& row=diagnostics.rejections[i];
        fprintf(file,"cover_camera_rejection reason=%s neutral_reason=%s first_invocation=%llu context_stage=%s field=%ls\n",reason_name(row.reason),neutral::reason_name(row.neutral_reason),static_cast<unsigned long long>(row.invocation),row.stage,row.field);}
    for(unsigned i=0;i<neutral_reason_count;++i)if(diagnostics.neutral_counts[i])fprintf(file,"cover_camera_neutral_rejection reason=%s count=%llu\n",neutral::reason_name(neutral::Reason(i)),diagnostics.neutral_counts[i]);
}
} // namespace cover_camera_live
