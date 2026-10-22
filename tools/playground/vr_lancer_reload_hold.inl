// Private, Lancer-only boundary trial. R3 starts the original Reloading state,
// then moves its original completion timer to a bounded fallback. The explicit
// completion request is test-only until a calibrated magazine/bolt stroke can
// invoke this same game-thread path. No guessed perfect-reload reward is issued.
std::atomic<bool> lancer_reload_test_start{},lancer_reload_test_complete{};
std::atomic<unsigned> lancer_reload_status{};
struct LancerReloadHold {
    uptr world{},pawn{},weapon{},finish_function{},mesh{};
    Name timer{};
    int ammo_used{};
    bool active{};
    gears_weapon::LancerFeedWorld feed{};
    ULONGLONG next_anchor_log{};
};
LancerReloadHold lancer_reload_hold{};

bool lancer_fixed_feed(uptr weapon,uptr expected_mesh,
                       gears_weapon::LancerFeedWorld& feed,uptr& mesh){
    MeshPoseProbe gun{};
    if(!mesh_pose_probe(weapon,gun)||gun.hidden||
       wcscmp(gun.asset_name,L"COGAssaultRifle")||
       (expected_mesh&&gun.mesh!=expected_mesh))return false;
    std::array<float,16> matrix{};
    std::memcpy(matrix.data(),gun.local_to_world,sizeof(gun.local_to_world));
    if(!gears_weapon::lancer_feed_world(matrix,feed))return false;
    mesh=gun.mesh;return true;
}

bool lancer_int(uptr object,const wchar_t* field,int& value){
    const auto p=playground::property(object,field);
    if(!p||get<unsigned>(p+0x74)!=sizeof(int)||
       !playground::named(get<uptr>(p+0x50),L"IntProperty"))return false;
    value=get<int>(object+get<unsigned>(p+0x94));return true;
}
bool lancer_bool_call(uptr weapon,const wchar_t* function,bool& value){
    playground::Args call(weapon,function);
    if(!result_property(call,L"BoolProperty",4)||!call.run())return false;
    value=(call.output<unsigned>()&get<unsigned>(call.prop(L"ReturnValue")+0xd0))!=0;
    return true;
}
uptr lancer_finish_function(uptr weapon){
    auto cls=get<uptr>(weapon+0x50);
    for(unsigned depth=0;cls&&depth<40;++depth,cls=get<uptr>(cls+0x60)){
        auto state=get<uptr>(cls+0x80);
        for(unsigned n=0;state&&n<4096;++n,state=get<uptr>(state+0x68)){
            if(!registered(state))return 0;
            if(!playground::named(state,L"Reloading")||
               !playground::named(get<uptr>(state+0x50),L"State"))continue;
            auto function=get<uptr>(state+0x80);
            for(unsigned i=0;function&&i<256;++i,function=get<uptr>(function+0x68)){
                if(!registered(function))return 0;
                if(playground::named(function,L"EndOfReloadTimer")&&
                   playground::named(get<uptr>(function+0x50),L"Function")&&
                   get<uptr>(function+0x40)==state&&
                   get<unsigned short>(function+0x106)==0)return function;
            }
        }
    }
    return 0;
}
bool lancer_timer_active(uptr weapon,Name timer,bool& active){
    playground::Args call(weapon,L"IsTimerActive");
    if(!call.valid||!result_property(call,L"BoolProperty",4)||
       !call.set(L"inTimerFunc",timer).run())return false;
    active=(call.output<unsigned>()&get<unsigned>(call.prop(L"ReturnValue")+0xd0))!=0;
    return true;
}
bool lancer_clear_timer(uptr weapon,Name timer){
    playground::Args call(weapon,L"ClearTimer");
    bool active{};
    return call.valid&&call.set(L"inTimerFunc",timer).run()&&
        lancer_timer_active(weapon,timer,active)&&!active;
}
bool lancer_set_fallback(uptr weapon,Name timer){
    playground::Args call(weapon,L"SetTimer");
    bool active{};
    return call.valid&&call.set(L"InRate",8.f)
        .flag(L"inbLoop",false).set(L"inTimerFunc",timer).run()&&
        lancer_timer_active(weapon,timer,active)&&active;
}
bool lancer_finish_now(uptr weapon,uptr function){
    Request request{};request.object=weapon;request.function=function;request.size=0;
    return invoke(&request);
}
bool lancer_same_game(uptr pc,gears_weapon::EquipmentSnapshot const& game){
    return playground::ready&&test_environment()&&pc==playground::arena_pc&&
        game.world==playground::arena_world&&game.pawn==playground::arena_pawn&&
        game.current_weapon&&playground::pointer(game.pawn,L"Controller")==pc&&
        playground::pointer(game.pawn,L"Weapon")==game.current_weapon&&
        playground::pointer(game.current_weapon,L"Instigator")==game.pawn;
}
unsigned lancer_begin(uptr pc,gears_weapon::EquipmentSnapshot const& game){
    if(lancer_reload_hold.active||!lancer_same_game(pc,game)||game.animation_busy||
       !playground::instance(game.current_weapon,L"Weap_AssaultRifle"))return 2;
    const auto weapon=static_cast<uptr>(game.current_weapon);
    int used{};bool spare{},can_reload{},already{};
    if(!lancer_int(weapon,L"AmmoUsedCount",used)||used<=0||used>10000||
       !read_ammo_supply(weapon,spare))return 3;
    if(!spare)return play_empty_slot_sound(game,4)?10:11;
    if(!lancer_bool_call(weapon,L"CanReload",can_reload)||!can_reload||
       !lancer_bool_call(weapon,L"IsReloading",already)||already)return 3;
    const auto finish=lancer_finish_function(weapon);
    if(!finish)return 4;
    gears_weapon::LancerFeedWorld feed{};uptr mesh{};
    if(!lancer_fixed_feed(weapon,0,feed,mesh))return 4;
    const auto timer=get<Name>(finish+0x48);
    playground::Args start(weapon,L"ForceReload");
    if(!start.valid||!start.run()||
       !lancer_bool_call(weapon,L"IsReloading",already)||!already)return 5;
    bool timer_active{};
    if(!lancer_timer_active(weapon,timer,timer_active)||!timer_active||
       !lancer_clear_timer(weapon,timer)||!lancer_set_fallback(weapon,timer)){
        // A failed timer transfer must never strand the original weapon.
        if(!lancer_finish_now(weapon,finish))return 6;
        return 7;
    }
    lancer_reload_hold={game.world,game.pawn,weapon,finish,mesh,timer,used,true,
        feed,GetTickCount64()};
    return 1;
}
unsigned lancer_complete(uptr pc,gears_weapon::EquipmentSnapshot const& game){
    auto& hold=lancer_reload_hold;
    if(!hold.active||!lancer_same_game(pc,game)||game.world!=hold.world||
       game.pawn!=hold.pawn||game.current_weapon!=hold.weapon)return 2;
    bool reloading{},timer_active{};
    if(!lancer_bool_call(hold.weapon,L"IsReloading",reloading)||!reloading||
       !lancer_timer_active(hold.weapon,hold.timer,timer_active)||!timer_active||
       !lancer_clear_timer(hold.weapon,hold.timer))return 3;
    if(!lancer_finish_now(hold.weapon,hold.finish_function)){
        // Re-arm the original completion path if explicit completion failed.
        return lancer_set_fallback(hold.weapon,hold.timer)?4:7;
    }
    int used{};
    if(!lancer_bool_call(hold.weapon,L"IsReloading",reloading)||reloading||
       !lancer_int(hold.weapon,L"AmmoUsedCount",used)||used>=hold.ammo_used)return 5;
    hold={};return 6;
}
void lancer_refresh(uptr pc,gears_weapon::EquipmentSnapshot const& game){
    auto& hold=lancer_reload_hold;
    if(!hold.active)return;
    if(!lancer_same_game(pc,game)||game.world!=hold.world||
       game.pawn!=hold.pawn||game.current_weapon!=hold.weapon){
        // The eight-second original timer remains armed for this old weapon.
        hold={};lancer_reload_status.store(8,std::memory_order_release);return;
    }
    uptr mesh{};
    gears_weapon::LancerFeedWorld feed{};
    if(!lancer_fixed_feed(hold.weapon,hold.mesh,feed,mesh)){
        // Keep the original fallback timer; reject further VR interaction.
        hold={};lancer_reload_status.store(8,std::memory_order_release);return;
    }
    hold.feed=feed;
    const auto now=GetTickCount64();
    if(log_file&&now>=hold.next_anchor_log){
        hold.next_anchor_log=now+500;
        fwprintf(log_file,L"lancer_fixed_feed_anchor weapon=%p mesh=%p "
            L"point=%.6f,%.6f,%.6f outward=%.6f,%.6f,%.6f\n",
            reinterpret_cast<void*>(hold.weapon),reinterpret_cast<void*>(mesh),
            feed.point_raw.x,feed.point_raw.y,feed.point_raw.z,
            feed.outward_axis.x,feed.outward_axis.y,feed.outward_axis.z);
        fflush(log_file);
    }
    bool reloading{};
    if(lancer_bool_call(hold.weapon,L"IsReloading",reloading)&&!reloading){
        hold={};lancer_reload_status.store(9,std::memory_order_release);
    }
}
