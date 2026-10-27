// Owned Playground Junker only. Reuse the campaign vehicle and seat controllers;
// do not write PC.Pawn, camera state, global weapon defaults or campaign actors.
bool flag_value(uptr object,const wchar_t* value){
    const auto p=property(object,value);
    return p&&named(get<uptr>(p+0x50),L"BoolProperty")&&
        (get<unsigned>(object+get<unsigned>(p+0x94))&get<unsigned>(p+0xd0));
}
bool vehicle_seat(uptr pawn){
    return !deleted(range_vehicle)&&instance(range_vehicle,L"Vehicle_Junker")&&
        (pawn==range_vehicle||(instance(pawn,L"WarJunkerSpotlightPawn")&&pointer(pawn,L"MyVehicle")==range_vehicle));
}
struct VehicleSeats {Array array{};uptr type{};unsigned stride{};};
VehicleSeats vehicle_seats(){
    VehicleSeats result{};if(!vehicle_seat(range_vehicle))return result;
    const auto p=property(range_vehicle,L"Seats"),inner=p?get<uptr>(p+0xd0):0;
    if(!registered(inner)||!named(get<uptr>(inner+0x50),L"StructProperty"))return result;
    const auto type=get<uptr>(inner+0xd0);const auto stride=get<unsigned>(inner+0x74);
    const auto a=scalar<Array>(range_vehicle,L"Seats");
    if(!registered(type)||!named(get<uptr>(type+0x50),L"ScriptStruct")||stride<16||stride>1024||
        !a.data||a.count<2||a.count>8||a.count>a.capacity||a.capacity>8)return result;
    return {a,type,stride};
}
uptr seat_field(VehicleSeats const& seats,int i,const wchar_t* field_name){
    if(i<0||i>=seats.array.count)return 0;
    const auto p=field(seats.type,field_name,L"ObjectProperty");
    if(!p||get<unsigned>(p+0x74)!=8)return 0;
    const auto offset=get<unsigned>(p+0x94);if(offset+8>seats.stride)return 0;
    return seats.array.data+size_t(i)*seats.stride+offset;
}
bool arm_range_vehicle(){
    const auto seats=vehicle_seats();if(!seats.type)return false;
    const auto gun=get<uptr>(seat_field(seats,1,L"Gun"));
    if(!instance(gun,L"WarVWeap_UVTurret")||pointer(gun,L"MyVehicle")!=range_vehicle)return false;
    // Campaign UV never needs a bullet muzzle. Its inherited GunFireSocket is
    // absent from this mesh and returns world zero. Use the actual roof socket.
    const auto sockets_prop=field(seats.type,L"GunSocket",L"ArrayProperty");
    if(!sockets_prop||get<unsigned>(sockets_prop+0x94)+sizeof(Array)>seats.stride)return false;
    const auto socket_offset=get<unsigned>(sockets_prop+0x94);
    const auto sockets=get<Array>(seats.array.data+seats.stride+socket_offset);
    if(!sockets.data||sockets.count!=1||sockets.capacity!=1)return false;
    const auto socket_name=get<Name>(find_class(L"Turret")+0x48);
    Args socket(pointer(range_vehicle,L"Mesh"),L"GetSocketWorldLocationAndRotation");socket.set(L"InSocketName",socket_name);
    if(!socket.run()||!socket.output<unsigned>())return false;
    // Original UV has zero ordinary-actor damage. Its existing instant-hit path
    // supports the range's targets once the spawned gun's own values are set.
    const auto prm=property(gun,L"PRM_Damage");
    if(!prm||!named(get<uptr>(prm+0x50),L"StructProperty"))return false;
    const auto type=get<uptr>(prm+0xd0),value=field(type,L"Value",L"ArrayProperty");
    if(!value)return false;
    const auto inner=get<uptr>(value+0xd0);
    if(!registered(inner)||!named(get<uptr>(inner+0x50),L"FloatProperty")||get<unsigned>(inner+0x74)!=4)return false;
    const auto offset=get<unsigned>(prm+0x94)+get<unsigned>(value+0x94);
    const auto values=get<Array>(gun+offset);
    if(!values.data||values.count<1||values.count>8||values.count>values.capacity||values.capacity>8)return false;
    // Refuse a shared CDO array: only the freshly spawned weapon is writable.
    const auto objects=get<Array>(base+0x1c24730);
    bool private_values=false,private_sockets=false;
    for(int i=0;i<objects.count;++i){const auto o=get<uptr>(objects.data+size_t(i)*8);
        if(o&&named(o,L"Default__WarVWeap_UVTurret")&&get<uptr>(o+0x50)==get<uptr>(gun+0x50)){
            private_values=get<Array>(o+offset).data!=values.data;}
        if(o&&named(o,L"Default__Vehicle_Junker")&&get<uptr>(o+0x50)==get<uptr>(range_vehicle+0x50)){
            const auto defaults=scalar<Array>(o,L"Seats");
            if(defaults.data&&defaults.count>1&&defaults.count<=defaults.capacity)
                private_sockets=get<Array>(defaults.data+seats.stride+socket_offset).data!=sockets.data;}}
    if(!private_values||!private_sockets)return false;
    *reinterpret_cast<Name*>(sockets.data)=socket_name;
    for(int i=0;i<values.count;++i)reinterpret_cast<float*>(values.data)[i]=35.f;
    return infinite(gun);
}
bool cancel_vehicle_load(){
    if(!vehicle_loading)return true;
    if(!instance(vehicle_loader,L"GameEngine"))return false;
    // Remove only our own delayed script retry before cancelling native work.
    if(registered(arena_pc)){
        const auto queued=scalar<Array>(arena_pc,L"PendingMapChangeLevelNames");
        const auto first=get<Name>(queued.data);
        if(queued.count==1&&queued.capacity>=1&&first.index==vehicle_level.index&&first.number==vehicle_level.number){
            const auto delayed=field(get<uptr>(arena_pc+0x50),L"DelayedPrepareMapChange",L"Function");
            if(!delayed)return false;
            Args clear(arena_pc,L"ClearTimer");clear.set(L"inTimerFunc",get<Name>(delayed+0x48));
            if(!clear.run()||!put(arena_pc,L"PendingMapChangeLevelNames",Array{queued.data,0,queued.capacity}))return false;
        }
    }
    const auto names=scalar<Array>(vehicle_loader,L"LevelsToLoadForPendingMapChange");
    if(names.count==0){vehicle_loading=false;return true;}
    if(names.count!=1||!names.data||names.count>names.capacity)return false;
    const auto selected=get<Name>(names.data);
    if(selected.index!=vehicle_level.index||selected.number!=vehicle_level.number){
        // A later engine travel superseded our load; leave its preparation alone.
        vehicle_loading=false;vehicle_phase="load_superseded";return true;
    }
    Args cancel(vehicle_loader,L"CancelMapPendingMapChangeAndAsyncLoads");
    if(!cancel.run()||scalar<Array>(vehicle_loader,L"LevelsToLoadForPendingMapChange").count!=0)return false;
    vehicle_loading=false;return true;
}
bool spawn_range_vehicle(){
    Vector location{};const auto cls=find_class(L"Vehicle_Junker");
    if(!cls||!floor_at(-3000,-900,900,location))return false;
    location.z+=164;range_vehicle=spawn(cls,location);
    if(!vehicle_seat(range_vehicle)||pointer(range_vehicle,L"WorldInfo")!=arena_world||!arm_range_vehicle()){
        if(!deleted(range_vehicle)){Args destroy(range_vehicle,L"Destroy");destroy.run();}
        range_vehicle=0;return false;
    }
    vehicle_phase="ready";message(L"junker_ready");return true;
}
void begin_vehicle(){
    vehicle_phase="loading";vehicle_deadline=GetTickCount64()+30000;
    if(find_class(L"Vehicle_Junker")){
        if(!spawn_range_vehicle()){vehicle_phase="spawn_failed";++failures;}return;
    }
    Args preparing_map(arena_world,L"IsPreparingMapChange");
    if(!preparing_map.run()||preparing_map.output<unsigned>()){vehicle_phase="resource_loader_busy";++failures;return;}
    // Cooked map-contained classes cannot be resolved with DynamicLoadObject
    // alone. This primes its FName, then the engine's asynchronous package loader
    // loads the dependency map. Never commit or run that map's mission sequence.
    Args lookup(arena_pc,L"DynamicLoadObject");lookup.text(L"ObjectName",L"SP_Viaducts_Scripting_01.TheWorld")
        .set(L"ObjectClass",find_class(L"World")).flag(L"MayFail",true);lookup.run();
    vehicle_loader=find_instance(L"GameEngine");
    if(!vehicle_loader){vehicle_phase="resource_loader_missing";++failures;return;}
    const auto names=get<Array>(base+0x1c34800);Name level{};
    const wchar_t wanted[]=L"SP_Viaducts_Scripting_01";
    for(int i=0;i<names.count&&i<4000000;++i){const auto entry=get<uptr>(names.data+size_t(i)*8);wchar_t text[std::size(wanted)]{};
        if(entry&&get<unsigned>(entry)==unsigned(i)&&copy(text,entry+20,sizeof(text))&&!wcscmp(text,wanted)){level.index=i;break;}}
    if(!level.index){vehicle_phase="resource_name_missing";++failures;return;}
    Args prepare(arena_pc,L"ClientPrepareMapChange");prepare.set(L"LevelName",level).flag(L"bFirst",true).flag(L"bLast",true);
    vehicle_level=level;vehicle_loading=true;
    if(!prepare.run()){cancel_vehicle_load();vehicle_phase="resource_load_failed";++failures;}
}
void advance_vehicle(){
    if(!vehicle_loading)return;
    Args loaded(arena_world,L"IsMapChangeReady");
    if(loaded.run()&&loaded.output<unsigned>()){
        const bool okay=range_vehicle||spawn_range_vehicle();
        if(!cancel_vehicle_load()){vehicle_phase="resource_cleanup_pending";return;}
        if(!okay){vehicle_phase="spawn_failed";++failures;}
    }else if(GetTickCount64()>vehicle_deadline){cancel_vehicle_load();vehicle_phase="resource_load_timeout";++failures;}
}
void reset_vehicle(){
    range_vehicle=vehicle_driver=vehicle_loader=previous_vehicle_pawn=0;vehicle_loading=false;vehicle_use_held=vehicle_exit_held=false;vehicle_phase="idle";
}
void vehicle_controls(){
    if(!ready||!registered(arena_pc)||pointer(arena_pc,L"WorldInfo")!=arena_world||!in_map(arena_world)||deleted(range_vehicle))return;
    const auto pc=arena_pc,pawn=pointer(pc,L"Pawn");
    const bool walking=instance(pawn,L"WarPawn")&&pointer(pawn,L"Controller")==pc;
    if(walking){vehicle_driver=pawn;vehicle_shadow=flag_value(pointer(pawn,L"Mesh"),L"CastShadow");}
    if(vehicle_seat(pawn)&&!vehicle_seat(previous_vehicle_pawn))++vehicle_entries;
    previous_vehicle_pawn=pawn;
    bool use=false;
    if(flag_value(pc,L"bHoldingButton_X")||flag_value(pc,L"bHoldingButton_Y")){
        Args scheme(pc,L"GetActiveControlScheme");
        if(scheme.run())use=flag_value(pc,scheme.output<unsigned char>()==1?L"bHoldingButton_Y":L"bHoldingButton_X");
    }
    const bool leave=flag_value(pc,L"bHoldingButton_B");
    const bool use_edge=use&&!vehicle_use_held,leave_edge=leave&&!vehicle_exit_held;
    vehicle_use_held=use;vehicle_exit_held=leave;
    if(scalar<int>(pc,L"InputIsDisabledCount")||pointer(arena_world,L"Pauser"))return;
    if(walking){
        if(!use_edge)return;
        const auto a=scalar<Vector>(pawn,L"Location"),v=scalar<Vector>(range_vehicle,L"Location");
        const auto dx=a.x-v.x,dy=a.y-v.y,dz=a.z-v.z;
        if(dx*dx+dy*dy>650.f*650.f||std::fabs(dz)>250.f)return;
        Args enter(range_vehicle,L"TryToDrive");enter.set(L"P",pawn);
        enter.run();
    }else if(leave_edge&&vehicle_seat(pawn)&&pointer(pawn,L"Controller")==pc){
        const auto driver=pointer(pawn,L"Driver");
        if(!instance(driver,L"WarPawn")||pointer(driver,L"DrivenVehicle")!=pawn)return;
        const auto velocity=scalar<Vector>(range_vehicle,L"Velocity");
        if(velocity.x*velocity.x+velocity.y*velocity.y+velocity.z*velocity.z>150.f*150.f)return;
        Args exit(pawn,L"DriverLeave");exit.flag(L"bForceLeave",true);
        if(exit.prop(L"bAllowRelocation"))exit.flag(L"bAllowRelocation",true);
        if(!exit.run()||!exit.output<unsigned>()||pointer(pc,L"Pawn")!=driver||pointer(driver,L"Controller")!=pc)return;
        Args restore(driver,L"RestoreAnimSetsToDefault");if(restore.run()&&restore.output<unsigned>()){Args update(pointer(driver,L"Mesh"),L"UpdateAnimations");update.run();}
        Args stance(driver,L"StopAllBodyStances");stance.set(L"BlendOutTime",0.f);stance.run();
        if(driver==vehicle_driver)boolean(pointer(driver,L"Mesh"),L"CastShadow",vehicle_shadow);
        const auto seats=vehicle_seats();
        for(int i=0;i<seats.array.count;++i){const auto storage=seat_field(seats,i,L"StoragePawn");if(storage&&get<uptr>(storage)==driver)*reinterpret_cast<uptr*>(storage)=0;}
        ++vehicle_exits;vehicle_driver=0;
    }
}
