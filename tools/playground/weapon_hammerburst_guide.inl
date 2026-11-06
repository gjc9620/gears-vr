// World-space, one-frame yellow interaction-region outline. No material,
// pawn/weapon pose, inventory, persistent line or tracking state is changed.
namespace hammerburst_guide {
namespace hb=hammerburst_runtime;
namespace hand=gears_weapon_hand;
unsigned previous_step{~0u},logs{};
bool native_logged{};
gears_vr::Vector last_logged_center{};bool center_logged{};
std::uint64_t last_frame{};
// Used only inside tick's synchronous, read-only metadata span, after the
// complete copy_frame_on_gt game observation. No engine call or user action
// occurs before draw's final dispatch. Do not reuse this as an equipment,
// inventory, bone-write or action-ownership permit.
bool enclosed_current(hand::Frame const& f) noexcept {
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
       !weaponhand::client.armed()||!weaponhand::client.registration_alive(
            f.key.registration_cookie,f.key.native_started_qpc)||
       !hand::frame_active(f,nativeequipment::clock_ns(nullptr))||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       !hb::geometry_current(f))return false;
    for(const auto& expected:{f.world,f.controller,f.pawn,f.weapon}){
        hand::ObjectIdentity actual{};
        if(!nativeequipment::identify(nullptr,expected.address,actual)||!hand::same(expected,actual))return false;
    }
    return true;
}
bool draw(hand::Frame const& f,gears_vr::Vector center,double radius) noexcept {
    if(!gears_vr::finite(center)||!std::isfinite(radius)||radius<=0||radius>20||!enclosed_current(f))return false;
    playground::Args call(f.world.address,L"DrawDebugSphere");
    if(!call.valid||!registered(call.r.function)||call.r.size>64)return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    if(!registered(owner)||!registered(package)||!playground::named(owner,L"Actor")||!playground::named(package,L"Engine")||
       get<uptr>(package+0x40)!=0||(get<unsigned>(call.r.function+0xf4)&0x2400)!=0x2400||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr)))return false;
    if(log_file&&!native_logged){
        fwprintf(log_file,L"hammerburst_guide_native function=%llx native=%llx module=%llx\n",
            call.r.function,get<uptr>(call.r.function+0x114),reinterpret_cast<uptr>(GetModuleHandleW(nullptr)));
        fflush(log_file);native_logged=true;
    }
    const wchar_t* fields[]={L"Center",L"Radius",L"Segments",L"R",L"G",L"B",L"bPersistentLines"};
    const wchar_t* kinds[]={L"StructProperty",L"FloatProperty",L"IntProperty",L"ByteProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned bytes[]={12,4,4,1,1,1,4};std::uint64_t occupied{};
    for(unsigned i=0;i<7;++i){
        const auto p=call.prop(fields[i]);
        if(!hammerburst_animation::input_field(call,p,kinds[i],bytes[i]))return false;
        const auto offset=get<unsigned>(p+0x94);
        for(unsigned n=0;n<bytes[i];++n){const auto bit=std::uint64_t(1)<<(offset+n);if(occupied&bit)return false;occupied|=bit;}
        if(i==0){const auto type=get<uptr>(p+0xd0);
            if(!registered(type)||!playground::named(type,L"Vector")||get<unsigned>(type+0x88)!=12)return false;}
    }
    const playground::Vector point{float(center.x),float(center.y),float(center.z)};
    return call.set(L"Center",point).set(L"Radius",float(radius)).set(L"Segments",12)
        .set(L"R",static_cast<unsigned char>(255)).set(L"G",static_cast<unsigned char>(225))
        .set(L"B",static_cast<unsigned char>(0)).valid&&
        nativelancerfollow::bool_arg(call,L"bPersistentLines",false)&&enclosed_current(f)&&call.run();
}
void tick(uptr pc) noexcept {
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||hb::stage==hb::Stage::recovering||
       hb::stage==hb::Stage::completing)return;
    // The held latch itself already supplies the current action feedback.
    // Optional reflected debug drawing must not compete with the live stroke
    // for its unchanged 100 ms continuity/freshness budget. Show the region
    // again when released, before a new grab.
    if(hb::stroke.active())return;
    // An optional outline cannot exist without this same held geometry/plan.
    // This is only an early rejection; drawing still performs all live checks.
    if(!hb::geometry.known||!nativelancerfollow::plan.valid||!hb::geometry_current(nativelancerfollow::plan.source))return;
    hand::Frame f{};hand::GripGameState game{};
    if(!weaponhand::client.copy_frame_on_gt(f,game)||f.controller.address!=pc||!game.known||!game.main_hand_held||
       game.busy||game.animation_owns_hands||f.key.frame==last_frame||!hb::geometry_current(f))return;
    unsigned step{};
    switch(hb::stage){
    case hb::Stage::idle:{int used{};if(!lancerreload_inventory::integer(f.weapon.address,L"AmmoUsedCount",used)||used<=0)return;step=1;break;}
    case hb::Stage::acquiring:case hb::Stage::opening:case hb::Stage::closing:step=1;break;
    case hb::Stage::waiting_ammo:step=2;break;
    case hb::Stage::old_magazine:case hb::Stage::new_magazine:step=3;break;
    default:return;
    }
    // Consume only this callback's already prepared same-frame data. There
    // has been no engine call since the full source/game observation above.
    const auto& plan=nativelancerfollow::plan;
    if(!plan.valid||!hand::same_frame(f,plan.source)||!enclosed_current(f))return;
    const auto& rig=plan.prepared;
    if(!std::isfinite(rig.units_per_metre)||
       rig.units_per_metre<10||rig.units_per_metre>500||!(rig.world_pose_mask&gears_rig_tracking::right_grip_pose))return;
    gears_vr::Vector world{};double radius=.022*rig.units_per_metre;
    if(step==2){
        gears_vr::Vector raw{};const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
        if(!gears_weapon::shoulder_ammo_center(nativeequipment::calibration,body,raw)||
           !gears_vr::rig_tracking::point(rig.base_view,rig.reference,raw,rig.units_per_metre,world))return;
        radius=.055*rig.units_per_metre;
    }else{
        const auto& right=rig.world_grip[1];
        const gears_vr::Viewpoint grip{right.position,gears_vr::ue_angles(gears_vr::rotation_matrix(right.orientation))};
        std::array<float,16> raw{};hb::P gun{};double size{};
        if(!nativelancerfollow::mount.model_matrix(grip,raw)||!hb::matrix(raw,gun,size))return;
        const auto local=step==1?hb::contact():hb::feed.port_in_weapon.position;
        world=hb::add(gun.position,hb::scale(gears_weapon::rotate(gun.orientation,local),size*rig.units_per_metre));
        radius*=size;
    }
    // Private visibility discrimination only, never a delivered guide point.
    // Require the same isolated test/mute/behind guards as the owning runner.
    static const bool visibility_probe=[](){
        wchar_t value[4]{},mute[4]{},behind[4]{};
        return GetEnvironmentVariableW(L"GEARSVR_TEST_GUIDE_VISIBLE_PROBE",value,4)==1&&value[0]==L'1'&&
            GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",mute,4)==1&&mute[0]==L'1'&&
            GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)==1&&behind[0]==L'1';
    }();
    if(visibility_probe){
        world=hb::add(rig.world_head.position,gears_weapon::rotate(rig.world_head.orientation,
            {.8*rig.units_per_metre,0,0}));radius=.05*rig.units_per_metre;
    }
    last_frame=f.key.frame;
    const bool dispatched=draw(f,world,radius);
    const bool center_moved=!center_logged||hb::length(hb::sub(world,last_logged_center))>5.;
    if(log_file&&logs<12&&(step!=previous_step||(!dispatched&&logs<3)||(dispatched&&center_moved))){
        fwprintf(log_file,L"hammerburst_guide step=%u dispatched=%u world=%.9g,%.9g,%.9g radius=%.9g frame=%llu one_frame=1 visibility_probe=%u\n",
            step,unsigned(dispatched),world.x,world.y,world.z,radius,f.key.frame,unsigned(visibility_probe));fflush(log_file);++logs;
        if(dispatched){last_logged_center=world;center_logged=true;}
    }
    if(dispatched)previous_step=step;
}
} // namespace hammerburst_guide
