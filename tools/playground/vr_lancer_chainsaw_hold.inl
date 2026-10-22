// Test-only game-thread boundary for the original Lancer's B-held saw state.
// An eventual verified pull gesture may request this same begin operation.
// No victim acquisition is allowed here: the original forward target timer is
// cleared immediately and checked every owned PlayerTick.
std::atomic<bool> lancer_saw_test_start{},lancer_saw_test_stop{};
std::atomic<bool> lancer_saw_active_visible{};
std::atomic<unsigned> lancer_saw_status{};
struct LancerSawHold {
    uptr pc{},pawn{},weapon{},world{};
    Name b{},target_timer{};
    bool active{};
};
LancerSawHold lancer_saw_hold{};

bool lancer_saw_name(const wchar_t* wanted,Name& out){
    const auto names=get<Array>(base+0x1c34800);
    if(!wanted||!names.data||names.count<1000||names.count>names.capacity||
       names.capacity>4000000)return false;
    const auto chars=wcslen(wanted)+1;
    if(chars<2||chars>64)return false;
    for(int index=0;index<names.count;++index){
        const auto entry=get<uptr>(names.data+size_t(index)*8);
        if(!entry||get<unsigned>(entry)!=unsigned(index))continue;
        wchar_t value[64]{};
        if(copy(value,entry+20,chars*sizeof(wchar_t))&&
           !std::memcmp(value,wanted,chars*sizeof(wchar_t))){
            out={unsigned(index),0};return true;
        }
    }
    return false;
}
bool lancer_saw_original_bool(uptr object,const wchar_t* function,bool& value){
    playground::Args call(object,function);
    if(!call.valid||!result_property(call,L"BoolProperty",4)||!call.run())return false;
    value=(call.output<unsigned>()&get<unsigned>(call.prop(L"ReturnValue")+0xd0))!=0;
    return true;
}
bool lancer_saw_timer_active(uptr weapon,Name timer,bool& active){
    playground::Args call(weapon,L"IsTimerActive");
    if(!call.valid||!result_property(call,L"BoolProperty",4)||
       !call.set(L"inTimerFunc",timer).run())return false;
    active=(call.output<unsigned>()&get<unsigned>(call.prop(L"ReturnValue")+0xd0))!=0;
    return true;
}
bool lancer_saw_clear_target_timer(uptr weapon,Name timer){
    bool active{};
    if(!lancer_saw_timer_active(weapon,timer,active))return false;
    if(!active)return true;
    playground::Args clear(weapon,L"ClearTimer");
    return clear.valid&&clear.set(L"inTimerFunc",timer).run()&&
        lancer_saw_timer_active(weapon,timer,active)&&!active;
}
bool lancer_saw_button(uptr pc,Name b,bool press){
    playground::Args call(pc,press?L"ButtonPress":L"ButtonRelease");
    return call.valid&&call.set(L"ButtonName",b).run();
}
bool lancer_saw_same_game(uptr pc,uptr pawn,uptr weapon,uptr world){
    return playground::ready&&test_environment()&&pc==playground::arena_pc&&
        pawn==playground::arena_pawn&&world==playground::arena_world&&
        playground::instance(pc,L"WarPC")&&
        playground::instance(pawn,L"WarPawn")&&
        playground::instance(weapon,L"Weap_AssaultRifle")&&
        playground::pointer(pc,L"Pawn")==pawn&&
        playground::pointer(pc,L"WorldInfo")==world&&
        playground::pointer(pawn,L"Controller")==pc&&
        playground::pointer(pawn,L"Weapon")==weapon&&
        playground::pointer(weapon,L"Instigator")==pawn&&
        !playground::deleted(pawn)&&!playground::deleted(weapon);
}
unsigned lancer_saw_end_hold(){
    auto& hold=lancer_saw_hold;
    if(!hold.active)return 3;
    if(!playground::instance(hold.pc,L"WarPC")||
       !lancer_saw_button(hold.pc,hold.b,false))return 5;
    bool held{};
    if(!read_bool(hold.pc,L"bHoldingButton_B",held)||held)return 5;
    hold={};lancer_saw_active_visible.store(false,std::memory_order_release);
    return 3;
}
unsigned lancer_saw_begin_hold(uptr pc){
    if(lancer_saw_hold.active)return 4;
    const auto pawn=playground::pointer(pc,L"Pawn");
    const auto weapon=playground::pointer(pawn,L"Weapon");
    const auto world=playground::pointer(pc,L"WorldInfo");
    if(!lancer_saw_same_game(pc,pawn,weapon,world))return 4;
    bool can_melee{},held{};
    if(!lancer_saw_original_bool(weapon,L"CanDoSpecialMeleeAttack",can_melee)||
       !can_melee||!read_bool(pc,L"bHoldingButton_B",held)||held)return 4;
    Name b{},timer{};
    if(!lancer_saw_name(L"B",b)||
       !lancer_saw_name(L"CheckForMeleeTarget",timer))return 4;
    if(!lancer_saw_button(pc,b,true))return 5;
    lancer_saw_hold={pc,pawn,weapon,world,b,timer,true};
    lancer_saw_active_visible.store(true,std::memory_order_release);
    if(!lancer_saw_clear_target_timer(weapon,timer)){
        return lancer_saw_end_hold()==3?6:5;
    }
    return 2;
}
void lancer_saw_hold_tick(uptr pc){
    if(lancer_saw_test_stop.exchange(false,std::memory_order_acq_rel)){
        lancer_saw_status.store(lancer_saw_end_hold(),std::memory_order_release);
    }
    if(lancer_saw_hold.active){
        const auto& hold=lancer_saw_hold;
        if(!lancer_saw_same_game(pc,hold.pawn,hold.weapon,hold.world)){
            lancer_saw_status.store(lancer_saw_end_hold()==3?7:5,
                std::memory_order_release);
        }else if(!lancer_saw_clear_target_timer(hold.weapon,hold.target_timer)){
            lancer_saw_status.store(lancer_saw_end_hold()==3?6:5,
                std::memory_order_release);
        }
    }
    if(lancer_saw_test_start.exchange(false,std::memory_order_acq_rel)){
        const auto status=lancer_saw_begin_hold(pc);
        lancer_saw_status.store(status,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"lancer_saw_hold_begin status=%u weapon=%p\n",
            status,reinterpret_cast<void*>(lancer_saw_hold.weapon));fflush(log_file);}
    }
}
