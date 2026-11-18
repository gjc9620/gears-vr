// Normal-GT original B ownership. No arena dependency, secondary input writer,
// synthetic melee target or damage setter. Native callbacks only revoke; they
// may never enter these original helpers or borrow their restoration journal.
namespace lancersaw_original {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
namespace policy=gears_lancer_saw_input;
policy::Button button;
policy::ReadyStop ready_stop;
policy::HoldInput hold_input;
hand::Frame source_parent{}; // Association only; never refreshed or used as current tracking.
hand::ExistingActionLifetime action_lifetime{};
std::atomic<bool> stop_requested{},outstanding{};
gears_equipment_names::Lookup names({L"B",L"CheckForMeleeTarget",L"B"});
std::array<unsigned,3> indices{};
bool names_ready{},mutating{},stop_pending{},seen_running{},timer_clear_confirmed{},ready_transition_seen{},ready_notice_pending{};
struct ReadySchema {hand::ObjectIdentity function{},move{},enumeration{};unsigned offset{},bytes{};};
ReadySchema ready_schema{};
std::int64_t started_ns{},last_source_ns{};
std::uint64_t source_session{},source_input{},source_space{},source_reference{},source_hold{},previous_sequence{};
std::uint32_t right_press{};
unsigned last_status{~0u},last_report_failure{~0u},status_rows{},failure{};
// Existing stop rules, now distinguished for the first real motor handoff.
// Bits:1 context,2 source age,4 startup deadline,8 hold lost,16 hold generation,
//32 original context-change notice,64 actor association,128 source cohort,256 release-tail expiry.
unsigned stop_reasons{},notice_rows{};std::int64_t observed_source_age_ns{};
// Only refine the existing bounded stop receipt. These values grant no
// tracking fallback, retry, input lease or extra runtime poll.
unsigned hold_source_gate{},input_pin_gate{};std::uint32_t hold_lifetime_failure{};
gears_current_tracking::Failure input_pin_failure{};
std::int64_t input_pin_elapsed_ns{};
// Freeze the first service failure BEFORE release/ready-stop calls can cause
// another invalidation. This is diagnostic POD only, never a source permit.
bool first_hold_failure_known{};
unsigned first_hold_source_gate{},first_input_pin_gate{};
std::uint32_t first_hold_lifetime_failure{},first_hand_disarm_reason{};
gears_current_tracking::Failure first_input_pin_failure{};
policy::Observation before_service{};

policy::Actors actors(hand::Frame const& f) noexcept {return {f.world,f.controller,f.pawn,f.weapon};}
bool active() noexcept {return button.owed()||ready_stop.owed();}
bool starter_continuation(gears_weapon_call::Invocation const& call) noexcept {
    // Original B dispatch is asynchronous: the following ordinary Tick asks
    // CanDoSpecialMeleeAttack again. Continue ONLY the already journalled,
    // physically acquired hold. This read creates no new press, pose, actor
    // association, target, deadline or tracking sample.
    const auto now=weaponhand::clock_ns(nullptr);
    return button.owed()&&!ready_stop.owed()&&!stop_pending&&!stop_requested.load(std::memory_order_acquire)&&
        call.self==button.owner().weapon.address&&call.hand_cookie==source_parent.key.registration_cookie&&
        call.native_started_qpc==source_parent.key.native_started_qpc&&call.thread_id==GetCurrentThreadId()&&
        last_source_ns>0&&now>=last_source_ns&&now-last_source_ns<=100000000&&
        started_ns>0&&now>=started_ns&&(seen_running||now-started_ns<=700000000);
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    return (button.owed()&&hand::same(button.owner().weapon,weapon))||
        (ready_stop.owed()&&hand::same(ready_stop.owner().weapon,weapon));
}
template<std::size_t N>bool name_current(Name const& value,const wchar_t (&label)[N]) noexcept {
    meta::Table objects{},table{};meta::NameText text{};
    return !value.number&&meta::tables(base,objects,table)&&meta::fresh_name(table,value.index,text)&&
        meta::exact_text(text,label);
}
bool prepared() noexcept {return names_ready;}
bool prepare_ready_schema(hand::Frame const& f) noexcept {
    ready_schema={};playground::Args call(f.pawn.address,L"DoSpecialMove");
    const auto move=call.prop(L"NewMove"),force=call.prop(L"bForce");
    if(!lancerreload_ui::original_function(call.r.function,L"DoSpecialMove",L"WarPawn",false)||
       call.r.size>64||!hammerburst_animation::input_field(call,move,L"ByteProperty",1)||
       !hammerburst_animation::input_field(call,force,L"BoolProperty",4))return false;
    const auto mo=get<unsigned>(move+0x94),fo=get<unsigned>(force+0x94);
    const auto enumeration=get<uptr>(move+0xd0);
    if(call.r.size!=(std::max)(mo+1,fo+4)||!(mo+1<=fo||fo+4<=mo)||
       !registered(enumeration)||!playground::named(enumeration,L"ESpecialMove")||
       !playground::named(get<uptr>(enumeration+0x50),L"Enum")||get<uptr>(enumeration+0x40)!=get<uptr>(call.r.function+0x40))return false;
    ReadySchema schema{};schema.offset=mo;schema.bytes=call.r.size;
    if(!nativeequipment::identify(nullptr,call.r.function,schema.function)||
       !nativeequipment::identify(nullptr,move,schema.move)||!nativeequipment::identify(nullptr,enumeration,schema.enumeration))return false;
    ready_schema=schema;return true;
}
void prepare(hand::Frame const& f) noexcept {
    if(active()||stop_requested.load(std::memory_order_acquire)||!nativeequipment::current(nullptr,f))return;
    const auto begin_ns=weaponhand::clock_ns(nullptr);
    names_ready=names.step(
        [](gears_equipment_names::Table& t){const auto a=get<Array>(base+0x1c34800);t={a.data,unsigned(a.count),unsigned(a.capacity)};return a.count>=0&&a.capacity>=0;},
        [](gears_equipment_names::Table t,unsigned i,std::array<wchar_t,32>& text){const auto entry=get<uptr>(t.data+std::size_t(i)*8);unsigned index{};
            return entry&&copy(&index,entry,4)&&index==i&&copy(text.data(),entry+20,sizeof(text));},
        [&](){return nativeequipment::current(nullptr,f);},[&](){const auto now=weaponhand::clock_ns(nullptr);
            return now>=begin_ns&&now-begin_ns<3000000;},indices,8192)==gears_equipment_names::State::ready;
}
bool controller_up(policy::Actors const& expected,policy::Observation& out) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField held{},again{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,expected.controller)||
       !weaponhand::derives(r,expected.controller.address,L"WarPC",L"WarfareGame")||
       !weaponhand::boolean(r,expected.controller.address,L"bHoldingButton_B",held)||
       !weaponhand::boolean(r,expected.controller.address,L"bHoldingButton_B",again)||
       !meta::same_property(held.property,again.property)||held.mask!=again.mask||
       (held.bits&held.mask)!=(again.bits&again.mask)||!unified_weapon_fire::retained_metadata(r))return false;
    out.actors=expected;out.controller_known=out.button_known=true;out.button_held=(held.bits&held.mask)!=0;return true;
}
bool context(policy::Actors const& expected,policy::Observation& out) noexcept {
    out={};if(!weaponhand::current_gt(nullptr)||!controller_up(expected,out))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return true;
    for(auto const& object:{expected.world,expected.controller,expected.pawn,expected.weapon})
        if(!weaponhand::object(r,object))return true;
    if(!weaponhand::derives(r,expected.pawn.address,L"WarPawn",L"WarfareGame")||
       !weaponhand::derives(r,expected.weapon.address,L"Weap_AssaultRifle",L"WarfareGame")||
       !weaponhand::derives(r,expected.world.address,L"WorldInfo",L"Engine"))return true;
    const auto read_links=[&](std::array<meta::PointerField,5>& links){return
        weaponhand::pointer(r,expected.controller.address,L"Pawn",expected.pawn.address,links[0])&&
        weaponhand::pointer(r,expected.controller.address,L"WorldInfo",expected.world.address,links[1])&&
        weaponhand::pointer(r,expected.pawn.address,L"Controller",expected.controller.address,links[2])&&
        weaponhand::pointer(r,expected.pawn.address,L"Weapon",expected.weapon.address,links[3])&&
        weaponhand::pointer(r,expected.weapon.address,L"Instigator",expected.pawn.address,links[4]);};
    std::array<meta::PointerField,5> links{};
    if(!read_links(links))return true;
    for(auto const& object:{expected.controller,expected.pawn,expected.weapon}){
        weaponhand::BoolField deleted{};
        if(!weaponhand::boolean(r,object.address,L"bDeleteMe",deleted)||(deleted.bits&deleted.mask))return true;
    }
    weaponhand::BoolField move{},mode{},special{},special_again{},motor{},melee{},motor_again{},melee_again{};
    unsigned char sm{},sm_again{},fm{},fm_again{};
    if(!weaponhand::field(r,expected.pawn.address,L"SpecialMove",L"ByteProperty",1,move)||
       !r.at(expected.pawn.address,move.property.offset,sm)||
       !weaponhand::field(r,expected.weapon.address,L"CurrentFireMode",L"ByteProperty",1,mode)||
       !r.at(expected.weapon.address,mode.property.offset,fm)||
       !weaponhand::boolean(r,expected.pawn.address,L"bDoingSpecialAttackMove",special)||
       !weaponhand::boolean(r,expected.weapon.address,L"bChainSawRunning",motor)||
       !weaponhand::boolean(r,expected.pawn.address,L"bDoingMeleeAttack",melee))return true;
    std::array<meta::PointerField,5> fresh_links{};if(!read_links(fresh_links))return true;
    for(unsigned i=0;i<5;++i)if(!meta::same_property(fresh_links[i].property,links[i].property))return true;
    if(!r.at(expected.pawn.address,move.property.offset,sm_again)||sm!=sm_again||
       !r.at(expected.weapon.address,mode.property.offset,fm_again)||fm!=fm_again||
       !weaponhand::boolean(r,expected.pawn.address,L"bDoingSpecialAttackMove",special_again)||
       !meta::same_property(special.property,special_again.property)||special.mask!=special_again.mask||
       (special.bits&special.mask)!=(special_again.bits&special_again.mask)||
       !weaponhand::boolean(r,expected.weapon.address,L"bChainSawRunning",motor_again)||
       !weaponhand::boolean(r,expected.pawn.address,L"bDoingMeleeAttack",melee_again)||
       !meta::same_property(motor.property,motor_again.property)||motor.mask!=motor_again.mask||
       (motor.bits&motor.mask)!=(motor_again.bits&motor_again.mask)||
       !meta::same_property(melee.property,melee_again.property)||melee.mask!=melee_again.mask||
       (melee.bits&melee.mask)!=(melee_again.bits&melee_again.mask)||!unified_weapon_fire::retained_metadata(r))return true;
    policy::Observation closing{};
    if(!controller_up(expected,closing)||closing.button_held!=out.button_held)return false;
    out.actors_current=out.current_lancer=out.fire_mode_known=true;out.special_move=sm;out.fire_mode=fm;
    out.doing_special=(special.bits&special.mask)!=0;out.motor_running=(motor.bits&motor.mask)!=0;
    out.doing_melee=(melee.bits&melee.mask)!=0;return true;
}
struct Mutation {
    Mutation(){mutating=true;}~Mutation(){mutating=false;}
};
bool button_schema(playground::Args& call,bool press) noexcept {
    const auto field=call.prop(L"ButtonName");
    return call.valid&&call.r.size==sizeof(Name)&&
        lancerreload_ui::original_function(call.r.function,press?L"ButtonPress":L"ButtonRelease",L"WarPC",false)&&
        hammerburst_animation::input_field(call,field,L"NameProperty",8)&&
        (get<unsigned long long>(field+0x78)&0x580)==0x80;
}
bool clear_target_timer() noexcept {
    const Name timer{indices[1],0};
    if(!name_current(timer,L"CheckForMeleeTarget"))return false;
    Mutation mutation;
    bool timer_active{};
    return lancerreload_state::timer_active(button.owner().weapon.address,timer,timer_active)&&
        (!timer_active||lancerreload_state::clear_timer(button.owner().weapon.address,timer));
}
bool refresh_input_pin(policy::HoldInput& pin,bool simulated,std::int64_t begin) noexcept {
    gears_vr::StereoSample stereo{};gears_vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput continuity{};gears_hand_continuity::Publisher publisher{};
    input_pin_gate=1;input_pin_elapsed_ns=0;
    input_pin_failure=nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
            simulated,stereo,input,continuity,publisher);
    if(input_pin_failure!=gears_current_tracking::Failure::none)return false;
    const auto end=weaponhand::clock_ns(nullptr);
    input_pin_elapsed_ns=end>=begin?end-begin:-1;
    input_pin_gate=2;if(begin<=0||end<begin||end-begin>50000000)return false;
    input_pin_gate=3;if(!pin.refresh(stereo,continuity,publisher,end,simulated))return false;
    input_pin_gate=0;return true;
}
bool refresh_hold_source() noexcept {
    hold_source_gate=1;input_pin_gate=0;input_pin_failure={};input_pin_elapsed_ns=0;hold_lifetime_failure=0;
    if(!button.owed()||!nativeequipment::domain_ready()||
       !policy::same(button.owner(),actors(source_parent))||source_hold!=vrequip::model.hold_generation())return false;
    const auto begin=weaponhand::clock_ns(nullptr);weaponhand::HoldSourceObservation game{};
    hold_source_gate=2;if(begin<=0)return false;
    hold_source_gate=3;if(!weaponhand::client.existing_action_lifetime_on_gt(action_lifetime,&hold_lifetime_failure))return false;
    hold_source_gate=4;if(!weaponhand::observe_hold_source(nullptr,source_parent,begin,game))return false;
    hold_source_gate=5;if(!game.known||!game.main_hand_held)return false;
    hold_source_gate=6;if(!refresh_input_pin(hold_input,source_parent.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated,begin))return false;
    hold_source_gate=7;if(!weaponhand::client.existing_action_lifetime_on_gt(action_lifetime,&hold_lifetime_failure))return false;
    // Only keep the already-owned B journal through the original animation.
    // No stale Frame/pose/decision is renewed; new actions still require them.
    last_source_ns=hold_input.sync_ns();hold_source_gate=0;return true;
}
bool retain_hold_during_pose_gap(uptr pc) noexcept {
    // A missing pose publication is not an equipment stow. In54304, generic
    // inventory suspension erased the committed hold BEFORE the saw's own
    // fresh-input owner check; that fabricated "not held" at age51ms.
    // Keep only an existing real ready33 owner, with exactly the same live
    // actor/weapon/lifetime/cohort and100ms controller-pin checks as service().
    // No pose, Frame, input edge, lease or equipment commit is supplied here.
    if(!button.owed()||mutating||stop_pending||stop_requested.load()||
       !weaponhand::current_gt(nullptr)||pc!=button.owner().controller.address||
       !nativeequipment::restoration_quiet())return false;
    policy::Observation observation{};
    if(!context(button.owner(),observation)||!observation.actors_current||!observation.current_lancer||
       !observation.button_known||!observation.button_held||observation.special_move!=33||
       !observation.motor_running||!observation.doing_melee||observation.doing_special||
       !vrequip::model.held_for(button.owner().world.address,button.owner().pawn.address,button.owner().weapon.address)||
       vrequip::model.hold_generation()!=source_hold||!refresh_hold_source())return false;
    const auto now=weaponhand::clock_ns(nullptr);
    return last_source_ns>0&&now>=last_source_ns&&now-last_source_ns<=100000000;
}
void report(policy::Status status) noexcept {
    const auto value=unsigned(status);
    if((value!=last_status||failure!=last_report_failure)&&status_rows++<32&&log_file){
        fwprintf(log_file,L"lancer_saw_original status=%u owed=%u stop=%u gate=%u last_timer_clear_verified=%u stop_reasons=%u source_age_ns=%lld observed_sm=%u observed_motor=%u observed_melee=%u observed_button_before_service=%u observed_actors_current=%u ready_stop_owed=%u observed_fire_mode=%u hold_source_gate=%u input_pin_gate=%u input_pin_failure=%u input_pin_elapsed_ns=%lld hold_lifetime_failure=%u hand_disarm_reason=%u first_hold_failure_known=%u first_hold_source_gate=%u first_input_pin_gate=%u first_input_pin_failure=%u first_hold_lifetime_failure=%u first_hand_disarm_reason=%u\n",
            value,unsigned(button.owed()),unsigned(stop_pending||stop_requested.load()),failure,unsigned(timer_clear_confirmed),stop_reasons,observed_source_age_ns,
            before_service.special_move,unsigned(before_service.motor_running),unsigned(before_service.doing_melee),unsigned(before_service.button_held),unsigned(before_service.actors_current),
            unsigned(ready_stop.owed()),before_service.fire_mode,hold_source_gate,input_pin_gate,unsigned(input_pin_failure),input_pin_elapsed_ns,hold_lifetime_failure,
            weaponhand::client.last_disarm_reason(),unsigned(first_hold_failure_known),first_hold_source_gate,
            first_input_pin_gate,unsigned(first_input_pin_failure),first_hold_lifetime_failure,
            first_hand_disarm_reason);fflush(log_file);}
    last_status=value;last_report_failure=failure;outstanding.store(active(),std::memory_order_release);
}
policy::Status service_ready_stop(uptr pc) noexcept {
    if(!ready_stop.owed())return policy::Status::released;
    policy::Observation observation{};
    if(pc!=ready_stop.owner().controller.address||!context(ready_stop.owner(),observation))return policy::Status::cleanup_debt;
    const auto stop=[&]() noexcept ->policy::Call {
        if(!nativeequipment::restoration_quiet())return {};
        playground::Args call(ready_stop.owner().pawn.address,L"StopMeleeAttack");
        const auto force=call.prop(L"bForceStopFire"),damage=call.prop(L"bForcedByDamage");
        if(!lancerreload_ui::original_function(call.r.function,L"StopMeleeAttack",L"WarPawn",false)||
           call.r.size>16||!hammerburst_animation::input_field(call,force,L"BoolProperty",4)||
           !hammerburst_animation::input_field(call,damage,L"BoolProperty",4))return {};
        const auto fo=get<unsigned>(force+0x94),dm=get<unsigned>(damage+0x94);
        const auto fm=get<unsigned>(force+0xd0),mm=get<unsigned>(damage+0xd0);
        if(!fm||(fm&(fm-1))||!mm||(mm&(mm-1))||
           (fo==dm?(fm&mm)!=0:!(fo+4<=dm||dm+4<=fo))||call.r.size!=(std::max)(fo+4,dm+4)||
           !call.set(L"bForceStopFire",true).set(L"bForcedByDamage",false).valid)return {};
        policy::Observation closing{};
        if(!context(ready_stop.owner(),closing)||!closing.actors_current||closing.button_held||
           closing.special_move==34||closing.doing_special||(closing.special_move&&closing.special_move!=33))return {};
        Mutation mutation;return {true,call.run()};
    };
    auto status=ready_stop.service(observation,stop);
    if(status==policy::Status::release_pending){
        policy::Observation after{};
        if(context(ready_stop.owner(),after))status=ready_stop.service(after,[](){return policy::Call{};});
        else status=policy::Status::cleanup_debt;
    }
    return status;
}
void service(uptr pc) noexcept {
    if(!button.owed()){
        if(ready_stop.owed()&&!mutating&&weaponhand::current_gt(nullptr)&&nativeequipment::restoration_quiet())report(service_ready_stop(pc));
        else if(!ready_stop.owed())outstanding.store(false,std::memory_order_release);
        return;
    }
    if(mutating||!weaponhand::current_gt(nullptr)||pc!=button.owner().controller.address||!nativeequipment::restoration_quiet())return;
    const auto now=weaponhand::clock_ns(nullptr);policy::Observation observation{};
    timer_clear_confirmed=false;
    failure=1;if(!context(button.owner(),observation)){stop_reasons|=1;stop_pending=true;report(policy::Status::cleanup_debt);return;}
    before_service=observation;failure=0;
    if(observation.actors_current&&observation.button_held){
        failure=2;timer_clear_confirmed=clear_target_timer();if(!timer_clear_confirmed)stop_pending=true;else failure=0;
        seen_running=seen_running||(observation.special_move==33&&observation.motor_running&&observation.doing_melee)||observation.special_move==34;
    }
    const bool source_current=refresh_hold_source();const auto source_now=weaponhand::clock_ns(nullptr);
    if(!source_current&&!first_hold_failure_known){
        first_hold_failure_known=true;first_hold_source_gate=hold_source_gate;
        first_input_pin_gate=input_pin_gate;first_input_pin_failure=input_pin_failure;
        first_hold_lifetime_failure=hold_lifetime_failure;
        first_hand_disarm_reason=weaponhand::client.last_disarm_reason();
    }
    observed_source_age_ns=last_source_ns>0&&source_now>=last_source_ns?source_now-last_source_ns:-1;
    if(hold_input.release_tail_expired(source_now)){stop_reasons|=256;stop_pending=true;}
    else if(!source_current||last_source_ns<=0||source_now<last_source_ns||source_now-last_source_ns>100000000){stop_reasons|=2;stop_pending=true;}
    if(ready_notice_pending){
        if(source_current&&observation.actors_current&&observation.current_lancer&&observation.button_held&&
           observation.special_move==33&&observation.motor_running&&observation.doing_melee){
            ready_transition_seen=true;
            if(log_file&&notice_rows++<8){fwprintf(log_file,L"lancer_saw_original_ready_confirmed sm=33 motor=1 melee=1 actual_context=1 input_age_ns=%lld\n",observed_source_age_ns);fflush(log_file);}
        }else{stop_reasons|=32;stop_pending=true;}
        ready_notice_pending=false;
    }
    if(!seen_running&&started_ns>0&&now-started_ns>700000000){stop_reasons|=4;stop_pending=true;}
    if(!vrequip::model.held_for(button.owner().world.address,button.owner().pawn.address,button.owner().weapon.address)){stop_reasons|=8;stop_pending=true;}
    if(vrequip::model.hold_generation()!=source_hold){stop_reasons|=16;stop_pending=true;}
    const auto release=[&]() noexcept ->policy::Call {
        if(!nativeequipment::restoration_quiet())return {};
        policy::Observation current{};if(!context(button.owner(),current)||!current.actors_current||
           !current.button_known||!current.button_held||current.special_move==34||current.doing_special)return {};
        playground::Args call(button.owner().controller.address,L"ButtonRelease");
        if(!button_schema(call,false)||!name_current(Name{indices[0],0},L"B")||!call.set(L"ButtonName",Name{indices[0],0}).valid)return {};
        Mutation mutation;return {true,call.run()};
    };
    const auto old_owner=button.owner();
    auto status=button.service(observation,stop_pending||stop_requested.load(std::memory_order_acquire),release);
    if(status==policy::Status::release_pending){
        policy::Observation after{};
        if(context(button.owner(),after))status=button.service(after,true,[](){return policy::Call{};});
        else status=policy::Status::cleanup_debt;
    }
    if(!button.owed()){
        ready_stop.arm(old_owner);status=service_ready_stop(pc);
        stop_pending=ready_stop.owed();seen_running=false;started_ns=last_source_ns=0;
    }
    report(status);
}
void after_original_tick(uptr pc) noexcept {
    if(ready_stop.owed()||(button.owed()&&(ready_notice_pending||stop_pending||stop_requested.load(std::memory_order_acquire))))service(pc);
}
bool begin(hand::Frame const& f) noexcept {
    struct AttemptReport {bool invoked{};~AttemptReport(){if(!invoked&&!button.owed())report(policy::Status::not_started);}} attempt;
    failure=10;
    weapon_part_assets::WeaponPoseOwner held_owner{};
    if(active()||stop_requested.load(std::memory_order_acquire)||!names_ready||!unified_weapon_fire::manual_profile()||
       !nativeequipment::domain_ready()||!nativeequipment::current(nullptr,f))return false;
    // Authenticate the exact grant cohort before the original/reflection
    // queries. The transport retains8 samples (~89ms at90Hz), less than the
    // unchanged100ms Frame lifetime. Re-reading that history only at the end
    // can lose an otherwise-valid grant. This immutable copy is NOT a permit:
    // final source/currentness checks and a latest publisher/epoch pin remain.
    gears_hand_continuity::TrackedInput continuity{};gears_hand_continuity::Publisher publisher{};
    failure=15;if(!nativeequipment::read_input_continuity(f,continuity,publisher))return false;
    failure=10;if(!weapon_part_assets::weapon_owner(f,held_owner)||
       !nativeequipment::attached_frame(f,held_owner.component.address))return false;
    // This invocation-local exception only permits the original boolean
    // result after the real physical starter has reached its evaluated gate.
    if(!lancerreload_manual::enter_starter(f))return false;
    struct StarterScope {~StarterScope(){lancerreload_manual::leave_starter();}} starter_scope;
    // Drain the existing fire/aim owners before special melee. Their original
    // calls may retire this frame; retry requires another fresh association.
    if(!vrfire::yield_for_native_equipment(f)||!nativeequipment::current(nullptr,f))return false;
    policy::Observation observation{};
    failure=11;if(!context(actors(f),observation)||!observation.actors_current||observation.button_held||observation.special_move||observation.doing_special||!prepare_ready_schema(f))return false;
    playground::Args can(f.weapon.address,L"CanDoSpecialMeleeAttack");
    failure=12;if(!lancerreload_ui::original_function(can.r.function,L"CanDoSpecialMeleeAttack",L"Weap_AssaultRifle",false)||
       can.r.size!=4||!vrequip::result_property(can,L"BoolProperty",4)||!nativeequipment::current(nullptr,f))return false;
    {Mutation mutation;if(!can.run())return false;}
    observation.can_start=(can.output<unsigned>()&get<unsigned>(can.prop(L"ReturnValue")+0xd0))!=0;
    if(!observation.can_start){failure=14;return false;}
    playground::Args call(f.controller.address,L"ButtonPress");
    failure=13;if(!button_schema(call,true)||!name_current(Name{indices[0],0},L"B")||
       !name_current(Name{indices[1],0},L"CheckForMeleeTarget")||!call.set(L"ButtonName",Name{indices[0],0}).valid||
       !nativeequipment::current(nullptr,f))return false;
    failure=16;if(!hold_input.bind(f.stereo,continuity,publisher,weaponhand::clock_ns(nullptr),
           f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated,hand::left_control(f),true))return false;
    failure=17;if(!refresh_input_pin(hold_input,f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated,weaponhand::clock_ns(nullptr)))return false;
    failure=18;if(!weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,action_lifetime))return false;
    failure=19;if(!nativeequipment::current(nullptr,f))return false;
    source_parent=f;ready_transition_seen=ready_notice_pending=false;
    started_ns=weaponhand::clock_ns(nullptr);last_source_ns=hold_input.sync_ns();
    stop_reasons=0;observed_source_age_ns=0;before_service=observation;
    first_hold_failure_known=false;first_hold_source_gate=first_input_pin_gate=0;
    first_hold_lifetime_failure=first_hand_disarm_reason=0;first_input_pin_failure={};
    source_session=f.stereo.session;source_space=f.stereo.space_epoch;source_input=f.controllers.input_epoch;
    source_reference=f.reference_generation;source_hold=vrequip::model.hold_generation();
    previous_sequence=f.controllers.sequence;right_press=f.controllers.hands[1].pressed[gears_vr::control_squeeze];
    bool invoked{};
    const auto status=button.begin(observation,true,[&]() noexcept ->policy::Call {
        if(!nativeequipment::current(nullptr,f))return {};
        outstanding.store(true,std::memory_order_release);invoked=true;Mutation mutation;return {true,call.run()};
    });
    stop_pending=status==policy::Status::cleanup_debt;if(status==policy::Status::owned)failure=0;report(status);
    if(button.owed())service(f.controller.address); // immediate first timer suppression
    attempt.invoked=invoked;return invoked;
}
bool pre_equipment(hand::Frame const& f) noexcept {
    if(!button.owed()){
        if(!ready_stop.owed())return false;
        service(f.controller.address);return active()||!nativeequipment::current(nullptr,f);
    }
    if(!policy::same(button.owner(),actors(f))){stop_reasons|=64;stop_pending=true;service(button.owner().controller.address);return true;}
    const bool cohort=f.stereo.session==source_session&&f.stereo.space_epoch==source_space&&
        f.controllers.input_epoch==source_input&&f.reference_generation==source_reference;
    if(!cohort){stop_reasons|=128;stop_pending=true;service(f.controller.address);return true;}
    source_parent=f; // Exact current association; the live input witness stays independent.
    const auto& right=f.controllers.hands[1];const auto presses=right.pressed[gears_vr::control_squeeze];
    bool body_grip=false;
    if(presses!=right_press&&right.last_squeeze_press_sequence>previous_sequence&&
       right.last_squeeze_press_sequence<=f.controllers.sequence&&
       gears_vr::controller_pose_usable(right.last_squeeze_press_grip,right.last_squeeze_press_flags,right.squeeze_active)){
        const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
        body_grip=gears_weapon::original_inventory_slot(gears_weapon::body_slot(nativeequipment::calibration,body,right.last_squeeze_press_grip.position,1)).has_value();
    }
    right_press=presses;previous_sequence=f.controllers.sequence;
    if(body_grip)stop_pending=true;
    if(stop_pending||stop_requested.load(std::memory_order_acquire))service(f.controller.address);
    // The sole equipment input reader has not consumed this Grip edge. When
    // the exact old B is already PROVEN up and this frame is still current,
    // continue into equipment now. Deferring unconditionally loses a100ms
    // edge at the observed ~100ms game cadence, then a later shoulder touch
    // toggles the still-held Lancer into stow instead of recovering it.
    // Owed/deferred animation or an expired frame still blocks equipment.
    return active()||!nativeequipment::current(nullptr,f);
}
bool acquisition_allowed(hand::Frame const& f) noexcept {
    return button.owed()&&!ready_stop.owed()&&!mutating&&!stop_pending&&!ready_notice_pending&&ready_transition_seen&&!stop_requested.load(std::memory_order_acquire)&&
        timer_clear_confirmed&&policy::same(button.owner(),actors(f))&&
        source_session==f.stereo.session&&source_space==f.stereo.space_epoch&&source_input==f.controllers.input_epoch&&
        source_reference==f.reference_generation&&source_hold==vrequip::model.hold_generation()&&
        nativeequipment::domain_ready()&&nativeequipment::current(nullptr,f);
}
bool ready_motion_allowed(hand::Frame const& f) noexcept {
    if(!acquisition_allowed(f))return false;
    policy::Observation actual{};
    return context(button.owner(),actual)&&policy::ready_motion_context(actual)&&acquisition_allowed(f);
}
bool expected_ready_move(uptr object,uptr function,void const* params) noexcept {
    if(object!=button.owner().pawn.address||ready_transition_seen||seen_running||stop_pending||
       source_hold!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(button.owner().world.address,button.owner().pawn.address,button.owner().weapon.address))return false;
    if(function!=ready_schema.function.address||!weapon_part_assets::object(ready_schema.function)||
       !weapon_part_assets::object(ready_schema.move)||!weapon_part_assets::object(ready_schema.enumeration)||
       get<unsigned short>(function+0x106)!=ready_schema.bytes||
       get<unsigned>(ready_schema.move.address+0x94)!=ready_schema.offset||
       get<uptr>(ready_schema.move.address+0xd0)!=ready_schema.enumeration.address)return false;
    if(params){unsigned char value{},again{};
        if(!copy(&value,reinterpret_cast<uptr>(params)+ready_schema.offset,1)||value!=33||
           !copy(&again,reinterpret_cast<uptr>(params)+ready_schema.offset,1)||again!=value)return false;}
    // A native CallFunction notice has no callee parameter buffer. Defer only
    // this one expected startup call; actual SM33/motor/melee and fresh input
    // must all be confirmed on normal GT before contact can acquire a target.
    ready_notice_pending=true;return true;
}
void observe_call_no_engine(uptr object,uptr function,void const* params) noexcept {
    if(mutating||!button.owed()||!weaponhand::current_gt(nullptr)||!registered(function))return;
    const auto& owner=button.owner();
    if(object!=owner.controller.address&&object!=owner.pawn.address&&object!=owner.weapon.address&&object!=owner.world.address)return;
    wchar_t label[96]{};if(!name(function,label,96)){stop_pending=true;return;}
    if(!std::wcscmp(label,L"DoSpecialMove")&&expected_ready_move(object,function,params)){
        if(log_file&&notice_rows++<8){fwprintf(log_file,L"lancer_saw_original_ready_transition function=DoSpecialMove params_known=%u awaiting_actual_confirmation=1 same_owned_pawn=1 schema_verified=1\n",unsigned(params!=nullptr));fflush(log_file);}
        return;
    }
    const wchar_t* changes[]={L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",L"ClientWeaponSet",L"Possess",L"UnPossess",
        L"ForceReload",L"Died",L"Destroyed",L"DoSpecialMove",L"StartSpecialMove",L"ClientTravel",L"ServerTravel"};
    for(auto change:changes)if(!std::wcscmp(label,change)){
        stop_reasons|=32;stop_pending=true;
        if(log_file&&notice_rows++<8){fwprintf(log_file,L"lancer_saw_original_context_notice function=%ls object=%llx at_ns=%lld stop_rules_unchanged=1\n",
            label,object,weaponhand::clock_ns(nullptr));fflush(log_file);}break;
    }
}
void reset_after_drain() noexcept {
    if(active())return;
    names.reset();indices={};names_ready=false;stop_pending=seen_running=timer_clear_confirmed=ready_transition_seen=ready_notice_pending=false;started_ns=last_source_ns=0;
    source_parent={};action_lifetime={};hold_input.reset();ready_schema={};
    first_hold_failure_known=false;first_hold_source_gate=first_input_pin_gate=0;
    first_hold_lifetime_failure=first_hand_disarm_reason=0;first_input_pin_failure={};
    source_session=source_input=source_space=source_reference=source_hold=previous_sequence=0;right_press=0;
    stop_requested.store(false,std::memory_order_release);outstanding.store(false,std::memory_order_release);last_status=last_report_failure=~0u;status_rows=failure=0;
    stop_reasons=notice_rows=0;observed_source_age_ns=0;before_service={};
    hold_source_gate=input_pin_gate=0;input_pin_failure={};input_pin_elapsed_ns=0;
}
} // namespace lancersaw_original
