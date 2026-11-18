// Included inside troika_mounted_parts. Resolve ORIGINAL byte meanings through
// Core.Object.GetEnum; never assume a host/compiler enum's ordinal mapping.
namespace control_axes {
struct EnumLayout {
    uptr function{},enum_class{};unsigned bytes{};std::array<unsigned,3> offsets{};bool valid{};
};
template<class Call>bool enum_schema(Call const& call,metadata::Reader& r,EnumLayout& output) noexcept {
    if(!call.valid||!call.r.function||call.r.size>sizeof call.r.params)return false;
    EnumLayout value{};value.function=call.r.function;value.bytes=call.r.size;
    unsigned fn{},kind{},owner{},owner_kind{},package{};metadata::Structure layout{};
    if(!r.path(call.r.function,fn)||!r.named(fn,L"GetEnum")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||!r.named(owner,L"Object")||
       !r.path(r.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Core")||r.result.identities[package].outer||
       !r.structure(call.r.function,layout))return false;
    std::uint16_t bytes{},returned{};
    if(!r.at(call.r.function,0x106,bytes)||bytes!=value.bytes||!r.at(call.r.function,0x108,returned))return false;
    std::array<uptr,8> visited{};unsigned count{},found{};
    for(auto child=layout.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
        metadata::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=call.r.function||
           p.dimension!=1||!metadata::property_fits(p,layout.extent)||!troika_reflection::retain(r,p))return false;
        if(p.flags&0x80){
            unsigned index{};
            if(r.named(p.identity,L"E")&&weaponhand::core_type(r,p.type,L"ObjectProperty")&&p.element_bytes==8&&(p.flags&0x590)==0x80){
                uptr reference{};unsigned cls{},cls_kind{},cls_package{};
                if(!r.at(child,0xd0,reference)||!r.path(reference,cls)||(!r.named(cls,L"Enum")&&!r.named(cls,L"Object"))||
                   !r.path(r.result.identities[cls].klass,cls_kind)||!weaponhand::core_type(r,cls_kind,L"Class")||
                   !r.path(r.result.identities[cls].outer,cls_package)||!r.named(cls_package,L"Core")||r.result.identities[cls_package].outer)return false;
                value.enum_class=reference;index=0;
            }else if(r.named(p.identity,L"I")&&weaponhand::core_type(r,p.type,L"IntProperty")&&p.element_bytes==4&&(p.flags&0x590)==0x80)index=1;
            else if(r.named(p.identity,L"ReturnValue")&&weaponhand::core_type(r,p.type,L"NameProperty")&&
                    p.element_bytes==8&&((p.flags&0x590)==0x480||(p.flags&0x590)==0x580)&&p.offset==returned)index=2;
            else return false;
            if((found&(1u<<index))||!metadata::property_fits(p,bytes)||p.offset%(index==0?8:4))return false;
            found|=1u<<index;value.offsets[index]=p.offset;
        }
        child=p.next;
    }
    constexpr unsigned widths[]={8,4,8};
    if(found!=7)return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<i;++j)
        if(value.offsets[i]<value.offsets[j]+widths[j]&&value.offsets[j]<value.offsets[i]+widths[i])return false;
    if(!socket_contract::retained_metadata(r))return false;value.valid=true;output=value;return true;
}
enum class Axis:unsigned {unknown,x,y,z};
enum class Space:unsigned {unknown,world,actor,component,parent_bone,bone,other_bone};
struct Decoded {std::array<Axis,2> look{},up{};std::array<Space,2> rotation{},target{};};
inline Axis axis_label(metadata::NameText const& text) noexcept {
    if(metadata::exact_text(text,L"AXIS_X"))return Axis::x;
    if(metadata::exact_text(text,L"AXIS_Y"))return Axis::y;
    if(metadata::exact_text(text,L"AXIS_Z"))return Axis::z;return Axis::unknown;
}
inline Space space_label(metadata::NameText const& text) noexcept {
    if(metadata::exact_text(text,L"BCS_WorldSpace"))return Space::world;
    if(metadata::exact_text(text,L"BCS_ActorSpace"))return Space::actor;
    if(metadata::exact_text(text,L"BCS_ComponentSpace"))return Space::component;
    if(metadata::exact_text(text,L"BCS_ParentBoneSpace"))return Space::parent_bone;
    if(metadata::exact_text(text,L"BCS_BoneSpace"))return Space::bone;
    if(metadata::exact_text(text,L"BCS_OtherBoneSpace"))return Space::other_bone;return Space::unknown;
}
template<class Factory,class Current>bool decode(hand::ObjectIdentity const& turret,Snapshot const& input,
    Factory&& make_call,Current&& current,Decoded& output) noexcept {
    try{
        struct Cache {EnumByte input{};metadata::NameText text{};};
        std::array<Cache,8> cache{};unsigned used{};Decoded value{};
        const auto resolve=[&](EnumByte const& raw,metadata::NameText& text){
            metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::final_check};
            auto call=make_call(turret.address);EnumLayout before{},after{};
            if(!current()||!metadata::tables(base,schema.objects,schema.names)||!weaponhand::object(r,turret)||
               !weaponhand::object(r,raw.enumeration)||!enum_schema(call,r,before)||
               !class_derives(r,raw.enumeration.klass,before.enum_class))return false;
            const auto enumeration=raw.enumeration.address;const auto ordinal=static_cast<std::int32_t>(raw.value);
            std::memcpy(call.r.params+before.offsets[0],&enumeration,8);
            std::memcpy(call.r.params+before.offsets[1],&ordinal,4);
            if(!current()||!call.run()||!enum_schema(call,r,after)||before.function!=after.function||
               before.bytes!=after.bytes||before.offsets!=after.offsets||before.enum_class!=after.enum_class||!current())return false;
            metadata::RawName result{};std::memcpy(&result,call.r.params+before.offsets[2],8);
            return !result.number&&metadata::fresh_name(schema.names,result.index,text)&&
                socket_contract::retained_metadata(r)&&current();
        };
        for(unsigned side=0;side<2;++side)for(unsigned field=0;field<4;++field){
            if(!current())return false;auto const& raw=input.controls[side].bytes[field];
            metadata::NameText text{};bool cached=false;
            for(unsigned i=0;i<used;++i)if(hand::same(cache[i].input.enumeration,raw.enumeration)&&cache[i].input.value==raw.value){
                text=cache[i].text;cached=true;break;}
            if(!cached){
                if(!resolve(raw,text))return false;
                cache[used++]={raw,text};
            }
            if(field<2){const auto axis=axis_label(text);if(axis==Axis::unknown)return false;
                (field?value.up:value.look)[side]=axis;
            }else {const auto space=space_label(text);if(space==Space::unknown)return false;
                (field==2?value.rotation:value.target)[side]=space;}
        }
        // Requery each distinct original enum/value at closing. Neither a
        // cached FName nor a stable UEnum identity proves unchanged contents.
        for(unsigned i=0;i<used;++i){metadata::NameText closing{};
            if(!resolve(cache[i].input,closing)||!metadata::same_name_text(cache[i].text,closing))return false;}
        if(!current())return false;output=value;return true;
    }catch(...){return false;}
}
} // namespace control_axes
