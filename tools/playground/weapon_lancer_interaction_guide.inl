// Cosmetic, one-frame original world lines. No new hook/writer, persistent
// component, material, collision volume, hand grant or inventory operation.
namespace lancer_interaction_guide {
namespace hand=gears_weapon_hand;
namespace guide=gears_weapon::lancer_guide;
std::uint64_t last_frame{};unsigned logs{};
guide::Step last_reported_step{};
bool transport_current(hand::Frame const& f) noexcept {
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
       !weaponhand::client.registration_alive(f.key.registration_cookie,f.key.native_started_qpc)||
       !hand::frame_active(f,nativeequipment::clock_ns(nullptr))||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       !nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_lancer||
       !hand::same(f.weapon,nativelancerfollow::bound_key.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation())return false;
    return true;
}
bool current(hand::Frame const& f) noexcept {
    if(!transport_current(f))return false;
    for(auto const& expected:{f.world,f.controller,f.pawn,f.weapon}){
        hand::ObjectIdentity actual{};
        if(!nativeequipment::identify(nullptr,expected.address,actual)||!hand::same(expected,actual))return false;
    }
    return true;
}
bool line_call(hand::Frame const& f,playground::Args& call,hand::ObjectIdentity& function) noexcept {
    if(!current(f)||!call.valid||call.r.size>64||!nativeequipment::identify(nullptr,call.r.function,function))return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    if(!registered(owner)||!registered(package)||!playground::named(owner,L"Actor")||!playground::named(package,L"Engine")||
       get<uptr>(package+0x40)!=0||(get<unsigned>(call.r.function+0xf4)&0x2400)!=0x2400||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr)))return false;
    const wchar_t* labels[]={L"LineStart",L"LineEnd",L"R",L"G",L"B",L"bPersistentLines"};
    const wchar_t* kinds[]={L"StructProperty",L"StructProperty",L"ByteProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned sizes[]={12,12,1,1,1,4};std::uint64_t occupied{};
    for(unsigned i=0;i<6;++i){const auto p=call.prop(labels[i]);
        if(!hammerburst_animation::input_field(call,p,kinds[i],sizes[i]))return false;
        const auto offset=get<unsigned>(p+0x94);if(offset>64-sizes[i])return false;
        for(unsigned n=0;n<sizes[i];++n){const auto bit=std::uint64_t(1)<<(offset+n);if(occupied&bit)return false;occupied|=bit;}
        if(i<2){const auto type=get<uptr>(p+0xd0);
            if(!registered(type)||!playground::named(type,L"Vector")||get<unsigned>(type+0x88)!=12)return false;}
    }
    return call.set(L"R",static_cast<unsigned char>(255)).set(L"G",static_cast<unsigned char>(220))
        .set(L"B",static_cast<unsigned char>(0)).valid&&nativelancerfollow::bool_arg(call,L"bPersistentLines",false);
}
bool empty_magazine_hint(hand::Frame const& f) noexcept {
    // Original WarWeapon.GetMagazineSize returns this exact MagSize field.
    // Cosmetic read only: no CanReload call, new transaction or ammo write.
    int used{},spare{},again_used{},again_spare{};float size{},again_size{};
    if(!lancerreload_inventory::integer(f.weapon.address,L"AmmoUsedCount",used)||used<=0||
       !lancerreload_state::time_field(f.weapon.address,L"MagSize",size)||
       size<1||size>10000||std::floor(size)!=size||used!=int(size)||
       !lancerreload_inventory::integer(f.weapon.address,L"SpareAmmoCount",spare))return false;
    weaponhand::meta::Snapshot schema{};schema.module_base=base;
    weaponhand::meta::Reader reader{schema,weaponhand::meta::Stage::final_check};weaponhand::BoolField infinite{};
    if(!weaponhand::meta::tables(base,schema.objects,schema.names)||
       !weaponhand::boolean(reader,f.weapon.address,L"bInfiniteSpareAmmo",infinite)||
       !guide::empty_magazine_hint(int(size),used,spare,(infinite.bits&infinite.mask)!=0))return false;
    return lancerreload_inventory::integer(f.weapon.address,L"AmmoUsedCount",again_used)&&again_used==used&&
        lancerreload_inventory::integer(f.weapon.address,L"SpareAmmoCount",again_spare)&&again_spare==spare&&
        lancerreload_state::time_field(f.weapon.address,L"MagSize",again_size)&&again_size==size&&current(f);
}
bool supply_shape(hand::Frame const& f,guide::Shape& out) noexcept {
    gears_committed_interaction::Frame actual{};
    if(!lancerreload_visual::interaction(f,actual))return false;
    gears_vr::Vector shoulder{};
    const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
    if(!gears_weapon::shoulder_ammo_center(nativeequipment::calibration,body,shoulder))return false;
    const auto& raw=f.controllers.hands[0].grip;const auto& world=actual.physical_left_grip;
    const auto basis=gears_vr::multiply(gears_vr::rotation_matrix(world.orientation),
        gears_vr::transpose(gears_vr::xr_to_ue(gears_vr::rotation_matrix(raw.orientation))));
    const auto delta=gears_vr::transform(basis,gears_vr::xr_to_ue(guide::add(shoulder,guide::scale(raw.position,-1))));
    const double units=actual.right.units_per_metre;
    if(!std::isfinite(units)||units<10||units>500)return false;
    // Align the frame with the stable torso, not the gun or latest head yaw.
    const auto torso=gears_vr::multiply(basis,gears_vr::xr_to_ue(gears_vr::rotation_matrix(body.orientation)));
    return guide::marker(guide::add(world.position,guide::scale(delta,units)),
        gears_vr::transform(torso,{0,1,0}),gears_vr::transform(torso,{0,0,1}),.065*units,out)&&current(f);
}
bool part_marker(std::array<float,16> const& matrix,guide::V center,double half,guide::Shape& out) noexcept {
    gears_weapon::LancerFeedWorld feed{};
    if(!gears_weapon::lancer_feed_world(matrix,feed))return false;
    const double y=std::hypot(std::hypot(matrix[4],matrix[5]),matrix[6]);
    const double z=std::hypot(std::hypot(matrix[8],matrix[9]),matrix[10]);
    return guide::marker(center,{matrix[4]/y,matrix[5]/y,matrix[6]/y},
        {matrix[8]/z,matrix[9]/z,matrix[10]/z},half,out);
}
void tick(uptr pc) noexcept {
    const auto began=weaponhand::clock_ns(nullptr);
    using Stage=lancerreload_runtime::Stage;
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||
       !nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_lancer)return;
    guide::Step step{};bool initial_empty{};
    switch(lancerreload_runtime::stage){
    // Loaded/finished has no cue. A later genuinely empty magazine can show
    // its first step BEFORE grabbing, without starting/authorizing reload.
    case Stage::idle:step=guide::Step::remove_magazine;initial_empty=true;break;
    case Stage::extracting:step=guide::Step::remove_magazine;break;
    case Stage::holding_original:step=guide::Step::discard_magazine;break;
    case Stage::waiting_ammo:step=guide::Step::take_magazine;break;
    case Stage::holding_new:step=guide::Step::insert_magazine;break;
    case Stage::charging:step=guide::charging_step(unsigned(lancer_charging::stroke.phase()),lancer_charging::stroke.actual_full_pull());break;
    case Stage::parked:step=lancerreload_runtime::parked_charging?guide::Step::pull_charging_handle:guide::Step::take_magazine;break;
    default:return;
    }
    hand::Frame f{};hand::GripGameState game{};
    if(!weaponhand::client.copy_frame_on_gt(f,game)||f.controller.address!=pc||f.key.frame==last_frame||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       !f.left_tracked||!f.left_grip_known||step==guide::Step::none||!current(f))return;
    if(initial_empty&&lancer_charging::required(f)){step=guide::Step::pull_charging_handle;initial_empty=false;}
    if(initial_empty&&(lancersaw_original::active()||lancerreload_runtime::needs_priority()||!empty_magazine_hint(f)))return;
    const bool mechanism=step==guide::Step::pull_charging_handle||
        step==guide::Step::release_charging||step==guide::Step::wait_return;
    if(lancer_charging::required(f)!=mechanism)return;
    // Each one-frame cue describes only the unfinished step: pull during
    // extraction, the empty port while carrying a magazine, then the empty-
    // chamber mechanism. Insertion/closure removes the port cue, and charging
    // completion returns to idle with no cue. No persistent lines to retire.
    vrequip::MeshPoseProbe before{},after{};
    if(!vrequip::mesh_pose_probe(f.weapon.address,before))return;
    std::array<float,16> matrix{};std::memcpy(matrix.data(),before.local_to_world,sizeof(before.local_to_world));
    guide::Shape shape{};
    if(step==guide::Step::take_magazine){
        if(!supply_shape(f,shape))return;
    }else if(step==guide::Step::discard_magazine){
        if(!lancerreload_visual::active||!lancerreload_visual::last_grab_known||
           !hand::same(f.weapon,lancerreload_visual::saved.weapon)||
           !part_marker(matrix,lancerreload_visual::last_grab_world,4.,shape))return;
    }else if(mechanism){
        if(!lancer_charging::current_geometry(f))return;
        if(!guide::charging(matrix,lancer_charging::last_observed*nativelancerfollow::bound_units,step,shape))return;
    }else if(!guide::magazine(matrix,step,shape))return;
    if(!vrequip::mesh_pose_probe(f.weapon.address,after)||before.mesh!=after.mesh||before.asset!=after.asset||
       std::memcmp(before.local_to_world,after.local_to_world,sizeof(before.local_to_world))||!current(f))return;
    playground::Args call(f.world.address,L"DrawDebugLine");hand::ObjectIdentity function{};
    if(!line_call(f,call,function))return;
    last_frame=f.key.frame;unsigned drawn{};
    for(unsigned i=0;i<shape.count;++i){const auto& edge=shape.lines[i];hand::ObjectIdentity fresh{};
        // This synchronous GT batch calls only certified original transient
        // DrawDebugLine. Actor identities bracket the batch; every edge still
        // checks the unchanged registration/held weapon, original source age
        // and function identity. Avoid four complete actor metadata walks per
        // rail, which consumed the frame that the cue described.
        if(!transport_current(f)||!registered(f.world.address)||
           !nativeequipment::identify(nullptr,call.r.function,fresh)||!hand::same(function,fresh))break;
        const playground::Vector start{float(edge.start.x),float(edge.start.y),float(edge.start.z)};
        const playground::Vector end{float(edge.end.x),float(edge.end.y),float(edge.end.z)};
        if(!call.set(L"LineStart",start).set(L"LineEnd",end).valid||!call.run())break;
        ++drawn;
    }
    const bool closed=current(f);
    if(log_file&&logs<24&&step!=last_reported_step){fwprintf(log_file,L"lancer_interaction_guide frame=%llu step=%u lines=%u/%u one_frame=1 actual_mesh=1 closing_current=%u elapsed_ns=%lld\n",
        f.key.frame,unsigned(step),drawn,shape.count,unsigned(closed),weaponhand::clock_ns(nullptr)-began);fflush(log_file);++logs;last_reported_step=step;}
}
}
