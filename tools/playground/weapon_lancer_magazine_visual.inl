namespace lancerreload_visual {
namespace hand=gears_weapon_hand;
struct Snapshot {
    hand::ObjectIdentity weapon{},pawn{},component{},asset{};
    playground::Vector translation{};playground::Rotation rotation{};
    bool absolute_translation{},absolute_rotation{},absolute_scale{},hidden{};
    bool precomputed_shadows{},dynamic_shadow_owned{};
};
Snapshot saved;
Snapshot restored;
float restored_reload_start{};
bool restored_known{};
bool active{};
gears_weapon::LancerMagazineGraspMount pinch_mount;
std::uint32_t pinch_mount_press{},pinch_mount_release{};
gears_vr::Viewpoint last_lancer_target{};bool last_lancer_target_known{};
bool insertion_assist_enabled{};
bool extraction_display_enabled{};
gears_weapon::LancerMagazineInsertionAssist insertion_assist;
void set_insertion_assist(bool requested) noexcept {
    if(requested!=insertion_assist_enabled)insertion_assist.reset();
    insertion_assist_enabled=requested;
}
gears_vr::Vector last_grab_world{};
bool last_grab_known{};
unsigned interaction_failure{};
bool interaction(hand::Frame const& source,gears_committed_interaction::Frame& out) noexcept {
    out={};gears_committed_interaction::Frame candidate{};
    interaction_failure=1;
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_lancer||
       !hand::same(source.weapon,nativelancerfollow::bound_key.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation())return false;
    interaction_failure=2;if(!handrig::read_interaction(candidate))return false;
    interaction_failure=3;
    if(!gears_committed_interaction::matches(candidate,source,vrequip::model.hold_generation(),
            weaponhand::clock_ns(nullptr),source.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated))return false;
    out=candidate;interaction_failure=0;return true;
}
bool same_object(hand::ObjectIdentity const& wanted) noexcept {
    hand::ObjectIdentity actual{};
    return nativeequipment::identify(nullptr,wanted.address,actual)&&hand::same(wanted,actual);
}
bool alive() noexcept {
    return active&&same_object(saved.weapon)&&same_object(saved.pawn)&&
        same_object(saved.component)&&same_object(saved.asset)&&
        playground::pointer(saved.weapon.address,L"MagazineMesh")==saved.component.address&&
        playground::pointer(saved.component.address,L"StaticMesh")==saved.asset.address&&
        playground::pointer(saved.weapon.address,L"Instigator")==saved.pawn.address;
}
template<class T>bool read_struct(uptr object,const wchar_t* field,T& out) noexcept;
bool physical_wrist(gears_committed_interaction::Frame const& actual,gears_vr::Pose& out) noexcept {
    gears_hand_authored_mount::Mount mount{};
    return gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::left,actual.model_scale,actual.right.units_per_metre,mount)&&
        gears_hand_wrist::native_wrist_target(actual.physical_left_grip,mount.calibration,
            actual.right.units_per_metre,out)==gears_hand_wrist::Status::okay;
}
void observe_actual_pair(gears_committed_interaction::Frame const& actual) noexcept {
    static hand::ObjectIdentity observed_component{};static std::uint64_t sequence{};static unsigned rows{};
    if(!hand::same(observed_component,saved.component)){observed_component=saved.component;sequence=0;rows=0;}
    if(!log_file||rows>=128||actual.right.serial==sequence||!alive())return;
    // Existing retail PrimitiveComponent LocalToWorld layout, also used by
    // held-gun readback. Observe the ACTUAL delayed matrix on the game thread,
    // bracket it, and never treat a component setter's arguments as a result.
    std::array<float,16> matrix{},after{};
    if(!copy(matrix.data(),saved.component.address+0xa0,sizeof(matrix))||!alive()||
       !copy(after.data(),saved.component.address+0xa0,sizeof(after))||matrix!=after)return;
    for(const auto value:matrix)if(!std::isfinite(value))return;
    gears_vr::Pose wrist{};if(!physical_wrist(actual,wrist))return;
    playground::Vector translation{};playground::Rotation rotation{};bool abs_t{},abs_r{};
    const bool fields=read_struct(saved.component.address,L"Translation",translation)&&
        read_struct(saved.component.address,L"Rotation",rotation)&&
        vrequip::read_bool(saved.component.address,L"AbsoluteTranslation",abs_t)&&
        vrequip::read_bool(saved.component.address,L"AbsoluteRotation",abs_r);
    gears_vr::Viewpoint expected{};
    if(!gears_weapon::lancer_magazine_from_left({wrist.position,
        gears_vr::ue_angles(gears_vr::rotation_matrix(wrist.orientation))},expected,actual.model_scale))return;
    fwprintf(log_file,L"lancer_magazine_actual serial=%llu sequence=%llu wrist=%.9g,%.9g,%.9g expected=%.9g,%.9g,%.9g expected_rotation=%.9g,%.9g,%.9g actual=%.9g,%.9g,%.9g axes=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
        actual.right.serial,actual.right.controllers.sequence,wrist.position.x,wrist.position.y,wrist.position.z,
        expected.position.x,expected.position.y,expected.position.z,
        expected.rotation.pitch,expected.rotation.yaw,expected.rotation.roll,matrix[12],matrix[13],matrix[14],
        matrix[0],matrix[1],matrix[2],matrix[4],matrix[5],matrix[6],matrix[8],matrix[9],matrix[10]);fflush(log_file);
    fwprintf(log_file,L"lancer_magazine_fields serial=%llu known=%u translation=%.9g,%.9g,%.9g rotation=%d,%d,%d absolute=%u,%u\n",
        actual.right.serial,unsigned(fields),translation.x,translation.y,translation.z,
        rotation.pitch,rotation.yaw,rotation.roll,unsigned(abs_t),unsigned(abs_r));fflush(log_file);
    sequence=actual.right.serial;++rows;
}
template<class T>bool read_struct(uptr object,const wchar_t* field,T& out) noexcept {
    const auto p=playground::property(object,field),cls=get<uptr>(object+0x50);
    if(!p||!cls||get<unsigned>(p+0x74)!=sizeof(T)||
       !playground::named(get<uptr>(p+0x50),L"StructProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),extent=get<unsigned>(cls+0x88);
    return offset<=extent&&sizeof(T)<=extent-offset&&object<=UINTPTR_MAX-offset-sizeof(T)&&
        copy(&out,object+offset,sizeof(T));
}
bool flag(uptr object,const wchar_t* function,const wchar_t* parameter,bool value) noexcept {
    playground::Args call(object,function);
    return nativelancerfollow::bool_arg(call,parameter,value)&&call.run();
}
bool component_pointer(uptr object,const wchar_t* field,uptr& value) noexcept {
    value=0;const auto p=playground::property(object,field),cls=get<uptr>(object+0x50);
    if(!p||!cls||get<unsigned>(p+0x74)!=sizeof(uptr)||
       !playground::named(get<uptr>(p+0x50),L"ComponentProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),extent=get<unsigned>(cls+0x88);
    uptr before{},after{};
    if(offset>extent||sizeof(uptr)>extent-offset||object>UINTPTR_MAX-offset-sizeof(uptr)||
       !copy(&before,object+offset,sizeof(before))||!copy(&after,object+offset,sizeof(after))||before!=after)return false;
    value=before;return true;
}
bool precomputed_shadow_mode(bool desired) noexcept {
    if(!alive()||!saved.dynamic_shadow_owned)return !saved.dynamic_shadow_owned;
    const auto object=saved.component.address;
    const auto p=playground::property(object,L"bUsePrecomputedShadows"),cls=get<uptr>(object+0x50);
    if(!registered(p)||!playground::named(get<uptr>(p+0x50),L"BoolProperty")||
       get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=4)return false;
    const auto offset=get<unsigned>(p+0x94),extent=get<unsigned>(cls+0x88),mask=get<unsigned>(p+0xd0);
    unsigned bits{},checked{};
    if(!mask||(mask&(mask-1))||offset>extent||4>extent-offset||!copy(&bits,object+offset,4))return false;
    if(bool(bits&mask)==desired)return true;
    const auto wanted=desired?bits|mask:bits&~mask;
    return lancerreload_state::ui_store(object+offset,&wanted,4)&&alive()&&
        copy(&checked,object+offset,4)&&checked==wanted;
}
bool independent_shadow_parent() noexcept {
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||!alive())return false;
    uptr parent{};
    if(!component_pointer(saved.component.address,L"ShadowParent",parent))return false;
    if(!parent)return true;
    // The retail handoff installs the animated body as shadow parent. This
    // component is subsequently posed independently in absolute world space.
    // Remove only that exact installed parent through the original setter;
    // ordinary shadow casting and the light environment remain enabled.
    const auto body=playground::pointer(saved.pawn.address,L"Mesh");
    if(!body||parent!=body||!playground::instance(body,L"SkeletalMeshComponent"))return false;
    playground::Args detach(saved.component.address,L"SetShadowParent");
    const auto field=detach.prop(L"NewShadowParent");
    if(!detach.valid||!field||get<unsigned>(field+0x74)!=sizeof(uptr)||
       !playground::named(get<uptr>(field+0x50),L"ComponentProperty")||
       !detach.set(L"NewShadowParent",uptr{}).run()||!alive()||
       !component_pointer(saved.component.address,L"ShadowParent",parent)||parent)return false;
    static unsigned observations{};
    if(log_file&&observations++<16){fwprintf(log_file,L"lancer_reload_shadow independent=1 component=%llx removed_body_parent=%llx casting_unchanged=1 light_environment_unchanged=1\n",
        saved.component.address,body);fflush(log_file);}
    return true;
}
bool absolute(bool translation,bool rotation,bool scale) noexcept {
    playground::Args call(saved.component.address,L"SetAbsolute");
    return alive()&&nativelancerfollow::bool_arg(call,L"NewAbsoluteTranslation",translation)&&
        nativelancerfollow::bool_arg(call,L"NewAbsoluteRotation",rotation)&&
        nativelancerfollow::bool_arg(call,L"NewAbsoluteScale",scale)&&call.run();
}
bool transform(playground::Vector const& position,playground::Rotation const& rotation) noexcept {
    if(!alive())return false;
    playground::Args move(saved.component.address,L"SetTranslation"),turn(saved.component.address,L"SetRotation");
    return move.set(L"NewTranslation",position).run()&&alive()&&turn.set(L"NewRotation",rotation).run();
}
bool restore() noexcept {
    if(!active)return true;
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||!alive())return false;
    if(!flag(saved.weapon.address,L"SetPawnAmmoAttachment",L"bAttach",false)||
       !precomputed_shadow_mode(saved.precomputed_shadows)||
       !flag(saved.weapon.address,L"SetWeaponAmmoBoneDisplay",L"bShow",true)||
       !flag(saved.component.address,L"SetHidden",L"NewHidden",saved.hidden)||
       !absolute(saved.absolute_translation,saved.absolute_rotation,saved.absolute_scale)||
       !transform(saved.translation,saved.rotation))return false;
    vrequip::MagazineVisualProbe check{};
    uptr restored_shadow{},restored_environment{};
    if(!vrequip::magazine_visual_probe(saved.weapon.address,saved.pawn.address,check)||
       check.component!=saved.component.address||check.attached_to_pawn||check.hidden!=saved.hidden||
       !component_pointer(saved.component.address,L"ShadowParent",restored_shadow)||restored_shadow||
       !component_pointer(saved.component.address,L"LightEnvironment",restored_environment)||restored_environment)return false;
    restored=saved;
    restored_known=lancerreload_state::start_time(saved.weapon.address,restored_reload_start);
    active=false;saved={};last_grab_known=false;last_lancer_target_known=false;pinch_mount={};extraction_display_enabled=false;return true;
}
bool reclaim_restored(hand::Frame const& source) noexcept {
    float started{};bool reloading{};
    if(active||!restored_known||!hand::same(source.weapon,restored.weapon)||
       !hand::same(source.pawn,restored.pawn)||!same_object(restored.component)||!same_object(restored.asset)||
       playground::pointer(source.weapon.address,L"MagazineMesh")!=restored.component.address||
       playground::pointer(restored.component.address,L"StaticMesh")!=restored.asset.address||
       !lancerreload_state::start_time(source.weapon.address,started)||started!=restored_reload_start||
       !lancerreload_state::bool_call(source.weapon.address,L"IsReloading",reloading)||reloading||
       !lancerreload_state::magazine_owned(source)||!nativeequipment::current(nullptr,source))return false;
    // A new explicit grab may reclaim only the component we successfully
    // restored on this exact reload instance. Never normalize an unknown
    // attached magazine or interfere with a newly started original reload.
    saved=restored;active=true;
    const bool result=restore();
    if(log_file){fwprintf(log_file,L"lancer_reload_visual_reclaim result=%u\n",unsigned(result));fflush(log_file);}
    return result;
}
enum class Profile {lancer,hammerburst};
bool lancer_target(gears_committed_interaction::Frame const& actual,gears_vr::Viewpoint& target) noexcept {
    // Mechanical sampling always uses the fresh, reachable UNSNAPPED hand.
    // The display path may stop inward motion at the seat; that target never
    // feeds back into the extraction stroke or supplies a successful pose.
    const auto& left=actual.right.controllers.hands[0];
    if(active&&pinch_mount.known&&left.pressed[gears_vr::control_trigger]==pinch_mount_press&&
       left.released[gears_vr::control_trigger]==pinch_mount_release)
        return pinch_mount.apply(actual.physical_left_grip,target);
    if(!active){
        const auto& r=actual.right;std::array<float,16> gun{};
        return nativelancerfollow::endpoint_matrix(r.right_grip,gun)&&gears_weapon::lancer_seated_magazine(gun,target);
    }
    gears_vr::Pose wrist{};if(!physical_wrist(actual,wrist))return false;
    return gears_weapon::lancer_magazine_from_left({wrist.position,
        gears_vr::ue_angles(gears_vr::rotation_matrix(wrist.orientation))},target,actual.model_scale);
}
bool lancer_sample(gears_committed_interaction::Frame const& actual,bool grip_down,
    gears_weapon::LancerInsertionSample& out) noexcept {
    out={};const auto& r=actual.right;
    if(!nativelancerfollow::endpoint_matrix(r.right_grip,out.gun_mesh)||!lancer_target(actual,out.magazine))return false;
    out.weapon=r.weapon.address;out.component=nativelancerfollow::bound_key.component.address;
    out.world=r.world.address;out.session=r.stereo.session;out.space_epoch=r.stereo.space_epoch;
    out.input_epoch=r.controllers.input_epoch;out.world_generation=r.parent.world_generation;
    out.sequence=r.controllers.sequence;out.created_ms=std::uint64_t(r.controllers.sync_ns/1000000);
    out.tracked=true;out.grip_down=grip_down;out.units_per_metre=r.units_per_metre;
    if(insertion_assist_enabled&&!insertion_assist.apply(out))return false;
    return out.created_ms&&out.sequence;
}
bool prime_lancer_pose(gears_vr::Viewpoint const& target) noexcept {
    return gears_vr::finite(target.position)&&std::isfinite(target.rotation.pitch)&&
        std::isfinite(target.rotation.yaw)&&std::isfinite(target.rotation.roll)&&
        transform({float(target.position.x),float(target.position.y),float(target.position.z)},
            {gears_vr::to_rotator_units(target.rotation.pitch),gears_vr::to_rotator_units(target.rotation.yaw),
             gears_vr::to_rotator_units(target.rotation.roll)});
}
bool regrasp_original(hand::Frame const& source) noexcept {
    if(!alive()||!last_lancer_target_known||!hand::same(source.weapon,saved.weapon)||
       !hand::same(source.pawn,saved.pawn)||!lancerreload_state::hand_owned(source,hand::LeaseKind::magazine)||
       !nativeequipment::current(nullptr,source))return false;
    gears_committed_interaction::Frame actual{};
    if(!interaction(source,actual)||!pinch_mount.capture(actual.physical_left_grip,last_lancer_target))return false;
    pinch_mount_press=actual.right.controllers.hands[0].pressed[gears_vr::control_trigger];
    pinch_mount_release=actual.right.controllers.hands[0].released[gears_vr::control_trigger];
    return true; // Rebind the frozen visible item, with no movement or reload completion.
}
// A bounded observation in the existing combined reload, not a material
// replacement or an acceptance gate. Compare what the actual components use;
// a static saved-package material candidate is not the current retail binding.
uptr observed_material_weapon{};unsigned material_observations{};
bool read_material(uptr component,uptr& material) noexcept {
    material=0;playground::Args call(component,L"GetMaterial");
    const auto index=call.prop(L"ElementIndex"),result=call.prop(L"ReturnValue");
    if(!call.valid||!index||!result||get<unsigned>(index+0x74)!=4||
       !playground::named(get<uptr>(index+0x50),L"IntProperty")||get<unsigned>(result+0x74)!=sizeof(uptr)||
       !playground::named(get<uptr>(result+0x50),L"ObjectProperty")||!call.set(L"ElementIndex",0).run())return false;
    material=call.output<uptr>();return material&&playground::instance(material,L"MaterialInterface")&&!playground::deleted(material);
}
// Keep this separate mesh's authored material (or the owner's skin override).
// The saved Lancer gun uses Delta_MAT_MIC/EnvironmentBaseMaterials and Delta
// textures; its separate magazine uses PH_Mat and PH textures. Gun slot0 is
// not a compatibility certificate for that magazine's UV/material contract.
// Borrowing it produced a black face in55076 despite successful readback.
// Do not edit either component's Materials or a shared material to hide it.
void observe_materials(hand::Frame const& source,uptr magazine) noexcept {
    if(!log_file||material_observations>=4||observed_material_weapon==source.weapon.address||
       !nativeequipment::current(nullptr,source))return;
    observed_material_weapon=source.weapon.address;++material_observations;
    const auto gun=playground::pointer(source.weapon.address,L"Mesh");uptr gun_material{},magazine_material{};
    const bool gun_known=gun&&read_material(gun,gun_material),mag_known=read_material(magazine,magazine_material);
    wchar_t gun_name[160]{},mag_name[160]{};
    if(gun_known)name(gun_material,gun_name,160);if(mag_known)name(magazine_material,mag_name,160);
    if(!nativeequipment::current(nullptr,source))return;
    fwprintf(log_file,L"lancer_reload_material_observation weapon=%llx gun_known=%u gun_material=%llx gun_name=%ls magazine_known=%u magazine_material=%llx magazine_name=%ls same=%u read_only=1\n",
        source.weapon.address,unsigned(gun_known),gun_material,gun_name,unsigned(mag_known),magazine_material,mag_name,
        unsigned(gun_known&&mag_known&&gun_material==magazine_material));fflush(log_file);
}
// The caller may supply its decision copied in THIS synchronous GT invocation.
// Candidate proximity is a per-publication observation, not a second live
// holder. Recomputing it after read-only inventory queries can expire an older
// geometry result and clear an otherwise current input/lease. Recheck every
// live game/holder field and the exact publication, plus ACTUAL geometry's
// own unchanged deadline; never turn this snapshot into a new grant.
bool prepared_start(hand::Frame const& source,hand::GripGameState const& prepared_game,
    hand::Decision const& decision,gears_committed_interaction::Frame const& actual) noexcept {
    if(!hand::same(source.key,decision.key)||decision.release_required||
       (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
       decision.lease.kind!=hand::LeaseKind::magazine||!decision.lease.id||
       decision.lease.id!=source.press_token||!hand::lease_valid(decision.lease,source.key.lifecycle)||
       !source.left_tracked||!source.left_grip_down)return false;
    hand::GripGameState current{};
    if(!weaponhand::client.source_game_on_gt(source.key,current)||!current.known||
       !current.main_hand_held||current.busy||current.animation_owns_hands)return false;
    current.request_ammo=prepared_game.request_ammo;current.request_magazine=prepared_game.request_magazine;
    current.magazine_observation_pending=prepared_game.magazine_observation_pending;
    if(!hand::same_game(current,prepared_game))return false;
    return gears_committed_interaction::matches(actual,source,vrequip::model.hold_generation(),
        weaponhand::clock_ns(nullptr),source.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);
}
bool begin_retryable{};
bool begin(hand::Frame const& source,Profile profile=Profile::lancer,
    hand::GripGameState const* prepared_game=nullptr,hand::Decision const* decision=nullptr,
    gears_committed_interaction::Frame const* actual=nullptr) noexcept {
    begin_retryable=true;
    const auto fail=[](unsigned gate){
        if(log_file){fwprintf(log_file,L"lancer_reload_visual_begin gate=%u\n",gate);fflush(log_file);}return false;
    };
    const bool hb=profile==Profile::hammerburst;
    if(active)return fail(1);
    pinch_mount={};last_lancer_target_known=false;
    if(!playground::instance(source.weapon.address,hb?L"Weap_LocustAssaultRifle":L"Weap_AssaultRifle"))return fail(11);
    if(prepared_game&&decision&&actual){if(!prepared_start(source,*prepared_game,*decision,*actual))return fail(12);}
    else if(prepared_game||decision||actual)return fail(15);
    else if(!lancerreload_state::magazine_owned(source))return fail(13);
    if(!nativeequipment::current(nullptr,source))return fail(14);
    vrequip::MagazineVisualProbe magazine{};vrequip::AmmoBoneVisualProbe bone{};
    bool magazine_known=vrequip::magazine_visual_probe(source.weapon.address,source.pawn.address,magazine);
    if(magazine_known&&magazine.attached_to_pawn){
        begin_retryable=false;
        if(!reclaim_restored(source))return fail(5);
        magazine_known=vrequip::magazine_visual_probe(source.weapon.address,source.pawn.address,magazine);
    }
    if(!magazine_known||
       !magazine.present||magazine.hidden||magazine.attached_to_pawn||
       wcscmp(magazine.asset_name,hb?L"Hammerburst_Magazine":L"Assault_Rifle_Magazine")||
       !vrequip::ammo_bone_visual_probe(source.weapon.address,bone)||!bone.present||
       bone.strength!=0.f||bone.target!=0.f){
        if(log_file)fwprintf(log_file,L"lancer_reload_visual_probe present=%u hidden=%u attached=%u asset=%ls bone=%u strength=%g target=%g\n",
            unsigned(magazine.present),unsigned(magazine.hidden),unsigned(magazine.attached_to_pawn),magazine.asset_name,
            unsigned(bone.present),bone.strength,bone.target);
        return fail(2);
    }
    // GetMaterial is optional observation, not acquisition preflight. Its two
    // engine calls consumed this actual contact result's lifetime in6880.
    // The original binding is already recorded; do not repeat diagnostics on
    // a fresh player's grip before the first visible item can be acquired.
    Snapshot s{};s.weapon=source.weapon;s.pawn=source.pawn;s.hidden=magazine.hidden;
    if(!nativeequipment::identify(nullptr,magazine.component,s.component)||
       !nativeequipment::identify(nullptr,magazine.asset,s.asset)||
       !read_struct(magazine.component,L"Translation",s.translation)||
       !read_struct(magazine.component,L"Rotation",s.rotation)||
       !vrequip::read_bool(magazine.component,L"AbsoluteTranslation",s.absolute_translation)||
       !vrequip::read_bool(magazine.component,L"AbsoluteRotation",s.absolute_rotation)||
       !vrequip::read_bool(magazine.component,L"AbsoluteScale",s.absolute_scale)||
       !vrequip::read_bool(magazine.component,L"bUsePrecomputedShadows",s.precomputed_shadows)||
       !nativeequipment::current(nullptr,source))return fail(3);
    // Only adopt the retail detached baseline that the original cleanup can
    // restore. Do not clear an unrelated pre-existing lighting/shadow owner.
    uptr original_shadow{},original_environment{};
    if(!component_pointer(magazine.component,L"ShadowParent",original_shadow)||original_shadow||
       !component_pointer(magazine.component,L"LightEnvironment",original_environment)||original_environment)return fail(17);
    // Acquire from this exact already-admitted contact, not a later cached
    // pose. SetAbsolute on a zero relative Translation previously exposed a
    // world-origin component until the NEXT publication's move(). Its shared
    // body light environment also observed those erroneous bounds. Finish the
    // initial pose in this same GT acquisition before returning to animation.
    // Hammerburst has its own independently measured magazine mount.
    gears_vr::Viewpoint initial_target{};
    if(!hb){
        gears_committed_interaction::Frame initial_actual{};
        if(actual)initial_actual=*actual;
        else if(!interaction(source,initial_actual))return fail(18);
        if(!gears_committed_interaction::matches(initial_actual,source,vrequip::model.hold_generation(),
                weaponhand::clock_ns(nullptr),source.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)||
           !lancer_target(initial_actual,initial_target))return fail(18);
        if(!pinch_mount.capture(initial_actual.physical_left_grip,initial_target))return fail(18);
        pinch_mount_press=initial_actual.right.controllers.hands[0].pressed[gears_vr::control_trigger];
        pinch_mount_release=initial_actual.right.controllers.hands[0].released[gears_vr::control_trigger];
    }
    begin_retryable=false;
    // The original detached StaticMesh retains baked-shadow mode, unlike the
    // actual moving body/gun (retail11356 readback). While hand-held, use its
    // dynamic light environment instead of nonexistent static shadow data.
    // Change only this leased component BEFORE the original attachment creates
    // its scene proxy; restore the saved bit AFTER original detachment. No
    // shared material, body light environment or global rendering flag changes.
    s.dynamic_shadow_owned=!hb&&s.precomputed_shadows;
    saved=s;active=true;last_grab_known=false; // Own restoration even when a following call fails.
    if(!precomputed_shadow_mode(false)||!flag(s.weapon.address,L"SetPawnAmmoAttachment",L"bAttach",true)||
       !flag(s.weapon.address,L"SetWeaponAmmoBoneDisplay",L"bShow",false)||
       !absolute(true,true,s.absolute_scale)||!independent_shadow_parent()||
       (!hb&&!prime_lancer_pose(initial_target))){restore();return fail(4);}
    if(!hb){last_lancer_target=initial_target;last_lancer_target_known=true;}
    if(log_file&&s.dynamic_shadow_owned){fwprintf(log_file,L"lancer_reload_lighting component=%llx precomputed_before=1 held_dynamic=1 material_unchanged=1\n",s.component.address);fflush(log_file);}
    return true;
}
unsigned move_failure{};
// Called only after the current hand lease and frame have been checked. The
// original reload animation can detach this SAME component after we acquire
// it. Successful transform setters on a detached component do not update its
// scene matrix. Reattach only the saved, still-identical retail magazine;
// never adopt a replacement component or treat a failed probe as detached.
bool retain_attachment() noexcept {
    vrequip::MagazineVisualProbe current{};
    if(!alive()||!vrequip::magazine_visual_probe(saved.weapon.address,saved.pawn.address,current)||
       current.component!=saved.component.address||current.asset!=saved.asset.address)return false;
    if(!current.attached_to_pawn){
        if(!flag(saved.weapon.address,L"SetPawnAmmoAttachment",L"bAttach",true)||!alive()||
           !vrequip::magazine_visual_probe(saved.weapon.address,saved.pawn.address,current)||
           current.component!=saved.component.address||current.asset!=saved.asset.address||
           !current.attached_to_pawn)return false;
    }
    if(!independent_shadow_parent())return false;
    // Verify first. These original setters may enqueue reattachment/animation
    // work; issuing all three on EVERY move is neither a freshness proof nor
    // necessary ownership work. Reassert only an actual changed value.
    if(current.hidden&&!flag(saved.component.address,L"SetHidden",L"NewHidden",false))return false;
    vrequip::AmmoBoneVisualProbe ammo_control{};
    if(!vrequip::ammo_bone_visual_probe(saved.weapon.address,ammo_control)||!ammo_control.present||
       ammo_control.bone_scale!=0.f||ammo_control.strength<0.f||ammo_control.strength>1.f||
       ammo_control.target<0.f||ammo_control.target>1.f)return false;
    // Original SetWeaponAmmoBoneDisplay(false) activates AmmoControl. Keep
    // its own interpolation; repeatedly activating it can restart that work.
    if(ammo_control.target!=1.f&&
       !flag(saved.weapon.address,L"SetWeaponAmmoBoneDisplay",L"bShow",false))return false;
    bool abs_translation{},abs_rotation{},abs_scale{};
    if(!alive()||!vrequip::read_bool(saved.component.address,L"AbsoluteTranslation",abs_translation)||
       !vrequip::read_bool(saved.component.address,L"AbsoluteRotation",abs_rotation)||
       !vrequip::read_bool(saved.component.address,L"AbsoluteScale",abs_scale))return false;
    return (abs_translation&&abs_rotation&&abs_scale==saved.absolute_scale)||absolute(true,true,saved.absolute_scale);
}
// Caller supplies its own per-asset item mount; do not reuse Lancer geometry
// for a magazine whose native attachment/orientation was independently read.
bool move_target(hand::Frame const& source,gears_vr::Viewpoint const& target,
    gears_vr::Vector const& grab) noexcept {
    if(!alive()||!hand::same(source.weapon,saved.weapon)||!hand::same(source.pawn,saved.pawn)||
       !lancerreload_state::item_owned(source)||!nativeequipment::current(nullptr,source)||
       !gears_vr::finite(target.position)||!gears_vr::finite(grab)||
       !std::isfinite(target.rotation.pitch)||!std::isfinite(target.rotation.yaw)||!std::isfinite(target.rotation.roll))return false;
    const bool moved=transform({float(target.position.x),float(target.position.y),float(target.position.z)},
        {gears_vr::to_rotator_units(target.rotation.pitch),gears_vr::to_rotator_units(target.rotation.yaw),
         gears_vr::to_rotator_units(target.rotation.roll)});
    if(moved){last_grab_world=grab;last_grab_known=true;}
    return moved;
}
bool move(hand::Frame const& source) noexcept {
    move_failure=1;
    if(!alive()||!hand::same(source.weapon,saved.weapon)||!hand::same(source.pawn,saved.pawn))return false;
    move_failure=2;
    if(!lancerreload_state::item_owned(source)||!nativeequipment::current(nullptr,source))return false;
    move_failure=3;
    gears_committed_interaction::Frame actual{};
    if(!interaction(source,actual))return false;
    gears_weapon::LancerInsertionSample sample{};
    if(!lancer_sample(actual,true,sample))return false;
    auto target=sample.magazine;
    if(extraction_display_enabled&&!gears_weapon::lancer_extraction_display_target(sample,target))return false;
    move_failure=4;
    if(!retain_attachment())return false;
    const bool moved=transform({float(target.position.x),float(target.position.y),float(target.position.z)},
        {gears_vr::to_rotator_units(target.rotation.pitch),gears_vr::to_rotator_units(target.rotation.yaw),
         gears_vr::to_rotator_units(target.rotation.roll)});
    if(moved){last_grab_world=actual.physical_left_grip.position;last_grab_known=true;move_failure=0;
        last_lancer_target=target;last_lancer_target_known=true;}
    return moved;
}
// Hide rather than deliberately detach while putting the item away. The game
// may still detach it later; the next authenticated grab/move must recover the
// attachment before writing a transform. Cleanup restores the saved state.
bool attached() noexcept {
    vrequip::MagazineVisualProbe current{};
    return alive()&&vrequip::magazine_visual_probe(saved.weapon.address,saved.pawn.address,current)&&
        current.component==saved.component.address&&current.asset==saved.asset.address&&current.attached_to_pawn;
}
bool stow() noexcept {
    return attached()&&flag(saved.component.address,L"SetHidden",L"NewHidden",true)&&
        flag(saved.weapon.address,L"SetWeaponAmmoBoneDisplay",L"bShow",false)&&
        absolute(saved.absolute_translation,saved.absolute_rotation,saved.absolute_scale)&&
        transform(saved.translation,saved.rotation);
}
unsigned replacement_failure{};
bool take_replacement(hand::Frame const& source) noexcept {
    replacement_failure=1;
    if(!alive()||!hand::same(source.weapon,saved.weapon)||!hand::same(source.pawn,saved.pawn))return false;
    replacement_failure=2;if(!lancerreload_state::hand_owned(source,hand::LeaseKind::ammo))return false;
    replacement_failure=3;if(!nativeequipment::current(nullptr,source))return false;
    gears_vr::Viewpoint initial_target{};
    const bool lancer=playground::instance(source.weapon.address,L"Weap_AssaultRifle");
    if(lancer){
        gears_committed_interaction::Frame actual{};
        replacement_failure=8;
        if(!interaction(source,actual)||!lancer_target(actual,initial_target))return false;
        if(!pinch_mount.capture(actual.physical_left_grip,initial_target))return false;
        pinch_mount_press=actual.right.controllers.hands[0].pressed[gears_vr::control_trigger];
        pinch_mount_release=actual.right.controllers.hands[0].released[gears_vr::control_trigger];
    }
    replacement_failure=4;if(!retain_attachment())return false;
    // The stowed item has relative attachment coordinates. Prime its current
    // authenticated hand position before showing it; do not expose world0 or
    // wait for a subsequent grant/move to correct the first visible pose.
    replacement_failure=9;if(lancer&&!prime_lancer_pose(initial_target))return false;
    if(lancer){last_lancer_target=initial_target;last_lancer_target_known=true;}
    replacement_failure=5;if(!flag(saved.component.address,L"SetHidden",L"NewHidden",false))return false;
    replacement_failure=6;if(!flag(saved.weapon.address,L"SetWeaponAmmoBoneDisplay",L"bShow",false))return false;
    replacement_failure=7;if(!absolute(true,true,saved.absolute_scale))return false;
    replacement_failure=0;return true;
}
}
