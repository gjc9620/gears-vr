// Include after weapon_hand_scene_binding. Globally include
// run_camera_stabilizer.h first. Caller holds stereo_mutex and an admitted,
// current Scene Scope/Receipt. No engine call, HMD composition or memory write.
namespace run_camera_live {
namespace b=weapon_hand_scene_binding;
bool enabled{};
bool have_last{};
gears_run_camera::Kernel kernel;
struct Diagnostics {unsigned long long attempts{},calibrating{},applied{},rejected{},retries{};unsigned reason{};uint64_t frame{};double height{};} diagnostics;
uint64_t system_ns(){LARGE_INTEGER q{},f{};if(!QueryPerformanceCounter(&q)||!QueryPerformanceFrequency(&f)||q.QuadPart<=0||f.QuadPart<=0)return 0;
    const auto ticks=uint64_t(q.QuadPart),frequency=uint64_t(f.QuadPart),seconds=ticks/frequency,remainder=ticks%frequency;
    if(seconds>UINT64_MAX/1000000000||remainder>UINT64_MAX/1000000000)return 0;
    return seconds*1000000000+remainder*1000000000/frequency;}
uint64_t system_scene_ns(int64_t ticks){LARGE_INTEGER f{};if(ticks<=0||!QueryPerformanceFrequency(&f)||f.QuadPart<=0)return 0;
    const auto t=uint64_t(ticks),frequency=uint64_t(f.QuadPart),seconds=t/frequency,remainder=t%frequency;
    if(seconds>UINT64_MAX/1000000000||remainder>UINT64_MAX/1000000000)return 0;
    return seconds*1000000000+remainder*1000000000/frequency;}
uint64_t (*clock_ns)()=system_ns;
uint64_t (*scene_ns)(int64_t)=system_scene_ns;
bool decline(unsigned reason){kernel.reset();have_last=false;diagnostics.reason=reason;++diagnostics.rejected;return false;}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
b::Proof proof; // Its five generic fields are unused: our eight are separate.
std::array<b::Property,8> fields{};
std::array<unsigned,8> masks{};
struct Structure {gears_anchor::Identity type{};std::array<b::Property,3> components{};};
std::array<Structure,2> structures{};
std::array<unsigned char,sizeof(gears_anchor::Config)> metadata_bytes{};
uint64_t metadata_generation{};
struct Values {std::array<float,3> location{};float eye_height{};std::array<int32_t,3> rotation{};
    unsigned char cover{};bool crouched{},wants_crouch{},cover_transition{},mirror_transition{};};
bool same_values(Values const& a,Values const& c){return a.location==c.location&&a.eye_height==c.eye_height&&a.rotation==c.rotation&&
    a.cover==c.cover&&a.crouched==c.crouched&&a.wants_crouch==c.wants_crouch&&a.cover_transition==c.cover_transition&&a.mirror_transition==c.mirror_transition;}
bool structure(b::Property& property,const wchar_t* name,const wchar_t* scalar,Structure& out){
    uintptr_t address{};unsigned extent{};if(!b::source_field(proof,address,property.id.address,0xd0)||!b::certified(proof,address,out.type)||
       !b::named(out.type,name)||!b::kind(proof,out.type,L"ScriptStruct")||!b::source_field(proof,extent,address,0x88)||extent!=12)return false;
    property.reference=out.type;uintptr_t child{};if(!b::source_field(proof,child,address,0x80))return false;
    const wchar_t* vector_names[]={L"X",L"Y",L"Z"};const wchar_t* rotation_names[]={L"Pitch",L"Yaw",L"Roll"};
    auto names=!wcscmp(name,L"Vector")?vector_names:rotation_names;
    unsigned found{};std::array<uintptr_t,128> seen{};unsigned count{};
    while(child){if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;seen[count++]=child;
        gears_anchor::Identity id{};if(!b::identity(child,id,false,true)||id.outer!=address)return false;
        for(unsigned i=0;i<3;++i)if(!wcscmp(id.name.data(),names[i])){
            auto& component=out.components[i];if(id.number||(found&(1u<<i))||!b::certified(proof,child,component.id)||!b::kind(proof,id,scalar)||
               !b::source_field(proof,component.dimension,child,0x70)||component.dimension!=1||
               !b::source_field(proof,component.width,child,0x74)||component.width!=4||
               !b::source_field(proof,component.offset,child,0x94)||component.offset>8||(component.offset&3))return false;
            component.owner=out.type;found|=1u<<i;
        }
        if(!b::source_field(proof,child,child,0x68))return false;
    }
    return found==7&&out.components[0].offset!=out.components[1].offset&&out.components[0].offset!=out.components[2].offset&&out.components[1].offset!=out.components[2].offset;
}
bool prepare_layout(b::Binding const& actor){
    if(metadata_generation!=b::metadata_generation||memcmp(metadata_bytes.data(),b::metadata_bytes.data(),metadata_bytes.size())){
        proof={};fields={};masks={};structures={};metadata_generation=b::metadata_generation;metadata_bytes=b::metadata_bytes;}
    gears_anchor::Identity pawn_class{},pc_class{};if(!b::identity(actor.pawn.cls,pawn_class)||!b::identity(actor.controller.cls,pc_class))return false;
    b::keys(proof,pawn_class,pc_class);if(!b::begin_prepare(proof))return false;if(proof.ready)return true;
    fields={};masks={};structures={};
    if(!b::property(proof,pawn_class.address,L"Location",L"StructProperty",12,nullptr,fields[0])||
       !structure(fields[0],L"Vector",L"FloatProperty",structures[0])||
       !b::property(proof,pawn_class.address,L"EyeHeight",L"FloatProperty",4,nullptr,fields[1])||
       !b::property(proof,pc_class.address,L"Rotation",L"StructProperty",12,nullptr,fields[2])||
       !structure(fields[2],L"Rotator",L"IntProperty",structures[1])||
       fields[0].offset!=b::metadata.pawn_location||fields[1].offset!=b::metadata.eye_height||fields[2].offset!=b::metadata.controller_rotation)return false;
    const wchar_t* names[]={L"bIsCrouched",L"bWantsToCrouch",L"CoverType",L"bDoingCoverActionAnimTransition",L"bDoingMirrorTransition"};
    for(unsigned i=3;i<8;++i){const bool byte=i==5;
        if(!b::property(proof,pawn_class.address,names[i-3],byte?L"ByteProperty":L"BoolProperty",byte?1u:4u,nullptr,fields[i]))return false;
        if(!byte&&(!b::source_field(proof,masks[i],fields[i].id.address,0xd0)||!masks[i]||(masks[i]&(masks[i]-1))))return false;
    }
    if(!b::current(proof))return false;proof.ready=true;return true;
}
bool read_values(b::Binding const& actor,Values& out){
    Values value{};
    for(unsigned i=0;i<3;++i)if(!b::field(value.location[i],actor.pawn.address,fields[0].offset+structures[0].components[i].offset)||
        !std::isfinite(value.location[i])||!b::field(value.rotation[i],actor.controller.address,fields[2].offset+structures[1].components[i].offset))return false;
    if(!b::field(value.eye_height,actor.pawn.address,fields[1].offset)||!std::isfinite(value.eye_height)||
       !b::field(value.cover,actor.pawn.address,fields[5].offset))return false;
    bool* booleans[]={&value.crouched,&value.wants_crouch,&value.cover_transition,&value.mirror_transition};
    const unsigned indices[]={3,4,6,7};for(unsigned i=0;i<4;++i){unsigned word{};if(!b::field(word,actor.pawn.address,fields[indices[i]].offset))return false;*booleans[i]=(word&masks[indices[i]])!=0;}
    out=value;return true;
}
bool same_actor(b::Binding const& a,b::Binding const& c){return b::same_key(a,c)&&a.scene_invocation==c.scene_invocation&&a.special_move==c.special_move;}
bool current(gears_anchor::Receipt const& scene_receipt,b::Binding const& actor,Values const& values){
    return recording.load()&&b::context_ready()&&b::game_thread.load()==GetCurrentThreadId()&&scene_receipt.thread==GetCurrentThreadId()&&
        b::metadata_generation==metadata_generation&&b::metadata_bytes==metadata_bytes&&b::metadata_current()&&b::current(proof)&&
        actor.world_generation==committed_world_generation.load()&&scene_receipt.invocation==actor.scene_invocation&&
        gears_anchor::equal_identity(scene_receipt.anchor.pawn_identity,actor.pawn)&&scene_receipt.anchor.pawn==actor.pawn.address&&
        scene_receipt.anchor.controller==actor.controller.address&&!scene_receipt.anchor.cinematic&&!scene_receipt.anchor.unpossessed&&
        scene_receipt.anchor.location==values.location&&scene_receipt.anchor.eye_height==values.eye_height;
}
gears_run_camera::Sample last_sample{};Values last_values{};
b::Binding last_actor{};gears_vr::StereoResult last_stereo{};gears_run_camera::Result last_result{};gears_run_camera::Status last_status=gears_run_camera::Status::inactive;
bool prepare(gears_anchor::Receipt const* scene_receipt,gears_vr::StereoResult const& stereo,gears_vr::StereoMatrix& view,bool fp_active){
    if(!enabled)return false;++diagnostics.attempts;
    try{
        const auto started=clock_ns();
        auto fresh=[&](uint64_t t,uint64_t at){return t&&at>=t&&at-t<=50000000;};
        if(!scene_receipt||!stereo||!stereo.sample.session||!stereo.sample.space_epoch||!stereo.reference_generation||stereo.sample.received_ns<=0||
           !fresh(uint64_t(stereo.sample.received_ns),started)||!fresh(scene_ns(scene_receipt->scene_qpc),started))return decline(1);
        std::array<unsigned char,sizeof(gears_anchor::Receipt)> receipt{};memcpy(receipt.data(),scene_receipt,receipt.size());
        b::Binding first{},second{};Values values{},again{};
        if(!b::sample(scene_receipt,first))return decline(2);
        {std::lock_guard<std::mutex> lock(b::mutex);
            if(!b::context_ready()||!prepare_layout(first)||!read_values(first,values)||!current(*scene_receipt,first,values)||
               !read_values(first,again)||!same_values(values,again))return decline(3);}
        if(!b::sample(scene_receipt,second)||!same_actor(first,second))return decline(4);
        std::lock_guard<std::mutex> closing(b::mutex);
            if(!current(*scene_receipt,second,values)||!read_values(second,again)||!same_values(values,again)||!current(*scene_receipt,second,again)||
               memcmp(receipt.data(),scene_receipt,receipt.size())||!fresh(scene_ns(scene_receipt->scene_qpc),clock_ns())||
               !fresh(uint64_t(stereo.sample.received_ns),clock_ns())||!current(*scene_receipt,second,again)||
               memcmp(receipt.data(),scene_receipt,receipt.size()))return decline(5);
        gears_run_camera::Sample input{};input.key={first.world,first.pawn,first.mesh,first.controller,stereo.sample.session,
            first.world_generation,stereo.reference_generation,stereo.sample.space_epoch};
        input.frame=scene_receipt->invocation;input.sample_ns=scene_ns(scene_receipt->scene_qpc);input.pawn_location={values.location[0],values.location[1],values.location[2]};
        input.eye_height=values.eye_height;constexpr double units_to_radians=6.2831853071795864769/65536.;
        input.controller_pitch_radians=std::remainder(double(values.rotation[0]),65536.)*units_to_radians;
        input.controller_yaw_radians=std::remainder(double(values.rotation[1]),65536.)*units_to_radians;
        input.units_per_metre=stereo_units_per_metre;input.special_move=first.special_move;input.context_verified=input.eligible=true;
        input.standing_sm0_verified=first.special_move==0&&values.cover==0&&!values.crouched&&!values.wants_crouch&&!values.cover_transition&&!values.mirror_transition;
        if(have_last&&input.frame==last_sample.frame){
            if(!same_actor(first,last_actor)||!gears_run_camera::same(input.key,last_sample.key)||input.sample_ns!=last_sample.sample_ns||input.units_per_metre!=last_sample.units_per_metre||
               input.special_move!=last_sample.special_move||!same_values(values,last_values)||
               stereo.status!=last_stereo.status||stereo.recentered!=last_stereo.recentered||stereo.heading_deferred!=last_stereo.heading_deferred||
               !gears_vr::stereo_same(stereo.sample,last_stereo.sample)||stereo.reference_generation!=last_stereo.reference_generation||
               !gears_vr::stereo_same_pose(stereo.reference,last_stereo.reference))return decline(6);
            ++diagnostics.retries;if(fp_active&&last_status==gears_run_camera::Status::applied){view=last_result.view;++diagnostics.applied;return true;}return false;
        }
        gears_run_camera::Result result{};const auto status=kernel.update(input,clock_ns(),result);
        diagnostics.frame=input.frame;diagnostics.height=kernel.protected_height();diagnostics.reason=unsigned(kernel.reason());
        have_last=status!=gears_run_camera::Status::rejected;last_sample=input;last_values=values;last_actor=first;last_stereo=stereo;last_status=status;last_result=result;
        if(status==gears_run_camera::Status::calibrating)++diagnostics.calibrating;
        if(status==gears_run_camera::Status::rejected){++diagnostics.rejected;return false;}
        if(fp_active&&status==gears_run_camera::Status::applied){view=result.view;++diagnostics.applied;return true;}return false;
    }catch(...){have_last=false;return decline(7);}
}
#else
bool prepare(gears_anchor::Receipt const*,gears_vr::StereoResult const&,gears_vr::StereoMatrix&,bool){return false;}
#endif
bool configure(const wchar_t* ini,FILE* file){
    const auto request=GetPrivateProfileIntW(L"RunCameraStabilizer",L"enabled",0,ini);if(!request)return !enabled;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
    if(request!=1||enabled||!continuous_session||!stereo_xr||!anchor_observer_enabled)return false;
    enabled=true;if(file)fprintf(file,"run_camera_config enabled=1 experimental=1 read_only=1 standing_typed_proof=1 sm4_only=1 hmd_composed_here=0 sustained_fresh_sprint=1 engine_calls=0 runtime_acceptance=0\n");return true;
#else
    (void)file;return false;
#endif
}
void summary(FILE* file){fprintf(file,"run_camera_enabled=%u run_camera_attempts=%llu run_camera_calibrating=%llu run_camera_applied=%llu run_camera_rejected=%llu run_camera_retries=%llu run_camera_frame=%llu run_camera_height=%.9g run_camera_reason=%u run_camera_read_only=1 run_camera_runtime_accepted=0 ",unsigned(enabled),diagnostics.attempts,diagnostics.calibrating,diagnostics.applied,diagnostics.rejected,diagnostics.retries,static_cast<unsigned long long>(diagnostics.frame),diagnostics.height,diagnostics.reason);}
void print(FILE* file){summary(file);fprintf(file,"\n");}
} // namespace run_camera_live
