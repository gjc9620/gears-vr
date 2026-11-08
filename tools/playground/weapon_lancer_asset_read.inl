// Include after weapon_hand_client.inl. Engine-free metadata evidence only;
// caller separately validates registration, age, held/animation state and the
// current right-hand attachment. No cached Frame, mount or writer permission.
namespace lancermain_asset {
namespace meta=weaponhand::meta;
using U32=std::uint32_t;
enum class Status:U32 {ready,wrong_thread,invalid_identity,unsupported_weapon,invalid_link,
    invalid_component,hidden_component,unsupported_asset,changed};
struct Evidence {gears_weapon_hand::ObjectIdentity component{},asset{};};
enum class AssetScope { lancer_only,basic_hold,supported_instant_fire,frag_hold };
// Basic visual holding is separate from Lancer muzzle/reload eligibility.
// Never accept an arbitrary firearm/mesh pair under the broader scope.
enum class HeldAsset {unknown,lancer,pistol,hammerburst,frag};
HeldAsset held_asset_name(meta::Reader& r,uptr weapon,AssetScope scope) noexcept {
    // Frag is deliberately outside firearm/muzzle eligibility. This explicit
    // scope is for its separate original-projectile adapter only.
    if(scope==AssetScope::frag_hold)return weaponhand::derives(r,weapon,L"Weap_FragGrenade",L"WarfareGame")?
        HeldAsset::frag:HeldAsset::unknown;
    if(weaponhand::derives(r,weapon,L"Weap_AssaultRifle",L"WarfareGame"))return HeldAsset::lancer;
    if(scope==AssetScope::lancer_only)return HeldAsset::unknown;
    if(weaponhand::derives(r,weapon,L"Weap_COGPistol",L"WarfareGame"))return HeldAsset::pistol;
    if(weaponhand::derives(r,weapon,L"Weap_LocustAssaultRifle",L"WarfareGame"))return HeldAsset::hammerburst;
    return HeldAsset::unknown;
}
bool held_asset_matches(meta::Reader& r,U32 slot,HeldAsset kind) noexcept {
    switch(kind){
    case HeldAsset::lancer:return r.named(slot,L"COGAssaultRifle");
    case HeldAsset::pistol:return r.named(slot,L"COG_Pistol");
    case HeldAsset::hammerburst:return r.named(slot,L"Locust_Hammerburst");
    case HeldAsset::frag:{
        if(!r.named(slot,L"Frag_Grenade"))return false;
        const auto outer=r.result.identities[slot].outer;U32 package{};
        return outer&&r.path(outer,package)&&r.named(package,L"COG_Bolo_Grenade")&&
            weaponhand::derives(r,outer,L"Package",L"Core");
    }
    default:return false;
    }
}
bool identify(meta::Reader& r,uptr address,gears_weapon_hand::ObjectIdentity& out) noexcept {
    U32 slot{};if(!address||!r.path(address,slot))return false;
    const auto& id=r.result.identities[slot];
    const gears_weapon_hand::ObjectIdentity value{id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};
    if(!weaponhand::object(r,value))return false;out=value;return true;
}
bool mesh_pointer(meta::Reader& r,uptr weapon,meta::PointerField& out) noexcept {
    // Preserve the single-field API for independent existing consumers.
    const bool found=weaponhand::read_pointer(r,weapon,L"Mesh",out,true)||
        (r.result.failure.reason==meta::Reason::none&&weaponhand::read_pointer(r,weapon,L"Mesh",out));
    return found&&r.result.failure.reason==meta::Reason::none;
}
// Like AimObserveFields, this owns only one synchronous read query. Resolve
// the two fields of each object together; resolve them AGAIN at finish so an
// inserted earlier same-name descriptor cannot hide behind a cached address.
class AssetObserveFields {
    using Field=weaponhand::BoolField;
    bool component_{},ready_{};uptr object_{},class_{};
    std::array<Field,2> fields_{};
    U32 label(meta::NameText const& text) const noexcept {
        if(component_){
            if(meta::exact_text(text,L"SkeletalMesh"))return 0;
            if(meta::exact_text(text,L"HiddenGame"))return 1;
        }else{
            if(meta::exact_text(text,L"Instigator"))return 0;
            if(meta::exact_text(text,L"Mesh"))return 1;
        }
        return 2;
    }
    bool type(meta::Reader& r,U32 slot,U32 index) const noexcept {
        if(component_&&index==1)return weaponhand::core_type(r,slot,L"BoolProperty");
        if(!component_&&index==1){
            // The first Mesh descriptor may have either original Core type;
            // an unreadable/mutating first attempt is never a fallback.
            if(weaponhand::core_type(r,slot,L"ComponentProperty"))return true;
            if(r.result.failure.reason!=meta::Reason::none)return false;
        }
        return weaponhand::core_type(r,slot,L"ObjectProperty");
    }
    bool resolve(meta::Reader& r,uptr object,std::array<Field,2>& out,uptr& object_class) const noexcept {
        U32 slot{};if(!r.path(object,slot))return false;
        const auto first_class=r.result.identities[slot].klass;
        auto cls=first_class;U32 depth{},extent{},remaining=2;
        std::array<Field,2> found{};std::array<uptr,meta::max_parents> parents{};
        while(cls){
            if(depth==parents.size())return false;
            for(U32 i=0;i<depth;++i)if(parents[i]==cls)return false;
            parents[depth++]=cls;meta::Structure s{};if(!r.structure(cls,s))return false;
            if(depth==1)extent=s.extent;
            auto address=s.children;std::array<uptr,meta::max_search> seen{};U32 searched{};
            weapon_metadata_parse::Walk traversal;
            while(address){
                if(searched==seen.size()||traversal.seen(address,seen.data(),searched))return false;
                seen[searched++]=address;meta::Identity id{};meta::NameText text{};
                if(!meta::raw_identity(r.result.objects,r.result.names,address,id,text))return false;
                const auto index=label(text);
                if(index<2&&!found[index].property.element_bytes){
                    Field value{};value.object=object;value.owner=s.identity;
                    const U32 bytes=component_&&index==1?4u:8u;
                    if(id.name.number||!r.property(address,value.property)||
                       r.result.identities[value.property.identity].outer!=cls||!type(r,value.property.type,index)||
                       value.property.dimension!=1||value.property.element_bytes!=bytes||
                       !meta::property_fits(value.property,s.extent)||!meta::property_fits(value.property,extent))return false;
                    found[index]=value;if(!--remaining){out=found;object_class=first_class;return true;}
                }
                if(!r.at(address,0x68,address))return false;
            }
            cls=s.super;
        }
        return false;
    }
    bool current(meta::Reader& r,U32 index) const noexcept {
        if(!ready_||index>=2)return false;
        U32 slot{};meta::Structure owner{};meta::Property fresh{};auto const& field=fields_[index];
        return r.path(object_,slot)&&r.result.identities[slot].klass==class_&&
            r.structure(r.result.identities[field.owner].address,owner)&&owner.identity==field.owner&&
            r.property(r.result.identities[field.property.identity].address,fresh)&&meta::same_property(fresh,field.property);
    }
public:
    explicit AssetObserveFields(bool component) noexcept:component_(component){}
    bool begin(meta::Reader& r,uptr object) noexcept {
        ready_=false;std::array<Field,2> fields{};uptr cls{};
        if(!resolve(r,object,fields,cls))return false;
        object_=object;class_=cls;fields_=fields;ready_=true;return true;
    }
    bool pointer(meta::Reader& r,U32 index,meta::PointerField& out) const noexcept {
        if((component_&&index==1)||!current(r,index))return false;
        auto const& field=fields_[index];meta::PointerField value{};
        if(!r.at(object_,field.property.offset,value.value))return false;
        value.property=field.property;value.owner=field.owner;out=value;return true;
    }
    bool hidden(meta::Reader& r,Field& out) const noexcept {
        if(!component_||!current(r,1))return false;
        auto value=fields_[1];
        if(!r.at(r.result.identities[value.property.identity].address,0xd0,value.mask)||!value.mask||(value.mask&(value.mask-1))||
           !r.at(object_,value.property.offset,value.bits))return false;
        out=value;return true;
    }
    bool finish(meta::Reader& r) noexcept {
        if(!ready_)return false;ready_=false;std::array<Field,2> fresh{};uptr cls{};
        if(!resolve(r,object_,fresh,cls)||cls!=class_)return false;
        for(U32 i=0;i<2;++i)if(fresh[i].object!=fields_[i].object||fresh[i].owner!=fields_[i].owner||
            !meta::same_property(fresh[i].property,fields_[i].property))return false;
        return true;
    }
};
Status read(gears_weapon_hand::Frame const& f,Evidence& out,AssetScope scope=AssetScope::lancer_only) noexcept {
    const auto gt=weaponhand::current_gt(nullptr);
    if(!gt||(f.key.thread_id&&gt!=f.key.thread_id))return Status::wrong_thread;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    // One read-only discovery, as in the binding observer. Reuse only the
    // identities already read into THIS local Snapshot while walking its
    // types/fields; retain no metadata or eligibility across queries. Disable
    // before the original fresh selector, identity, layout and value closure.
    r.reuse_initial_identities=true;
    if(!gears_weapon_hand::identity_valid(f.weapon)||!gears_weapon_hand::identity_valid(f.mesh)||
       !meta::tables(schema.module_base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||
       !weaponhand::object(r,f.mesh))return Status::invalid_identity;
    const auto expected_asset=held_asset_name(r,f.weapon.address,scope);
    if(expected_asset==HeldAsset::unknown)return Status::unsupported_weapon;
    AssetObserveFields weapon_fields(false),component_fields(true);
    std::array<meta::PointerField,3> links{};weaponhand::BoolField hidden{};Evidence candidate{};
    if(!f.pawn.address||!weapon_fields.begin(r,f.weapon.address)||!weapon_fields.pointer(r,0,links[0])||links[0].value!=f.pawn.address||
       !weapon_fields.pointer(r,1,links[1])||!links[1].value)return Status::invalid_link;
    if(!identify(r,links[1].value,candidate.component)||
       !weaponhand::derives(r,links[1].value,L"SkeletalMeshComponent",L"Engine")||
       !component_fields.begin(r,links[1].value)||!component_fields.hidden(r,hidden))return Status::invalid_component;
    if(hidden.bits&hidden.mask)return Status::hidden_component;
    if(!component_fields.pointer(r,0,links[2])||
       !identify(r,links[2].value,candidate.asset)||
       !weaponhand::derives(r,links[2].value,L"SkeletalMesh",L"Engine"))return Status::unsupported_asset;
    U32 asset_slot{};
    if(!r.path(candidate.asset.address,asset_slot)||!held_asset_matches(r,asset_slot,expected_asset))return Status::unsupported_asset;
    r.reuse_initial_identities=false;
    // Repeat actual selectors too: a newly inserted earlier same-name field
    // must not leave the old selected descriptor looking valid in isolation.
    std::array<meta::PointerField,3> fresh_links{};weaponhand::BoolField fresh_hidden{};
    if(!weapon_fields.pointer(r,0,fresh_links[0])||!weapon_fields.pointer(r,1,fresh_links[1])||
       !component_fields.pointer(r,0,fresh_links[2])||!component_fields.hidden(r,fresh_hidden)||
       !weapon_fields.finish(r)||!component_fields.finish(r))return Status::changed;
    for(U32 i=0;i<links.size();++i)if(!meta::same_pointer(links[i],fresh_links[i]))return Status::changed;
    if(hidden.object!=fresh_hidden.object||hidden.owner!=fresh_hidden.owner||hidden.mask!=fresh_hidden.mask||
       hidden.bits!=fresh_hidden.bits||!meta::same_property(hidden.property,fresh_hidden.property))return Status::changed;
    const auto identities=schema.identity_count,structures=schema.structure_count;
    for(U32 i=0;i<identities;++i){U32 slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return Status::changed;}
    for(U32 i=0;i<structures;++i){meta::Structure fresh{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,fresh)||
           !meta::same_structure(schema.structures[i],fresh))return Status::changed;}
    const std::array<uptr,3> objects{f.weapon.address,f.weapon.address,candidate.component.address};
    for(U32 i=0;i<links.size();++i){meta::Property fresh{};uptr pointer{};
        if(!r.property(schema.identities[links[i].property.identity].address,fresh)||
           !meta::same_property(links[i].property,fresh)||!r.at(objects[i],links[i].property.offset,pointer)||
           pointer!=links[i].value)return Status::changed;}
    meta::Property property{};U32 mask{},bits{};
    const auto hidden_property=schema.identities[hidden.property.identity].address;
    if(!r.property(hidden_property,property)||!meta::same_property(hidden.property,property)||
       !r.at(hidden_property,0xd0,mask)||mask!=hidden.mask||
       !r.at(hidden.object,hidden.property.offset,bits)||bits!=hidden.bits)return Status::changed;
    meta::Table objects_table{},names_table{};
    if(!meta::tables(schema.module_base,objects_table,names_table)||!meta::same_table(objects_table,schema.objects)||
       !meta::same_table(names_table,schema.names)||base!=schema.module_base||weaponhand::current_gt(nullptr)!=gt||
       r.result.failure.reason!=meta::Reason::none)return Status::changed;
    out=candidate;return Status::ready;
}
} // namespace lancermain_asset
