// Separate query-local metadata operation. Never enters old Frame-current
// wrappers while Client's Gate is held. All old V1 readers stay unchanged.
namespace bodymountequipment {
namespace sender=gears_body_main_mount_sender;
namespace wire=gears_body_main_mount_provider;
namespace hand=gears_weapon_hand;
bool held_current(hand::EquipmentIdentity const& identity,std::uint64_t generation) noexcept {
    const auto& h=weaponhand::held_binding;
    return generation&&vrequip::model.hold_generation()==generation&&h.known&&h.generation==generation&&
        hand::same(h.world,identity.world)&&hand::same(h.controller,identity.controller)&&
        hand::same(h.pawn,identity.pawn)&&hand::same(h.weapon,identity.weapon)&&
        h.native_cookie==identity.key.registration_cookie&&h.native_started==identity.key.native_started_qpc&&
        h.world_generation==identity.key.world_generation&&
        vrequip::model.held_for(identity.world.address,identity.pawn.address,identity.weapon.address);
}
struct Reader {
    wire::Request const& request;
    sender::Parent const& parent;
    wire::Status status{wire::Status::inactive};
    template<class Current>
    bool operator()(hand::EquipmentIdentity const& identity,Current const& client_current,
        hand::EquipmentObservation& out) noexcept {
        const auto current=[&]() noexcept {
            hand::EquipmentIdentity checked{};
            return client_current()&&enabled.load(std::memory_order_acquire)&&nativeequipment::domain_ready()&&
                sender::context(request,parent,weaponhand::clock_ns(nullptr),checked)==wire::Status::ready;
        };
        if(!current())return false;
        const auto generation=vrequip::model.hold_generation();
        hand::GripGameState before{},after{};
        if(!weaponhand::observe_equipment(identity,before))return false;
        status=sender::eligibility(identity,before,generation);
        if(status!=wire::Status::ready)return false;
        status=wire::Status::inactive;
        lancermain_asset::Evidence first{},last{};
        if(!current())return false;
        if(lancermain_asset::read_equipment(identity,first)!=lancermain_asset::Status::ready){status=wire::Status::unsupported_asset;return false;}
        if(!current())return false;
        if(!nativeequipment::read_attached_equipment(identity,first.component.address)){status=wire::Status::not_attached;return false;}
        if(!current())return false;
        if(lancermain_asset::read_equipment(identity,last)!=lancermain_asset::Status::ready||
           !hand::same(first.component,last.component)||!hand::same(first.asset,last.asset)){
            status=wire::Status::unsupported_asset;return false;
        }
        if(!weaponhand::observe_equipment(identity,after)||!hand::same_game(before,after)||
           !held_current(identity,generation)||!current())return false;
        status=sender::eligibility(identity,after,generation);
        if(status!=wire::Status::ready)return false;
        out.game=after;out.hold_generation=generation;out.component=last.component;out.asset=last.asset;return true;
    }
};
wire::Status observe(wire::Request const& request,sender::Parent const& parent,
    hand::EquipmentIdentity const& identity,hand::EquipmentObservation& out) noexcept {
    out={};Reader reader{request,parent};
    if(!weaponhand::client.observe_equipment_on_gt(identity,reader,out))
        return reader.status==wire::Status::ready?wire::Status::inactive:reader.status;
    return wire::Status::ready;
}
} // namespace bodymountequipment
