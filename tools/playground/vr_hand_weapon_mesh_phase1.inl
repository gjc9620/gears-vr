// Included after the real equipment backend, inside handstream. Private only.
// The mount is geometry; only the currently scoped hand Plan can grant a write.
namespace component {
namespace stow=vrequip::stow;
using Value=gears_weapon::WeaponMeshComponentValue;
using Bytes=gears_weapon::MeshComponentBytes12;
using Absolute=gears_weapon::MeshComponentAbsolute;
struct Key {
    std::array<stow::ObjectKey,8> objects{};
    std::array<std::uint64_t,4> slots{};
    stow::ObjectKey asset{};
    bool operator==(Key const& other) const noexcept {
        return slots==other.slots&&!std::memcmp(objects.data(),other.objects.data(),sizeof(objects))&&
            !std::memcmp(&asset,&other.asset,sizeof(asset));
    }
};
using State=gears_weapon::WeaponMeshComponentState<Key>;
gears_weapon::WeaponMeshComponentLease<Key> lease;
gears_weapon::WeaponMeshFollow mount;
Key fixed_key{};
bool key_known{},recorded{},terminal_dumped{},terminal_clean_dumped{};
std::uint64_t acquired{},restored{},setter_calls{},relinquished{};
unsigned hand_lease{};
std::uint64_t plan_sequence{},plan_revision{},action_epoch{};
gears_hand_continuity::Certificate certificate{}; // Evidence only, never read by authorization.
Value target{};
std::array<unsigned,3> baseline_absolute_storage{},target_absolute_storage{};

bool absolute_storage(Key const& key,std::array<unsigned,3>& bits,std::array<uptr,3>* addresses=nullptr,
                      std::array<unsigned,3>* masks=nullptr){
    const wchar_t* fields[]={L"AbsoluteTranslation",L"AbsoluteRotation",L"AbsoluteScale"};
    for(unsigned i=0;i<3;++i){
        const auto object=key.objects[7].address,property=playground::property(object,fields[i]);
        if(!stow::field(object,fields[i],L"BoolProperty",&bits[i],4))return false;
        const auto mask=get<unsigned>(property+0xd0);
        if(!mask||(mask&(mask-1)))return false;
        if(addresses)(*addresses)[i]=object+get<unsigned>(property+0x94);
        if(masks)(*masks)[i]=mask;
    }
    return true;
}

// invoke_engine directly invokes the original trampoline/native wrapper. Only
// its initial Args -> invoke admission is exempt; every hooked recursive engine
// action remains an external boundary, including identical same-value setters.
struct SelfSetter {
    uptr object{},function{};
    bool armed{};
};
SelfSetter self;
struct SelfScope {
    SelfSetter saved;
    SelfScope(uptr object,uptr function):saved(self){self={object,function,true};}
    ~SelfScope(){self=saved;}
};
bool consume_self(uptr object,uptr function){
    if(!self.armed||self.object!=object||self.function!=function)return false;
    self.armed=false;return true;
}
bool current(Key const& expected){
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    for(auto const& old:expected.objects){
        stow::ObjectKey found{};
        if(!stow::object_key(old.address,found)||std::memcmp(&old,&found,sizeof(old)))return false;
    }
    stow::ObjectKey asset{};
    if(!stow::object_key(expected.asset.address,asset)||
       std::memcmp(&asset,&expected.asset,sizeof(asset)))return false;
    const auto address=[&](unsigned i){return expected.objects[i].address;};
    uptr pointer{};
    if(!stow::pointer(address(2),L"WorldInfo",pointer)||pointer!=address(0)||
       !stow::pointer(address(2),L"Pawn",pointer)||pointer!=address(3)||
       !stow::pointer(address(3),L"Controller",pointer)||pointer!=address(2)||
       !stow::pointer(address(3),L"InvManager",pointer)||pointer!=address(4)||
       !stow::pointer(address(3),L"Mesh",pointer,true)||pointer!=address(5)||
       !stow::pointer(address(3),L"Weapon",pointer)||pointer!=address(6)||
       !stow::pointer(address(6),L"Instigator",pointer)||pointer!=address(3)||
       !stow::pointer(address(6),L"Mesh",pointer,true)||pointer!=address(7)||
       !stow::pointer(address(7),L"SkeletalMesh",pointer)||pointer!=expected.asset.address||
       get<uptr>(address(0)+0x40)!=address(1)||get<uptr>(address(2)+0x40)!=address(1)||
       get<uptr>(address(3)+0x40)!=address(1))return false;
    for(std::uint8_t i=0;i<4;++i){
        uptr slot{};if(!vrequip::read_slot(address(4),i,slot)||slot!=expected.slots[i])return false;
    }
    return playground::instance(address(6),L"Weap_AssaultRifle")&&
        playground::instance(address(7),L"SkeletalMeshComponent")&&
        playground::instance(expected.asset.address,L"SkeletalMesh")&&
        playground::named(expected.asset.address,L"COGAssaultRifle");
}
bool capture(Snapshot const& hand,Key& key){
    gears_weapon::StowState<stow::Key> state{};
    if(!stow::read_state(state)||!state.ready||state.attachment!=gears_weapon::StowAttachment::original_hand||
       state.key.objects[2].address!=hand.pc||state.key.objects[3].address!=hand.pawn||
       state.key.objects[5].address!=hand.mesh||state.key.objects[6].address!=hand.weapon)return false;
    key={};key.objects=state.key.objects;key.slots=state.key.slots;
    uptr asset{};
    return stow::pointer(key.objects[7].address,L"SkeletalMesh",asset)&&
        stow::object_key(asset,key.asset)&&current(key);
}
bool triple_schema(uptr property,bool rotation){
    if(!registered(property)||get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=12||
       !playground::named(get<uptr>(property+0x50),L"StructProperty"))return false;
    const auto structure=get<uptr>(property+0xd0);
    if(!registered(structure)||!playground::named(structure,rotation?L"Rotator":L"Vector")||
       get<unsigned>(structure+0x88)!=12)return false;
    const wchar_t* fields[2][3]={{L"X",L"Y",L"Z"},{L"Pitch",L"Yaw",L"Roll"}};
    for(unsigned i=0;i<3;++i){unsigned offset{};
        if(!handanim::structure_field(structure,fields[rotation?1:0][i],
            rotation?L"IntProperty":L"FloatProperty",4,12,offset)||offset!=i*4)return false;
    }
    return true;
}
bool read(Key const& key,State& out){
    out={};
    if(!current(key))return false;
    const auto mesh=key.objects[7].address;
    for(unsigned i=0;i<3;++i){
        const wchar_t* fields[]={L"AbsoluteTranslation",L"AbsoluteRotation",L"AbsoluteScale"};
        if(!stow::boolean(mesh,fields[i],out.value.absolute[i]))return false;
    }
    if(!triple_schema(playground::property(mesh,L"Translation"),false)||
       !triple_schema(playground::property(mesh,L"Rotation"),true)||
       !stow::field(mesh,L"Translation",L"StructProperty",out.value.translation.data(),12)||
       !stow::field(mesh,L"Rotation",L"StructProperty",out.value.rotation.data(),12))return false;
    std::array<float,3> position{};std::memcpy(position.data(),out.value.translation.data(),12);
    for(float x:position)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    out.key=key;out.known=true;return current(key);
}
bool world(Key const& key,std::array<float,16>& value,int& tick){
    return current(key)&&handik::structure(key.objects[7].address,L"LocalToWorld",L"Matrix",value.data(),64)&&
        stow::field(key.objects[5].address,L"TickTag",L"IntProperty",&tick,4);
}
struct Forward {
    gears_hand_stream::Plan const& plan;
    unsigned mask;
    gears_hand_stream::Context context{};
    bool prepare(Key const& key){
        Snapshot hand{};
        if(trial.ended||trial.xr_equipment_phase!=1||trial.epoch!=plan.context.action_epoch||
           trial.tick_epoch!=trial.epoch||!handik::snapshot(key.objects[2].address,hand)||
           !xr_identity(hand)||!hand.action_idle||!handik::baseline_strengths(hand)||!slots_quiet(hand)||
           !xr_equipment_gate(hand,1,L"component_setter",EquipmentTiming::prewrite_phase))return false;
        context=xr_context(hand,trial.xr_ready);return true;
    }
    bool authorize(){
        return !trial.ended&&trial.xr_equipment_phase==1&&trial.tick_epoch==trial.epoch&&
            trial.epoch==plan.context.action_epoch&&(mask&2)&&
            authorize_current_xr_plan(plan,context,GetTickCount64())==mask;
    }
};
struct Backend {
    Key expected_key{};
    Forward* forward{}; // nullptr is rollback, authenticated by old Key/value only.
    bool read(State& state){return component::read(expected_key,state);}
    bool schema(playground::Args& call,unsigned group){
        if(!call.valid||!registered(call.r.function)||call.r.size!=12||
           !playground::named(get<uptr>(call.r.function+0x50),L"Function")||
           get<unsigned short>(call.r.function+0x108)!=0xffff)return false;
        // Validate the actual instance's resolved native wrapper, without
        // inventing an index/address from the old MagazineMesh-only evidence.
        const auto native=get<uptr>(call.r.function+0x114);
        if(native<base+0x1000||native>=base+0x131b000)return false;
        if(group==1){
            if(stow::parameters(call)!=3)return false;
            const wchar_t* fields[]={L"NewAbsoluteTranslation",L"NewAbsoluteRotation",L"NewAbsoluteScale"};
            for(unsigned i=0;i<3;++i){
                if(!stow::argument(call,fields[i],L"BoolProperty",4,i*4,0x80))return false;
                const auto mask=get<unsigned>(call.prop(fields[i])+0xd0);
                if(!mask||(mask&(mask-1)))return false;
            }
            return true; // classification deliberately accepts OptionalParm (0x90).
        }
        const auto field=group==2?L"NewTranslation":L"NewRotation";
        return stow::parameters(call)==1&&stow::argument(call,field,L"StructProperty",12,0,0x80)&&
            triple_schema(call.prop(field),group==4);
    }
    bool set(Key const& key,Value const& expected,Value const& wanted,unsigned group){
        if(!(key==expected_key))return false;
        const wchar_t* function=group==1?L"SetAbsolute":group==2?L"SetTranslation":L"SetRotation";
        playground::Args call(key.objects[7].address,function);
        if(!schema(call,group))return false;
        if(group==1){
            call.flag(L"NewAbsoluteTranslation",wanted.absolute[0]);
            call.flag(L"NewAbsoluteRotation",wanted.absolute[1]);call.flag(L"NewAbsoluteScale",wanted.absolute[2]);
        }else if(!call.bytes(group==2?L"NewTranslation":L"NewRotation",
                  group==2?wanted.translation.data():wanted.rotation.data(),12))return false;
        State before{};
        if(!read(before)||!gears_weapon::mesh_component_exact(before.value,expected))return false;
        if(forward&&!forward->prepare(key))return false;
        // Revalidate the entire old instance/value and actual schema after all
        // engine getters, then authorize the same Plan last. No new receipt.
        if(!read(before)||!gears_weapon::mesh_component_exact(before.value,expected)||!schema(call,group)||
           (forward&&!forward->authorize()))return false;
        if(!(lease.owned_mask()&group))return false; // Reentrant action may have revoked this transaction.
        SelfScope self_call(call.r.object,call.r.function);count_add(setter_calls);
        return call.run(); // Never interpret a native void/RAX as success.
    }
    bool set_absolute(Key const& key,Value const& expected,Absolute const& wanted){
        auto value=expected;value.absolute=wanted;return set(key,expected,value,1);
    }
    bool set_translation(Key const& key,Value const& expected,Bytes const& wanted){
        auto value=expected;value.translation=wanted;return set(key,expected,value,2);
    }
    bool set_rotation(Key const& key,Value const& expected,Bytes const& wanted){
        auto value=expected;value.rotation=wanted;return set(key,expected,value,4);
    }
};
void bytes(Bytes const& value){for(auto byte:value)fwprintf(log_file,L"%02x",unsigned(byte));}
void value(const wchar_t* label,Value const& value){
    fwprintf(log_file,L" %ls_absolute=%u%u%u %ls_translation=",label,
        unsigned(value.absolute[0]),unsigned(value.absolute[1]),unsigned(value.absolute[2]),label);
    bytes(value.translation);fwprintf(log_file,L" %ls_rotation=",label);bytes(value.rotation);
}
void log(const wchar_t* phase){
    if(!recorded||!log_file)return;
    State readback{};const bool known=read(lease.key(),readback);
    std::array<float,16> matrix{};int tick{};
    const bool world_known=world(lease.key(),matrix,tick);
    std::array<unsigned,3> raw{};const bool raw_known=known&&absolute_storage(lease.key(),raw);
    fwprintf(log_file,L"hand_weapon_component_sample schema=1 phase=%ls hand_lease=%u plan_revision=%llu "
        L"sequence=%llu action_epoch=%llu equipment_phase=%u readback_known=%u world_known=%u mesh_tick=%d "
        L"native_started_qpc=%llu xr_pid=%llu xr_start=%llu stream_epoch=%llu reference_epoch=%llu "
        L"reference_generation=%llu left_tracking_epoch=%llu right_tracking_epoch=%llu "
        L"source=%u profile_generation=%llu stereo_ns=%lld input_ns=%lld dirty_mask=%u owned_mask=%u revoked_mask=%u",
        phase,hand_lease,plan_revision,plan_sequence,action_epoch,trial.xr_equipment_phase,unsigned(known),unsigned(world_known),tick,
        certificate.native_started_qpc,certificate.xr_publisher_pid,certificate.xr_publisher_start,
        certificate.stream_epoch,certificate.reference_epoch,certificate.reference_generation,
        certificate.tracking_epoch[0],certificate.tracking_epoch[1],certificate.source,
        certificate.profile_generation,certificate.stereo_received_ns,certificate.input_received_ns,
        lease.dirty_mask(),lease.owned_mask(),lease.revoked_mask());
    for(unsigned i=0;i<9;++i){const auto& object=i<8?lease.key().objects[i]:lease.key().asset;
        fwprintf(log_file,L" key%u=%llx:%llx:%llx:%u:%u:%u",i,object.address,object.object_class,object.outer,
            object.index,object.name_index,object.name_number);}
    for(unsigned i=0;i<4;++i)fwprintf(log_file,L" slot%u=%llu",i,lease.key().slots[i]);
    value(L"baseline",lease.original_value());value(L"target",target);value(L"readback",readback.value);
    // Exact storage evidence includes unrelated packed bits; ownership and
    // equality cover only the three logical flags, never those unrelated bits.
    fwprintf(log_file,L" absolute_storage_known=%u baseline_absolute_storage=",unsigned(raw_known));
    for(auto bits:baseline_absolute_storage)fwprintf(log_file,L"%08x",bits);
    fwprintf(log_file,L" target_absolute_storage=");for(auto bits:target_absolute_storage)fwprintf(log_file,L"%08x",bits);
    fwprintf(log_file,L" readback_absolute_storage=");for(auto bits:raw)fwprintf(log_file,L"%08x",bits);
    freehands::print_matrix(L"gun",matrix);fwprintf(log_file,L"\n");fflush(log_file);
}
} // namespace component

bool component_dirty(){return component::lease.dirty();}
bool component_faulted(){return component::lease.faulted();}
bool component_owned(){
    component::State state{};
    if(component_faulted()||!component::lease.has_snapshot()||
       !component::read(component::lease.key(),state)||
       !gears_weapon::mesh_component_exact(state.value,component::lease.owned_value())){
        component::lease.relinquish();return false;
    }
    return true;
}
bool component_related(uptr object){
    if(component::key_known){
        // SetWeaponFromSlot is owned by InvManager, not necessarily Pawn/PC.
        // Every frozen participant is a boundary even while stow is clean.
        for(auto const& key:component::fixed_key.objects)if(object==key.address)return true;
        return false;
    }
    // Before the first mount, the authenticated hand anchor supplies the two
    // additional participants absent from the ordinary hand slot watch list.
    if(!trial.anchored)return false;
    uptr inv{},mesh{};
    return (vrequip::stow::pointer(trial.anchor.pawn,L"InvManager",inv)&&object==inv)||
        (vrequip::stow::pointer(trial.anchor.weapon,L"Mesh",mesh,true)&&object==mesh);
}
void component_reset_binding(unsigned mask){if(mask&2)component::mount.reset();}
void component_relinquish(){
    component::lease.relinquish();component::mount.reset();count_add(component::relinquished);
}
void component_pre_restore(){if(component_dirty())component::log(L"pre_restore");}
void component_log_written(){if(component_dirty())component::log(L"written");}
bool component_restore(){
    if(!component_dirty())return true;
    component::Backend backend{component::lease.key(),nullptr};
    const bool clean=component::lease.restore(backend);
    if(clean)count_add(component::restored);
    component::recorded=false;publish_journals();
    return clean&&!component_faulted();
}
bool component_reset(){
    if(!component::lease.reset())return false;
    component::mount.reset();component::fixed_key={};component::key_known=component::recorded=false;
    component::terminal_dumped=component::terminal_clean_dumped=false;
    component::acquired=component::restored=component::setter_calls=component::relinquished=0;
    component::hand_lease=0;component::self={};return true;
}
void component_dump_end(){
    if(!trial.ended||!log_file)return;
    const bool clean=!component_dirty();
    if(component::terminal_dumped&&(!clean||component::terminal_clean_dumped))return;
    component::terminal_dumped=true;if(clean)component::terminal_clean_dumped=true;
    fwprintf(log_file,L"hand_weapon_component_end schema=1 acquired=%llu restored=%llu setter_calls=%llu "
        L"relinquished=%llu dirty=%u faulted=%u dirty_mask=%u owned_mask=%u revoked_mask=%u "
        L"hand_dirty=%u safe_to_unload=%u phase1_only=1 right_hand_only=1 ownership_proved=0\n",
        component::acquired,component::restored,component::setter_calls,component::relinquished,
        unsigned(component_dirty()),unsigned(component_faulted()),component::lease.dirty_mask(),
        component::lease.owned_mask(),component::lease.revoked_mask(),unsigned(trial.lease.dirty()),
        unsigned(!journals_dirty()));fflush(log_file);
}
bool component_apply(gears_hand_stream::Plan const& plan,gears_weapon::HandWorldFrame const& frame,
                     Snapshot const& hand,unsigned mask,unsigned lease_number){
    if(trial.xr_equipment_phase!=1||!(mask&2))return !component_dirty();
    component::Key key{};
    if(!component::capture(hand,key)||component_dirty()||component_faulted())return false;
    if(component::key_known&&!(key==component::fixed_key))return false;
    component::fixed_key=key;component::key_known=true;
    std::array<float,16> matrix{};int tick{};
    if(!component::world(key,matrix,tick))return false;
    if(!component::mount.bound()&&!component::mount.bind(frame.right,matrix,false))return false;
    gears_vr::Viewpoint pose{};
    if(!component::mount.solve(frame.right,pose))return false;
    component::State before{};
    if(!component::read(key,before))return false;
    auto wanted=before.value;wanted.absolute[0]=wanted.absolute[1]=true;
    std::array<uptr,3> flag_addresses{};std::array<unsigned,3> flag_masks{};
    if(!component::absolute_storage(key,component::baseline_absolute_storage,&flag_addresses,&flag_masks))return false;
    component::target_absolute_storage=component::baseline_absolute_storage;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(flag_addresses[i]==flag_addresses[j]){
        auto& bits=component::target_absolute_storage[i];
        bits=wanted.absolute[j]?bits|flag_masks[j]:bits&~flag_masks[j];
    }
    const playground::Vector position{float(pose.position.x),float(pose.position.y),float(pose.position.z)};
    const playground::Rotation rotation{gears_vr::to_rotator_units(pose.rotation.pitch),
        gears_vr::to_rotator_units(pose.rotation.yaw),gears_vr::to_rotator_units(pose.rotation.roll)};
    std::memcpy(wanted.translation.data(),&position,12);std::memcpy(wanted.rotation.data(),&rotation,12);
    component::Forward forward{plan,mask,{}};component::Backend backend{key,&forward};
    component::target=wanted;component::hand_lease=lease_number;component::plan_sequence=plan.ticket.sequence;
    component::plan_revision=plan.revision;component::action_epoch=plan.context.action_epoch;
    component::certificate=plan.ticket.certificate;
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    component::recorded=trial.record_lease;
#else
    component::recorded=true;
#endif
    const bool ok=component::lease.acquire(key,wanted,backend);
    if(ok)count_add(component::acquired);
    return ok;
}
bool bridge_before_allowed(uptr object,uptr function){
    // Foreign threads cannot read the private GT state or borrow its token.
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    if(component::consume_self(object,function))return true;
    // Only a registered, successfully named, known non-boundary call bypasses
    // related-object lookup. Unknown calls keep the original fallback below.
    wchar_t passive_name[96]{};
    if(registered(function)&&name(function,passive_name,96)&&!component_action_name(passive_name))return true;
    before_action(object,function,true);
    // Only related mutating action requests are blocked, not passive getters.
    // before_action synchronously drains/revokes its Plan for all watched names.
    if(!related(object)||!registered(function))return true;
    wchar_t label[96]{};if(!name(function,label,96))return false;
    return !component_action_name(label)||equipment_ready();
}
