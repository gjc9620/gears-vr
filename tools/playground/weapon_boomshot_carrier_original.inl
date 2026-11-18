#include "../vr-common/weapon_boomshot_insertion.h"
// Reuse the ORIGINAL MagazineMesh0/Notify_AmmoGrab and original detach call.
// No clone pose, new component, guessed hand transform or inventory mutation.
#if defined(GEARS_PLAYGROUND_BODY_MAIN_MOUNT_PROVIDER_TRIAL)
namespace bodymountprovider {
bool boomshot_model_scale(gears_weapon_hand::Frame const&,gears_rig_tracking::Frame const&,double&) noexcept;
}
#endif

namespace boomshot_carrier_original {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;namespace meta=gripdamage_metadata;
#include "weapon_boomshot_carrier_calls.inl"
#include "weapon_boomshot_carrier_component.inl"
enum class RawResult {unavailable,pending,ready};
class Owner {
    gears_weapon::WeaponMeshComponentLease<boomshot_carrier_component::Key> visual_lease_{};
    double visual_units_{},visual_scale_{};
    inv::Identity identity_{};hand::ObjectIdentity pawn_mesh_{},component_{},mesh_{};inv::Counts counts_{};
    std::uint64_t serial_{},item_{};bool active_{},dispatched_{},detach_sent_{},detach_returned_{},seat_sent_{},busy_{};
    hand::ObjectIdentity gun_component_{},gun_asset_{};
    gears_vr::Pose mount_candidate_{},raw_mount_{},candidate_left_{};gears_rig_tracking::Frame mount_cohort_{};
    std::uint64_t grabbed_frame_{},candidate_frame_{};bool mount_candidate_known_{},raw_mount_known_{};
    double mount_scale_candidate_{},raw_mount_scale_{};
    gears_boomshot_owner_grabposes::AmmoCarrierRelation authored_relation_{};
    bool authored_relation_known_{};double raw_hand_scale_{};
    hand::ObjectIdentity ammo_control_{};bool hidden_before_{},hide_sent_{},hide_ack_{},restore_sent_{};
    bool grab_returned_{},seat_returned_{};inv::Inventory seat_counts_{};
    bool ready_reported_{}; // Observation only; never participates in admission.
    RawResult ready_result(hand::Frame const& f,bool actual_ready) noexcept {
        if(!actual_ready)return RawResult::pending;
        static unsigned rows{};
        if(!ready_reported_&&log_file){
            ready_reported_=true;
            if(rows<32){++rows;
                fwprintf(log_file,L"boomshot_carrier_ready serial=%llu item=%llu game_start=%llu native_start=%llu registration=%llu lifecycle=%llu world_generation=%llu hold_generation=%llu session=%llu space=%llu input=%llu world=%llx pawn=%llx weapon=%llx controller=%llx component=%llx component_outer=%llx component_class=%llx component_index=%u component_name=%u component_number=%u asset=%llx asset_outer=%llx asset_class=%llx asset_index=%u asset_name=%u asset_number=%u frame=%llu grabbed_frame=%llu sequence=%llu sync_ns=%lld reference=%llu profile=%llu source=%u attached_verified=1 original_grab_returned=1 hide_ack=1 observation_only=1\n",
                    serial_,item_,identity_.game_start,identity_.native_start,identity_.registration,identity_.lifecycle,
                    identity_.world_generation,identity_.hold_generation,identity_.session,identity_.space,identity_.input,
                    identity_.world.address,identity_.pawn.address,identity_.weapon.address,f.controller.address,
                    component_.address,component_.outer,component_.klass,component_.registry_index,component_.name_index,component_.name_number,
                    mesh_.address,mesh_.outer,mesh_.klass,mesh_.registry_index,mesh_.name_index,mesh_.name_number,
                    f.key.frame,grabbed_frame_,f.controllers.sequence,f.controllers.sync_ns,
                    f.reference_generation,f.controllers.profile_generation,unsigned(f.controllers.source));
                fflush(log_file);
            }
        }
        return RawResult::ready;
    }
    struct Busy {bool& flag;explicit Busy(bool& f):flag(f){flag=true;}~Busy(){flag=false;}};
    bool context(bool counts) const noexcept {
        if(!active_||!weaponhand::current_gt(nullptr)||!lancerreload_state::retains_serial(serial_)||
           !(lancerreload_state::lease.identity==identity_)||!lancerreload_state::owned_state())return false;
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
        if(!meta::tables(base,snapshot.objects,snapshot.names))return false;
        for(const auto& object:{identity_.world,identity_.pawn,identity_.weapon,pawn_mesh_,component_,mesh_})if(!weaponhand::object(r,object))return false;
        meta::PointerField pawn_mesh{},component{},asset{};inv::Counts actual{};
        return weaponhand::pointer(r,identity_.pawn.address,L"Mesh",pawn_mesh_.address,pawn_mesh,true)&&
            weaponhand::pointer(r,identity_.weapon.address,L"MagazineMesh",component_.address,component,true)&&
            weaponhand::pointer(r,component_.address,L"StaticMesh",mesh_.address,asset)&&
            (!counts||(inv::query(identity_.weapon.address,actual)&&actual==counts_))&&unified_weapon_fire::retained_metadata(r);
    }
    bool attached(bool& value) const noexcept {
        value=false;if(!context(false))return false;playground::Args call(pawn_mesh_.address,L"IsComponentAttached");
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};Layout before{},after{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||
           !signature(call,pawn_mesh_.address,L"IsComponentAttached",L"SkeletalMeshComponent",true,2,r,before)||
           !unified_weapon_fire::owner_in_receiver(r,component_.address,before.component_class)||
           !call.set(L"Component",component_.address).valid||!context(false)||!call.run()||
           !signature(call,pawn_mesh_.address,L"IsComponentAttached",L"SkeletalMeshComponent",true,2,r,after)||!(before==after)||!context(false))return false;
        unsigned bits{};std::memcpy(&bits,call.r.params+before.offsets[2],4);value=(bits&before.mask)!=0;return true;
    }
    bool scale() const noexcept {
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};weaponhand::BoolField field{};
        float actual{},again{};const float wanted=playground::instance(identity_.pawn.address,L"Pawn_GeistBoomer")?1.f:.75f;
        return context(true)&&meta::tables(base,snapshot.objects,snapshot.names)&&weaponhand::object(r,component_)&&
            weaponhand::field(r,component_.address,L"Scale",L"FloatProperty",4,field)&&r.at(component_.address,field.property.offset,actual)&&
            actual==wanted&&r.at(component_.address,field.property.offset,again)&&again==actual&&
            unified_weapon_fire::retained_metadata(r)&&context(true);
    }
    static gears_vr::Vector sub(gears_vr::Vector a,gears_vr::Vector b) noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
    static double distance(gears_vr::Vector a,gears_vr::Vector b) noexcept {const auto d=sub(a,b);return std::hypot(std::hypot(d.x,d.y),d.z);}
    static double angle(gears_vr::Quaternion a,gears_vr::Quaternion b) noexcept {
        return 2*std::acos((std::clamp)(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w),0.,1.))*180/3.14159265358979323846;
    }
    bool world_pose(gears_vr::Pose& pose,double& measured_scale) const noexcept {
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
        boomshot_socket::Reads reads{};boomshot_pose::Matrix before{},after{};weaponhand::BoolField field{};double scale{};
        if(!context(true)||!meta::tables(base,snapshot.objects,snapshot.names))return false;
        const auto read=[&](boomshot_pose::Matrix& matrix) noexcept {
            return reads.field(r,component_.address,L"LocalToWorld",L"StructProperty",64,field)&&
                boomshot_pose::declared(r,field,L"PrimitiveComponent")&&boomshot_pose::matrix_type(r,reads,field.property)&&
                boomshot_pose::matrix_bytes(r,reads,component_.address,field.property.offset,matrix);
        };
        if(!read(before))return false;reads.repeat=true;
        if(!read(after)||after!=before||reads.cursor!=reads.count||reads.field_cursor!=reads.field_count||
           !unified_weapon_fire::retained_metadata(r)||!context(true)||!hammerburst_runtime::matrix(before,pose,scale))return false;
        if(!gears_vr::stereo_pose_valid(pose))return false;measured_scale=scale;return true;
    }
    bool ammo_state(bool& hidden,bool bind=false) noexcept {
        hidden=false;vrequip::AmmoBoneVisualProbe probe{};
        if(!context(false)||!vrequip::ammo_bone_visual_probe(identity_.weapon.address,probe)||!probe.present||
           probe.mesh!=gun_component_.address||probe.bone_scale!=0||probe.strength!=probe.target||
           (probe.strength!=0&&probe.strength!=1))return false;
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};hand::ObjectIdentity actual{};
        weaponhand::BoolField named{};meta::RawName name{},again{};meta::NameText text{};meta::PointerField mesh{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||!lancermain_asset::identify(r,probe.control,actual)||
           !weaponhand::derives(r,actual.address,L"SkelControlSingleBone",L"Engine")||
           (!bind&&!hand::same(actual,ammo_control_))||!weaponhand::object(r,gun_component_)||
           !weaponhand::pointer(r,identity_.weapon.address,L"Mesh",gun_component_.address,mesh,true)||
           !weaponhand::field(r,actual.address,L"ControlName",L"NameProperty",8,named)||
           !r.at(actual.address,named.property.offset,name)||name.number||!meta::fresh_name(snapshot.names,name.index,text)||
           !meta::exact_text(text,L"AmmoControl")||!r.at(actual.address,named.property.offset,again)||
           again.index!=name.index||again.number!=name.number)return false;
        for(const auto& entry:{std::pair{L"ControlStrength",probe.strength},std::pair{L"StrengthTarget",probe.target},
            std::pair{L"BoneScale",0.f},std::pair{L"BlendInTime",0.f},std::pair{L"BlendOutTime",0.f}}){
            // field() takes a literal array; the generic original typed float
            // reader repeats its actual descriptor width/offset and values.
            float first{},last{};
            if(!vrequip::read_control_float(actual.address,entry.first,first)||first!=entry.second||
               !vrequip::read_control_float(actual.address,entry.first,last)||last!=first)return false;
        }
        if(!unified_weapon_fire::retained_metadata(r)||!context(false))return false;
        if(bind)ammo_control_=actual;hidden=probe.strength==1;return true;
    }
    bool display(bool show,bool& sent) noexcept {
        if(sent||!context(false))return false;
        playground::Args call(identity_.weapon.address,L"SetWeaponAmmoBoneDisplay");
        hand::ObjectIdentity before{},after{};const auto property=call.prop(L"bShow");
        if(!call.valid||call.r.size!=4||!property||get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=4||
           get<unsigned>(property+0x94)!=0||(get<unsigned long long>(property+0x78)&0x580)!=0x80||
           (get<unsigned>(call.r.function+0xf4)&0x400)||get<std::uint16_t>(call.r.function+0xf8)||
           get<std::uint16_t>(call.r.function+0x106)!=4||get<std::uint16_t>(call.r.function+0x108)!=0xffff||
           !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr))||
           !lancerreload_ui::original_function(call.r.function,L"SetWeaponAmmoBoneDisplay",L"WarWeapon",false)||
           !nativeequipment::identify(nullptr,call.r.function,before)||!nativelancerfollow::bool_arg(call,L"bShow",show)||!context(false))return false;
        sent=true;const bool returned=call.run();
        return returned&&nativeequipment::identify(nullptr,call.r.function,after)&&hand::same(before,after)&&
            lancerreload_ui::original_function(call.r.function,L"SetWeaponAmmoBoneDisplay",L"WarWeapon",false)&&
            context(false);
    }
    bool restore_visibility() noexcept {
        bool hidden{};if(!ammo_state(hidden))return false;
        if(hidden==hidden_before_)return true; // Read-only reconciliation, never redispatch an uncertain setter.
        if(!display(!hidden_before_,restore_sent_))return false;
        return ammo_state(hidden)&&hidden==hidden_before_;
    }
    void clear() noexcept {restore_visual();authored_relation_known_=false;identity_={};pawn_mesh_={};component_={};mesh_={};counts_={};serial_=item_=0;active_=dispatched_=detach_sent_=detach_returned_=seat_sent_=false;
        gun_component_={};gun_asset_={};mount_candidate_={};raw_mount_={};candidate_left_={};mount_cohort_={};grabbed_frame_=candidate_frame_=0;mount_candidate_known_=raw_mount_known_=false;
        mount_scale_candidate_=raw_mount_scale_=0;
        authored_relation_={};authored_relation_known_=false;
        ammo_control_={};hidden_before_=hide_sent_=hide_ack_=restore_sent_=grab_returned_=seat_returned_=ready_reported_=false;seat_counts_={};}
public:
    Owner()=default;Owner(Owner const&)=delete;Owner& operator=(Owner const&)=delete;
    bool active()const noexcept{return active_;}
    bool matches(hand::Frame const& f)const noexcept{return active_&&boomshot_sessions::same_actor(identity_,inv::identity(f));}
    bool holding(hand::Frame const& f,std::uint64_t serial,std::uint64_t item) const noexcept {
        bool actual{};return !busy_&&!detach_sent_&&!seat_sent_&&dispatched_&&grab_returned_&&hide_ack_&&serial==serial_&&item==item_&&
            identity_==inv::identity(f)&&context(true)&&nativeequipment::current(nullptr,f)&&attached(actual)&&actual;
    }
    bool visual_pose(hand::Frame const& f,std::uint64_t serial,std::uint64_t item,gears_vr::Pose& out,double& scale_out) const noexcept {
        // Read-only original attachment witness. Its fitted/component pose is
        // never an insertion sample or a new target for its own parent hand.
        gears_vr::Pose value{};double size{};
        if(!holding(f,serial,item)||!world_pose(value,size)||!holding(f,serial,item))return false;
        out=value;scale_out=size;return true;
    }
    // Calibrate ONLY the held component-to-RAW-controller mount. This does not
    // certify its seated transform. Once bound, animated/snapped component
    // positions are never used as continuous physical insertion input.
    RawResult sample_raw(hand::Frame const& f,std::uint64_t serial,std::uint64_t item,
        boomshot_core_source::Snapshot const& source,gears_vr::Pose& out,
        gears_weapon::boomshot_insertion::Geometry const* snap_geometry=nullptr) noexcept {
        const auto unavailable=[&](unsigned line)noexcept{static unsigned last{},rows{};if(log_file&&line!=last&&rows++<12){fwprintf(log_file,L"boomshot_raw_unavailable line=%u frame=%llu\n",line,f.key.frame);fflush(log_file);last=line;}return RawResult::unavailable;};
        out={};const auto now=weaponhand::clock_ns(nullptr);const auto& rig=source.rig;const auto& left=rig.world_grip[0];
        if(!active_||serial!=serial_||item!=item_||detach_sent_||seat_sent_||!context(false)||!(source.context==identity_)||
           gears_equipment_rig::match(f,rig,now,unsigned(f.stereo.source))!=gears_equipment_rig::Status::matched||
           !std::isfinite(rig.units_per_metre)||rig.units_per_metre<10||rig.units_per_metre>500)return unavailable(__LINE__);
        const auto units=rig.units_per_metre;
        if(!raw_mount_known_){
            if(!snap_geometry||!configure_relation(f,*snap_geometry))return RawResult::pending;
        }
#if defined(GEARS_PLAYGROUND_BODY_MAIN_MOUNT_PROVIDER_TRIAL)
        double current_model_scale{};
        if(!bodymountprovider::boomshot_model_scale(f,rig,current_model_scale)||current_model_scale!=raw_hand_scale_)
            return unavailable(__LINE__);
#endif
        const auto offset=gears_weapon::rotate(left.orientation,raw_mount_.position);
        const gears_vr::Pose carrier{{left.position.x+offset.x*units,left.position.y+offset.y*units,left.position.z+offset.z*units},
            gears_vr::normalized(gears_vr::multiply(left.orientation,raw_mount_.orientation))};
        hammerburst_animation::Parts parts{};gears_vr::Pose root{};double scale{};
        if(!boomshot_parts::read(f,parts)||!hand::same(parts.component,gun_component_)||!hand::same(parts.asset,gun_asset_)||
           !hammerburst_runtime::matrix(parts.world[0],root,scale)||!std::isfinite(scale)||scale<=0||
           !std::isfinite(raw_mount_scale_)||std::abs(raw_mount_scale_/scale-1)>.002)return unavailable(__LINE__);
        const auto position=gears_weapon::rotate(gears_vr::inverse(root.orientation),sub(carrier.position,root.position));
        out={{position.x/(scale*units),position.y/(scale*units),position.z/(scale*units)},
            gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(root.orientation),carrier.orientation))};
        if(!gears_vr::stereo_pose_valid(out)||!matches(f)||
           gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),unsigned(f.stereo.source))!=gears_equipment_rig::Status::matched){out={};return unavailable(__LINE__);}
        if(snap_geometry&&log_file){
            static std::uint64_t logged_item{};
            if(logged_item!=item&&gears_weapon::boomshot_insertion::valid(*snap_geometry)){
                const auto seated=snap_geometry->seated_in_root_m;
                const auto delta=gears_weapon::rotate(root.orientation,{seated.position.x*scale*units,seated.position.y*scale*units,seated.position.z*scale*units});
                const auto target_rotation=gears_vr::normalized(gears_vr::multiply(gears_vr::multiply(root.orientation,seated.orientation),gears_vr::inverse(raw_mount_.orientation)));
                const auto hand_delta=gears_weapon::rotate(target_rotation,raw_mount_.position);
                const gears_vr::Vector target{root.position.x+delta.x-hand_delta.x*units,root.position.y+delta.y-hand_delta.y*units,root.position.z+delta.z-hand_delta.z*units};
                const auto v=snap_geometry->outward;
                const auto outward=gears_weapon::rotate(root.orientation,{v.x*scale*units*snap_geometry->travel_m,v.y*scale*units*snap_geometry->travel_m,v.z*scale*units*snap_geometry->travel_m});
                const auto raw=f.controllers.hands[1].grip,world=rig.world_grip[1];
                fwprintf(log_file,L"boomshot_demo_snap item=%llu sequence=%llu units=%.9g px=%.9g py=%.9g pz=%.9g pqx=%.9g pqy=%.9g pqz=%.9g pqw=%.9g dx=%.9g dy=%.9g dz=%.9g rx=%.9g ry=%.9g rz=%.9g rqx=%.9g rqy=%.9g rqz=%.9g rqw=%.9g wx=%.9g wy=%.9g wz=%.9g wqx=%.9g wqy=%.9g wqz=%.9g wqw=%.9g\n",
                    item,f.controllers.sequence,units,target.x,target.y,target.z,target_rotation.x,target_rotation.y,target_rotation.z,target_rotation.w,outward.x,outward.y,outward.z,
                    raw.position.x,raw.position.y,raw.position.z,raw.orientation.x,raw.orientation.y,raw.orientation.z,raw.orientation.w,
                    world.position.x,world.position.y,world.position.z,world.orientation.x,world.orientation.y,world.orientation.z,world.orientation.w);fflush(log_file);logged_item=item;
            }
        }
        return RawResult::ready;
    }
    // Only an already committed q1 inventory receipt may request the ORIGINAL
    // visual-seat notify. It detaches this component and shows the gun's ammo
    // bone; it supplies no insertion geometry, tracking or inventory authority.

    bool restore_visual() noexcept {
        if(visual_lease_.dirty()){
            boomshot_carrier_component::Backend backend(visual_lease_.key());
            if(!visual_lease_.restore(backend))return false;
        }
        if(visual_lease_.has_snapshot())visual_lease_.reset();
        return !visual_lease_.faulted();
    }
    bool configure_relation(hand::Frame const& f,gears_weapon::boomshot_insertion::Geometry const& geometry) noexcept {
        if(!active_||detach_sent_||seat_sent_||!dispatched_||!grab_returned_||!hide_ack_||
           !gears_weapon::boomshot_insertion::valid(geometry)||!(inv::identity(f)==identity_)||!context(false))return false;
        if(authored_relation_known_)return true;
        hammerburst_animation::Parts parts{};gears_vr::Pose root_pose{},component_pose{};double root_scale{},component_scale{};
        if(!boomshot_parts::read(f,parts)||!hand::same(parts.component,gun_component_)||!hand::same(parts.asset,gun_asset_)||
           !hammerburst_runtime::matrix(parts.world[0],root_pose,root_scale)||!world_pose(component_pose,component_scale))return false;
        double model_scale{};
#if defined(GEARS_PLAYGROUND_BODY_MAIN_MOUNT_PROVIDER_TRIAL)
        gears_rig_tracking::Frame tracking{};
        const auto source=unsigned(f.stereo.source);
        if(handrig::read(tracking,0,source).reply!=gears_rig_tracking::Reply::accepted||
           gears_equipment_rig::match(f,tracking,weaponhand::clock_ns(nullptr),source)!=gears_equipment_rig::Status::matched||
           !bodymountprovider::boomshot_model_scale(f,tracking,model_scale))return false;
#else
        // Earlier builds lack the certified body-model publication.
        gears_committed_interaction::Frame actual{};
        if(!handrig::read_interaction(actual)||!hand::same(actual.right.weapon,f.weapon)||
           !hand::same(actual.right.mesh,f.mesh)||actual.model_scale<.01||actual.model_scale>10)return false;
        model_scale=actual.model_scale;
#endif
        if(!nativeequipment::current(nullptr,f))return false;
        const double units=nativelancerfollow::bound_units;
#if defined(GEARS_PLAYGROUND_BODY_MAIN_MOUNT_PROVIDER_TRIAL)
        if(units!=tracking.units_per_metre)return false;
#endif
        if(!gears_boomshot_owner_grabposes::ammo_wrist_relation(geometry.seated_in_root_m,component_scale/root_scale,units,authored_relation_))return false;
        gears_vr::Pose local{};
        if(!gears_boomshot_owner_grabposes::ammo_carrier_from_grip({{},{}},authored_relation_,component_scale,units,model_scale,local))return false;
        raw_mount_={{local.position.x/units,local.position.y/units,local.position.z/units},local.orientation};
        raw_mount_scale_=component_scale;raw_hand_scale_=model_scale;raw_mount_known_=true;authored_relation_known_=true;
        visual_units_=units;visual_scale_=component_scale;
        if(log_file){fwprintf(log_file,L"boomshot_authored_carrier_relation serial=%llu item=%llu units=%.9g scale=%.9g hand_scale=%.9g capture=1794499171798-4\n",serial_,item_,units,component_scale,model_scale);fflush(log_file);}
        return true;
    }
    bool render_visual(gears_committed_hand::Frame const& result) noexcept {
        if(!active_||!authored_relation_known_||detach_sent_||seat_sent_||!context(false)||
           result.hold_generation!=identity_.hold_generation||!hand::same(result.weapon,identity_.weapon)||
           !hand::same(result.pawn,identity_.pawn)||!hand::same(result.world,identity_.world)||
           !gears_committed_hand::current_result(result,weaponhand::clock_ns(nullptr),result.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated))return false;
        gears_committed_interaction::Frame actual{};
        if(!handrig::read_interaction(actual)||actual.right.serial!=result.serial||actual.right.tick!=result.tick||
           actual.right.compose!=result.compose||actual.right.units_per_metre!=visual_units_||!restore_visual())return false;
        const auto& wrist=actual.left_wrist;
        const auto orientation=gears_vr::normalized(gears_vr::multiply(wrist.orientation,gears_vr::inverse(authored_relation_.wrist_in_carrier.orientation)));
        const auto& local=authored_relation_.wrist_in_carrier.position;
        const auto delta=gears_weapon::rotate(orientation,{local.x*visual_scale_*visual_units_,local.y*visual_scale_*visual_units_,local.z*visual_scale_*visual_units_});
        const playground::Vector position{float(wrist.position.x-delta.x),float(wrist.position.y-delta.y),float(wrist.position.z-delta.z)};
        const auto angles=gears_vr::ue_angles(gears_vr::rotation_matrix(orientation));
        const playground::Rotation rotation{gears_vr::to_rotator_units(angles.pitch),gears_vr::to_rotator_units(angles.yaw),gears_vr::to_rotator_units(angles.roll)};
        const boomshot_carrier_component::Key key{identity_.world,identity_.pawn,identity_.weapon,component_,mesh_,serial_,identity_.hold_generation,item_};
        boomshot_carrier_component::Backend backend(key);boomshot_carrier_component::State state{};
        if(!backend.read(state))return false;
        auto wanted=state.value;wanted.absolute[0]=wanted.absolute[1]=true;
        std::memcpy(wanted.translation.data(),&position,12);std::memcpy(wanted.rotation.data(),&rotation,12);
        const bool applied=visual_lease_.acquire(key,wanted,backend);
        if(log_file){static unsigned rows{};if(rows++<16){fwprintf(log_file,L"boomshot_carrier_render serial=%llu item=%llu compose=%llu applied=%u position=%.9g,%.9g,%.9g\n",serial_,item_,result.compose,unsigned(applied),double(position.x),double(position.y),double(position.z));fflush(log_file);}}
        return applied;
    }
    bool seat_owned(std::uint64_t serial,std::uint64_t item,inv::Inventory const& committed) noexcept {
        if(!restore_visual())return false;
        if(seat_sent_)return reconcile_seat();
        if(busy_||!active_||detach_sent_||!dispatched_||!grab_returned_||serial!=serial_||item!=item_||
           !committed.valid()||!(committed.identity==identity_)||committed.capacity!=1||committed.loaded!=1||
           counts_.capacity!=1||counts_.used!=1||committed.infinite_reserve!=counts_.infinite||
           (counts_.infinite?committed.reserve!=unsigned(counts_.reserve):
            (!counts_.reserve||committed.reserve!=unsigned(counts_.reserve-1))))return false;
        Busy guard(busy_);const auto unchanged=[&]() noexcept {
            inv::Counts actual{};return context(false)&&inv::query(identity_.weapon.address,actual)&&
                actual.capacity==1&&actual.used==0&&unsigned(actual.reserve)==committed.reserve&&actual.infinite==committed.infinite_reserve;
        };
        playground::Args call(identity_.weapon.address,L"Notify_AmmoReload");
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};Layout before{},after{};
        bool actual{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||!unchanged()||!attached(actual)||!actual||
           !signature(call,identity_.weapon.address,L"Notify_AmmoReload",L"Weap_Boomer_base",false,0,r,before)||
           !unchanged())return false;
        seat_counts_=committed;seat_sent_=true;const bool returned=call.run();
        seat_returned_=returned&&signature(call,identity_.weapon.address,L"Notify_AmmoReload",L"Weap_Boomer_base",false,0,r,after)&&before==after;
        // The original active-strength/attachment result may become visible
        // on a later GT update. Keep its receipt, never replay the notify.
        return false;
    }
    bool seat_pending()const noexcept{return active_&&seat_sent_&&seat_returned_;}
    bool reconcile_seat() noexcept {
        if(busy_)return false;if(!active_)return true;Busy guard(busy_);
        if(!seat_sent_||!seat_returned_||!context(false)||!(seat_counts_.identity==identity_)||!seat_counts_.valid())return false;
        inv::Counts counts{};bool actual{},hidden{};
        if(!inv::query(identity_.weapon.address,counts)||counts.capacity!=1||counts.used!=0||
           unsigned(counts.reserve)!=seat_counts_.reserve||counts.infinite!=seat_counts_.infinite_reserve||
           !attached(actual)||actual||!ammo_state(hidden)||hidden||!context(false))return false;
        clear();return true;
    }
    bool seat(hand::Frame const& f,std::uint64_t serial,std::uint64_t item,inv::Inventory const& committed) noexcept {
        if(!weaponhand::current_gt(nullptr)||!(identity_==inv::identity(f))||!nativeequipment::current(nullptr,f))return false;
        if(seat_owned(serial,item,committed))return true;
        return seat_pending()&&reconcile_seat();
    }
    bool begin(hand::Frame const& f,std::uint64_t serial,std::uint64_t item,boomshot_original::Source const& source) noexcept {
        const auto rejected=[&](unsigned line)noexcept{if(log_file){fwprintf(log_file,L"boomshot_carrier_begin_reject line=%u frame=%llu\n",line,f.key.frame);fflush(log_file);}return false;};
        if(busy_||active_||!serial||!item||!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,f)||
           !lancerreload_state::owns(f,serial))return rejected(__LINE__);Busy guard(busy_);
        identity_=inv::identity(f);pawn_mesh_=f.mesh;component_=source.carrier_component;mesh_=source.carrier;counts_=source.counts;serial_=serial;item_=item;active_=true;
        gun_component_=source.parts.component;gun_asset_=source.parts.asset;grabbed_frame_=f.key.frame;
        bool already{};
        const bool current=context(true),attachment=current&&attached(already);
        if(!current||!attachment){
            if(log_file){fwprintf(log_file,L"boomshot_carrier_context current=%u attachment=%u already=%u\n",unsigned(current),unsigned(attachment),unsigned(already));fflush(log_file);}
            clear();return rejected(__LINE__);
        }
        // Original ForceReload may already have delivered its AmmoGrab notify.
        // Adopt that exact attached component rather than dispatching it twice.
        if(already){
            if(!ammo_state(hidden_before_,true)){clear();return rejected(__LINE__);}
            dispatched_=grab_returned_=true;hide_ack_=hidden_before_;
            if(!hidden_before_&&!display(false,hide_sent_))return rejected(__LINE__);
            return true;
        }
        playground::Args call(f.weapon.address,L"Notify_AmmoGrab");
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};Layout before{},after{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||
           !signature(call,f.weapon.address,L"Notify_AmmoGrab",L"Weap_Boomer_base",false,0,r,before)||
           !context(true)||!nativeequipment::current(nullptr,f)){clear();return rejected(__LINE__);}
        if(!ammo_state(hidden_before_,true)){clear();return rejected(__LINE__);}
        if(!context(true)||!nativeequipment::current(nullptr,f)){clear();return rejected(__LINE__);}
        if(!hidden_before_&&!display(false,hide_sent_))return rejected(__LINE__);
        return true; // Adopted; service() observes hide then attaches on a fresh real frame.
    }
    RawResult service(hand::Frame const& f,std::uint64_t serial,std::uint64_t item) noexcept {
        if(busy_||!active_||serial!=serial_||item!=item_||detach_sent_||seat_sent_||!(identity_==inv::identity(f))||
           !context(true)||!nativeequipment::current(nullptr,f))return RawResult::unavailable;
        Busy guard(busy_);bool hidden{};
        if(!hide_ack_){if(!ammo_state(hidden)||!hidden)return RawResult::pending;hide_ack_=true;}
        if(dispatched_){
            if(!grab_returned_)return RawResult::unavailable;bool actual{};
            return ready_result(f,attached(actual)&&actual&&scale());
        }
        if(!nativeequipment::current(nullptr,f))return RawResult::unavailable;
        // Durable before the ORIGINAL call. A lost receipt never replays it.
        playground::Args call(f.weapon.address,L"Notify_AmmoGrab");
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};Layout before{},after{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||
           !signature(call,f.weapon.address,L"Notify_AmmoGrab",L"Weap_Boomer_base",false,0,r,before)||
           !context(true)||!nativeequipment::current(nullptr,f))return RawResult::unavailable;
        grabbed_frame_=f.key.frame;dispatched_=true;const bool returned=call.run();
        grab_returned_=returned&&signature(call,f.weapon.address,L"Notify_AmmoGrab",L"Weap_Boomer_base",false,0,r,after)&&before==after;
        if(!grab_returned_)return RawResult::unavailable;bool actual{};
        return ready_result(f,context(true)&&attached(actual)&&actual&&scale());
    }
    bool detach(bool restore_embedded=true) noexcept {
        if(!restore_visual())return false;
        if(busy_)return false;if(!active_)return true;if(seat_sent_)return reconcile_seat();Busy guard(busy_);if(!context(false))return false;
        if(dispatched_&&!detach_sent_){
            playground::Args call(identity_.weapon.address,L"SetPawnAmmoAttachment");
            meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};Layout layout{};
            if(!meta::tables(base,snapshot.objects,snapshot.names)||
               !signature(call,identity_.weapon.address,L"SetPawnAmmoAttachment",L"WarWeapon",false,1,r,layout)||
               !nativelancerfollow::bool_arg(call,L"bAttach",false)||!context(false))return false;
            Layout after{};detach_sent_=true;const bool returned=call.run();
            detach_returned_=returned&&signature(call,identity_.weapon.address,L"SetPawnAmmoAttachment",L"WarWeapon",false,1,r,after)&&layout==after;
        }
        bool actual{};
        if((dispatched_&&!detach_returned_)||!context(false)||!attached(actual)||actual||
           (restore_embedded&&!restore_visibility()))return false;
        clear();return true;
    }
};
Owner owner;
bool restore_visual() noexcept {return owner.restore_visual();}
bool render_visual(gears_committed_hand::Frame const& f) noexcept {return owner.render_visual(f);}
}
