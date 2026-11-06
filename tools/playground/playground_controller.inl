// Included in the bridge namespace. All engine state below is touched only by
// the game thread. The enable flag is set once by the verified launcher.
namespace playground {
std::atomic<bool> automatic{};
std::atomic<bool> test_enter_pending{};
uptr viewport{},menu_button{},player{},arena_world{},arena_pc{},arena_pawn{},turret{};
uptr scene_client{},menu_scene{};
uptr range_vehicle{},vehicle_driver{};
uptr vehicle_loader{};
uptr previous_vehicle_pawn{};
Name vehicle_level{};
std::atomic<bool> stop_requested{},stop_acknowledged{};
const char* vehicle_phase="idle";
bool vehicle_loading{},vehicle_use_held{},vehicle_exit_held{},vehicle_shadow{};
ULONGLONG vehicle_deadline{};
unsigned vehicle_entries{},vehicle_exits{};
unsigned player_tick_index{};
bool requested{},ready{},menu_layout_valid{};unsigned failures{},pickup_respawns{},target_respawns{};
ULONGLONG next_tick{},requested_at{};
const wchar_t* phase=L"menu";
struct Vector {float x,y,z;};
struct Rotation {int pitch,yaw,roll;};
struct MenuBounds {float left,top,right,bottom;};
MenuBounds menu_bounds{};
struct String {const wchar_t* data;int count,capacity;};
const wchar_t* weapon_names[]={L"Weap_AssaultRifle",L"Weap_LocustAssaultRifle",L"Weap_Shotgun",
    L"Weap_SniperRifle",L"Weap_COGPistol",L"Weap_LocustPistol",L"Weap_Boomer",L"Weap_Bow",
    L"WEAP_HammerOfDawn",L"Weap_FragGrenade",L"Weap_SmokeGrenade"};
struct RangeIdentity {uptr address{},klass{},world{},outer{};unsigned index{};Name name{};};
struct Slot {uptr actor{},inventory{},cls{};Vector location{};ULONGLONG after{};bool melee{};RangeIdentity controller{},identity{};};
std::array<Slot,11> guns{};std::array<Slot,6> targets{};

#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
bool named(uptr object,const wchar_t* value,std::size_t limit){
    wchar_t text[128]{};return value&&limit&&limit<=128&&name(object,text,limit)&&!_wcsicmp(text,value);
}
#endif
bool named(uptr object,const wchar_t* value){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(component_names::enabled){
        if(!object)return false;
        return named(object,value,component_names::query_limit(value));
    }
#endif
wchar_t text[128]{};return name(object,text,128)&&!_wcsicmp(text,value);}
bool derives(uptr cls,const wchar_t* value){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(component_names::enabled){
        if(!cls)return false;
        const auto limit=component_names::query_limit(value);
        for(unsigned n=0;cls&&n<40;++n,cls=get<uptr>(cls+0x60))if(named(cls,value,limit))return true;
        return false;
    }
#endif
    for(unsigned n=0;cls&&n<40;++n,cls=get<uptr>(cls+0x60))if(named(cls,value))return true;
    return false;
}
bool instance(uptr object,const wchar_t* cls){
    if(!registered(object)||!derives(get<uptr>(object+0x50),cls))return false;
    wchar_t text[128]{};return name(object,text,128)&&wcsncmp(text,L"Default__",9);
}
uptr find_class(const wchar_t* value){
    const auto a=get<Array>(base+0x1c24730);
    for(int i=0;i<a.count;++i){const auto p=get<uptr>(a.data+size_t(i)*8);
        if(p&&named(p,value)&&named(get<uptr>(p+0x50),L"Class"))return p;}
    return 0;
}
uptr find_instance(const wchar_t* value){
    const auto a=get<Array>(base+0x1c24730);
    for(int i=0;i<a.count;++i){const auto p=get<uptr>(a.data+size_t(i)*8);if(instance(p,value))return p;}
    return 0;
}
uptr field(uptr cls,const wchar_t* value,const wchar_t* kind=nullptr){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(component_names::enabled){
        if(!cls)return 0;
        const auto value_limit=component_names::query_limit(value),kind_limit=kind?component_names::query_limit(kind):0;
        for(unsigned depth=0;cls&&depth<40;++depth,cls=get<uptr>(cls+0x60)){
            auto f=get<uptr>(cls+0x80);
            for(unsigned n=0;f&&n<4096;++n,f=get<uptr>(f+0x68))
                if(named(f,value,value_limit)&&(!kind||named(get<uptr>(f+0x50),kind,kind_limit)))return f;
        }
        return 0;
    }
#endif
    for(unsigned depth=0;cls&&depth<40;++depth,cls=get<uptr>(cls+0x60)){
        auto f=get<uptr>(cls+0x80);
        for(unsigned n=0;f&&n<4096;++n,f=get<uptr>(f+0x68))
            if(named(f,value)&&(!kind||named(get<uptr>(f+0x50),kind)))return f;
    }
    return 0;
}
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
// Addresses only, local to one non-invoking observation. Match the original
// untyped field's first name in derived-to-base order, even for a bad type.
// Descriptor validation and a fresh final resolution remain the caller's job.
template<std::size_t N>
bool fields(uptr cls,const wchar_t* const (&labels)[N],std::array<uptr,N>& out){
    out={};std::array<uptr,N> found{};std::size_t remaining=N,limit=1;
    for(auto label:labels){
        if(!label)return false;
        limit=(std::max)(limit,component_names::query_limit(label));
    }
    if(!component_names::enabled)limit=128;
    for(unsigned depth=0;cls&&depth<40;++depth,cls=get<uptr>(cls+0x60)){
        auto f=get<uptr>(cls+0x80);
        for(unsigned n=0;f&&n<4096;++n,f=get<uptr>(f+0x68)){
            wchar_t text[128]{};
            if(!name(f,text,limit))continue;
            for(std::size_t i=0;i<N;++i){
                if(found[i]||_wcsicmp(text,labels[i]))continue;
                found[i]=f;--remaining;
            }
            if(!remaining){out=found;return true;}
        }
    }
    return false;
}
#endif
uptr property(uptr object,const wchar_t* value){return registered(object)?field(get<uptr>(object+0x50),value):0;}
uptr pointer(uptr object,const wchar_t* value){const auto p=property(object,value);return p?get<uptr>(object+get<unsigned>(p+0x94)):0;}
template<class T>T scalar(uptr object,const wchar_t* value){const auto p=property(object,value);return p?get<T>(object+get<unsigned>(p+0x94)):T{};}
bool write_field(uptr object,const wchar_t* value,const void* data,unsigned bytes){
    const auto p=property(object,value);if(!p||get<unsigned>(p+0x74)!=bytes)return false;
    const auto offset=get<unsigned>(p+0x94),size=get<unsigned>(get<uptr>(object+0x50)+0x88);
    if(offset>size||bytes>size-offset)return false;
    memcpy(reinterpret_cast<void*>(object+offset),data,bytes);return true;
}
template<class T>bool put(uptr object,const wchar_t* value,T const& v){return write_field(object,value,&v,sizeof(v));}
bool boolean(uptr object,const wchar_t* value,bool v){
    const auto p=property(object,value);if(!p||!named(get<uptr>(p+0x50),L"BoolProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),mask=get<unsigned>(p+0xd0);
    auto bits=get<unsigned>(object+offset);bits=v?bits|mask:bits&~mask;
    return write_field(object,value,&bits,4);
}
bool deleted(uptr object){
    if(!registered(object))return true;
    const auto p=property(object,L"bDeleteMe");
    return p&&(get<unsigned>(object+get<unsigned>(p+0x94))&get<unsigned>(p+0xd0));
}
struct Args {
    Request r{};bool valid{};
    Args(uptr object,const wchar_t* fn){
        r.object=object;r.function=registered(object)?field(get<uptr>(object+0x50),fn,L"Function"):0;
        r.size=r.function?get<unsigned short>(r.function+0x106):0;
        valid=r.function&&r.size<=4096;
    }
    uptr prop(const wchar_t* n){return r.function?field(r.function,n):0;}
    bool bytes(const wchar_t* n,const void* v,unsigned size){
        const auto p=prop(n);if(!p||get<unsigned>(p+0x74)!=size){valid=false;return false;}
        const auto offset=get<unsigned>(p+0x94);if(offset+size>r.size){valid=false;return false;}
        memcpy(r.params+offset,v,size);return true;
    }
    template<class T>Args& set(const wchar_t* n,T const& v){bytes(n,&v,sizeof(v));return *this;}
    Args& flag(const wchar_t* n,bool v){const auto p=prop(n);unsigned bits=v&&p?get<unsigned>(p+0xd0):0;return set(n,bits);}
    Args& text(const wchar_t* n,const wchar_t* v){const int size=int(wcslen(v))+1;return set(n,String{v,size,size});}
    template<class T>T output(const wchar_t* n=L"ReturnValue"){
        const auto p=prop(n);T v{};if(p){const auto offset=get<unsigned>(p+0x94);if(offset+sizeof(T)<=r.size)memcpy(&v,r.params+offset,sizeof(v));}return v;
    }
    bool run(){return valid&&invoke(&r);}
};
void message(const wchar_t* text){if(log_file){fwprintf(log_file,L"playground %ls\n",text);fflush(log_file);}}
void status(){
    wchar_t path[32768]{};GetModuleFileNameW(module,path,32768);auto slash=wcsrchr(path,L'\\');if(!slash)return;
    swprintf_s(slash+1,32768-(slash+1-path),L"playground-status-%lu.json",GetCurrentProcessId());
    unsigned live_guns=0,live_targets=0;
    if(ready){for(auto const& s:guns)if(!deleted(s.actor))++live_guns;for(auto const& s:targets)if(!deleted(s.actor)&&scalar<int>(s.actor,L"Health")>0)++live_targets;}
    auto file=_wfsopen(path,L"wb",_SH_DENYNO);if(!file)return;
    fprintf(file,"{\"schema\":1,\"pid\":%lu,\"phase\":\"%ls\",\"ready\":%s,\"menu_button\":\"0x%llx\",\"menu_layout_valid\":%s,\"menu_bounds\":[%.3f,%.3f,%.3f,%.3f],\"world\":\"0x%llx\",\"pawn\":\"0x%llx\",\"pickups\":%u,\"targets\":%u,\"turret\":\"0x%llx\",\"failures\":%u,\"pickup_respawns\":%u,\"target_respawns\":%u,\"vehicle\":\"0x%llx\",\"vehicle_phase\":\"%s\",\"vehicle_entries\":%u,\"vehicle_exits\":%u}\n",
        GetCurrentProcessId(),phase,ready?"true":"false",menu_button,menu_layout_valid?"true":"false",menu_bounds.left,menu_bounds.top,menu_bounds.right,menu_bounds.bottom,arena_world,arena_pawn,live_guns,live_targets,turret,failures,pickup_respawns,target_respawns,range_vehicle,vehicle_phase,vehicle_entries,vehicle_exits);
    fclose(file);
}
bool in_map(uptr object){
    for(unsigned n=0;object&&n<8;++n,object=get<uptr>(object+0x40))if(named(object,L"MP_WarMachine"))return true;
    return false;
}
uptr spawn(uptr cls,Vector position,Rotation rotation={}){
    Args a(arena_pc,L"Spawn");a.set(L"SpawnClass",cls).set(L"SpawnLocation",position).set(L"SpawnRotation",rotation).flag(L"bNoCollisionFail",true);
    return a.run()?a.output<uptr>():0;
}
bool floor_at(float x,float y,float start_z,Vector& position){
    Args a(arena_pc,L"Trace");a.set(L"TraceStart",Vector{x,y,start_z}).set(L"TraceEnd",Vector{x,y,-3000}).flag(L"bTraceActors",false);
    if(!a.run())return false;
    const auto hit=a.output<uptr>();const auto normal=a.output<Vector>(L"HitNormal");position=a.output<Vector>(L"HitLocation");
    return hit&&std::isfinite(position.z)&&normal.z>.65f&&position.z>-2000&&position.z<start_z-10;
}
bool infinite(uptr weapon){
    if(!instance(weapon,L"WarWeapon")||deleted(weapon))return false;
    // Private reload verification must exercise the production finite-ammo
    // path without the range's periodic refill masking its accounting.
    const auto range_test_flag=[](const wchar_t* key){wchar_t value[4]{};
        return GetEnvironmentVariableW(key,value,4)==1&&value[0]==L'1';};
    if(range_test_flag(L"GEARSVR_TEST_FINITE_RANGE_AMMO")&&
       range_test_flag(L"GEARSVR_TEST_WINDOWS_BEHIND")&&range_test_flag(L"GEARSVR_TEST_MUTE_GAME")&&
       range_test_flag(L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY"))
        return boolean(weapon,L"bInfiniteSpareAmmo",false);
    // Set only owned range objects; normal magazine consumption/reload survives.
    if(!boolean(weapon,L"bInfiniteSpareAmmo",true))return false;
    const auto spare=scalar<int>(weapon,L"SpareAmmoCount");
    const auto maximum=scalar<float>(weapon,L"MaxSpareAmmo");
    if(spare<1&&std::isfinite(maximum)&&maximum>=1&&maximum<10000)
        put(weapon,L"SpareAmmoCount",int(maximum));
    return true;
}
#include "playground_vehicle.inl"
bool create_gun(Slot& slot){
    if(!slot.cls)return false;
    auto weapon=spawn(slot.cls,slot.location);if(!weapon)return false;
    const auto mesh=pointer(weapon,L"DroppedPickupMesh");
    auto pickup=mesh?spawn(find_class(L"WarDroppedPickup"),slot.location):0;
    if(!pickup){Args cleanup(weapon,L"Destroy");cleanup.run();return false;}
    const bool bound=put(pickup,L"InventoryClass",slot.cls)&&put(pickup,L"Inventory",weapon)&&put(pickup,L"LifeSpan",0.f)&&infinite(weapon);
    Args set_mesh(pickup,L"SetPickupMesh");set_mesh.set(L"PickupMesh",mesh);
    if(!bound||!set_mesh.run()){Args cleanup(pickup,L"Destroy");cleanup.run();Args cleanup_weapon(weapon,L"Destroy");cleanup_weapon.run();return false;}
    slot.actor=pickup;slot.inventory=weapon;slot.after=0;return true;
}
bool current_range_object(RangeIdentity const& id){
    if(!id.address||!registered(id.address)||get<unsigned>(id.address+8)!=id.index||
       get<uptr>(id.address+0x50)!=id.klass||get<uptr>(id.address+0x40)!=id.outer||
       pointer(id.address,L"WorldInfo")!=id.world)return false;
    const auto n=get<Name>(id.address+0x48);
    return n.index==id.name.index&&n.number==id.name.number;
}
bool current_range_controller(RangeIdentity const& id){
    return current_range_object(id)&&instance(id.address,L"WarAI_Drone");
}
void retire_target(Slot& slot){
    const auto actor=slot.actor;const auto controller=slot.controller;
    const bool actor_owned=current_range_object(slot.identity);
    if(actor_owned&&!deleted(actor)){Args destroy(actor,L"Destroy");destroy.run();}
    // Only this range-created, identity-pinned controller may be retired.
    // Never destroy a controller that has since possessed somebody else.
    if(current_range_controller(controller)&&!deleted(controller.address)){
        const auto pawn=pointer(controller.address,L"Pawn");
        if(!pawn||(actor_owned&&pawn==actor)){Args destroy(controller.address,L"Destroy");destroy.run();}
    }
    slot.actor=0;slot.controller={};slot.identity={};
}
bool prepare_melee_target(Slot& slot,uptr actor){
    // The retail special-melee validator requires a real controller. Keep
    // that gameplay rule, rather than accepting the unpossessed shooting
    // dummies as fake victims. Spawn and possess through the original Pawn.
    Args possess(actor,L"SpawnDefaultController");
    if(!possess.run()){message(L"melee_target_spawn_controller_failed");return false;}
    const auto controller=pointer(actor,L"Controller");
    if(!current_range_object(slot.identity)||!instance(controller,L"WarAI_Drone")||pointer(controller,L"Pawn")!=actor||
       pointer(controller,L"WorldInfo")!=arena_world){message(L"melee_target_controller_links_failed");return false;}
    slot.controller={controller,get<uptr>(controller+0x50),arena_world,get<uptr>(controller+0x40),get<unsigned>(controller+8),get<Name>(controller+0x48)};
    // Match the original OnAIToggleCombat disabled policy on this owned
    // practice AI. DebugState alone was insufficient:41864 left its start
    // and fired at the player. Do not stop Tick/animation or modify team,
    // health, collision, original melee validators or victim PushState.
    if(!boolean(controller,L"bAllowCombatTransitions",false)){
        message(L"melee_target_combat_disable_failed");return false;}
    // Select the original DebugState and suspend only automatic perception
    // timers on this owned practice AI. Stationary behavior is checked in the
    // actual range; Tick/animation and the original victim PushState stay on.
    for(const auto label:{L"FillEnemyList",L"UpdateAllEnemyInfo",L"HandleInfoRequests"}){
        const auto function=field(slot.controller.klass,label,L"Function");
        if(!function){message(L"melee_target_perception_timer_schema_failed");return false;}
        const auto timer=get<Name>(function+0x48);
        Args clear(controller,L"ClearTimer");
        if(!current_range_controller(slot.controller)||!clear.set(L"inTimerFunc",timer).run()){
            message(L"melee_target_clear_perception_timer_failed");return false;}
    }
    const auto state=field(slot.controller.klass,L"DebugState",L"State");
    if(!state){message(L"melee_target_debug_state_missing");return false;}
    const auto state_name=get<Name>(state+0x48);
    Args stop(controller,L"StopFiring"),clear_move(controller,L"ClearMovementInfo"),
        idle(controller,L"GotoState"),state_query(controller,L"GetStateName");
    if(!stop.run()||!clear_move.run()||!idle.set(L"NewState",state_name).run()||!state_query.run()){
        message(L"melee_target_debug_state_call_failed");return false;}
    const auto actual=state_query.output<Name>();
    if(actual.index!=state_name.index||actual.number!=state_name.number||
       !current_range_controller(slot.controller)||pointer(actor,L"Controller")!=controller||
       pointer(controller,L"Pawn")!=actor){message(L"melee_target_debug_state_or_links_not_current");return false;}
    const auto position=scalar<Vector>(actor,L"Location");
    if(log_file){fwprintf(log_file,L"playground melee_target_ready pawn=%llx controller=%llx state=DebugState original_controller=1 combat_transitions=0 x=%.3f y=%.3f z=%.3f\n",
        actor,controller,position.x,position.y,position.z);fflush(log_file);}
    return true;
}
bool create_target(Slot& slot){
    const auto actor=spawn(slot.cls,slot.location,Rotation{0,32768,0});
    if(!actor)return false;
    // A dynamic Pawn is deliberately left without an AI controller: a stationary,
    // damageable enemy target. It is placed on a world trace before spawning.
    if(pointer(actor,L"Controller")){Args cleanup(actor,L"Destroy");cleanup.run();return false;}
    boolean(actor,L"bDontPossess",true);slot.actor=actor;slot.after=0;
    slot.identity={actor,get<uptr>(actor+0x50),arena_world,get<uptr>(actor+0x40),get<unsigned>(actor+8),get<Name>(actor+0x48)};
    if(slot.melee&&!prepare_melee_target(slot,actor)){
        message(L"melee_target_controller_setup_failed");retire_target(slot);return false;
    }
    return true;
}
bool initialize_arena(uptr pc,uptr pawn,uptr world){
    arena_pc=pc;arena_pawn=pawn;arena_world=world;
    turret=0;
    const auto array=get<Array>(base+0x1c24730);
    for(int i=0;i<array.count;++i){auto p=get<uptr>(array.data+size_t(i)*8);
        if(instance(p,L"Turret_TroikaCabal")&&in_map(p)){turret=p;break;}}
    if(!turret){message(L"existing_troika_missing");return false;}
    // The raised platform behind the existing Troika is the equipment area.
    for(unsigned i=0;i<guns.size();++i){auto& s=guns[i];s={};s.cls=find_class(weapon_names[i]);
        // Keep grenade pickups on the walkable inner platform. The old
        // positive-y rack rows put them behind the equipment sandbags;
        // approach stopped at y159.64 and original X selected a nearer Bow.
        // They remain real original pickups with original ammo/interaction.
        const bool grenade=i>=9;
        const float x=grenade?-3770.f:-4100.f+float(i%2)*180.f;
        const float y=grenade?(-140.f-float(i-9)*220.f):-550.f+float(i/2)*210.f;
        if(!s.cls||!floor_at(x,y,900,s.location)){message(weapon_names[i]);message(L"weapon_class_or_floor_missing");return false;}
        s.location.z+=45;if(!create_gun(s)){message(weapon_names[i]);message(L"pickup_creation_failed");return false;}}
    const auto target_class=find_class(L"Pawn_GeistLocust");
    for(unsigned i=0;i<targets.size();++i){auto& s=targets[i];s={};s.cls=target_class;
        // Keep the practice opponent on the player's raised starting area.
        // The old -900 side lies beyond the equipment sandbags;41864's
        // straight walking route was blocked there. Other five are unchanged.
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        s.melee=i==0;
#endif
        const float x=s.melee?-3650.f:-2850.f+float(i/2)*450.f,y=s.melee?0.f:((i%2)?350.f:-350.f);
        if(!s.cls||!floor_at(x,y,900,s.location)){message(L"target_floor_missing");return false;}
        s.location.z+=90;if(!create_target(s)){message(L"target_creation_failed");return false;}}
    Vector start{};if(!floor_at(-3900,0,900,start))return false;start.z+=90;
    Args location(pawn,L"SetLocation");location.set(L"NewLocation",start);if(!location.run()||!location.output<unsigned>())return false;
    Args rotation(pc,L"SetRotation");rotation.set(L"NewRotation",Rotation{0,0,0});rotation.run();
    ready=true;phase=L"ready";message(L"ready");begin_vehicle();status();return true;
}
void maintain(){
    arena_pawn=pointer(arena_pc,L"Pawn");
    if(instance(arena_pawn,L"WarTurret")||vehicle_seat(arena_pawn))arena_pawn=pointer(arena_pawn,L"Driver");
    if(instance(arena_pawn,L"WarPawn")){
        auto inv=pointer(arena_pawn,L"InvManager");auto item=pointer(inv,L"InventoryChain");
        for(unsigned n=0;item&&n<32;++n){infinite(item);const auto next=pointer(item,L"Inventory");if(next==item)break;item=next;}
        infinite(pointer(arena_pawn,L"Weapon"));
    }
    // The map's mounted weapon has its own inventory outside the player's chain.
    infinite(pointer(turret,L"Weapon"));
    advance_vehicle();
    const auto now=GetTickCount64();
    for(auto& s:guns){
        const bool missing=deleted(s.actor)||pointer(s.actor,L"Inventory")!=s.inventory||pointer(s.inventory,L"Instigator")!=0;
        if(missing){
            if(!s.after)s.after=now+2000;
            if(now>=s.after){if(!deleted(s.actor)){Args d(s.actor,L"Destroy");d.run();}
                if(create_gun(s))++pickup_respawns;else{++failures;s.after=now+5000;}}
        }
    }
    for(auto& s:targets)if(!current_range_object(s.identity)||deleted(s.actor)||scalar<int>(s.actor,L"Health")<=0){
        // A chainsaw victim can be dead while its original paired animation
        // still owns both actors. Respawning at the old fixed3s deadline
        // would destroy that victim under the attacker's animation.
        if(s.melee&&current_range_object(s.identity)&&!deleted(s.actor)&&
           (scalar<unsigned char>(s.actor,L"SpecialMove")==35||
            (scalar<unsigned char>(arena_pawn,L"SpecialMove")==34&&
             pointer(pointer(arena_pawn,L"Weapon"),L"SpecialMeleeAttackVictim")==s.actor))){s.after=0;continue;}
        if(!s.after)s.after=now+3000;
        if(now>=s.after){retire_target(s);
            if(create_target(s))++target_respawns;else{++failures;s.after=now+5000;}}
    }
}
bool navigation(uptr object,unsigned char face,uptr target){
    Args a(object,L"SetForcedNavigationTarget");a.set(L"Face",face).set(L"NavTarget",target).flag(L"bIsNullOverride",false);
    return a.run()&&a.output<unsigned>();
}
bool add_menu(){
    if(!instance(scene_client,L"GameUISceneClient"))scene_client=find_instance(L"GameUISceneClient");
    if(!scene_client)return false;
    const auto scenes=scalar<Array>(scene_client,L"ActiveScenes");
    if(!scenes.data||scenes.count<1||scenes.count>64||scenes.count>scenes.capacity)return false;
    uptr scene{};
    for(int i=scenes.count-1;i>=0;--i){const auto s=get<uptr>(scenes.data+i*8);
        if(instance(s,L"WarUIScene_MainMenu")){scene=s;break;}}
    if(!scene)return false;
    if(scene==menu_scene&&instance(menu_button,L"UILabelButton")&&pointer(menu_button,L"OwnerScene")==scene){
        const auto bounds=scalar<MenuBounds>(menu_button,L"RenderBounds");
        const bool valid=std::isfinite(bounds.left)&&std::isfinite(bounds.top)&&std::isfinite(bounds.right)&&std::isfinite(bounds.bottom)&&
            bounds.right-bounds.left>80.f&&bounds.bottom-bounds.top>8.f;
        if(valid&&!menu_layout_valid){menu_bounds=bounds;menu_layout_valid=true;message(L"menu_layout_ready");status();}
        return true;
    }
    const auto p=property(scene,L"CampaignButton");const auto campaign=p?pointer(scene,L"CampaignButton"):0;
    if(!instance(campaign,L"UILabelButton"))return false;
    const auto parent=pointer(campaign,L"Owner");if(!parent)return false;
    auto unique=get<Name>(campaign+0x48);unique.number=60001;
    Args create(scene,L"CreateWidget");create.set(L"Owner",parent).set(L"WidgetClass",get<uptr>(campaign+0x50)).set(L"WidgetArchetype",campaign).set(L"WidgetName",unique);
    if(!create.run())return false;const auto button=create.output<uptr>();if(!button)return false;
    // The duplicate uses the game's existing button style and font. Its own
    // delegate consumes activation before the archetype's campaign sequence.
    put(button,L"WidgetTag",unique);put(button,L"TabIndex",8);
    const auto clicked_fn=field(get<uptr>(button+0x50),L"OnClicked",L"Function");
    if(!clicked_fn)return false;
    struct Delegate {uptr object;Name fn;} delegate{button,get<Name>(clicked_fn+0x48)};
    if(!put(button,L"__OnClicked__Delegate",delegate))return false;
    Args insert(parent,L"InsertChild");insert.set(L"NewChild",button).set(L"InsertIndex",-1).flag(L"bRenameExisting",false);
    if(!insert.run()||insert.output<int>()<0)return false;
    // Zero is UIFACE_Left, not an empty docking target. Clear links through the
    // engine so all faces use UIFACE_MAX and OwnerWidget remains initialized.
    for(unsigned char face=0;face<4;++face){Args dock(button,L"SetDockTarget");dock.set(L"SourceFace",face).set(L"Target",uptr{}).set(L"TargetFace",static_cast<unsigned char>(4));if(!dock.run()||!dock.output<unsigned>())return false;}
    Args caption(button,L"SetCaption");caption.text(L"NewText",L"happyotaku playground");if(!caption.run())return false;
    // Existing menu rows have explicit links and a forced end stop. Insert the
    // new entry into that same controller/keyboard cycle in both directions.
    uptr last=campaign;int last_index=-1;const auto siblings=scalar<Array>(parent,L"Children");
    if(!siblings.data||siblings.count<1||siblings.count>128||siblings.count>siblings.capacity)return false;
    for(int i=0;i<siblings.count;++i){const auto child=get<uptr>(siblings.data+i*8);
        if(child==button||!instance(child,L"UILabelButton"))continue;
        const auto index=scalar<int>(child,L"TabIndex");const auto hidden=property(child,L"bHidden");
        const bool invisible=hidden&&(get<unsigned>(child+get<unsigned>(hidden+0x94))&get<unsigned>(hidden+0xd0));
        if(!invisible&&index>=0&&index<=7&&index>last_index){last_index=index;last=child;}}
    // Follow the existing menu column and row height across resolutions/safe
    // regions instead of placing an unrelated fixed rectangle on the screen.
    auto coordinate=[](uptr widget,unsigned char face,float& value){Args a(widget,L"GetPosition");a.set(L"Face",face).set(L"OutputType",static_cast<unsigned char>(4)).flag(L"bIncludeOrigin",true).flag(L"bIgnoreDockPadding",true);if(!a.run())return false;value=a.output<float>();return std::isfinite(value);};
    float left{},right{},top{},bottom{};
    if(!coordinate(campaign,0,left)||!coordinate(campaign,2,right)||!coordinate(last,1,top)||!coordinate(last,3,bottom))return false;
    const float height=bottom-top;
    if(right-left<.1f||height<.005f||height>.15f)return false;
    const float positions[]={left,bottom+height*.25f,right,bottom+height*1.25f};
    for(unsigned char face=0;face<4;++face){Args position(button,L"SetPosition");position.set(L"NewValue",positions[face]).set(L"Face",face).set(L"InputType",static_cast<unsigned char>(4)).flag(L"bZeroOrigin",true);if(!position.run())return false;}
    if(!navigation(last,3,button)||!navigation(button,1,last)||!navigation(button,3,campaign)||!navigation(campaign,1,button))return false;
    Args update(scene,L"RequestSceneUpdate");update.flag(L"bDockingStackChanged",true).flag(L"bNavLinksOutdated",true).flag(L"bPositionsChanged",true).flag(L"bWidgetStylesChanged",true);update.run();
    Args owner(scene,L"GetPlayerOwner");if(owner.run())player=owner.output<uptr>();
    menu_button=button;menu_scene=scene;menu_layout_valid=false;menu_bounds={};phase=L"menu";message(L"menu_added");status();return true;
}
bool clicked(uptr object,uptr function,void* params){
    if(!automatic||object!=menu_button||!named(function,L"OnClicked"))return false;
    const auto result=get<unsigned short>(function+0x108);
    if(params&&result!=0xffff){unsigned yes=1;memcpy(static_cast<unsigned char*>(params)+result,&yes,4);}
    requested=true;requested_at=GetTickCount64();phase=L"travel_requested";next_tick=0;return true;
}
void tick(){
    if(stop_requested.load()){
        if(cancel_vehicle_load())stop_acknowledged=true;
        return;
    }
    if(test_enter_pending.exchange(false)){
        player=find_instance(L"WarLocalPlayer");requested_at=GetTickCount64();
        phase=L"travel_requested";requested=true;next_tick=0;
    }
    vehicle_controls();
    const auto now=GetTickCount64();if(now<next_tick)return;next_tick=now+500;
    try{
        if(!viewport)viewport=find_instance(L"WarGameViewportClient");
        if(!viewport)return;
        add_menu();
        if(!requested)return;
        if(!registered(player)){phase=L"player_missing";status();return;}
        auto pc=pointer(player,L"Actor");
        if(instance(pc,L"WarPC_Menu")){
            if(!cancel_vehicle_load())return;
            ready=false;arena_world=arena_pc=arena_pawn=turret=0;guns={};targets={};
            reset_vehicle();
            if(!wcscmp(phase,L"travel_requested")){
                Args travel(pc,L"ClientTravel");travel.text(L"URL",L"MP_WarMachine?Game=WarfareGame.WarGameSP").set(L"TravelType",static_cast<unsigned char>(0)).flag(L"bSeamless",false);
                if(travel.run()){phase=L"loading";message(L"travel_dispatched");}
                else{++failures;phase=L"travel_failed";requested=false;}
            }else if(!wcscmp(phase,L"ready")){requested=false;phase=L"menu";}
            status();return;
        }
        const auto world=pointer(pc,L"WorldInfo"),pawn=pointer(pc,L"Pawn");
        const auto driver=(instance(pawn,L"WarTurret")||vehicle_seat(pawn))?pointer(pawn,L"Driver"):0;
        const bool ordinary=instance(pawn,L"WarPawn")&&pointer(pawn,L"Controller")==pc;
        const bool mounted=driver&&instance(driver,L"WarPawn")&&pointer(driver,L"DrivenVehicle")==pawn&&pointer(pawn,L"Controller")==pc;
        if(!instance(pc,L"WarPC")||!in_map(world)||(!ordinary&&!mounted)){
            if(!cancel_vehicle_load())return;
            if(ready){ready=false;requested=false;phase=L"outside_playground";}
            else if(now-requested_at>120000){requested=false;phase=L"load_timeout";++failures;}
            status();return;
        }
        if(!ready){if(!initialize_arena(pc,pawn,world)){++failures;phase=L"setup_failed";requested=false;}}
        else if(world!=arena_world){cancel_vehicle_load();ready=false;requested=false;phase=L"outside_playground";}
        if(ready)maintain();status();
    }catch(...){cancel_vehicle_load();++failures;requested=false;ready=false;phase=L"setup_exception";status();}
}
}
