// Read-only original appearance for the private arrow component. The common
// committed avatar reader supplies its current model/socket scale; no Lancer
// private reload owner, original component setter or cosmetic hand is reused.
namespace torquebow_arrow_appearance {
namespace hand=gears_weapon_hand;namespace source=torquebow_arrow_source;
namespace meta=gripdamage_metadata;
unsigned read_gate{},material_gate{};
struct Appearance {
    source::Binding binding{};source::Attributes attributes{};
    hand::ObjectIdentity material{};double world_scale{},units_per_metre{};
};
void material_rejection(playground::Args& call,bool writing=false) noexcept {
    static unsigned rows{};if(!log_file||rows>=2||!call.valid||!registered(call.r.function))return;
    ++rows;
    const auto fn=call.r.function;const auto index_field=call.prop(L"ElementIndex"),return_field=call.prop(writing?L"Material":L"ReturnValue");
    const auto emit=[&](wchar_t const* role,uptr object) noexcept {
        if(!registered(object))return;
        meta::Table objects{},names{};meta::NameText label{};std::array<wchar_t,128> text{};
        const auto kind=get<uptr>(object+0x50);
        if(registered(kind)&&meta::tables(base,objects,names)&&
           meta::fresh_name(names,get<Name>(kind+0x48).index,label)){
            const auto count=(std::min)(text.size()-1,std::size_t(label.length));
            std::copy_n(label.units.begin(),count,text.begin());
        }
        fwprintf(log_file,L"torquebow_material_field role=%ls object=%llx owner=%llx type=%ls dimension=%u bytes=%u offset=%u flags=%llx related=%llx observation_only=1\n",
            role,object,get<uptr>(object+0x40),text.data(),get<unsigned>(object+0x70),get<unsigned>(object+0x74),
            get<unsigned>(object+0x94),get<unsigned long long>(object+0x78),get<uptr>(object+0xd0));
    };
    const auto owner=get<uptr>(fn+0x40);
    fwprintf(log_file,L"torquebow_material_abi method=%ls function=%llx parameter_bytes=%u extent=%u return_offset=%u flags=%u native_index=%u exec=%llx owner=%llx owner_MeshComponent=%u native_contract=%u observation_only=1\n",
        writing?L"SetMaterial":L"GetMaterial",fn,call.r.size,get<unsigned>(fn+0x88),unsigned(get<unsigned short>(fn+0x108)),get<unsigned>(fn+0xf4),
        unsigned(get<unsigned short>(fn+0xf8)),get<uptr>(fn+0x114),owner,
        unsigned(registered(owner)&&playground::named(owner,L"MeshComponent")),
        unsigned(weapon_part_assets::native_function(call,writing?L"SetMaterial":L"GetMaterial",L"MeshComponent")));
    emit(L"index",index_field);emit(writing?L"material":L"return",return_field);fflush(log_file);
}
bool material(hand::Frame const& f,source::Binding const& binding,hand::ObjectIdentity& out) noexcept {
    out={};material_gate=1;
    if(!weapon_part_assets::object(binding.component)||!weapon_part_assets::object(binding.asset)||
       !nativeequipment::current(nullptr,f))return false;
    playground::Args call(binding.component.address,L"GetMaterial");
    material_gate=2;
    if(!weapon_part_assets::material_schema(call,false)){material_rejection(call);return false;}
    material_gate=4;if(!call.set(L"ElementIndex",0).run())return false;
    material_gate=5;if(!weapon_part_assets::material_schema(call,false)||!weapon_part_assets::object(binding.component)||
       !nativeequipment::current(nullptr,f))return false;
    hand::ObjectIdentity value{};
    material_gate=3;
    if(!nativeequipment::identify(nullptr,call.output<uptr>(),value)||
       !weapon_part_assets::object(value)||!playground::instance(value.address,L"MaterialInterface"))return false;
    out=value;material_gate=0;return true;
}
bool read(hand::Frame const& f,gears_committed_interaction::Frame const& avatar,Appearance& out) noexcept {
    out={};Appearance value{};source::Binding final_source{};hand::ObjectIdentity final_material{};
    const auto held=vrequip::model.hold_generation();
    const bool simulated=f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
    read_gate=1;if(!held||!vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return false;
    read_gate=2;if(!gears_committed_interaction::matches(avatar,f,held,weaponhand::clock_ns(nullptr),simulated))return false;
    read_gate=3;if(!nativeequipment::current(nullptr,f)||!source::read(f,value.binding))return false;
    read_gate=4;if(!material(f,value.binding,value.material))return false;
    read_gate=5;if(!source::attributes(f,value.binding,value.attributes))return false;
    // Original attachment is to this socket, which can add its own scale.
    read_gate=6;if(!gears_torque_bow_arrow_scale::world(value.attributes.scale,avatar.model_scale,value.attributes.socket.xyz,value.world_scale)||
       !material(f,value.binding,final_material)||!hand::same(value.material,final_material)||
       !source::read(f,final_source)||!source::same(value.binding,final_source)||
       !gears_committed_interaction::matches(avatar,f,held,weaponhand::clock_ns(nullptr),simulated)||
       held!=vrequip::model.hold_generation()||!nativeequipment::current(nullptr,f))return false;
    value.units_per_metre=avatar.right.units_per_metre;out=value;read_gate=0;return true;
}
bool same(Appearance const& a,Appearance const& b) noexcept {
    return source::same(a.binding,b.binding)&&source::same(a.attributes,b.attributes)&&
        hand::same(a.material,b.material)&&a.world_scale==b.world_scale&&a.units_per_metre==b.units_per_metre;
}
}
