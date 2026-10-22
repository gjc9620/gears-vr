// Read-only, same-PlayerTick original Lancer blade-bone baseline. Sequential
// remote calls can straddle animation updates and cannot establish a segment.
std::atomic<bool> lancer_blade_probe_requested{};
std::atomic<bool> lancer_blade_probe_expected_hold{};
std::atomic<unsigned> lancer_blade_probe_status{},lancer_blade_probe_count{};
struct LancerBladeProbe {
    uptr world{},pawn{},weapon{},mesh{};
    int root{-1},tip{-1};
    ULONGLONG next_ms{};
};
LancerBladeProbe lancer_blade_probe{};

void lancer_blade_probe_fail(const wchar_t* reason){
    lancer_blade_probe_requested.store(false,std::memory_order_release);
    lancer_blade_probe_status.store(4,std::memory_order_release);
    if(log_file){fwprintf(log_file,L"lancer_blade_probe_failed reason=%ls samples=%u\n",
        reason,lancer_blade_probe_count.load());fflush(log_file);}
}
void lancer_blade_probe_post_tick(uptr pc){
    if(!lancer_blade_probe_requested.load(std::memory_order_acquire))return;
    if(!playground::ready||!test_environment()||!log_file||
       mesh_follow_requested.load(std::memory_order_acquire)||mesh_follow.active||
       support_probe_requested.load(std::memory_order_acquire)||
       reload_visual_requested.load(std::memory_order_acquire)||
       lancer_saw_hold.active!=
           lancer_blade_probe_expected_hold.load(std::memory_order_acquire)){
        lancer_blade_probe_fail(L"environment_or_conflicting_trial");return;
    }
    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto weapon=playground::pointer(pawn,L"Weapon");
    if(pc!=playground::arena_pc||pawn!=playground::arena_pawn||
       playground::pointer(pc,L"WorldInfo")!=playground::arena_world||
       !playground::instance(weapon,L"Weap_AssaultRifle")){
        lancer_blade_probe_fail(L"original_lancer_missing");return;
    }
    bool switching{};
    MeshPoseProbe gun{};
    if(!read_bool(pawn,L"bSwitchingWeapons",switching)||switching||
       !mesh_pose_probe(weapon,gun)||gun.hidden||
       wcscmp(gun.asset_name,L"COGAssaultRifle")){
        lancer_blade_probe_fail(L"original_mesh_unavailable");return;
    }
    bool b_held{},saw_running{},doing_melee{};
    const bool expected=lancer_blade_probe_expected_hold.load(std::memory_order_acquire);
    if(!read_bool(pc,L"bHoldingButton_B",b_held)||
       !read_bool(weapon,L"bChainSawRunning",saw_running)||
       !read_bool(pawn,L"bDoingMeleeAttack",doing_melee)||
       b_held!=expected||saw_running!=expected||doing_melee!=expected){
        lancer_blade_probe_fail(L"original_saw_state_changed");return;
    }
    auto& state=lancer_blade_probe;
    if(!state.weapon){
        Name root{},tip{};
        if(!lancer_saw_name(L"b_AR_Chainsaw",root)||
           !lancer_saw_name(L"b_AR_ChainsawEnd",tip)||
           !support_probe_bone_index(gun.mesh,root,state.root)||
           !support_probe_bone_index(gun.mesh,tip,state.tip)||
           state.root==state.tip){
            lancer_blade_probe_fail(L"blade_bones_missing");return;
        }
        state.world=playground::arena_world;state.pawn=pawn;
        state.weapon=weapon;state.mesh=gun.mesh;
        fwprintf(log_file,L"lancer_blade_probe_begin pawn=%p weapon=%p mesh=%p "
            L"root=%d tip=%d\n",reinterpret_cast<void*>(pawn),
            reinterpret_cast<void*>(weapon),reinterpret_cast<void*>(gun.mesh),
            state.root,state.tip);fflush(log_file);
    }
    if(state.world!=playground::arena_world||state.pawn!=pawn||
       state.weapon!=weapon||state.mesh!=gun.mesh){
        lancer_blade_probe_fail(L"identity_changed");return;
    }
    const auto now=GetTickCount64();
    if(now<state.next_ms)return;
    std::array<float,16> before{},after{},root{},tip{};
    if(!support_probe_component_matrix(gun.mesh,before)||
       !support_probe_bone_matrix(gun.mesh,state.root,root)||
       !support_probe_bone_matrix(gun.mesh,state.tip,tip)||
       !support_probe_component_matrix(gun.mesh,after)||
       std::memcmp(before.data(),after.data(),sizeof(before))){
        lancer_blade_probe_fail(L"same_tick_matrix_unstable");return;
    }
    LARGE_INTEGER qpc{};QueryPerformanceCounter(&qpc);
    const auto index=lancer_blade_probe_count.load(std::memory_order_relaxed);
    fwprintf(log_file,L"lancer_blade_pose sample=%u qpc=%lld",index,qpc.QuadPart);
    support_probe_print_matrix(L"gun",before.data());
    support_probe_print_matrix(L"root",root.data());
    support_probe_print_matrix(L"tip",tip.data());
    fwprintf(log_file,L"\n");fflush(log_file);
    lancer_blade_probe_count.store(index+1,std::memory_order_release);
    state.next_ms=now+50;
    if(index+1==16){
        lancer_blade_probe_requested.store(false,std::memory_order_release);
        lancer_blade_probe_status.store(3,std::memory_order_release);
    }else lancer_blade_probe_status.store(2,std::memory_order_release);
}
