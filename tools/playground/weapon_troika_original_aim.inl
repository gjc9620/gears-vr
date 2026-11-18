// Included inside troika_mounted_parts. These are original GT engine calls,
// not raw Rotation/AimDir writes. Caller owns input and one-shot dispatch.
namespace original_aim {
namespace metadata=weaponhand::meta;namespace hand=gears_weapon_hand;
enum class Mode {set,face};
enum class Dispatch {not_dispatched,returned,uncertain};
struct Result {
    Dispatch dispatch{Dispatch::not_dispatched};
    bool original_bool_known{},original_bool{},metadata_closed{},source_closed{};
    bool accepted_set() const noexcept {return dispatch==Dispatch::returned&&original_bool_known&&original_bool&&metadata_closed&&source_closed;}
    bool completed_face() const noexcept {return dispatch==Dispatch::returned&&metadata_closed&&source_closed;}
};
struct Layout {uptr function{};unsigned bytes{},return_mask{};std::array<unsigned,2> offsets{};};
inline bool same(Layout const& a,Layout const& b) noexcept {
    return a.function==b.function&&a.bytes==b.bytes&&a.return_mask==b.return_mask&&a.offsets==b.offsets;
}
template<class Call>bool schema(Call const& call,metadata::Reader& r,Mode mode,Layout& output) noexcept {
    if(!call.valid||!call.r.function||call.r.size>sizeof call.r.params)return false;
    Layout value{};value.function=call.r.function;value.bytes=call.r.size;
    unsigned fn{},kind{},owner{},owner_kind{},package{};metadata::Structure function{};
    if(!r.path(call.r.function,fn)||!(mode==Mode::set?r.named(fn,L"SetRotation"):r.named(fn,L"FaceRotation"))||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||!(mode==Mode::set?r.named(owner,L"Actor"):r.named(owner,L"Turret_TroikaCabal_base"))||
       !r.path(r.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||!(mode==Mode::set?r.named(package,L"Engine"):r.named(package,L"WarfareGame"))||
       r.result.identities[package].outer||!r.structure(call.r.function,function))return false;
    std::uint16_t bytes{},returned{};
    if(!r.at(call.r.function,0x106,bytes)||bytes!=value.bytes||!r.at(call.r.function,0x108,returned)||
       (mode==Mode::face&&returned!=0xffff))return false;
    std::array<uptr,8> visited{};unsigned count{},found{};
    for(auto child=function.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
        metadata::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=call.r.function||p.dimension!=1||
           !metadata::property_fits(p,function.extent)||!troika_reflection::retain(r,p))return false;
        if(p.flags&0x80){unsigned index{};
            if(r.named(p.identity,L"NewRotation")&&(p.flags&0x590)==0x80){
                troika_reflection::ScriptStruct type{};
                if(!troika_reflection::numeric(r,p,L"Rotator",L"IntProperty",std::array<wchar_t const*,3>{L"Pitch",L"Yaw",L"Roll"},type))return false;
                index=0;
            }else if(mode==Mode::set&&r.named(p.identity,L"ReturnValue")&&(p.flags&0x590)==0x480&&
                     weaponhand::core_type(r,p.type,L"BoolProperty")&&p.element_bytes==4&&p.offset==returned){
                if(!r.at(child,0xd0,value.return_mask)||!value.return_mask||(value.return_mask&(value.return_mask-1)))return false;index=1;
            }else if(mode==Mode::face&&r.named(p.identity,L"DeltaTime")&&(p.flags&0x590)==0x80&&
                     weaponhand::core_type(r,p.type,L"FloatProperty")&&p.element_bytes==4)index=1;
            else return false;
            if((found&(1u<<index))||!metadata::property_fits(p,bytes)||p.offset%4)return false;
            found|=1u<<index;value.offsets[index]=p.offset;
        }
        child=p.next;
    }
    if(found!=3||(value.offsets[0]<value.offsets[1]+4&&value.offsets[1]<value.offsets[0]+12)||
       !socket_contract::retained_metadata(r))return false;
    output=value;return true;
}
template<class Factory,class Invoke,class Current,class Before>Result execute(hand::ObjectIdentity const& receiver,
    std::array<std::int32_t,3> const& rotation,Mode mode,Factory&& factory,Invoke&& invoke,Current&& current,Before&& before) noexcept {
    Result result{};
    try{
        if(!current())return result;
        metadata::Snapshot schema_value{};schema_value.module_base=base;metadata::Reader r{schema_value,metadata::Stage::final_check};
        auto call=factory(receiver.address);Layout first{},closing{};
        if(!metadata::tables(base,schema_value.objects,schema_value.names)||!weaponhand::object(r,receiver)||
           !(mode==Mode::set?weaponhand::derives(r,receiver.address,L"PlayerController",L"Engine"):
                             weaponhand::derives(r,receiver.address,L"Turret_TroikaCabal_base",L"WarfareGame"))||
           !schema(call,r,mode,first))return result;
        std::memset(call.r.params,0,call.r.size);std::memcpy(call.r.params+first.offsets[0],rotation.data(),12);
        if(!current()||!before())return result;
        result.dispatch=Dispatch::uncertain; // Before Invoke, including C++/SEH failure.
        result.dispatch=invoke(call);
        if(result.dispatch!=Dispatch::returned)return result;
        result.metadata_closed=schema(call,r,mode,closing)&&same(first,closing)&&socket_contract::retained_metadata(r);
        if(result.metadata_closed&&mode==Mode::set){unsigned answer{};std::memcpy(&answer,call.r.params+first.offsets[1],4);
            result.original_bool_known=true;result.original_bool=(answer&first.return_mask)!=0;}
        result.source_closed=current();return result;
    }catch(...){return result;}
}
inline bool rotation_matches(std::array<std::int32_t,3> const& actual,std::array<std::int32_t,3> const& expected) noexcept {
    for(unsigned i=0;i<3;++i){auto difference=(std::int64_t(actual[i])-expected[i])%65536;
        if(difference>32768)difference-=65536;if(difference< -32768)difference+=65536;
        if(difference< -1||difference>1)return false;}
    return true;
}
struct ActorReceipt {
    Result setter{},face{};std::array<std::int32_t,3> requested{},controller{},aim{};
    bool controller_read{},aim_read{};
    bool verified_actor() const noexcept {return setter.accepted_set()&&face.completed_face()&&controller_read&&aim_read&&
        rotation_matches(controller,requested)&&rotation_matches(aim,controller);}
};
// Pure per-caller dispatch order; never a permission token. begin() consumes a
// publication even when its later prepare fails. No same-publication replay.
class AttemptGate {
    std::array<std::uint64_t,7> scope_{};std::uint64_t publication_{};bool seen_{},active_{};
public:
    bool begin(std::array<std::uint64_t,7> const& scope,std::uint64_t publication) noexcept {
        if(active_||!publication)return false;for(auto value:scope)if(!value)return false;
        if(seen_&&scope==scope_&&publication<=publication_)return false;
        scope_=scope;publication_=publication;seen_=active_=true;return true;
    }
    void finish() noexcept {active_=false;}
    bool active() const noexcept {return active_;}
};
#if !defined(GEARS_TROIKA_AIM_FIXTURE_ONLY)
inline Dispatch invoke_native(playground::Args& call) {
    __try{return call.run()?Dispatch::returned:Dispatch::uncertain;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return Dispatch::uncertain;}
}
using Current=bool(*)(void*) noexcept;
using Before=bool(*)(void*) noexcept;
Result set_native(hand::ObjectIdentity const& controller,std::array<std::int32_t,3> const& value,void* context,Current current,Before before) noexcept {
    return execute(controller,value,Mode::set,[](uptr receiver){return playground::Args(receiver,L"SetRotation");},invoke_native,
        [&](){return current&&current(context);},[&](){return before&&before(context);});
}
Result face_native(hand::ObjectIdentity const& turret,std::array<std::int32_t,3> const& value,void* context,Current current,Before before) noexcept {
    return execute(turret,value,Mode::face,[](uptr receiver){return playground::Args(receiver,L"FaceRotation");},invoke_native,
        [&](){return current&&current(context);},[&](){return before&&before(context);});
}
#endif
} // namespace original_aim
