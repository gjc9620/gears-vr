// Included inside vrequip. Private Lancer-only motion trial, never a default
// launcher path. All component calls and saved state live on the game thread.
std::atomic<bool> mesh_follow_requested{};
std::atomic<bool> mesh_follow_shutdown_requested{};
std::atomic<bool> mesh_follow_active_visible{};
std::atomic<unsigned> mesh_follow_status{};
gears_weapon::HandWorldFrame mesh_follow_hands{};
bool mesh_follow_hands_known{};
struct MeshFollowState {
    uptr world{},pawn{},weapon{},mesh{};
    std::uint64_t session{},space_epoch{},world_generation{};
    MeshPoseProbe original{};
    gears_weapon::WeaponMeshFollow solver;
    bool active{};
};
MeshFollowState mesh_follow{};

bool mesh_follow_bool_arg(playground::Args& call,const wchar_t* field,bool value){
    const auto p=call.prop(field);
    if(!p||get<unsigned>(p+0x74)!=4||
       !playground::named(get<uptr>(p+0x50),L"BoolProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),mask=get<unsigned>(p+0xd0);
    if(!mask||offset>call.r.size||4>call.r.size-offset)return false;
    unsigned bits{};std::memcpy(&bits,call.r.params+offset,4);
    bits=value?bits|mask:bits&~mask;
    std::memcpy(call.r.params+offset,&bits,4);return true;
}
bool mesh_follow_absolute(uptr mesh,bool translation,bool rotation,bool scale){
    playground::Args call(mesh,L"SetAbsolute");
    return call.valid&&
        mesh_follow_bool_arg(call,L"NewAbsoluteTranslation",translation)&&
        mesh_follow_bool_arg(call,L"NewAbsoluteRotation",rotation)&&
        mesh_follow_bool_arg(call,L"NewAbsoluteScale",scale)&&call.run();
}
template<class T>bool mesh_follow_struct_call(uptr mesh,const wchar_t* function,
                                               const wchar_t* parameter,T const& value){
    playground::Args call(mesh,function);
    const auto p=call.prop(parameter);
    return call.valid&&p&&get<unsigned>(p+0x74)==sizeof(T)&&
        playground::named(get<uptr>(p+0x50),L"StructProperty")&&
        call.set(parameter,value).run();
}
bool mesh_follow_transform(uptr mesh,playground::Vector position,
                           playground::Rotation rotation){
    return mesh_follow_struct_call(mesh,L"SetTranslation",L"NewTranslation",position)&&
        mesh_follow_struct_call(mesh,L"SetRotation",L"NewRotation",rotation);
}
bool mesh_follow_component_alive(){
    return playground::instance(mesh_follow.weapon,L"Weap_AssaultRifle")&&
        !playground::deleted(mesh_follow.weapon)&&
        playground::instance(mesh_follow.mesh,L"SkeletalMeshComponent")&&
        !playground::deleted(mesh_follow.mesh)&&
        playground::pointer(mesh_follow.weapon,L"Mesh")==mesh_follow.mesh;
}
bool mesh_follow_restore(){
    if(!mesh_follow.active)return true;
    if(!mesh_follow_component_alive()){
        mesh_follow={};mesh_follow_active_visible.store(false,std::memory_order_release);
        mesh_follow_status.store(7,std::memory_order_release);return true;
    }
    auto const& saved=mesh_follow.original;
    if(!mesh_follow_absolute(mesh_follow.mesh,saved.absolute_translation,
                            saved.absolute_rotation,saved.absolute_scale)||
       !mesh_follow_transform(mesh_follow.mesh,saved.translation,saved.rotation)){
        mesh_follow_status.store(8,std::memory_order_release);return false;
    }
    MeshPoseProbe verified{};
    if(!mesh_pose_probe(mesh_follow.weapon,verified)||
       verified.mesh!=mesh_follow.mesh||
       verified.absolute_translation!=saved.absolute_translation||
       verified.absolute_rotation!=saved.absolute_rotation||
       verified.absolute_scale!=saved.absolute_scale||
       std::memcmp(&verified.translation,&saved.translation,sizeof(saved.translation))||
       std::memcmp(&verified.rotation,&saved.rotation,sizeof(saved.rotation))){
        mesh_follow_status.store(8,std::memory_order_release);return false;
    }
    mesh_follow={};mesh_follow_active_visible.store(false,std::memory_order_release);
    mesh_follow_status.store(6,std::memory_order_release);return true;
}
bool mesh_follow_pose_current(uptr pc,gears_weapon::HandWorldFrame const& pose){
    if(!(pose.valid_mask&2)||!playground::ready||!test_environment()||
       pc!=playground::arena_pc||pose.controller!=pc||
       pose.pawn!=playground::arena_pawn||
       playground::pointer(pc,L"WorldInfo")!=playground::arena_world||
       playground::pointer(pc,L"Pawn")!=pose.pawn||
       playground::pointer(pose.pawn,L"Weapon")!=pose.weapon||
       model.held_weapon()!=pose.weapon)return false;
    bool busy{};
    return read_bool(pose.pawn,L"bSwitchingWeapons",busy)&&!busy;
}
void mesh_follow_post_tick(uptr pc){
    const auto requested=mesh_follow_requested.load(std::memory_order_acquire);
    const auto now=GetTickCount64();
    const auto& pose=mesh_follow_hands;
    const bool fresh=mesh_follow_hands_known&&now>=pose.created_ms&&
        now-pose.created_ms<=100&&mesh_follow_pose_current(pc,pose);
    if(!requested||!fresh||
       (mesh_follow.active&&(mesh_follow.world!=playground::arena_world||
         mesh_follow.pawn!=pose.pawn||mesh_follow.weapon!=pose.weapon||
         mesh_follow.session!=pose.session||mesh_follow.space_epoch!=pose.space_epoch||
         mesh_follow.world_generation!=pose.world_generation))){
        mesh_follow_restore();
        if(requested&&!mesh_follow.active)mesh_follow_status.store(1,std::memory_order_release);
        return;
    }
    if(!mesh_follow.active){
        MeshPoseProbe probe{};
        if(!playground::instance(pose.weapon,L"Weap_AssaultRifle")||
           !mesh_pose_probe(pose.weapon,probe)||probe.hidden||!probe.asset||
           !playground::instance(probe.asset,L"SkeletalMesh")){
            mesh_follow_status.store(3,std::memory_order_release);return;
        }
        std::array<float,16> matrix{};
        std::memcpy(matrix.data(),probe.local_to_world,sizeof(probe.local_to_world));
        gears_weapon::WeaponMeshFollow solver;
        if(!solver.bind(pose.right,matrix)){
            mesh_follow_status.store(4,std::memory_order_release);return;
        }
        mesh_follow.world=playground::arena_world;
        mesh_follow.pawn=pose.pawn;mesh_follow.weapon=pose.weapon;
        mesh_follow.mesh=probe.mesh;mesh_follow.original=probe;
        mesh_follow.session=pose.session;mesh_follow.space_epoch=pose.space_epoch;
        mesh_follow.world_generation=pose.world_generation;
        mesh_follow.solver=solver;mesh_follow.active=true;
        mesh_follow_active_visible.store(true,std::memory_order_release);
        if(!mesh_follow_absolute(probe.mesh,true,true,probe.absolute_scale)){
            mesh_follow_restore();mesh_follow_status.store(5,std::memory_order_release);return;
        }
    }
    gears_vr::Viewpoint target{};
    if(!mesh_follow.solver.solve(pose.right,target)){
        mesh_follow_restore();mesh_follow_status.store(4,std::memory_order_release);return;
    }
    if(std::abs(target.position.x)>1e7||std::abs(target.position.y)>1e7||
       std::abs(target.position.z)>1e7){
        mesh_follow_restore();mesh_follow_status.store(4,std::memory_order_release);return;
    }
    const playground::Vector translation{float(target.position.x),float(target.position.y),
        float(target.position.z)};
    const playground::Rotation rotation{
        gears_vr::to_rotator_units(target.rotation.pitch),
        gears_vr::to_rotator_units(target.rotation.yaw),
        gears_vr::to_rotator_units(target.rotation.roll)};
    if(!mesh_follow_transform(mesh_follow.mesh,translation,rotation)){
        mesh_follow_restore();mesh_follow_status.store(5,std::memory_order_release);return;
    }
    mesh_follow_status.store(2,std::memory_order_release);
}
