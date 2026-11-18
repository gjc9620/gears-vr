// Included inside troika_mounted_parts after control_axes. ORIGINAL operations
// only; this result is not a Controller rotation write or a fire permit.
namespace original_limits {
namespace metadata=weaponhand::meta;namespace hand=gears_weapon_hand;
enum class Mode {point,rotation};
struct Layout {uptr function{};unsigned bytes{},mask{};std::array<unsigned,5> offsets{},masks{},widths{};};
template<class Call>bool schema(Call const& call,metadata::Reader& r,Mode mode,Layout& output) noexcept {
    if(!call.valid||!call.r.function||call.r.size>sizeof call.r.params)return false;
    Layout value{};value.function=call.r.function;value.bytes=call.r.size;
    unsigned fn{},kind{},owner{},owner_kind{},package{};metadata::Structure function{};
    if(!r.path(call.r.function,fn)||!(mode==Mode::point?r.named(fn,L"CanLookAtPoint"):r.named(fn,L"ProcessViewRotation"))||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||
       !(mode==Mode::point?r.named(owner,L"SkelControlLookAt"):r.named(owner,L"Turret_TroikaCabal_base"))||
       !r.path(r.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||
       !(mode==Mode::point?r.named(package,L"Engine"):r.named(package,L"WarfareGame"))||r.result.identities[package].outer||
       !r.structure(call.r.function,function))return false;
    std::uint16_t bytes{},returned{};
    if(!r.at(call.r.function,0x106,bytes)||bytes!=value.bytes||!r.at(call.r.function,0x108,returned)||
       (mode==Mode::rotation&&returned!=0xffff))return false;
    std::array<uptr,12> visited{};unsigned count{},found{};
    for(auto child=function.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
        metadata::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=call.r.function||p.dimension!=1||
           !metadata::property_fits(p,function.extent)||!troika_reflection::retain(r,p))return false;
        if(p.flags&0x80){
            unsigned index=5;bool vector=false,rotator=false,boolean=false;
            if(mode==Mode::point){
                if(r.named(p.identity,L"PointLoc")&&(p.flags&0x590)==0x80){index=0;vector=true;}
                else if(r.named(p.identity,L"bDrawDebugInfo")&&((p.flags&0x590)==0x80||(p.flags&0x590)==0x90)){index=1;boolean=true;}
                else if(r.named(p.identity,L"bDebugUsePersistentLines")&&((p.flags&0x590)==0x80||(p.flags&0x590)==0x90)){index=2;boolean=true;}
                else if(r.named(p.identity,L"bDebugFlushLinesFirst")&&((p.flags&0x590)==0x80||(p.flags&0x590)==0x90)){index=3;boolean=true;}
                else if(r.named(p.identity,L"ReturnValue")&&(p.flags&0x590)==0x480&&p.offset==returned){index=4;boolean=true;}
            }else {
                if(r.named(p.identity,L"DeltaTime")&&(p.flags&0x590)==0x80&&weaponhand::core_type(r,p.type,L"FloatProperty")&&p.element_bytes==4)index=0;
                else if(r.named(p.identity,L"OutViewRot")&&(p.flags&0x590)==0x180){index=1;rotator=true;}
                else if(r.named(p.identity,L"OutDeltaRot")&&(p.flags&0x590)==0x180){index=2;rotator=true;}
            }
            if(index==5||(found&(1u<<index))||!metadata::property_fits(p,bytes)||p.offset%4)return false;
            if(vector||rotator){troika_reflection::ScriptStruct reflected{};
                if(!(vector?troika_reflection::numeric(r,p,L"Vector",L"FloatProperty",std::array<wchar_t const*,3>{L"X",L"Y",L"Z"},reflected):
                            troika_reflection::numeric(r,p,L"Rotator",L"IntProperty",std::array<wchar_t const*,3>{L"Pitch",L"Yaw",L"Roll"},reflected)))return false;
            }
            if(boolean){
                if(!weaponhand::core_type(r,p.type,L"BoolProperty")||p.element_bytes!=4||!r.at(child,0xd0,value.masks[index])||
                   !value.masks[index]||(value.masks[index]&(value.masks[index]-1)))return false;
            }
            value.widths[index]=p.element_bytes;value.offsets[index]=p.offset;found|=1u<<index;
        }
        child=p.next;
    }
    const auto expected=mode==Mode::point?31u:7u;if(found!=expected)return false;
    for(unsigned i=0;i<5;++i)if(found&(1u<<i))for(unsigned j=0;j<i;++j)if(found&(1u<<j)){
        const bool overlaps=value.offsets[i]<value.offsets[j]+value.widths[j]&&value.offsets[j]<value.offsets[i]+value.widths[i];
        if(overlaps&&!(value.offsets[i]==value.offsets[j]&&value.masks[i]&&value.masks[j]&&!(value.masks[i]&value.masks[j])))return false;
    }
    if(!socket_contract::retained_metadata(r))return false;value.mask=found;output=value;return true;
}
inline bool same(Layout const& a,Layout const& b) noexcept {
    return a.function==b.function&&a.bytes==b.bytes&&a.mask==b.mask&&a.offsets==b.offsets&&a.masks==b.masks&&a.widths==b.widths;
}
template<class Factory,class Current>bool point(hand::ObjectIdentity const& control,std::array<float,3> const& target,
    Factory&& factory,Current&& current,bool& output) noexcept {
    try{
        for(auto x:target)if(!std::isfinite(x))return false;if(!current())return false;
        metadata::Snapshot source{};source.module_base=base;metadata::Reader r{source,metadata::Stage::final_check};
        auto call=factory(control.address);Layout before{},after{};
        if(!metadata::tables(base,source.objects,source.names)||!weaponhand::object(r,control)||
           !weaponhand::derives(r,control.address,L"SkelControlLookAt",L"Engine")||!schema(call,r,Mode::point,before))return false;
        std::memset(call.r.params,0,call.r.size);std::memcpy(call.r.params+before.offsets[0],target.data(),12);
        // All original debug inputs stay false, including packed Bool masks.
        if(!current()||!call.run()||!schema(call,r,Mode::point,after)||!same(before,after)||!current())return false;
        unsigned returned{};std::memcpy(&returned,call.r.params+before.offsets[4],4);
        if(!socket_contract::retained_metadata(r)||!current())return false;
        output=(returned&before.masks[4])!=0;return true;
    }catch(...){return false;}
}
template<class Factory,class Current>bool rotation(hand::ObjectIdentity const& turret,std::array<std::int32_t,3> const& requested,
    Factory&& factory,Current&& current,std::array<std::int32_t,3>& output) noexcept {
    try{
        if(!current())return false;metadata::Snapshot source{};source.module_base=base;metadata::Reader r{source,metadata::Stage::final_check};
        auto call=factory(turret.address);Layout before{},after{};
        if(!metadata::tables(base,source.objects,source.names)||!weaponhand::object(r,turret)||
           !weaponhand::derives(r,turret.address,L"Turret_TroikaCabal_base",L"WarfareGame")||!schema(call,r,Mode::rotation,before))return false;
        // Query an absolute requested view with no added controller delta or
        // elapsed-time interpolation. Original pawn/turret limits still run.
        std::memset(call.r.params,0,call.r.size);std::memcpy(call.r.params+before.offsets[1],requested.data(),12);
        if(!current()||!call.run()||!schema(call,r,Mode::rotation,after)||!same(before,after)||!current())return false;
        std::array<std::int32_t,3> result{},delta{};
        std::memcpy(result.data(),call.r.params+before.offsets[1],12);std::memcpy(delta.data(),call.r.params+before.offsets[2],12);
        if(delta!=std::array<std::int32_t,3>{}||!socket_contract::retained_metadata(r)||!current())return false;
        output=result;return true;
    }catch(...){return false;}
}
#if !defined(GEARS_TROIKA_LIMITS_FIXTURE_ONLY)
using Current=bool(*)(void*) noexcept;
bool point_native(hand::ObjectIdentity const& control,std::array<float,3> const& target,void* context,Current current,bool& output) noexcept {
    return point(control,target,[](uptr receiver){return playground::Args(receiver,L"CanLookAtPoint");},
        [&](){return current&&current(context);},output);
}
bool rotation_native(hand::ObjectIdentity const& turret,std::array<std::int32_t,3> const& requested,
    void* context,Current current,std::array<std::int32_t,3>& output) noexcept {
    return rotation(turret,requested,[](uptr receiver){return playground::Args(receiver,L"ProcessViewRotation");},
        [&](){return current&&current(context);},output);
}
#endif
} // namespace original_limits
