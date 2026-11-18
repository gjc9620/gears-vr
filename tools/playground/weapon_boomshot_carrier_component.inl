// Boomshot-only component lease for the original MagazineMesh0 StaticMesh.
// It owns no clone, attachment, hand, inventory or bone authoring.
namespace boomshot_carrier_component {
namespace hand=gears_weapon_hand;namespace lease=gears_weapon;
struct Key {
    hand::ObjectIdentity world{},pawn{},weapon{},component{},asset{};
    std::uint64_t serial{},hold_generation{},generation{};
    bool operator==(Key const& other) const noexcept {
        return hand::same(world,other.world)&&hand::same(pawn,other.pawn)&&
            hand::same(weapon,other.weapon)&&hand::same(component,other.component)&&
            hand::same(asset,other.asset)&&serial==other.serial&&
            hold_generation==other.hold_generation&&generation==other.generation;
    }
};
using Value=lease::WeaponMeshComponentValue;
using State=lease::WeaponMeshComponentState<Key>;
using Absolute=lease::MeshComponentAbsolute;
using Bytes=lease::MeshComponentBytes12;

inline bool source_current(Key const& key,hand::Frame const& f) noexcept {
    return key.serial&&key.generation&&key.hold_generation&&
        hand::same(key.world,f.world)&&hand::same(key.pawn,f.pawn)&&
        hand::same(key.weapon,f.weapon)&&key.hold_generation==vrequip::model.hold_generation()&&
        vrequip::model.held_for(key.world.address,key.pawn.address,key.weapon.address)&&
        weaponhand::current_gt(nullptr)&&nativeequipment::current(nullptr,f);
}

inline bool object_identity(meta::Reader& r,hand::ObjectIdentity const& identity) noexcept {
    return weaponhand::object(r,identity);
}

class Backend {
    Key key_{};
public:
    explicit Backend(Key const& key) noexcept:key_(key){}
    bool current() const noexcept {
        if(!weaponhand::current_gt(nullptr))return false;
        hand::ObjectIdentity actual{};
        for(auto const& expected:{key_.world,key_.pawn,key_.weapon,key_.component,key_.asset})
            if(!nativeequipment::identify(nullptr,expected.address,actual)||!hand::same(actual,expected))return false;
        meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
        meta::PointerField component{},asset{};
        return meta::tables(base,schema.objects,schema.names)&&
            weaponhand::pointer(reader,key_.weapon.address,L"MagazineMesh",key_.component.address,component,true)&&
            weaponhand::pointer(reader,key_.component.address,L"StaticMesh",key_.asset.address,asset)&&
            playground::pointer(key_.weapon.address,L"Instigator")==key_.pawn.address&&
            playground::pointer(key_.pawn.address,L"WorldInfo")==key_.world.address;
    }
    bool read(State& out) noexcept {
        out={};meta::Snapshot snapshot{};snapshot.module_base=base;
        meta::Reader r{snapshot,meta::Stage::final_check};
        if(!current()||!meta::tables(base,snapshot.objects,snapshot.names)||
           !object_identity(r,key_.world)||!object_identity(r,key_.pawn)||
           !object_identity(r,key_.weapon)||!object_identity(r,key_.component)||
           !object_identity(r,key_.asset))return false;
        hand::ObjectIdentity actual_component{},actual_asset{};
        if(!nativeequipment::identify(nullptr,key_.component.address,actual_component)||
           !hand::same(actual_component,key_.component)||
           !nativeequipment::identify(nullptr,key_.asset.address,actual_asset)||
           !hand::same(actual_asset,key_.asset))return false;
        Value value{};
        weaponhand::BoolField translation{},rotation{};
        if(!weaponhand::field(r,key_.component.address,L"Translation",L"StructProperty",12,translation)||
           !r.at(key_.component.address,translation.property.offset,value.translation)||
           !weaponhand::field(r,key_.component.address,L"Rotation",L"StructProperty",12,rotation)||
           !r.at(key_.component.address,rotation.property.offset,value.rotation))return false;
        weaponhand::BoolField flags[3]{};
        if(!weaponhand::boolean(r,key_.component.address,L"AbsoluteTranslation",flags[0])||
           !weaponhand::boolean(r,key_.component.address,L"AbsoluteRotation",flags[1])||
           !weaponhand::boolean(r,key_.component.address,L"AbsoluteScale",flags[2]))return false;
        for(unsigned i=0;i<3;++i)value.absolute[i]=(flags[i].bits&flags[i].mask)!=0;
        out={key_,value,true};return true;
    }
    bool set_absolute(Key const& wanted,Value const& expected,Absolute const& absolute) noexcept {
        State before{};if(!(wanted==key_)||!read(before)||!lease::mesh_component_exact(before.value,expected))return false;
        playground::Args call(key_.component.address,L"SetAbsolute");
        if(!call.valid||call.r.size!=12)return false;
        const bool t=nativelancerfollow::bool_arg(call,L"NewAbsoluteTranslation",absolute[0]);
        const bool r=nativelancerfollow::bool_arg(call,L"NewAbsoluteRotation",absolute[1]);
        const bool s=nativelancerfollow::bool_arg(call,L"NewAbsoluteScale",absolute[2]);
        return t&&r&&s&&call.run();
    }
    bool set_translation(Key const& wanted,Value const& expected,Bytes const& bytes) noexcept {
        State before{};if(!(wanted==key_)||!read(before)||!lease::mesh_component_exact(before.value,expected))return false;
        playground::Args call(key_.component.address,L"SetTranslation");const auto p=call.prop(L"NewTranslation");
        return call.valid&&p&&get<unsigned>(p+0x74)==12&&
            playground::named(get<uptr>(p+0x50),L"StructProperty")&&
            call.bytes(L"NewTranslation",bytes.data(),12)&&call.run();
    }
    bool set_rotation(Key const& wanted,Value const& expected,Bytes const& bytes) noexcept {
        State before{};if(!(wanted==key_)||!read(before)||!lease::mesh_component_exact(before.value,expected))return false;
        playground::Args call(key_.component.address,L"SetRotation");const auto p=call.prop(L"NewRotation");
        return call.valid&&p&&get<unsigned>(p+0x74)==12&&
            playground::named(get<uptr>(p+0x50),L"StructProperty")&&
            call.bytes(L"NewRotation",bytes.data(),12)&&call.run();
    }
};
}
