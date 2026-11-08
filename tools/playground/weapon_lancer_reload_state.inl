// Original Reloading lease. No timeout completes physical reload. Start/exit
// are called only by the hand transaction, not by an independent tick worker.
namespace lancerreload_state {
namespace inv=lancerreload_inventory;
namespace hand=gears_weapon_hand;
struct ActiveWindow {
    float start{},pre_reaction{},perfect_duration{},ordinary_duration{};
    bool valid{};
};
// Ask the current weapon override, rather than deriving a window from default
// fields (some of which contain -1). Validate all four reflected out arguments
// before invoking the existing game-thread script/native dispatcher.
ActiveWindow active_window(uptr weapon) noexcept {
    ActiveWindow out{};
    playground::Args call(weapon,L"GetActiveReloadValues");
    if(!call.valid||call.r.size!=4*sizeof(float))return out;
    const wchar_t* names[]={L"ActiveReloadStartTime",L"PreReactionWindowDuration",
        L"SuperSweetSpotDuration",L"SweetSpotDuration"};
    unsigned occupied=0;
    for(const auto name:names){
        const auto p=call.prop(name);
        if(!p||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=sizeof(float)||
           !playground::named(get<uptr>(p+0x50),L"FloatProperty")||
           (get<unsigned long long>(p+0x78)&0x580)!=0x180)return out;
        const auto offset=get<unsigned>(p+0x94);
        if(offset%sizeof(float)||offset>call.r.size-sizeof(float))return out;
        const unsigned bit=1u<<(offset/sizeof(float));
        if(occupied&bit)return out;
        occupied|=bit;
        call.set(name,std::numeric_limits<float>::quiet_NaN());
    }
    if(occupied!=15||!call.run())return out;
    out.start=call.output<float>(names[0]);
    out.pre_reaction=call.output<float>(names[1]);
    out.perfect_duration=call.output<float>(names[2]);
    out.ordinary_duration=call.output<float>(names[3]);
    out.valid=std::isfinite(out.start)&&out.start>=0&&
        std::isfinite(out.pre_reaction)&&out.pre_reaction>=0&&
        std::isfinite(out.perfect_duration)&&out.perfect_duration>0&&
        std::isfinite(out.ordinary_duration)&&out.ordinary_duration>=0&&
        std::isfinite(out.start+out.perfect_duration);
    return out;
}
struct Lease {
    inv::Identity identity{};
    std::uint64_t serial{};
    uptr abort_function{},finish_function{};
    float original_start{};
    Name timer{};
    bool acquired{};
    hand::ObjectIdentity abort_identity{},finish_identity{};
    ActiveWindow window{};
    // Bound the accepted original call in the controller sample clock. Use
    // the earlier bound for deadlines so dispatcher latency cannot buy time.
    std::int64_t accepted_begin_ms{},accepted_end_ms{},perfect_end_ms{};
    float reward_start{}; // Survives reacquiring native state after a switch.
    // Original HUD DrawActionReloadBar reads these exact weapon properties.
    // Journal only this owned instance; restoration precedes the native exit.
    std::array<float,6> ui_before{},ui_wanted{};
    bool ui_owned{},ui_replaced{};
    unsigned attempt_offset{},attempt_mask{};bool attempts_owned{},attempt_before{};
    hand::ObjectIdentity ui_hud{},ui_controller{};
    unsigned hud_tick_offset{},hud_result_offset{},hud_fade_offset{},hud_flash_offset{};
    float hud_tick_before{},hud_tick_wanted{};
    unsigned char hud_result_before{};bool hud_owned{};
    hand::ObjectIdentity ui_draw_function{},ui_getter_function{};
    std::array<hand::ObjectIdentity,4> ui_local_properties{};
    std::array<unsigned,4> ui_local_offsets{};
    Array ui_draw_script{};unsigned ui_locals_extent{};std::uint64_t ui_draw_updates{};
    std::array<float,4> ui_draw_original{},ui_draw_current{};
    unsigned ui_draw_gate{},ui_reported_gate{};bool ui_draw_reported{};
    std::array<hand::ObjectIdentity,2> ui_clock_properties{};
    std::array<unsigned,2> ui_clock_offsets{};
    std::uint64_t ui_display_scopes{},ui_display_late{},ui_display_restores{};
    std::array<float,4> ui_display_last{}; // real start, drawing start, real elapsed, marker
    std::array<float,2> ui_fade_durations{};
    // DrawX/DrawY/Scale input-value parameters, Canvas/ClipX/ClipY,
    // ResScaleSmall and the original AR background texture's UL/VL.
    std::array<hand::ObjectIdentity,14> ui_layout_properties{};
    std::array<unsigned,14> ui_layout_offsets{},ui_layout_raw_offsets{};
    hand::ObjectIdentity ui_canvas_class{},ui_setpos_function{};
    std::uint64_t ui_layout_updates{},ui_body_observations{},ui_shape_updates{};
    std::uint64_t ui_marker_scopes{},ui_marker_restores{};
    float ui_marker_resolution{};
    float ui_shape_last{};
    unsigned ui_body_gate{};
    std::array<float,5> ui_layout_last{}; // x,y,scale,width,height
    std::array<float,7> ui_body_last{}; // DrawX,DrawY,Scale,BarLength,fade,flash,Alpha
    // Optional read-only receipt AFTER the original Canvas.SetPos returned.
    // Missing observation metadata never blocks a reload or drawing adapter.
    std::array<hand::ObjectIdentity,5> ui_canvas_properties{};
    std::array<unsigned,5> ui_canvas_offsets{};
    bool ui_canvas_known{},ui_final_reported{};
    std::uint64_t ui_canvas_call{},ui_canvas_observations{};
    std::array<float,7> ui_canvas_last{}; // CurX/Y,OrgX/Y,ClipX/Y,alpha
    unsigned ui_canvas_color{};
};
Lease lease;
std::uint64_t next_serial{1};
unsigned ui_setup_gate{};
bool bool_result(playground::Args& call,bool& value) noexcept {
    if(!vrequip::result_property(call,L"BoolProperty",4))return false;
    const auto mask=get<unsigned>(call.prop(L"ReturnValue")+0xd0);
    if(!mask||(mask&(mask-1))||!call.run())return false;
    value=(call.output<unsigned>()&mask)!=0;return true;
}
bool bool_call(uptr weapon,const wchar_t* function,bool& value) noexcept {
    playground::Args call(weapon,function);return bool_result(call,value);
}
bool timer_argument(playground::Args& call,Name timer) noexcept {
    const auto p=call.prop(L"inTimerFunc");
    return call.valid&&p&&get<unsigned>(p+0x74)==sizeof(Name)&&
        playground::named(get<uptr>(p+0x50),L"NameProperty")&&
        call.set(L"inTimerFunc",timer).valid;
}
bool timer_active(uptr weapon,Name timer,bool& value) noexcept {
    playground::Args call(weapon,L"IsTimerActive");
    return timer_argument(call,timer)&&bool_result(call,value);
}
bool clear_timer(uptr weapon,Name timer) noexcept {
    playground::Args call(weapon,L"ClearTimer");bool active{};
    return timer_argument(call,timer)&&call.run()&&timer_active(weapon,timer,active)&&!active;
}
uptr state_function(uptr weapon,const wchar_t* wanted) noexcept {
    auto cls=get<uptr>(weapon+0x50);
    for(unsigned depth=0;cls&&depth<40;++depth,cls=get<uptr>(cls+0x60)){
        unsigned states=0;
        for(auto state=get<uptr>(cls+0x80);state&&states++<4096;state=get<uptr>(state+0x68)){
            if(!registered(state))return 0;
            if(!playground::named(state,L"Reloading")||
               !playground::named(get<uptr>(state+0x50),L"State"))continue;
            unsigned count=0;
            for(auto fn=get<uptr>(state+0x80);fn&&count++<256;fn=get<uptr>(fn+0x68)){
                if(!registered(fn))return 0;
                if(playground::named(fn,wanted)&&playground::named(get<uptr>(fn+0x50),L"Function")&&
                   get<uptr>(fn+0x40)==state&&get<unsigned short>(fn+0x106)==0)return fn;
            }
        }
    }
    return 0;
}
bool time_field(uptr weapon,const wchar_t* name,float& out) noexcept {
    const auto p=playground::property(weapon,name);
    if(!p||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=4||
       !playground::named(get<uptr>(p+0x50),L"FloatProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),extent=get<unsigned>(get<uptr>(weapon+0x50)+0x88);
    return offset<=extent&&4<=extent-offset&&weapon<=UINTPTR_MAX-offset-4&&
        copy(&out,weapon+offset,4)&&std::isfinite(out)&&out>=0;
}
bool start_time(uptr weapon,float& out) noexcept {
    return time_field(weapon,L"ReloadStartTime",out);
}
// Capture at physical closure, before inventory setters/exit callbacks. Map
// elapsed GAME time into the ledger's start coordinate; wall-clock pauses and
// dispatch latency must not change the original game's reward deadline.
bool closure_time(hand::Frame const& source,std::uint64_t serial,std::int64_t& tick) noexcept {
    if(!lease.acquired||lease.serial!=serial||!(inv::identity(source)==lease.identity))return false;
    hand::ObjectIdentity world{};float now{};
    if(!nativeequipment::identify(nullptr,lease.identity.world.address,world)||
       !hand::same(world,lease.identity.world)||
       playground::pointer(source.pawn.address,L"WorldInfo")!=world.address||
       !time_field(world.address,L"TimeSeconds",now)||now<lease.reward_start)return false;
    const double elapsed=std::ceil((double(now)-double(lease.reward_start))*1000.0);
    if(elapsed<0||elapsed>=double(INT64_MAX-lease.accepted_begin_ms))return false;
    tick=lease.accepted_begin_ms+std::int64_t(elapsed);return true;
}
bool normal_mode(uptr weapon) noexcept {
    const auto p=playground::property(weapon,L"CurrentFireMode");
    if(!p||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=1||
       !playground::named(get<uptr>(p+0x50),L"ByteProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),extent=get<unsigned>(get<uptr>(weapon+0x50)+0x88);
    unsigned char mode{};
    return offset<extent&&weapon<UINTPTR_MAX-offset&&copy(&mode,weapon+offset,1)&&mode==1;
}
bool owned_state() noexcept {
    if(!lease.acquired||GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    hand::ObjectIdentity actual{};
    for(const auto& expected:{lease.identity.weapon,lease.identity.pawn,lease.identity.world}){
        if(!nativeequipment::identify(nullptr,expected.address,actual)||!hand::same(actual,expected))return false;
    }
    const auto weapon=lease.identity.weapon.address;
    if(playground::pointer(weapon,L"Instigator")!=lease.identity.pawn.address||
       playground::pointer(lease.identity.pawn.address,L"WorldInfo")!=lease.identity.world.address)return false;
    float started{};bool reloading{};
    return start_time(weapon,started)&&started==lease.original_start&&normal_mode(weapon)&&
        bool_call(weapon,L"IsReloading",reloading)&&reloading&&
        nativeequipment::identify(nullptr,lease.abort_function,actual)&&hand::same(actual,lease.abort_identity)&&
        nativeequipment::identify(nullptr,lease.finish_function,actual)&&hand::same(actual,lease.finish_identity);
}
bool owns(hand::Frame const& source,std::uint64_t serial) noexcept {
    return lease.acquired&&serial==lease.serial&&inv::identity(source)==lease.identity&&
        nativeequipment::current(nullptr,source)&&owned_state()&&nativeequipment::current(nullptr,source);
}
constexpr const wchar_t* ui_fields[]={L"ReloadDuration",L"AR_PossibleSuccessStartPoint",
    L"AR_PreReactionWindowDuration",L"AR_SuperSweetSpotDuration",L"AR_SweetSpotDuration",L"ReloadStartTime"};
bool ui_store(uptr address,const void* data,unsigned bytes) noexcept {
    if(!address||!data||!bytes||bytes>4)return false;
    __try{std::memcpy(reinterpret_cast<void*>(address),data,bytes);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ui_float(uptr weapon,const wchar_t* name,float& out) noexcept {
    const auto p=playground::property(weapon,name),cls=get<uptr>(weapon+0x50);
    if(!p||!cls||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=sizeof(float)||
       !playground::named(get<uptr>(p+0x50),L"FloatProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),extent=get<unsigned>(cls+0x88);
    return offset<=extent&&sizeof(float)<=extent-offset&&weapon<=UINTPTR_MAX-offset-sizeof(float)&&
        copy(&out,weapon+offset,sizeof(out))&&std::isfinite(out);
}
bool ui_hud_current(Lease const& l) noexcept {
    hand::ObjectIdentity actual{};
    return nativeequipment::identify(nullptr,l.ui_controller.address,actual)&&hand::same(actual,l.ui_controller)&&
        nativeequipment::identify(nullptr,l.ui_hud.address,actual)&&hand::same(actual,l.ui_hud)&&
        playground::pointer(l.identity.pawn.address,L"Controller")==l.ui_controller.address&&
        playground::pointer(l.ui_controller.address,L"myHUD")==l.ui_hud.address&&
        playground::pointer(l.ui_hud.address,L"WorldInfo")==l.identity.world.address;
}
bool ui_hud_current() noexcept {return !lease.hud_owned||ui_hud_current(lease);}
bool restore_ui_timing() noexcept {
    if(!lease.ui_owned&&!lease.attempts_owned&&!lease.hud_owned)return true;
    hand::ObjectIdentity actual{};
    if(!nativeequipment::identify(nullptr,lease.identity.weapon.address,actual)||!hand::same(actual,lease.identity.weapon))return false;
    const auto weapon=actual.address;
    for(unsigned i=0;lease.ui_owned&&i<6;++i){
        float value{};
        if(!ui_float(weapon,ui_fields[i],value))return false;
        if(value==lease.ui_before[i]){if(i==5)lease.original_start=value;continue;}
        if(value!=lease.ui_wanted[i]||!playground::put(weapon,ui_fields[i],lease.ui_before[i])||
           !ui_float(weapon,ui_fields[i],value)||value!=lease.ui_before[i])return false;
        if(i==5)lease.original_start=value;
    }
    lease.ui_owned=false;
    if(lease.attempts_owned){
        const auto address=weapon+lease.attempt_offset;
        unsigned value{};
        if(!copy(&value,address,4))return false;
        const unsigned restored=lease.attempt_before?value|lease.attempt_mask:value&~lease.attempt_mask;
        if(!ui_store(address,&restored,4))return false;
        lease.attempts_owned=false;
    }
    if(lease.hud_owned){
        if(!ui_hud_current())return false;
        float tick{};unsigned char result{};
        if(!copy(&tick,lease.ui_hud.address+lease.hud_tick_offset,4)||
           !copy(&result,lease.ui_hud.address+lease.hud_result_offset,1))return false;
        if(tick!=lease.hud_tick_before&&tick!=lease.hud_tick_wanted)return false;
        if(result!=0&&result!=lease.hud_result_before)return false;
        if(!ui_store(lease.ui_hud.address+lease.hud_tick_offset,&lease.hud_tick_before,4)||
           !ui_store(lease.ui_hud.address+lease.hud_result_offset,&lease.hud_result_before,1))return false;
        lease.hud_owned=false;
    }
    if(!lease.ui_final_reported&&log_file){
        lease.ui_final_reported=true;
        fwprintf(log_file,L"lancer_reload_ui_final serial=%llu updates=%llu display_scopes=%llu display_restores=%llu last_draw_elapsed=%.6g fade_duration=%.6g flash_duration=%.6g height_updates=%llu body_height=%.6g marker_scopes=%llu marker_restores=%llu marker_resolution=%.6g canvas_known=%u canvas_observations=%llu canvas=%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g canvas_color=%08x\n",
            lease.serial,lease.ui_draw_updates,lease.ui_display_scopes,lease.ui_display_restores,lease.ui_display_last[2],
            lease.ui_fade_durations[0],lease.ui_fade_durations[1],lease.ui_shape_updates,lease.ui_shape_last,
            lease.ui_marker_scopes,lease.ui_marker_restores,lease.ui_marker_resolution,
            unsigned(lease.ui_canvas_known),lease.ui_canvas_observations,lease.ui_canvas_last[0],lease.ui_canvas_last[1],
            lease.ui_canvas_last[2],lease.ui_canvas_last[3],lease.ui_canvas_last[4],lease.ui_canvas_last[5],lease.ui_canvas_last[6],lease.ui_canvas_color);
        fflush(log_file);
    }
    return true;
}
bool set_ui_timing(hand::Frame const& source,std::uint64_t serial,
    std::int64_t display_ms,std::int64_t perfect_ms,bool replace_timing=true) noexcept {
    ui_setup_gate=1;
    if(!owns(source,serial)||lease.ui_owned||display_ms<100||display_ms>60000||perfect_ms<0||
       (replace_timing&&perfect_ms>display_ms))return false;
    const auto weapon=source.weapon.address;
    ui_setup_gate=2;
    std::array<float,6> saved_values{};
    for(unsigned i=0;i<6;++i)if(!ui_float(weapon,ui_fields[i],saved_values[i]))return false;
    lease.ui_before=saved_values;
    lease.ui_replaced=replace_timing;
    lease.ui_wanted=replace_timing?std::array<float,6>{float(double(display_ms)/1000.),0.f,0.f,float(double(perfect_ms)/1000.),0.f,lease.reward_start}:saved_values;
    // A re-equipped original transaction keeps its original GAME clock in
    // both HUD calculations. Restore the reacquired native timestamp on exit.
    lease.ui_wanted[5]=lease.reward_start;
    lease.ui_owned=true;
    ui_setup_gate=3;
    for(unsigned i=0;i<6;++i){
        float value{};
        if(!nativeequipment::current(nullptr,source)||!playground::put(weapon,ui_fields[i],lease.ui_wanted[i])||
           !ui_float(weapon,ui_fields[i],value)||value!=lease.ui_wanted[i])return false;
        if(i==5)lease.original_start=value;
    }
    // Verify what the original HUD actually queries, not just our setters.
    // This synchronous getter is outside call-boundary/animation callbacks.
    const auto window=active_window(weapon);
    ui_setup_gate=4;
    if(!owns(source,serial))return false;
    // Retail's explicit super-width branch substitutes a fixed0.125s rather
    // than returning this field. The scoped HUD CallFunction adapter supplies
    // the actual four drawing locals after the original getter returns. Keep
    // its original values as evidence; do not claim the setter changed it.
    if(log_file){fwprintf(log_file,L"lancer_reload_ui_original_getter serial=%llu actual=%.6g,%.6g,%.6g,%.6g wanted=%.6g\n",
        serial,window.start,window.pre_reaction,window.perfect_duration,window.ordinary_duration,lease.ui_wanted[3]);fflush(log_file);}
    // The saved Reloading.HandleButtonPress enters the original success/fumble
    // function only when bActiveReloadAttempted is false. Reserve that edge
    // for this physical transaction; its result is awarded only at closure.
    const auto attempt=playground::property(weapon,L"bActiveReloadAttempted"),cls=get<uptr>(weapon+0x50);
    ui_setup_gate=5;
    if(!attempt||!playground::named(get<uptr>(attempt+0x50),L"BoolProperty")||
       get<unsigned>(attempt+0x70)!=1||get<unsigned>(attempt+0x74)!=4)return false;
    const auto offset=get<unsigned>(attempt+0x94),extent=get<unsigned>(cls+0x88),mask=get<unsigned>(attempt+0xd0);
    unsigned bits{};
    if(!mask||(mask&(mask-1))||offset>extent||4>extent-offset||!copy(&bits,weapon+offset,4))return false;
    lease.attempt_offset=offset;lease.attempt_mask=mask;lease.attempt_before=(bits&mask)!=0;lease.attempts_owned=true;
    bits|=mask;if(!ui_store(weapon+offset,&bits,4))return false;
    ui_setup_gate=6;
    // The native attempted path stops calculating its marker. Own only its
    // two reflected HUD values while retaining the original drawing function.
    const auto hud=playground::pointer(source.controller.address,L"myHUD");
    if(!nativeequipment::identify(nullptr,source.controller.address,lease.ui_controller)||
       !nativeequipment::identify(nullptr,hud,lease.ui_hud)||!playground::instance(hud,L"WarHUD"))return false;
    ui_setup_gate=7;
    const auto bar=playground::property(hud,L"ARBarData");
    if(!bar||!playground::named(get<uptr>(bar+0x50),L"StructProperty"))return false;
    const auto structure=get<uptr>(bar+0xd0);const auto bytes=get<unsigned>(bar+0x74),start=get<unsigned>(bar+0x94);
    // Retail reports157 bytes of fields and a160-byte four-aligned property.
    // Validate the typed extent and its exact tail padding, not byte equality.
    const auto structure_extent=get<unsigned>(structure+0x88);
    if(!registered(structure)||!playground::named(structure,L"ActionReloadBarData")||
       !structure_extent||structure_extent>4096||bytes!=((structure_extent+3u)&~3u))return false;
    ui_setup_gate=8;
    const auto hud_extent=get<unsigned>(get<uptr>(hud+0x50)+0x88);
    if(start>hud_extent||bytes>hud_extent-start||hud>UINTPTR_MAX-hud_extent)return false;
    bool tick_found{},result_found{},fade_found{},flash_found{};unsigned fields_seen{};
    ui_setup_gate=9;
    for(auto field=get<uptr>(structure+0x80);field;field=get<uptr>(field+0x68)){
        if(++fields_seen>64||!registered(field)||get<uptr>(field+0x40)!=structure)return false;
        const auto at=get<unsigned>(field+0x94),size=get<unsigned>(field+0x74);
        if(at>structure_extent||size>structure_extent-at)return false;
        if(playground::named(field,L"TickMarkTime")&&size==4&&playground::named(get<uptr>(field+0x50),L"FloatProperty")){
            lease.hud_tick_offset=start+at;tick_found=true;
        }
        if(playground::named(field,L"ARResult")&&size==1&&playground::named(get<uptr>(field+0x50),L"ByteProperty")){
            lease.hud_result_offset=start+at;result_found=true;
        }
        if(playground::named(field,L"FadeDuration")&&size==4&&playground::named(get<uptr>(field+0x50),L"FloatProperty")){
            lease.hud_fade_offset=start+at;fade_found=true;
        }
        if(playground::named(field,L"FlashDuration")&&size==4&&playground::named(get<uptr>(field+0x50),L"FloatProperty")){
            lease.hud_flash_offset=start+at;flash_found=true;
        }
    }
    if(!tick_found||!result_found||!fade_found||!flash_found||!copy(&lease.hud_tick_before,hud+lease.hud_tick_offset,4)||
       !copy(&lease.hud_result_before,hud+lease.hud_result_offset,1)||!std::isfinite(lease.hud_tick_before))return false;
    ui_setup_gate=10;
    lease.hud_tick_wanted=lease.hud_tick_before;lease.hud_owned=true;
    if(!ui_hud_current())return false;
    ui_setup_gate=11;
    if(replace_timing&&!lancerreload_ui::prepare(source))return false;
    ui_setup_gate=12;
    if(log_file){fwprintf(log_file,L"lancer_reload_ui serial=%llu duration_ms=%lld perfect_ms=%lld configured=%u draw_adapter_prepared=%u legacy_attempt_blocked=1\n",
        serial,std::int64_t(std::lround(double(lease.ui_wanted[0])*1000.)),perfect_ms,unsigned(replace_timing),unsigned(replace_timing));fflush(log_file);}
    return true;
}
bool tick_ui_timing() noexcept {
    if(!lease.hud_owned)return true;
    if(!owned_state()||!ui_hud_current())return false;
    // CallFunction callbacks only record POD receipts; I/O stays on this
    // ordinary GT path, outside the original native invocation.
    if(log_file&&lease.ui_draw_gate&&lease.ui_reported_gate!=lease.ui_draw_gate){
        lease.ui_reported_gate=lease.ui_draw_gate;
        fwprintf(log_file,L"lancer_reload_ui_draw_gate serial=%llu gate=%u updates=%llu display_scopes=%llu display_late=%llu display_restores=%llu real_start=%.6g draw_start=%.6g elapsed=%.6g marker=%.6g values=%.6g,%.6g,%.6g,%.6g layout_updates=%llu x=%.6g y=%.6g scale=%.6g width=%.6g height=%.6g body_observations=%llu body_x=%.6g body_y=%.6g body_scale=%.6g body_bar_length=%.6g body_fade=%.6g body_flash=%.6g body_alpha=%.6g\n",
            lease.serial,lease.ui_draw_gate,lease.ui_draw_updates,lease.ui_display_scopes,lease.ui_display_late,lease.ui_display_restores,
            lease.ui_display_last[0],lease.ui_display_last[1],lease.ui_display_last[2],lease.ui_display_last[3],
            lease.ui_draw_current[0],lease.ui_draw_current[1],lease.ui_draw_current[2],lease.ui_draw_current[3],
            lease.ui_layout_updates,lease.ui_layout_last[0],lease.ui_layout_last[1],lease.ui_layout_last[2],lease.ui_layout_last[3],lease.ui_layout_last[4],
            lease.ui_body_observations,lease.ui_body_last[0],lease.ui_body_last[1],lease.ui_body_last[2],lease.ui_body_last[3],lease.ui_body_last[4],lease.ui_body_last[5],lease.ui_body_last[6]);fflush(log_file);
    }
    if(lease.ui_draw_updates&&!lease.ui_draw_reported&&log_file){
        lease.ui_draw_reported=true;const auto& a=lease.ui_draw_current;const auto& b=lease.ui_draw_original;
        fwprintf(log_file,L"lancer_reload_ui_draw serial=%llu values=%.6g,%.6g,%.6g,%.6g original=%.6g,%.6g,%.6g,%.6g caller_locals=1\n",
            lease.serial,a[0],a[1],a[2],a[3],b[0],b[1],b[2],b[3]);fflush(log_file);
    }
    float now{};
    if(!time_field(lease.identity.world.address,L"TimeSeconds",now)||now<lease.reward_start)return false;
    const float elapsed=now-lease.reward_start;
    const float marker=(std::min)(elapsed,lease.ui_wanted[0]);
    const unsigned char no_failure=0;
    if(!ui_store(lease.ui_hud.address+lease.hud_tick_offset,&marker,4)||
       !ui_store(lease.ui_hud.address+lease.hud_result_offset,&no_failure,1))return false;
    lease.hud_tick_wanted=marker;return true;
}
enum class BeginResult { rejected,acquired,recovery_required };
bool hand_owned(hand::Frame const& source,hand::LeaseKind kind,bool allow_ammo=false) noexcept {
    hand::Frame current{};hand::GripGameState game{};hand::Decision decision{};
    return weaponhand::client.copy_frame_on_gt(current,game,decision)&&
        hand::same_frame(source,current)&&game.known&&game.main_hand_held&&
        !game.busy&&!game.animation_owns_hands&&
        (decision.status==hand::DecisionStatus::granted||decision.status==hand::DecisionStatus::retained)&&
        (decision.lease.kind==kind||(allow_ammo&&decision.lease.kind==hand::LeaseKind::ammo))&&
        hand::lease_valid(decision.lease,source.key.lifecycle)&&decision.lease.id&&
        !decision.release_required&&source.left_tracked&&source.left_grip_down;
}
bool magazine_owned(hand::Frame const& source) noexcept {return hand_owned(source,hand::LeaseKind::magazine);}
bool item_owned(hand::Frame const& source) noexcept {
    return hand_owned(source,hand::LeaseKind::magazine,true);
}
// Physical Lancer reload owns the item, not the timed pawn/weapon animation.
// The original pawn reload emits AmmoRelease (spawns a SECOND magazine),
// AmmoGrab and AmmoReload after ForceReload. Clearing the inventory timer does
// not stop those notifies. Stop only this weapon's exact authored reload stance
// and its weapon animation, through the same original calls used by Abort.
// Keep Reloading, its start time and the active-reload window for our ledger;
// do not call Abort/PerformReload here or touch unrelated pawn stances.
bool stop_lancer_automatic_animation() noexcept {
    if(!owned_state())return false;
    const auto weapon=lease.identity.weapon.address,pawn=lease.identity.pawn.address;
    const bool lancer=playground::instance(weapon,L"Weap_AssaultRifle");
    const bool snub=playground::instance(weapon,L"Weap_COGPistol");
    if(!lancer&&!snub)return true;
    playground::Args stop_weapon(weapon,L"StopWeaponAnim"),stop_body(pawn,L"StopBodyStance");
    const auto original_call=[](playground::Args const& call,const wchar_t* owner_name) noexcept {
        if(!call.valid||!registered(call.r.function))return false;
        const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
        return registered(owner)&&registered(package)&&playground::named(owner,owner_name)&&
            playground::named(package,L"WarfareGame")&&get<uptr>(package+0x40)==0;
    };
    if(!original_call(stop_weapon,L"WarWeapon")||stop_weapon.r.size!=sizeof(float)||
       !original_call(stop_body,L"WarPawn"))return false;
    // Retail reflection exposes optional BlendOutTime as a real four-byte
    // input parameter. The historical script export mislabeled it a local;
    // requiring a zero-byte call rejected every accepted physical reload.
    const auto weapon_blend=stop_weapon.prop(L"BlendOutTime");
    if(!registered(weapon_blend)||get<uptr>(weapon_blend+0x40)!=stop_weapon.r.function||
       !playground::named(get<uptr>(weapon_blend+0x50),L"FloatProperty")||
       get<unsigned>(weapon_blend+0x70)!=1||get<unsigned>(weapon_blend+0x74)!=sizeof(float)||
       get<unsigned>(weapon_blend+0x94)!=0||
       (get<unsigned long long>(weapon_blend+0x78)&0x580)!=0x80||
       !stop_weapon.set(L"BlendOutTime",0.f).valid)return false;
    const auto parameter=stop_body.prop(L"Stance"),blend=stop_body.prop(L"BlendOutTime");
    const auto property=playground::property(weapon,L"BS_PawnWeaponReload");
    if(!registered(parameter)||!registered(blend)||!registered(property)||
       get<uptr>(parameter+0x40)!=stop_body.r.function||get<uptr>(blend+0x40)!=stop_body.r.function||
       !playground::named(get<uptr>(parameter+0x50),L"StructProperty")||
       !playground::named(get<uptr>(property+0x50),L"StructProperty")||
       !playground::named(get<uptr>(blend+0x50),L"FloatProperty")||
       get<unsigned>(parameter+0x70)!=1||get<unsigned>(property+0x70)!=1||get<unsigned>(blend+0x70)!=1||
       (get<unsigned long long>(parameter+0x78)&0x580)!=0x80||
       (get<unsigned long long>(blend+0x78)&0x580)!=0x80||get<unsigned>(blend+0x74)!=4)return false;
    const auto structure=get<uptr>(property+0xd0);
    const auto bytes=get<unsigned>(property+0x74);
    const auto offset=get<unsigned>(property+0x94),extent=get<unsigned>(get<uptr>(weapon+0x50)+0x88);
    const auto parameter_offset=get<unsigned>(parameter+0x94),blend_offset=get<unsigned>(blend+0x94);
    if(!registered(structure)||!playground::named(structure,L"BodyStance")||
       get<uptr>(parameter+0xd0)!=structure||get<unsigned>(parameter+0x74)!=bytes||
       get<unsigned>(structure+0x88)!=bytes||bytes<sizeof(Array)||bytes>256||
       offset>extent||bytes>extent-offset||weapon>UINTPTR_MAX-offset-bytes||
       parameter_offset>stop_body.r.size||bytes>stop_body.r.size-parameter_offset||
       blend_offset>stop_body.r.size||4>stop_body.r.size-blend_offset||
       !(parameter_offset+bytes<=blend_offset||blend_offset+4<=parameter_offset))return false;
    std::array<unsigned char,256> stance{},again{};
    if(!copy(stance.data(),weapon+offset,bytes))return false;
    // StopBodyStance reads the AnimName array. Authenticate its reflected
    // layout and bounded backing storage before passing the original value.
    const auto names=playground::field(structure,L"AnimName",L"ArrayProperty");
    if(!registered(names)||get<unsigned>(names+0x70)!=1||get<unsigned>(names+0x74)!=sizeof(Array))return false;
    const auto names_offset=get<unsigned>(names+0x94);
    const auto inner=get<uptr>(names+0xd0);
    if(names_offset>bytes||sizeof(Array)>bytes-names_offset||!registered(inner)||
       !playground::named(get<uptr>(inner+0x50),L"NameProperty")||
       get<unsigned>(inner+0x70)!=1||get<unsigned>(inner+0x74)!=sizeof(Name))return false;
    Array values{};std::memcpy(&values,stance.data()+names_offset,sizeof(values));
    std::array<Name,32> names_before{},names_after{};
    if(values.count<=0||values.count>int(names_before.size())||values.capacity<values.count||
       values.capacity>256||!values.data||
       !copy(names_before.data(),values.data,std::size_t(values.count)*sizeof(Name))||
       !copy(again.data(),weapon+offset,bytes)||std::memcmp(stance.data(),again.data(),bytes)||
       !copy(names_after.data(),values.data,std::size_t(values.count)*sizeof(Name))||
       std::memcmp(names_before.data(),names_after.data(),std::size_t(values.count)*sizeof(Name))||
       !stop_body.bytes(L"Stance",stance.data(),bytes)||!stop_body.set(L"BlendOutTime",0.f).valid||
       !owned_state())return false;
    // This synchronous stop belongs to the accepted reload action. Any partial
    // failure retains lease so existing original Abort recovery drains it.
    // Snub's original gun-only AR_Reload must remain selected for its single
    // animation owner to stop/seek; suppress only the pawn's automatic stance.
    // Lancer keeps its established independent charging-clip path unchanged.
    if((lancer&&!stop_weapon.run())||!owned_state()||!stop_body.run()||!owned_state())return false;
    if(log_file){fwprintf(log_file,L"lancer_reload_automatic_animation_stopped serial=%llu\n",lease.serial);fflush(log_file);}
    return true;
}
BeginResult begin(hand::Frame const& source,std::uint64_t& serial,Lease const* suspended=nullptr) noexcept {
    serial=0;
    if(lease.acquired||next_serial==UINT64_MAX||!nativeequipment::current(nullptr,source)||
       (suspended?(!(suspended->identity==inv::identity(source))&&
                    !inv::same_instance_across_hold(suspended->identity,inv::identity(source))):
                   !magazine_owned(source)))return BeginResult::rejected;
    // Only this authenticated physical intent may pass CanReload. Empty-shot
    // Active.BeginFire/ShouldAutoReload must not start a timed vanilla refill.
    // The scope covers our preflight AND the nested original ForceReload;
    // every early return/unwind closes it before the next game action.
    struct ManualStart {
        bool held{};~ManualStart(){if(held)lancerreload_manual::leave();}
    } manual{lancerreload_manual::requires_permission(source)&&lancerreload_manual::enter(source)};
    if(lancerreload_manual::requires_permission(source)&&!manual.held)return BeginResult::rejected;
    inv::Counts before{};bool can{},already{};
    if(!inv::read(source,before)||!before.used||(!before.infinite&&!before.reserve)||
       !inv::setters_available(source)||
       !bool_call(source.weapon.address,L"CanReload",can)||!can||
       !bool_call(source.weapon.address,L"IsReloading",already)||already)return BeginResult::rejected;
    const auto abort=state_function(source.weapon.address,L"AbortWeaponReload");
    const auto finish=state_function(source.weapon.address,L"EndOfReloadTimer");
    if(!abort||!finish)return BeginResult::rejected;
    const auto identity=inv::identity(source);
    playground::Args start(source.weapon.address,L"ForceReload");
    if(!start.valid||start.r.size!=0||!nativeequipment::current(nullptr,source))return BeginResult::rejected;
    const auto call_begin=weaponhand::clock_ns(nullptr);
    if(call_begin<=0||!start.run())return BeginResult::rejected;
    const auto call_end=weaponhand::clock_ns(nullptr);
    float started{};
    if(!start_time(source.weapon.address,started))return BeginResult::recovery_required;
    lease={identity,next_serial++,abort,finish,started,get<Name>(finish+0x48),true};
    lease.reward_start=started;
    serial=lease.serial;
    if(call_end<call_begin)return BeginResult::recovery_required;
    lease.accepted_begin_ms=call_begin/1000000;
    lease.accepted_end_ms=call_end/1000000;
    if(!nativeequipment::identify(nullptr,abort,lease.abort_identity)||
       !nativeequipment::identify(nullptr,finish,lease.finish_identity))return BeginResult::recovery_required;
    inv::Counts after{};
    // ForceReload may retire its input frame. The accepted action now owns
    // state/timer cleanup; verify the exact objects and original reload instead.
    if(!owned_state()||!inv::query(source.weapon.address,after)||!(before==after)||
       !clear_timer(source.weapon.address,lease.timer))return BeginResult::recovery_required;
    if(!stop_lancer_automatic_animation())return BeginResult::recovery_required;
    lease.window=active_window(source.weapon.address);
    if(lease.window.valid){
        const double end_ms=(double(lease.window.start)+double(lease.window.perfect_duration))*1000.0;
        if(end_ms>=1.0&&end_ms<double(INT64_MAX))lease.perfect_end_ms=std::int64_t(std::floor(end_ms));
    }
    // An unavailable window means ordinary reload, never invented perfection.
    if(!owned_state())return BeginResult::recovery_required;
    if(suspended){
        lease.serial=suspended->serial;serial=lease.serial;
        lease.reward_start=suspended->reward_start;
        lease.accepted_begin_ms=suspended->accepted_begin_ms;
        lease.perfect_end_ms=suspended->perfect_end_ms;
    }
    return BeginResult::acquired;
}
// The saved Reloading.AbortWeaponReload calls global Abort then Active. The
// global implementation fills ONLY in fire modes 10/11. Admit mode 1 alone;
// never call EndOfReloadTimer/PerformReload for an already committed insert.
// Perfect reward needs its separately authenticated adapter, not a fake receipt.
unsigned exit_failure{};
bool exit_without_fill(hand::Frame const& source,std::uint64_t serial) noexcept {
    exit_failure=1;if(!owns(source,serial))return false;
    inv::Counts before{},after{};
    exit_failure=2;if(!inv::read(source,before))return false;
    exit_failure=3;if(!clear_timer(source.weapon.address,lease.timer))return false;
    exit_failure=4;if(!owns(source,serial)||!restore_ui_timing())return false;
    Request call{};call.object=source.weapon.address;call.function=lease.abort_function;
    exit_failure=5;
    if(!invoke(&call))return false;
    bool reloading{},timer_running{};
    // Original state exit can invalidate the hand frame. Authenticate the
    // saved objects/counts, not a tracking authorization that just ended.
    hand::ObjectIdentity current_weapon{},current_pawn{};
    exit_failure=6;
    if(!nativeequipment::identify(nullptr,source.weapon.address,current_weapon)||!hand::same(current_weapon,source.weapon)||
       !nativeequipment::identify(nullptr,source.pawn.address,current_pawn)||!hand::same(current_pawn,source.pawn)||
       playground::pointer(source.weapon.address,L"Instigator")!=source.pawn.address||
       !inv::query(source.weapon.address,after)||!(before==after)||
       !bool_call(source.weapon.address,L"IsReloading",reloading)||reloading||
       !timer_active(source.weapon.address,lease.timer,timer_running)||timer_running)return false;
    lease={};exit_failure=0;return true;
}
// Tracking loss must not leave a weapon waiting forever for a hand gesture.
// Cleanup uses only the original object/reload ownership, never a stale input
// frame. A changed/recycled object is not touched; caller retains unresolved
// ownership until the lifecycle coordinator can retire it safely.
bool recover_owned() noexcept {
    if(!lease.acquired)return true;
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    hand::ObjectIdentity weapon{},pawn{},world{};
    if(!nativeequipment::identify(nullptr,lease.identity.weapon.address,weapon)||
       !hand::same(weapon,lease.identity.weapon)||
       !nativeequipment::identify(nullptr,lease.identity.pawn.address,pawn)||!hand::same(pawn,lease.identity.pawn)||
       !nativeequipment::identify(nullptr,lease.identity.world.address,world)||!hand::same(world,lease.identity.world)||
       playground::pointer(weapon.address,L"Instigator")!=pawn.address)return false;
    bool reloading{};
    if(!bool_call(weapon.address,L"IsReloading",reloading))return false;
    if(!reloading){
        // Do not retire ownership while the old automatic completion callback
        // can still run and refill a second time.
        bool active{};
        if(!timer_active(weapon.address,lease.timer,active)||active)return false;
        if(!restore_ui_timing())return false;
        lease={};return true;
    }
    float started{};
    if(!start_time(weapon.address,started)||started!=lease.original_start||!normal_mode(weapon.address)||
       state_function(weapon.address,L"AbortWeaponReload")!=lease.abort_function)return false;
    inv::Counts before{},after{};
    if(!inv::query(weapon.address,before)||!clear_timer(weapon.address,lease.timer)||!restore_ui_timing())return false;
    Request call{};call.object=weapon.address;call.function=lease.abort_function;
    if(!invoke(&call)||!inv::query(weapon.address,after)||!(before==after)||
       !bool_call(weapon.address,L"IsReloading",reloading)||reloading)return false;
    bool active{};
    if(!timer_active(weapon.address,lease.timer,active)||active)return false;
    lease={};return true;
}

struct RewardReceipt {
    bool dispatched{},applied{},call_confirmed{};
    int observed_bonus{},final_bonus{};
    inv::Identity eligibility_owner{};
    bool eligibility_known{},eligible{},qualification_applied{},failed{},bonus_observed{};
};
bool capture_reward_eligibility(inv::Inventory const& opening,RewardReceipt& receipt) noexcept {
    if(receipt.eligibility_known)return receipt.eligibility_owner==opening.identity;
    // opening was just authenticated by observer.sample on this same GT.
    // Bracket this read-only qualification query with typed raw counters and
    // identities, rather than four additional magazine-size/infinite getters.
    // This is no inventory/hand action permit; all actual writes retain their
    // existing complete inventory and current-source checks.
    const auto unchanged=[&]() noexcept {
        const auto& id=opening.identity;hand::ObjectIdentity actual{};int used{},reserve{};
        if(!opening.valid())return false;
        for(const auto& o:{id.world,id.pawn,id.weapon})
            if(!nativeequipment::identify(nullptr,o.address,actual)||!hand::same(o,actual))return false;
        return playground::pointer(id.weapon.address,L"Instigator")==id.pawn.address&&
            playground::pointer(id.pawn.address,L"Weapon")==id.weapon.address&&
            playground::pointer(id.pawn.address,L"WorldInfo")==id.world.address&&
            inv::integer(id.weapon.address,L"AmmoUsedCount",used)&&used>=0&&unsigned(used)==opening.capacity-opening.loaded&&
            inv::integer(id.weapon.address,L"SpareAmmoCount",reserve)&&reserve>=0&&unsigned(reserve)==opening.reserve;
    };
    bool eligible{};
    if(!unchanged()||!bool_call(opening.identity.weapon.address,L"IsApplicableForARDamageBonus",eligible)||!unchanged())return false;
    receipt.eligibility_owner=opening.identity;receipt.eligible=eligible;receipt.eligibility_known=true;return true;
}
bool restore_partial_insert(inv::Request const& request,inv::ObservedInventory& observer) noexcept {
    if(request.call!=gears_weapon::ReloadTxnCall::insert)return true;
    inv::Inventory counts{};
    if(!observer.sample_owned(request.identity,counts))return false;
    if(inv::counts_equal(counts,request.before)||inv::counts_equal(counts,request.expected))return true;
    auto partial=request.before;partial.reserve=request.expected.reserve;
    if(request.before.infinite_reserve||!inv::counts_equal(counts,partial)||
       !lease.acquired||lease.serial!=request.reload_serial||!(lease.identity==request.identity)||
       !owned_state())return false;
    // Only the exact reserve-first half-write may be compensated, while the
    // original reload still belongs to us. Never refill or rewrite used ammo.
    playground::Args restore(request.identity.weapon.address,L"SetSpareAmmoCount");
    if(!inv::setter(request.identity.weapon.address,L"SetSpareAmmoCount",restore))return false;
    const bool invoked=restore.set(L"NewValue",int(request.before.reserve)).run();
    return invoked&&observer.sample_owned(request.identity,counts)&&inv::counts_equal(counts,request.before);
}
bool input_parameter(playground::Args& call,const wchar_t* name,const wchar_t* type,unsigned size) noexcept {
    const auto p=call.prop(name);
    return call.valid&&call.r.size==size&&p&&get<unsigned>(p+0x70)==1&&
        get<unsigned>(p+0x74)==size&&get<unsigned>(p+0x94)==0&&
        playground::named(get<uptr>(p+0x50),type)&&
        (get<unsigned long long>(p+0x78)&0x580)==0x80;
}
bool byte_value(uptr object,const wchar_t* name,unsigned char& value) noexcept {
    const auto p=playground::property(object,name);
    if(!p||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=1||
       !playground::named(get<uptr>(p+0x50),L"ByteProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),extent=get<unsigned>(get<uptr>(object+0x50)+0x88);
    return offset<extent&&object<UINTPTR_MAX-offset&&copy(&value,object+offset,1);
}
// Called only after the physical closure, committed inventory and original
// non-refilling exit. LocalActiveReloadSuccess supplies the original pawn/HUD,
// score, feedback, tier and bonus timers. Its fire mode must be handed back.
bool perfect_reward(inv::Request const& request,inv::Inventory const& opening,
    inv::ObservedInventory& observer,RewardReceipt& receipt) noexcept {
    if(receipt.applied)return true;
    if(receipt.failed)return false;
    if(receipt.dispatched){
        // Never redispatch score/feedback. A successful original call can
        // finish its mode hand-back and exact bonus correction on a later GT
        // even if the first post-call observation was temporarily unavailable.
        if(!receipt.call_confirmed)return false;
        inv::Inventory counts{};unsigned char role{},mode{};bool reloading{};
        const auto weapon=request.identity.weapon.address;
        if(!observer.sample_owned(request.identity,counts)||!inv::counts_equal(counts,request.expected)||
           !byte_value(weapon,L"Role",role)||role!=3||
           !bool_call(weapon,L"IsReloading",reloading)||reloading||
           !byte_value(weapon,L"CurrentFireMode",mode)||(mode!=0&&mode!=11))return false;
        if(mode==11){
            playground::Args reset(weapon,L"SetCurrentFireMode");
            if(!input_parameter(reset,L"FiringModeNum",L"ByteProperty",1)||
               !reset.set(L"FiringModeNum",static_cast<unsigned char>(0)).run())return false;
        }
        int bonus{};
        if(!inv::integer(weapon,L"ActiveReload_NumBonusShots",bonus))return false;
        if(!receipt.bonus_observed){receipt.observed_bonus=bonus;receipt.bonus_observed=true;}
        if(
           (bonus!=receipt.observed_bonus&&bonus!=receipt.final_bonus))return false;
        if(bonus!=receipt.final_bonus&&!playground::put(weapon,L"ActiveReload_NumBonusShots",receipt.final_bonus))return false;
        if(!observer.sample_owned(request.identity,counts)||!inv::counts_equal(counts,request.expected)||
           !byte_value(weapon,L"CurrentFireMode",mode)||mode!=0||
           !inv::integer(weapon,L"ActiveReload_NumBonusShots",bonus)||bonus!=receipt.final_bonus)return false;
        receipt.applied=true;return true;
    }
    if(request.result!=gears_weapon::ReloadTxnResult::perfect||
       !(opening.identity==request.identity)||!receipt.eligibility_known||
       !inv::same_instance_across_hold(receipt.eligibility_owner,opening.identity))return false;
    inv::Inventory current_counts{};
    const auto weapon=request.identity.weapon.address;
    unsigned char role{},mode{};bool reloading{};
    if(!observer.sample_owned(request.identity,current_counts)||
       !inv::counts_equal(current_counts,request.expected)||
       !byte_value(weapon,L"Role",role)||role!=3||
       !byte_value(weapon,L"CurrentFireMode",mode)||mode!=0||
       !bool_call(weapon,L"IsReloading",reloading)||reloading)return false;
    playground::Args success(weapon,L"LocalActiveReloadSuccess");
    playground::Args reset(weapon,L"SetCurrentFireMode");
    if(!input_parameter(success,L"bDidSuperSweetReload",L"BoolProperty",4)||
       !input_parameter(reset,L"FiringModeNum",L"ByteProperty",1))return false;
    const auto mask=get<unsigned>(success.prop(L"bDidSuperSweetReload")+0xd0);
    if(!mask||(mask&(mask-1)))return false;
    int bonus_before{};
    if(!inv::integer(weapon,L"ActiveReload_NumBonusShots",bonus_before)||bonus_before<0)return false;
    // Reward exactly the newly supplied rounds of this authenticated opening
    // inventory. The original call supplies tier, timers, sound and visuals;
    // its arithmetic now sees committed ammo, so only its bonus counter is
    // reconciled afterward. Never temporarily rewind real ammunition.
    if(request.expected.loaded<opening.loaded)return false;
    const auto original_add=request.expected.loaded-opening.loaded;
    if(original_add>unsigned(INT_MAX-bonus_before))return false;
    receipt.final_bonus=bonus_before+(receipt.eligible?int(original_add):0);
    const bool invoked=lancerreload_reward::invoke(request.identity,receipt.eligible,success,
        receipt.qualification_applied,receipt.dispatched);
    receipt.call_confirmed=invoked;
    // A rejected preflight made no original call and cannot have entered its
    // temporary fire mode. Do not issue an unrelated reset in that case.
    if(!receipt.dispatched)return false;
    // Even an uncertain success must not leave mode 11 active on this gun.
    const bool reset_ok=reset.set(L"FiringModeNum",static_cast<unsigned char>(0)).run();
    if(invoked&&!receipt.qualification_applied){receipt.failed=true;return false;}
    int bonus_after{};
    if(!invoked||!reset_ok||!observer.sample_owned(request.identity,current_counts)||
       !inv::counts_equal(current_counts,request.expected)||
       !byte_value(weapon,L"CurrentFireMode",mode)||mode!=0||
       !bool_call(weapon,L"IsReloading",reloading)||reloading||
       !inv::integer(weapon,L"ActiveReload_NumBonusShots",bonus_after))return false;
    receipt.observed_bonus=bonus_after;receipt.bonus_observed=true;
    const int expected_bonus=receipt.final_bonus;
    if(expected_bonus!=bonus_after&&
       !playground::put(weapon,L"ActiveReload_NumBonusShots",expected_bonus))return false;
    if(!inv::integer(weapon,L"ActiveReload_NumBonusShots",bonus_after)||bonus_after!=expected_bonus)return false;
    receipt.applied=true;return true;
}
}
