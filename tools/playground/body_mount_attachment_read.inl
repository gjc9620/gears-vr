// Identity-only metadata carrier; original V1 logic and entry remain unchanged.
namespace nativeequipment {
bool read_attached_equipment(abi::EquipmentIdentity const& frame,uptr expected_mesh) noexcept {
    try{
        if(!frame.weapon.address)return false;
        const auto mesh=playground::pointer(frame.weapon.address,L"Mesh");
        if(expected_mesh&&mesh!=expected_mesh)return false;
        if(!playground::instance(mesh,L"SkeletalMeshComponent")||playground::deleted(mesh)||
           playground::pointer(frame.weapon.address,L"Instigator")!=frame.pawn.address)return false;
        weaponhand::meta::Snapshot schema{};schema.module_base=base;
        weaponhand::meta::Reader reader{schema,weaponhand::meta::Stage::final_check};weaponhand::BoolField hidden{};
        if(!weaponhand::meta::tables(base,schema.objects,schema.names)||
           !weaponhand::boolean(reader,mesh,L"HiddenGame",hidden)||(hidden.bits&hidden.mask))return false;
        const auto property=playground::property(frame.mesh.address,L"Attachments");
        if(!property||!playground::named(get<uptr>(property+0x50),L"ArrayProperty")||
           get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=sizeof(Array))return false;
        const auto offset=get<unsigned>(property+0x94),extent=get<unsigned>(get<uptr>(frame.mesh.address+0x50)+0x88);
        if(!extent||offset>extent||sizeof(Array)>extent-offset)return false;
        Array entries{};
        if(!copy(&entries,frame.mesh.address+offset,sizeof entries)||
           entries.count<0||entries.count>entries.capacity||entries.capacity>128||(entries.count&&!entries.data))return false;
        const auto inner=get<uptr>(property+0xd0),structure=get<uptr>(inner+0xd0);
        if(!registered(inner)||!registered(structure)||
           !playground::named(get<uptr>(inner+0x50),L"StructProperty")||get<unsigned>(inner+0x74)!=52||get<unsigned>(structure+0x88)!=52)return false;
        const wchar_t* labels[]={L"Component",L"BoneName",L"RelativeLocation",L"RelativeRotation",L"RelativeScale"};
        const wchar_t* kinds[]={L"ComponentProperty",L"NameProperty",L"StructProperty",L"StructProperty",L"StructProperty"};
        const unsigned offsets[]={0,8,16,28,40},sizes[]={8,8,12,12,12};
        for(unsigned i=0;i<5;++i){const auto field=playground::field(structure,labels[i],kinds[i]);
            if(!registered(field)||get<unsigned>(field+0x70)!=1||get<unsigned>(field+0x74)!=sizes[i]||get<unsigned>(field+0x94)!=offsets[i])return false;}
        unsigned matches{};uptr entry_address{};std::array<unsigned char,52> selected{};
        for(int i=0;i<entries.count;++i){std::array<unsigned char,52> bytes{};uptr component{};
            const auto address=entries.data+size_t(i)*52;
            if(!copy(bytes.data(),address,bytes.size()))return false;std::memcpy(&component,bytes.data(),8);
            if(component==mesh){selected=bytes;entry_address=address;++matches;}}
        if(matches!=1)return false;
        Name bone{};std::memcpy(&bone,selected.data()+8,8);if(!hand_bone(bone))return false;
        for(unsigned vector_offset:{16u,40u})for(unsigned axis=0;axis<3;++axis){float value{};
            std::memcpy(&value,selected.data()+vector_offset+axis*4,4);if(!std::isfinite(value))return false;}
        Array after{};std::array<unsigned char,52> again{};
        return copy(&after,frame.mesh.address+offset,sizeof after)&&
            !std::memcmp(&after,&entries,sizeof after)&&copy(again.data(),entry_address,again.size())&&again==selected&&
            playground::pointer(frame.weapon.address,L"Mesh")==mesh;
    }catch(...){return false;}
}
}
