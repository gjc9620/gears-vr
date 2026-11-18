// Private v1 participant client. Native owns arbitration and callback-module
// pinning; this module retains Native until exact-cookie quiescence. No engine
// calls, IK writes, inventory mutations, or logging occur inside callbacks.
namespace weaponhand {
namespace abi=gears_weapon_hand;
namespace meta=gripdamage_metadata;
using U32=std::uint32_t;
struct HoldBinding {
    abi::ObjectIdentity world{},controller{},pawn{},weapon{};std::uint64_t generation{};bool known{};
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL)
    std::uint64_t native_cookie{},native_started{},world_generation{};
#endif
};
HoldBinding held_binding; // GT-only; created by an actual equipment commit.
std::uint32_t current_gt(void*) noexcept {
    const auto id=game_thread.load(std::memory_order_acquire);
    return id&&id==GetCurrentThreadId()?id:0;
}
std::int64_t clock_ns(void*) noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
template<std::size_t N>bool core_type(meta::Reader& r,U32 slot,wchar_t const (&label)[N]) noexcept {
    if(!r.named(slot,label))return false;
    auto const& id=r.result.identities[slot];U32 package{};
    return r.path(id.outer,package)&&r.named(package,L"Core")&&!r.result.identities[package].outer;
}
bool object(meta::Reader& r,abi::ObjectIdentity const& expected) noexcept {
    if(!expected.address)return abi::identity_valid(expected,true);
    U32 slot{};if(!r.path(expected.address,slot))return false;
    auto const& id=r.result.identities[slot];
    const abi::ObjectIdentity actual{id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};
    auto const& text=r.result.name_text[id.name_slot];
    return abi::same(actual,expected)&&!(text.length>=9&&!wmemcmp(text.units.data(),L"Default__",9));
}
template<std::size_t N>bool derives(meta::Reader& r,uptr object_address,wchar_t const (&label)[N],
                                  wchar_t const* package,bool* known=nullptr) noexcept {
    if(known)*known=false;
    U32 slot{};if(!r.path(object_address,slot))return false;
    auto cls=r.result.identities[slot].klass;std::array<uptr,meta::max_parents> seen{};U32 count{};
    while(cls){
        if(count==seen.size())return false;
        for(U32 i=0;i<count;++i)if(seen[i]==cls)return false;
        seen[count++]=cls;meta::Structure structure{};U32 type{},outer{};
        if(!r.structure(cls,structure)||!r.path(r.result.identities[structure.identity].klass,type)||
           !core_type(r,type,L"Class"))return false;
        if(r.named(structure.identity,label)){
            if(!r.path(r.result.identities[structure.identity].outer,outer))return false;
            auto const& p=r.result.identities[outer];auto const& text=r.result.name_text[p.name_slot];
            const auto length=wcslen(package);
            const auto match=!p.outer&&!p.name.number&&text.length==length&&!wmemcmp(text.units.data(),package,length+1);
            if(known)*known=match;
            return match;
        }
        cls=structure.super;
    }
    if(known)*known=true;
    return false;
}
struct BoolField {meta::Property property{};uptr object{};U32 owner{},mask{},bits{};};
template<std::size_t N,std::size_t K>bool field(meta::Reader& r,uptr object_address,wchar_t const (&label)[N],
                                             wchar_t const (&kind)[K],U32 bytes,BoolField& out) noexcept {
    U32 object_slot{};if(!r.path(object_address,object_slot))return false;
    auto cls=r.result.identities[object_slot].klass;U32 extent{},depth{};
    std::array<uptr,meta::max_parents> parents{};
    while(cls){
        if(depth==parents.size())return false;
        for(U32 i=0;i<depth;++i)if(parents[i]==cls)return false;
        parents[depth++]=cls;meta::Structure s{};if(!r.structure(cls,s))return false;
        if(depth==1)extent=s.extent;
        auto property_address=s.children;std::array<uptr,meta::max_search> seen{};U32 count{};
        weapon_metadata_parse::Walk traversal;
        while(property_address){
            if(count==seen.size())return false;
            if(traversal.seen(property_address,seen.data(),count))return false;
            seen[count++]=property_address;meta::Identity id{};meta::NameText text{};
            if(!meta::raw_identity(r.result.objects,r.result.names,property_address,id,text))return false;
            if(meta::exact_text(text,label)){
                BoolField value{};value.object=object_address;value.owner=s.identity;
                if(id.name.number||!r.property(property_address,value.property)||
                   r.result.identities[value.property.identity].outer!=cls||!core_type(r,value.property.type,kind)||
                   value.property.dimension!=1||value.property.element_bytes!=bytes||
                   !meta::property_fits(value.property,s.extent)||!meta::property_fits(value.property,extent))return false;
                out=value;return true;
            }
            if(!r.at(property_address,0x68,property_address))return false;
        }
        cls=s.super;
    }
    return false;
}
template<std::size_t N>bool boolean(meta::Reader& r,uptr address,wchar_t const (&label)[N],BoolField& out) noexcept {
    BoolField value{};
    if(!field(r,address,label,L"BoolProperty",4,value)||
       !r.at(r.result.identities[value.property.identity].address,0xd0,value.mask)||!value.mask||(value.mask&(value.mask-1))||
       !r.at(address,value.property.offset,value.bits))return false;
    out=value;return true;
}
template<std::size_t N>bool read_pointer(meta::Reader& r,uptr address,wchar_t const (&label)[N],
                                       meta::PointerField& out,bool component=false) noexcept {
    BoolField binding{};meta::PointerField value{};
    if(!(component?field(r,address,label,L"ComponentProperty",8,binding):field(r,address,label,L"ObjectProperty",8,binding))||
       !r.at(address,binding.property.offset,value.value))return false;
    value.property=binding.property;value.owner=binding.owner;out=value;return true;
}
template<std::size_t N>bool pointer(meta::Reader& r,uptr address,wchar_t const (&label)[N],
                                  uptr expected,meta::PointerField& out,bool component=false) noexcept {
    return read_pointer(r,address,label,out,component)&&out.value==expected;
}
#include "weapon_hand_pawn_read.inl"
#include "weapon_hand_binding_read.inl"
#include "weapon_hand_aim_read.inl"
// Fresh reflection is intentionally bounded and read-only. It does not use
// get<T>'s unreadable-as-zero fallback, nor vrequip::snapshot's engine queries.
bool observe_game(abi::Frame const& f,abi::GripGameState& out,bool candidates,bool support_mount=true) noexcept {
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
    if(!weaponrestore::healthy())return false;
#endif
    out={};if(!enabled.load(std::memory_order_acquire)||!current_gt(nullptr))return false;
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
    // The old trial has its own support latch. Never report an empty hand
    // while that independent driver is active; migrate it before coexistence.
    if(vrequip::mesh_follow_active_visible.load(std::memory_order_acquire))return false;
#endif
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
    if(vrequip::lancer_magazine_active_visible.load(std::memory_order_acquire))return false;
#endif
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    // No engine calls or writes occur during this local metadata discovery.
    // Reuse the first identity/name already recorded in THIS Snapshot, then
    // validate every one freshly below before exposing any result/proposal.
    r.reuse_initial_identities=true;
    if(!meta::tables(base,schema.objects,schema.names)||!object(r,f.world)||!object(r,f.controller)||
       !object(r,f.pawn)||!object(r,f.mesh)||!object(r,f.weapon)||
       !derives(r,f.controller.address,L"WarPC",L"WarfareGame")||
       !derives(r,f.pawn.address,L"WarPawn",L"WarfareGame")||
       !derives(r,f.world.address,L"WorldInfo",L"Engine")||
       !derives(r,f.mesh.address,L"SkeletalMeshComponent",L"Engine"))return false;
    std::array<meta::PointerField,6> links{};PawnObserveFields pawn_fields;ControllerObserveFields controller_fields;WeaponObserveFields weapon_fields;
    if(!controller_fields.begin(r,f.controller.address)||
       !controller_fields.pointer(r,ControllerObserveFields::pawn_link,f.pawn.address,links[0])||
       !controller_fields.pointer(r,ControllerObserveFields::world_link,f.world.address,links[1])||
       !pawn_fields.begin(r,f.pawn.address)||
       !pawn_fields.pointer(r,PawnObserveFields::controller_link,f.controller.address,links[2])||
       !pawn_fields.pointer(r,PawnObserveFields::mesh_link,f.mesh.address,links[3])||
       !pawn_fields.pointer(r,PawnObserveFields::weapon_link,f.weapon.address,links[4]))return false;
    if(f.weapon.address&&(!derives(r,f.weapon.address,L"WarWeapon",L"WarfareGame")||
       !weapon_fields.begin(r,f.weapon.address)||
       !weapon_fields.pointer(r,WeaponObserveFields::instigator_link,f.pawn.address,links[5])))return false;
    std::array<BoolField,6> flags{};
    const std::array<uptr,4> actors{f.world.address,f.controller.address,f.pawn.address,f.weapon.address};
    for(std::size_t i=0;i<actors.size();++i)if(actors[i]&&
       (!(i==2?pawn_fields.boolean(r,PawnObserveFields::deleted_flag,flags[i]):i==1?controller_fields.boolean(r,ControllerObserveFields::deleted_flag,flags[i]):i==3?weapon_fields.boolean(r,WeaponObserveFields::deleted_flag,flags[i]):boolean(r,actors[i],L"bDeleteMe",flags[i]))||
        (flags[i].bits&flags[i].mask)))return false;
    if(!pawn_fields.boolean(r,PawnObserveFields::switching_flag,flags[4])||
       !pawn_fields.boolean(r,PawnObserveFields::no_firing_flag,flags[5]))return false;

#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL)
    // This optional runtime comparison belongs to the Native-equipment host,
    // which owns log_file. Standalone metadata readers have no runtime logger.
    static const bool compare_bindings=[](){wchar_t flag[4]{},mute[4]{},behind[4]{};return
        GetEnvironmentVariableW(L"GEARSVR_TEST_POSE_BATCH_COMPARE",flag,4)==1&&flag[0]==L'1'&&
        GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",mute,4)==1&&mute[0]==L'1'&&
        GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)==1&&behind[0]==L'1';}();
    static unsigned binding_comparisons{};
    if(compare_bindings&&binding_comparisons<12){std::array<meta::PointerField,3> old{};BoolField pc_deleted{},weapon_deleted{};
        bool equal=pointer(r,f.controller.address,L"Pawn",f.pawn.address,old[0])&&
            pointer(r,f.controller.address,L"WorldInfo",f.world.address,old[1])&&boolean(r,f.controller.address,L"bDeleteMe",pc_deleted)&&
            meta::same_property(old[0].property,links[0].property)&&old[0].value==links[0].value&&
            meta::same_property(old[1].property,links[1].property)&&old[1].value==links[1].value&&
            meta::same_property(pc_deleted.property,flags[1].property)&&pc_deleted.mask==flags[1].mask&&pc_deleted.bits==flags[1].bits;
        if(equal&&f.weapon.address)equal=pointer(r,f.weapon.address,L"Instigator",f.pawn.address,old[2])&&
            boolean(r,f.weapon.address,L"bDeleteMe",weapon_deleted)&&meta::same_property(old[2].property,links[5].property)&&
            old[2].value==links[5].value&&meta::same_property(weapon_deleted.property,flags[3].property)&&
            weapon_deleted.mask==flags[3].mask&&weapon_deleted.bits==flags[3].bits;
        if(log_file){fwprintf(log_file,L"binding_field_batch_compare sample=%u equal=%u weapon=%u\n",binding_comparisons,unsigned(equal),unsigned(f.weapon.address!=0));fflush(log_file);}
        ++binding_comparisons;if(!equal)return false;
    }
#endif
    abi::GripGameState game{};game.known=true;
    game.main_hand_held=f.weapon.address&&vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        held_binding.known&&held_binding.generation&&held_binding.generation==vrequip::model.hold_generation()&&
        abi::same(held_binding.world,f.world)&&abi::same(held_binding.controller,f.controller)&&
        abi::same(held_binding.pawn,f.pawn)&&abi::same(held_binding.weapon,f.weapon);
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL)
    game.main_hand_held=game.main_hand_held&&held_binding.native_cookie==f.key.registration_cookie&&
        held_binding.native_started==f.key.native_started_qpc&&held_binding.world_generation==f.key.world_generation;
#endif
    game.animation_owns_hands=vrequip::model.animation_owns_hands();
    game.busy=vrequip::model.switch_pending()||(flags[4].bits&flags[4].mask)||(flags[5].bits&flags[5].mask);
    if(f.weapon.address){bool known{};
        const auto grenade=derives(r,f.weapon.address,L"Weap_Grenade",L"WarfareGame",&known);
        if(!known)return false;
        game.kind=grenade?abi::WeaponKind::hand_thrown:abi::WeaponKind::firearm;
    }
    // Until an actual per-asset controller/palm mount is certified, no support
    // candidate is invented from temporary mesh-follow calibration.
    game.mount_known=false;
    // Close the local search binding before all original final rereads.
    r.reuse_initial_identities=false;
    if(!pawn_fields.finish(r))return false;
    const auto identities=schema.identity_count,structures=schema.structure_count;
    for(U32 i=0;i<identities;++i){U32 slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(U32 i=0;i<structures;++i){meta::Structure fresh{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,fresh)||
           !meta::same_structure(schema.structures[i],fresh))return false;
    }
    for(auto const& link:links)if(link.property.element_bytes){
        meta::Property fresh{};
        if(!r.property(schema.identities[link.property.identity].address,fresh)||!meta::same_property(fresh,link.property))return false;
    }
    const std::array<uptr,6> link_objects{f.controller.address,f.controller.address,f.pawn.address,
        f.pawn.address,f.pawn.address,f.weapon.address};
    for(std::size_t i=0;i<links.size();++i)if(links[i].property.element_bytes){uptr value{};
        if(!r.at(link_objects[i],links[i].property.offset,value)||value!=links[i].value)return false;
    }
    for(auto const& flag:flags)if(flag.mask){meta::Property fresh{};U32 mask{},bits{};
        const auto property=schema.identities[flag.property.identity].address;
        if(!r.property(property,fresh)||!meta::same_property(fresh,flag.property)||!r.at(property,0xd0,mask)||mask!=flag.mask||
           !r.at(flag.object,flag.property.offset,bits)||(bits&mask)!=(flag.bits&mask))return false;
    }
    meta::Table objects{},names{};
    if(!meta::tables(base,objects,names)||!meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names))return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    // The adapter's boolean current() witness consumes actor/ownership/flag
    // liveness, never a support mount or contact candidate. Full observations
    // AND existing R2/source consumers retain their original live mount.
    if(support_mount&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&game.kind==abi::WeaponKind::firearm){
        if(derives(r,f.weapon.address,L"Weap_Boomer_base",L"WarfareGame"))boomshot_runtime::fill_support_mount(f,game);
        else if(derives(r,f.weapon.address,L"Weap_Bow_base",L"WarfareGame"))torquebow_runtime::fill_support_mount(f,game);
        else if(derives(r,f.weapon.address,L"Weap_LocustPistol",L"WarfareGame"))boltok_runtime::fill_support_mount(f,game);
        else nativelancerfollow::fill_support_mount(f,game);
    }
    if(candidates&&game.main_hand_held){
        // Composition selects one producer. A foreign weapon's pending state
        // cannot contaminate this source; each selected producer still proves
        // its own geometry, identity, currentness and action prerequisites.
        if(derives(r,f.weapon.address,L"Weap_LocustAssaultRifle",L"WarfareGame"))hammerburst_runtime::propose(f,game);
        else if(derives(r,f.weapon.address,L"Weap_SniperRifle",L"WarfareGame"))longshot_runtime::propose(f,game);
        else if(derives(r,f.weapon.address,L"Weap_Bow_base",L"WarfareGame"))torquebow_runtime::propose(f,game);
        else if(derives(r,f.weapon.address,L"Weap_COGPistol",L"WarfareGame"))snub_runtime::propose(f,game);
        else if(derives(r,f.weapon.address,L"Weap_LocustPistol",L"WarfareGame"))boltok_runtime::propose(f,game);
        else if(derives(r,f.weapon.address,L"Weap_Shotgun",L"WarfareGame"))gnasher_runtime::propose(f,game);
        else if(derives(r,f.weapon.address,L"Weap_Boomer_base",L"WarfareGame"))boomshot_runtime::propose(f,game);
        else if(derives(r,f.weapon.address,L"Weap_AssaultRifle",L"WarfareGame"))lancerreload_contact::propose(f,game);
    }

#else
    (void)candidates;(void)support_mount;
#endif
    out=game;return true;
}
bool observe(void*,abi::Frame const& f,std::int64_t,abi::GripGameState& out) noexcept {
    return observe_game(f,out,true);
}
bool observe_source(void*,abi::Frame const& f,std::int64_t,abi::GripGameState& out) noexcept {
    // Source-current witnesses and R2's metadata prerequisites use this.
    // Keep every fresh reflection,
    // identity, ownership/flag and final reread above. Repeated display checks
    // must not reenter Native contact observation or calculate new proposals.
    // R2's before/after comparison still consumes the original support mount.
    return observe_game(f,out,false);
}
// Existing logical-hold checks consume no support mount or contact geometry.
// Keep their full fresh actor/ownership/flag reflection and closing rereads,
// then expose only these fields so geometry cannot escape into an action or
// before/after comparison. R2 keeps the complete observe_source API above.
struct HoldSourceObservation {
    bool known{},main_hand_held{},busy{},animation_owns_hands{};
    abi::WeaponKind kind{};
};
bool observe_hold_source(void*,abi::Frame const& f,std::int64_t,HoldSourceObservation& out) noexcept {
    out={};abi::GripGameState game{};
    if(!observe_game(f,game,false,false))return false;
    out={game.known!=0,game.main_hand_held!=0,game.busy!=0,game.animation_owns_hands!=0,game.kind};
    return true;
}
#include "weapon_source_validation_scope.inl"
bool observe_liveness(void*,abi::Frame const& f,std::int64_t,abi::GripGameState& out) noexcept {
    // HandClient discards this optional adapter's game payload; it is ONLY a
    // source-current boolean witness. Keep all fresh metadata/actor/flag and
    // closing rereads, without geometry or new contact proposals. Explicit
    // observe_source and full Frame/game readers above are unchanged.
    return source_validation::observe(f,out);
}
// Only actual successful unholster/equip commits may call this. It never
// adopts an existing raw-address latch just because a Native frame arrived.
void equipment_hold_committed(uptr pc,gears_weapon::EquipmentSnapshot const& game) noexcept {
    held_binding={};
    if(!current_gt(nullptr)||!vrequip::model.held_for(game.world,game.pawn,game.current_weapon))return;
    const auto generation=vrequip::model.hold_generation();if(!generation)return;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return;
    const auto identify=[&](uptr address,abi::ObjectIdentity& out){
        meta::Identity id{};meta::NameText text{};
        if(!meta::raw_identity(schema.objects,schema.names,address,id,text))return false;
        out={id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};return true;
    };
    abi::Frame frame{};meta::PointerField mesh{};
    if(!identify(game.world,frame.world)||!identify(pc,frame.controller)||!identify(game.pawn,frame.pawn)||
       !identify(game.current_weapon,frame.weapon)||!read_pointer(r,game.pawn,L"Mesh",mesh,true)||
       !identify(mesh.value,frame.mesh))return;
    abi::GripGameState verified{};
    if(!observe_liveness(nullptr,frame,clock_ns(nullptr),verified)||
       !vrequip::model.held_for(game.world,game.pawn,game.current_weapon)||vrequip::model.hold_generation()!=generation)return;
    held_binding={frame.world,frame.controller,frame.pawn,frame.weapon,generation,true};
}
void committed(void*,abi::Frame const& frame,abi::Decision const& decision) noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    lancerreload_contact::committed(frame,decision);
    longshot_runtime::committed(frame,decision);
    torquebow_runtime::committed(frame,decision);
    snub_runtime::committed(frame,decision);
    boltok_runtime::committed(frame,decision);
    gnasher_runtime::committed(frame,decision);
    boomshot_runtime::committed(frame,decision);
#else
    (void)frame;(void)decision;
#endif
}
bool hold_generation_current(void*,abi::Frame const& f,std::uint64_t generation) noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL)
    const auto& held=held_binding;
    return current_gt(nullptr)&&generation&&held.known&&held.generation==generation&&
        vrequip::model.hold_generation()==generation&&vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        abi::same(held.world,f.world)&&abi::same(held.controller,f.controller)&&abi::same(held.pawn,f.pawn)&&abi::same(held.weapon,f.weapon)&&
        held.native_cookie==f.key.registration_cookie&&held.native_started==f.key.native_started_qpc&&held.world_generation==f.key.world_generation;
#else
    (void)f;(void)generation;return false;
#endif
}
abi::HandClient client({nullptr,observe,current_gt,clock_ns,committed,observe_liveness,hold_generation_current});
HMODULE native_module{};abi::Unregister host_unregister{};bool native_quiescent{};
bool call_register(abi::Register fn,abi::Registration const* registration,abi::RegistrationResult* result,abi::Reply& reply) noexcept {
    __try{reply=fn(registration,result);return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool call_unregister(abi::Unregister fn,std::uint64_t cookie,abi::Reply& reply) noexcept {
    __try{reply=fn(cookie);return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool executable(FARPROC address,HMODULE owner) noexcept {
    MEMORY_BASIC_INFORMATION memory{};
    if(!address||VirtualQuery(reinterpret_cast<void const*>(address),&memory,sizeof(memory))!=sizeof(memory)||
       memory.State!=MEM_COMMIT||memory.Type!=MEM_IMAGE||memory.AllocationBase!=owner||
       (memory.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const auto access=memory.Protect&0xffu;
    return access==PAGE_EXECUTE||access==PAGE_EXECUTE_READ||access==PAGE_EXECUTE_READWRITE||access==PAGE_EXECUTE_WRITECOPY;
}
abi::Reply start(HMODULE requested) noexcept {
    if(current_gt(nullptr))return abi::Reply::reentrant;
    if(!enabled.load(std::memory_order_acquire)||!requested||requested==module)return abi::Reply::unavailable;
    if(client.phase()!=abi::ClientPhase::off||native_module)return abi::Reply::busy;
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL)
    // An earlier equipment tick can outlive Native unregister. Fence its
    // actual configuration borrow, not every unrelated engine dispatch.
    nativeequipment::ConfigurationWrite configuration;
    if(!configuration.held)return abi::Reply::busy;
#endif
    HMODULE held{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(requested),&held))return abi::Reply::unavailable;
    const auto register_address=GetProcAddress(held,abi::register_export),unregister_address=GetProcAddress(held,abi::unregister_export);
    if(held!=requested||!executable(register_address,held)||!executable(unregister_address,held)){
        FreeLibrary(held);return abi::Reply::unavailable;
    }
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
    if(!executable(GetProcAddress(held,gears_weapon_call::attach_export),held)){FreeLibrary(held);return abi::Reply::unavailable;}
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL)
    if(!executable(GetProcAddress(held,gears_weapon_main_mount_provider::attach_export),held)){
        FreeLibrary(held);return abi::Reply::unavailable;
    }
#endif
#if defined(GEARS_PLAYGROUND_BODY_MAIN_MOUNT_PROVIDER_TRIAL)
    if(!executable(GetProcAddress(held,gears_body_main_mount_provider::attach_export),held)){
        FreeLibrary(held);return abi::Reply::unavailable;
    }
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
    if(!executable(GetProcAddress(held,gears_weapon_action::attach_export),held)){FreeLibrary(held);return abi::Reply::unavailable;}
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
    for(auto export_name:{gears_weapon_boundary::revoke_export,gears_weapon_boundary::open_export,gears_weapon_boundary::close_export})
        if(!executable(GetProcAddress(held,export_name),held)){FreeLibrary(held);return abi::Reply::unavailable;}
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL)
    // Body-slot calibration gates slot gestures, not registration or free
    // hands. The manual worker refreshes the real measurement after connect.
    // An invalid calibration still fails closed in EquipmentInput::next.
    nativeequipment::configure_native(held);
#endif
    FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user)){FreeLibrary(held);return abi::Reply::unavailable;}
    const auto birth=(std::uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;
    const auto beginning=client.begin_connect({birth,GetCurrentProcessId()});
    if(beginning!=abi::Reply::accepted){FreeLibrary(held);return beginning;}
    native_module=held;host_unregister=reinterpret_cast<abi::Unregister>(unregister_address);native_quiescent=false;
    auto registration=client.registration();abi::RegistrationResult result{};abi::Reply reply=abi::Reply::invalid;
    const auto completed=call_register(reinterpret_cast<abi::Register>(register_address),&registration,&result,reply);
    if(!completed||reply!=abi::Reply::accepted){
        static unsigned registration_failure_reports{};
        if(log_file&&registration_failure_reports++<8){
            fwprintf(log_file,L"weapon_hand_parent_registration_failure completed=%u reply=%u gt=%u caller_thread=%lu requested=%llx held=%llx pg_module=%llx size=%u abi=%u frame_bytes=%u proposal_bytes=%u decision_bytes=%u reserved=%u result_size=%u result_abi=%u result_cookie=%llu result_native_started=%llu\n",
                unsigned(completed),unsigned(reply),game_thread.load(std::memory_order_acquire),GetCurrentThreadId(),
                reinterpret_cast<uptr>(requested),reinterpret_cast<uptr>(held),reinterpret_cast<uptr>(module),
                registration.size,registration.abi_version,registration.frame_bytes,registration.proposal_bytes,
                registration.decision_bytes,registration.reserved,result.size,result.abi_version,
                result.cookie,result.native_started_qpc);
            const uptr parent_callback_addresses[]={reinterpret_cast<uptr>(registration.prepare),reinterpret_cast<uptr>(registration.commit),reinterpret_cast<uptr>(registration.invalidate)};
            for(unsigned index=0;index<3;++index){MEMORY_BASIC_INFORMATION page{};
                const auto queried=VirtualQuery(reinterpret_cast<void const*>(parent_callback_addresses[index]),&page,sizeof page);
                fwprintf(log_file,L"weapon_hand_parent_callback_page index=%u address=%llx query_bytes=%zu allocation=%llx state=%lu protect=%lu type=%lu\n",
                    index,parent_callback_addresses[index],queried,reinterpret_cast<uptr>(page.AllocationBase),page.State,page.Protect,page.Type);
            }
            fflush(log_file);
        }
    }

    const auto outcome=client.connected(reply,result,completed);
    if(client.phase()==abi::ClientPhase::off){FreeLibrary(native_module);native_module=nullptr;host_unregister=nullptr;}
#if defined(GEARS_PLAYGROUND_HAND_RIG_TRACKING_TRIAL) || defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    if(outcome==abi::Reply::accepted){
        const auto attached=handrig::attach(held,birth,game_thread.load(std::memory_order_acquire));
        if(attached!=abi::Reply::accepted){client.begin_disconnect();return attached;}
    }
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
    if(outcome==abi::Reply::accepted){
        auto attached=weaponcall::attach(held);
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
        if(attached==abi::Reply::accepted)attached=weaponaction::attach(held);
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
        if(attached==abi::Reply::accepted)attached=weaponboundary::attach(held);
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL)
        // Optional pure-data child shares the exact parent cookie/pin/drain.
        // On any partial attach failure retain the parent for normal stop.
        if(attached==abi::Reply::accepted)attached=mainmountprovider::attach(held);
#endif
#if defined(GEARS_PLAYGROUND_BODY_MAIN_MOUNT_PROVIDER_TRIAL)
        if(attached==abi::Reply::accepted)attached=bodymountprovider::attach(held);
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        if(attached==abi::Reply::accepted)attached=handrender::attach(held);
#endif
        if(attached!=abi::Reply::accepted)client.begin_disconnect();
        return attached;
    }
#endif
    return outcome;
}
abi::Reply stop() noexcept {
#if defined(GEARS_PLAYGROUND_HAND_RIG_TRACKING_TRIAL) || defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    if(current_gt(nullptr))return abi::Reply::reentrant;
    handrig::seal();
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
    if(current_gt(nullptr))return abi::Reply::reentrant;
    weaponboundary::seal();
#endif
    const auto begun=client.begin_disconnect();
    if(begun==abi::Reply::quiescent){
#if defined(GEARS_PLAYGROUND_HAND_RIG_TRACKING_TRIAL) || defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
        if(!handrig::quiescent()||!handrig::detached())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
        if(!weaponrestore::healthy())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
        if(!weaponboundary::local_quiescent())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
        if(!weaponaction::local_quiescent())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
        if(!weaponcall::detached())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
        if(!weaponaction::detached())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
        if(!weaponboundary::detached())return abi::Reply::draining;
#endif
        if(native_module){
            if(!FreeLibrary(native_module))return abi::Reply::unavailable;
            native_module=nullptr;host_unregister=nullptr;native_quiescent=false;
        }
        return begun;
    }
    if(begun!=abi::Reply::draining||!host_unregister||!native_module)return begun;
    const auto cookie=client.cookie();
    if(!native_quiescent){abi::Reply reply=abi::Reply::invalid;
        if(!call_unregister(host_unregister,cookie,reply))return abi::Reply::unavailable;
        if(reply!=abi::Reply::quiescent)return reply;
        native_quiescent=true;
    }
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
    if(!weaponcall::local_quiescent())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
    if(!weaponaction::local_quiescent())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
    if(!weaponboundary::local_quiescent())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
    if(!weaponrestore::healthy())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_HAND_RIG_TRACKING_TRIAL) || defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    if(!handrig::quiescent())return abi::Reply::draining;
#endif
    const auto finished=client.finish_disconnect(cookie);
    if(finished!=abi::Reply::quiescent)return finished;
#if defined(GEARS_PLAYGROUND_HAND_RIG_TRACKING_TRIAL) || defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    if(!handrig::detached())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
    if(!weaponcall::detached())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
    if(!weaponaction::detached())return abi::Reply::draining;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
    if(!weaponboundary::detached())return abi::Reply::draining;
#endif
    if(!FreeLibrary(native_module))return abi::Reply::unavailable;
    native_module=nullptr;host_unregister=nullptr;native_quiescent=false;return abi::Reply::quiescent;
}
} // namespace weaponhand
