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
std::atomic<bool> dirty_visible{},stop_requested{},shutdown_requested{};
std::atomic<unsigned> status{};
std::uint64_t epoch{};
uptr controller{};
ULONGLONG deadline{};
bool mutating{};
std::array<unsigned char,44> hand_attachment{};
bool have_hand_attachment{};

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
bool active(uptr weapon){
    playground::Args call(weapon,L"GetStateName");
    return call.r.size==8&&get<unsigned short>(call.r.function+0x108)==0&&parameters(call)==1&&
        argument(call,L"ReturnValue",L"NameProperty",8,0,0x580)&&call.run()&&
        name_is(call.output<Name>(),L"Active");
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
struct Backend {
    bool read(gears_weapon::StowState<Key>& out){
        const bool ok=read_state(out);
        if(log_file){
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
        // Only the same game-thread lease may reach this original helper.
        if(value&&(shutdown_requested.load()||stop_requested.load()))return false;
        playground::Args call(playground::arena_pawn,L"SetWeaponTemporarilyHolstered");
        if(call.r.size!=4||get<unsigned short>(call.r.function+0x108)!=0xffff||parameters(call)!=1||
           !argument(call,L"bHolstered",L"BoolProperty",4,0,0x80))return false;
        const auto mask=get<unsigned>(call.prop(L"bHolstered")+0xd0);
        if(!mask||(mask&(mask-1)))return false;
        struct Scope {Scope(){mutating=true;}~Scope(){mutating=false;}} scope;
        if(log_file){fwprintf(log_file,L"equipment_stow_helper phase=before holstered=%u epoch=%llu\n",
            unsigned(value),epoch);fflush(log_file);}
        const bool called=call.flag(L"bHolstered",value).run();
        if(log_file){fwprintf(log_file,L"equipment_stow_helper phase=after holstered=%u called=%u epoch=%llu\n",
            unsigned(value),unsigned(called),epoch);fflush(log_file);}
        return called;
    }
};
void publish(){dirty_visible.store(lease.dirty(),std::memory_order_release);}
bool restore(const wchar_t* reason){
    const bool was_dirty=lease.dirty();Backend backend{};
    const bool ok=lease.restore(backend);publish();
    if(was_dirty){
        model.reset();deadline=0;
        status.store(ok?6u:8u,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"equipment_stow_restore reason=%ls clean=%u fault=%u revoked=%u epoch=%llu\n",
            reason,unsigned(!lease.dirty()),unsigned(lease.faulted()),unsigned(lease.revoked()),epoch);fflush(log_file);}
    }
    return ok;
}
bool idle(gears_weapon::EquipmentSnapshot const& game){
    bool melee{},special{},mirrored{},wants{},reload{},transition{};std::uint8_t move{};int health{};
    return game.can_switch&&!game.animation_busy&&
        field(game.pawn,L"Health",L"IntProperty",&health,4)&&health>0&&
        field(game.pawn,L"SpecialMove",L"ByteProperty",&move,1)&&!move&&
        boolean(game.pawn,L"bDoingMeleeAttack",melee)&&!melee&&
        boolean(game.pawn,L"bDoingSpecialAttackMove",special)&&!special&&
        boolean(game.pawn,L"bIsMirrored",mirrored)&&!mirrored&&
        boolean(game.pawn,L"bWantsToMelee",wants)&&!wants&&
        bool_call(game.pawn,L"DoingAnimationTransition",transition)&&!transition&&
        bool_call(game.current_weapon,L"IsReloading",reload)&&!reload&&active(game.current_weapon);
}
bool transition(uptr pc,gears_weapon::EquipmentSnapshot const& game,bool holster){
    if(shutdown_requested.load()||stop_requested.load()||lease.faulted())return false;
    controller=pc;
    if(!vrfire::yield_for_equipment(pc,game.current_weapon))return false;
    if(!idle(game))return false;
    if(!holster){
        if(lease.dirty())return restore(L"body_slot_unholster");
        // The original game may have detached this weapon. Never pull such a
        // weapon out merely because the VR latch is currently neutral.
        have_hand_attachment=false;gears_weapon::StowState<Key> state{};
        return read_state(state)&&state.key.objects[6].address==game.current_weapon&&
            state.attachment==gears_weapon::StowAttachment::original_hand;
    }
    if(lease.dirty())return false;
    have_hand_attachment=false;
    Backend backend{};
    const bool ok=lease.acquire(backend);publish();
    if(ok){deadline=GetTickCount64()+10000;status=5;}
    else status=8;
    if(log_file){fwprintf(log_file,L"equipment_stow_acquire success=%u dirty=%u fault=%u weapon=%p epoch=%llu\n",
        unsigned(ok),unsigned(lease.dirty()),unsigned(lease.faulted()),
        reinterpret_cast<void*>(static_cast<uptr>(game.current_weapon)),epoch);fflush(log_file);}
    return ok;
}
bool related(uptr object){
    for(auto const& key:lease.key().objects)if(key.address==object)return registered(object);
    if(!registered(object)||playground::deleted(object))return false;
    uptr owner{};
    return playground::instance(object,L"WarSpecialMove")&&
        pointer(object,L"PawnOwner",owner)&&owner==playground::arena_pawn;
}
void before_action(uptr object,uptr function){
    if(!dirty_visible.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||mutating||
       !lease.dirty()||!registered(function)||!related(object))return;
    wchar_t label[96]{};if(!name(function,label,96))return;
    constexpr const wchar_t* actions[]={L"SetWeaponTemporarilyHolstered",L"HolsterWeaponTemporarily",
        L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",L"ClientWeaponSet",
        L"AttachWeaponToHand",L"DetachWeapon",L"AttachWeaponToSlot",L"DetachWeaponFromSlot",
        L"AttachComponent",L"AttachComponentToSocket",L"DetachComponent",L"DetachComponentFromSocket",
        L"ServerUse",L"GrabPickup",L"ServerDictateSpecialMove",L"DoSpecialMove",L"EndSpecialMove",
        L"StartSpecialMove",L"StopSpecialMove",L"StartMeleeAttack",L"StopMeleeAttack",
        L"PlayBodyStance",L"PlayBodyStanceByDuration",L"SetBodyStance",L"StopAllBodyStances",
        L"BeginState",L"EndState",L"ForceReload",L"Died",L"Destroyed"};
    bool watched=false;for(const auto action:actions)if(!wcscmp(label,action)){watched=true;break;}
    if(!watched)return;
    const bool restored=restore(L"before_original_action");
    ++epoch; // This lease is never reopened after handing control to the game.
    if(log_file){fwprintf(log_file,L"equipment_stow_action function=%ls restored=%u dirty=%u epoch=%llu\n",
        label,unsigned(restored),unsigned(lease.dirty()),epoch);fflush(log_file);}
}
void tick(){
    if(!lease.dirty())return;
    if(stop_requested.load()||shutdown_requested.load()||GetTickCount64()>=deadline){
        restore(L"stop_or_bounded_deadline");return;
    }
    // Detect loss without guessing a new owner or overwriting foreign state.
    gears_weapon::StowState<Key> current{};
    if(!read_state(current)||!(current.key==lease.key())||
       (current.attachment!=gears_weapon::StowAttachment::detached&&
        current.attachment!=gears_weapon::StowAttachment::original_hand)){
        lease.revoke();status=8;stop_requested=true;return;
    }
    if(current.attachment==gears_weapon::StowAttachment::original_hand)
        restore(L"original_hand_already_restored");
}
void reset(){
    if(!lease.reset())return;
    dirty_visible=false;stop_requested=false;shutdown_requested=false;status=0;
    epoch=0;controller=0;deadline=0;mutating=false;
    hand_attachment={};have_hand_attachment=false;
}
} // namespace stow
bool stow_fire_permitted(uptr pc,uptr weapon){
    return !stow::shutdown_requested.load()&&!stow::stop_requested.load()&&
        !stow::lease.dirty()&&!stow::lease.faulted()&&playground::ready&&test_environment()&&
        pc==playground::arena_pc&&model.held_weapon()==weapon&&
        playground::pointer(pc,L"Pawn")==playground::arena_pawn&&
        playground::pointer(playground::arena_pawn,L"Weapon")==weapon;
}
