// Complete the engine's pending gun transform AFTER the visual callback and
// outer mesh Tick. PG remains the sole owner/writer of component fields.
// No actor-wide update, bone write, new input or action permit is supplied.
namespace lancer_transform_finish {
namespace wh=gears_weapon_hand;
struct Pending {wh::ObjectIdentity component{},asset{},weapon{};uint64_t serial{},tick{},hold{},cookie{};int64_t sync_ns{};bool active{};} pending;
struct Layout {
    binding::Proof proof{};uintptr_t component_class{},weapon_class{},pawn_class{};
    binding::Property mesh{},weapon{},asset{},needs{},attached{},translation{},rotation{};
    bool ready{};
} layout;
std::atomic<uint64_t> attempts{},completed{},rejected{},already_current{};
std::atomic<uint64_t> source_reject_tick{},source_reject_serial{},source_reject_age_ns{};
std::array<std::atomic<uint64_t>,6> rejection_stage{};
bool identity(wh::ObjectIdentity const& expected) noexcept {
    gears_anchor::Identity actual{};
    return binding::identity(expected.address,actual,true)&&wh::same(expected,rig_tracking_live::object(actual));
}
bool fields(Pending const& p,std::array<unsigned char,24>& values,uintptr_t& table,uintptr_t& method) noexcept {
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
    if(!lock.owns_lock()||!binding::metadata_current()||!identity(p.component)||!identity(p.asset)||!identity(p.weapon))return false;
    const auto& f=endpoint_value;
    if(!layout.ready||layout.component_class!=p.component.klass||layout.weapon_class!=p.weapon.klass||layout.pawn_class!=f.pawn.klass){
        layout={};layout.component_class=p.component.klass;layout.weapon_class=p.weapon.klass;layout.pawn_class=f.pawn.klass;
        auto& proof=layout.proof;
        if(!binding::property(proof,p.weapon.klass,L"Mesh",L"MeshPointer",8,L"MeshComponent",layout.mesh)||
           !binding::property(proof,f.pawn.klass,L"Weapon",L"ObjectProperty",8,L"Weapon",layout.weapon)||
           !binding::property(proof,p.component.klass,L"SkeletalMesh",L"ObjectProperty",8,L"SkeletalMesh",layout.asset)||
           !binding::property(proof,p.component.klass,L"bNeedsUpdateTransform",L"BoolProperty",4,nullptr,layout.needs)||
           !binding::property(proof,p.component.klass,L"bAttached",L"BoolProperty",4,nullptr,layout.attached)||
           !binding::property(proof,p.component.klass,L"Translation",L"StructProperty",12,nullptr,layout.translation)||
           !binding::property(proof,p.component.klass,L"Rotation",L"StructProperty",12,nullptr,layout.rotation))return false;
        unsigned needs_mask{},attached_mask{};
        if(layout.needs.offset!=0x80||layout.attached.offset!=0x80||
           !binding::source_field(proof,needs_mask,layout.needs.id.address,0xd0)||needs_mask!=8||
           !binding::source_field(proof,attached_mask,layout.attached.id.address,0xd0)||attached_mask!=1)return false;
        layout.ready=true;
    }
    uintptr_t mesh{},weapon{},asset{};uintptr_t module{};
    return binding::current(layout.proof)&&identity(f.pawn)&&
        binding::field(mesh,p.weapon.address,layout.mesh.offset)&&mesh==p.component.address&&
        binding::field(weapon,f.pawn.address,layout.weapon.offset)&&weapon==p.weapon.address&&
        binding::field(asset,p.component.address,layout.asset.offset)&&asset==p.asset.address&&
        native::read_memory(values.data(),p.component.address+layout.translation.offset,12)&&
        native::read_memory(values.data()+12,p.component.address+layout.rotation.offset,12)&&
        binding::field(table,p.component.address,0)&&binding::field(method,table,0x240)&&
        wh::weapon_broker_win32::executable_image(method,module)&&module==reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
}
bool current(Pending const& p) noexcept {
    const auto& f=endpoint_value;auto* broker=weapon_hand_runtime::current();
    return !state.captured&&enabled&&!stopping.load()&&endpoint_valid&&p.active&&
        GetCurrentThreadId()==owner_thread.load()&&f.serial==p.serial&&f.tick==p.tick&&
        f.parent.world_generation==committed_world_generation.load()&&
        f.hold_generation==p.hold&&f.parent.registration_cookie==p.cookie&&wh::same(f.weapon,p.weapon)&&
        f.purpose!=endpoint::Purpose::free_hand_visual&&endpoint::fresh(f,rig_tracking_live::clock_ns(),simulator_operator)&&
        broker&&broker->registration_current(p.cookie,f.parent.native_started_qpc);
}
void queue() noexcept {
    // Only actually applied current held-Lancer visual work can nominate the
    // SAME component already certified by the current Native mount response.
    auto const& m=state.body_mount_response.mount;auto const& f=endpoint_value;
    if(state.right_mount!=hands::adapter::MountKind::held_lancer||!endpoint_valid||
       !wh::same(m.weapon,f.weapon)||m.hold_generation!=f.hold_generation||
       m.key.registration_cookie!=f.parent.registration_cookie||!wh::identity_valid(m.component)||!wh::identity_valid(m.asset))return;
    pending={m.component,m.asset,m.weapon,f.serial,f.tick,f.hold_generation,f.parent.registration_cookie,f.controllers.sync_ns,true};
}
bool invoke(uintptr_t method,uintptr_t component) noexcept {
    // Original DD608 assembly tail-calls [vtable+0x240] with RCX=this.
    __try {reinterpret_cast<void(__fastcall*)(uintptr_t)>(method)(component);return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
void finish(Entry const& e,bool returned,bool original_called) noexcept {
    if(!pending.active||(e.kind!=0&&e.kind!=2))return;
    const auto p=pending;
    // The first opportunity is immediately after the CLOSED Compose and
    // broker callback return. Do not spend the original40ms input deadline
    // waiting for the rest of outer Tick. Outer Tick can finish only NEW
    // engine-pending work on this same still-current endpoint; never a replay.
    if(e.kind==2)pending={};
    const auto refuse=[](unsigned stage){++rejected;++rejection_stage[stage];};
    const bool entry=e.kind==0?e.invocation==endpoint_value.compose:e.invocation==p.tick;
    if(!returned||!original_called||!e.selected||!entry||e.thread!=GetCurrentThreadId()){refuse(0);return;}
    if(!current(p)){
        source_reject_tick=p.tick;source_reject_serial=p.serial;
        const auto now=rig_tracking_live::clock_ns();source_reject_age_ns=now>=p.sync_ns?
            uint64_t(now-p.sync_ns):UINT64_MAX;
        refuse(1);return;
    }
    std::array<unsigned char,24> before{},after{};uintptr_t table{},method{},next_table{},next_method{};unsigned flags{};
    if(!fields(p,before,table,method)||!binding::field(flags,p.component.address,0x80)){refuse(2);return;}
    if((flags&9)==1){++already_current;return;}
    if((flags&9)!=9){refuse(3);return;}
    if(!current(p)){refuse(4);return;}
    ++attempts;
    const bool okay=invoke(method,p.component.address)&&current(p)&&fields(p,after,next_table,next_method)&&
        before==after&&table==next_table&&method==next_method;
    if(okay)++completed;else{
        refuse(5);endpoint_valid=false;interaction_valid=false;
        notify_endpoint(gears_committed_hand_consumer::Kind::revoke);
    }
}
}
