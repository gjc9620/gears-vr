// Included inside vrequip. Bounded, fixed-arena test only. A detached mesh is
// not an ownership marker: every observed original mutation ends our lease.
namespace stow {
struct ObjectKey {
    uptr address{},object_class{},outer{};
    unsigned index{},name_index{},name_number{},reserved{};
};
static_assert(sizeof(ObjectKey)==40);
struct Key {
    // WorldInfo, Level, PC, Pawn, InvManager, Pawn.Mesh, Weapon, Weapon.Mesh.
    std::array<ObjectKey,8> objects{};
    std::array<std::uint64_t,4> slots{};
    std::uint64_t epoch{};
    bool operator==(Key const& other) const noexcept {
        return epoch==other.epoch&&slots==other.slots&&
            !std::memcmp(objects.data(),other.objects.data(),sizeof(objects));
    }
};
gears_weapon::WeaponStowLease<Key> lease;
gears_weapon::EquipmentStowPending<Key> pending;
std::atomic<bool> dirty_visible{},pending_visible{},stop_requested{},shutdown_requested{};
std::atomic<unsigned> status{};
std::uint64_t epoch{};
uptr controller{};
ULONGLONG deadline{};
bool mutating{},yielding{};
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
struct InitialSelf {
    Request const* request{};uptr object{},function{};bool armed{};
};
InitialSelf initial_self;
inline bool same_initial_self(InitialSelf const& a,InitialSelf const& b) noexcept {
    return a.request==b.request&&a.object==b.object&&a.function==b.function&&a.armed==b.armed;
}
struct InitialSelfScope {
    InitialSelf saved;
    explicit InitialSelfScope(Request const* request) noexcept:saved(initial_self){
        initial_self={request,request->object,request->function,true};
    }
    ~InitialSelfScope(){initial_self=saved;}
};
inline bool consume_initial_self(Request const* request) noexcept {
    const auto gt=game_thread.load(std::memory_order_acquire);
    if(!gt||GetCurrentThreadId()!=gt||!request||!initial_self.armed||initial_self.request!=request||
       initial_self.object!=request->object||initial_self.function!=request->function)return false;
    initial_self.armed=false;return true;
}
#endif
gears_weapon::WeaponEquipmentModel pending_model;
unsigned pending_slot=4;
unsigned pending_down_mask{};
bool pending_from_grip{};
ULONGLONG pending_until{};
std::array<unsigned char,44> hand_attachment{};
bool have_hand_attachment{};

bool yield_hands(const wchar_t* reason){
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    const bool clean=handstream::yield_for_equipment(reason);
    if(log_file){fwprintf(log_file,L"hand_equipment_handback reason=%ls clean=%u\n",reason,unsigned(clean));fflush(log_file);}
    return clean;
#else
    (void)reason;return true;
#endif
}

bool field(uptr object,const wchar_t* label,const wchar_t* kind,void* output,unsigned bytes){
    const auto property=playground::property(object,label);
    const auto cls=get<uptr>(object+0x50);
    if(!registered(property)||!registered(cls)||get<unsigned>(property+0x70)!=1||
       get<unsigned>(property+0x74)!=bytes||
       !playground::named(get<uptr>(property+0x50),kind))return false;
    const auto offset=get<unsigned>(property+0x94),size=get<unsigned>(cls+0x88);
    return offset<=size&&bytes<=size-offset&&copy(output,object+offset,bytes);
}
bool pointer(uptr object,const wchar_t* label,uptr& out,bool component=false){
    return field(object,label,component?L"ComponentProperty":L"ObjectProperty",&out,sizeof(out));
}
bool boolean(uptr object,const wchar_t* label,bool& out){
    unsigned bits{};
    if(!field(object,label,L"BoolProperty",&bits,4))return false;
    const auto mask=get<unsigned>(playground::property(object,label)+0xd0);
    if(!mask||(mask&(mask-1)))return false;
    out=(bits&mask)!=0;return true;
}
bool object_key(uptr object,ObjectKey& key){
    key={};
    if(!registered(object)||playground::deleted(object))return false;
    Name n{};
    key.address=object;
    if(!copy(&key.index,object+8,4)||!copy(&key.outer,object+0x40,8)||
       !copy(&n,object+0x48,8)||!copy(&key.object_class,object+0x50,8)||
       !registered(key.object_class)||!registered(key.outer))return false;
    key.name_index=n.index;key.name_number=n.number;
    return registered(object)&&!playground::deleted(object);
}
bool name_is(Name value,const wchar_t* label){
    if(value.number)return false;
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||names.capacity>4000000||
       value.index>=unsigned(names.count))return false;
    const auto entry=get<uptr>(names.data+size_t(value.index)*8);
    const auto chars=wcslen(label)+1;
    wchar_t text[96]{};
    return chars<=96&&entry&&get<unsigned>(entry)==value.index&&
        copy(text,entry+20,chars*sizeof(wchar_t))&&!std::memcmp(text,label,chars*sizeof(wchar_t));
}
unsigned parameters(playground::Args const& call){
    if(!call.valid)return ~0u;
    unsigned count{};auto p=get<uptr>(call.r.function+0x80);
    for(unsigned n=0;p&&n<128;++n,p=get<uptr>(p+0x68)){
        if(!registered(p))return ~0u;
        if(get<unsigned long long>(p+0x78)&0x80)++count;
        if(n==127)return ~0u;
    }
    return count;
}
bool argument(playground::Args& call,const wchar_t* label,const wchar_t* kind,
              unsigned bytes,unsigned offset,unsigned long long flags){
    const auto p=call.prop(label);
    return call.valid&&registered(p)&&get<unsigned>(p+0x70)==1&&
        get<unsigned>(p+0x74)==bytes&&get<unsigned>(p+0x94)==offset&&
        playground::named(get<uptr>(p+0x50),kind)&&
        (get<unsigned long long>(p+0x78)&0x580)==flags&&
        offset<=call.r.size&&bytes<=call.r.size-offset;
}
bool bool_call(uptr object,const wchar_t* label,bool& out){
    playground::Args call(object,label);
    if(call.r.size!=4||get<unsigned short>(call.r.function+0x108)!=0||parameters(call)!=1||
       !argument(call,L"ReturnValue",L"BoolProperty",4,0,0x580))return false;
    const auto mask=get<unsigned>(call.prop(L"ReturnValue")+0xd0);
    if(!mask||(mask&(mask-1))||!call.run())return false;
    out=(call.output<unsigned>()&mask)!=0;return true;
}
enum class WeaponState:unsigned {unknown,active,firing,burst_delay};
WeaponState weapon_state(uptr weapon){
    playground::Args call(weapon,L"GetStateName");
    if(call.r.size!=8||get<unsigned short>(call.r.function+0x108)!=0||parameters(call)!=1||
       !argument(call,L"ReturnValue",L"NameProperty",8,0,0x580)||!call.run())return WeaponState::unknown;
    const auto value=call.output<Name>();
    if(name_is(value,L"Active"))return WeaponState::active;
    if(name_is(value,L"WeaponFiring"))return WeaponState::firing;
    if(name_is(value,L"BurstDelaying"))return WeaponState::burst_delay;
    return WeaponState::unknown;
}
bool attachment(uptr mesh,uptr weapon_mesh,gears_weapon::StowAttachment& out){
    using A=gears_weapon::StowAttachment;
    out=A::unknown;Array entries{};
    if(!field(mesh,L"Attachments",L"ArrayProperty",&entries,sizeof(entries))||
       entries.count<0||entries.count>entries.capacity||entries.capacity>128||
       (entries.count&&!entries.data))return false;
    const auto property=playground::property(mesh,L"Attachments");
    const auto inner=get<uptr>(property+0xd0),structure=get<uptr>(inner+0xd0);
    if(!registered(inner)||!registered(structure)||
       !playground::named(get<uptr>(inner+0x50),L"StructProperty")||
       get<unsigned>(inner+0x74)!=52||get<unsigned>(structure+0x88)!=52)return false;
    const wchar_t* labels[]={L"Component",L"BoneName",L"RelativeLocation",L"RelativeRotation",L"RelativeScale"};
    const wchar_t* kinds[]={L"ComponentProperty",L"NameProperty",L"StructProperty",L"StructProperty",L"StructProperty"};
    const unsigned offsets[]={0,8,16,28,40},sizes[]={8,8,12,12,12};
    for(unsigned i=0;i<5;++i){
        const auto p=playground::field(structure,labels[i],kinds[i]);
        if(!registered(p)||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=sizes[i]||
           get<unsigned>(p+0x94)!=offsets[i])return false;
    }
    unsigned matches{};std::array<unsigned char,52> found{};
    for(int i=0;i<entries.count;++i){
        std::array<unsigned char,52> value{};uptr component{};
        if(!copy(value.data(),entries.data+size_t(i)*52,value.size()))return false;
        std::memcpy(&component,value.data(),8);
        if(component==weapon_mesh){found=value;++matches;}
    }
    if(!matches){out=A::detached;return true;}
    if(matches!=1){out=A::other;return true;}
    Name bone{};std::memcpy(&bone,found.data()+8,8);
    if(!name_is(bone,L"b_MF_Weapon_R_End")){out=A::other;return true;}
    std::array<unsigned char,44> pose{};std::memcpy(pose.data(),found.data()+8,44);
    if(!have_hand_attachment){hand_attachment=pose;have_hand_attachment=true;}
    out=pose==hand_attachment?A::original_hand:A::other;return true;
}
bool read_state(gears_weapon::StowState<Key>& out){
    out={};
    if(!playground::ready||!playground::automatic||!log_file||!test_environment()||
       controller!=playground::arena_pc||GetCurrentThreadId()!=game_thread.load())return false;
    uptr pawn{},world{},inv{},mesh{},weapon{},weapon_mesh{},owner{};
    if(!pointer(controller,L"Pawn",pawn)||pawn!=playground::arena_pawn||
       !pointer(controller,L"WorldInfo",world)||world!=playground::arena_world||
       !pointer(pawn,L"Controller",owner)||owner!=controller||
       !pointer(pawn,L"InvManager",inv)||!pointer(pawn,L"Mesh",mesh,true)||
       !pointer(pawn,L"Weapon",weapon)||!pointer(weapon,L"Instigator",owner)||owner!=pawn||
       !pointer(weapon,L"Mesh",weapon_mesh,true)||
       !playground::instance(controller,L"WarPC")||!playground::instance(pawn,L"WarPawn")||
       !playground::instance(world,L"WorldInfo")||!playground::instance(inv,L"WarInventoryManager")||
       !playground::instance(mesh,L"SkeletalMeshComponent")||
       !playground::instance(weapon_mesh,L"SkeletalMeshComponent")||
       (!playground::instance(weapon,L"Weap_AssaultRifle")&&
        !playground::instance(weapon,L"Weap_LocustAssaultRifle")))return false;
    const auto level=get<uptr>(world+0x40);
    if(!playground::instance(level,L"Level")||get<uptr>(controller+0x40)!=level||
       get<uptr>(pawn+0x40)!=level)return false;
    const uptr objects[]={world,level,controller,pawn,inv,mesh,weapon,weapon_mesh};
    for(unsigned i=0;i<8;++i)if(!object_key(objects[i],out.key.objects[i]))return false;
    unsigned matches{};
    for(std::uint8_t i=0;i<4;++i){
        uptr item{};if(!read_slot(inv,i,item))return false;
        out.key.slots[i]=item;if(item==weapon)++matches;
    }
    if(matches!=1||!attachment(mesh,weapon_mesh,out.attachment))return false;
    out.key.epoch=epoch;out.ready=true;return true;
}
bool commit_allowed(Key const& expected,ULONGLONG until);
struct Backend {
    Key expected{};
    ULONGLONG commit_until{};
    bool enforce_expected{};
    bool read(gears_weapon::StowState<Key>& out){
        const bool ok=read_state(out)&&(!enforce_expected||out.key==expected);
        if(log_file
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
           &&handstream::diagnostic_event()
#endif
        ){
            fwprintf(log_file,L"equipment_stow_state read=%u ready=%u attachment=%u epoch=%llu "
                L"pawn=%p weapon=%p pawn_mesh=%p weapon_mesh=%p original_hand_pose=",
                unsigned(ok),unsigned(out.ready),unsigned(out.attachment),out.key.epoch,
                reinterpret_cast<void*>(out.key.objects[3].address),
                reinterpret_cast<void*>(out.key.objects[6].address),
                reinterpret_cast<void*>(out.key.objects[5].address),
                reinterpret_cast<void*>(out.key.objects[7].address));
            if(ok&&out.attachment==gears_weapon::StowAttachment::original_hand)
                for(const auto byte:hand_attachment)fwprintf(log_file,L"%02x",unsigned(byte));
            fwprintf(log_file,L"\n");fflush(log_file);
        }
        return ok;
    }
    bool set_holstered(bool value){
        if(!weaponrestore::healthy())return false;
        // Only the same game-thread lease may reach this original helper.
        if(value&&(shutdown_requested.load()||stop_requested.load()))return false;
        playground::Args call(playground::arena_pawn,L"SetWeaponTemporarilyHolstered");
        if(call.r.size!=4||get<unsigned short>(call.r.function+0x108)!=0xffff||parameters(call)!=1||
           !argument(call,L"bHolstered",L"BoolProperty",4,0,0x80))return false;
        const auto mask=get<unsigned>(call.prop(L"bHolstered")+0xd0);
        if(!mask||(mask&(mask-1)))return false;
        call.flag(L"bHolstered",value);
        struct Scope {Scope(){mutating=true;}~Scope(){mutating=false;}} scope;
        if(log_file){fwprintf(log_file,L"equipment_stow_helper phase=before holstered=%u epoch=%llu\n",
            unsigned(value),epoch);fflush(log_file);}
        // Logging can block while an external Stop arrives. Validate after it,
        // using the original request deadline/key, immediately before true.
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        if(log_file){fwprintf(log_file,L"hand_equipment_helper holstered=%u hand_clean=%u\n",
            unsigned(value),unsigned(handstream::equipment_ready()));fflush(log_file);}
        const bool hand_clean=handstream::equipment_ready();
#else
        constexpr bool hand_clean=true;
#endif
        const bool allowed=hand_clean&&(!value||(enforce_expected&&commit_allowed(expected,commit_until)));
        // acquire pre-journals dirty before entering this backend. If no true
        // was attempted, a newly detached mesh must never grant rollback false.
        if(!allowed)lease.revoke();
        bool called=false;
        if(allowed&&weaponrestore::healthy()){
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
            // The exact initial Args hop only. Commit queries above and any
            // subsequent recursive PE/CF cannot borrow this one-shot token.
            InitialSelfScope own_call(&call.r);
#endif
            called=call.run();
        }
        if(log_file){fwprintf(log_file,L"equipment_stow_helper phase=after holstered=%u called=%u epoch=%llu commit_allowed=%u\n",
            unsigned(value),unsigned(called),epoch,unsigned(allowed));fflush(log_file);}
        return called;
    }
};
void publish(){dirty_visible.store(lease.dirty(),std::memory_order_release);}
bool restore(const wchar_t* reason){
    if(!weaponrestore::healthy())return false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_names::Scope names_scope(false);
#endif
    const bool was_dirty=lease.dirty();Backend backend{};
    const bool hands=!was_dirty||yield_hands(reason);
    if(!hands){lease.revoke();stop_requested=true;}
    const bool ok=lease.restore(backend);publish();
    if(was_dirty){
        model.reset();deadline=0;
        status.store(ok?6u:8u,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"equipment_stow_restore reason=%ls clean=%u fault=%u revoked=%u epoch=%llu\n",
            reason,unsigned(!lease.dirty()),unsigned(lease.faulted()),unsigned(lease.revoked()),epoch);fflush(log_file);}
    }
    return ok&&hands;
}
bool relinquish_original_hand(const wchar_t* reason){
    Backend backend{};
    // No handback engine call precedes revocation of the old stow journal.
    const bool ok=lease.relinquish(backend);publish();
    model.reset();deadline=0;
    if(!ok)stop_requested=true;
    const bool hands=yield_hands(reason);
    if(!hands)stop_requested=true;
    status=ok&&hands?6u:8u;
    if(log_file){fwprintf(log_file,L"equipment_stow_relinquish reason=%ls clean=%u hand_clean=%u fault=%u revoked=%u epoch=%llu\n",
        reason,unsigned(!lease.dirty()),unsigned(hands),unsigned(lease.faulted()),unsigned(lease.revoked()),epoch);fflush(log_file);}
    return ok&&hands;
}
bool idle_flags(gears_weapon::EquipmentSnapshot const& game){
    bool melee{},special{},mirrored{},wants{},reload{},transition{};std::uint8_t move{};int health{};
    return game.can_switch&&!game.animation_busy&&
        field(game.pawn,L"Health",L"IntProperty",&health,4)&&health>0&&
        field(game.pawn,L"SpecialMove",L"ByteProperty",&move,1)&&!move&&
        boolean(game.pawn,L"bDoingMeleeAttack",melee)&&!melee&&
        boolean(game.pawn,L"bDoingSpecialAttackMove",special)&&!special&&
        boolean(game.pawn,L"bIsMirrored",mirrored)&&!mirrored&&
        boolean(game.pawn,L"bWantsToMelee",wants)&&!wants&&
        bool_call(game.pawn,L"DoingAnimationTransition",transition)&&!transition&&
        bool_call(game.current_weapon,L"IsReloading",reload)&&!reload;
}
bool buttons(unsigned& down_mask){
    bool r2{},l2{};
    if(!boolean(controller,L"bHoldingButton_R2",r2)||
       !boolean(controller,L"bPressingTargetingKey",l2))return false;
    down_mask=(r2?1u:0u)|(l2?2u:0u);return true;
}
bool can_wait(uptr weapon,WeaponState state){
    return state==WeaponState::active||
        (playground::instance(weapon,L"Weap_LocustAssaultRifle")&&
         (state==WeaponState::firing||state==WeaponState::burst_delay));
}
bool commit_allowed(Key const& expected,ULONGLONG until){
    if(!weaponrestore::healthy())return false;
    gears_weapon::StowState<Key> current{};
    gears_weapon::EquipmentSnapshot fresh{};const wchar_t* error{};unsigned down_mask{};
    if(!read_state(current)||!(current.key==expected)||
       current.attachment!=gears_weapon::StowAttachment::original_hand||
       !snapshot(controller,fresh,error)||fresh.current_weapon!=expected.objects[6].address||
       !idle_flags(fresh)||weapon_state(fresh.current_weapon)!=WeaponState::active||
       !buttons(down_mask)||down_mask||!read_state(current)||!(current.key==expected)||
       current.attachment!=gears_weapon::StowAttachment::original_hand)return false;
    // This final decision is the commit point. A later Stop observes dirty
    // and drains its restore before hooks can be disabled.
    return weaponrestore::healthy()&&!stop_requested.load()&&!shutdown_requested.load()&&GetTickCount64()<until
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        &&handstream::equipment_ready()
#endif
        ;
}
bool acquire(Key const& expected,ULONGLONG until){
    if(!weaponrestore::healthy())return false;
    Backend backend{expected,until,true};
    const bool ok=lease.acquire(backend);publish();
    if(ok){deadline=GetTickCount64()+10000;status=5;}
    else status=8;
    if(log_file){fwprintf(log_file,L"equipment_stow_acquire success=%u dirty=%u fault=%u weapon=%p epoch=%llu\n",
        unsigned(ok),unsigned(lease.dirty()),unsigned(lease.faulted()),
        reinterpret_cast<void*>(expected.objects[6].address),epoch);fflush(log_file);}
    return ok;
}
struct PendingFinishResult {bool completed{},hands_clean{};};
PendingFinishResult finish_pending_checked(bool success,const wchar_t* reason,WeaponState state){
    const auto slot=pending_slot;
    if(slot>=4)return {};
    const bool hands_clean=success||yield_hands(reason);
    if(!hands_clean)stop_requested=true;
    const auto weapon=pending.key().objects[6].address;
    if(success)model=pending_model;else model.reset();
    const unsigned result=success?10u:12u;
    switch_requests.observe(result);
    if(!success)status=lease.dirty()||lease.faulted()?8u:6u;
    if(log_file){
        fwprintf(log_file,L"equipment_stow_pending_complete reason=%ls success=%u state=%u dirty=%u epoch=%llu\n",
            reason,unsigned(success),unsigned(state),unsigned(lease.dirty()),epoch);
        fwprintf(log_file,L"equipment_switch_test source=%ls slot=%u status=%u weapon=%p\n",
            pending_from_grip?L"xr_grip":L"external",slot,result,reinterpret_cast<void*>(weapon));
        fflush(log_file);
    }
    // No game writer belongs to a pending proposal. This cancels only the
    // wait and publishes its one terminal acknowledgement.
    if(pending.active())pending.poll(false,Key{},GetTickCount64(),false,false,true);
    pending_slot=4;pending_down_mask=0;pending_until=0;pending_model.reset();pending_from_grip=false;
    switch_requests.complete();
    pending_visible.store(false,std::memory_order_release);
    return {true,hands_clean};
}
void finish_pending(bool success,const wchar_t* reason,WeaponState state){
    (void)finish_pending_checked(success,reason,state);
}
enum class Transition:unsigned {failed,complete,deferred};
Transition transition(uptr pc,gears_weapon::EquipmentSnapshot const& game,bool holster,
                      gears_weapon::WeaponEquipmentModel const& proposed,unsigned slot,bool from_grip){
    if(shutdown_requested.load()||stop_requested.load()||lease.faulted()||pending_visible.load())
        return Transition::failed;
    const auto request_started=GetTickCount64();
    controller=pc;
    // Seal permission before the synchronous releases can re-enter game code.
    struct YieldScope {YieldScope(){yielding=true;}~YieldScope(){yielding=false;}} scope;
    if(!yield_hands(L"before_body_slot_transition"))return Transition::failed;
    gears_weapon::StowState<Key> before{};
    if(holster){
        if(lease.dirty())return Transition::failed;
        have_hand_attachment=false;
        if(!read_state(before)||before.key.objects[6].address!=game.current_weapon||
           before.attachment!=gears_weapon::StowAttachment::original_hand)return Transition::failed;
    }
    const auto released=vrfire::yield_for_equipment(pc,game.current_weapon);
    if(released==vrfire::EquipmentYield::failed)return Transition::failed;
    // Query again after release; never reuse pre-release idle or identity.
    gears_weapon::EquipmentSnapshot fresh{};const wchar_t* error{};
    const bool flags=snapshot(pc,fresh,error)&&fresh.current_weapon==game.current_weapon&&idle_flags(fresh);
    const auto state=weapon_state(game.current_weapon);
    if(!flags||!can_wait(game.current_weapon,state))return Transition::failed;
    unsigned down_mask{};
    if(!buttons(down_mask))return Transition::failed;
    const bool ready=released==vrfire::EquipmentYield::ready&&!down_mask&&state==WeaponState::active;
    if(!holster){
        if(!ready)return Transition::failed;
        if(lease.dirty())return restore(L"body_slot_unholster")?Transition::complete:Transition::failed;
        // Original-game detachment is not ours to undo.
        have_hand_attachment=false;gears_weapon::StowState<Key> current{};
        return read_state(current)&&current.key.objects[6].address==game.current_weapon&&
            current.attachment==gears_weapon::StowAttachment::original_hand?Transition::complete:Transition::failed;
    }
    gears_weapon::StowState<Key> current{};
    if(!read_state(current)||!(current.key==before.key)||
       current.attachment!=gears_weapon::StowAttachment::original_hand)return Transition::failed;
    if(ready)return acquire(current.key,request_started+1000)?Transition::complete:Transition::failed;
    // The entire request, including its initial release/queries, shares one
    // deadline; neither the first wait nor its final acquire extends it.
    if(!pending.begin(current.key,request_started,1000))return Transition::failed;
    pending_until=request_started+1000;
    pending_model=proposed;pending_slot=slot;pending_down_mask=down_mask;pending_from_grip=from_grip;
    status=13;pending_visible.store(true,std::memory_order_release);
    if(log_file){fwprintf(log_file,L"equipment_stow_deferred slot=%u state=%u yield=%u timeout_ms=1000 epoch=%llu\n",
        slot,unsigned(state),unsigned(released),epoch);fflush(log_file);}
    return Transition::deferred;
}
bool before_switch(uptr pc,uptr weapon){
    struct YieldScope {YieldScope(){yielding=true;}~YieldScope(){yielding=false;}} scope;
    return yield_hands(L"before_body_slot_switch")&&
        (!weapon||vrfire::yield_for_equipment(pc,weapon)==vrfire::EquipmentYield::ready)&&
        restore(L"before_body_slot_switch");
}
bool advance_pending(uptr pc){
    if(!pending.active())return false;
    gears_weapon::StowState<Key> current{};
    const bool known=pc==controller&&read_state(current)&&
        current.attachment==gears_weapon::StowAttachment::original_hand;
    gears_weapon::EquipmentSnapshot fresh{};const wchar_t* error{};
    const bool flags=known&&snapshot(pc,fresh,error)&&idle_flags(fresh);
    const auto state=known?weapon_state(current.key.objects[6].address):WeaponState::unknown;
    unsigned down_mask{};
    // Only originally released buttons may still be settling. Once read up,
    // a new down is foreign input and cancels rather than being hidden.
    const bool keys=buttons(down_mask)&&!(down_mask&~pending_down_mask);
    const bool allowed=flags&&keys&&can_wait(current.key.objects[6].address,state);
    if(keys)pending_down_mask=down_mask;
    const bool ready=allowed&&state==WeaponState::active&&!down_mask;
    const auto outcome=pending.poll(known,current.key,GetTickCount64(),ready,allowed,
        stop_requested.load()||shutdown_requested.load());
    if(outcome==gears_weapon::StowPendingResult::waiting)return true;
    if(log_file){fwprintf(log_file,L"equipment_stow_pending_guard known=%u flags=%u keys=%u down=%u state=%u outcome=%u\n",
        unsigned(known),unsigned(flags),unsigned(keys),down_mask,unsigned(state),unsigned(outcome));fflush(log_file);}
    const bool success=outcome==gears_weapon::StowPendingResult::ready&&
        acquire(pending.key(),pending_until);
    finish_pending(success,outcome==gears_weapon::StowPendingResult::ready?L"ready":L"cancelled",state);
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
    if(success)weaponhand::equipment_hold_committed(pc,fresh);
#endif
    return true;
}
bool related(uptr object){
    const auto& current=pending.active()?pending.key():lease.key();
    for(auto const& key:current.objects)if(key.address==object)return registered(object);
    if(!registered(object)||playground::deleted(object))return false;
    uptr owner{};
    return playground::instance(object,L"WarSpecialMove")&&
        pointer(object,L"PawnOwner",owner)&&owner==playground::arena_pawn;
}
enum class BeforeActionOutcome {ignored,pending_state_exempt,clean,unresolved};
BeforeActionOutcome before_action_checked(uptr object,uptr function){
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
    if(!weaponrestore::healthy())return BeforeActionOutcome::unresolved;
#endif
    if((!dirty_visible.load(std::memory_order_acquire)&&!pending_visible.load(std::memory_order_acquire))||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||mutating||
       (!lease.dirty()&&!pending.active())||!registered(function)||!related(object))return BeforeActionOutcome::ignored;
    wchar_t label[96]{};if(!name(function,label,96))return BeforeActionOutcome::ignored;
    constexpr const wchar_t* actions[]={L"SetWeaponTemporarilyHolstered",L"HolsterWeaponTemporarily",
        L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",L"ClientWeaponSet",
        L"AttachWeaponToHand",L"DetachWeapon",L"AttachWeaponToSlot",L"DetachWeaponFromSlot",
        L"AttachComponent",L"AttachComponentToSocket",L"DetachComponent",L"DetachComponentFromSocket",
        L"ServerUse",L"GrabPickup",L"ServerDictateSpecialMove",L"DoSpecialMove",L"EndSpecialMove",
        L"StartSpecialMove",L"StopSpecialMove",L"StartMeleeAttack",L"StopMeleeAttack",
        L"PlayBodyStance",L"PlayBodyStanceByDuration",L"SetBodyStance",L"StopAllBodyStances",
        L"BeginState",L"EndState",L"ForceReload",L"Died",L"Destroyed"};
    bool watched=false;for(const auto action:actions)if(!wcscmp(label,action)){watched=true;break;}
    if(pending.active()&&(!wcscmp(label,L"ButtonPress")||!wcscmp(label,L"ButtonRelease")))watched=true;
    if(!watched)return BeforeActionOutcome::ignored;
    if(pending.active()){
        // Only this Hammerburst's natural firing state exit may finish while
        // waiting. A similarly named action on Pawn/PC is not an exemption.
        const bool state_boundary=!wcscmp(label,L"BeginState")||!wcscmp(label,L"EndState");
        if(state_boundary&&object==pending.key().objects[6].address&&
           playground::instance(object,L"Weap_LocustAssaultRifle")){
            const auto state=weapon_state(object);
            if(can_wait(object,state)){
                if(log_file){fwprintf(log_file,L"equipment_stow_pending_state function=%ls state=%u epoch=%llu\n",
                    label,unsigned(state),epoch);fflush(log_file);}
                return BeforeActionOutcome::pending_state_exempt;
            }
        }
        const auto finished=finish_pending_checked(false,label,WeaponState::unknown);++epoch;
        return finished.completed&&finished.hands_clean&&!pending.active()&&pending_slot==4&&
            !pending_visible.load(std::memory_order_acquire)&&!lease.dirty()&&!lease.faulted()&&
            !dirty_visible.load(std::memory_order_acquire)?BeforeActionOutcome::clean:BeforeActionOutcome::unresolved;
    }
    const bool restored=restore(L"before_original_action");
    ++epoch; // This lease is never reopened after handing control to the game.
    if(log_file){fwprintf(log_file,L"equipment_stow_action function=%ls restored=%u dirty=%u epoch=%llu\n",
        label,unsigned(restored),unsigned(lease.dirty()),epoch);fflush(log_file);}
    return restored&&!lease.dirty()&&!lease.faulted()&&!dirty_visible.load(std::memory_order_acquire)&&
        !pending.active()&&!pending_visible.load(std::memory_order_acquire)?BeforeActionOutcome::clean:BeforeActionOutcome::unresolved;
}
void before_action(uptr object,uptr function){
    (void)before_action_checked(object,function);
}
void tick(){
    if(!lease.dirty())return;
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    handstream::EquipmentTimingScope timing(handstream::EquipmentTiming::dirty_stow_tick);
#endif
    if(stop_requested.load()||shutdown_requested.load()||GetTickCount64()>=deadline){
        restore(L"stop_or_bounded_deadline");return;
    }
    // Detect loss without guessing a new owner or overwriting foreign state.
    gears_weapon::StowState<Key> current{};
    if(!read_state(current)||!(current.key==lease.key())||
       (current.attachment!=gears_weapon::StowAttachment::detached&&
        current.attachment!=gears_weapon::StowAttachment::original_hand)){
        lease.revoke();status=8;stop_requested=true;
        yield_hands(L"stow_authentication_lost");return;
    }
    if(current.attachment==gears_weapon::StowAttachment::original_hand)
        relinquish_original_hand(L"original_hand_already_restored");
}
void reset(){
    if(!weaponrestore::healthy())return;
    if(!lease.reset())return;
    pending.reset();pending_model.reset();pending_slot=4;pending_down_mask=0;pending_until=0;pending_from_grip=false;
    dirty_visible=false;pending_visible=false;stop_requested=false;shutdown_requested=false;status=0;
    epoch=0;controller=0;deadline=0;mutating=false;yielding=false;
    hand_attachment={};have_hand_attachment=false;
}
} // namespace stow
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
unsigned hand_equipment_phase(uptr pc,uptr pawn,uptr weapon){
    if(!weaponrestore::healthy())return 0;
    // This is private evidence for the same fixed arena, not a new public
    // animation ownership bit. A bare detached component grants no authority.
    if(GetCurrentThreadId()!=game_thread.load()||pc!=playground::arena_pc||
       pawn!=playground::arena_pawn||!weapon||stow::yielding||
       stow::pending_visible.load()||stow::stop_requested.load()||
       stow::shutdown_requested.load()||stow::lease.faulted()||
       (stow::controller&&stow::controller!=pc))return 0;
    stow::controller=pc;
    if(stow::lease.dirty()&&GetTickCount64()>=stow::deadline){
        stow::restore(L"hand_phase_stow_deadline");return 0;
    }
    gears_weapon::StowState<stow::Key> current{};
    bool known{};
    {
        handstream::EquipmentTimingScope timing(handstream::EquipmentTiming::phase_read_before);
        known=stow::read_state(current)&&current.key.objects[2].address==pc&&
            current.key.objects[3].address==pawn&&current.key.objects[6].address==weapon;
    }
    if(stow::lease.dirty()){
        if(known&&current.key==stow::lease.key()&&
           current.attachment==gears_weapon::StowAttachment::original_hand){
            stow::relinquish_original_hand(L"hand_phase_original_hand_restored");return 0;
        }
        if(!known||!(current.key==stow::lease.key())||stow::lease.revoked()||
           current.attachment!=gears_weapon::StowAttachment::detached){
            stow::lease.revoke();stow::status=8;stow::stop_requested=true;
            stow::yield_hands(L"hand_phase_stow_authentication_lost");return 0;
        }
    }else if(!known||current.attachment!=gears_weapon::StowAttachment::original_hand)return 0;
    const auto expected=current.key;
    const bool owned=stow::lease.dirty();
    gears_weapon::EquipmentSnapshot fresh{};const wchar_t* error{};
    bool snapshot_known{};
    {
        handstream::EquipmentTimingScope timing(handstream::EquipmentTiming::phase_snapshot);
        snapshot_known=snapshot(pc,fresh,error);
    }
    if(!snapshot_known||fresh.current_weapon!=weapon||!stow::idle_flags(fresh)||
       stow::weapon_state(weapon)!=stow::WeaponState::active||stow::yielding||
       stow::pending_visible.load()||stow::stop_requested.load()||stow::shutdown_requested.load())return 0;
    gears_weapon::StowState<stow::Key> final{};
    const auto wanted=owned?gears_weapon::StowAttachment::detached:gears_weapon::StowAttachment::original_hand;
    bool final_known{};
    {
        handstream::EquipmentTimingScope timing(handstream::EquipmentTiming::phase_read_after);
        final_known=stow::read_state(final);
    }
    if(!final_known||!(final.key==expected)||final.attachment!=wanted||
       stow::lease.dirty()!=owned){
        if(owned&&stow::lease.dirty()){
            stow::lease.revoke();stow::status=8;stow::stop_requested=true;
            stow::yield_hands(L"hand_phase_final_authentication_lost");
        }
        return 0;
    }
    if(stow::stop_requested.load()||stow::shutdown_requested.load()||
       stow::lease.faulted()||(owned&&(stow::lease.revoked()||GetTickCount64()>=stow::deadline)))return 0;
    return owned?2u:1u;
}
#endif
bool stow_fire_permitted(uptr pc,uptr weapon){
    return weaponrestore::healthy()&&!stow::shutdown_requested.load()&&!stow::stop_requested.load()&&
        !stow::yielding&&!stow::pending_visible.load()&&
        !stow::lease.dirty()&&!stow::lease.faulted()&&playground::ready&&test_environment()&&
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        handstream::equipment_ready()&&
#endif
        pc==playground::arena_pc&&model.held_weapon()==weapon&&
        playground::pointer(pc,L"Pawn")==playground::arena_pawn&&
        playground::pointer(playground::arena_pawn,L"Weapon")==weapon;
}
