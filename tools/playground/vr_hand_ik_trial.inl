// Single-use private right-hand pulse. No XR apply mask, animation graph,
// visibility flag, rotation control, bone array or window is changed here.
namespace handik {
using Triple=gears_hand_ik::Triple;
constexpr unsigned right_ik=2;
std::atomic<bool> requested{},stop_requested{},used{},safe_to_unload{true},shutdown_requested{};
std::atomic<unsigned> status{},samples{};
struct Control {
    uptr object{};
    Triple strength{},effector{};
    playground::Rotation rotation{};
    std::vector<unsigned char> fixed;
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    handtail::Observed translation;
#endif
};
struct Snapshot {
    uptr pc{},pawn{},mesh{},tree{},weapon{};
    std::vector<unsigned char> identity;
    std::array<Control,8> controls{};
    std::array<float,16> mesh_world{},left{},right{};
    std::uint64_t ms{};
    float world_time{},last_render_time{};
    int mesh_tick{};
    bool control_tick_tag{},mesh_tick_known{},control_tag_known{},action_idle{};
};
struct Trial {
    unsigned phase{}; // 0 idle, 1 stability, 2 pulse, 3 restored observation.
    Snapshot previous{},original{};
    gears_hand_ik::StabilityGate stability;
    gears_hand_ik::OwnedWrite owned;
    std::uint64_t wait_deadline{},wrote_ms{},restored_ms{};
    unsigned setter_calls{},effector_writes{};
    bool target_written{},strength_written{},restored{},render_evidence{},effect_evidence{};
};
Trial trial{};
struct Rejection {
    const wchar_t* group{L"unknown"};const wchar_t* field{L"unknown"};
    const wchar_t* expected_kind{L"none"};
    uptr object{},related{},function{};
    unsigned expected_bytes{};
    int control{-1},expected{},observed{},extra{};
    bool read_ok{};
    unsigned long long raw{};
};
Rejection rejection{};
bool rejection_logged{};
void mark(const wchar_t* group,uptr object,const wchar_t* field,
          const wchar_t* kind=L"none",unsigned bytes=0){
    const auto control=rejection.control;
    rejection={};rejection.control=control;rejection.group=group;
    rejection.object=object;rejection.field=field;
    rejection.expected_kind=kind;rejection.expected_bytes=bytes;
}
bool checked_field(uptr object,const wchar_t* field,const wchar_t* kind,void* out,unsigned bytes){
    mark(L"property",object,field,kind,bytes);
    rejection.read_ok=freehands::read_field(object,field,kind,out,bytes);
    if(rejection.read_ok)std::memcpy(&rejection.raw,out,(std::min)(size_t(bytes),sizeof(rejection.raw)));
    return rejection.read_ok;
}
bool checked_bool(uptr object,const wchar_t* field,bool& out){
    mark(L"bool_property",object,field,L"BoolProperty",4);
    rejection.read_ok=freehands::read_bool(object,field,out);
    if(rejection.read_ok)rejection.raw=out?1:0;
    return rejection.read_ok;
}
template<class T>void append(std::vector<unsigned char>& to,T const& value){
    const auto bytes=reinterpret_cast<unsigned char const*>(&value);
    to.insert(to.end(),bytes,bytes+sizeof(value));
}
bool name_is(Name value,const wchar_t* wanted){
    const auto reject_name=[&](){const auto object=rejection.object;
        mark(L"name_value",object,wanted,L"NameProperty",8);
        std::memcpy(&rejection.raw,&value,sizeof(value));return false;};
    if(value.number)return reject_name();
    const auto names=get<Array>(base+0x1c34800);
    const auto chars=wcslen(wanted)+1;
    if(chars>96||!names.data||names.count<=0||names.count>names.capacity||
       names.capacity>4000000||value.index>=unsigned(names.count))return reject_name();
    const auto entry=get<uptr>(names.data+size_t(value.index)*8);
    wchar_t text[96]{};
    if(!entry||get<unsigned>(entry)!=value.index||!copy(text,entry+20,chars*sizeof(wchar_t))||
       std::memcmp(text,wanted,chars*sizeof(wchar_t)))return reject_name();
    return true;
}
bool pointer(uptr object,const wchar_t* field,uptr& result,bool component=false){
    const bool ok=checked_field(object,field,L"ObjectProperty",&result,sizeof(result))||
        (component&&checked_field(object,field,L"ComponentProperty",&result,sizeof(result)));
    if(ok)rejection.related=result;
    return ok;
}
bool structure(uptr object,const wchar_t* field,const wchar_t* type,void* value,unsigned bytes){
    mark(L"struct_type",object,field,type,bytes);
    const auto property=playground::property(object,field);
    const auto inner=property?get<uptr>(property+0xd0):0;
    rejection.related=inner;
    return registered(inner)&&playground::named(inner,type)&&
        checked_field(object,field,L"StructProperty",value,bytes);
}
bool identity(uptr object,std::vector<unsigned char>& out){
    mark(L"registered_identity",object,L"UObject");
    if(!registered(object)||playground::deleted(object))return false;
    append(out,object);append(out,get<unsigned>(object+8));
    append(out,get<uptr>(object+0x40));append(out,get<Name>(object+0x48));
    append(out,get<uptr>(object+0x50));return true;
}
unsigned parameter_count(uptr function){
    unsigned count=0,visited=0;
    for(auto field=get<uptr>(function+0x80);field;field=get<uptr>(field+0x68)){
        if(!registered(field)||++visited>32)return 100;
        if(get<unsigned long long>(field+0x78)&0x80)++count;
    }
    return count;
}
unsigned parameter_count(playground::Args const& call){return parameter_count(call.r.function);}
void log_rejection(){
    if(rejection_logged||!log_file)return;
    rejection_logged=true;
    wchar_t object_name[96]{},object_class[96]{},property_kind[96]{},related_name[96]{},actual_name[96]{};
    name(rejection.object,object_name,96);name(get<uptr>(rejection.object+0x50),object_class,96);
    const auto property=rejection.function?playground::field(rejection.function,L"ReturnValue"):
        playground::property(rejection.object,rejection.field);
    name(get<uptr>(property+0x50),property_kind,96);name(rejection.related,related_name,96);
    if(!wcscmp(rejection.group,L"name_value")){
        Name value{};std::memcpy(&value,&rejection.raw,sizeof(value));
        const auto names=get<Array>(base+0x1c34800);
        if(names.data&&names.count>0&&names.count<=names.capacity&&names.capacity<=4000000&&
           value.index<unsigned(names.count)){
            const auto entry=get<uptr>(names.data+size_t(value.index)*8);
            for(unsigned i=0;i<95;++i)if(!copy(&actual_name[i],entry+20+size_t(i)*2,2)||!actual_name[i])break;
        }
    }
    fwprintf(log_file,L"hand_ik_trial_reject group=%ls field=%ls control=%d object=%p "
        L"object_name=%ls object_class=%ls property=%p property_kind=%ls property_bytes=%u "
        L"property_offset=%u expected_kind=%ls expected_bytes=%u read_ok=%u raw0=%llx "
        L"related=%p related_name=%ls actual_name=%ls expected=%d observed=%d extra=%d\n",
        rejection.group,rejection.field,rejection.control,reinterpret_cast<void*>(rejection.object),
        object_name[0]?object_name:L"unreadable",object_class[0]?object_class:L"unreadable",
        reinterpret_cast<void*>(property),property_kind[0]?property_kind:L"missing",
        property?get<unsigned>(property+0x74):0,property?get<unsigned>(property+0x94):0,
        rejection.expected_kind,rejection.expected_bytes,unsigned(rejection.read_ok),rejection.raw,
        reinterpret_cast<void*>(rejection.related),related_name[0]?related_name:L"none",actual_name[0]?actual_name:L"none",
        rejection.expected,rejection.observed,rejection.extra);
    const auto function=rejection.function;
    if(registered(function)){
        fwprintf(log_file,L"hand_ik_trial_reject_function function=%p parameter_bytes=%u "
            L"return_offset=%u parameter_count=%u native_index=%u\n",
            reinterpret_cast<void*>(function),unsigned(get<unsigned short>(function+0x106)),
            unsigned(get<unsigned short>(function+0x108)),parameter_count(function),
            unsigned(get<unsigned short>(function+0xf8)));
        unsigned emitted=0,visited=0;
        for(auto field=get<uptr>(function+0x80);field&&emitted<4;field=get<uptr>(field+0x68)){
            if(!registered(field)||++visited>32)break;
            const auto flags=get<unsigned long long>(field+0x78);
            if(!(flags&0x80))continue;
            wchar_t field_name[96]{},kind[96]{};name(field,field_name,96);name(get<uptr>(field+0x50),kind,96);
            fwprintf(log_file,L"hand_ik_trial_reject_parameter name=%ls type=%ls offset=%u bytes=%u flags=%llx\n",
                field_name[0]?field_name:L"unreadable",kind[0]?kind:L"unreadable",
                get<unsigned>(field+0x94),get<unsigned>(field+0x74),flags);++emitted;
        }
    }
    fflush(log_file);
}
bool bool_call(uptr object,const wchar_t* function,bool& result){
    mark(L"getter_signature",object,function,L"BoolProperty",4);
    playground::Args call(object,function);
    rejection.function=call.r.function;
    if(call.r.size!=4||get<unsigned short>(call.r.function+0x108)!=0||
       parameter_count(call)!=1||!freehands::argument(call,L"ReturnValue",L"BoolProperty",4))return false;
    const auto property=call.prop(L"ReturnValue");
    const auto mask=get<unsigned>(property+0xd0);
    if(!mask||(mask&(mask-1))||get<unsigned>(property+0x94)!=0)return false;
    rejection.group=L"getter_invoke";
    if(!call.run())return false;
    result=(call.output<unsigned>()&mask)!=0;return true;
}
bool weapon_active(uptr weapon){
    mark(L"getter_state",weapon,L"GetStateName",L"NameProperty",8);
    playground::Args call(weapon,L"GetStateName");
    rejection.function=call.r.function;
    return call.r.size==8&&get<unsigned short>(call.r.function+0x108)==0&&
        parameter_count(call)==1&&freehands::argument(call,L"ReturnValue",L"NameProperty",8)&&
        get<unsigned>(call.prop(L"ReturnValue")+0x94)==0&&call.run()&&
        name_is(call.output<Name>(),L"Active");
}
bool read_control(uptr pawn,uptr mesh,uptr tree,unsigned index,Control& out){
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    struct TailRead {
        uptr pawn,mesh,tree;unsigned index;bool complete{};
        ~TailRead(){if(!complete)handstream::tail_observation_failed(pawn,mesh,tree,index);}
    } tail_read_guard{pawn,mesh,tree,index};
#endif
    rejection.control=int(index);
    mark(L"control_cache",pawn,L"cached_hand_control");
    constexpr const wchar_t* caches[]={L"IKBoneCtrl_RightHand",L"IKBoneRotCtrl_RightHand",
        L"IKCtrl_RightHand",L"IKRotCtrl_RightHand",L"IKBoneCtrl_LeftHand",
        L"IKBoneRotCtrl_LeftHand",L"IKCtrl_LeftHand",L"IKRotCtrl_LeftHand"};
    constexpr const wchar_t* names[]={L"IKBone_RightHand",L"IKBoneRot_RightHand",
        L"IK_RightHand",L"IKRot_RightHand",L"IKBone_LeftHand",L"IKBoneRot_LeftHand",
        L"IK_LeftHand",L"IKRot_LeftHand"};
    const bool rotation=(index&1)!=0,tail=(index%4)>=2,left=index>=4;
    if(!pointer(pawn,caches[index],out.object))return false;
    mark(L"control_class_or_owner",out.object,caches[index],rotation?L"SkelControlSingleBone":L"SkelControlLimb");
    rejection.related=tree;rejection.raw=get<uptr>(out.object+0x40);
    if(!playground::instance(out.object,rotation?L"SkelControlSingleBone":L"SkelControlLimb")||
       !playground::named(get<uptr>(out.object+0x50),rotation?L"SkelControlSingleBone":L"SkelControlLimb")||
       get<uptr>(out.object+0x40)!=tree||!identity(out.object,out.fixed))return false;
    uptr component{},next{};Name control_name{};
    if(!pointer(out.object,L"SkelComponent",component,true)||component!=mesh||
       !pointer(out.object,L"NextControl",next)||
       !checked_field(out.object,L"ControlName",L"NameProperty",&control_name,sizeof(control_name))||
       !name_is(control_name,names[index]))return false;
    append(out.fixed,component);append(out.fixed,next);append(out.fixed,control_name);
    constexpr const wchar_t* strengths[]={L"ControlStrength",L"StrengthTarget",L"BlendTimeToGo"};
    for(unsigned i=0;i<3;++i)if(!checked_field(out.object,strengths[i],L"FloatProperty",
        &out.strength[i],sizeof(float))||!std::isfinite(out.strength[i]))return false;
    for(const auto field:{L"BlendInTime",L"BlendOutTime"}){
        float value{};
        if(!checked_field(out.object,field,L"FloatProperty",&value,sizeof(value))||
           !std::isfinite(value)||value<0||value>10)return false;
        append(out.fixed,value);
    }
    for(const auto field:{L"bSetStrengthFromAnimNode",L"bPropagateSetActive",L"bIgnoreWhenNotRendered"}){
        bool value{};
        if(!checked_bool(out.object,field,value)||value)return false;
        append(out.fixed,value);
    }
    std::uint8_t space{};Name bone{};
    if(rotation){
        bool add{},apply{},translate{};
        if(!structure(out.object,L"BoneRotation",L"Rotator",&out.rotation,sizeof(out.rotation))||
           !checked_field(out.object,L"BoneRotationSpace",L"ByteProperty",&space,1)||
           space!=(tail?0:5)||!checked_field(out.object,L"RotationSpaceBoneName",
               L"NameProperty",&bone,sizeof(bone))||
           !name_is(bone,tail?L"None":left?L"b_MF_IK_Hand_L":L"b_MF_IK_Hand_R")||
           !checked_bool(out.object,L"bAddRotation",add)||add||
           !checked_bool(out.object,L"bApplyRotation",apply)||!apply||
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
           !checked_bool(out.object,L"bApplyTranslation",translate))return false;
        if(!handtail::observe(out.object,out.translation)||
           (tail&&!handstream::tail_value_allowed(index,out.object,out.translation)))return false;
        // Keep real values separately. Only the same owned whole tuple can
        // normalize this one formerly fixed flag while the lease is active.
        append(out.fixed,out.translation.schema.size());
        out.fixed.insert(out.fixed.end(),out.translation.schema.begin(),out.translation.schema.end());
        if(tail)translate=false;
#else
           !checked_bool(out.object,L"bApplyTranslation",translate)||(tail&&translate))return false;
#endif
        append(out.fixed,space);append(out.fixed,bone);
        append(out.fixed,add);append(out.fixed,apply);append(out.fixed,translate);
    }else{
        Triple joint{};std::uint8_t joint_space{};Name joint_bone{};bool take{},maintain{};
        if(!structure(out.object,L"EffectorLocation",L"Vector",out.effector.data(),sizeof(out.effector))||
           !gears_hand_ik::finite(out.effector)||
           !checked_field(out.object,L"EffectorLocationSpace",L"ByteProperty",&space,1)||
           space!=(tail?0:5)||!checked_field(out.object,L"EffectorSpaceBoneName",
               L"NameProperty",&bone,sizeof(bone))||
           !name_is(bone,tail?L"None":left?L"b_MF_IK_Hand_L":L"b_MF_IK_Hand_R")||
           !structure(out.object,L"JointTargetLocation",L"Vector",joint.data(),sizeof(joint))||
           !gears_hand_ik::finite(joint)||
           !checked_field(out.object,L"JointTargetLocationSpace",L"ByteProperty",&joint_space,1)||
           joint_space!=5||!checked_field(out.object,L"JointTargetSpaceBoneName",
               L"NameProperty",&joint_bone,sizeof(joint_bone))||
           !name_is(joint_bone,left?L"b_MF_Forearm_L":L"b_MF_Forearm_R")||
           !checked_bool(out.object,L"bTakeRotationFromEffectorSpace",take)||take||
           !checked_bool(out.object,L"bMaintainEffectorRelRot",maintain)||maintain)return false;
        append(out.fixed,space);append(out.fixed,bone);append(out.fixed,joint);
        append(out.fixed,joint_space);append(out.fixed,joint_bone);append(out.fixed,take);append(out.fixed,maintain);
    }
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    if((index==3||index==7)&&!handstream::tail_layout_allowed(index,out.object,out.fixed))return false;
    tail_read_guard.complete=true;
#endif
    return true;
}
bool read_lists(Snapshot& out){
    rejection.control=-1;
    Array lists{},indices{};
    const auto property=playground::property(out.tree,L"SkelControlLists");
    const auto index_property=playground::property(out.mesh,L"SkelControlIndex");
    if(!checked_field(out.tree,L"SkelControlLists",L"ArrayProperty",&lists,sizeof(lists))||
       !checked_field(out.mesh,L"SkelControlIndex",L"ArrayProperty",&indices,sizeof(indices)))return false;
    mark(L"control_list_bounds",out.tree,L"SkelControlLists");
    rejection.expected=6;rejection.observed=lists.count;rejection.extra=lists.capacity;rejection.related=lists.data;
    if(!lists.data||lists.count!=6||lists.capacity<6||lists.capacity>64)return false;
    mark(L"control_index_bounds",out.mesh,L"SkelControlIndex");
    rejection.expected=115;rejection.observed=indices.count;rejection.extra=indices.capacity;rejection.related=indices.data;
    if(!indices.data||indices.count!=115||indices.capacity<115||indices.capacity>512)return false;
    mark(L"control_list_inner_type",out.tree,L"SkelControlLists");
    const auto inner=get<uptr>(property+0xd0),index_inner=get<uptr>(index_property+0xd0);
    if(!registered(inner)||!playground::named(get<uptr>(inner+0x50),L"StructProperty")||
       !registered(index_inner)||!playground::named(get<uptr>(index_inner+0x50),L"ByteProperty")||
       get<unsigned>(index_inner+0x74)!=1)return false;
    mark(L"control_list_element",inner,L"StructProperty");
    const auto element=get<uptr>(inner+0xd0);rejection.related=element;
    if(!registered(element))return false;
    const auto stride=get<unsigned>(element+0x88);
    const auto bone_property=playground::field(element,L"BoneName",L"NameProperty");
    const auto head_property=playground::field(element,L"ControlHead",L"ObjectProperty");
    mark(L"control_list_layout",element,L"BoneName_ControlHead");rejection.observed=int(stride);
    if(stride<16||stride>64||!bone_property||!head_property||
       get<unsigned>(bone_property+0x74)!=8||get<unsigned>(head_property+0x74)!=8)return false;
    const auto bone_offset=get<unsigned>(bone_property+0x94),head_offset=get<unsigned>(head_property+0x94);
    if(bone_offset>stride-8||head_offset>stride-8)return false;
    std::array<unsigned char,115> index_values{};
    mark(L"control_index_values",out.mesh,L"SkelControlIndex");rejection.related=indices.data;
    if(!copy(index_values.data(),indices.data,index_values.size())||index_values[11]!=0||index_values[35]!=2)return false;
    append(out.identity,index_values);append(out.identity,lists.data);append(out.identity,indices.data);
    append(out.identity,lists.capacity);append(out.identity,indices.capacity);
    for(int i=0;i<lists.count;++i){
        mark(L"control_list_entry",out.tree,L"SkelControlLists");rejection.observed=i;
        Name bone{};uptr head{};
        if(!copy(&bone,lists.data+size_t(i)*stride+bone_offset,sizeof(bone))||
           !copy(&head,lists.data+size_t(i)*stride+head_offset,sizeof(head)))return false;
        append(out.identity,bone);append(out.identity,head);
        if(i==0&&(!name_is(bone,L"b_MF_Hand_L")||head!=out.controls[4].object))return false;
        if(i==2&&(!name_is(bone,L"b_MF_Hand_R")||head!=out.controls[0].object))return false;
    }
    for(unsigned i=0;i<8;++i){
        rejection.control=int(i);mark(L"control_chain",out.controls[i].object,L"NextControl");
        for(unsigned n=0;n<i;++n)if(out.controls[n].object==out.controls[i].object)return false;
        uptr next{};
        if(!pointer(out.controls[i].object,L"NextControl",next)||
           next!=((i%4)==3?0:out.controls[i+1].object))return false;
    }
    return true;
}
bool read_snapshot(uptr pc,Snapshot& out){
    out={};gears_weapon::FreeHandGameState game{};unsigned known{};std::uint8_t special{};
    rejection={};mark(L"player_state",pc,L"owned_arena_game_state");
    if(!freehands::game_state(pc,game,known,special))return false;
    mark(L"player_gate",game.pawn,L"alive_known_flags_current_weapon");
    rejection.expected=15;rejection.observed=int(known);rejection.extra=int(game.alive);rejection.related=game.weapon;
    if(!game.alive||known!=15||!game.weapon)return false;
    out.pc=pc;out.pawn=game.pawn;out.mesh=game.mesh;out.weapon=game.weapon;
    uptr asset{};int ignore{};bool ignore_hidden{},mirrored{},wants_melee{},transition{},can_switch{},reloading{};
    if(!pointer(out.mesh,L"Animations",out.tree))return false;
    mark(L"tree_class_or_owner",out.tree,L"Animations",L"AnimTree");
    rejection.related=out.mesh;rejection.raw=get<uptr>(out.tree+0x40);
    if(!playground::instance(out.tree,L"AnimTree")||
       !playground::named(get<uptr>(out.tree+0x50),L"AnimTree")||get<uptr>(out.tree+0x40)!=out.mesh||
       !pointer(out.mesh,L"SkeletalMesh",asset)||!registered(asset)||
       !playground::named(asset,L"COG_Marcus_Fenix_CamSkel")||
       !checked_field(out.mesh,L"bIgnoreControllers",L"IntProperty",&ignore,sizeof(ignore))||ignore||
       !checked_bool(out.mesh,L"bIgnoreControllersWhenNotRendered",ignore_hidden)||!ignore_hidden||
       !checked_bool(out.pawn,L"bIsMirrored",mirrored)||
       !checked_bool(out.pawn,L"bWantsToMelee",wants_melee)||
       !bool_call(out.pawn,L"DoingAnimationTransition",transition)||
       !bool_call(pc,L"CanSwitchWeapons",can_switch)||!bool_call(out.weapon,L"IsReloading",reloading))return false;
    out.action_idle=!game.fixed_animation_mask&&!mirrored&&!wants_melee&&!transition&&
        can_switch&&!reloading&&weapon_active(out.weapon);
    for(const auto object:{uptr(game.world),pc,out.pawn,out.mesh,out.tree,out.weapon,asset})
        if(!identity(object,out.identity))return false;
    int left{},right{};
    rejection.control=-1;mark(L"bone_index",out.mesh,L"b_MF_Hand_L");rejection.expected=11;
    rejection.function=playground::field(get<uptr>(out.mesh+0x50),L"MatchRefBone",L"Function");
    const bool left_known=freehands::bone_index(out.mesh,L"b_MF_Hand_L",left);rejection.observed=left;
    if(!left_known||left!=11)return false;
    mark(L"bone_index",out.mesh,L"b_MF_Hand_R");rejection.expected=35;
    rejection.function=playground::field(get<uptr>(out.mesh+0x50),L"MatchRefBone",L"Function");
    const bool right_known=freehands::bone_index(out.mesh,L"b_MF_Hand_R",right);rejection.observed=right;
    if(!right_known||right!=35)return false;
    append(out.identity,left);append(out.identity,right);append(out.identity,ignore_hidden);
    for(unsigned i=0;i<8;++i)if(!read_control(out.pawn,out.mesh,out.tree,i,out.controls[i]))return false;
    if(!read_lists(out)||!checked_field(uptr(game.world),L"TimeSeconds",L"FloatProperty",
          &out.world_time,sizeof(float))||!std::isfinite(out.world_time)||
       !checked_field(out.mesh,L"LastRenderTime",L"FloatProperty",&out.last_render_time,sizeof(float))||
       !std::isfinite(out.last_render_time)||
       !structure(out.mesh,L"LocalToWorld",L"Matrix",out.mesh_world.data(),sizeof(out.mesh_world)))return false;
    rejection.control=-1;mark(L"bone_matrix",out.mesh,L"left_GetBoneMatrix");rejection.observed=left;
    rejection.function=playground::field(get<uptr>(out.mesh+0x50),L"GetBoneMatrix",L"Function");
    if(!freehands::bone_matrix(out.mesh,left,out.left))return false;
    mark(L"bone_matrix",out.mesh,L"right_GetBoneMatrix");rejection.observed=right;
    rejection.function=playground::field(get<uptr>(out.mesh+0x50),L"GetBoneMatrix",L"Function");
    if(!freehands::bone_matrix(out.mesh,right,out.right))return false;
    mark(L"finite_matrix",out.mesh,L"LocalToWorld");
    for(float value:out.mesh_world)if(!std::isfinite(value))return false;
    out.mesh_tick_known=checked_field(out.mesh,L"TickTag",L"IntProperty",&out.mesh_tick,sizeof(int));
    // The shipped Engine.SkelControlBase.ControlTickTag is a BoolProperty.
    // Its value is telemetry, not a per-frame counter or effect proof.
    out.control_tag_known=checked_bool(out.controls[right_ik].object,L"ControlTickTag",out.control_tick_tag);
    out.ms=GetTickCount64();return true;
}
bool snapshot(uptr pc,Snapshot& out){
    const bool ok=read_snapshot(pc,out);
    if(!ok)log_rejection();
    return ok;
}
bool same_controls(Snapshot const& a,Snapshot const& b,bool skip_right=false){
    if(a.identity!=b.identity)return false;
    for(unsigned i=0;i<8;++i){
        if(a.controls[i].fixed!=b.controls[i].fixed)return false;
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
        if(!gears_hand_tail_translation::exact(a.controls[i].translation.value,b.controls[i].translation.value))return false;
#endif
        if(skip_right&&i==right_ik)continue;
        if(!gears_hand_ik::exact(a.controls[i].strength,b.controls[i].strength)||
           !gears_hand_ik::exact(a.controls[i].effector,b.controls[i].effector)||
           std::memcmp(&a.controls[i].rotation,&b.controls[i].rotation,sizeof(playground::Rotation)))return false;
    }
    return true;
}
bool baseline_strengths(Snapshot const& value){
    for(unsigned i=0;i<8;++i){
        const auto expected=(i%4)<2?1.f:0.f;
        if(!gears_hand_ik::exact(value.controls[i].strength,Triple{{expected,expected,0}}))return false;
    }
    return true;
}
Triple position(std::array<float,16> const& matrix){return {{matrix[12],matrix[13],matrix[14]}};}
void log_sample(const wchar_t* phase,Snapshot const& value){
    if(!log_file)return;
    const auto& right=value.controls[right_ik];
    LARGE_INTEGER stamp{};QueryPerformanceCounter(&stamp);
    fwprintf(log_file,L"hand_ik_trial_sample phase=%ls sample=%u qpc=%lld ms=%llu world_time=%.9g "
        L"last_render_time=%.9g mesh_tick_known=%u control_tag_known=%u mesh_tick=%d control_tag=%u action_idle=%u "
        L"strength=%.9g target=%.9g blend=%.9g effector=%.9g,%.9g,%.9g stable_ticks=%u",
        phase,samples.load(),stamp.QuadPart,value.ms,value.world_time,value.last_render_time,
        unsigned(value.mesh_tick_known),unsigned(value.control_tag_known),value.mesh_tick,
        unsigned(value.control_tick_tag),unsigned(value.action_idle),
        right.strength[0],right.strength[1],right.strength[2],right.effector[0],right.effector[1],right.effector[2],
        trial.stability.ticks());
    freehands::print_matrix(L"mesh",value.mesh_world);freehands::print_matrix(L"left",value.left);
    freehands::print_matrix(L"right",value.right);
    fwprintf(log_file,L"\n");fflush(log_file);++samples;
}
void end(unsigned code,const wchar_t* reason,bool safe){
    trial.phase=0;safe_to_unload.store(safe,std::memory_order_release);
    status.store(code,std::memory_order_release);
    if(log_file){fwprintf(log_file,L"hand_ik_trial_end status=%u restored=%u setter_calls=%u "
        L"effector_writes=%u render_evidence=%u effect_evidence=%u inconclusive=%u safe_to_unload=%u reason=%ls\n",
        code,unsigned(trial.restored),trial.setter_calls,trial.effector_writes,
        unsigned(trial.render_evidence),unsigned(trial.effect_evidence),unsigned(code==5),unsigned(safe),reason);fflush(log_file);}
}
void reset(){
    trial={};requested=false;stop_requested=false;used=false;shutdown_requested=false;
    safe_to_unload=true;status=0;samples=0;
    rejection={};rejection_logged=false;
}
bool set_strength(uptr object,float value){
    playground::Args call(object,L"SetSkelControlStrength");
    const auto new_strength=call.prop(L"NewStrength"),blend=call.prop(L"InBlendTime");
    if(!call.valid||call.r.size!=8||get<unsigned short>(call.r.function+0x108)!=0xffff||
       parameter_count(call)!=2||!freehands::argument(call,L"NewStrength",L"FloatProperty",4)||
       !freehands::argument(call,L"InBlendTime",L"FloatProperty",4)||
       get<unsigned>(new_strength+0x94)!=0||get<unsigned>(blend+0x94)!=4||
       get<unsigned long long>(new_strength+0x78)!=0x80||get<unsigned long long>(blend+0x78)!=0x80)return false;
    ++trial.setter_calls;
    return call.set(L"NewStrength",value).set(L"InBlendTime",0.f).run();
}
bool write_effector(uptr object,Triple const& expected,Triple const& value){
    Triple current{};
    if(!structure(object,L"EffectorLocation",L"Vector",current.data(),sizeof(current))||
       !gears_hand_ik::exact(current,expected)||!gears_hand_ik::finite(value))return false;
    const auto property=playground::property(object,L"EffectorLocation");
    const auto address=object+get<unsigned>(property+0x94);
    __try{std::memcpy(reinterpret_cast<void*>(address),value.data(),sizeof(value));}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    ++trial.effector_writes;
    return structure(object,L"EffectorLocation",L"Vector",current.data(),sizeof(current))&&
        gears_hand_ik::exact(current,value);
}
void pre_tick(uptr pc){
    if(status.load()==8)return; // Never fight a writer after relinquishing ownership.
    if(stop_requested.load(std::memory_order_acquire)&&
       (trial.phase==1||(trial.phase==0&&requested.load(std::memory_order_acquire)))){
        requested=false;end(6,L"stopped_before_write",true);return;
    }
    if(trial.phase!=2)return;
    Snapshot current{};
    if(!snapshot(pc,current)||!same_controls(trial.original,current,true)||
       !trial.owned.can_restore(current.controls[right_ik].strength,current.controls[right_ik].effector)){
        end(8,L"ownership_lost_no_rollback",false);return;
    }
    log_sample(L"pre_restore",current);
    const bool in_time=gears_hand_ik::pulse_in_time(trial.wrote_ms,current.ms);
    const bool render=current.last_render_time>trial.original.last_render_time&&
        current.last_render_time>0&&current.world_time>=current.last_render_time&&
        current.world_time-current.last_render_time<=.1f&&
        std::memcmp(current.mesh_world.data(),trial.original.mesh_world.data(),sizeof(current.mesh_world))==0;
    trial.render_evidence=render;
    // This single pulse has no sham or animation-drift baseline. A changing
    // mesh tick, render time, or BoolProperty tag cannot attribute bone motion
    // to the IK setter, so this diagnostic trial must remain inconclusive.
    trial.effect_evidence=false;
    if(!set_strength(current.controls[right_ik].object,0.f)){
        end(8,L"zero_strength_call_failed",false);return;
    }
    Control zeroed{};
    if(!read_control(current.pawn,current.mesh,current.tree,right_ik,zeroed)||
       zeroed.fixed!=current.controls[right_ik].fixed||!gears_hand_ik::zero_strength(zeroed.strength)||
       !gears_hand_ik::exact(zeroed.effector,trial.owned.written_target)||
       !write_effector(zeroed.object,trial.owned.written_target,trial.owned.original_target)){
        end(8,L"zero_or_target_restore_failed",false);return;
    }
    Snapshot restored{};
    if(!snapshot(pc,restored)||!same_controls(trial.original,restored)){
        end(8,L"restore_verification_failed",false);return;
    }
    trial.restored=true;trial.restored_ms=restored.ms;log_sample(L"restored",restored);
    if(!in_time||!current.action_idle||!restored.action_idle){
        end(8,!in_time?L"pulse_deadline_exceeded":L"original_action_started",false);return;
    }
    trial.phase=3;status.store(3,std::memory_order_release);
}
void post_tick(uptr pc){
    if(status.load()==8)return;
    if(requested.exchange(false,std::memory_order_acq_rel)){
        trial={};trial.phase=1;trial.wait_deadline=GetTickCount64()+5000;samples=0;
        if(log_file){fwprintf(log_file,L"hand_ik_trial_begin strength=%.9g offset_units=%.9g "
            L"stable_ticks=8 stable_ms=250 pulse_limit_ms=400 recovery_ms=250 apply_mask=0\n",
            gears_hand_ik::trial_strength,gears_hand_ik::trial_offset_units);fflush(log_file);}
    }
    if(!trial.phase)return;
    if(stop_requested.load(std::memory_order_acquire)&&trial.phase==1){end(6,L"stopped_before_write",true);return;}
    Snapshot current{};
    if(!snapshot(pc,current)){end(trial.target_written?8:7,L"snapshot_or_gate_unknown",!trial.target_written);return;}
    if(trial.phase==3){
        if(!current.action_idle||!same_controls(trial.original,current)){
            end(8,L"recovery_ownership_changed",false);return;
        }
        log_sample(L"observe",current);
        if(current.ms>=trial.restored_ms&&current.ms-trial.restored_ms>=gears_hand_ik::recovery_ms){
            const bool effect=trial.effect_evidence&&gears_hand_ik::returned(position(trial.original.right),position(current.right));
            trial.effect_evidence=effect;end(effect?4:5,effect?L"directional_effect_and_return":L"no_natural_bone_effect",true);
        }
        return;
    }
    if(trial.phase!=1)return;
    if(current.ms>trial.wait_deadline){end(7,L"stability_timeout",true);return;}
    const bool qualified=current.action_idle&&baseline_strengths(current);
    const bool stable=trial.stability.observe(current.ms,qualified,same_controls(trial.previous,current));
    trial.previous=current;
    if(!stable)return;
    trial.original=current;trial.owned.original_target=current.controls[right_ik].effector;
    trial.owned.written_target=position(current.right);
    trial.owned.written_target[0]+=gears_hand_ik::trial_offset_units;
    log_sample(L"before",current);
    // Compare the exact same fully checked instance once more before the sole write.
    Snapshot immediately_before{};
    if(!snapshot(pc,immediately_before)||!immediately_before.action_idle||
       !same_controls(current,immediately_before)||
       std::memcmp(current.right.data(),immediately_before.right.data(),sizeof(current.right))){
        end(7,L"prewrite_state_changed",true);return;
    }
    if(!write_effector(current.controls[right_ik].object,trial.owned.original_target,trial.owned.written_target)){
        end(8,L"effector_write_unverified",false);return;
    }
    trial.target_written=true;trial.wrote_ms=GetTickCount64();
    // The owned record is installed before dispatch; even a partial failure
    // blocks shutdown. Never reassert the pulse on subsequent frames.
    trial.phase=2;status.store(2,std::memory_order_release);
    if(!set_strength(current.controls[right_ik].object,gears_hand_ik::trial_strength)){
        end(8,L"strength_call_failed",false);return;
    }
    trial.strength_written=true;
    Snapshot written{};
    if(!snapshot(pc,written)||!written.action_idle||!same_controls(trial.original,written,true)||
       !trial.owned.can_restore(written.controls[right_ik].strength,written.controls[right_ik].effector)){
        end(8,L"written_state_unexpected",false);return;
    }
    log_sample(L"written",written);
}
DWORD stop_and_wait(){
    stop_requested.store(true,std::memory_order_release);
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId())return 41;
    const auto unload_ready=[](){return gears_hand_ik::unload_allowed(
        safe_to_unload.load(std::memory_order_acquire),requested.load(std::memory_order_acquire),
        status.load(std::memory_order_acquire));};
    const auto until=GetTickCount64()+2000;
    while(!unload_ready()&&status.load(std::memory_order_acquire)!=8&&
          GetTickCount64()<until)Sleep(1);
    return unload_ready()?0:40;
}
} // namespace handik
