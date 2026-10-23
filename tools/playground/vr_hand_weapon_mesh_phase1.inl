// Included after the real equipment backend, inside handstream. Private only.
// The mount is geometry; only the currently scoped hand Plan can grant a write.
namespace component {
namespace stow=vrequip::stow;
// Private name parsing only. No game metadata, success, or field address is
// cached. Every lookup follows the original live first-match traversal.
namespace lookup {
using ObjectKey=stow::ObjectKey;
std::size_t page_bytes(){
    static const std::size_t bytes=[](){SYSTEM_INFO info{};GetSystemInfo(&info);return std::size_t(info.dwPageSize);}();
    return bytes;
}
bool name(uptr object,wchar_t (&out)[128]){
    if(!object)return false;
    const auto n=get<Name>(object+0x48);const auto a=get<Array>(base+0x1c34800);
    if(!a.data||a.count<1000||a.count>a.capacity||a.capacity>4000000||n.index>=unsigned(a.count))return false;
    const auto entry=get<uptr>(a.data+n.index*8);
    if(!entry||get<unsigned>(entry)!=n.index)return false;
    const auto page=page_bytes();if(!page)return false;
    for(std::size_t at=0;at<128;){
        const auto address=entry+20+at*sizeof(wchar_t);
        const auto room=page-std::size_t(address%page);
        auto chars=(std::min)(std::size_t(128-at),room/sizeof(wchar_t));
        // An unaligned wchar may straddle the page. Preserve the original
        // single two-byte copy and its failure; never retry a guarded page.
        if(!chars)chars=1;
        if(!copy(out+at,address,chars*sizeof(wchar_t)))return false;
        for(std::size_t i=0;i<chars;++i)if(!out[at+i])return true;
        at+=chars;
    }
    out[127]=0;return false;
}
bool named(uptr object,const wchar_t* value){wchar_t text[128]{};return name(object,text)&&!_wcsicmp(text,value);}
bool derives(uptr cls,const wchar_t* value){
    for(unsigned n=0;cls&&n<40;++n,cls=get<uptr>(cls+0x60))if(named(cls,value))return true;
    return false;
}
bool instance(uptr object,const wchar_t* cls){
    if(!registered(object)||!derives(get<uptr>(object+0x50),cls))return false;
    wchar_t text[128]{};return name(object,text)&&wcsncmp(text,L"Default__",9);
}
uptr field(uptr cls,const wchar_t* value,const wchar_t* kind=nullptr){
    for(unsigned depth=0;cls&&depth<40;++depth,cls=get<uptr>(cls+0x60)){
        auto f=get<uptr>(cls+0x80);
        for(unsigned n=0;f&&n<4096;++n,f=get<uptr>(f+0x68))
            if(named(f,value)&&(!kind||named(get<uptr>(f+0x50),kind)))return f;
    }
    return 0;
}
uptr property(uptr object,const wchar_t* value){return registered(object)?field(get<uptr>(object+0x50),value):0;}
bool deleted(uptr object){
    if(!registered(object))return true;
    const auto p=property(object,L"bDeleteMe");
    return p&&(get<unsigned>(object+get<unsigned>(p+0x94))&get<unsigned>(p+0xd0));
}
bool field(uptr object,const wchar_t* label,const wchar_t* kind,void* output,unsigned bytes){
    const auto property=lookup::property(object,label);
    const auto cls=get<uptr>(object+0x50);
    if(!registered(property)||!registered(cls)||get<unsigned>(property+0x70)!=1||
       get<unsigned>(property+0x74)!=bytes||
       !lookup::named(get<uptr>(property+0x50),kind))return false;
    const auto offset=get<unsigned>(property+0x94),size=get<unsigned>(cls+0x88);
    return offset<=size&&bytes<=size-offset&&copy(output,object+offset,bytes);
}
bool pointer(uptr object,const wchar_t* label,uptr& out,bool component=false){
    return field(object,label,component?L"ComponentProperty":L"ObjectProperty",&out,sizeof(out));
}
bool boolean(uptr object,const wchar_t* label,bool& out){
    unsigned bits{};
    if(!field(object,label,L"BoolProperty",&bits,4))return false;
    const auto mask=get<unsigned>(lookup::property(object,label)+0xd0);
    if(!mask||(mask&(mask-1)))return false;
    out=(bits&mask)!=0;return true;
}
bool object_key(uptr object,ObjectKey& key){
    key={};
    if(!registered(object)||lookup::deleted(object))return false;
    Name n{};
    key.address=object;
    if(!copy(&key.index,object+8,4)||!copy(&key.outer,object+0x40,8)||
       !copy(&n,object+0x48,8)||!copy(&key.object_class,object+0x50,8)||
       !registered(key.object_class)||!registered(key.outer))return false;
    key.name_index=n.index;key.name_number=n.number;
    return registered(object)&&!lookup::deleted(object);
}
} // namespace lookup

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
} // namespace component
namespace component_trace {
void masks(Row& row) noexcept {
    row.pending=component::lease.dirty_mask();row.owned=component::lease.owned_mask();row.revoked=component::lease.revoked_mask();
    row.snapshot=component::lease.has_snapshot();row.faulted=component::lease.faulted();
}
} // namespace component_trace
namespace component {
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
                      std::array<unsigned,3>* masks=nullptr,bool fast=false){
    const wchar_t* fields[]={L"AbsoluteTranslation",L"AbsoluteRotation",L"AbsoluteScale"};
    for(unsigned i=0;i<3;++i){
        const auto object=key.objects[7].address,property=fast?lookup::property(object,fields[i]):playground::property(object,fields[i]);
        if(!(fast?lookup::field(object,fields[i],L"BoolProperty",&bits[i],4):stow::field(object,fields[i],L"BoolProperty",&bits[i],4)))return false;
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
bool current(Key const& expected,bool fast=false){
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    component_cost::AggregateSpan cost(component_cost::AggregateKind::current_total);
    const auto identify=[fast](uptr object,stow::ObjectKey& key){return fast?lookup::object_key(object,key):stow::object_key(object,key);};
    const auto link=[fast](uptr object,const wchar_t* label,uptr& value,bool component=false){return fast?lookup::pointer(object,label,value,component):stow::pointer(object,label,value,component);};
    const auto instance=[fast](uptr object,const wchar_t* label){return fast?lookup::instance(object,label):playground::instance(object,label);};
    for(auto const& old:expected.objects){
        stow::ObjectKey found{};
        if(!identify(old.address,found)||std::memcmp(&old,&found,sizeof(old)))return false;
    }
    stow::ObjectKey asset{};
    if(!identify(expected.asset.address,asset)||
       std::memcmp(&asset,&expected.asset,sizeof(asset)))return false;
    const auto address=[&](unsigned i){return expected.objects[i].address;};
    uptr pointer{};
    if(!link(address(2),L"WorldInfo",pointer)||pointer!=address(0)||
       !link(address(2),L"Pawn",pointer)||pointer!=address(3)||
       !link(address(3),L"Controller",pointer)||pointer!=address(2)||
       !link(address(3),L"InvManager",pointer)||pointer!=address(4)||
       !link(address(3),L"Mesh",pointer,true)||pointer!=address(5)||
       !link(address(3),L"Weapon",pointer)||pointer!=address(6)||
       !link(address(6),L"Instigator",pointer)||pointer!=address(3)||
       !link(address(6),L"Mesh",pointer,true)||pointer!=address(7)||
       !link(address(7),L"SkeletalMesh",pointer)||pointer!=expected.asset.address||
       get<uptr>(address(0)+0x40)!=address(1)||get<uptr>(address(2)+0x40)!=address(1)||
       get<uptr>(address(3)+0x40)!=address(1))return false;
    {
        component_cost::SlotOwner slot_cost;
        for(std::uint8_t i=0;i<4;++i){
            uptr slot{};if(!vrequip::read_slot(address(4),i,slot)||slot!=expected.slots[i])return false;
        }
    }
    return instance(address(6),L"Weap_AssaultRifle")&&
        instance(address(7),L"SkeletalMeshComponent")&&
        instance(expected.asset.address,L"SkeletalMesh")&&
        (fast?lookup::named(expected.asset.address,L"COGAssaultRifle"):playground::named(expected.asset.address,L"COGAssaultRifle"));
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
bool read(Key const& key,State& out,bool fast=false){
    component_cost::AggregateSpan cost(component_cost::AggregateKind::read_total);
    out={};
    if(!current(key,fast))return false;
    const auto mesh=key.objects[7].address;
    for(unsigned i=0;i<3;++i){
        const wchar_t* fields[]={L"AbsoluteTranslation",L"AbsoluteRotation",L"AbsoluteScale"};
        if(!(fast?lookup::boolean(mesh,fields[i],out.value.absolute[i]):stow::boolean(mesh,fields[i],out.value.absolute[i])))return false;
    }
    if(!triple_schema((fast?lookup::property(mesh,L"Translation"):playground::property(mesh,L"Translation")),false)||
       !triple_schema((fast?lookup::property(mesh,L"Rotation"):playground::property(mesh,L"Rotation")),true)||
       !(fast?lookup::field(mesh,L"Translation",L"StructProperty",out.value.translation.data(),12):stow::field(mesh,L"Translation",L"StructProperty",out.value.translation.data(),12))||
       !(fast?lookup::field(mesh,L"Rotation",L"StructProperty",out.value.rotation.data(),12):stow::field(mesh,L"Rotation",L"StructProperty",out.value.rotation.data(),12)))return false;
    std::array<float,3> position{};std::memcpy(position.data(),out.value.translation.data(),12);
    for(float x:position)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    out.key=key;out.known=true;return current(key,fast);
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
        using component_trace::gate;using component_trace::Stage;
        Snapshot hand{};
        if(!gate(Stage::prepare_ended,!trial.ended)||!gate(Stage::prepare_phase,trial.xr_equipment_phase==1)||
           !gate(Stage::prepare_epoch,trial.epoch==plan.context.action_epoch)||!gate(Stage::prepare_tick,trial.tick_epoch==trial.epoch)||
           !gate(Stage::prepare_snapshot,component_cost::snapshot(component_trace::state.meta.group,
               component_trace::state.meta.forward,lease.faulted(),[&](){return handik::snapshot(key.objects[2].address,hand);}))||
           !gate(Stage::prepare_identity,xr_identity(hand))||!gate(Stage::prepare_idle,hand.action_idle)||
           !gate(Stage::prepare_baseline,handik::baseline_strengths(hand))||!gate(Stage::prepare_quiet,slots_quiet(hand))||
           !gate(Stage::prepare_equipment,xr_equipment_gate(hand,1,L"component_setter",EquipmentTiming::prewrite_phase)))return false;
        context=xr_context(hand,trial.xr_ready);gate(Stage::prepare_context,true);return true;
    }
    bool authorize(){
        using component_trace::gate;using component_trace::Stage;
        return gate(Stage::authorize_ended,!trial.ended)&&gate(Stage::authorize_phase,trial.xr_equipment_phase==1)&&
            gate(Stage::authorize_tick,trial.tick_epoch==trial.epoch)&&gate(Stage::authorize_epoch,trial.epoch==plan.context.action_epoch)&&
            gate(Stage::authorize_hand,(mask&2)!=0)&&
            gate(Stage::authorize,authorize_current_xr_plan(plan,context,GetTickCount64())==mask);
    }
};
struct Backend {
    Key expected_key{};
    Forward* forward{}; // nullptr is rollback, authenticated by old Key/value only.
    bool read(State& state){
        component_names::Scope names_scope(forward&&!lease.faulted());
        component_cost::Owner cost_owner(component_trace::state.meta.group,forward&&!lease.faulted());
        component_trace::Scope scope;
        const bool ok=component::read(expected_key,state,forward&&!lease.faulted());
        const auto saved=component_trace::state.meta;
        component_trace::state.meta.forward=forward!=nullptr;
        if(!component_trace::state.meta.group)component_trace::state.meta.expected=lease.owned_value();
        component_trace::Row row{};row.stage=component_trace::Stage::backend_read;row.passed=ok;
        row.read_executed=true;row.read_known=state.known;row.key_equal=state.key==expected_key;
        row.read_value=state.value;row.compare_known=lease.has_snapshot()&&state.known;
        if(row.compare_known)row.mismatch=component_trace::mismatch(state.value,component_trace::state.meta.expected);
        component_trace::add(row);
        // Only pure acquisition reads have no setter group, a forward guard,
        // a baseline snapshot and no earlier fault. Their expected full value
        // is the journal's current owned value. Rollback is deliberately excluded.
        if(ok&&state.known&&row.key_equal&&!component_trace::state.meta.group&&forward&&lease.has_snapshot()&&!lease.faulted()){
            row.stage=component_trace::Stage::pure_acquire_value;row.passed=row.mismatch==0;component_trace::add(row);
        }
        component_trace::state.meta=saved;return ok;
    }
    bool schema(playground::Args& call,unsigned group){
        using component_trace::gate;using component_trace::Stage;
        if(!gate(Stage::schema_valid,call.valid)||!gate(Stage::schema_registered,registered(call.r.function))||
           !gate(Stage::schema_size,call.r.size==12)||
           !gate(Stage::schema_class,playground::named(get<uptr>(call.r.function+0x50),L"Function"))||
           !gate(Stage::schema_return,get<unsigned short>(call.r.function+0x108)==0xffff))return false;
        const auto native=get<uptr>(call.r.function+0x114);
        if(!gate(Stage::schema_native,native>=base+0x1000&&native<base+0x131b000))return false;
        if(group==1){
            if(!gate(Stage::schema_count,stow::parameters(call)==3))return false;
            const wchar_t* fields[]={L"NewAbsoluteTranslation",L"NewAbsoluteRotation",L"NewAbsoluteScale"};
            for(unsigned i=0;i<3;++i){
                if(!gate(Stage::schema_bool_argument,stow::argument(call,fields[i],L"BoolProperty",4,i*4,0x80),i))return false;
                const auto mask=get<unsigned>(call.prop(fields[i])+0xd0);
                if(!gate(Stage::schema_bool_mask,mask&&!(mask&(mask-1)),i,mask))return false;
            }
            return true;
        }
        const auto field=group==2?L"NewTranslation":L"NewRotation";
        return gate(Stage::schema_count,stow::parameters(call)==1)&&
            gate(Stage::schema_struct_argument,stow::argument(call,field,L"StructProperty",12,0,0x80))&&
            gate(Stage::schema_triple,triple_schema(call.prop(field),group==4));
    }
    bool set(Key const& key,Value const& expected,Value const& wanted,unsigned group){
        component_names::Scope names_scope(forward&&!lease.faulted());
        using component_trace::gate;using component_trace::Stage;
        component_trace::SetScope scope(group,forward!=nullptr,expected,wanted,
            (component_trace::mismatch(wanted,lease.original_value())&group)==0);
        if(!gate(Stage::set_key,key==expected_key))return false;
        const wchar_t* function=group==1?L"SetAbsolute":group==2?L"SetTranslation":L"SetRotation";
        playground::Args call(key.objects[7].address,function);
        if(!schema(call,group))return false;
        if(group==1){
            call.flag(L"NewAbsoluteTranslation",wanted.absolute[0]);
            call.flag(L"NewAbsoluteRotation",wanted.absolute[1]);call.flag(L"NewAbsoluteScale",wanted.absolute[2]);
        }else if(!gate(Stage::set_bytes,call.bytes(group==2?L"NewTranslation":L"NewRotation",
                  group==2?wanted.translation.data():wanted.rotation.data(),12)))return false;
        State before{};
        if(!gate(Stage::set_read_before,read(before))||
           !gate(Stage::set_expected_before,gears_weapon::mesh_component_exact(before.value,expected),0,
                 component_trace::mismatch(before.value,expected)))return false;
        if(forward&&!forward->prepare(key))return false;
        if(!gate(Stage::set_read_after,read(before))||
           !gate(Stage::set_expected_after,gears_weapon::mesh_component_exact(before.value,expected),0,
                 component_trace::mismatch(before.value,expected))||!schema(call,group)||
           (forward&&!forward->authorize()))return false;
        if(!gate(Stage::set_owned,(lease.owned_mask()&group)!=0))return false;
        SelfScope self_call(call.r.object,call.r.function);count_add(setter_calls);
        component_trace::Row attempt{};attempt.stage=Stage::call_begin;attempt.passed=true;attempt.call_attempted=true;component_trace::add(attempt);
        const bool called=call.run();
        component_trace::Row result{};result.stage=Stage::call_end;result.passed=called;
        result.call_attempted=result.call_return_known=true;result.call_returned=called;component_trace::add(result);
        return called;
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
    component_trace::Scope trace_scope;component_trace::gate(component_trace::Stage::restore_enter,true);
    component::Backend backend{component::lease.key(),nullptr};
    const bool clean=component::lease.restore(backend);
    if(clean)count_add(component::restored);
    component::recorded=false;publish_journals();
    return component_trace::gate(component_trace::Stage::restore_result,clean&&!component_faulted(),unsigned(clean));
}
bool component_reset(){
    if(!component::lease.reset())return false;
    component_trace::reset();component_cost::reset();
    component::mount.reset();component::fixed_key={};component::key_known=component::recorded=false;
    component::terminal_dumped=component::terminal_clean_dumped=false;
    component::acquired=component::restored=component::setter_calls=component::relinquished=0;
    component::hand_lease=0;component::self={};return true;
}
void component_dump_end(){
    if(!trial.ended||!log_file)return;
    component_cost::dump(!journals_dirty());
    component_trace::DumpAtExit trace_dump;
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
    component_cost::begin(plan.ticket.sequence,plan.revision,lease_number);
    component_cost::Span initial_cost(component_cost::Kind::initial,0,!component::lease.faulted());
    component_trace::begin();component_trace::Scope trace_scope;
    component_trace::gate(component_trace::Stage::acquire_enter,true);
    component::Key key{};
    component::Value wanted{};
    {
        component_names::Scope names_scope(!component::lease.faulted());
        component_cost::Owner cost_owner(0,!component::lease.faulted());
        if(!component_trace::gate(component_trace::Stage::capture,component::capture(hand,key))||
           !component_trace::gate(component_trace::Stage::component_clean,!component_dirty())||
           !component_trace::gate(component_trace::Stage::component_not_faulted,!component_faulted()))return false;
        if(component::key_known&&!component_trace::gate(component_trace::Stage::key_match,key==component::fixed_key))return false;
        component::fixed_key=key;component::key_known=true;
        std::array<float,16> matrix{};int tick{};
        if(!component_trace::gate(component_trace::Stage::world,component::world(key,matrix,tick)))return false;
        if(!component::mount.bound()&&!component_trace::gate(component_trace::Stage::mount_bind,component::mount.bind(frame.right,matrix,false)))return false;
        gears_vr::Viewpoint pose{};
        if(!component_trace::gate(component_trace::Stage::mount_solve,component::mount.solve(frame.right,pose)))return false;
        component::State before{};
        if(!component_trace::gate(component_trace::Stage::initial_read,component::read(key,before,true)))return false;
        wanted=before.value;wanted.absolute[0]=wanted.absolute[1]=true;
        std::array<uptr,3> flag_addresses{};std::array<unsigned,3> flag_masks{};
        if(!component_trace::gate(component_trace::Stage::absolute_storage,
            component::absolute_storage(key,component::baseline_absolute_storage,&flag_addresses,&flag_masks,true)))return false;
        component::target_absolute_storage=component::baseline_absolute_storage;
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(flag_addresses[i]==flag_addresses[j]){
            auto& bits=component::target_absolute_storage[i];
            bits=wanted.absolute[j]?bits|flag_masks[j]:bits&~flag_masks[j];
        }
        const playground::Vector position{float(pose.position.x),float(pose.position.y),float(pose.position.z)};
        const playground::Rotation rotation{gears_vr::to_rotator_units(pose.rotation.pitch),
            gears_vr::to_rotator_units(pose.rotation.yaw),gears_vr::to_rotator_units(pose.rotation.roll)};
        std::memcpy(wanted.translation.data(),&position,12);std::memcpy(wanted.rotation.data(),&rotation,12);
    }
    component::Forward forward{plan,mask,{}};component::Backend backend{key,&forward};
    component::target=wanted;component::hand_lease=lease_number;component::plan_sequence=plan.ticket.sequence;
    component::plan_revision=plan.revision;component::action_epoch=plan.context.action_epoch;
    component::certificate=plan.ticket.certificate;
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    component::recorded=trial.record_lease;
#else
    component::recorded=true;
#endif
    initial_cost.finish(true);
    const bool ok=component::lease.acquire(key,wanted,backend);
    component_trace::gate(component_trace::Stage::acquire_result,ok);
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
