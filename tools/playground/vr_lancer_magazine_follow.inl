// Private Lancer-only visual trial. The original independent MagazineMesh is
// attached, moved by the certified left-hand pose, and restored on release.
// No insertion, ammo consumption, bolt action, or perfect reward is inferred.
std::atomic<bool> lancer_magazine_active_visible{};
std::atomic<unsigned> lancer_magazine_status{};
std::atomic<bool> lancer_magazine_test_request{};
bool lancer_magazine_test_active{};
ULONGLONG lancer_magazine_test_started{};
gears_weapon::HandWorldFrame lancer_magazine_test_pose{};
gears_vr::Vector lancer_magazine_test_axis{};
struct LancerMagazineFollow {
    uptr world{},pawn{},weapon{},component{},pawn_mesh{};
    std::uint64_t session{},space_epoch{},input_epoch{},world_generation{};
    playground::Vector translation{};
    playground::Rotation rotation{};
    bool absolute_translation{},absolute_rotation{},absolute_scale{},active{};
    gears_weapon::LancerMagazineInsertionStroke insertion{};
    bool insertion_seen{};
    std::uint64_t last_insertion_sequence{};
    ULONGLONG next_log{};
};
LancerMagazineFollow lancer_magazine{};

template<class T>bool lancer_magazine_read_struct(uptr object,const wchar_t* field,T& out){
    const auto property=playground::property(object,field);
    const auto cls=get<uptr>(object+0x50);
    if(!property||!cls||get<unsigned>(property+0x74)!=sizeof(T)||
       !playground::named(get<uptr>(property+0x50),L"StructProperty"))return false;
    const auto offset=get<unsigned>(property+0x94),size=get<unsigned>(cls+0x88);
    return offset<=size&&sizeof(T)<=size-offset&&copy(&out,object+offset,sizeof(T));
}
bool lancer_magazine_bool_call(uptr object,const wchar_t* function,
                               const wchar_t* parameter,bool value){
    playground::Args call(object,function);
    const auto property=call.prop(parameter);
    return call.valid&&property&&get<unsigned>(property+0x74)==4&&
        playground::named(get<uptr>(property+0x50),L"BoolProperty")&&
        get<unsigned>(property+0xd0)!=0&&call.flag(parameter,value).run();
}
bool lancer_magazine_component_alive(){
    return playground::instance(lancer_magazine.weapon,L"Weap_AssaultRifle")&&
        !playground::deleted(lancer_magazine.weapon)&&
        playground::instance(lancer_magazine.pawn,L"WarPawn")&&
        !playground::deleted(lancer_magazine.pawn)&&
        playground::pointer(lancer_magazine.weapon,L"Instigator")==lancer_magazine.pawn&&
        playground::pointer(lancer_magazine.pawn,L"Mesh")==lancer_magazine.pawn_mesh&&
        playground::instance(lancer_magazine.component,L"StaticMeshComponent")&&
        !playground::deleted(lancer_magazine.component)&&
        playground::pointer(lancer_magazine.weapon,L"MagazineMesh")==
            lancer_magazine.component;
}
bool lancer_magazine_restore(){
    auto& state=lancer_magazine;
    if(!state.active)return true;
    if(!lancer_magazine_component_alive()){
        lancer_magazine_test_active=false;
        state={};lancer_magazine_active_visible.store(false,std::memory_order_release);
        lancer_magazine_status.store(7,std::memory_order_release);return true;
    }
    const bool restored=
        lancer_magazine_bool_call(state.weapon,L"SetPawnAmmoAttachment",L"bAttach",false)&&
        lancer_magazine_bool_call(state.weapon,L"SetWeaponAmmoBoneDisplay",L"bShow",true)&&
        mesh_follow_absolute(state.component,state.absolute_translation,
                             state.absolute_rotation,state.absolute_scale)&&
        mesh_follow_transform(state.component,state.translation,state.rotation);
    MagazineVisualProbe probe{};
    if(!restored||!magazine_visual_probe(state.weapon,state.pawn,probe)||
       probe.component!=state.component||probe.attached_to_pawn){
        lancer_magazine_status.store(8,std::memory_order_release);return false;
    }
    lancer_magazine_test_active=false;
    state={};lancer_magazine_active_visible.store(false,std::memory_order_release);
    lancer_magazine_status.store(3,std::memory_order_release);return true;
}
bool lancer_magazine_move(gears_vr::Viewpoint const& left){
    gears_vr::Viewpoint target{};
    if(!gears_weapon::lancer_magazine_from_left(left,target))return false;
    const playground::Vector translation{float(target.position.x),float(target.position.y),
                                         float(target.position.z)};
    const playground::Rotation rotation{
        gears_vr::to_rotator_units(target.rotation.pitch),
        gears_vr::to_rotator_units(target.rotation.yaw),
        gears_vr::to_rotator_units(target.rotation.roll)};
    return mesh_follow_transform(lancer_magazine.component,translation,rotation);
}
bool lancer_magazine_grab(uptr pc,gears_weapon::EquipmentSnapshot const& game,
                          gears_weapon::HandWorldFrame const& pose){
    if(lancer_magazine.active||!lancer_reload_hold.active||
       lancer_reload_hold.world!=game.world||lancer_reload_hold.pawn!=game.pawn||
       lancer_reload_hold.weapon!=game.current_weapon||
       !lancer_same_game(pc,game)||!(pose.valid_mask&1)||!(pose.grip_down_mask&1))return false;
    MagazineVisualProbe magazine{};
    AmmoBoneVisualProbe ammo_bone{};
    if(!magazine_visual_probe(game.current_weapon,game.pawn,magazine)||
       !magazine.present||magazine.hidden||
       wcscmp(magazine.asset_name,L"Assault_Rifle_Magazine")||
       !ammo_bone_visual_probe(game.current_weapon,ammo_bone)||
       !ammo_bone.present||ammo_bone.mesh!=lancer_reload_hold.mesh)return false;
    auto& state=lancer_magazine;
    state.world=game.world;state.pawn=game.pawn;state.weapon=game.current_weapon;
    state.component=magazine.component;
    state.pawn_mesh=playground::pointer(game.pawn,L"Mesh");
    state.session=pose.session;state.space_epoch=pose.space_epoch;
    state.input_epoch=pose.input_epoch;state.world_generation=pose.world_generation;
    gears_weapon::LancerInsertionSample initial{};
    initial.weapon=state.weapon;initial.component=state.component;
    initial.world=state.world;initial.session=state.session;
    initial.space_epoch=state.space_epoch;initial.input_epoch=state.input_epoch;
    initial.world_generation=state.world_generation;
    if(!playground::instance(state.pawn_mesh,L"SkeletalMeshComponent")||
       !state.insertion.arm(initial)||
       !lancer_magazine_read_struct(state.component,L"Translation",state.translation)||
       !lancer_magazine_read_struct(state.component,L"Rotation",state.rotation)||
       !read_bool(state.component,L"AbsoluteTranslation",state.absolute_translation)||
       !read_bool(state.component,L"AbsoluteRotation",state.absolute_rotation)||
       !read_bool(state.component,L"AbsoluteScale",state.absolute_scale)){
        state={};return false;
    }
    state.active=true;
    lancer_magazine_active_visible.store(true,std::memory_order_release);
    if((!magazine.attached_to_pawn&&
        !lancer_magazine_bool_call(state.weapon,L"SetPawnAmmoAttachment",L"bAttach",true))||
       !lancer_magazine_bool_call(state.weapon,L"SetWeaponAmmoBoneDisplay",L"bShow",false)||
       !mesh_follow_absolute(state.component,true,true,state.absolute_scale)||
       !lancer_magazine_move(pose.left)){
        lancer_magazine_restore();lancer_magazine_status.store(8,std::memory_order_release);
        return false;
    }
    lancer_magazine_status.store(1,std::memory_order_release);return true;
}
void lancer_magazine_test_tick(uptr pc,gears_weapon::EquipmentSnapshot const& game){
    if(!lancer_magazine_test_request.exchange(false,std::memory_order_acq_rel))return;
    if(lancer_magazine.active||!lancer_reload_hold.active||
       !lancer_same_game(pc,game)){
        lancer_magazine_status.store(8,std::memory_order_release);return;
    }
    const auto& feed=lancer_reload_hold.feed;
    gears_weapon::HandWorldFrame pose{};
    pose.created_ms=GetTickCount64();pose.session=pose.sequence=1;
    pose.space_epoch=pose.input_epoch=pose.world_generation=1;
    pose.controller=pc;pose.pawn=game.pawn;pose.weapon=game.current_weapon;
    pose.valid_mask=pose.grip_down_mask=1;
    pose.left.position={feed.point_raw.x+feed.outward_axis.x*35,
                        feed.point_raw.y+feed.outward_axis.y*35,
                        feed.point_raw.z+feed.outward_axis.z*35};
    if(!lancer_magazine_grab(pc,game,pose)){
        lancer_magazine_status.store(8,std::memory_order_release);return;
    }
    lancer_magazine_test_pose=pose;
    lancer_magazine_test_axis=feed.outward_axis;
    lancer_magazine_test_started=pose.created_ms;
    lancer_magazine_test_active=true;
}
void lancer_magazine_post_tick(uptr pc){
    auto& state=lancer_magazine;
    if(!state.active)return;
    const auto now=GetTickCount64();
    auto pose=last_intent_pose;
    if(lancer_magazine_test_active){
        pose=lancer_magazine_test_pose;pose.created_ms=now;
        const auto elapsed=now-lancer_magazine_test_started;
        const auto travel=double(std::min<ULONGLONG>(elapsed,1600))*15./1600.;
        pose.left.position.x+=lancer_magazine_test_axis.x*travel;
        pose.left.position.y+=lancer_magazine_test_axis.y*travel;
        pose.left.position.z+=lancer_magazine_test_axis.z*travel;
        if(elapsed>=1800){pose.grip_down_mask=0;lancer_magazine_test_active=false;}
    }
    const bool fresh=(lancer_magazine_test_active||last_intent_pose_known)&&
        now>=pose.created_ms&&
        now-pose.created_ms<=100&&(pose.valid_mask&1)&&(pose.grip_down_mask&1)&&
        pose.controller==pc&&pose.pawn==state.pawn&&pose.weapon==state.weapon&&
        pose.session==state.session&&pose.space_epoch==state.space_epoch&&
        pose.input_epoch==state.input_epoch&&
        pose.world_generation==state.world_generation;
    if(mesh_follow_shutdown_requested.load(std::memory_order_acquire)||!fresh||
       !lancer_reload_hold.active||lancer_reload_hold.weapon!=state.weapon||
       pc!=playground::arena_pc||state.world!=playground::arena_world||
       state.pawn!=playground::arena_pawn||
       playground::pointer(pc,L"Pawn")!=state.pawn||
       playground::pointer(state.pawn,L"Weapon")!=state.weapon){
        lancer_magazine_restore();return;
    }
    MagazineVisualProbe magazine{};
    if(!lancer_magazine_component_alive()||
       !magazine_visual_probe(state.weapon,state.pawn,magazine)||
       magazine.component!=state.component){
        lancer_magazine_restore();return;
    }
    // Original animation notifies can detach the hand mesh and show the gun
    // magazine while the R3 completion timer is held. Reassert only this
    // captured component after the original PlayerTick, never a duplicate.
    if((!magazine.attached_to_pawn&&
        !lancer_magazine_bool_call(state.weapon,L"SetPawnAmmoAttachment",L"bAttach",true))||
       !lancer_magazine_bool_call(state.weapon,L"SetWeaponAmmoBoneDisplay",L"bShow",false)||
       !mesh_follow_absolute(state.component,true,true,state.absolute_scale)||
       !lancer_magazine_move(pose.left)){
        lancer_magazine_restore();lancer_magazine_status.store(8,std::memory_order_release);
        return;
    }
    if(pose.sequence!=state.last_insertion_sequence){
        MeshPoseProbe gun{};
        gears_vr::Viewpoint held_magazine{};
        if(!mesh_pose_probe(state.weapon,gun)||gun.mesh!=lancer_reload_hold.mesh||
           gun.hidden||wcscmp(gun.asset_name,L"COGAssaultRifle")||
           !gears_weapon::lancer_magazine_from_left(pose.left,held_magazine)){
            lancer_magazine_restore();lancer_magazine_status.store(8,std::memory_order_release);
            return;
        }
        gears_weapon::LancerInsertionSample sample{};
        sample.weapon=state.weapon;sample.component=state.component;
        sample.world=state.world;sample.session=state.session;
        sample.space_epoch=state.space_epoch;sample.input_epoch=state.input_epoch;
        sample.world_generation=state.world_generation;
        sample.sequence=pose.sequence;sample.created_ms=pose.created_ms;
        sample.magazine=held_magazine;sample.tracked=true;sample.grip_down=true;
        std::memcpy(sample.gun_mesh.data(),gun.local_to_world,sizeof(gun.local_to_world));
        gears_weapon::LancerInsertionGeometry geometry{};
        const auto result=state.insertion.observe(sample,&geometry);
        state.last_insertion_sequence=pose.sequence;
        if(result==gears_weapon::LancerInsertResult::inserted)
            state.insertion_seen=true;
        if(log_file&&result!=gears_weapon::LancerInsertResult::rejected){
            fwprintf(log_file,L"lancer_magazine_insertion_candidate result=%u "
                L"sequence=%llu depth=%.3f lateral=%.3f alignment=%.3f clock=%.3f\n",
                unsigned(result),pose.sequence,geometry.depth_raw,
                geometry.lateral_raw,geometry.axis_alignment,
                geometry.clock_alignment);fflush(log_file);
        }
    }
    lancer_magazine_status.store(state.insertion_seen?6u:2u,std::memory_order_release);
    if(log_file&&(now>=state.next_log||!magazine.attached_to_pawn)){
        state.next_log=now+500;
        MagazineVisualProbe after{};
        AmmoBoneVisualProbe ammo_bone{};
        std::array<float,16> world{};
        const bool attached=magazine_visual_probe(state.weapon,state.pawn,after)&&
            after.component==state.component&&after.attached_to_pawn;
        const bool matrix=support_probe_component_matrix(state.component,world);
        const bool control=ammo_bone_visual_probe(state.weapon,ammo_bone)&&
            ammo_bone.present;
        fwprintf(log_file,L"lancer_magazine_follow weapon=%p component=%p "
            L"left=%.3f,%.3f,%.3f world=%.3f,%.3f,%.3f "
            L"matrix=%u attached=%u ammo_control=%u target=%.3f reattached=%u\n",
            reinterpret_cast<void*>(state.weapon),reinterpret_cast<void*>(state.component),
            pose.left.position.x,pose.left.position.y,pose.left.position.z,
            world[12],world[13],world[14],unsigned(matrix),unsigned(attached),
            unsigned(control),ammo_bone.target,
            unsigned(!magazine.attached_to_pawn));fflush(log_file);
    }
}
