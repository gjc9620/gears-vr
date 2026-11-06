// Same existing C0 publisher; actual HB part observation is defined after its runtime.
namespace lancer_grasp {
bool hammerburst_pose(hand::Frame const& f,hand::Decision const& d,api::Kind& kind,P& goal) noexcept {
    namespace hb=hammerburst_runtime;
    if(!hb::eligible(f)||!hand::index_pinch(f))return false;
    P part{},physical{};V contact{},axis{};double size{};
    const bool latch=(hb::stage==hb::Stage::acquiring||hb::stage==hb::Stage::opening||hb::stage==hb::Stage::closing)&&
        hand::same(hb::held,d.lease);
    if(latch){
        hammerburst_animation::Parts parts{};
        if(!hammerburst_animation::read_parts(f,parts)||parts.component.address!=nativelancerfollow::bound_key.component.address||
           parts.asset.address!=nativelancerfollow::bound_key.asset.address||!hb::matrix(parts.world[1],part,size))return false;
        const auto local=gears_weapon::rotate(part.orientation,{-7.09357487*size,2.10061312*size,1.88769577*size});
        contact={part.position.x+local.x,part.position.y+local.y,part.position.z+local.z};
        axis=gears_weapon::rotate(part.orientation,{0,-1,0});kind=api::Kind::hammerburst_latch;
    }else{
        if(!lancerreload_visual::active||!hand::same(lancerreload_visual::saved.weapon,f.weapon)||
           !lancerreload_visual::alive()||(hb::stage!=hb::Stage::old_magazine&&hb::stage!=hb::Stage::new_magazine))return false;
        std::array<float,16> raw{};
        if(!copy(raw.data(),lancerreload_visual::saved.component.address+0xa0,sizeof raw)||!hb::matrix(raw,part,size))return false;
        contact=part.position;axis=gears_weapon::rotate(part.orientation,{0,1,0});kind=api::Kind::magazine;
    }
    gears_committed_interaction::Frame actual{};
    if(!handrig::read_interaction(actual)||!gears_committed_interaction::matches(actual,f,vrequip::model.hold_generation(),
        weaponhand::clock_ns(nullptr),f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)||
        !lancerreload_visual::physical_wrist(actual,physical))return false;
    const std::array<std::uint64_t,4> epochs{{actual.right.hold_generation,actual.right.producer.stream,
        actual.right.producer.tracking[0],actual.right.producer.tracking[1]}};
    const bool same=hb_fit.known&&hb_kind==kind&&hb_epochs==epochs&&hb_source.press_token==f.press_token&&
        hb_source.interaction_control==f.interaction_control&&hand::same(hb_source.weapon,f.weapon)&&
        hand::same(hb_source.pawn,f.pawn)&&hand::same(hb_source.mesh,f.mesh)&&hand::same(hb_source.world,f.world)&&
        hb_source.key.registration_cookie==f.key.registration_cookie&&hb_source.key.lifecycle==f.key.lifecycle&&
        hb_source.key.native_started_qpc==f.key.native_started_qpc&&hb_source.key.world_generation==f.key.world_generation&&
        hb_source.reference_generation==f.reference_generation&&hb_source.stereo.session==f.stereo.session&&
        hb_source.stereo.space_epoch==f.stereo.space_epoch&&hb_source.controllers.input_epoch==f.controllers.input_epoch&&
        hb_source.controllers.profile_generation==f.controllers.profile_generation;
    if(!hb_fit.solve(!same,part,contact,axis,physical,actual.model_scale,actual.right.units_per_metre,goal))return false;
    hb_source=f;hb_kind=kind;hb_epochs=epochs;return nativeequipment::current(nullptr,f);
}
}
