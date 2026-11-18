// Original Boomshot overrides WarWeapon's selected-node getter. The saved
// script reads AnimTree(Mesh.Animations).Children[0], not the Lancer override.
struct NodeGetterLayout {
    uptr function{},exec{},result_class{};unsigned bytes{},flags{},result{};
    std::array<uptr,2> local_classes{};std::array<unsigned,2> local_offsets{};
    bool operator==(NodeGetterLayout const& b) const noexcept {
        return function==b.function&&exec==b.exec&&result_class==b.result_class&&bytes==b.bytes&&flags==b.flags&&result==b.result&&
            local_classes==b.local_classes&&local_offsets==b.local_offsets;
    }
};
template<class Call>bool node_getter_schema(Call const& call,uptr receiver,meta::Reader& r,NodeGetterLayout& out) noexcept {
    out={};unsigned fn{},kind{},owner_index{},package{},flags{};std::uint16_t bytes{},offset{},native{};uptr exec{};
    meta::Structure structure{};
    if(!call.valid||call.r.size!=8||!r.path(call.r.function,fn)||!r.named(fn,L"GetWeaponAnimNodeSeq")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner_index)||!r.named(owner_index,L"Weap_Boomer_base")||
       !r.path(r.result.identities[owner_index].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
       !r.path(r.result.identities[owner_index].outer,package)||!r.named(package,L"WarfareGame")||r.result.identities[package].outer||
       !unified_weapon_fire::owner_in_receiver(r,receiver,r.result.identities[owner_index].address)||
       !r.at(call.r.function,0xf4,flags)||(flags&0x400)||!r.at(call.r.function,0xf8,native)||native||
       !r.at(call.r.function,0x114,exec)||!weaponhand::executable(reinterpret_cast<FARPROC>(exec),GetModuleHandleW(nullptr))||
       !r.at(call.r.function,0x106,bytes)||bytes!=8||!r.at(call.r.function,0x108,offset)||offset||
       !r.structure(call.r.function,structure)||bytes>structure.extent)return false;
    NodeGetterLayout value{};value.function=call.r.function;value.exec=exec;value.bytes=bytes;value.flags=flags;
    unsigned returns{},count{},locals{};std::array<uptr,meta::max_fields> visited{};
    for(auto child=structure.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;meta::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=call.r.function||p.dimension!=1||
           !meta::property_fits(p,structure.extent))return false;
        bool retained=false;
        for(unsigned i=0;i<r.result.field_count;++i)if(r.result.fields[i].identity==p.identity){
            if(!meta::same_property(r.result.fields[i],p))return false;retained=true;break;}
        if(!retained){if(r.result.field_count==r.result.fields.size())return false;r.result.fields[r.result.field_count++]=p;}
        if(p.flags&0x80){
            unsigned cls{},cls_kind{},cls_package{};uptr result_class{};
            if(++returns!=1||!r.named(p.identity,L"ReturnValue")||(p.flags&0x580)!=0x480||
               p.offset||p.element_bytes!=8||!meta::property_fits(p,bytes)||!weaponhand::core_type(r,p.type,L"ObjectProperty")||
               !r.at(child,0xd0,result_class)||!r.path(result_class,cls)||!r.named(cls,L"AnimNodeSequence")||
               !r.path(r.result.identities[cls].klass,cls_kind)||!weaponhand::core_type(r,cls_kind,L"Class")||
               !r.path(r.result.identities[cls].outer,cls_package)||!r.named(cls_package,L"Engine")||
               r.result.identities[cls_package].outer)return false;
            value.result_class=result_class;value.result=p.offset;
        }else{
            unsigned index{},cls{},cls_kind{},cls_package{};uptr local_class{};
            if(p.flags&0x580||p.element_bytes!=8||p.offset<bytes)return false;
            if(r.named(p.identity,L"SkelMesh")&&weaponhand::core_type(r,p.type,L"ComponentProperty"))index=0;
            else if(r.named(p.identity,L"AnimSeq")&&weaponhand::core_type(r,p.type,L"ObjectProperty"))index=1;
            else return false;
            if(locals&(1u<<index))return false;locals|=1u<<index;
            if(!r.at(child,0xd0,local_class)||!r.path(local_class,cls)||
               !(index?r.named(cls,L"AnimNodeSequence"):r.named(cls,L"SkeletalMeshComponent"))||
               !r.path(r.result.identities[cls].klass,cls_kind)||!weaponhand::core_type(r,cls_kind,L"Class")||
               !r.path(r.result.identities[cls].outer,cls_package)||!r.named(cls_package,L"Engine")||
               r.result.identities[cls_package].outer)return false;
            value.local_classes[index]=local_class;value.local_offsets[index]=p.offset;
        }
        child=p.next;
    }
    if(returns!=1||locals!=3||
       !(value.local_offsets[0]+8<=value.local_offsets[1]||value.local_offsets[1]+8<=value.local_offsets[0])||
       !unified_weapon_fire::retained_metadata(r))return false;
    out=value;return true;
}
