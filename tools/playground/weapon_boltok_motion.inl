// Included inside boltok_runtime. Existing producer-ring motion only; current
// Frame/lease/native command and measured endpoint remain the original owners.
namespace motion {
namespace source=gears_hand_continuity;namespace relative=gears_controller_relative_motion;
struct Registry {std::uintptr_t data{};std::int32_t count{},capacity{};};
struct Context {
    hand::Frame frame{};source::TrackedInput raw{};source::Publisher publisher{};
    gears_rig_tracking::ObservedContinuity observed{};gears_rig_tracking::NativeReference reference{};
    hand::ObjectIdentity component{},asset{};std::array<unsigned char,68> root_reference{};
    std::array<float,16> model_in_right{};P canonical{},actual{};V local{};
    double scale{},units{};std::uint64_t hold{},endpoint_serial{},binding_generation{};unsigned gate{1};bool valid{};
};
struct Input {hand::FrameKey key{};source::TrackedInput raw{};source::Publisher publisher{};
    source::MotionHistory history{};gears_current_tracking::Failure failure{};bool matched{};
    bool transport_matched{},merge_attempted{},merge_matched{};
    unsigned incoming_count{};std::uint64_t incoming_first{},incoming_last{};};
Context current{},previous{};Input input{};
bool same(Registry a,Registry b) noexcept {return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;}
bool root_reference(hand::Frame const& f,hand::ObjectIdentity asset,std::array<unsigned char,68>& out,V& position) noexcept {
    out={};position={};Registry bones{},again{},names{},names_after{};
    if(!nativeequipment::current(nullptr,f)||!copy(&bones,asset.address+0xa4,sizeof bones)||
       bones.count!=12||bones.capacity<12||bones.capacity>512||!bones.data||
       bones.data>UINTPTR_MAX-std::size_t(bones.capacity)*68||!copy(out.data(),bones.data,out.size()))return false;
    unsigned index{},number{};int parent{};std::array<float,4> q{};std::array<float,3> p{};
    std::memcpy(&index,out.data(),4);std::memcpy(&number,out.data()+4,4);
    std::memcpy(q.data(),out.data()+12,16);std::memcpy(p.data(),out.data()+28,12);std::memcpy(&parent,out.data()+56,4);
    if(number||parent||!std::isfinite(q[0])||!std::isfinite(q[1])||!std::isfinite(q[2])||!std::isfinite(q[3])||
       std::abs(q[0])+std::abs(q[1])+std::abs(q[2])>1e-6||std::abs(std::abs(q[3])-1)>1e-6||
       !copy(&names,base+0x1c34800,sizeof names)||!names.data||names.count<1000||names.capacity<names.count||
       names.capacity>4000000||index>=unsigned(names.count)||names.data>UINTPTR_MAX-std::size_t(names.capacity)*8)return false;
    std::uintptr_t entry{},repeat{};unsigned stored{};std::array<wchar_t,32> text{};
    if(!copy(&entry,names.data+std::size_t(index)*8,8)||!entry||!copy(&stored,entry,4)||stored!=index||
       !copy(text.data(),entry+20,sizeof text)||!std::wmemchr(text.data(),0,text.size())||std::wcscmp(text.data(),L"alow_poly")||
       !copy(&repeat,names.data+std::size_t(index)*8,8)||repeat!=entry||
       !copy(&names_after,base+0x1c34800,sizeof names_after)||!same(names,names_after))return false;
    std::array<unsigned char,68> after{};
    if(!copy(&again,asset.address+0xa4,sizeof again)||!same(bones,again)||
       !copy(after.data(),again.data,after.size())||after!=out||!nativeequipment::current(nullptr,f))return false;
    position={p[0],p[1],p[2]};return gears_vr::finite(position)&&gears_weapon::length(position)<100000;
}
bool close(V a,V b,double tolerance) noexcept {return gears_vr::finite(a)&&gears_vr::finite(b)&&gears_weapon::length(gears_weapon::difference(a,b))<=tolerance;}
bool orientation(P const& a,P const& b) noexcept {
    if(!gears_vr::stereo_pose_valid(a)||!gears_vr::stereo_pose_valid(b))return false;
    const auto p=gears_vr::normalized(a.orientation),q=gears_vr::normalized(b.orientation);
    return std::abs(p.x*q.x+p.y*q.y+p.z*q.z+p.w*q.w)>=std::cos(.0002*.5);
}
gears_boltok_motion_history::Buffer history_buffer;
void clear() noexcept {current=previous={};input={};history_buffer.clear();}
void read_early(hand::Frame const& f) noexcept {
    input={};current={};
    if(!moving||!stroke.original_motion_bound()||!f.left_grip_down){history_buffer.clear();return;}
    if(f.controllers.sequence<=control.source.controllers.sequence)return;
    input.key=f.key;
    input.matched=nativeequipment::read_input_continuity(f,input.raw,input.publisher,nullptr,&input.failure,&input.history);
    input.transport_matched=input.matched;
    input.incoming_count=input.history.count;
    if(input.incoming_count&&input.incoming_count<=source::motion_history_capacity){
        input.incoming_first=input.history.frames[0].input.sequence;
        input.incoming_last=input.history.frames[input.incoming_count-1].input.sequence;
    }
    if(input.matched){
        source::MotionHistory merged{};
        input.merge_attempted=true;
        input.matched=history_buffer.merge(input.raw,input.publisher,input.history,merged);
        input.merge_matched=input.matched;
        if(input.matched)input.history=merged;
    }else history_buffer.clear();
}
bool capture(hand::Frame const& f,gears_rig_tracking::Frame const& rig,
    boltok_parts::Mechanical const& parts,P const& actual,V local) noexcept {
    current={};if(!f.left_grip_down&&!moving)return false;
    auto& c=current;c.frame=f;c.actual=actual;c.local=local;c.component=parts.source.component;c.asset=parts.source.asset;
    c.scale=parts.component_scale;c.units=parts.units;c.hold=nativelancerfollow::bound_key.hold_generation;
    c.binding_generation=rig.binding_generation;
    c.gate=1;
    if(!(rig.flags&gears_rig_tracking::producer_continuity_available)||!source::same_input(f.controllers,rig.controllers)||
       !rig.controller_publisher_pid||!rig.controller_publisher_start||!c.hold||c.hold!=vrequip::model.hold_generation()||
       !hand::same(parts.source.key,f.key)||rig.native.game_pid!=f.key.game_pid||rig.native.game_start!=f.key.game_start||
       rig.native.native_started_qpc!=f.key.native_started_qpc||rig.native.registration_cookie!=f.key.registration_cookie||
       rig.native.game_thread!=f.key.thread_id||rig.world_generation!=f.key.world_generation||
       rig.reference_generation!=f.reference_generation||!hand::same(rig.world,f.world)||!hand::same(rig.controller,f.controller)||
       !hand::same(rig.pawn,f.pawn)||!hand::same(rig.mesh,f.mesh)||!hand::same(rig.weapon,f.weapon)||
       (rig.world_pose_mask&(gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose))!=
           (gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose))return false;
    c.raw={rig.controllers,rig.producer.stream,{rig.producer.tracking[0],rig.producer.tracking[1]}};
    c.publisher={rig.controller_publisher_pid,rig.controller_publisher_start};c.observed=rig.observed;c.reference=rig.native_reference;
    c.gate=2;
    if(!source::tracked_input_shape_valid(c.raw)||c.reference.flags!=gears_rig_tracking::reference_stable||!c.reference.epoch||
       !c.observed.stream||!c.observed.reference||!c.observed.head_tracking||!c.observed.hand_tracking[0]||!c.observed.hand_tracking[1]||
       !std::isfinite(c.units)||c.units<10||c.units>500||rig.units_per_metre!=c.units)return false;
    c.gate=3;
    // Boltok uses the existing one-right-controller rigid mount. No Lancer/
    // rifle two-hand correction or reach-resolved alternative may borrow it.
    if(!nativelancerfollow::bound_key_known||nativelancerfollow::bound_is_lancer||nativelancerfollow::bound_is_frag||
       nativelancerfollow::bound_is_hammerburst||nativelancerfollow::bound_is_longshot||!nativelancerfollow::mount.bound()||
       !hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||!hand::same(nativelancerfollow::bound_key.component,c.component)||
       !hand::same(nativelancerfollow::bound_key.asset,c.asset)||nativelancerfollow::bound_reference!=f.reference_generation||
       nativelancerfollow::bound_session!=f.stereo.session||nativelancerfollow::bound_space!=f.stereo.space_epoch||
       nativelancerfollow::bound_input!=f.controllers.input_epoch||nativelancerfollow::bound_world!=f.key.world_generation||
       nativelancerfollow::bound_units!=c.units)return false;
    c.gate=4;V root{};
    if(!root_reference(f,c.asset,c.root_reference,root)||!nativelancerfollow::mount.model_matrix({{},{}},c.model_in_right))return false;
    c.gate=5;P model{};double model_scale{};
    if(!hammerburst_runtime::matrix(c.model_in_right,model,model_scale)||std::abs(model_scale-c.scale)>1e-6)return false;
    // Actual decomposition was checked above. Retain the immutable model's
    // scale, not float normalization noise from each measured world matrix.
    c.scale=model_scale;
    // Live reference root composed with the SAME immutable mount used by the
    // original component follower. This is a controller-relative model, never
    // a guessed historical game/world transform. Current actual root must close.
    c.canonical.position=gears_weapon::scaled(gears_weapon::sum(model.position,
        gears_weapon::rotate(model.orientation,gears_weapon::scaled(root,model_scale))),1/c.units);
    c.canonical.orientation=model.orientation;c.gate=6;
    if(!close(c.canonical.position,actual.position,.0001)||!orientation(c.canonical,actual)){
        gears_committed_hand::Frame endpoint{};P endpoint_grip{};
        c.gate=8;const bool received=handrig::read_endpoint(endpoint);
        const auto observed_now=weaponhand::clock_ns(nullptr);
        const bool sim=f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
        const bool fresh=received&&gears_committed_hand::fresh(endpoint,observed_now,sim);
        const bool matched=received&&gears_committed_hand::matches(endpoint,f,rig,c.hold,observed_now,sim);
        const bool converted=received&&gears_weapon_lancer_wrist_mount::grip_from_palm_endpoint(endpoint.right_grip,endpoint_grip);
        // Observation only: read the existing Native endpoint snapshot rather
        // than an unsynchronized render Plan. Its pose cannot authorize this
        // rejected action or substitute for original controller history.
        static unsigned rows{};static std::uint64_t last_token{};
        if(log_file&&rows<4&&last_token!=f.press_token){
            last_token=f.press_token;++rows;
            const auto& grip=endpoint_grip;
            const auto raw=rig.world_grip[1];
            const auto endpoint_root=converted?gears_weapon::sum(grip.position,
                gears_weapon::rotate(grip.orientation,gears_weapon::scaled(c.canonical.position,c.units))):V{};
            const auto measured=gears_weapon::scaled(parts.root_metres.position,c.units);
            const double root_error=converted?gears_weapon::length(gears_weapon::difference(endpoint_root,measured))/c.units:-1.;
            fwprintf(log_file,L"boltok_root_endpoint_observation frame=%llu input=%llu token=%llu received=%u fresh=%u matched=%u converted=%u purpose=%u endpoint_frame=%llu endpoint_input=%llu hold=%llu endpoint_hold=%llu raw_right=%.9g,%.9g,%.9g endpoint_right=%.9g,%.9g,%.9g measured_root=%.9g,%.9g,%.9g endpoint_root_error_metres=%.9g observation_only=1 raw_mount_mismatch=1 action_permission=0\n",
                f.key.frame,f.controllers.sequence,f.press_token,unsigned(received),unsigned(fresh),unsigned(matched),unsigned(converted),
                unsigned(endpoint.purpose),endpoint.parent.frame,endpoint.controllers.sequence,c.hold,endpoint.hold_generation,
                raw.position.x,raw.position.y,raw.position.z,grip.position.x,grip.position.y,grip.position.z,
                measured.x,measured.y,measured.z,root_error);fflush(log_file);
        }
        // The held gun follows the existing committed C0 palm, which can be
        // reach-resolved away from raw tracking. Prove the SAME immutable
        // model/root against that current action-qualified endpoint first.
        // Only then express that root relative to CURRENT raw-right input.
        // The stroke freezes this registration and rejects later drift at
        // its original .1mm/.2mrad tolerances; no historical palm is invented.
        if(!received)return false;
        c.gate=9;if(!matched)return false;
        c.gate=10;if(!converted)return false;
        const auto& raw_right=rig.world_grip[1];
        const auto world_position=gears_weapon::sum(endpoint_grip.position,
            gears_weapon::rotate(endpoint_grip.orientation,gears_weapon::scaled(c.canonical.position,c.units)));
        const auto world_rotation=gears_vr::normalized(gears_vr::multiply(endpoint_grip.orientation,c.canonical.orientation));
        const auto inverse_right=gears_vr::inverse(raw_right.orientation);
        P registered{gears_weapon::scaled(gears_weapon::rotate(inverse_right,
            gears_weapon::difference(world_position,raw_right.position)),1/c.units),
            gears_vr::normalized(gears_vr::multiply(inverse_right,world_rotation))};
        c.gate=11;if(!close(registered.position,actual.position,.0001))return false;
        c.gate=12;
        if(!orientation(registered,actual)){
            // The engine evaluates an integer component Rotator plus the
            // asset RotOrigin. Authenticate the EXACT controls used by the
            // existing writer, then independently compare the measured root
            // to that component's evaluated basis. Do not enlarge the angular
            // tolerance to cover continuous-vs-engine rotation differences.
            gears_vr::Viewpoint wanted{};gears_vr::Rotator control_rotation{};
            std::array<std::int32_t,3> origin{};
            const gears_vr::Viewpoint grip_view{endpoint_grip.position,
                gears_vr::ue_angles(gears_vr::rotation_matrix(endpoint_grip.orientation))};
            c.gate=13;
            if(!nativelancerfollow::mount.solve(grip_view,wanted)||
               !nativelancerfollow::asset_rotation_origin(nativelancerfollow::bound_key,origin)||
               !gears_weapon::model_to_component_rotation(wanted.rotation,origin,control_rotation))return false;
            const playground::Rotation expected{gears_vr::to_rotator_units(control_rotation.pitch),
                gears_vr::to_rotator_units(control_rotation.yaw),gears_vr::to_rotator_units(control_rotation.roll)};
            nativelancerfollow::State before{},after{};vrequip::MeshPoseProbe first{},second{};
            if(!nativelancerfollow::value_of(nativelancerfollow::bound_key,before,&first)||
               !first.transform_known||!first.absolute_rotation||
               std::memcmp(&expected,&first.rotation,sizeof expected))return false;
            std::array<float,16> evaluated{};std::copy_n(first.local_to_world,16,evaluated.data());
            P component{};double evaluated_scale{};c.gate=14;
            if(!hammerburst_runtime::matrix(evaluated,component,evaluated_scale)||
               std::abs(evaluated_scale-c.scale)>1e-5||
               !orientation(component,parts.root_metres))return false;
            // A second full observation must still describe those exact
            // controls and the same evaluated matrix after the bone getter.
            if(!nativelancerfollow::value_of(nativelancerfollow::bound_key,after,&second)||
               !gears_weapon::mesh_component_exact(before.value,after.value)||
               std::memcmp(first.local_to_world,second.local_to_world,sizeof first.local_to_world))return false;
            // Associate the evaluated basis with the exact root measurement
            // used by this current Frame, after both component observations.
            // A changed C0/bone getter result retries rather than borrowing it.
            boltok_parts::Parts confirmed{};c.gate=15;
            if(!boltok_parts::read(f,confirmed)||!hand::same(confirmed.key,parts.source.key)||
               !hand::same(confirmed.component,parts.source.component)||!hand::same(confirmed.asset,parts.source.asset)||
               confirmed.world[0]!=parts.source.world[0])return false;
            registered.orientation=gears_vr::normalized(gears_vr::multiply(inverse_right,component.orientation));
            if(!orientation(registered,actual))return false;
        }
        c.canonical=registered;c.endpoint_serial=endpoint.serial;
    }
    V projected{};c.gate=7;
    if(!relative::project(c.raw,c.canonical,1,projected)||!close(projected,local,.00025)||!nativeequipment::current(nullptr,f))return false;
    c.valid=true;c.gate=0;return true;
}
bool same_domain(Context const& a,Context const& b) noexcept {
    const auto& x=a.frame;const auto& y=b.frame;
    return x.key.game_pid==y.key.game_pid&&x.key.game_start==y.key.game_start&&
        x.key.native_started_qpc==y.key.native_started_qpc&&x.key.registration_cookie==y.key.registration_cookie&&
        x.key.lifecycle==y.key.lifecycle&&x.key.world_generation==y.key.world_generation&&x.key.thread_id==y.key.thread_id&&
        x.flags==y.flags&&x.reference_generation==y.reference_generation&&x.interaction_control==y.interaction_control&&
        hand::same(x.world,y.world)&&hand::same(x.controller,y.controller)&&hand::same(x.pawn,y.pawn)&&
        hand::same(x.mesh,y.mesh)&&hand::same(x.weapon,y.weapon)&&hand::same(a.component,b.component)&&hand::same(a.asset,b.asset)&&
        a.publisher.pid==b.publisher.pid&&a.publisher.start==b.publisher.start&&a.hold==b.hold&&a.units==b.units&&a.scale==b.scale&&
        a.binding_generation&&a.binding_generation==b.binding_generation&&
        a.root_reference==b.root_reference&&a.model_in_right==b.model_in_right&&
        a.reference.epoch==b.reference.epoch&&a.reference.flags==b.reference.flags&&
        a.observed.stream==b.observed.stream&&a.observed.reference==b.observed.reference&&a.observed.head_tracking==b.observed.head_tracking&&
        a.observed.hand_tracking[0]==b.observed.hand_tracking[0]&&a.observed.hand_tracking[1]==b.observed.hand_tracking[1];
}
bool same_context(Context const& a,Context const& b) noexcept {
    return a.valid&&b.valid&&same_domain(a,b);
}
void report_context_difference(hand::Frame const& f,Context const& a,
    Context const& b) noexcept {
    static unsigned rows{};
    if(!log_file||rows>=4)return;
    ++rows;
    unsigned mask{};
    if(!a.valid||!b.valid)mask|=1;
    auto key_a=a.frame.key;key_a.frame=b.frame.key.frame;
    if(!hand::same(key_a,b.frame.key))mask|=2;
    if(a.frame.flags!=b.frame.flags||
       a.frame.reference_generation!=b.frame.reference_generation||
       a.frame.interaction_control!=b.frame.interaction_control)mask|=4;
    if(!hand::same(a.frame.world,b.frame.world)||
       !hand::same(a.frame.controller,b.frame.controller)||
       !hand::same(a.frame.pawn,b.frame.pawn)||
       !hand::same(a.frame.mesh,b.frame.mesh)||
       !hand::same(a.frame.weapon,b.frame.weapon))mask|=8;
    if(!hand::same(a.component,b.component)||
       !hand::same(a.asset,b.asset))mask|=16;
    if(a.publisher.pid!=b.publisher.pid||
       a.publisher.start!=b.publisher.start)mask|=32;
    if(a.hold!=b.hold)mask|=64;
    if(a.units!=b.units)mask|=128;
    if(a.scale!=b.scale)mask|=256;
    if(!a.binding_generation||
       a.binding_generation!=b.binding_generation)mask|=512;
    if(a.root_reference!=b.root_reference)mask|=1024;
    if(a.model_in_right!=b.model_in_right)mask|=2048;
    if(a.reference.epoch!=b.reference.epoch||
       a.reference.flags!=b.reference.flags)mask|=4096;
    if(a.observed.stream!=b.observed.stream||
       a.observed.reference!=b.observed.reference||
       a.observed.head_tracking!=b.observed.head_tracking||
       a.observed.hand_tracking[0]!=b.observed.hand_tracking[0]||
       a.observed.hand_tracking[1]!=b.observed.hand_tracking[1])mask|=8192;

    int root_index=-1;unsigned root_before{},root_after{};
    for(std::size_t i=0;i<a.root_reference.size();++i){
        if(a.root_reference[i]!=b.root_reference[i]){
            root_index=static_cast<int>(i);
            root_before=a.root_reference[i];
            root_after=b.root_reference[i];break;
        }
    }
    int model_index=-1;float model_before{},model_after{};
    for(std::size_t i=0;i<a.model_in_right.size();++i){
        if(a.model_in_right[i]!=b.model_in_right[i]){
            model_index=static_cast<int>(i);
            model_before=a.model_in_right[i];
            model_after=b.model_in_right[i];break;
        }
    }

    fwprintf(log_file,
        L"boltok_context_difference mask=%u frame=%llu sequence=%llu "
        L"prior_sequence=%llu a_valid=%u b_valid=%u "
        L"flags=%u,%u reference_generation=%llu,%llu "
        L"interaction_control=%u,%u hold=%llu,%llu "
        L"units=%.17g,%.17g scale=%.17g,%.17g "
        L"binding_generation=%llu,%llu "
        L"native_reference_epoch=%llu,%llu native_reference_flags=%u,%u "
        L"observed_stream=%llu,%llu observed_reference=%llu,%llu "
        L"observed_head=%llu,%llu observed_hand0=%llu,%llu "
        L"observed_hand1=%llu,%llu "
        L"root_difference_index=%d root_before=%u root_after=%u "
        L"model_difference_index=%d model_before=%.17g model_after=%.17g\n",
        mask,f.key.frame,f.controllers.sequence,a.frame.controllers.sequence,
        unsigned(a.valid),unsigned(b.valid),
        a.frame.flags,b.frame.flags,
        a.frame.reference_generation,b.frame.reference_generation,
        unsigned(a.frame.interaction_control),
        unsigned(b.frame.interaction_control),
        a.hold,b.hold,a.units,b.units,a.scale,b.scale,
        a.binding_generation,b.binding_generation,
        a.reference.epoch,b.reference.epoch,
        unsigned(a.reference.flags),unsigned(b.reference.flags),
        a.observed.stream,b.observed.stream,
        a.observed.reference,b.observed.reference,
        a.observed.head_tracking,b.observed.head_tracking,
        a.observed.hand_tracking[0],b.observed.hand_tracking[0],
        a.observed.hand_tracking[1],b.observed.hand_tracking[1],
        root_index,root_before,root_after,
        model_index,static_cast<double>(model_before),
        static_cast<double>(model_after));
    fflush(log_file);
}

void report(hand::Frame const& f,unsigned gate,bool okay) noexcept {
    static unsigned rows{};static std::array<std::uint64_t,3> last{};
    const std::array<std::uint64_t,3> key{f.press_token,gate,unsigned(okay)};
    if(!log_file||rows>=12||last==key)return;last=key;++rows;
    const auto p=current.canonical.position,a=current.actual.position;
    fwprintf(log_file,L"boltok_original_motion frame=%llu sequence=%llu prior_sequence=%llu token=%llu gate=%u okay=%u capture=%u early_matched=%u failure=%u count=%u stream=%llu tracking=%llu,%llu publisher=%llu,%llu reference=%llu hold=%llu canonical=%.9g,%.9g,%.9g actual=%.9g,%.9g,%.9g current_Frame_only=1 historical_native_ack=0\n",
        f.key.frame,f.controllers.sequence,previous.raw.input.sequence,f.press_token,gate,unsigned(okay),current.gate,
        unsigned(input.matched),unsigned(input.failure),input.history.count,current.raw.stream_epoch,current.raw.tracking_epoch[0],
        current.raw.tracking_epoch[1],current.publisher.pid,current.publisher.start,current.reference.epoch,current.hold,
        p.x,p.y,p.z,a.x,a.y,a.z);fflush(log_file);
}
void report_rejection(hand::Frame const& f,
    gears_weapon::BoltokSwingStroke::OriginalMotionRejection const& rejection) noexcept {
    static unsigned rows{};static std::uint64_t last_token{},last_sequence{};
    if(!log_file||rows>=8||rejection.code==
        gears_weapon::BoltokSwingStroke::OriginalMotionRejection::Code::none||
       (last_token==f.press_token&&last_sequence==f.controllers.sequence))return;
    last_token=f.press_token;last_sequence=f.controllers.sequence;++rows;
    fwprintf(log_file,L"boltok_original_motion_reject code=%u point=%u history=%u path=%u prior_sequence=%llu next_sequence=%llu pair_gap_ns=%lld root_distance_m=%.9g root_dot=%.9g start_difference_m=%.9g endpoint_difference_m=%.9g detector=%u frame=%llu sequence=%llu token=%llu detector_reason=%u detector_previous=%.9g,%.9g,%.9g detector_next=%.9g,%.9g,%.9g\n",
        unsigned(rejection.code),rejection.point_index,rejection.history_count,rejection.path_count,
        rejection.prior_sequence,rejection.next_sequence,rejection.pair_gap_ns,
        rejection.root_distance_metres,rejection.root_dot,rejection.start_difference_metres,
        rejection.endpoint_difference_metres,unsigned(rejection.detector_rejected),
        f.key.frame,f.controllers.sequence,f.press_token,rejection.detector_reason,
        rejection.detector_previous.x,rejection.detector_previous.y,rejection.detector_previous.z,
        rejection.detector_next.x,rejection.detector_next.y,rejection.detector_next.z);
    fflush(log_file);
}
bool bind(hand::Frame const& f,std::int64_t now) noexcept {
    previous={};history_buffer.clear();
    unsigned selected{};
    if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))selected=gears_vr::control_squeeze;
    else if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))selected=gears_vr::control_trigger;
    else{report(f,1,false);return false;}
    if(!current.valid){report(f,2,false);return false;}
    if(current.endpoint_serial){
        gears_committed_hand::Frame latest{};
        if(!handrig::read_endpoint(latest)||latest.serial!=current.endpoint_serial||
           !gears_committed_hand::current_result(latest,now,
               f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)){
            report(f,5,false);return false;
        }
    }
    bool okay=stroke.bind_original_motion(current.raw,current.canonical,selected,true,now);
    if(okay){
        source::MotionHistory seed{},merged{};seed.count=1;seed.frames[0]=current.raw;
        okay=history_buffer.merge(current.raw,current.publisher,seed,merged);
    }
    report(f,okay?0u:3u,okay);
    if(okay)previous=current;else{stroke.reset();history_buffer.clear();}
    return okay;
}
bool advance(hand::Frame const& f,gears_weapon::BoltokReloadStamp const& s,V local,std::int64_t now) noexcept {
    unsigned gate=1;
    gears_weapon::BoltokSwingStroke::OriginalMotionRejection rejection{};
    const bool ready=[&](){
        if(!same_context(previous,current))return false;
        gate=2;if(!input.matched||!hand::same(input.key,f.key))return false;
        gate=3;if(input.publisher.pid!=current.publisher.pid||input.publisher.start!=current.publisher.start||
            !source::same_tracking(input.raw,current.raw))return false;
        gate=4;return stroke.move_original_motion(s,local,f.press_token,true,now,current.raw,input.history,current.actual,true,&rejection);
    }();
    report(f,ready?0u:gate,ready);
    if(!ready&&gate==1)report_context_difference(f,previous,current);
    if(!ready&&gate==4)report_rejection(f,rejection);
    if(ready)previous=current;else{stroke.reset();previous={};}
    return ready;
}
bool can_wait_for_pose(hand::Frame const& f) noexcept {
    if(!moving||!stroke.original_motion_bound()||!previous.valid||current.valid||current.gate!=8||
       !hand::same(current.frame.key,f.key)||!same_domain(previous,current)||
       f.key.frame<previous.frame.key.frame||f.controllers.sequence<previous.frame.controllers.sequence||
       !input.matched||!hand::same(input.key,f.key))return false;
    unsigned selected{};if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))selected=gears_vr::control_squeeze;
    else if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))selected=gears_vr::control_trigger;
    else return false;
    if(input.publisher.pid!=current.publisher.pid||input.publisher.start!=current.publisher.start||
       !source::same_tracking(input.raw,current.raw))return false;
    if(!source::same_input(f.controllers,current.frame.controllers))return false;
    relative::Segment segment{};
    return relative::build(previous.raw,input.raw,input.history,selected,previous.canonical,1,segment);
}
}
