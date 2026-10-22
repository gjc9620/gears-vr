// Private, bounded observation of one original Lancer reload. The original
// ForceReload path is invoked by the owned test harness; this game-thread
// callback only reads the same live weapon after its normal PlayerTick.
std::atomic<bool> reload_visual_requested{};
std::atomic<unsigned> reload_visual_status{},reload_visual_count{};
struct ReloadVisualState {
    uptr world{},pawn{},weapon{},gun_mesh{},magazine{},pawn_mesh{};
    int left_hand_bone{-1};
    int latch_bone{-1},receiver_bone{-1};
    ULONGLONG start_ms{},next_ms{};
    bool saw_reloading{},saw_finish{};
};
ReloadVisualState reload_visual{};

bool reload_visual_name(const wchar_t* wanted,Name& out){
    if(!wanted)return false;
    const auto length=wcslen(wanted)+1;
    if(length<2||length>64)return false;
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||
       names.capacity>4000000)return false;
    for(int index=0;index<names.count;++index){
        const auto entry=get<uptr>(names.data+size_t(index)*8);
        if(!entry||get<unsigned>(entry)!=unsigned(index))continue;
        wchar_t value[64]{};
        if(copy(value,entry+20,length*sizeof(wchar_t))&&
           !std::memcmp(value,wanted,length*sizeof(wchar_t))){
            out={unsigned(index),0};return true;
        }
    }
    return false;
}

void reload_visual_fail(const wchar_t* reason){
    reload_visual_requested.store(false,std::memory_order_release);
    reload_visual_status.store(4,std::memory_order_release);
    if(log_file){fwprintf(log_file,L"lancer_reload_visual_failed reason=%ls samples=%u\n",
        reason,reload_visual_count.load());fflush(log_file);}
}
bool reload_visual_int(uptr object,const wchar_t* field,int& value){
    const auto p=playground::property(object,field);
    if(!p||get<unsigned>(p+0x74)!=sizeof(int)||
       !playground::named(get<uptr>(p+0x50),L"IntProperty"))return false;
    const auto cls=get<uptr>(object+0x50);
    const auto offset=get<unsigned>(p+0x94);
    const auto size=get<unsigned>(cls+0x88);
    if(offset>size||sizeof(int)>size-offset)return false;
    value=get<int>(object+offset);return value>=0&&value<1000000;
}
bool reload_visual_is_reloading(uptr weapon,bool& value){
    playground::Args call(weapon,L"IsReloading");
    if(!result_property(call,L"BoolProperty",4)||!call.run())return false;
    const auto mask=get<unsigned>(call.prop(L"ReturnValue")+0xd0);
    if(!mask)return false;
    value=(call.output<unsigned>()&mask)!=0;return true;
}
void reload_visual_post_tick(uptr pc){
    if(!reload_visual_requested.load(std::memory_order_acquire))return;
    if(!playground::ready||!test_environment()||
       mesh_follow_requested.load(std::memory_order_acquire)||mesh_follow.active||
       support_probe_requested.load(std::memory_order_acquire)){
        reload_visual_fail(L"environment_or_other_probe");return;
    }
    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto weapon=playground::pointer(pawn,L"Weapon");
    if(pc!=playground::arena_pc||pawn!=playground::arena_pawn||
       playground::pointer(pc,L"WorldInfo")!=playground::arena_world||
       !playground::instance(weapon,L"Weap_AssaultRifle")||
       playground::deleted(weapon)||playground::pointer(weapon,L"Instigator")!=pawn){
        reload_visual_fail(L"identity_changed");return;
    }
    const auto now=GetTickCount64();
    if(now<reload_visual.next_ms)return;
    MeshPoseProbe gun{};
    MagazineVisualProbe magazine{};
    AmmoBoneVisualProbe ammo_bone{};
    if(!mesh_pose_probe(weapon,gun)||gun.hidden||
       wcscmp(gun.asset_name,L"COGAssaultRifle")||
       !magazine_visual_probe(weapon,pawn,magazine)||!magazine.present||
       wcscmp(magazine.asset_name,L"Assault_Rifle_Magazine")||
       !ammo_bone_visual_probe(weapon,ammo_bone)||!ammo_bone.present){
        reload_visual_fail(L"original_components_unavailable");return;
    }
    if(!reload_visual.weapon){
        const auto pawn_mesh=playground::pointer(pawn,L"Mesh");
        std::array<Name,3> hand_names{};int left_hand_bone{};
        Name latch_name{},receiver_name{};
        int latch_bone{},receiver_bone{};
        if(!playground::instance(pawn_mesh,L"SkeletalMeshComponent")||
           !support_probe_names(hand_names)||
           !support_probe_bone_index(pawn_mesh,hand_names[1],left_hand_bone)||
           !reload_visual_name(L"b_AR_Latch",latch_name)||
           !reload_visual_name(L"b_AR_Receiver",receiver_name)||
           !support_probe_bone_index(gun.mesh,latch_name,latch_bone)||
           !support_probe_bone_index(gun.mesh,receiver_name,receiver_bone)||
           latch_bone==receiver_bone){
            reload_visual_fail(L"original_reload_bones_unavailable");return;
        }
        reload_visual={};
        reload_visual.world=playground::arena_world;
        reload_visual.pawn=pawn;reload_visual.weapon=weapon;
        reload_visual.gun_mesh=gun.mesh;reload_visual.magazine=magazine.component;
        reload_visual.pawn_mesh=pawn_mesh;reload_visual.left_hand_bone=left_hand_bone;
        reload_visual.latch_bone=latch_bone;
        reload_visual.receiver_bone=receiver_bone;
        reload_visual.start_ms=now;
        if(!log_file){reload_visual_fail(L"log_unavailable");return;}
        fwprintf(log_file,L"lancer_reload_visual_begin pawn=%p weapon=%p gun_mesh=%p "
            L"magazine=%p asset=%ls pawn_mesh=%p left_bone=%d latch_bone=%d receiver_bone=%d\n",reinterpret_cast<void*>(pawn),
            reinterpret_cast<void*>(weapon),reinterpret_cast<void*>(gun.mesh),
            reinterpret_cast<void*>(magazine.component),magazine.asset_name,
            reinterpret_cast<void*>(pawn_mesh),left_hand_bone,latch_bone,receiver_bone);fflush(log_file);
    }
    if(reload_visual.world!=playground::arena_world||reload_visual.pawn!=pawn||
       reload_visual.weapon!=weapon||reload_visual.gun_mesh!=gun.mesh||
       reload_visual.magazine!=magazine.component||
       reload_visual.pawn_mesh!=playground::pointer(pawn,L"Mesh")){
        reload_visual_fail(L"component_identity_changed");return;
    }
    int ammo_used{},spare{};
    float world_time{},reload_start{};
    bool reloading{};
    if(!reload_visual_int(weapon,L"AmmoUsedCount",ammo_used)||
       !reload_visual_int(weapon,L"SpareAmmoCount",spare)||
       !read_control_float(playground::arena_world,L"TimeSeconds",world_time)||
       !read_control_float(weapon,L"ReloadStartTime",reload_start)||
       !reload_visual_is_reloading(weapon,reloading)){
        reload_visual_fail(L"original_state_unreadable");return;
    }
    reload_visual.saw_reloading|=reloading;
    reload_visual.saw_finish|=reload_visual.saw_reloading&&!reloading;
    const auto sample=reload_visual_count.load(std::memory_order_relaxed);
    LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
    fwprintf(log_file,L"lancer_reload_visual sample=%u qpc=%lld ms=%llu "
        L"world=%.7g reload_start=%.7g reloading=%u ammo_used=%d spare=%d "
        L"mag_hidden=%u mag_pawn=%u control=%.7g target=%.7g scale=%.7g\n",
        sample,stamp.QuadPart,now-reload_visual.start_ms,world_time,reload_start,
        unsigned(reloading),ammo_used,spare,unsigned(magazine.hidden),
        unsigned(magazine.attached_to_pawn),ammo_bone.strength,ammo_bone.target,
        ammo_bone.bone_scale);fflush(log_file);
    // Sample the original moving parts and gun component within one PlayerTick.
    // The reload animation still owns both bones; this probe never moves them.
    std::array<float,16> gun_before{},gun_after{},latch{},receiver{};
    if(!support_probe_component_matrix(gun.mesh,gun_before)||
       !support_probe_bone_matrix(gun.mesh,reload_visual.latch_bone,latch)||
       !support_probe_bone_matrix(gun.mesh,reload_visual.receiver_bone,receiver)||
       !support_probe_component_matrix(gun.mesh,gun_after)||
       std::memcmp(gun_before.data(),gun_after.data(),sizeof(gun_before))){
        reload_visual_fail(L"same_tick_reload_bone_matrix_unstable");return;
    }
    fwprintf(log_file,L"lancer_reload_bone_pose sample=%u qpc=%lld",sample,stamp.QuadPart);
    support_probe_print_matrix(L"gun",gun_before.data());
    support_probe_print_matrix(L"latch",latch.data());
    support_probe_print_matrix(L"receiver",receiver.data());
    fwprintf(log_file,L"\n");fflush(log_file);
    if(magazine.attached_to_pawn){
        std::array<float,16> gun_before{},gun_after{},pawn_before{},pawn_after{};
        std::array<float,16> magazine_before{},magazine_after{},left{};
        if(!support_probe_component_matrix(gun.mesh,gun_before)||
           !support_probe_component_matrix(reload_visual.pawn_mesh,pawn_before)||
           !support_probe_component_matrix(magazine.component,magazine_before)||
           !support_probe_bone_matrix(reload_visual.pawn_mesh,
                                      reload_visual.left_hand_bone,left)||
           !support_probe_component_matrix(gun.mesh,gun_after)||
           !support_probe_component_matrix(reload_visual.pawn_mesh,pawn_after)||
           !support_probe_component_matrix(magazine.component,magazine_after)||
           std::memcmp(gun_before.data(),gun_after.data(),sizeof(gun_before))||
           std::memcmp(pawn_before.data(),pawn_after.data(),sizeof(pawn_before))||
           std::memcmp(magazine_before.data(),magazine_after.data(),
                       sizeof(magazine_before))){
            reload_visual_fail(L"held_magazine_matrix_changed");return;
        }
        fwprintf(log_file,L"lancer_reload_magazine_pose sample=%u qpc=%lld",
            sample,stamp.QuadPart);
        support_probe_print_matrix(L"gun",gun_before.data());
        support_probe_print_matrix(L"pawn",pawn_before.data());
        support_probe_print_matrix(L"left",left.data());
        support_probe_print_matrix(L"magazine",magazine_before.data());
        fwprintf(log_file,L"\n");fflush(log_file);
    }
    reload_visual_count.store(sample+1,std::memory_order_release);
    reload_visual.next_ms=now+15;
    if(now-reload_visual.start_ms>=5000||sample+1>=400){
        reload_visual_requested.store(false,std::memory_order_release);
        reload_visual_status.store(reload_visual.saw_reloading&&reload_visual.saw_finish?3:4,
            std::memory_order_release);
    }else reload_visual_status.store(2,std::memory_order_release);
}
