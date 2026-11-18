namespace physical_start {
namespace metadata=weaponhand::meta;namespace hand=gears_weapon_hand;
struct Layout {uptr function{};unsigned bytes{};std::array<unsigned,2> offsets{};};
template<class Call>bool schema(Call const& call,metadata::Reader& r,Layout& output) noexcept {
    if(!call.valid||!call.r.function||call.r.size>sizeof call.r.params)return false;
    Layout value{call.r.function,call.r.size,{}};unsigned fn{},kind{},owner{},owner_kind{},package{};metadata::Structure function{};
    std::uint16_t bytes{},returned{};
    if(!r.path(call.r.function,fn)||!r.named(fn,L"GetPhysicalFireStartLoc")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||!r.named(owner,L"Turret_TroikaCabal_base")||
       !r.path(r.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"WarfareGame")||r.result.identities[package].outer||
       !r.structure(call.r.function,function)||!r.at(call.r.function,0x106,bytes)||bytes!=value.bytes||!r.at(call.r.function,0x108,returned))return false;
    std::array<uptr,12> seen{};unsigned count{},found{};
    for(auto child=function.children;child;){
        if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;seen[count++]=child;
        metadata::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=call.r.function||p.dimension!=1||
           !metadata::property_fits(p,function.extent)||!troika_reflection::retain(r,p))return false;
        if(p.flags&0x80){unsigned index{};troika_reflection::ScriptStruct vector{};
            if(r.named(p.identity,L"FireOffset")&&((p.flags&0x590)==0x80||(p.flags&0x590)==0x90))index=0; // Always supply zero, including an optional original input.
            else if(r.named(p.identity,L"ReturnValue")&&((p.flags&0x590)==0x480||(p.flags&0x590)==0x580)&&p.offset==returned)index=1;
            else return false;
            if((found&(1u<<index))||p.offset%4||!metadata::property_fits(p,value.bytes)||
               !troika_reflection::numeric(r,p,L"Vector",L"FloatProperty",std::array<wchar_t const*,3>{L"X",L"Y",L"Z"},vector))return false;
            found|=1u<<index;value.offsets[index]=p.offset;
        }child=p.next;
    }
    if(found!=3||(value.offsets[0]<value.offsets[1]+12&&value.offsets[1]<value.offsets[0]+12)||!socket_contract::retained_metadata(r))return false;
    output=value;return true;
}
template<class Factory,class Invoke,class Current>bool read(hand::ObjectIdentity const& turret,
    Factory&& factory,Invoke&& invoke,Current&& current,std::array<float,3>& output) noexcept {
    try{
        if(!current())return false;metadata::Snapshot snapshot{};snapshot.module_base=base;metadata::Reader r{snapshot,metadata::Stage::final_check};
        auto call=factory(turret.address);Layout before{},after{};
        if(!metadata::tables(base,snapshot.objects,snapshot.names)||!weaponhand::object(r,turret)||
           !weaponhand::derives(r,turret.address,L"Turret_TroikaCabal_base",L"WarfareGame")||!schema(call,r,before))return false;
        std::memset(call.r.params,0,call.r.size);
        if(!current()||!invoke(call)||!schema(call,r,after)||before.function!=after.function||before.bytes!=after.bytes||
           before.offsets!=after.offsets||!current())return false;
        std::array<float,3> value{};std::memcpy(value.data(),call.r.params+before.offsets[1],12);
        for(float component:value)if(!std::isfinite(component))return false;output=value;return true;
    }catch(...){return false;}
}
#if !defined(GEARS_TROIKA_PHYSICAL_START_FIXTURE_ONLY)
template<class Current>bool native(hand::ObjectIdentity const& turret,Current&& current,std::array<float,3>& output) noexcept {
    return read(turret,[](uptr receiver){return playground::Args(receiver,L"GetPhysicalFireStartLoc");},
        [](playground::Args& call){return original_aim::invoke_native(call)==original_aim::Dispatch::returned;},current,output);
}
#endif
} // namespace physical_start
