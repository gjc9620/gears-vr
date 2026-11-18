// Included inside troika_mounted_parts after control_axes. Data only: binds
// the original cached controls to ORIGINAL active animation lists/bone slots.
namespace control_binding {
namespace metadata=weaponhand::meta;namespace hand=gears_weapon_hand;
constexpr unsigned maximum_lists=32,maximum_nodes=64,troika_bones=13;
struct Row {metadata::RawName bone{};uptr head{};int draw_y{};};
struct Node {hand::ObjectIdentity object{},component{},component_reference{},next_reference{};uptr next{};};
struct Snapshot {
    hand::ObjectIdentity tree{},tree_reference{},list_type{},head_reference{},asset{},asset_reference{},parent_anim_reference{};
    metadata::Table lists{},indices{},references{};unsigned stride{},node_count{};
    // Indexed by cached CONTROL identity, not assumed mechanical joint order.
    std::array<unsigned,2> list_index{},bone_index{};
    std::array<Row,maximum_lists> rows{};
    std::array<Node,maximum_nodes> nodes{};
    std::array<unsigned char,troika_bones> index_values{};
    // Existing certified native ReferenceRecord68 ABI; script RefSkeleton's
    // dummy IntProperty/4 is authenticated separately and is not its stride.
    std::array<std::array<unsigned char,68>,troika_bones> reference_bytes{};
};
static_assert(std::is_trivially_copyable_v<Snapshot>);
template<class Field,std::size_t N>bool field_owner(metadata::Reader& r,Field const& field,wchar_t const (&name)[N]) noexcept {
    unsigned kind{},package{};
    return r.named(field.owner,name)&&r.path(r.result.identities[field.owner].klass,kind)&&weaponhand::core_type(r,kind,L"Class")&&
        r.path(r.result.identities[field.owner].outer,package)&&r.named(package,L"Engine")&&!r.result.identities[package].outer;
}
template<std::size_t N,std::size_t K,std::size_t O>bool link(metadata::Reader& r,uptr object,wchar_t const (&label)[N],
    wchar_t const (&declared_name)[K],wchar_t const (&owner_name)[O],bool nullable,
    hand::ObjectIdentity& value,hand::ObjectIdentity& declaration,bool component=false) noexcept {
    metadata::PointerField field{};uptr reference{};unsigned slot{},kind{},package{};
    if(!weaponhand::read_pointer(r,object,label,field,component)||!troika_reflection::retain(r,field.property)||!field_owner(r,field,owner_name)||
       !r.at(r.result.identities[field.property.identity].address,0xd0,reference)||!r.path(reference,slot)||!r.named(slot,declared_name)||
       !r.path(r.result.identities[slot].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
       !r.path(r.result.identities[slot].outer,package)||!r.named(package,L"Engine")||r.result.identities[package].outer||
       !control_axes::identity(r,reference,declaration))return false;
    if(!field.value){value={};return nullable;}
    return control_axes::identity(r,field.value,value)&&control_axes::class_derives(r,value.klass,reference);
}
inline bool bounds(metadata::Table const& a,unsigned stride,unsigned maximum,bool required) noexcept {
    return a.count>=0&&a.capacity>=a.count&&unsigned(a.capacity)<=maximum&&(!required||a.count>0)&&
        (!a.count||a.data)&&(!a.data||a.data<=UINTPTR_MAX-std::size_t(a.capacity)*stride);
}
struct ListLayout {
    troika_reflection::ScriptStruct reflected{};
    hand::ObjectIdentity head_reference{};
    std::array<unsigned,3> offsets{}; // BoneName, ControlHead, DrawY
};
bool list_layout(metadata::Reader& r,metadata::Property const& array,ListLayout& output) noexcept {
    uptr inner_address{};metadata::Property inner{};ListLayout result{};
    if(!r.at(r.result.identities[array.identity].address,0xd0,inner_address)||!r.property(inner_address,inner)||
       r.result.identities[inner.identity].outer!=r.result.identities[array.identity].address||
       !r.named(inner.identity,L"SkelControlLists")||inner.offset||inner.dimension!=1||inner.element_bytes<20||inner.element_bytes>64||
       !troika_reflection::read(r,inner,L"SkelControlListHead",L"AnimTree",L"Engine",inner.element_bytes,result.reflected))return false;
    unsigned found{},count{};std::array<uptr,8> visited{};
    for(auto child=result.reflected.layout.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
        metadata::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=result.reflected.address||p.dimension!=1||
           !metadata::property_fits(p,result.reflected.layout.extent)||!troika_reflection::retain(r,p))return false;
        unsigned index{};
        if(r.named(p.identity,L"BoneName")&&weaponhand::core_type(r,p.type,L"NameProperty")&&p.element_bytes==8)index=0;
        else if(r.named(p.identity,L"ControlHead")&&weaponhand::core_type(r,p.type,L"ObjectProperty")&&p.element_bytes==8){
            uptr reference{};unsigned slot{},kind{},package{};
            if(!r.at(child,0xd0,reference)||!r.path(reference,slot)||!r.named(slot,L"SkelControlBase")||
               !r.path(r.result.identities[slot].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
               !r.path(r.result.identities[slot].outer,package)||!r.named(package,L"Engine")||r.result.identities[package].outer||
               !control_axes::identity(r,reference,result.head_reference))return false;
            index=1;
        }else if(r.named(p.identity,L"DrawY")&&weaponhand::core_type(r,p.type,L"IntProperty")&&p.element_bytes==4)index=2;
        else return false;
        if((found&(1u<<index))||p.offset%(index==2?4:8))return false;
        result.offsets[index]=p.offset;found|=1u<<index;child=p.next;
    }
    constexpr unsigned widths[]={8,8,4};if(found!=7)return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<i;++j)
        if(result.offsets[i]<result.offsets[j]+widths[j]&&result.offsets[j]<result.offsets[i]+widths[i])return false;
    output=result;return true;
}
bool reference(metadata::Reader& r,hand::ObjectIdentity const& mesh,Snapshot& result) noexcept {
    hand::ObjectIdentity parent{};weaponhand::BoolField field{};unsigned asset{},package{};
    if(!link(r,mesh.address,L"SkeletalMesh",L"SkeletalMesh",L"SkeletalMeshComponent",false,result.asset,result.asset_reference)||
       !r.path(result.asset.address,asset)||!r.named(asset,L"TroikaTurret")||
       !r.path(r.result.identities[asset].outer,package)||!r.named(package,L"Geist_TroikaCabal")||r.result.identities[package].outer||
       !link(r,mesh.address,L"ParentAnimComponent",L"SkeletalMeshComponent",L"SkeletalMeshComponent",true,parent,result.parent_anim_reference,true)||parent.address||
       !weaponhand::field(r,result.asset.address,L"RefSkeleton",L"ArrayProperty",16,field)||!troika_reflection::retain(r,field.property)||
       !field_owner(r,field,L"SkeletalMesh")||
       !r.at(result.asset.address,field.property.offset,result.references)||!bounds(result.references,68,512,true)||result.references.count!=troika_bones)return false;
    uptr inner_address{};metadata::Property inner{};
    if(!r.at(r.result.identities[field.property.identity].address,0xd0,inner_address)||!r.property(inner_address,inner)||
       r.result.identities[inner.identity].outer!=r.result.identities[field.property.identity].address||!r.named(inner.identity,L"RefSkeleton")||
       inner.offset||inner.dimension!=1||inner.element_bytes!=4||!weaponhand::core_type(r,inner.type,L"IntProperty")||
       !troika_reflection::retain(r,inner)||!metadata::read_bytes(result.references.data,0,result.reference_bytes.data(),sizeof result.reference_bytes))return false;
    constexpr wchar_t const* names[]={L"b_troika_root",L"b_troika_gun_latitude",L"b_troika_gun_longitude",
        L"b_troika_spinner_L",L"b_troika_spinner_R",L"b_troika_port_R",L"b_troika_port_L",L"b_troika_muzzle_R",
        L"b_troika_muzzle_L",L"b_troika_handleroot",L"b_troika_handle_L",L"b_troika_handle_R",L"b_troika_playerref"};
    constexpr std::int32_t parents[]={0,0,1,2,2,2,2,2,2,2,9,9,1};
    for(unsigned i=0;i<troika_bones;++i){auto const& bytes=result.reference_bytes[i];
        metadata::RawName name{};metadata::NameText text{};std::int32_t parent_index{};
        std::array<float,4> rotation{};std::array<float,3> position{};
        std::memcpy(&name,bytes.data(),8);std::memcpy(rotation.data(),bytes.data()+12,16);
        std::memcpy(position.data(),bytes.data()+28,12);std::memcpy(&parent_index,bytes.data()+56,4);
        if(name.number||!metadata::fresh_name(r.result.names,name.index,text)||parent_index!=parents[i])return false;
        const auto length=std::wcslen(names[i]);if(text.length!=length||std::wmemcmp(text.units.data(),names[i],length+1))return false;
        double norm{};for(auto x:rotation){if(!std::isfinite(x))return false;norm+=double(x)*x;}
        if(std::abs(norm-1)>1e-4)return false;for(auto x:position)if(!std::isfinite(x))return false;
    }
    return true;
}
bool sample(metadata::Reader& r,hand::ObjectIdentity const& turret,hand::ObjectIdentity const& mesh,
    control_axes::Snapshot const& controls,Snapshot& output) noexcept {
    Snapshot result{};std::memset(&result,0,sizeof result);
    if(!reference(r,mesh,result)||!link(r,mesh.address,L"Animations",L"AnimNode",L"SkeletalMeshComponent",false,result.tree,result.tree_reference)||
       !weaponhand::derives(r,result.tree.address,L"AnimTree",L"Engine"))return false;
    weaponhand::BoolField list_field{},index_field{};ListLayout layout{};
    if(!weaponhand::field(r,result.tree.address,L"SkelControlLists",L"ArrayProperty",16,list_field)||
       !field_owner(r,list_field,L"AnimTree")||!troika_reflection::retain(r,list_field.property)||!list_layout(r,list_field.property,layout)||
       !r.at(result.tree.address,list_field.property.offset,result.lists)||
       !bounds(result.lists,layout.reflected.layout.extent,maximum_lists,true)||
       !control_axes::identity(r,layout.reflected.address,result.list_type))return false;
    result.stride=layout.reflected.layout.extent;
    result.head_reference=layout.head_reference;
    uptr inner_address{};metadata::Property inner{};
    if(!weaponhand::field(r,mesh.address,L"SkelControlIndex",L"ArrayProperty",16,index_field)||
       !field_owner(r,index_field,L"SkeletalMeshComponent")||!troika_reflection::retain(r,index_field.property)||!r.at(mesh.address,index_field.property.offset,result.indices)||
       !bounds(result.indices,1,512,true)||result.indices.count!=troika_bones||
       !r.at(r.result.identities[index_field.property.identity].address,0xd0,inner_address)||!r.property(inner_address,inner)||
       r.result.identities[inner.identity].outer!=r.result.identities[index_field.property.identity].address||
       !r.named(inner.identity,L"SkelControlIndex")||inner.offset||inner.dimension!=1||inner.element_bytes!=1||
       !weaponhand::core_type(r,inner.type,L"ByteProperty")||!troika_reflection::retain(r,inner)||
       !metadata::read_bytes(result.indices.data,0,result.index_values.data(),result.index_values.size()))return false;
    std::array<unsigned,2> matched{};
    for(unsigned i=0;i<unsigned(result.lists.count);++i){
        auto const address=result.lists.data+std::size_t(i)*result.stride;auto& row=result.rows[i];
        metadata::NameText text{};
        if(!r.at(address,layout.offsets[0],row.bone)||row.bone.number||!metadata::fresh_name(r.result.names,row.bone.index,text)||
           !r.at(address,layout.offsets[1],row.head)||!r.at(address,layout.offsets[2],row.draw_y))return false;
        for(unsigned j=0;j<i;++j)if(result.rows[j].bone.index==row.bone.index)return false;
        for(auto node=row.head;node;){
            if(result.node_count==result.nodes.size())return false;
            for(unsigned j=0;j<result.node_count;++j)if(result.nodes[j].object.address==node)return false;
            auto& record=result.nodes[result.node_count++];hand::ObjectIdentity next{};
            if(!control_axes::identity(r,node,record.object)||!weaponhand::derives(r,node,L"SkelControlBase",L"Engine")||
               !control_axes::class_derives(r,record.object.klass,result.head_reference.address)||
               !control_axes::pointer(r,node,L"SkelComponent",true,record.component,record.component_reference)||!hand::same(record.component,mesh)||
               !link(r,node,L"NextControl",L"SkelControlBase",L"SkelControlBase",true,next,record.next_reference))return false;
            record.next=next.address;
            for(unsigned role=0;role<2;++role)if(hand::same(record.object,controls.controls[role].object)){
                if(++matched[role]!=1)return false;
                if(metadata::exact_text(text,L"b_troika_gun_latitude"))result.bone_index[role]=1;
                else if(metadata::exact_text(text,L"b_troika_gun_longitude"))result.bone_index[role]=2;
                else return false;
                result.list_index[role]=i;
            }
            node=record.next;
        }
    }
    if(matched[0]!=1||matched[1]!=1||result.list_index[0]==result.list_index[1]||result.bone_index[0]==result.bone_index[1])return false;
    for(unsigned bone=0;bone<troika_bones;++bone){const auto list=result.index_values[bone];
        if(list!=255&&list>=result.lists.count)return false;
        for(unsigned role=0;role<2;++role)
            if((bone==result.bone_index[role])!=(list==result.list_index[role]))return false;
    }
    for(unsigned role=0;role<2;++role){hand::ObjectIdentity value{},declaration{};
        if(!(role?control_axes::pointer(r,turret.address,L"Pivot_Longitude",false,value,declaration):
                  control_axes::pointer(r,turret.address,L"Pivot_Latitude",false,value,declaration))||
           !hand::same(value,controls.controls[role].object)||!hand::same(declaration,controls.controls[role].object_reference))return false;
    }
    if(!troika_reflection::current(r,layout.reflected,L"SkelControlListHead",L"AnimTree",L"Engine"))return false;
    output=result;return true;
}
template<class Current>bool read(hand::ObjectIdentity const& turret,hand::ObjectIdentity const& mesh,
    control_axes::Snapshot const& controls,Current&& current,Snapshot& output) noexcept {
    try{
        if(!current()||controls.controls[0].object.address==controls.controls[1].object.address)return false;
        metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::final_check};
        if(!metadata::tables(base,schema.objects,schema.names)||!weaponhand::object(r,turret)||!weaponhand::object(r,mesh)||
           !weaponhand::derives(r,turret.address,L"Turret_TroikaCabal_base",L"WarfareGame")||
           !weaponhand::derives(r,mesh.address,L"SkeletalMeshComponent",L"Engine"))return false;
        Snapshot first{},closing{};
        if(!sample(r,turret,mesh,controls,first)||!control_axes::class_children(r)||!socket_contract::retained_metadata(r)||!current()||
           !sample(r,turret,mesh,controls,closing)||std::memcmp(&first,&closing,sizeof first)||
           !control_axes::class_children(r)||!socket_contract::retained_metadata(r)||!current())return false;
        output=first;return true;
    }catch(...){return false;}
}
} // namespace control_binding
