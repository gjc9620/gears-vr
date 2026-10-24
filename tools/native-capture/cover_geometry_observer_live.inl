// Diagnostic prerequisite for real cover edges. Called on the admitted Scene
// GT under stereo_mutex. Neither a view, grab lease nor body target is changed.
namespace cover_geometry_observer_live {
namespace geometry=cover_geometry_live;
bool enabled{};
constexpr unsigned sample_limit=16;
struct Row {
    uint64_t frame{},world{};uintptr_t pawn{},link{};int slot=-1;
    unsigned thread{},ct{},ca{};gears_vr::Vector native{},candidate{};
    geometry::Rotator rotation{};double candidate_error{};
};
std::array<Row,sample_limit> rows{};unsigned row_count{};
uint64_t last_frame{},attempts{},accepted{},rejected{},last_ms{};
bool attempted{};
const char* reason="disabled";
bool configure(wchar_t const* ini,FILE* file){
    const auto request=GetPrivateProfileIntW(L"CoverGeometryObserver",L"enabled",0,ini);
    bool supported=false;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
    supported=continuous_session&&stereo_xr&&anchor_observer_enabled&&cover_body_live::enabled;
#endif
    if(request>1||(request&&!supported))return false;
    enabled=request==1;reason=enabled?"waiting_for_scene":"disabled";
    fprintf(file,"cover_geometry_observer enabled=%u read_only=1 maximum_samples=%u minimum_interval_ms=100 collision_verified=0 grab_authorized=0 view_modified=0\n",unsigned(enabled),sample_limit);
    return true;
}
void observe(gears_anchor::Receipt const* scene){
    if(!enabled||row_count==sample_limit||!scene||!scene->invocation||scene->invocation<=last_frame)return;
    const auto time=GetTickCount64();
    if(attempted&&(time<last_ms||time-last_ms<100))return;
    attempted=true;last_ms=time;last_frame=scene->invocation;++attempts;
    cover_neutral_eye_live::EyeSample eye{};geometry::Geometry value{};
    if(!cover_neutral_eye_live::sample(scene,eye)){
        ++rejected;reason=cover_neutral_eye_live::reason_name(cover_neutral_eye_live::last_reason);return;
    }
    if(!geometry::sample_native_with(scene,eye,value)){
        ++rejected;reason=geometry::reason_name(geometry::last_reason);return;
    }
    auto const& native=value.native_slot_world;auto const& candidate=value.candidates.slot_world;
    const auto error=std::hypot(std::hypot(native.x-candidate.x,native.y-candidate.y),native.z-candidate.z);
    if(!value.native_getter_verified||!std::isfinite(error)){
        ++rejected;reason="native_output";return;
    }
    rows[row_count++]={scene->invocation,eye.world_generation,eye.pawn.address,eye.link.address,eye.slot,
        scene->thread,eye.native_ct,eye.native_ca,native,candidate,value.native_slot_rotation,error};
    ++accepted;reason="ready";
}
void summary(FILE* file){
    fprintf(file,"cover_geometry_observer_enabled=%u cover_geometry_observer_attempts=%llu cover_geometry_observer_accepted=%llu cover_geometry_observer_rejected=%llu cover_geometry_observer_rows=%u cover_geometry_observer_reason=%s cover_geometry_grab_authorized=0 cover_geometry_collision_verified=0 ",
        unsigned(enabled),attempts,accepted,rejected,row_count,reason);
}
void print(FILE* file){
    summary(file);fprintf(file,"\n");
    for(unsigned i=0;i<row_count;++i){auto const& row=rows[i];
        fprintf(file,"cover_geometry_native row=%u frame=%llu world=%llu pawn=%p link=%p slot=%d thread=%u ct=%u ca=%u native_xyz=%.9g,%.9g,%.9g native_rot=%d,%d,%d candidate_xyz=%.9g,%.9g,%.9g candidate_error=%.9g native_getter_verified=1 candidate_only=1 collision_verified=0 atomic_snapshot_verified=0\n",
            i,row.frame,row.world,reinterpret_cast<void*>(row.pawn),reinterpret_cast<void*>(row.link),row.slot,row.thread,row.ct,row.ca,
            row.native.x,row.native.y,row.native.z,row.rotation[0],row.rotation[1],row.rotation[2],row.candidate.x,row.candidate.y,row.candidate.z,row.candidate_error);
    }
}
} // namespace cover_geometry_observer_live
