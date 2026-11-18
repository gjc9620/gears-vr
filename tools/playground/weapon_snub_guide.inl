// Cosmetic single-frame next-part hint on the existing GT owner. Never grants
// a hand lease, changes geometry, writes a bone or modifies reload state.
namespace snub_runtime {
namespace snub_guide_detail {
std::uint64_t last_frame{};unsigned logs{},last_step{~0u};
bool current(hand::Frame const& f) noexcept {
    return weaponhand::current_gt(nullptr)&&visual::eligible(f)&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        !lancerreload_runtime::stop_requested.load(std::memory_order_acquire);
}
bool draw(hand::Frame const& f,V center,double radius) noexcept {
    if(!gears_vr::finite(center)||!std::isfinite(radius)||radius<=0||radius>20||!current(f))return false;
    playground::Args call(f.world.address,L"DrawDebugSphere");
    if(!call.valid||!registered(call.r.function)||call.r.size>64)return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    if(!registered(owner)||!registered(package)||!playground::named(owner,L"Actor")||
       !playground::named(package,L"Engine")||get<uptr>(package+0x40)||
       (get<unsigned>(call.r.function+0xf4)&0x2400)!=0x2400||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr)))return false;
    const wchar_t* fields[]={L"Center",L"Radius",L"Segments",L"R",L"G",L"B",L"bPersistentLines"};
    const wchar_t* kinds[]={L"StructProperty",L"FloatProperty",L"IntProperty",L"ByteProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned bytes[]={12,4,4,1,1,1,4};std::uint64_t occupied{};
    for(unsigned i=0;i<7;++i){
        const auto p=call.prop(fields[i]);
        if(!anim::input_field(call,p,kinds[i],bytes[i]))return false;
        const auto offset=get<unsigned>(p+0x94);if(offset>64-bytes[i])return false;
        for(unsigned n=0;n<bytes[i];++n){const auto bit=std::uint64_t(1)<<(offset+n);if(occupied&bit)return false;occupied|=bit;}
        if(i==0){const auto type=get<uptr>(p+0xd0);
            if(!registered(type)||!playground::named(type,L"Vector")||get<unsigned>(type+0x88)!=12)return false;}
    }
    const playground::Vector point{float(center.x),float(center.y),float(center.z)};
    return call.set(L"Center",point).set(L"Radius",float(radius)).set(L"Segments",12)
        .set(L"R",static_cast<unsigned char>(255)).set(L"G",static_cast<unsigned char>(225))
        .set(L"B",static_cast<unsigned char>(0)).valid&&
        nativelancerfollow::bool_arg(call,L"bPersistentLines",false)&&current(f)&&call.run();
}
}
void guide_tick(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    namespace detail=snub_guide_detail;
    // The moving held part supplies immediate feedback. Optional reflected
    // drawing must not compete with the live stroke or actual R2 admission.
    if(!manual_enabled()||!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       !f.left_tracked||!f.left_grip_known||f.left_grip_down||f.controllers.hands[1].trigger>.2f||
       !visual::ready||!visual::current(f)||detail::last_frame==f.key.frame)return;
    if(stage==Stage::charging&&snub_charging::stroke.phase()!=snub_charging::Stroke::Phase::idle)return;
    unsigned step{};
    switch(stage){
    case Stage::idle:step=find_debt(inv::identity(f))>=0?3u:1u;break;
    case Stage::waiting_ammo:if(!transaction.target())return;step=2;break;
    case Stage::charging:step=3;break;
    default:return;
    }
    const auto& plan=nativelancerfollow::plan;
    if(!plan.valid||!hand::same_frame(f,plan.source)||!detail::current(f))return;
    const auto& rig=plan.prepared;const double units=rig.units_per_metre;
    if(!std::isfinite(units)||units<10||units>500)return;
    V center{};double radius=.025*units;
    if(step==2){
        V raw{};const P body{f.stereo.head.position,f.reference.orientation};
        if(!gears_weapon::shoulder_ammo_center(nativeequipment::calibration,body,raw)||
           !gears_vr::rig_tracking::point(rig.base_view,rig.reference,raw,units,center))return;
        radius=.055*units;
    }else{
        P gun{};double size{};
        if(!visual::gun_pose(f,gun,size)||std::abs(size-visual::object_scale)>1e-6)return;
        V local=visual::geometry.centre;
        if(step==3){
            if(!snub_charging::contact_known||!(snub_charging::contact_identity==inv::identity(f)))return;
            local=snub_charging::contact_local;
        }
        // Both geometry and contact already bake object scale exactly once.
        center=visual::add(gun.position,gears_weapon::rotate(gun.orientation,local));
    }
    detail::last_frame=f.key.frame;const bool dispatched=detail::draw(f,center,radius);
    if(log_file&&detail::logs<12&&(step!=detail::last_step||!dispatched)){
        fwprintf(log_file,L"snub_guide step=%u dispatched=%u center=%.9g,%.9g,%.9g frame=%llu one_frame=1\n",
            step,unsigned(dispatched),center.x,center.y,center.z,f.key.frame);fflush(log_file);++detail::logs;
    }
    if(dispatched)detail::last_step=step;
}
} // namespace snub_runtime
