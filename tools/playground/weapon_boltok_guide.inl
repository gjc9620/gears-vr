// One-frame original world lines only; guidance never changes contact volumes,
// leases, ammunition, mechanics or completion deadlines.
namespace boltok_guide {
namespace hand=gears_weapon_hand;namespace runtime=boltok_runtime;
namespace draw=lancer_interaction_guide;namespace shape=gears_weapon::lancer_guide;
std::uint64_t last_frame{};
bool shoulder(hand::Frame const& f,shape::Shape& out) noexcept {
    gears_rig_tracking::Frame rig{};const auto source=static_cast<std::uint32_t>(f.stereo.source);
    if(handrig::read(rig,0,source).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),source)!=gears_equipment_rig::Status::matched||
       !(rig.world_pose_mask&gears_rig_tracking::left_grip_pose))return false;
    const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};gears_vr::Vector center{};
    if(!gears_weapon::shoulder_ammo_center(nativeequipment::calibration,body,center))return false;
    const auto& raw=f.controllers.hands[0].grip;const auto& world=rig.world_grip[0];
    const auto basis=gears_vr::multiply(gears_vr::rotation_matrix(world.orientation),
        gears_vr::transpose(gears_vr::xr_to_ue(gears_vr::rotation_matrix(raw.orientation))));
    const auto delta=gears_vr::transform(basis,gears_vr::xr_to_ue(gears_weapon::difference(center,raw.position)));
    const auto torso=gears_vr::multiply(basis,gears_vr::xr_to_ue(gears_vr::rotation_matrix(body.orientation)));
    const double units=nativelancerfollow::bound_units;if(!std::isfinite(units)||units<10||units>500)return false;
    return shape::marker(gears_weapon::sum(world.position,gears_weapon::scaled(delta,units)),
        gears_vr::transform(torso,{0,1,0}),gears_vr::transform(torso,{0,0,1}),.065*units,out)&&draw::current(f);
}
void tick(uptr pc) noexcept {
    if(!runtime::outstanding()||runtime::stage==runtime::Stage::recovering||
       runtime::stage==runtime::Stage::completing)return;
    hand::Frame f{};hand::GripGameState game{};hand::Decision d{};
    if(!weaponhand::client.copy_frame_on_gt(f,game,d)||f.controller.address!=pc||f.key.frame==last_frame)return;
    // Closed idle is contextual marker state: skip guidance before expensive measurement.
    // This freshly copied Frame only decides not to draw; it authorizes nothing.
    // Left support is serviced by the shared support owner, not this mechanism marker.
    if(runtime::stage==runtime::Stage::idle&&!runtime::cycle&&!runtime::moving&&
       !runtime::round_item&&!runtime::reload_serial&&
       !runtime::pending_start.known&&!runtime::control.hide_cylinder&&
       runtime::angle==0&&
       (!f.left_grip_down||d.lease.kind==hand::LeaseKind::support))return;
    // Share the existing layout/witness reader for this freshly copied Frame.
    // Each current() still rereads its proof; Engine changes use the original
    // fallback. No geometry, input permission or successful bool is retained.
    std::optional<weaponhand::source_validation::Scope> source_read_scope;
    if(!weaponhand::source_validation::active)source_read_scope.emplace(f);
    if(!runtime::same_source(f)||!draw::current(f)||!game.known||game.busy||game.animation_owns_hands)return;
    shape::Shape lines{};
    const bool supply=runtime::stage==runtime::Stage::opened&&!runtime::round_item&&runtime::cycle&&
        runtime::transaction.inserted()<runtime::transaction.target();
    if(supply){if(!shoulder(f,lines))return;}
    else{
        boltok_parts::Mechanical parts{};
        if(!boltok_parts::measure(f,parts,runtime::control.hide_cylinder?&runtime::cylinder_relative:nullptr))return;
        const auto& root=parts.root_metres;const auto& cylinder=parts.in_root[1];
        gears_vr::Pose bone{gears_weapon::scaled(gears_weapon::sum(root.position,
            gears_weapon::rotate(root.orientation,cylinder.position)),parts.units),
            gears_vr::normalized(gears_vr::multiply(root.orientation,cylinder.orientation))};
        if(runtime::round_item){
            gears_vr::Pose face{};if(!boltok_visual::geometry.cylinder(bone,face))return;
            for(unsigned i=0;i<6;++i)if(!(runtime::live_mask&(1u<<i))){
                gears_vr::Pose round{};shape::Shape marker{};
                if(!boltok_visual::geometry.round(face,i,round)||!shape::marker(round.position,
                    gears_weapon::rotate(face.orientation,{1,0,0}),gears_weapon::rotate(face.orientation,{0,0,1}),
                    .007*gears_weapon::boltok_authoring_raw_per_metre*parts.component_scale,marker))return;
                for(unsigned j=0;j<marker.count&&lines.count<lines.lines.size();++j)lines.lines[lines.count++]=marker.lines[j];
            }
        }else if(!shape::marker(bone.position,gears_weapon::rotate(bone.orientation,{1,0,0}),
            gears_weapon::rotate(bone.orientation,{0,0,1}),.025*parts.units,lines))return;
    }
    playground::Args call(f.world.address,L"DrawDebugLine");hand::ObjectIdentity function{};
    if(!lines.count||!draw::line_call(f,call,function))return;
    last_frame=f.key.frame;
    for(unsigned i=0;i<lines.count;++i){hand::ObjectIdentity now{};
        if(!draw::transport_current(f)||!nativeequipment::identify(nullptr,call.r.function,now)||!hand::same(now,function))return;
        const auto& line=lines.lines[i];
        const playground::Vector a{float(line.start.x),float(line.start.y),float(line.start.z)};
        const playground::Vector b{float(line.end.x),float(line.end.y),float(line.end.z)};
        if(!call.set(L"LineStart",a).set(L"LineEnd",b).valid||!call.run())return;
    }
}
}
