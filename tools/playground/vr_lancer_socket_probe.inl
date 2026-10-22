// Original Lancer magazine socket sampled with the gun component matrix in
// one owned game-thread callback. Read-only; no socket, bone, or mesh changes.
std::atomic<bool> lancer_socket_requested{};
std::atomic<bool> lancer_socket_reload_mode{};
std::atomic<unsigned> lancer_socket_status{},lancer_socket_count{};
struct LancerSocketState {
    uptr world{},pawn{},weapon{},mesh{},asset{},socket{};
    Name socket_name{};
    ULONGLONG next_ms{},start_ms{};
    bool reload_sequence{},saw_reloading{},saw_finish{};
};
LancerSocketState lancer_socket{};

bool lancer_socket_name(Name value,wchar_t (&out)[64]){
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||
       names.capacity>4000000||value.index>=unsigned(names.count))return false;
    const auto entry=get<uptr>(names.data+size_t(value.index)*8);
    if(!entry||get<unsigned>(entry)!=value.index)return false;
    for(unsigned i=0;i<64;++i){
        if(!copy(&out[i],entry+20+i*2,2))return false;
        if(!out[i])return true;
    }
    out[63]=0;return false;
}
template<class T>bool lancer_socket_property(uptr object,const wchar_t* field,
                                              const wchar_t* kind,T& out){
    const auto p=playground::property(object,field);
    const auto cls=get<uptr>(object+0x50);
    if(!p||!cls||!playground::named(get<uptr>(p+0x50),kind)||
       get<unsigned>(p+0x74)!=sizeof(T))return false;
    const auto offset=get<unsigned>(p+0x94),size=get<unsigned>(cls+0x88);
    return offset<=size&&sizeof(T)<=size-offset&&copy(&out,object+offset,sizeof(T));
}
void lancer_socket_fail(const wchar_t* reason){
    lancer_socket_requested.store(false,std::memory_order_release);
    lancer_socket_status.store(4,std::memory_order_release);
    if(log_file){fwprintf(log_file,L"lancer_socket_failed reason=%ls samples=%u\n",
        reason,lancer_socket_count.load());fflush(log_file);}
}
bool lancer_socket_bind(uptr pc,uptr pawn,uptr weapon,MeshPoseProbe const& gun){
    const auto asset=gun.asset;
    if(!playground::instance(asset,L"SkeletalMesh")||
       wcscmp(gun.asset_name,L"COGAssaultRifle"))return false;
    Array sockets{};
    if(!lancer_socket_property(asset,L"Sockets",L"ArrayProperty",sockets)||
       !sockets.data||sockets.count<1||sockets.count>32||
       sockets.capacity<sockets.count||sockets.capacity>32)return false;
    uptr selected{};Name selected_name{},selected_bone{};
    playground::Vector relative{};playground::Rotation rotation{};
    for(int i=0;i<sockets.count;++i){
        const auto socket=get<uptr>(sockets.data+size_t(i)*8);
        if(!playground::instance(socket,L"SkeletalMeshSocket")||
           playground::deleted(socket))return false;
        Name socket_name{};
        if(!lancer_socket_property(socket,L"SocketName",L"NameProperty",socket_name))return false;
        wchar_t text[64]{};
        if(!lancer_socket_name(socket_name,text))return false;
        if(wcscmp(text,L"Magazine"))continue;
        if(selected||!lancer_socket_property(socket,L"BoneName",L"NameProperty",selected_bone)||
           !lancer_socket_property(socket,L"RelativeLocation",L"StructProperty",relative)||
           !lancer_socket_property(socket,L"RelativeRotation",L"StructProperty",rotation))return false;
        selected=socket;selected_name=socket_name;
    }
    if(!selected||!std::isfinite(relative.x)||!std::isfinite(relative.y)||
       !std::isfinite(relative.z))return false;
    wchar_t bone[64]{};
    if(!lancer_socket_name(selected_bone,bone)||!log_file)return false;
    lancer_socket={playground::arena_world,pawn,weapon,gun.mesh,asset,selected,selected_name,
        0,GetTickCount64(),lancer_socket_reload_mode.load(std::memory_order_acquire),false,false};
    fwprintf(log_file,L"%ls pc=%p pawn=%p weapon=%p mesh=%p "
        L"asset=%p socket=%p name=Magazine bone=%ls relative=%.9g,%.9g,%.9g "
        L"rotation=%d,%d,%d count=%d\n",
        lancer_socket.reload_sequence?L"lancer_socket_reload_begin":L"lancer_socket_begin",
        reinterpret_cast<void*>(pc),
        reinterpret_cast<void*>(pawn),reinterpret_cast<void*>(weapon),
        reinterpret_cast<void*>(gun.mesh),reinterpret_cast<void*>(asset),
        reinterpret_cast<void*>(selected),bone,relative.x,relative.y,relative.z,
        rotation.pitch,rotation.yaw,rotation.roll,sockets.count);
    fflush(log_file);return true;
}
bool lancer_socket_world(uptr mesh,Name socket,playground::Vector& location,
                         playground::Rotation& rotation){
    playground::Args call(mesh,L"GetSocketWorldLocationAndRotation");
    const auto input=call.prop(L"InSocketName"),out_location=call.prop(L"OutLocation");
    const auto out_rotation=call.prop(L"OutRotation");
    if(!result_property(call,L"BoolProperty",4)||!input||!out_location||!out_rotation||
       get<unsigned>(input+0x74)!=sizeof(Name)||
       !playground::named(get<uptr>(input+0x50),L"NameProperty")||
       get<unsigned>(out_location+0x74)!=sizeof(location)||
       !playground::named(get<uptr>(out_location+0x50),L"StructProperty")||
       get<unsigned>(out_rotation+0x74)!=sizeof(rotation)||
       !playground::named(get<uptr>(out_rotation+0x50),L"StructProperty")||
       !call.set(L"InSocketName",socket).run())return false;
    const auto result=call.output<unsigned>();
    if(!(result&get<unsigned>(call.prop(L"ReturnValue")+0xd0)))return false;
    location=call.output<playground::Vector>(L"OutLocation");
    rotation=call.output<playground::Rotation>(L"OutRotation");
    return std::isfinite(location.x)&&std::isfinite(location.y)&&
        std::isfinite(location.z);
}
void lancer_socket_post_tick(uptr pc){
    if(!lancer_socket_requested.load(std::memory_order_acquire))return;
    if(!playground::ready||!test_environment()||
       mesh_follow_requested.load(std::memory_order_acquire)||mesh_follow.active||
       support_probe_requested.load(std::memory_order_acquire)||
       reload_visual_requested.load(std::memory_order_acquire)){
        lancer_socket_fail(L"environment_or_other_probe");return;
    }
    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto weapon=playground::pointer(pawn,L"Weapon");
    if(pc!=playground::arena_pc||pawn!=playground::arena_pawn||
       playground::pointer(pc,L"WorldInfo")!=playground::arena_world||
       !playground::instance(weapon,L"Weap_AssaultRifle")||
       playground::deleted(weapon)||playground::pointer(weapon,L"Instigator")!=pawn){
        lancer_socket_fail(L"identity_changed");return;
    }
    MeshPoseProbe gun{};
    if(!mesh_pose_probe(weapon,gun)||gun.hidden){lancer_socket_fail(L"gun_mesh_missing");return;}
    if(!lancer_socket.socket&&!lancer_socket_bind(pc,pawn,weapon,gun)){
        lancer_socket_fail(L"socket_bind_failed");return;
    }
    if(lancer_socket.world!=playground::arena_world||lancer_socket.pawn!=pawn||
       lancer_socket.weapon!=weapon||lancer_socket.mesh!=gun.mesh||
       lancer_socket.asset!=gun.asset){lancer_socket_fail(L"component_changed");return;}
    const auto now=GetTickCount64();
    if(now<lancer_socket.next_ms)return;
    std::array<float,16> before{},after{};
    playground::Vector location{};playground::Rotation rotation{};
    if(!support_probe_component_matrix(gun.mesh,before)||
       !lancer_socket_world(gun.mesh,lancer_socket.socket_name,location,rotation)||
       !support_probe_component_matrix(gun.mesh,after)||
       std::memcmp(before.data(),after.data(),sizeof(before))){
        lancer_socket_fail(L"socket_or_same_tick_matrix_failed");return;
    }
    const auto sample=lancer_socket_count.load(std::memory_order_relaxed);
    LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
    float world_time{},reload_start{};bool reloading{};
    if(lancer_socket.reload_sequence){
        if(!read_control_float(playground::arena_world,L"TimeSeconds",world_time)||
           !read_control_float(weapon,L"ReloadStartTime",reload_start)||
           !reload_visual_is_reloading(weapon,reloading)){
            lancer_socket_fail(L"reload_state_unreadable");return;
        }
        lancer_socket.saw_reloading|=reloading;
        lancer_socket.saw_finish|=lancer_socket.saw_reloading&&!reloading;
    }
    fwprintf(log_file,L"%ls sample=%u qpc=%lld",
        lancer_socket.reload_sequence?L"lancer_socket_reload_sample":L"lancer_socket_sample",
        sample,stamp.QuadPart);
    support_probe_print_matrix(L"gun",before.data());
    fwprintf(log_file,L" world=%.9g,%.9g,%.9g rotation=%d,%d,%d",
        location.x,location.y,location.z,rotation.pitch,rotation.yaw,rotation.roll);
    if(lancer_socket.reload_sequence)
        fwprintf(log_file,L" time=%.9g start=%.9g reloading=%u",
            world_time,reload_start,unsigned(reloading));
    fwprintf(log_file,L"\n");
    fflush(log_file);
    lancer_socket_count.store(sample+1,std::memory_order_release);
    lancer_socket.next_ms=now+(lancer_socket.reload_sequence?15:50);
    if(lancer_socket.reload_sequence){
        if(lancer_socket.saw_finish&&sample+1>=3){
            lancer_socket_requested.store(false,std::memory_order_release);
            lancer_socket_status.store(3,std::memory_order_release);
        }else if(now-lancer_socket.start_ms>=5000||sample+1>=400)
            lancer_socket_fail(L"reload_not_completed");
        else lancer_socket_status.store(2,std::memory_order_release);
    }else if(sample+1==16){
        lancer_socket_requested.store(false,std::memory_order_release);
        lancer_socket_status.store(3,std::memory_order_release);
    }else lancer_socket_status.store(2,std::memory_order_release);
}
