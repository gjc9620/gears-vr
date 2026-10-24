// Private, bounded XR -> game-thread hand-target and original-hand probe.
// No skeletal/animation/controller state is written by this candidate.
namespace freehands {
gears_weapon::EquipmentIntentBridge bridge;
gears_weapon::FreeHandControl control;
gears_weapon::HandWorldFrame last_pose{};
std::uint64_t pose_cursor=UINT64_MAX;
bool bridge_open{},have_pose{};
std::atomic<bool> requested{},stop_requested{},active_visible{};
std::atomic<unsigned> status{},sample_count{};
struct Probe {
    uptr world{},pc{},pawn{},mesh{};
    std::array<int,2> bones{{-1,-1}};
    bool active{},bones_resolved{};
    ULONGLONG next_sample{},deadline{},next_open{};
};
Probe probe{};

bool test_environment(){
    constexpr const wchar_t* wanted[]={L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY",
        L"GEARSVR_TEST_WINDOWS_BEHIND",L"GEARSVR_TEST_MUTE_GAME"};
    for(const auto field:wanted){
        wchar_t value[4]{};
        if(GetEnvironmentVariableW(field,value,4)!=1||value[0]!=L'1')return false;
    }
    return true;
}
bool read_field(uptr object,const wchar_t* field,const wchar_t* kind,
                void* output,unsigned bytes){
    const auto property=playground::property(object,field);
    if(!property||get<unsigned>(property+0x74)!=bytes||
       !playground::named(get<uptr>(property+0x50),kind))return false;
    const auto offset=get<unsigned>(property+0x94);
    const auto size=get<unsigned>(get<uptr>(object+0x50)+0x88);
    return offset<=size&&bytes<=size-offset&&copy(output,object+offset,bytes);
}
bool read_bool(uptr object,const wchar_t* field,bool& out){
    unsigned bits{};
    if(!read_field(object,field,L"BoolProperty",&bits,sizeof(bits)))return false;
    const auto property=playground::property(object,field);
    const auto mask=get<unsigned>(property+0xd0);
    if(!mask||(mask&(mask-1)))return false;
    out=(bits&mask)!=0;return true;
}
bool name_value(const wchar_t* wanted,Name& out){
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||
       names.capacity>4000000)return false;
    const auto chars=wcslen(wanted)+1;
    if(chars<2||chars>64)return false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_cost::Scan cost_scan(names.count);
#endif
    for(int i=0;i<names.count;++i){
        const auto entry=get<uptr>(names.data+size_t(i)*8);
        if(!entry||get<unsigned>(entry)!=unsigned(i))continue;
        wchar_t value[64]{};
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        if(copy(value,entry+20,chars*sizeof(wchar_t))&&
           !std::memcmp(value,wanted,chars*sizeof(wchar_t))){cost_scan.hit(i);out={unsigned(i),0};return true;}
#else
        if(copy(value,entry+20,chars*sizeof(wchar_t))&&
           !std::memcmp(value,wanted,chars*sizeof(wchar_t))){out={unsigned(i),0};return true;}
#endif
    }
    return false;
}
bool argument(playground::Args& call,const wchar_t* field,const wchar_t* kind,unsigned bytes){
    const auto property=call.prop(field);
    if(!call.valid||!property||get<unsigned>(property+0x74)!=bytes||
       !playground::named(get<uptr>(property+0x50),kind))return false;
    const auto offset=get<unsigned>(property+0x94);
    return offset<=call.r.size&&bytes<=call.r.size-offset;
}
bool bone_index(uptr mesh,const wchar_t* wanted,int& out){
    Name bone{};
    if(!name_value(wanted,bone))return false;
    // MatchRefBone/GetBoneMatrix are the already used read-only support-probe
    // boundary. Check the current instance's reflected signature again.
    playground::Args call(mesh,L"MatchRefBone");
    if(!argument(call,L"BoneName",L"NameProperty",sizeof(Name))||
       !argument(call,L"ReturnValue",L"IntProperty",sizeof(int))||
       !call.set(L"BoneName",bone).run())return false;
    out=call.output<int>();return out>=0&&out<256;
}
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
// The caller supplies a freshly validated live control-list BoneName. This
// does not replace the standalone probe's global first-match name scan.
bool bone_index_value(uptr mesh,Name bone,int& out){
    // MatchRefBone/GetBoneMatrix are the already used read-only support-probe
    // boundary. Check the current instance's reflected signature again.
    playground::Args call(mesh,L"MatchRefBone");
    if(!argument(call,L"BoneName",L"NameProperty",sizeof(Name))||
       !argument(call,L"ReturnValue",L"IntProperty",sizeof(int))||
       !call.set(L"BoneName",bone).run())return false;
    out=call.output<int>();return out>=0&&out<256;
}
#endif
bool bone_matrix(uptr mesh,int index,std::array<float,16>& out){
    playground::Args call(mesh,L"GetBoneMatrix");
    if(index<0||!argument(call,L"BoneIndex",L"IntProperty",sizeof(int))||
       !argument(call,L"ReturnValue",L"StructProperty",sizeof(out))||
       !call.set(L"BoneIndex",index).run())return false;
    out=call.output<std::array<float,16>>();
    for(const auto value:out)if(!std::isfinite(value))return false;
    return true;
}
void print_matrix(const wchar_t* field,std::array<float,16> const& matrix){
    fwprintf(log_file,L" %ls=",field);
    for(unsigned i=0;i<16;++i)fwprintf(log_file,L"%ls%.9g",i?L",":L"",matrix[i]);
}
void close_bridge(){
    bridge.close();bridge_open=have_pose=false;last_pose={};pose_cursor=UINT64_MAX;
}
void reset(){
    requested=false;stop_requested=false;active_visible=false;
    close_bridge();control.reset();probe={};status=0;sample_count=0;
}
void finish(unsigned code,const wchar_t* reason){
    probe.active=false;active_visible.store(false,std::memory_order_release);
    status.store(code,std::memory_order_release);
    close_bridge();control.reset();
    if(log_file){fwprintf(log_file,L"free_hand_probe_end status=%u reason=%ls samples=%u writes=0\n",
        code,reason,sample_count.load());fflush(log_file);}
}
bool game_state(uptr pc,gears_weapon::FreeHandGameState& game,unsigned& known,
                std::uint8_t& special_move){
    game={};known=0;special_move=0;
    if(!playground::ready||!playground::automatic||!test_environment()||
       pc!=playground::arena_pc||!playground::instance(pc,L"WarPC")||playground::deleted(pc))return false;
    const auto pawn=playground::pointer(pc,L"Pawn"),world=playground::pointer(pc,L"WorldInfo");
    const auto mesh=playground::pointer(pawn,L"Mesh");
    if(pawn!=playground::arena_pawn||world!=playground::arena_world||
       !playground::instance(pawn,L"WarPawn")||playground::deleted(pawn)||
       playground::pointer(pawn,L"Controller")!=pc||
       !playground::instance(mesh,L"SkeletalMeshComponent")||playground::deleted(mesh))return false;
    const auto weapon=playground::pointer(pawn,L"Weapon");
    if(weapon&&(!playground::instance(weapon,L"WarWeapon")||playground::deleted(weapon)||
                playground::pointer(weapon,L"Instigator")!=pawn))return false;
    int health{};
    if(!read_field(pawn,L"Health",L"IntProperty",&health,sizeof(health)))return false;
    game.world=world;game.controller=pc;game.pawn=pawn;game.mesh=mesh;game.weapon=weapon;
    game.observed_ms=GetTickCount64();game.alive=health>0;game.player_controlled=true;
    bool switching{},melee{},special{};
    if(read_bool(pawn,L"bSwitchingWeapons",switching))known|=1;
    if(read_bool(pawn,L"bDoingMeleeAttack",melee))known|=2;
    if(read_bool(pawn,L"bDoingSpecialAttackMove",special))known|=4;
    if(read_field(pawn,L"SpecialMove",L"ByteProperty",&special_move,sizeof(special_move)))known|=8;
    if(switching)game.fixed_animation_mask|=gears_weapon::hand_weapon_switch;
    if(melee)game.fixed_animation_mask|=gears_weapon::hand_melee;
    if(special||special_move)game.fixed_animation_mask|=gears_weapon::hand_other_fixed_action;
    // These observations are conservative reasons to yield, not a complete
    // pickup/cancel/end contract. No combination of them certifies a writer.
    game.ownership_proved=false;
    return true;
}
void post_tick(uptr pc){
    if(stop_requested.exchange(false,std::memory_order_acq_rel)){
        requested.store(false,std::memory_order_release);
        if(probe.active)finish(4,L"explicit_stop");
    }
    if(requested.exchange(false,std::memory_order_acq_rel)){
        close_bridge();control.reset();probe={};sample_count=0;
        probe.active=true;probe.deadline=GetTickCount64()+10000;
        active_visible.store(true,std::memory_order_release);status=1;
    }
    if(!probe.active)return;
    gears_weapon::FreeHandGameState game{};
    unsigned known{};std::uint8_t special_move{};
    if(!game_state(pc,game,known,special_move)||!log_file){finish(5,L"player_or_environment");return;}
    if(!probe.pawn){
        probe.world=game.world;probe.pc=pc;probe.pawn=game.pawn;probe.mesh=game.mesh;
        probe.bones_resolved=bone_index(game.mesh,L"b_MF_Hand_L",probe.bones[0])&&
            bone_index(game.mesh,L"b_MF_Hand_R",probe.bones[1])&&probe.bones[0]!=probe.bones[1];
        const auto animations=playground::pointer(game.mesh,L"Animations");
        Array indices{},lists{};
        const bool indices_known=read_field(game.mesh,L"SkelControlIndex",L"ArrayProperty",&indices,sizeof(indices));
        const bool lists_known=playground::instance(animations,L"AnimTree")&&
            read_field(animations,L"SkelControlLists",L"ArrayProperty",&lists,sizeof(lists));
        fwprintf(log_file,L"free_hand_probe_begin world=%p pc=%p pawn=%p mesh=%p animations=%p "
            L"bones_known=%u bones=%d,%d skel_indices_known=%u skel_indices=%d "
            L"control_lists_known=%u control_lists=%d ownership_proved=0 apply_mask=0 writes=0\n",
            reinterpret_cast<void*>(game.world),reinterpret_cast<void*>(pc),
            reinterpret_cast<void*>(game.pawn),reinterpret_cast<void*>(game.mesh),
            reinterpret_cast<void*>(animations),unsigned(probe.bones_resolved),probe.bones[0],probe.bones[1],
            unsigned(indices_known),indices_known?indices.count:-1,
            unsigned(lists_known),lists_known?lists.count:-1);fflush(log_file);
    }
    if(probe.world!=game.world||probe.pc!=pc||probe.pawn!=game.pawn||probe.mesh!=game.mesh){
        finish(5,L"player_identity_changed");return;
    }
    const auto now=GetTickCount64();
    if(now>probe.deadline){finish(5,L"sample_timeout");return;}
    if(!bridge_open&&now>=probe.next_open){bridge_open=bridge.open(false);probe.next_open=now+250;}
    gears_weapon::HandWorldFrame fresh{};
    if(bridge_open&&bridge.read_pose(pose_cursor,fresh)){last_pose=fresh;have_pose=true;}
    const auto targets=control.advance(game,have_pose?&last_pose:nullptr,now,true);
    if(now<probe.next_sample)return;
    std::array<float,16> before{},after{},left{},right{};
    const bool matrix_known=read_field(game.mesh,L"LocalToWorld",L"StructProperty",before.data(),sizeof(before));
    const bool bones_known=matrix_known&&probe.bones_resolved&&
        bone_matrix(game.mesh,probe.bones[0],left)&&bone_matrix(game.mesh,probe.bones[1],right)&&
        read_field(game.mesh,L"LocalToWorld",L"StructProperty",after.data(),sizeof(after))&&
        !std::memcmp(before.data(),after.data(),sizeof(before));
    LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
    fwprintf(log_file,L"free_hand_pose sample=%u qpc=%lld ms=%llu generation=%llu "
        L"session=%llu sequence=%llu space_epoch=%llu input_epoch=%llu world_generation=%llu "
        L"pose_ms=%llu units_per_metre=%.9g weapon=%p "
        L"phase=%u tracked_mask=%u eligible_mask=%u apply_mask=%u "
        L"state_known=%u fixed_mask=%u special_move=%u bones_known=%u "
        L"left=%.9g,%.9g,%.9g left_rotation=%.9g,%.9g,%.9g "
        L"right=%.9g,%.9g,%.9g right_rotation=%.9g,%.9g,%.9g",
        sample_count.load(),stamp.QuadPart,now,targets.generation,targets.session,targets.sequence,
        targets.space_epoch,targets.input_epoch,targets.world_generation,targets.pose_ms,
        targets.units_per_metre,reinterpret_cast<void*>(game.weapon),
        unsigned(targets.phase),unsigned(targets.tracked_mask),
        unsigned(targets.eligible_mask),unsigned(targets.apply_mask),known,
        unsigned(game.fixed_animation_mask),unsigned(special_move),unsigned(bones_known),
        targets.world[0].position.x,targets.world[0].position.y,targets.world[0].position.z,
        targets.world[0].rotation.pitch,targets.world[0].rotation.yaw,targets.world[0].rotation.roll,
        targets.world[1].position.x,targets.world[1].position.y,targets.world[1].position.z,
        targets.world[1].rotation.pitch,targets.world[1].rotation.yaw,targets.world[1].rotation.roll);
    if(bones_known){print_matrix(L"mesh",before);print_matrix(L"hand_left",left);print_matrix(L"hand_right",right);}
    fwprintf(log_file,L"\n");fflush(log_file);
    probe.next_sample=now+50;
    const auto samples=sample_count.fetch_add(1,std::memory_order_acq_rel)+1;
    status.store(2,std::memory_order_release);
    if(samples>=160)finish(3,L"sample_limit");
}
} // namespace freehands
