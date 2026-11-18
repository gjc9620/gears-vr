// Included inside lancer_interaction_ranges. Preview never calls a publisher,
// captures a PartFit, creates a lease, or writes a real skeletal atom.
bool preview(hand::Frame const& f,gears_committed_interaction::Frame const& actual,
    std::array<float,16> const& gun,double units,gears_pose_editor::Config const& view,
    playground::Args& call,hand::ObjectIdentity const& fn) noexcept {
    namespace edit=gears_pose_editor;namespace vr=gears_vr;
    const unsigned part=edit::part(view),side=part?0u:1u;
    edit::Config config{};pose_editor_client::read(config,part);
    vr::Pose wrist{},mesh{};double model_scale{};
    if(!hammerburst_runtime::matrix(gun,mesh,model_scale))return false;
    gears_hand_authored_mount::Mount hand_mount{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
       side?gears_hand_authored_mount::Side::right:gears_hand_authored_mount::Side::left,
       actual.model_scale,units,hand_mount))return false;
    const auto from_grip=[&](vr::Pose const& grip){
        const auto shift=vr::transform(vr::rotation_matrix(grip.orientation),hand_mount.calibration.grip_to_wrist_metres);
        wrist={{grip.position.x+shift.x*units,grip.position.y+shift.y*units,grip.position.z+shift.z*units},
            vr::normalized(vr::multiply(grip.orientation,hand_mount.calibration.grip_to_wrist_rotation))};
    };
    std::array<vr::Quaternion,15> joints{};
    if(part==0){
        V marker{};if(!gears_weapon_lancer_contact::palm_marker_in_wrist(marker))return false;
        const auto p=actual.right.right_grip;
        const auto delta=vr::transform(vr::rotation_matrix(p.orientation),marker);
        wrist={{p.position.x-delta.x,p.position.y-delta.y,p.position.z-delta.z},p.orientation};
        joints=gears_lancer_main_hand::rest;
    }else if(part==1){
        V palm{};if(!gears_weapon_lancer_foregrip::grip_in_mesh(palm))return false;
        const auto q=vr::normalized(vr::multiply(mesh.orientation,gears_weapon_lancer_foregrip::wrist_in_mesh()));
        const auto grip_q=vr::normalized(vr::multiply(q,vr::inverse(hand_mount.calibration.grip_to_wrist_rotation)));
        from_grip({line::point(gun,palm),grip_q});
        std::copy_n(gears_lancer_natural_hand::support_closed,15,joints.begin());
    }else if(part==2){
        const auto component=playground::pointer(f.weapon.address,L"MagazineMesh");
        std::array<float,16> matrix{},after{};vr::Pose item{};double size{};vr::Viewpoint result{};
        if(!lancerreload_visual::active){
            vr::Viewpoint seated{};
            if(!gears_weapon::lancer_seated_magazine(gun,seated)||
               !gears_lancer_natural_magazine::grip.wrist_for_item(seated,result,actual.model_scale)||
               !vr::rig_tracking::detail::quaternion(vr::ue_matrix(result.rotation),wrist.orientation))return false;
        }else if(!lancer_grasp::natural_magazine_current(f)||!playground::instance(component,L"StaticMeshComponent")||
           !copy(matrix.data(),component+0xa0,sizeof matrix)||!hammerburst_runtime::matrix(matrix,item,size)||
           !gears_lancer_natural_magazine::grip.wrist_for_item({item.position,vr::ue_angles(vr::rotation_matrix(item.orientation))},result,actual.model_scale)||
           !copy(after.data(),component+0xa0,sizeof after)||matrix!=after||
           playground::pointer(f.weapon.address,L"MagazineMesh")!=component||
           !vr::rig_tracking::detail::quaternion(vr::ue_matrix(result.rotation),wrist.orientation))return false;
        wrist.position=result.position;joints=gears_lancer_part_hand::magazine;
    }else{
        vr::Pose physical{},grip{};if(!lancerreload_visual::physical_wrist(actual,physical))return false;
        V contact{};vr::Quaternion motion{};
        const auto kind=part==3?gears_hand_object_grasp::Kind::loading:gears_hand_object_grasp::Kind::starter;
        if(part==3){
            auto p=gears_weapon::lancer_front::contact_raw;
            if(lancer_charging::current_geometry(f)){
                const auto d=lancer_charging::last_observed;
                if(!std::isfinite(d)||d<0||d>lancer_charging::geometry.slide.travel+.001)return false;
                const auto axis=lancer_charging::geometry.slide.axis;
                p={p.x+axis.x*d*units,p.y+axis.y*d*units,p.z+axis.z*d*units};
            }
            contact=line::point(gun,p);joints=gears_lancer_part_hand::loading;
        }else{
            lancersaw_visual::Observation saw{};if(!lancersaw_visual::observe(f,saw))return false;
            contact=saw.grasp;motion={0,0,std::sin(saw.angle*.5),std::cos(saw.angle*.5)};
            joints=gears_lancer_part_hand::starter;
        }
        if(!gears_weapon_lancer_grasp::part_grip(kind,mesh.orientation,motion,contact,physical,
            actual.model_scale,units,grip))return false;
        from_grip(grip);
    }
    // Main alignment moves the gun's mount; its physical wrist stays current.
    // Left alignment changes only the existing cosmetic wrist solve.
    if(part)wrist=edit::adjusted_grip(config,wrist,units);
    line::Shape shape{};std::array<vr::Pose,15> world{};
    auto const* local=side?gears_pose_editor_rig::right:gears_pose_editor_rig::left;
    for(unsigned n=0;n<15;++n){
        const auto parent=n%3?world[n-1]:wrist;
        const auto shift=vr::transform(vr::rotation_matrix(parent.orientation),scale(local[n],actual.model_scale));
        world[n]={plus(parent.position,shift),vr::normalized(vr::multiply(parent.orientation,edit::finger(config,n,joints[n])))};
        if(!edge(shape,parent.position,world[n].position))return false;
        if(n%3==2)for(auto axis:{V{.01*units,0,0},V{0,.01*units,0},V{0,0,.01*units}}){
            const auto tip=plus(world[n].position,vr::transform(vr::rotation_matrix(world[n].orientation),axis));
            if(!edge(shape,world[n].position,tip))return false;
        }
    }
    Range color{};color.r=255;color.g=230;color.b=40;
    if(!lines(f,call,fn,shape,color))return false;
    pose_editor_client::receipt(view.revision,f.key.frame,3);return true;
}
