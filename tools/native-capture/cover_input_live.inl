// Experimental GT consumer of the SAME StereoResult later used for both eyes.
// The caller holds stereo_mutex, its one-use Scene Receipt and admitted Scope.
// All game reads are recertified through neutral::context. The bridge reads
// only the current game's authenticated controller-v3 mapping, never input APIs.
namespace cover_input_live {
namespace camera=cover_camera_live;
namespace neutral=cover_neutral_eye_live;
bool enabled{};
gears_vr::ControllerInputBridge controllers;
gears_vr::CoverInputOwner participant;
// The sole GT arbiter. Future weapon participants must submit to this instance;
// a second Playground/render-thread arbiter cannot grant cover's left hand.
gears_vr::LeftHandInteractionOwner left_hand_owner;
std::array<gears_vr::CoverHeightKey,3> height_curve{{{0,0},{.5,0},{1,0}}};
gears_vr::CoverEpochs epochs{};
neutral::EyeSample last_eye{};
std::uint64_t last_session{},last_space{},last_reference{},last_frame{};
ULONGLONG last_open{};
bool attempted_open{},bridge_open{},interrupted=true;
struct Diagnostics {
    unsigned long long attempts{},camera_owned{},rejected{},matched{},missing{},targets{},target_rejected{},bindings{};
    std::array<unsigned long long,12> controller_status{};
    gears_vr::CoverInputOwnerResult latest{};
    gears_vr::Vector hand_world{};
    std::uint64_t sequence{},reference{},frame{};
    bool hand_world_valid{};
} diagnostics;

bool increment(std::uint64_t& value){if(value==(std::numeric_limits<std::uint64_t>::max)())return false;++value;return true;}
bool native_key(neutral::EyeSample const& a,neutral::EyeSample const& b){return
    gears_anchor::equal_identity(a.pawn,b.pawn)&&gears_anchor::equal_identity(a.mesh,b.mesh)&&
    gears_anchor::equal_identity(a.link,b.link)&&a.current_world==b.current_world&&
    a.world_generation==b.world_generation&&a.slot==b.slot&&a.calibrated_ms==b.calibrated_ms;}
bool update_epochs(neutral::EyeSample const& eye,gears_vr::StereoResult const& stereo){
    auto next=epochs;
    const bool first=!next.world;
    if(first||last_eye.current_world!=eye.current_world||last_eye.world_generation!=eye.world_generation)if(!increment(next.world))return false;
    if(first||!gears_anchor::equal_identity(last_eye.pawn,eye.pawn)||!gears_anchor::equal_identity(last_eye.mesh,eye.mesh))if(!increment(next.pawn))return false;
    if(first||!native_key(last_eye,eye))if(!increment(next.cover))return false;
    if(first||!gears_anchor::equal_identity(last_eye.link,eye.link)||last_eye.slot!=eye.slot)if(!increment(next.slot))return false;
    if(first||last_session!=stereo.sample.session)if(!increment(next.session))return false;
    if(first||last_session!=stereo.sample.session||last_space!=stereo.sample.space_epoch)if(!increment(next.space))return false;
    if(first||last_session!=stereo.sample.session||last_space!=stereo.sample.space_epoch||last_reference!=stereo.reference_generation)if(!increment(next.reference))return false;
    if(!first&&next.cover!=epochs.cover)++diagnostics.bindings;
    epochs=next;last_eye=eye;last_session=stereo.sample.session;last_space=stereo.sample.space_epoch;last_reference=stereo.reference_generation;return true;
}
bool matching_input(gears_vr::StereoResult const& stereo,std::int64_t stamp,gears_vr::ControllerInputFrame& out,
    gears_vr::ControllerMatchingRead* diagnostic=nullptr){
    if(diagnostic)*diagnostic={};
    const auto clock=GetTickCount64();
    if(!bridge_open&&(!attempted_open||clock-last_open>=1000)){
        attempted_open=true;last_open=clock;bridge_open=controllers.open(GetCurrentProcessId(),false);
    }
    if(bridge_open&&controllers.read_matching(stereo.sample,stamp,out,simulator_operator,nullptr,diagnostic))return true;
    if(bridge_open){std::uint64_t pid{},start{};if(!controllers.publisher_identity(pid,start)){
        controllers.close();bridge_open=false;
    }}
    return false;
}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
bool publish(gears_anchor::Receipt const* scene,neutral::EyeSample const& eye,float u){
    cover_body_live::LiveTarget target{};
    {
        std::lock_guard<std::mutex> lock(cover_body_live::mutex);
        neutral::Context current{};
        if(!scene||!neutral::body_ready()||!neutral::proof_current()||!neutral::context(*scene,current)||
           !camera::matches(current,eye)||current.sm)return false;
        target.pawn=current.binding.pawn;target.mesh=current.mesh;target.world=current.world;
        target.world_generation=current.generation;target.frame=scene->invocation;
        target.timestamp_qpc=now();target.thread=GetCurrentThreadId();target.u=u;
    }
    // publish_target takes body::mutex itself. Never enter it under that lock.
    return cover_body_live::publish_target(target);
}
#else
bool publish(gears_anchor::Receipt const*,neutral::EyeSample const&,float){return false;}
#endif
bool prepare(gears_anchor::Receipt const* scene,gears_vr::StereoMatrix& view,gears_vr::StereoResult const& stereo){
    if(!enabled)return false;++diagnostics.attempts;
    bool owned=false;
    try{
    neutral::EyeSample eye{};
    // Let the existing preview retain its diagnostics and sample exactly once.
    owned=camera::prepare_with(scene,view,[&](auto const* receipt,auto& out){
        if(!neutral::sample(receipt,eye))return false;out=eye;return true;
    });
    if(!owned){interrupted=true;++diagnostics.rejected;return false;}
    ++diagnostics.camera_owned;
    // A rendering retry cannot advance the hand owner or refresh body TTL.
    if(!scene||scene->thread!=GetCurrentThreadId()||!scene->invocation||
       !stereo||!stereo.reference_generation){interrupted=true;++diagnostics.rejected;return true;}
    if(scene->invocation==last_frame)return true;
    if(scene->invocation<last_frame){interrupted=true;++diagnostics.rejected;return true;}
    last_frame=scene->invocation;
    if(!update_epochs(eye,stereo)){interrupted=true;++diagnostics.rejected;return true;}
    gears_vr::CoverTrackingBasis axes{};
    if(!gears_vr::cover_tracking_basis(stereo.reference,axes)){interrupted=true;++diagnostics.rejected;return true;}
    const auto stamp=gears_vr::monotonic_ns();gears_vr::ControllerInputFrame input{};
    const bool matched=matching_input(stereo,stamp,input);
    if(matched)++diagnostics.matched;else ++diagnostics.missing;
    gears_vr::CoverInputOwnerFrame frame{};frame.lifecycle=1;frame.frame=scene->invocation;
    frame.cover.sequence=frame.frame;frame.cover.observed_ns=stamp;frame.cover.epochs=epochs;
    frame.cover.cover_valid=true;frame.cover.interaction_enabled=!eye.special_move;
    frame.cover.right_axis_tracking=axes.right;frame.cover.up_axis_tracking=axes.up;
    // Geometry remains untrusted until native surface/reach queries pass. A
    // candidate CoverSlot plane MUST NOT silently acquire LGrip ownership.
    frame.cover.range={0,0,0,0};frame.cover.edge={};
    frame.anchor={eye.neutral_eye_world,camera::facing.forward,camera::facing.right};
    frame.stereo=stereo;frame.controllers=matched&&!interrupted?&input:nullptr;
    frame.height_curve=height_curve.data();frame.height_key_count=unsigned(height_curve.size());
    frame.units_per_metre=stereo_units_per_metre;frame.now_ns=stamp;
    frame.allow_sim=simulator_operator;frame.owner_context_valid=!eye.special_move;
    interrupted=false;
    const auto result=participant.update(frame,left_hand_owner);diagnostics.latest=result;
    const auto status=unsigned(result.controller_status);if(status<diagnostics.controller_status.size())++diagnostics.controller_status[status];
    diagnostics.sequence=stereo.sample.sequence;diagnostics.reference=stereo.reference_generation;diagnostics.frame=scene->invocation;
    diagnostics.hand_world_valid=matched&&gears_vr::controller_pose_usable(input.hands[0].grip,input.hands[0].grip_flags,input.hands[0].grip_active)&&
        result.view_valid&&gears_vr::cover_tracking_point(frame.anchor,result.view,stereo.reference,
        input.hands[0].grip.position,stereo_units_per_metre,diagnostics.hand_world);
    if(result.accepted&&result.view_valid){
        view=result.view.base_view;
        if(result.body_weight_valid&&!eye.special_move&&publish(scene,eye,float(result.requested_up_weight)))++diagnostics.targets;
        else ++diagnostics.target_rejected;
    }else ++diagnostics.rejected;
    return true;
    }catch(...){
        // Only this observer's own preparation/publication is inside this
        // boundary. No original game function is dispatched by this module.
        interrupted=true;diagnostics.hand_world_valid=false;++diagnostics.rejected;return owned;
    }
}
bool read_height(const wchar_t* ini,const wchar_t* key,double& out){
    wchar_t text[64]{};const auto length=GetPrivateProfileStringW(L"CoverInputPrototype",key,L"",text,64,ini);
    if(!length||length>=63)return false;wchar_t* end{};const double value=std::wcstod(text,&end);
    if(end==text||*end||!std::isfinite(value)||value<=0||value>2)return false;out=value;return true;
}
bool configure(const wchar_t* ini,FILE* file){
    const auto request=GetPrivateProfileIntW(L"CoverInputPrototype",L"enabled",0,ini);
    if(!request)return true;
#if !defined(GEARS_COVER_BODY_PROTOTYPE) || !GEARS_COVER_BODY_PROTOTYPE
    (void)file;return false;
#else
    if(request!=1||enabled||!camera::enabled||!cover_body_live::live_source()||
       cover_body_live::pose_profile!=cover_body_live::PoseProfile::top||
       !read_height(ini,L"height_half_metres",height_curve[1].height_metres)||
       !read_height(ini,L"height_top_metres",height_curve[2].height_metres)||
       height_curve[1].height_metres>=height_curve[2].height_metres)return false;
    enabled=true;
    fprintf(file,"cover_input_config enabled=1 experimental=1 exact_stereo_controller_v3=1 gt_owner=1 physical_position_drives_top=1 rotation_drives_stance=0 measured_half_metres=%.9g measured_top_metres=%.9g measured_profile_universal_acceptance=0 grip_geometry_ready=0 grip_acquisition_enabled=0 weapon_participants_connected=0\n",
        height_curve[1].height_metres,height_curve[2].height_metres);return true;
#endif
}
bool camera_frame_aligned(){return diagnostics.frame&&camera::diagnostics.last==camera::Reason::applied&&camera::diagnostics.latest_invocation==diagnostics.frame;}
void summary(FILE* file){
    const auto& r=diagnostics.latest;
    fprintf(file,"cover_input_enabled=%u cover_input_attempts=%llu cover_input_camera_owned=%llu cover_input_rejected=%llu cover_input_matched=%llu cover_input_missing=%llu cover_input_targets=%llu cover_input_target_rejected=%llu cover_input_sequence=%llu cover_input_reference=%llu cover_input_frame=%llu cover_input_physical=%.9g,%.9g,%.9g cover_input_requested_up=%.9g cover_input_requested_weight=%.9g cover_input_hand_world_valid=%u cover_input_hand_world=%.9g,%.9g,%.9g ",
        unsigned(enabled),diagnostics.attempts,diagnostics.camera_owned,diagnostics.rejected,diagnostics.matched,diagnostics.missing,diagnostics.targets,diagnostics.target_rejected,
        static_cast<unsigned long long>(diagnostics.sequence),static_cast<unsigned long long>(diagnostics.reference),static_cast<unsigned long long>(diagnostics.frame),
        r.physical.right,r.physical.up,r.physical.forward,r.requested_up_metres,r.requested_up_weight,unsigned(diagnostics.hand_world_valid),diagnostics.hand_world.x,diagnostics.hand_world.y,diagnostics.hand_world.z);
    fprintf(file,"cover_input_camera_frame_aligned=%u ",unsigned(camera_frame_aligned()));
}
void progress(FILE* file){std::lock_guard<decltype(stereo_mutex)> lock(stereo_mutex);summary(file);}
void print(FILE* file){summary(file);fprintf(file,"\n");}
void close(){std::lock_guard<decltype(stereo_mutex)> lock(stereo_mutex);controllers.close();bridge_open=false;interrupted=true;}
} // namespace cover_input_live
