// Inside troika_mounted_parts, after the original control binding reader.
// Read the ORIGINAL selected socket's actual asset entry and bone attachment.
namespace socket_binding {
namespace metadata=weaponhand::meta;
namespace hand=gears_weapon_hand;
constexpr unsigned maximum_sockets=128;
struct Snapshot {
    hand::ObjectIdentity object{};metadata::RawName socket_name{},bone_name{};
    std::array<float,3> location{},scale{};std::array<std::int32_t,3> rotation{};
    unsigned bone{};metadata::Table table{};
};
template<class T,std::size_t N,std::size_t K>bool field(metadata::Reader& r,uptr object,wchar_t const(&label)[N],
    wchar_t const(&kind)[K],T& output,weaponhand::BoolField& retained) noexcept {
    return weaponhand::field(r,object,label,kind,sizeof(T),retained)&&
        control_binding::field_owner(r,retained,L"SkeletalMeshSocket")&&troika_reflection::retain(r,retained.property)&&
        r.at(object,retained.property.offset,output);
}
template<std::size_t N>bool vector(metadata::Reader& r,uptr object,wchar_t const(&label)[N],std::array<float,3>& output) noexcept {
    weaponhand::BoolField f{};troika_reflection::ScriptStruct type{};
    if(!field(r,object,label,L"StructProperty",output,f)||
       !troika_reflection::numeric(r,f.property,L"Vector",L"FloatProperty",std::array<wchar_t const*,3>{L"X",L"Y",L"Z"},type))return false;
    for(auto v:output)if(!std::isfinite(v))return false;return true;
}
inline bool same(Snapshot const& a,Snapshot const& b) noexcept {
    return hand::same(a.object,b.object)&&a.socket_name.index==b.socket_name.index&&a.socket_name.number==b.socket_name.number&&
        a.bone_name.index==b.bone_name.index&&a.bone_name.number==b.bone_name.number&&a.bone==b.bone&&
        a.location==b.location&&a.rotation==b.rotation&&a.scale==b.scale&&
        a.table.data==b.table.data&&a.table.count==b.table.count&&a.table.capacity==b.table.capacity;
}
template<class Current>bool read(hand::ObjectIdentity const& asset,metadata::RawName selected,
    control_binding::Snapshot const& binding,Current&& current,Snapshot& output) noexcept {
    try{
        if(selected.number||!current()||!hand::same(asset,binding.asset))return false;
        metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::final_check};
        weaponhand::BoolField array{};metadata::Property inner{};uptr inner_address{},socket_class{};unsigned declared{},kind{},package{};metadata::NameText selected_text{};
        if(!metadata::tables(base,schema.objects,schema.names)||!weaponhand::object(r,asset)||
           !metadata::fresh_name(schema.names,selected.index,selected_text)||!selected_text.length||metadata::exact_text(selected_text,L"None")||
           !weaponhand::derives(r,asset.address,L"SkeletalMesh",L"Engine")||
           !weaponhand::field(r,asset.address,L"Sockets",L"ArrayProperty",16,array)||
           !control_binding::field_owner(r,array,L"SkeletalMesh")||!troika_reflection::retain(r,array.property)||
           !r.at(r.result.identities[array.property.identity].address,0xd0,inner_address)||!r.property(inner_address,inner)||
           r.result.identities[inner.identity].outer!=r.result.identities[array.property.identity].address||
           !r.named(inner.identity,L"Sockets")||inner.offset||inner.dimension!=1||inner.element_bytes!=8||
           !weaponhand::core_type(r,inner.type,L"ObjectProperty")||!troika_reflection::retain(r,inner)||
           !r.at(inner_address,0xd0,socket_class)||!r.path(socket_class,declared)||!r.named(declared,L"SkeletalMeshSocket")||
           !r.path(r.result.identities[declared].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
           !r.path(r.result.identities[declared].outer,package)||!r.named(package,L"Engine")||r.result.identities[package].outer)return false;
        std::array<uptr,maximum_sockets> entries{},first_entries{},closing_entries{};
        const auto sample=[&](Snapshot& out){
            if(!r.at(asset.address,array.property.offset,out.table)||!control_binding::bounds(out.table,8,maximum_sockets,true)||
               !metadata::read_bytes(out.table.data,0,entries.data(),std::size_t(out.table.count)*8))return false;
            unsigned matches{};
            for(int i=0;i<out.table.count;++i){const auto address=entries[unsigned(i)];
                if(!address)return false;for(int j=0;j<i;++j)if(entries[unsigned(j)]==address)return false;
                hand::ObjectIdentity object{};metadata::RawName name{};weaponhand::BoolField f{};
                if(!control_axes::identity(r,address,object)||object.outer!=asset.address||!control_axes::class_derives(r,object.klass,socket_class)||
                   !field(r,address,L"SocketName",L"NameProperty",name,f))return false;
                if(name.index!=selected.index||name.number!=selected.number)continue;
                if(++matches!=1)return false;out.object=object;out.socket_name=name;
                if(!field(r,address,L"BoneName",L"NameProperty",out.bone_name,f)||out.bone_name.number||
                   !vector(r,address,L"RelativeLocation",out.location)||!vector(r,address,L"RelativeScale",out.scale)||
                   !field(r,address,L"RelativeRotation",L"StructProperty",out.rotation,f))return false;
                troika_reflection::ScriptStruct rotation{};
                if(!troika_reflection::numeric(r,f.property,L"Rotator",L"IntProperty",
                   std::array<wchar_t const*,3>{L"Pitch",L"Yaw",L"Roll"},rotation))return false;
                unsigned bone_matches{};
                for(unsigned bone:{7u,8u}){metadata::RawName candidate{};
                    std::memcpy(&candidate,binding.reference_bytes[bone].data(),8);
                    if(candidate.index==out.bone_name.index&&candidate.number==out.bone_name.number){++bone_matches;out.bone=bone;}}
                if(bone_matches!=1)return false;
                for(float scale:out.scale)if(scale<=0||std::abs(scale-out.scale[0])>1e-5)return false;
            }
            return matches==1;
        };
        Snapshot first{},closing{};
        if(!sample(first))return false;first_entries=entries;
        if(!current()||!control_axes::class_children(r)||!socket_contract::retained_metadata(r)||
           !metadata::read_bytes(first.table.data,0,closing_entries.data(),std::size_t(first.table.count)*8)||entries!=closing_entries||
           !sample(closing)||entries!=first_entries||!same(first,closing)||!control_axes::class_children(r)||!socket_contract::retained_metadata(r)||!current())return false;
        output=first;return true;
    }catch(...){return false;}
}
} // namespace socket_binding
