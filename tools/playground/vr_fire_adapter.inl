// Private opt-in bridge from an exact XR input frame to the game's own R2
// semantic. This is deliberately limited to the Hammerburst proved in the
// retail playground. Other weapons require their own release/abort checks.
namespace vrfire {
enum : unsigned {off=0,running=1,stopping=2,stopped=3,fault=4};
std::atomic<unsigned> mode{off};
std::atomic<unsigned> last_status{};
gears_vr::StereoBridge stereo;
gears_vr::ControllerInputBridge hands;
gears_weapon::OriginalR2Fire fire;
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
gears_weapon::OriginalL2Aim aim;
std::atomic<unsigned> last_aim_status{};
#endif
gears_vr::StereoSample last_stereo{};
gears_vr::ControllerInputFrame last_input{};
bool have_last{};
Name r2{};
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
Name l2{};
#endif
#if defined(GEARS_PLAYGROUND_VR_R2_SIM_TEST)
constexpr bool simulated=true;
#else
constexpr bool simulated=false;
#endif

bool find_button(const wchar_t first,const wchar_t second,Name& out){
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||names.capacity>4000000)return false;
    for(int index=0;index<names.count;++index){
        const auto entry=get<uptr>(names.data+size_t(index)*8);
        if(!entry||get<unsigned>(entry)!=unsigned(index))continue;
        wchar_t value[4]{};
        if(copy(value,entry+20,6)&&value[0]==first&&value[1]==second&&!value[2]){
            out={unsigned(index),0};return true;
        }
    }
    return false;
}
bool game_bool(uptr object,const wchar_t* field_name){
    const auto field=playground::property(object,field_name);
    if(!field||!playground::named(get<uptr>(field+0x50),L"BoolProperty"))return false;
    return (get<unsigned>(object+get<unsigned>(field+0x94))&get<unsigned>(field+0xd0))!=0;
}
gears_weapon::OriginalR2Context context(uptr pc){
    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto weapon=playground::pointer(pawn,L"Weapon");
    const auto world=playground::pointer(pc,L"WorldInfo");
    const bool ordinary=playground::instance(pc,L"WarPC")&&
        playground::instance(pawn,L"WarPawn")&&
        playground::pointer(pawn,L"Controller")==pc&&
        playground::instance(weapon,L"WarWeapon")&&
        playground::instance(world,L"WorldInfo")&&
        !playground::deleted(pawn)&&!playground::deleted(weapon);
    const bool proved=ordinary&&playground::instance(weapon,L"Weap_LocustAssaultRifle");
    const bool permitted=proved
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
        &&vrequip::stow_fire_permitted(pc,weapon)
#endif
        ;
    return {world,pc,weapon,ordinary,game_bool(pc,L"bHoldingButton_R2"),
            permitted,proved};
}
bool execute(gears_weapon::OriginalR2Action action,std::uint64_t controller){
    const auto pc=static_cast<uptr>(controller);
    if(!playground::instance(pc,L"WarPC"))return false;
    playground::Args call(pc,action==gears_weapon::OriginalR2Action::press?
        L"ButtonPress":L"ButtonRelease");
    return call.set(L"ButtonName",r2).run();
}
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
gears_weapon::OriginalL2Context aim_context(uptr pc,
                                             gears_weapon::OriginalR2Context const& current){
    return {current.world,current.controller,current.weapon,current.ordinary_player,
        game_bool(pc,L"bPressingTargetingKey"),game_bool(pc,L"bToggleTargeting"),
        current.allow_press};
}
bool execute_aim(gears_weapon::OriginalL2Action action,std::uint64_t controller){
    const auto pc=static_cast<uptr>(controller);
    if(!playground::instance(pc,L"WarPC"))return false;
    playground::Args call(pc,action==gears_weapon::OriginalL2Action::press?
        L"ButtonPress":L"ButtonRelease");
    return call.set(L"ButtonName",l2).run();
}
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
bool yield_for_equipment(uptr pc,uptr weapon){
    const auto state=mode.load(std::memory_order_acquire);
    if(state==fault||state==stopping)return false;
    const auto current=context(pc);
    if(!current.ordinary_player||current.weapon!=weapon)return false;
    if(state==running){
        const auto released=fire.yield(current,execute);
        last_status.store(unsigned(released),std::memory_order_release);
        if(released==gears_weapon::OriginalR2Status::abort_required){mode=fault;return false;}
        const auto aim_released=aim.yield(aim_context(pc,current),execute_aim);
        last_aim_status.store(unsigned(aim_released),std::memory_order_release);
        if(aim_released==gears_weapon::OriginalL2Status::abort_required){mode=fault;return false;}
    }else if(fire.owns_press()||aim.owns_press())return false;
    // A never-owned original press is not ours to release or hide behind a
    // body-slot operation. The original fields must actually be up now.
    bool r2_held{},l2_held{};
    return vrequip::read_bool(pc,L"bHoldingButton_R2",r2_held)&&!r2_held&&
        vrequip::read_bool(pc,L"bPressingTargetingKey",l2_held)&&!l2_held;
}
#endif
void tick(uptr pc){
    const auto state=mode.load(std::memory_order_acquire);
    if(state!=running&&state!=stopping)return;
    auto current=context(pc);
    const auto now=gears_vr::monotonic_ns();
    gears_vr::StereoSample sample{};
    gears_vr::ControllerInputFrame input{};
    const bool matched=state==running&&stereo.read(sample)&&
        hands.read_matching(sample,now,input,simulated);
    if(matched){last_stereo=sample;last_input=input;have_last=true;}
    else if(state==running&&have_last){
        // A lock collision or one late packet is not lost tracking. The frame
        // itself still expires after 100 ms and then releases safe fire.
        sample=last_stereo;input=last_input;
    }
    const auto outcome=fire.advance(input,sample,current,now,execute,simulated);
    last_status.store(unsigned(outcome),std::memory_order_release);
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
    const auto aim_outcome=aim.advance(input,sample,aim_context(pc,current),now,execute_aim,simulated);
    last_aim_status.store(unsigned(aim_outcome),std::memory_order_release);
#endif
    if(outcome==gears_weapon::OriginalR2Status::abort_required){
        playground::message(L"vr_r2_abort_required");
        mode.store(fault,std::memory_order_release);return;
    }
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
    if(aim_outcome==gears_weapon::OriginalL2Status::abort_required){
        playground::message(L"vr_l2_abort_required");
        mode.store(fault,std::memory_order_release);return;
    }
    if(aim_outcome==gears_weapon::OriginalL2Status::pressed)
        playground::message(L"vr_l2_pressed");
    else if(aim_outcome==gears_weapon::OriginalL2Status::released)
        playground::message(L"vr_l2_released");
    else if(aim_outcome==gears_weapon::OriginalL2Status::recovered)
        playground::message(L"vr_l2_recovered");
    else if(aim_outcome==gears_weapon::OriginalL2Status::tapped)
        playground::message(L"vr_l2_tapped");
#endif
    if(outcome==gears_weapon::OriginalR2Status::pressed)
        playground::message(L"vr_r2_pressed");
    else if(outcome==gears_weapon::OriginalR2Status::released)
        playground::message(L"vr_r2_released");
    else if(outcome==gears_weapon::OriginalR2Status::recovered)
        playground::message(L"vr_r2_recovered");
    else if(outcome==gears_weapon::OriginalR2Status::tapped)
        playground::message(L"vr_r2_tapped");
    if(state==stopping){
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
        const bool still_owned=fire.owns_press()||aim.owns_press();
#else
        const bool still_owned=fire.owns_press();
#endif
        if(still_owned){
            playground::message(L"vr_r2_stop_still_owned");
            mode.store(fault,std::memory_order_release);
        }else{
            fire.reset();
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
            aim.reset();
#endif
            mode.store(stopped,std::memory_order_release);
        }
    }
}
DWORD enable(){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       mode.load(std::memory_order_acquire)!=off)return 1;
#if defined(GEARS_PLAYGROUND_VR_R2_SIM_TEST)
    wchar_t behind[4]{},muted[4]{};
    if(GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)!=1||behind[0]!=L'1'||
       GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",muted,4)!=1||muted[0]!=L'1')return 2;
#endif
    if(!find_button(L'R',L'2',r2))return 3;
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
    if(!find_button(L'L',L'2',l2))return 6;
#endif
    const auto pid=GetCurrentProcessId();
    if(!stereo.open(pid,false))return 4;
    if(!hands.open(pid,false)){stereo.close();return 5;}
    fire.reset();
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
    aim.reset();last_aim_status.store(0,std::memory_order_release);
#endif
    have_last=false;mode.store(running,std::memory_order_release);
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
    playground::message(L"vr_r2_l2_enabled_hammerburst_hold_aim_only");
#else
    playground::message(L"vr_r2_enabled_hammerburst_only");
#endif
    return 0;
}
DWORD disable(){
    auto state=mode.load(std::memory_order_acquire);
    if(state==off)return 0;
    if(state==fault)return 4;
    if(state==running){mode.store(stopping,std::memory_order_release);state=stopping;}
    if(state==stopping){
        const auto deadline=GetTickCount64()+2000;
        while(GetTickCount64()<deadline){
            state=mode.load(std::memory_order_acquire);
            if(state!=stopping)break;
            Sleep(1);
        }
        if(state==stopping)return 2;
        if(state==fault)return 4;
    }
    if(state!=stopped)return 3;
    hands.close();stereo.close();have_last=false;
    mode.store(off,std::memory_order_release);
    playground::message(L"vr_r2_disabled");return 0;
}
}
