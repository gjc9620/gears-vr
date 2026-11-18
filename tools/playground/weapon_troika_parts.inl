// Ordinary GT read of the ORIGINAL world turret, independent from hand-held
// weapon attachment/mount helpers. Source stays the unmodified free-hand Frame.
namespace troika_parts {
namespace hand=gears_weapon_hand;namespace meta=weaponhand::meta;
struct Parts {
    hand::FrameKey key{};std::int64_t input_sync_ns{};
    hand::ObjectIdentity turret{},component{},asset{};
    std::array<float,16> handleroot{};
};
gears_equipment_names::Lookup names({L"b_troika_handleroot",L"b_troika_handle_L",L"b_troika_handle_R"});
bool read(hand::Frame const& f,hand::ObjectIdentity const& turret,Parts& out) noexcept {
    out={};if(!nativeequipment::current(nullptr,f)||!troika_original::same_object(turret)||
       !troika_original::link(turret.address,L"WorldInfo",f.world.address))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    Parts result{};result.turret=turret;result.key=f.key;result.input_sync_ns=f.controllers.sync_ns;
    weaponhand::BoolField hidden{},deleted{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,turret)||
       !weaponhand::derives(reader,turret.address,L"Turret_TroikaCabal_base",L"WarfareGame")||
       !weaponhand::boolean(reader,turret.address,L"bDeleteMe",deleted)||(deleted.bits&deleted.mask)||
       !weaponhand::boolean(reader,turret.address,L"bHidden",hidden)||(hidden.bits&hidden.mask)||
       !troika_mounted_context::pointer(reader,turret.address,L"Mesh",result.component,true)||
       !weaponhand::derives(reader,result.component.address,L"SkeletalMeshComponent",L"Engine")||
       !weaponhand::boolean(reader,result.component.address,L"HiddenGame",hidden)||(hidden.bits&hidden.mask)||
       !troika_mounted_context::pointer(reader,result.component.address,L"SkeletalMesh",result.asset)||
       !weaponhand::derives(reader,result.asset.address,L"SkeletalMesh",L"Engine"))return false;
    unsigned asset_slot{},package_slot{};
    if(!reader.path(result.asset.address,asset_slot)||!reader.named(asset_slot,L"TroikaTurret")||
       !reader.path(reader.result.identities[asset_slot].outer,package_slot)||!reader.named(package_slot,L"Geist_TroikaCabal")||
       reader.result.identities[package_slot].outer||
       !weaponhand::derives(reader,reader.result.identities[package_slot].address,L"Package",L"Core"))return false;
    std::array<unsigned,3> indices{};const auto started=weaponhand::clock_ns(nullptr);
    if(names.step([](gears_equipment_names::Table& t){const auto a=get<Array>(base+0x1c34800);
            t={a.data,unsigned(a.count),unsigned(a.capacity)};return a.count>=0&&a.capacity>=0;},
        [](gears_equipment_names::Table t,unsigned index,std::array<wchar_t,32>& text){
            const auto entry=get<uptr>(t.data+std::size_t(index)*8);unsigned observed_name_index{};
            return entry&&copy(&observed_name_index,entry,4)&&observed_name_index==index&&copy(text.data(),entry+20,sizeof(text));},
        [&](){return nativeequipment::current(nullptr,f)&&troika_original::same_object(turret);},
        [&](){const auto now=weaponhand::clock_ns(nullptr);return now>=started&&now-started<1000000;},indices,2048)
            !=gears_equipment_names::State::ready)return false;
    constexpr const wchar_t* labels[]={L"b_troika_handleroot",L"b_troika_handle_L",L"b_troika_handle_R"};
    for(unsigned i=0;i<3;++i){
        meta::NameText text{};
        if(!meta::fresh_name(schema.names,indices[i],text)||text.length!=std::wcslen(labels[i])||
           std::wmemcmp(text.units.data(),labels[i],text.length+1))return false;
        playground::Args match(result.component.address,L"MatchRefBone");
        if(!hammerburst_animation::component_getter(match,L"MatchRefBone")||
           !hammerburst_animation::input_field(match,match.prop(L"BoneName"),L"NameProperty",8)||
           !vrequip::result_property(match,L"IntProperty",4)||
           !(get<unsigned>(match.prop(L"BoneName")+0x94)+8<=get<unsigned>(match.prop(L"ReturnValue")+0x94)||
             get<unsigned>(match.prop(L"ReturnValue")+0x94)+4<=get<unsigned>(match.prop(L"BoneName")+0x94))||
           !nativeequipment::current(nullptr,f)||
           !match.set(L"BoneName",Name{indices[i],0}).run()||match.output<int>()!=int(9+i))return false;
    }
    playground::Args matrix(result.component.address,L"GetBoneMatrix");
    if(!hammerburst_animation::component_getter(matrix,L"GetBoneMatrix")||
       !hammerburst_animation::input_field(matrix,matrix.prop(L"BoneIndex"),L"IntProperty",4)||
       !vrequip::result_property(matrix,L"StructProperty",64))return false;
    const auto property=matrix.prop(L"ReturnValue"),structure=get<uptr>(property+0xd0);
    const auto a=get<unsigned>(matrix.prop(L"BoneIndex")+0x94),b=get<unsigned>(property+0x94);
    if(!registered(structure)||!playground::named(structure,L"Matrix")||get<unsigned>(structure+0x88)!=64||
       !(a+4<=b||b+64<=a)||!nativeequipment::current(nullptr,f)||
       !matrix.set(L"BoneIndex",int(9)).run())return false;
    result.handleroot=matrix.output<std::array<float,16>>();
    gears_vr::Vector local{};double scale{};
    if(!gears_weapon::troika_handle_geometry::local_point(result.handleroot,
          {result.handleroot[12],result.handleroot[13],result.handleroot[14]},local,scale)||
       !troika_original::link(turret.address,L"Mesh",result.component.address,true)||
       !troika_original::link(result.component.address,L"SkeletalMesh",result.asset.address)||
       !unified_weapon_fire::retained_metadata(reader)||!nativeequipment::current(nullptr,f))return false;
    out=result;return true;
}
} // namespace troika_parts
