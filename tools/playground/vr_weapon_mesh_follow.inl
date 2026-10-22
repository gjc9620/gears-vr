// Included inside vrequip. Private Lancer-only motion trial, never a default
// launcher path. All component calls and saved state live on the game thread.
std::atomic<bool> mesh_follow_requested{};
std::atomic<bool> mesh_follow_shutdown_requested{};
std::atomic<bool> mesh_follow_active_visible{};
std::atomic<bool> mesh_follow_support_visible{};
std::atomic<unsigned> mesh_follow_status{};
gears_weapon::HandWorldFrame mesh_follow_hands{};
bool mesh_follow_hands_known{};
struct MeshFollowState {
    uptr world{},pawn{},weapon{},mesh{};
    std::uint64_t session{},space_epoch{},input_epoch{},world_generation{};
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
        mesh_follow_support_visible.store(false,std::memory_order_release);
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
    mesh_follow_support_visible.store(false,std::memory_order_release);
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
         mesh_follow.input_epoch!=pose.input_epoch||
         mesh_follow.world_generation!=pose.world_generation))){
        mesh_follow_restore();
        if(requested&&!mesh_follow.active)mesh_follow_status.store(1,std::memory_order_release);
        return;
    }
    if(!mesh_follow.active){
        MeshPoseProbe probe{};
        if(!playground::instance(pose.weapon,L"Weap_AssaultRifle")||
           !mesh_pose_probe(pose.weapon,probe)||probe.hidden||!probe.asset||
           !playground::instance(probe.asset,L"SkeletalMesh")||
           wcscmp(probe.asset_name,L"COGAssaultRifle")){
            mesh_follow_status.store(3,std::memory_order_release);return;
        }
        std::array<float,16> matrix{};
        std::memcpy(matrix.data(),probe.local_to_world,sizeof(probe.local_to_world));
        gears_weapon::WeaponMeshFollow solver;
        if(!solver.bind(pose.right,matrix,(pose.grip_down_mask&1)!=0)){
            mesh_follow_status.store(4,std::memory_order_release);return;
        }
        mesh_follow.world=playground::arena_world;
        mesh_follow.pawn=pose.pawn;mesh_follow.weapon=pose.weapon;
        mesh_follow.mesh=probe.mesh;mesh_follow.original=probe;
        mesh_follow.session=pose.session;mesh_follow.space_epoch=pose.space_epoch;
        mesh_follow.input_epoch=pose.input_epoch;
        mesh_follow.world_generation=pose.world_generation;
        mesh_follow.solver=solver;mesh_follow.active=true;
        mesh_follow_active_visible.store(true,std::memory_order_release);
        if(!mesh_follow_absolute(probe.mesh,true,true,probe.absolute_scale)){
            mesh_follow_restore();mesh_follow_status.store(5,std::memory_order_release);return;
        }
    }
    gears_vr::Viewpoint target{};
    bool support_used{};
    constexpr gears_vr::Vector lancer_left_palm_in_gun{8.556242,43.525180,.165157};
    if(!mesh_follow.solver.solve_with_support(pose.right,pose.left,
        (pose.valid_mask&1)!=0,(pose.grip_down_mask&1)!=0,
        lancer_left_palm_in_gun,target,support_used)){
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
    mesh_follow_support_visible.store(support_used,std::memory_order_release);
    mesh_follow_status.store(2,std::memory_order_release);
}

// One bounded, opt-in original-animation probe. It runs after the original
// PlayerTick on the same game thread, before this candidate moves the gun.
// No sampled transform is used as a grip mount or written back to the game.
std::atomic<bool> support_probe_requested{};
std::atomic<unsigned> support_probe_status{},support_probe_count{};
struct SupportProbeState {
    uptr world{},pawn{},weapon{},pawn_mesh{},weapon_mesh{};
    std::array<int,3> bones{};
    ULONGLONG next_ms{};
};
SupportProbeState support_probe{};

bool support_probe_names(std::array<Name,3>& names){
    constexpr const wchar_t* wanted[]={L"b_MF_IK_Hand_L",L"b_MF_Hand_L",L"b_MF_Hand_R"};
    const auto array=get<Array>(base+0x1c34800);
    if(!array.data||array.count<1000||array.count>array.capacity||array.capacity>4000000)return false;
    unsigned found=0;
    for(int i=0;i<array.count&&found!=7;++i){
        const auto entry=get<uptr>(array.data+size_t(i)*8);
        if(!entry||get<unsigned>(entry)!=unsigned(i))continue;
        wchar_t value[40]{};
        if(!copy(&value[0],entry+20,2)||value[0]!=L'b')continue;
        bool complete=false;
        for(unsigned n=1;n<40;++n){
            if(!copy(&value[n],entry+20+n*2,2))break;
            if(!value[n]){complete=true;break;}
        }
        if(!complete)continue;
        for(unsigned n=0;n<3;++n)if(!wcscmp(value,wanted[n])){
            names[n]={unsigned(i),0};found|=1u<<n;
        }
    }
    return found==7;
}
bool support_probe_bone_index(uptr mesh,Name name_value,int& out){
    playground::Args call(mesh,L"MatchRefBone");
    const auto input=call.prop(L"BoneName"),result=call.prop(L"ReturnValue");
    if(!call.valid||!input||!result||get<unsigned>(input+0x74)!=sizeof(Name)||
       !playground::named(get<uptr>(input+0x50),L"NameProperty")||
       get<unsigned>(result+0x74)!=sizeof(int)||
       !playground::named(get<uptr>(result+0x50),L"IntProperty")||
       !call.set(L"BoneName",name_value).run())return false;
    out=call.output<int>();return out>=0&&out<256;
}
bool support_probe_bone_matrix(uptr mesh,int bone,std::array<float,16>& out){
    playground::Args call(mesh,L"GetBoneMatrix");
    const auto input=call.prop(L"BoneIndex"),result=call.prop(L"ReturnValue");
    if(!call.valid||!input||!result||get<unsigned>(input+0x74)!=sizeof(int)||
       !playground::named(get<uptr>(input+0x50),L"IntProperty")||
       get<unsigned>(result+0x74)!=sizeof(out)||
       !playground::named(get<uptr>(result+0x50),L"StructProperty")||
       !call.set(L"BoneIndex",bone).run())return false;
    out=call.output<std::array<float,16>>();
    for(float value:out)if(!std::isfinite(value))return false;
    return true;
}
bool support_probe_component_matrix(uptr mesh,std::array<float,16>& out){
    const auto property=playground::property(mesh,L"LocalToWorld");
    const auto cls=get<uptr>(mesh+0x50);
    if(!property||get<unsigned>(property+0x74)!=sizeof(out)||
       !playground::named(get<uptr>(property+0x50),L"StructProperty"))return false;
    const auto offset=get<unsigned>(property+0x94),size=get<unsigned>(cls+0x88);
    if(offset>size||sizeof(out)>size-offset||!copy(out.data(),mesh+offset,sizeof(out)))return false;
    for(float value:out)if(!std::isfinite(value))return false;
    return true;
}
void support_probe_fail(const wchar_t* reason){
    support_probe_requested.store(false,std::memory_order_release);
    support_probe_status.store(4,std::memory_order_release);
    if(log_file){fwprintf(log_file,L"lancer_support_probe_failed reason=%ls samples=%u\n",
        reason,support_probe_count.load());fflush(log_file);}
}
void support_probe_print_matrix(const wchar_t* label,float const* values){
    fwprintf(log_file,L" %ls=",label);
    for(unsigned i=0;i<16;++i)fwprintf(log_file,L"%ls%.9g",i?L",":L"",values[i]);
}
void support_probe_post_tick(uptr pc){
    if(!support_probe_requested.load(std::memory_order_acquire))return;
    if(!playground::ready||!test_environment()||
       mesh_follow_requested.load(std::memory_order_acquire)||mesh_follow.active){
        support_probe_fail(L"environment_or_mesh_follow");return;
    }
    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto weapon=playground::pointer(pawn,L"Weapon");
    if(pc!=playground::arena_pc||pawn!=playground::arena_pawn||
       playground::pointer(pc,L"WorldInfo")!=playground::arena_world||
       !playground::instance(weapon,L"Weap_AssaultRifle"))return;
    bool switching{};
    if(!read_bool(pawn,L"bSwitchingWeapons",switching)||switching)return;
    const auto pawn_mesh=playground::pointer(pawn,L"Mesh");
    MeshPoseProbe gun{};
    if(!playground::instance(pawn_mesh,L"SkeletalMeshComponent")||
       playground::deleted(pawn_mesh)||!mesh_pose_probe(weapon,gun)||gun.hidden)return;
    if(!log_file){support_probe_fail(L"log_unavailable");return;}
    if(!support_probe.pawn){
        std::array<Name,3> names{};
        if(!support_probe_names(names)){support_probe_fail(L"bone_names_missing");return;}
        SupportProbeState candidate{};
        candidate.world=playground::arena_world;candidate.pawn=pawn;
        candidate.weapon=weapon;candidate.pawn_mesh=pawn_mesh;candidate.weapon_mesh=gun.mesh;
        for(unsigned i=0;i<3;++i)if(!support_probe_bone_index(pawn_mesh,names[i],candidate.bones[i])){
            support_probe_fail(L"bone_index_missing");return;
        }
        if(candidate.bones[0]==candidate.bones[1]||candidate.bones[0]==candidate.bones[2]||
           candidate.bones[1]==candidate.bones[2]){
            support_probe_fail(L"bone_indices_alias");return;
        }
        support_probe=candidate;
        fwprintf(log_file,L"lancer_support_probe_begin pawn=%p weapon=%p pawn_mesh=%p gun_mesh=%p bones=%d,%d,%d\n",
            reinterpret_cast<void*>(pawn),reinterpret_cast<void*>(weapon),
            reinterpret_cast<void*>(pawn_mesh),reinterpret_cast<void*>(gun.mesh),
            candidate.bones[0],candidate.bones[1],candidate.bones[2]);fflush(log_file);
    }
    if(support_probe.world!=playground::arena_world||support_probe.pawn!=pawn||
       support_probe.weapon!=weapon||support_probe.pawn_mesh!=pawn_mesh||
       support_probe.weapon_mesh!=gun.mesh){support_probe_fail(L"identity_changed");return;}
    const auto now_ms=GetTickCount64();
    if(now_ms<support_probe.next_ms)return;
    std::array<float,16> pawn_before{},pawn_after{};
    if(!support_probe_component_matrix(pawn_mesh,pawn_before)){
        support_probe_fail(L"pawn_matrix_unreadable");return;
    }
    std::array<std::array<float,16>,3> bone_matrices{};
    for(unsigned i=0;i<3;++i)if(!support_probe_bone_matrix(pawn_mesh,support_probe.bones[i],bone_matrices[i])){
        support_probe_fail(L"bone_matrix_unreadable");return;
    }
    MeshPoseProbe after{};
    if(!support_probe_component_matrix(pawn_mesh,pawn_after)||
       std::memcmp(pawn_before.data(),pawn_after.data(),sizeof(pawn_before))||
       !mesh_pose_probe(weapon,after)||after.mesh!=gun.mesh||
       std::memcmp(gun.local_to_world,after.local_to_world,sizeof(gun.local_to_world))){
        support_probe_fail(L"component_changed_during_sample");return;
    }
    const auto sample=support_probe_count.load(std::memory_order_relaxed);
    LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
    fwprintf(log_file,L"lancer_support_pose sample=%u qpc=%lld",sample,stamp.QuadPart);
    support_probe_print_matrix(L"pawn",pawn_before.data());
    support_probe_print_matrix(L"gun",gun.local_to_world);
    support_probe_print_matrix(L"ik_left",bone_matrices[0].data());
    support_probe_print_matrix(L"hand_left",bone_matrices[1].data());
    support_probe_print_matrix(L"hand_right",bone_matrices[2].data());
    fwprintf(log_file,L"\n");fflush(log_file);
    support_probe_count.store(sample+1,std::memory_order_release);
    support_probe.next_ms=now_ms+50;
    if(sample+1==24){
        support_probe_requested.store(false,std::memory_order_release);
        support_probe_status.store(3,std::memory_order_release);
    }else support_probe_status.store(2,std::memory_order_release);
}
