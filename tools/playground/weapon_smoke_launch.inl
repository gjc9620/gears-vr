// One lexical original AnimNotifyThrow dispatch. Native still owns every
// CallFunction/original call; this subscriber only replaces two typed vector
// results within this exact original Smoke invocation. No hook or replay.
// Local display-only result substitution on the existing Native subscriber.
// Keep SimulationDir, the original Tick, StopSimulation and projectile chain.
namespace smokepreview {
namespace callabi=gears_weapon_call;
struct Pending {callabi::Invocation call{};smoke_original::Entry entry{};std::uint64_t hold{};bool active{};};
std::array<Pending,4> pending{};
std::uint64_t suppressed{};
bool current(Pending const& p) noexcept {
    const auto& source=smokepreparation::journal.source;
    return weaponhand::current_gt(nullptr)&&smokepreparation::owed()&&smokepreparation::journal.prepared&&
        vrequip::model.hold_generation()==p.hold&&
        vrequip::model.held_for(source.world.address,source.pawn.address,source.weapon.address)&&
        weaponhand::client.existing_action_lifetime_on_gt(smokepreparation::journal.lifetime)&&
        weapon_part_assets::object(source.weapon)&&weapon_part_assets::object(source.pawn)&&
        playground::pointer(source.pawn.address,L"Weapon")==source.weapon.address&&
        playground::pointer(source.weapon.address,L"Instigator")==source.pawn.address;
}
void begin(callabi::Invocation const& call) noexcept {
    if(!weaponhand::current_gt(nullptr)||!smokepreparation::owed()||!smokepreparation::journal.prepared||
       !callabi::invocation_valid(call)||call.thread_id!=GetCurrentThreadId()||
       call.self!=smokepreparation::journal.source.weapon.address||!playground::named(call.callee,L"ShouldDrawTrajectoryPrediction"))return;
    smoke_original::Entry entry{};
    if(!smoke_original::bind(call.self,smoke_original::Method::prediction,entry)||entry.function.address!=call.callee||
       !call.result||(call.result&3)||call.result>UINTPTR_MAX-4)return;
    Pending candidate{call,entry,vrequip::model.hold_generation(),true};
    if(!current(candidate))return;
    for(auto& p:pending)if(!p.active){p=candidate;return;}
}
bool returned(callabi::Invocation const& call) noexcept {
    for(auto& p:pending)if(p.active&&p.call.call_id==call.call_id){
        smoke_original::Entry fresh{};
        if(!callabi::same(p.call,call)||!current(p)||
           !smoke_original::bind(call.self,smoke_original::Method::prediction,fresh)||!smoke_original::same(fresh,p.entry))return false;
        const auto mask=fresh.parameters[0].mask;unsigned before{};bool changed{};
        if(fresh.count!=1||!mask||!copy(&before,call.result,4)||!current(p)||
           !unified_weapon_fire::compare_word(call.result,before,before&~mask,changed)||!changed)return false;
        ++suppressed;p={};return true;
    }
    return true;
}
bool cleanup(callabi::Invocation const& call) noexcept {
    for(auto& p:pending)if(p.active&&p.call.call_id==call.call_id){const bool same=callabi::same(p.call,call);p={};return same;}
    return true;
}
}
namespace smokelaunch {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;namespace callabi=gears_weapon_call;
using Vector=playground::Vector;
struct Pending {callabi::Invocation call{};smoke_original::Entry entry{};Vector desired{};bool live{},uncertain{};};
struct Journal {
    hand::Frame source{};hand::ExistingActionLifetime lifetime{};
    smoke_original::Entry notify{},smoke_throw{},base_throw{},origin{},velocity{};
    hand::ObjectIdentity consume{},notification{},projectile{};
    gears_weapon::GameGrenadeLaunch plan{};
    std::array<Pending,4> pending{};
    hand::ObjectIdentity direction_property{};unsigned direction_offset{};
    std::array<unsigned,3> original_direction{},desired_direction{};
    unsigned direction_words{},origins{},velocities{},throws{},smoke_throws{},consumes{},notifications{},inits{};
    bool active{},fault{},dispatched{},returned{};
};
Journal journal{};
std::atomic<bool> outstanding{};
bool current_identity() noexcept {
    // This runs INSIDE the already admitted original invocation. Its action
    // depth is necessarily nonzero; the normal-GT new-action fence belongs
    // only to invoke_once preflight, not this exact nested result observer.
    if(!weaponhand::current_gt(nullptr)||!journal.active||!journal.dispatched||
       !weaponhand::client.existing_action_lifetime_on_gt(journal.lifetime))return false;
    for(const auto& i:{journal.source.world,journal.source.controller,journal.source.pawn,journal.source.weapon})
        if(!weapon_part_assets::object(i))return false;
    return playground::pointer(journal.source.controller.address,L"Pawn")==journal.source.pawn.address&&
        playground::pointer(journal.source.pawn.address,L"Weapon")==journal.source.weapon.address&&
        playground::pointer(journal.source.weapon.address,L"Instigator")==journal.source.pawn.address;
}
bool body_caller(callabi::Invocation const& call) noexcept {
    gears_ui_call_boundary::Frame frame{};
    return weaponhand::current_gt(nullptr)&&journal.active&&journal.dispatched&&!journal.fault&&callabi::invocation_valid(call)&&call.thread_id==GetCurrentThreadId()&&
        call.hand_cookie==journal.source.key.registration_cookie&&call.native_started_qpc==journal.source.key.native_started_qpc&&
        gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,frame)&&
        frame.node==journal.base_throw.function.address&&frame.object==journal.source.weapon.address&&
        gears_weapon_call_trace::script_pc(frame.code,journal.base_throw.script,journal.base_throw.script_bytes)&&current_identity();
}
bool original_method(uptr receiver,wchar_t const* owner_label,wchar_t const* label,hand::ObjectIdentity& out) noexcept {
    playground::Args call(receiver,label);if(!call.valid||!lancerreload_ui::original_function(call.r.function,label,owner_label,false))return false;
    const auto flags=get<unsigned>(call.r.function+0xf4);const auto native=get<unsigned short>(call.r.function+0xf8);
    meta::Table script{};std::uint64_t hash{};
    return !(flags&0x400)&&!native&&get<uptr>(call.r.function+0x114)==base+0x1d6ca0&&
        copy(&script,call.r.function+0x8c,sizeof script)&&script.count>0&&script.count<=script.capacity&&script.capacity<=131072&&
        weaponshot::hash_script(script.data,unsigned(script.count),hash)&&nativeequipment::identify(nullptr,call.r.function,out);
}
void begin(callabi::Invocation const& call) noexcept {
    smokepreview::begin(call);
    if(!weaponhand::current_gt(nullptr)||!journal.active||journal.fault)return;
    if(call.self==journal.source.weapon.address&&call.callee==journal.smoke_throw.function.address){
        if(++journal.smoke_throws!=1)journal.fault=true;return;
    }
    if(call.self==journal.source.weapon.address&&call.callee==journal.base_throw.function.address){
        if(++journal.throws!=1)journal.fault=true;return;
    }
    if(!body_caller(call))return;
    if(call.self==journal.source.weapon.address&&call.callee==journal.consume.address){
        if(++journal.consumes!=1)journal.fault=true;return;
    }
    if(call.self!=journal.source.weapon.address){
        // Observe the actual spawned actor at its original Init call. Never
        // treat the caller FFrame as a callee parameter buffer or edit Init.
        if(playground::named(call.callee,L"Init")){
            meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
            hand::ObjectIdentity actor{};meta::PointerField world{},instigator{};
            if(meta::tables(base,schema.objects,schema.names)&&smoke_original::identify(r,call.self,actor)&&
               weaponhand::derives(r,call.self,L"Proj_SmokeGrenade",L"WarfareGame")&&
               weaponhand::pointer(r,call.self,L"WorldInfo",journal.source.world.address,world)&&
               weaponhand::pointer(r,call.self,L"Instigator",journal.source.pawn.address,instigator)&&
               unified_weapon_fire::retained_metadata(r)){
                if(++journal.inits!=1)journal.fault=true;else journal.projectile=actor;
            }
        }
        return;
    }
    const smoke_original::Entry* entry=nullptr;Vector desired{};
    if(call.callee==journal.origin.function.address){entry=&journal.origin;
        desired={float(journal.plan.position.x),float(journal.plan.position.y),float(journal.plan.position.z)};
    }else if(call.callee==journal.velocity.function.address){entry=&journal.velocity;
        desired={float(journal.plan.velocity.x),float(journal.plan.velocity.y),float(journal.plan.velocity.z)};
    }else return;
    if(!call.result||(call.result&3)||call.result>UINTPTR_MAX-12){journal.fault=true;return;}
    for(auto& p:journal.pending)if(!p.live){p={call,*entry,desired,true,false};return;}
    journal.fault=true;
}
bool returned(callabi::Invocation const& call) noexcept {
    if(!smokepreview::returned(call))return false;
    if(!weaponhand::current_gt(nullptr)||!journal.active)return true;
    // Notification is invoked by the original base AnimNotifyThrow, outside
    // the ThrowGrenade caller. Count it without redirecting its arguments.
    if(call.self==journal.source.weapon.address&&call.callee==journal.notification.address){
        if(++journal.notifications!=1)journal.fault=true;
    }
    Pending* slot_ptr=nullptr;for(auto& p:journal.pending)if(p.live&&p.call.call_id==call.call_id){slot_ptr=&p;break;}
    if(!slot_ptr)return true;
    if(!callabi::same(call,slot_ptr->call)||!body_caller(call)){slot_ptr->uncertain=true;journal.fault=true;return false;}
    smoke_original::Entry fresh{};
    if(!smoke_original::bind(call.self,slot_ptr->entry.method,fresh)||!smoke_original::same(fresh,slot_ptr->entry)){
        slot_ptr->uncertain=true;journal.fault=true;return false;
    }
    std::array<unsigned,3> before_words{},desired{};
    Vector before{};
    if(!copy(&before,call.result,sizeof before)||!std::isfinite(before.x)||!std::isfinite(before.y)||!std::isfinite(before.z)){
        slot_ptr->uncertain=true;journal.fault=true;return false;}
    std::memcpy(before_words.data(),&before,12);std::memcpy(desired.data(),&slot_ptr->desired,12);
    unsigned changed{};
    for(;changed<3;++changed){bool exchanged{};
        if(!current_identity()||!unified_weapon_fire::compare_word(call.result+changed*4,before_words[changed],desired[changed],exchanged)||!exchanged)break;}
    if(changed!=3){
        bool restored=true;while(changed){--changed;bool exchanged{};
            if(!unified_weapon_fire::compare_word(call.result+changed*4,desired[changed],before_words[changed],exchanged)||!exchanged)restored=false;}
        slot_ptr->uncertain=!restored;journal.fault=true;return restored;
    }
    if(slot_ptr->entry.method==smoke_original::Method::origin)++journal.origins;else ++journal.velocities;
    if(journal.origins>1||journal.velocities>1)journal.fault=true;
    *slot_ptr={};return true;
}
bool cleanup(callabi::Invocation const& call) noexcept {
    if(!smokepreview::cleanup(call))return false;
    if(!weaponhand::current_gt(nullptr))return true;
    for(auto& p:journal.pending)if(p.live&&p.call.call_id==call.call_id){
        const bool okay=!p.uncertain&&callabi::same(p.call,call);p={};journal.fault=true;return okay;}
    return true;
}
bool restore_direction() noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    if(!journal.direction_words){outstanding.store(false,std::memory_order_release);return true;}
    if(!weapon_part_assets::object(journal.source.weapon)||!weapon_part_assets::object(journal.direction_property))return false;
    while(journal.direction_words){const unsigned n=journal.direction_words-1;bool exchanged{};
        if(!unified_weapon_fire::compare_word(journal.source.weapon.address+journal.direction_offset+n*4,
            journal.desired_direction[n],journal.original_direction[n],exchanged)||!exchanged)return false;
        --journal.direction_words;
    }
    outstanding.store(false,std::memory_order_release);return true;
}
bool install_direction() noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};Vector before{},again{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,journal.source.weapon)||
       !weaponhand::field(r,journal.source.weapon.address,L"SimulationDir",L"StructProperty",12,field)||
       !unified_weapon_fire::core_struct(r,field.property,L"Vector")||
       !smoke_original::identify(r,schema.identities[field.property.identity].address,journal.direction_property)||
       !r.at(journal.source.weapon.address,field.property.offset,before)||
       !r.at(journal.source.weapon.address,field.property.offset,again)||std::memcmp(&before,&again,12)||
       !std::isfinite(before.x)||!std::isfinite(before.y)||!std::isfinite(before.z)||!unified_weapon_fire::retained_metadata(r))return false;
    journal.direction_offset=field.property.offset;
    const auto& v=journal.plan.direction;const Vector desired{float(v.x),float(v.y),float(v.z)};
    if(!gears_vr::finite(v)||std::abs(gears_weapon::length(v)-1)>1e-3)return false;
    std::memcpy(journal.original_direction.data(),&before,12);std::memcpy(journal.desired_direction.data(),&desired,12);
    outstanding.store(true,std::memory_order_release);
    for(unsigned n=0;n<3;++n){bool exchanged{};
        if(!nativeequipment::current(nullptr,journal.source)||
           !unified_weapon_fire::compare_word(journal.source.weapon.address+journal.direction_offset+n*4,
              journal.original_direction[n],journal.desired_direction[n],exchanged)||!exchanged){restore_direction();return false;}
        ++journal.direction_words;
    }
    return true;
}
struct Result {
    bool token_taken{},dispatched{},returned{},direction_restored{},original_once{};
    unsigned preflight_stage{};
    std::int64_t executed_ns{};
    hand::ObjectIdentity projectile{};
    Vector actual_position{},actual_velocity{};
    bool initial_state_matches{};
    unsigned origins{},velocities{},throws{},smoke_throws{},consumes{},notifications{},inits{};
};
bool read_launch(Result& result) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,result.projectile))return false;
    const auto vector=[&](wchar_t const* label,Vector& out) noexcept {
        // Literal-specific field() owns the actual class ancestry and layout.
        weaponhand::BoolField field{};
        bool okay=false;
        if(wcscmp(label,L"Location")==0)okay=weaponhand::field(r,result.projectile.address,L"Location",L"StructProperty",12,field);
        else okay=weaponhand::field(r,result.projectile.address,L"Velocity",L"StructProperty",12,field);
        Vector a{},b{};
        if(!okay||!unified_weapon_fire::core_struct(r,field.property,L"Vector")||
           !r.at(result.projectile.address,field.property.offset,a)||!r.at(result.projectile.address,field.property.offset,b)||
           std::memcmp(&a,&b,sizeof a)||!std::isfinite(a.x)||!std::isfinite(a.y)||!std::isfinite(a.z))return false;
        out=a;return true;
    };
    weaponhand::BoolField x{},y{},z{};float vx{},vy{},vz{};
    if(!vector(L"Location",result.actual_position)||!vector(L"Velocity",result.actual_velocity)||
       !weaponhand::field(r,result.projectile.address,L"InitialVX",L"FloatProperty",4,x)||
       !weaponhand::field(r,result.projectile.address,L"InitialVY",L"FloatProperty",4,y)||
       !weaponhand::field(r,result.projectile.address,L"InitialVZ",L"FloatProperty",4,z)||
       !r.at(result.projectile.address,x.property.offset,vx)||!r.at(result.projectile.address,y.property.offset,vy)||
       !r.at(result.projectile.address,z.property.offset,vz)||!unified_weapon_fire::retained_metadata(r))return false;
    const auto close=[](double a,double b) noexcept {return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=.001+.00001*std::abs(b);};
    const auto& v=result.actual_velocity;const auto& p=result.actual_position;
    const auto& wanted=journal.plan;
    return std::hypot(std::hypot(p.x-wanted.position.x,p.y-wanted.position.y),p.z-wanted.position.z)<=5.&&
        close(v.x,wanted.velocity.x)&&close(v.y,wanted.velocity.y)&&close(v.z,wanted.velocity.z)&&
        close(vx,v.x)&&close(vy,v.y)&&close(vz,v.z);
}
bool collision_plan(hand::Frame const& source,gears_weapon::GameGrenadeLaunch const& raw,
    gears_weapon::GameGrenadeLaunch& out) noexcept {
    if(!nativeequipment::current(nullptr,source)||!gears_vr::finite(raw.position)||
       !gears_vr::finite(raw.velocity)||!gears_vr::finite(raw.direction))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};Vector pawn{},again{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,source.pawn)||
       !weaponhand::field(r,source.pawn.address,L"Location",L"StructProperty",12,field)||
       !unified_weapon_fire::core_struct(r,field.property,L"Vector")||
       !r.at(source.pawn.address,field.property.offset,pawn)||!r.at(source.pawn.address,field.property.offset,again)||
       std::memcmp(&pawn,&again,12)||!std::isfinite(pawn.x)||!std::isfinite(pawn.y)||
       !unified_weapon_fire::retained_metadata(r))return false;
    // Preserve the stock short volume trace from pawn XY at release height.
    // The clamped point is chosen BEFORE Spawn, never a projectile teleport.
    const Vector from{pawn.x,pawn.y,float(raw.position.z)},to{float(raw.position.x),float(raw.position.y),float(raw.position.z)},extent{5,5,5};
    playground::Args trace(source.pawn.address,L"Trace");
    const auto vector_input=[&](wchar_t const* label,Vector value) noexcept {
        const auto p=trace.prop(label),structure=p?get<uptr>(p+0xd0):0;
        return hammerburst_animation::input_field(trace,p,L"StructProperty",12)&&registered(structure)&&
            playground::named(structure,L"Vector")&&trace.set(label,value).valid;
    };
    const auto flags=trace.prop(L"ExtraTraceFlags"),hit=trace.prop(L"HitLocation");
    if(!weapon_part_assets::native_function(trace,L"Trace",L"Actor")||!vrequip::result_property(trace,L"ObjectProperty",8)||
       !vector_input(L"TraceStart",from)||!vector_input(L"TraceEnd",to)||!vector_input(L"Extent",extent)||
       !nativelancerfollow::bool_arg(trace,L"bTraceActors",false)||
       !hammerburst_animation::input_field(trace,flags,L"IntProperty",4)||!trace.set(L"ExtraTraceFlags",16).valid||
       !hit||get<unsigned>(hit+0x70)!=1||get<unsigned>(hit+0x74)!=12||
       !playground::named(get<uptr>(hit+0x50),L"StructProperty")||
       (get<unsigned long long>(hit+0x78)&0x580)!=0x180||!nativeequipment::current(nullptr,source)||!trace.run()||
       !nativeequipment::current(nullptr,source))return false;
    auto plan=raw;
    if(trace.output<uptr>()){
        const auto position=trace.output<Vector>(L"HitLocation");
        if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z))return false;
        plan.position={position.x,position.y,position.z};
    }
    out=plan;return true;
}
// The normal-GT owner supplies the pending Smoke request. This scope performs
// collision preflight and takes its token once. It never retries or starts a
// stock throw animation that could notify a second time later.
Result invoke_once(hand::Frame const& source,gears_weapon::SmokeGrenadeInteraction& owner) noexcept {
    Result result{};
    if(!weaponhand::current_gt(nullptr))return result;
    const auto request=owner.pending_request();
    result.preflight_stage=1;
    if(journal.active||journal.direction_words||!nativeequipment::current(nullptr,source)||!weaponcall::ready())return result;
    result.preflight_stage=2;
    if(request.action!=gears_weapon::SmokeAction::launch_original||!request.token||request.binding!=owner.binding_token()||
       request.identity.world!=source.world.address||request.identity.pawn!=source.pawn.address||
       request.identity.controller!=source.controller.address||request.identity.weapon!=source.weapon.address||
       request.identity.world_generation!=source.key.world_generation||
       request.identity.xr_session!=source.stereo.session||request.identity.space_epoch!=source.stereo.space_epoch||
       request.identity.input_epoch!=source.controllers.input_epoch||request.identity.profile_generation!=source.controllers.profile_generation)return result;
    result.preflight_stage=3;
    if(!request.executable_at(weaponhand::clock_ns(nullptr),request.identity,request.binding,request.motion_epoch,request.reference_generation))return result;
    result.preflight_stage=4;
    if(!smokepreparation::ready(source,request.original_preparation_serial))return result;
    journal={};journal.source=source;
    lancermain_asset::Evidence asset{};smoke_metadata::Evidence types{};
    result.preflight_stage=5;
    if(lancermain_asset::read(source,asset,lancermain_asset::AssetScope::smoke_hold)!=lancermain_asset::Status::ready)return result;
    result.preflight_stage=6;if(!nativeequipment::attached_frame(source,asset.component.address))return result;
    result.preflight_stage=7;if(!smoke_original::bind(source.weapon.address,smoke_original::Method::notify,journal.notify))return result;
    result.preflight_stage=8;if(!smoke_original::bind(source.weapon.address,smoke_original::Method::base_throw,journal.base_throw))return result;
    if(!smoke_original::bind(source.weapon.address,smoke_original::Method::smoke_throw,journal.smoke_throw))return result;
    result.preflight_stage=9;if(!smoke_original::bind(source.weapon.address,smoke_original::Method::origin,journal.origin))return result;
    result.preflight_stage=10;if(!smoke_original::bind(source.weapon.address,smoke_original::Method::velocity,journal.velocity))return result;
    result.preflight_stage=11;if(!original_method(source.weapon.address,L"WarWeapon",L"ConsumeAmmo",journal.consume))return result;
    result.preflight_stage=12;if(!original_method(source.weapon.address,L"Weap_Grenade",L"GrenadeThrownNotification",journal.notification))return result;
    result.preflight_stage=13;if(!weaponhand::client.capture_existing_action_lifetime_on_gt(source.key,journal.lifetime))return result;
    result.preflight_stage=14;if(!collision_plan(source,request.launch,journal.plan))return result;
    result.preflight_stage=15;
    if(!smoke_metadata::read(source,types)||request.original_entry!=journal.notify.function.address||
       request.original_projectile_type!=types.projectile_type.address||
       request.original_ammo_type!=types.ammo_type.address||request.original_emitter_type!=types.emitter_type.address)
        return result;
    // The token is consumed exactly once, only after preflight is ready. A
    // later uncertain original call cannot be retried with that preparation.
    const auto taken=owner.take_request();
    result.token_taken=bool(taken);
    result.preflight_stage=16;
    if(!taken||taken->token!=request.token)return result;
    result.preflight_stage=17;
    if(!request.executable_at(weaponhand::clock_ns(nullptr),
        request.identity,owner.binding_token(),request.motion_epoch,request.reference_generation))return result;
    result.preflight_stage=18;if(!install_direction())return result;
    result.preflight_stage=19;
    if(!smokepreparation::prepare_launch(source,request.original_preparation_serial)){
        result.direction_restored=restore_direction();return result;
    }
    result.preflight_stage=20;
    result.executed_ns=weaponhand::clock_ns(nullptr);
    if(!request.executable_at(result.executed_ns,request.identity,owner.binding_token(),
        request.motion_epoch,request.reference_generation)){
        result.direction_restored=restore_direction();return result;
    }
    journal.active=true;
    // invoke() already contains the existing normal action/restoration route.
    // Its original result and nested functions remain untouched by this owner.
    journal.returned=smoke_original::call(source,journal.notify,nullptr,0,nullptr,&journal.dispatched);
    journal.active=false;
    result.direction_restored=restore_direction();result.dispatched=journal.dispatched;result.returned=journal.returned;
    result.projectile=journal.projectile;result.origins=journal.origins;result.velocities=journal.velocities;
    result.throws=journal.throws;result.smoke_throws=journal.smoke_throws;result.consumes=journal.consumes;result.notifications=journal.notifications;result.inits=journal.inits;
    bool empty=true;for(auto const& p:journal.pending)if(p.live)empty=false;
    if(journal.projectile.address)result.initial_state_matches=read_launch(result);
    result.original_once=result.direction_restored&&journal.returned&&!journal.fault&&empty&&journal.projectile.address&&
        journal.origins==1&&journal.velocities==1&&journal.throws==1&&journal.smoke_throws==1&&journal.consumes==1&&journal.notifications==1&&journal.inits==1&&result.initial_state_matches;
    return result;
}
void service() noexcept {
    if(weaponhand::current_gt(nullptr)&&!journal.active&&outstanding.load(std::memory_order_acquire)&&
       nativeequipment::restoration_quiet())restore_direction();
}
void reset_after_drain() noexcept {
    if(!outstanding.load(std::memory_order_acquire))journal={};
}
} // namespace smokelaunch
