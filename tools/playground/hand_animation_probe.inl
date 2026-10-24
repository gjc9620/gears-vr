// Default-OFF, bounded, test-arena-only observation. This file contains no
// engine invocation, property setter, skeletal write, state change or input.
namespace handanim {
using Kind=gears_hand_animation::Kind;
using Source=gears_hand_animation::Source;
using Token=gears_hand_animation::Token;
std::atomic<bool> requested{},stop_requested{},active_visible{};
std::atomic<unsigned> status{},frames{};
constexpr unsigned max_slots=32,max_slot_allocation=256,max_children=8,max_fields=512,max_functions=512;
struct Field {uptr cls{},property{};const wchar_t* label{};const wchar_t* kind{};
    unsigned offset{},bytes{},dimension{},mask{};bool valid{};};
struct Function {unsigned index{};Kind kind{};bool watched{};};
struct Control {uptr object{},component{},next{},outer{},cls{};unsigned known{},list_mask{};
    Name control_name{},object_name{};float strength{},target{},blend{};};
struct Child {uptr object{};unsigned known{};float weight{},total{},target{},time{};
    Name animation{};bool playing{};};
struct Slot {uptr object{},component{};Name node_name{};unsigned known{};
    float total{},blend{},pending{};int custom_index{-1},target_index{-1};
    bool playing{};int children{};std::array<Child,max_children> child{};};
struct Snapshot {uptr world{},pc{},pawn{},mesh{},tree{},weapon{};unsigned known{};
    std::uint8_t special{};bool melee{},wants_melee{},switching{},special_attack{};
    float world_time{};int health{},slot_count{},control_list_count{};bool control_lists_known{};
    std::array<Control,8> controls{};std::array<Slot,max_slots> slots{};};
struct Probe {
    bool active{},field_limit{},function_limit{};uptr world{},pc{},pawn{},mesh{},tree{};
    unsigned snapshot{},event_lines{},hook_callbacks{},call_callbacks{},call_accepted{},
        call_completed{},call_frame_rejected{};ULONGLONG deadline{},next_sample{};
    std::vector<Field> fields;std::vector<Function> functions;
    std::array<uptr,320> watched{};unsigned watched_count{};
    gears_hand_animation::Ledger ledger;
};
Probe probe{};
struct CallerFrame {uptr node{},object{},code{},locals{},previous{},script{};int script_size{};};
bool caller_frame(uptr address,CallerFrame& out){
    if(!address||(address&7)||address>UINTPTR_MAX-0x40)return false;
    CallerFrame value{};Array script{};
    if(!copy(&value.node,address+0x18,8)||!copy(&value.object,address+0x20,8)||
       !copy(&value.code,address+0x28,8)||!copy(&value.locals,address+0x30,8)||
       !copy(&value.previous,address+0x38,8)||!registered(value.node)||
       !registered(value.object)||!playground::named(get<uptr>(value.node+0x50),L"Function")||
       !copy(&script,value.node+0x8c,sizeof(script))||!script.data||script.count<=0||
       script.count>script.capacity||script.capacity>131072||
       script.data>UINTPTR_MAX-unsigned(script.count)||value.code<script.data||
       value.code>script.data+unsigned(script.count))return false;
    value.script=script.data;value.script_size=script.count;out=value;return true;
}
bool test_environment(){
    for(const auto label:{L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY",L"GEARSVR_TEST_WINDOWS_BEHIND",
                          L"GEARSVR_TEST_MUTE_GAME"}){
        wchar_t value[4]{};
        if(GetEnvironmentVariableW(label,value,4)!=1||value[0]!=L'1')return false;
    }
    return true;
}
Field const* field(uptr object,const wchar_t* label,const wchar_t* kind,unsigned bytes){
    const auto cls=get<uptr>(object+0x50);
    for(auto const& cached:probe.fields)
        if(cached.cls==cls&&!wcscmp(cached.label,label)&&!wcscmp(cached.kind,kind))
            return cached.valid&&cached.bytes==bytes?&cached:nullptr;
    if(probe.fields.size()>=max_fields){probe.field_limit=true;return nullptr;}
    Field value{};value.cls=cls;value.label=label;value.kind=kind;
    if(registered(object)&&registered(cls)){
        value.property=playground::property(object,label);
        if(registered(value.property)&&playground::named(get<uptr>(value.property+0x50),kind)){
            value.offset=get<unsigned>(value.property+0x94);
            value.bytes=get<unsigned>(value.property+0x74);
            value.dimension=get<unsigned>(value.property+0x70);
            value.mask=get<unsigned>(value.property+0xd0);
            const auto size=get<unsigned>(cls+0x88);
            value.valid=value.dimension==1&&value.bytes==bytes&&value.offset<=size&&bytes<=size-value.offset;
        }
    }
    probe.fields.push_back(value);
    return value.valid?&probe.fields.back():nullptr;
}
bool read(uptr object,const wchar_t* label,const wchar_t* kind,void* out,unsigned bytes){
    const auto layout=field(object,label,kind,bytes);
    return layout&&copy(out,object+layout->offset,bytes);
}
template<class T>bool read(uptr object,const wchar_t* label,const wchar_t* kind,T& out){
    return read(object,label,kind,&out,sizeof(out));
}
bool boolean(uptr object,const wchar_t* label,bool& out){
    const auto layout=field(object,label,L"BoolProperty",4);unsigned bits{};
    if(!layout||!layout->mask||(layout->mask&(layout->mask-1))||!copy(&bits,object+layout->offset,4))return false;
    out=(bits&layout->mask)!=0;return true;
}
bool pointer(uptr object,const wchar_t* label,uptr& out,bool component=false){
    return read(object,label,L"ObjectProperty",out)||
        (component&&read(object,label,L"ComponentProperty",out));
}
bool floating(uptr object,const wchar_t* label,float& out){
    return read(object,label,L"FloatProperty",out)&&std::isfinite(out);
}
bool array(uptr object,const wchar_t* label,const wchar_t* inner_kind,unsigned element_bytes,
           unsigned maximum,Array& out,uptr* inner_out=nullptr){
    const auto layout=field(object,label,L"ArrayProperty",sizeof(Array));
    if(!layout||!copy(&out,object+layout->offset,sizeof(out))||out.count<0||
       out.capacity<out.count||unsigned(out.capacity)>maximum||(out.count&&!out.data))return false;
    const auto inner=get<uptr>(layout->property+0xd0);
    if(!registered(inner)||!playground::named(get<uptr>(inner+0x50),inner_kind)||
       get<unsigned>(inner+0x70)!=1||get<unsigned>(inner+0x74)!=element_bytes)return false;
    if(inner_out)*inner_out=inner;return true;
}
bool structure_field(uptr structure,const wchar_t* label,const wchar_t* kind,
                     unsigned bytes,unsigned stride,unsigned& offset){
    const auto property=playground::field(structure,label,kind);
    if(!registered(property)||get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=bytes)return false;
    offset=get<unsigned>(property+0x94);return offset<=stride&&bytes<=stride-offset;
}
bool text(Name value,wchar_t* out,unsigned maximum){
    if(!maximum)return false;out[0]=0;
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||names.capacity>4000000||
       value.index>=unsigned(names.count))return false;
    const auto entry=get<uptr>(names.data+size_t(value.index)*8);
    if(!entry||get<unsigned>(entry)!=value.index)return false;
    for(unsigned i=0;i+1<maximum;++i){
        if(!copy(&out[i],entry+20+size_t(i)*2,2)){out[i]=0;return false;}
        if(!out[i])return true;
        if(out[i]<=L' '||out[i]==L'='||out[i]==L','||out[i]==L':')out[i]=L'_';
    }
    out[maximum-1]=0;return false;
}
void watch(uptr object){
    if(!object)return;
    for(unsigned i=0;i<probe.watched_count;++i)if(probe.watched[i]==object)return;
    if(probe.watched_count<probe.watched.size())probe.watched[probe.watched_count++]=object;
}
void read_control(uptr pawn,unsigned index,Control& out){
    constexpr const wchar_t* labels[]={L"IKBoneCtrl_RightHand",L"IKBoneRotCtrl_RightHand",
        L"IKCtrl_RightHand",L"IKRotCtrl_RightHand",L"IKBoneCtrl_LeftHand",L"IKBoneRotCtrl_LeftHand",
        L"IKCtrl_LeftHand",L"IKRotCtrl_LeftHand"};
    if(!pointer(pawn,labels[index],out.object)||!playground::instance(out.object,L"SkelControlBase")||
       playground::deleted(out.object))return;
    out.known|=1;out.outer=get<uptr>(out.object+0x40);out.cls=get<uptr>(out.object+0x50);
    if(pointer(out.object,L"SkelComponent",out.component,true))out.known|=2;
    if(pointer(out.object,L"NextControl",out.next))out.known|=4;
    if(floating(out.object,L"ControlStrength",out.strength))out.known|=8;
    if(floating(out.object,L"StrengthTarget",out.target))out.known|=16;
    if(floating(out.object,L"BlendTimeToGo",out.blend))out.known|=32;
    if(read(out.object,L"ControlName",L"NameProperty",out.control_name))out.known|=64;
    if(copy(&out.object_name,out.object+0x48,sizeof(out.object_name)))out.known|=128;
}
void read_control_lists(Snapshot& out){
    const auto layout=field(out.tree,L"SkelControlLists",L"ArrayProperty",sizeof(Array));Array lists{};
    if(!layout||!copy(&lists,out.tree+layout->offset,sizeof(lists))||lists.count<0||
       lists.capacity<lists.count||lists.capacity>32||(lists.count&&!lists.data))return;
    const auto inner=get<uptr>(layout->property+0xd0);
    if(!registered(inner)||!playground::named(get<uptr>(inner+0x50),L"StructProperty")||
       get<unsigned>(inner+0x70)!=1)return;
    const auto structure=get<uptr>(inner+0xd0);
    const auto stride=get<unsigned>(inner+0x74);unsigned head_offset{};
    if(!registered(structure)||stride<16||stride>64||get<unsigned>(structure+0x88)!=stride||
       !structure_field(structure,L"ControlHead",L"ObjectProperty",8,stride,head_offset))return;
    out.control_list_count=lists.count;
    for(int i=0;i<lists.count;++i){uptr current{};
        if(!copy(&current,lists.data+size_t(i)*stride+head_offset,8))return;
        std::array<uptr,16> visited{};unsigned count{};
        while(current){
            if(count==visited.size()||!playground::instance(current,L"SkelControlBase")||playground::deleted(current))return;
            for(unsigned n=0;n<count;++n)if(visited[n]==current)return;
            visited[count++]=current;
            for(auto& control:out.controls)if(control.object==current)control.list_mask|=1u<<unsigned(i);
            uptr next{};if(!pointer(current,L"NextControl",next))return;current=next;
        }
    }
    out.control_lists_known=true;
}
void read_slot(uptr object,Slot& out){
    out.object=object;
    if(!playground::instance(object,L"AnimNodeSlot")||playground::deleted(object))return;
    out.known|=1;watch(object);
    if(pointer(object,L"SkelComponent",out.component,true))out.known|=2;
    if(read(object,L"NodeName",L"NameProperty",out.node_name))out.known|=4;
    if(floating(object,L"NodeTotalWeight",out.total))out.known|=8;
    if(floating(object,L"BlendTimeToGo",out.blend))out.known|=16;
    if(floating(object,L"PendingBlendOutTime",out.pending))out.known|=32;
    if(read(object,L"CustomChildIndex",L"IntProperty",out.custom_index))out.known|=64;
    if(read(object,L"TargetChildIndex",L"IntProperty",out.target_index))out.known|=128;
    if(boolean(object,L"bIsPlayingCustomAnim",out.playing))out.known|=256;
    Array children{},targets{};uptr inner{};
    const auto children_field=field(object,L"Children",L"ArrayProperty",sizeof(Array));
    if(!children_field||!copy(&children,object+children_field->offset,sizeof(children))||
       children.count<0||children.capacity<children.count||children.capacity>int(max_children)||
       (children.count&&!children.data))return;
    inner=get<uptr>(children_field->property+0xd0);
    if(!registered(inner)||!playground::named(get<uptr>(inner+0x50),L"StructProperty")||
       get<unsigned>(inner+0x70)!=1)return;
    const auto structure=get<uptr>(inner+0xd0);
    const auto stride=get<unsigned>(inner+0x74);
    if(!registered(structure)||stride<16||stride>256||get<unsigned>(structure+0x88)!=stride)return;
    unsigned anim_offset{},weight_offset{},total_offset{};
    if(!structure_field(structure,L"Anim",L"ObjectProperty",8,stride,anim_offset)||
       !structure_field(structure,L"Weight",L"FloatProperty",4,stride,weight_offset)||
       !structure_field(structure,L"TotalWeight",L"FloatProperty",4,stride,total_offset))return;
    out.children=children.count;out.known|=512;
    const bool targets_known=array(object,L"TargetWeight",L"FloatProperty",4,max_children,targets)&&
        targets.count==children.count;
    if(targets_known)out.known|=1024;
    for(int i=0;i<children.count;++i){
        auto& child=out.child[size_t(i)];const auto address=children.data+size_t(i)*stride;
        if(copy(&child.object,address+anim_offset,8)&&copy(&child.weight,address+weight_offset,4)&&
           copy(&child.total,address+total_offset,4)&&std::isfinite(child.weight)&&std::isfinite(child.total))child.known|=1;
        if(targets_known&&copy(&child.target,targets.data+size_t(i)*4,4)&&std::isfinite(child.target))child.known|=2;
        if(playground::instance(child.object,L"AnimNodeSequence")&&!playground::deleted(child.object)){
            watch(child.object);
            if(read(child.object,L"AnimSeqName",L"NameProperty",child.animation))child.known|=4;
            if(floating(child.object,L"CurrentTime",child.time))child.known|=8;
            if(boolean(child.object,L"bPlaying",child.playing))child.known|=16;
        }
    }
}
bool snapshot(Snapshot& out){
    if(!playground::automatic||!playground::ready||!test_environment()||
       playground::arena_pc!=probe.pc||playground::arena_pawn!=probe.pawn||
       playground::arena_world!=probe.world||!playground::instance(probe.pc,L"WarPC")||
       playground::deleted(probe.pc))return false;
    out.pc=probe.pc;
    if(!pointer(out.pc,L"Pawn",out.pawn)||out.pawn!=probe.pawn||
       !pointer(out.pc,L"WorldInfo",out.world)||out.world!=probe.world||
       !playground::instance(out.world,L"WorldInfo")||playground::deleted(out.world)||
       !playground::instance(out.pawn,L"WarPawn")||playground::deleted(out.pawn))return false;
    uptr controller{};
    if(!pointer(out.pawn,L"Controller",controller)||controller!=out.pc||
       !pointer(out.pawn,L"Mesh",out.mesh,true)||out.mesh!=probe.mesh||
       !playground::instance(out.mesh,L"SkeletalMeshComponent")||playground::deleted(out.mesh)||
       !pointer(out.mesh,L"Animations",out.tree)||out.tree!=probe.tree||
       !playground::instance(out.tree,L"AnimTree")||playground::deleted(out.tree))return false;
    probe.watched_count=0;
    if(pointer(out.pawn,L"Weapon",out.weapon))out.known|=1;
    if(read(out.pawn,L"SpecialMove",L"ByteProperty",out.special))out.known|=2;
    if(boolean(out.pawn,L"bDoingMeleeAttack",out.melee))out.known|=4;
    if(boolean(out.pawn,L"bWantsToMelee",out.wants_melee))out.known|=8;
    if(boolean(out.pawn,L"bSwitchingWeapons",out.switching))out.known|=16;
    if(boolean(out.pawn,L"bDoingSpecialAttackMove",out.special_attack))out.known|=32;
    if(floating(out.world,L"TimeSeconds",out.world_time))out.known|=64;
    if(read(out.pawn,L"Health",L"IntProperty",out.health))out.known|=128;
    for(unsigned i=0;i<out.controls.size();++i)read_control(out.pawn,i,out.controls[i]);
    read_control_lists(out);
    Array slots{};
    if(array(out.pawn,L"BodyStanceNodes",L"ObjectProperty",8,max_slot_allocation,slots)&&
       slots.count<=int(max_slots)){
        out.known|=256;out.slot_count=slots.count;
        for(int i=0;i<slots.count;++i){uptr object{};
            if(copy(&object,slots.data+size_t(i)*8,8))read_slot(object,out.slots[size_t(i)]);
        }
    }
    return true;
}
void emit_snapshot(Snapshot const& value,const wchar_t* phase,unsigned call){
    const auto serial=++probe.snapshot;LARGE_INTEGER qpc{};QueryPerformanceCounter(&qpc);
    const auto transition=probe.ledger.poll({(value.known&2)!=0,(value.known&4)!=0,value.special,value.melee});
    fwprintf(log_file,L"hand_animation_frame snapshot=%u sample=%u trigger_call=%u phase=%ls ms=%llu qpc=%lld "
        L"world_time=%.9g state_known=%u pc=%p pawn=%p mesh=%p tree=%p weapon=%p special=%u melee=%u "
        L"wants_melee=%u switching=%u special_attack=%u health=%d slots=%d control_lists_known=%u control_lists=%d "
        L"pickup_request=%u melee_request=%u "
        L"pickup_episode=%u melee_episode=%u transition=%u previous_known=%u previous_special=%u previous_melee=%u ownership_proved=0 writes=0\n",
        serial,frames.load(),call,phase,GetTickCount64(),qpc.QuadPart,value.world_time,value.known,
        reinterpret_cast<void*>(value.pc),reinterpret_cast<void*>(value.pawn),reinterpret_cast<void*>(value.mesh),
        reinterpret_cast<void*>(value.tree),reinterpret_cast<void*>(value.weapon),unsigned(value.special),unsigned(value.melee),
        unsigned(value.wants_melee),unsigned(value.switching),unsigned(value.special_attack),value.health,value.slot_count,
        unsigned(value.control_lists_known),value.control_list_count,
        probe.ledger.pickup_request(),probe.ledger.melee_request(),transition.pickup_episode,transition.melee_episode,
        transition.mask,unsigned(transition.before.special_known)|(unsigned(transition.before.melee_known)<<1),
        unsigned(transition.before.special),unsigned(transition.before.melee));
    for(unsigned i=0;i<value.controls.size();++i){auto const& c=value.controls[i];wchar_t control_name[96]{};
        text(c.control_name,control_name,96);
        fwprintf(log_file,L"hand_animation_control snapshot=%u index=%u known=%u object=%p object_index=%u "
            L"object_name_index=%u object_name_number=%u name=%ls name_index=%u name_number=%u "
            L"class=%p outer=%p component=%p next=%p strength=%.9g target=%.9g blend=%.9g mesh_match=%u tree_match=%u list_mask=%x\n",
            serial,i,c.known,reinterpret_cast<void*>(c.object),get<unsigned>(c.object+8),
            c.object_name.index,c.object_name.number,control_name[0]?control_name:L"unknown",c.control_name.index,c.control_name.number,
            reinterpret_cast<void*>(c.cls),reinterpret_cast<void*>(c.outer),reinterpret_cast<void*>(c.component),reinterpret_cast<void*>(c.next),
            c.strength,c.target,c.blend,unsigned(c.component==value.mesh),unsigned(c.outer==value.tree),c.list_mask);
    }
    for(int i=0;i<value.slot_count;++i){auto const& s=value.slots[size_t(i)];wchar_t node_name[96]{};
        text(s.node_name,node_name,96);
        fwprintf(log_file,L"hand_animation_slot snapshot=%u index=%d known=%u object=%p object_index=%u "
            L"node=%ls node_index=%u node_number=%u component=%p mesh_match=%u total=%.9g blend=%.9g pending=%.9g "
            L"playing=%u custom_index=%d target_index=%d children=%d child_data=",
            serial,i,s.known,reinterpret_cast<void*>(s.object),get<unsigned>(s.object+8),node_name[0]?node_name:L"unknown",
            s.node_name.index,s.node_name.number,reinterpret_cast<void*>(s.component),unsigned(s.component==value.mesh),
            s.total,s.blend,s.pending,unsigned(s.playing),s.custom_index,s.target_index,s.children);
        for(int j=0;j<s.children;++j){auto const& c=s.child[size_t(j)];wchar_t animation[96]{};text(c.animation,animation,96);
            fwprintf(log_file,L"%ls%p:%u:%.9g:%.9g:%.9g:%u:%u:%ls:%.9g:%u",j?L",":L"",
                reinterpret_cast<void*>(c.object),c.known,c.weight,c.total,c.target,c.animation.index,c.animation.number,
                animation[0]?animation:L"unknown",c.time,unsigned(c.playing));
        }
        fwprintf(log_file,L"\n");
    }
}
void finish(unsigned code,const wchar_t* reason){
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    gripdamage::freeze(code);
#endif
    probe.active=false;active_visible.store(false,std::memory_order_release);
    status.store(code,std::memory_order_release);
    if(log_file){
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
        fwprintf(log_file,L"hand_animation_call_probe_end callbacks=%u accepted=%u completed=%u frame_rejected=%u mask=%llx writes=0\n",
            probe.call_callbacks,probe.call_accepted,probe.call_completed,probe.call_frame_rejected,probe.ledger.call_mask());
#endif
        fwprintf(log_file,L"hand_animation_probe_end status=%u reason=%ls samples=%u snapshots=%u calls=%u "
        L"event_lines=%u hook_callbacks=%u hook_mask=%llx bridge_mask=%llx special_changes=%u melee_changes=%u "
        L"started_active=%u overflow=%u field_limit=%u function_limit=%u ownership_proved=0 writes=0 "
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
        L"gap=native_direct_calls_uninstrumented;caller_parameters_not_decoded;idle_is_not_release;slot_reuse_requires_review\n",
#else
        L"gap=vm_direct_calls_uninstrumented;idle_is_not_release;state_name_unobserved;slot_reuse_requires_review\n",
#endif
        code,reason,frames.load(),probe.snapshot,probe.ledger.calls(),probe.event_lines,probe.hook_callbacks,
        probe.ledger.hook_mask(),probe.ledger.bridge_mask(),probe.ledger.special_changes(),probe.ledger.melee_changes(),
        unsigned(probe.ledger.started_active()),unsigned(probe.ledger.overflow()),unsigned(probe.field_limit),unsigned(probe.function_limit));
        fflush(log_file);}
}
bool classify(uptr function,Kind& kind){
    const auto index=get<Name>(function+0x48).index;
    for(auto const& cached:probe.functions)if(cached.index==index){kind=cached.kind;return cached.watched;}
    if(probe.functions.size()>=max_functions){probe.function_limit=true;return false;}
    wchar_t label[96]{};name(function,label,96);
    struct Match {const wchar_t* label;Kind kind;};
    constexpr Match matches[]={
        {L"ServerUse",Kind::use_request},{L"GrabPickup",Kind::grab_pickup},
        {L"ServerDictateSpecialMove",Kind::dictate_special},{L"DoSpecialMove",Kind::do_special},
        {L"EndSpecialMove",Kind::end_special},{L"StartSpecialMove",Kind::special_start},
        {L"StopSpecialMove",Kind::special_stop},{L"BodyStanceAnimEndNotification",Kind::body_end},
        {L"OnAnimEnd",Kind::on_anim_end},{L"PlayBodyStance",Kind::body_play},
        {L"PlayBodyStanceByDuration",Kind::body_play},{L"SetBodyStance",Kind::body_play},
        {L"StopBodyStance",Kind::body_stop},{L"StopAllBodyStances",Kind::body_stop},
        {L"StartMeleeAttack",Kind::melee_request},{L"StopMeleeAttack",Kind::melee_stop_request},
        {L"ForceStopMeleeAttack",Kind::melee_stop_request},{L"ForciblyStopMeleeAttack",Kind::melee_stop_request},
        {L"ClientStopMeleeAttack",Kind::melee_stop_request},{L"MeleeAttackStarted",Kind::melee_begin},
        {L"MeleeAttackEnded",Kind::melee_end},{L"BeginState",Kind::melee_state_begin},
        {L"EndState",Kind::melee_state_end},{L"PlayCustomAnim",Kind::slot_play},
        {L"PlayCustomAnimByDuration",Kind::slot_play},{L"StopCustomAnim",Kind::slot_stop}};
    bool watched=false;
    for(auto const& item:matches)if(!wcscmp(label,item.label)){kind=item.kind;watched=true;break;}
    // Begin/EndState names are shared across states. Cache by name only above,
    // then independently require the precise state owner at each call below.
    probe.functions.push_back({index,kind,watched});return watched;
}
bool related(uptr object){
    if(object==probe.pc||object==probe.pawn)return true;
    for(unsigned i=0;i<probe.watched_count;++i)if(probe.watched[i]==object)return registered(object);
    if(playground::instance(object,L"WarWeapon")&&!playground::deleted(object)){
        uptr pawn{};return pointer(object,L"Instigator",pawn)&&pawn==probe.pawn;
    }
    if(playground::instance(object,L"WarSpecialMove")&&!playground::deleted(object)){
        uptr pawn{};return pointer(object,L"PawnOwner",pawn)&&pawn==probe.pawn;
    }
    return false;
}
void parameters(uptr function,void* params){
    if(!params)return;
    const auto bytes=get<unsigned short>(function+0x106);unsigned visited=0;
    for(auto property=get<uptr>(function+0x80);property&&visited<48;property=get<uptr>(property+0x68),++visited){
        if(!registered(property))break;
        if(!(get<unsigned long long>(property+0x78)&0x80))continue;
        wchar_t label[96]{},kind[64]{};name(property,label,96);name(get<uptr>(property+0x50),kind,64);
        const bool wanted=!wcscmp(label,L"AMove")||!wcscmp(label,L"NewMove")||!wcscmp(label,L"NewSpecialMove")||
            !wcscmp(label,L"PrevMove")||!wcscmp(label,L"bForce")||
            !wcscmp(label,L"Stance")||!wcscmp(label,L"BS")||!wcscmp(label,L"NewBodyStance")||
            !wcscmp(label,L"SeqNode")||!wcscmp(label,L"PlayedTime")||!wcscmp(label,L"ExcessTime")||
            !wcscmp(label,L"PreviousStateName")||!wcscmp(label,L"NextStateName")||!wcscmp(label,L"AnimName")||
            !wcscmp(label,L"bForcedByDamage")||!wcscmp(label,L"bForceStopFire")||!wcscmp(label,L"BlendOutTime");
        const auto offset=get<unsigned>(property+0x94),size=get<unsigned>(property+0x74);
        if(!wanted||get<unsigned>(property+0x70)!=1||offset>bytes||size>bytes-offset)continue;
        const auto address=reinterpret_cast<uptr>(params)+offset;
        if(!wcscmp(kind,L"ByteProperty")&&size==1){unsigned char value{};
            if(copy(&value,address,1))fwprintf(log_file,L" param_%ls=%u",label,unsigned(value));
        }else if(!wcscmp(kind,L"BoolProperty")&&size==4){unsigned value{};
            if(copy(&value,address,4))fwprintf(log_file,L" param_%ls=%u",label,unsigned((value&get<unsigned>(property+0xd0))!=0));
        }else if(!wcscmp(kind,L"FloatProperty")&&size==4){float value{};
            if(copy(&value,address,4)&&std::isfinite(value))fwprintf(log_file,L" param_%ls=%.9g",label,value);
        }else if(!wcscmp(kind,L"ObjectProperty")&&size==8){uptr value{};
            if(copy(&value,address,8))fwprintf(log_file,L" param_%ls=%p",label,reinterpret_cast<void*>(value));
        }else if(!wcscmp(kind,L"NameProperty")&&size==8){Name value{};wchar_t label_value[96]{};
            if(copy(&value,address,8)&&text(value,label_value,96))fwprintf(log_file,L" param_%ls=%ls:%u:%u",label,label_value,value.index,value.number);
        }else if(!wcscmp(kind,L"StructProperty")&&!wcscmp(label,L"Stance")){
            const auto structure=get<uptr>(property+0xd0);unsigned names_offset{};
            if(!registered(structure)||!playground::named(structure,L"BodyStance")||size>256||
               !structure_field(structure,L"AnimName",L"ArrayProperty",sizeof(Array),size,names_offset))continue;
            const auto names_property=playground::field(structure,L"AnimName",L"ArrayProperty");
            const auto inner=get<uptr>(names_property+0xd0);Array names{};
            if(!registered(inner)||!playground::named(get<uptr>(inner+0x50),L"NameProperty")||
               get<unsigned>(inner+0x70)!=1||get<unsigned>(inner+0x74)!=8||
               !copy(&names,address+names_offset,sizeof(names))||names.count<0||names.capacity<names.count||
               names.capacity>int(max_slots)||(names.count&&!names.data))continue;
            fwprintf(log_file,L" param_Stance_slots=%d param_Stance_names=",names.count);
            for(int i=0;i<names.count;++i){Name value{};wchar_t animation[96]{};
                const bool known=copy(&value,names.data+size_t(i)*8,8)&&text(value,animation,96);
                fwprintf(log_file,L"%ls%d:%u:%u:%u:%ls",i?L",":L"",i,unsigned(known),value.index,value.number,
                    known?animation:L"unknown");
            }
        }
    }
}
void emit_event(Token const& token,uptr object,uptr function,void* params,const wchar_t* phase,uptr caller=0){
    wchar_t label[96]{},outer[96]{};name(function,label,96);name(get<uptr>(function+0x40),outer,96);
    fwprintf(log_file,L"hand_animation_event seq=%u call=%u parent=%u depth=%u kind=%u phase=%ls source=%ls "
        L"function=%ls outer=%ls object=%p function_ptr=%p native_index=%u ms=%llu "
        L"pickup_request=%u melee_request=%u pickup_episode=%u melee_episode=%u",
        ++probe.event_lines,token.call,token.parent,token.depth,unsigned(token.kind),phase,
        token.source==Source::hook?L"hook":token.source==Source::bridge_invoke?L"bridge_invoke":L"call_function",label,outer,reinterpret_cast<void*>(object),
        reinterpret_cast<void*>(function),unsigned(get<unsigned short>(function+0xf8)),GetTickCount64(),
        token.pickup_request,token.melee_request,probe.ledger.pickup_episode(),probe.ledger.melee_episode());
    if(token.source==Source::call_function){
        CallerFrame frame{};const bool known=caller_frame(caller,frame);
        fwprintf(log_file,L" caller_frame=%p caller_known=%u caller_node=%p caller_object=%p caller_code=%p "
            L"caller_locals=%p caller_previous=%p caller_script=%p caller_script_size=%d param_transport=caller_frame_unconsumed",
            reinterpret_cast<void*>(caller),unsigned(known),reinterpret_cast<void*>(frame.node),
            reinterpret_cast<void*>(frame.object),reinterpret_cast<void*>(frame.code),
            reinterpret_cast<void*>(frame.locals),reinterpret_cast<void*>(frame.previous),
            reinterpret_cast<void*>(frame.script),frame.script_size);
    }else parameters(function,params);
    fwprintf(log_file,L" ownership_proved=0 writes=0\n");
    Snapshot state{};
    if(snapshot(state))emit_snapshot(state,phase,token.call);
    else fwprintf(log_file,L"hand_animation_snapshot_gap call=%u phase=%ls reason=identity_or_environment ownership_proved=0 writes=0\n",token.call,phase);
    fflush(log_file);
}
Token enter(uptr object,uptr function,void* params,Source source,uptr caller=0){
    // The atomics/thread gate precede every game-thread-only Probe access.
    if(!enabled.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||
       !active_visible.load(std::memory_order_acquire)||!probe.active||!log_file||
       !registered(object)||!registered(function)||
       !playground::named(get<uptr>(function+0x50),L"Function")||
       GetTickCount64()>probe.deadline)return {};
    if(source==Source::hook)++probe.hook_callbacks;
    else if(source==Source::call_function)++probe.call_callbacks;
    Kind kind{};
    if(!test_environment()||!playground::automatic||!playground::ready||
       playground::arena_pc!=probe.pc||playground::arena_pawn!=probe.pawn||
       playground::arena_world!=probe.world||!related(object)||!classify(function,kind))return {};
    if((kind==Kind::melee_state_begin||kind==Kind::melee_state_end)&&
       !playground::named(get<uptr>(function+0x40),L"MeleeAttacking"))return {};
    if(source==Source::call_function){CallerFrame frame{};
        if(!caller_frame(caller,frame)){++probe.call_frame_rejected;return {};}}
    const auto token=probe.ledger.enter(kind,source);
    if(token){
        if(source==Source::call_function)++probe.call_accepted;
        emit_event(token,object,function,params,L"pre",caller);
    }
    return token;
}
void leave(Token const& token,uptr object,uptr function,void* params,uptr caller=0){
    if(!token)return;
    emit_event(token,object,function,params,L"post",caller);
    if(probe.ledger.leave(token)&&token.source==Source::call_function)++probe.call_completed;
}
struct HookScope {
    uptr object{},function{};void* params{};Token token{};
    HookScope(uptr o,uptr f,void* p):object(o),function(f),params(p),token(enter(o,f,p,Source::hook)){}
    ~HookScope(){leave(token,object,function,params);}
};
std::array<Token,gears_hand_animation::max_depth> bridge_stack{};
unsigned bridge_depth{};
unsigned bridge_before(Request* request){
    if(!enabled.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||
       !active_visible.load(std::memory_order_acquire)||
       !request||bridge_depth>=bridge_stack.size()||request->size>sizeof(request->params)||
       !registered(request->object)||!registered(request->function)||
       get<unsigned short>(request->function+0x106)>request->size)return 0;
    const auto token=enter(request->object,request->function,request->params,Source::bridge_invoke);
    if(!token)return 0;
    bridge_stack[bridge_depth++]=token;return token.call;
}
void bridge_after(Request* request,unsigned call){
    if(!call||!request||!bridge_depth)return;
    const auto token=bridge_stack[--bridge_depth];
    if(token.call!=call){probe.function_limit=true;return;}
    leave(token,request->object,request->function,request->params);
}
void reset(){
    requested=false;stop_requested=false;active_visible=false;status=0;frames=0;
    probe={};bridge_stack={};bridge_depth=0;
}
void post_tick(uptr pc){
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    // Flush frozen POD even when the original animation probe is now inactive.
    gripdamage::pump();
#endif
    if(stop_requested.exchange(false,std::memory_order_acq_rel)){
        requested=false;if(probe.active)finish(4,L"explicit_stop");
        else {active_visible.store(false,std::memory_order_release);status.store(4,std::memory_order_release);}
    }
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    // Preserve a queued Start until the previous recorded callbacks drain.
    if(requested.load(std::memory_order_acquire)&&!gripdamage::can_begin())return;
#endif
    if(requested.exchange(false,std::memory_order_acq_rel)){
        probe={};probe.fields.reserve(max_fields);probe.functions.reserve(max_functions);frames=0;
        probe.pc=pc;probe.pawn=playground::arena_pawn;probe.world=playground::arena_world;
        probe.mesh=playground::pointer(probe.pawn,L"Mesh");probe.tree=playground::pointer(probe.mesh,L"Animations");
        probe.active=true;probe.deadline=GetTickCount64()+gears_hand_animation::duration_ms;
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
        if(!gripdamage::begin(probe.world,probe.pc,probe.pawn,probe.deadline)){
            finish(5,L"damage_observer_pending");return;
        }
#endif
        active_visible.store(true,std::memory_order_release);
        if(log_file){fwprintf(log_file,L"hand_animation_probe_begin schema=1 pc=%p pawn=%p world=%p mesh=%p tree=%p "
            L"duration_ms=%llu frame_limit=%u call_limit=%u depth_limit=%u slot_limit=%u child_limit=%u "
            L"sample_interval_ms=%llu ownership_proved=0 writes=0 original_hook_auto_filter=player_tick_only "
            L"hook_observer=before_auto_filter bridge_invoke_bypasses_hook=1\n",
            reinterpret_cast<void*>(probe.pc),reinterpret_cast<void*>(probe.pawn),reinterpret_cast<void*>(probe.world),
            reinterpret_cast<void*>(probe.mesh),reinterpret_cast<void*>(probe.tree),gears_hand_animation::duration_ms,
            gears_hand_animation::max_frames,gears_hand_animation::max_calls,gears_hand_animation::max_depth,
            max_slots,max_children,gears_hand_animation::sample_interval_ms);fflush(log_file);}
    }
    if(!probe.active)return;
    if(!log_file||probe.pc!=pc){finish(5,L"player_or_log");return;}
    if(probe.ledger.overflow()||probe.field_limit||probe.function_limit){finish(5,L"observer_capacity");return;}
    if(GetTickCount64()>probe.deadline){finish(3,L"duration_limit");return;}
    if(probe.ledger.calls()>=gears_hand_animation::max_calls){finish(3,L"call_limit");return;}
    if(GetTickCount64()<probe.next_sample)return;
    Snapshot state{};
    if(!snapshot(state)){finish(5,L"identity_or_environment");return;}
    emit_snapshot(state,L"periodic",0);fflush(log_file);
    probe.next_sample=GetTickCount64()+gears_hand_animation::sample_interval_ms;
    const auto count=frames.fetch_add(1,std::memory_order_acq_rel)+1;
    status.store(2,std::memory_order_release);
    if(count>=gears_hand_animation::max_frames)finish(3,L"frame_limit");
}
} // namespace handanim
