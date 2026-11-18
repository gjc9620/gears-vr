// Separate query-local metadata operation. Never enters old Frame-current
// wrappers while Client's Gate is held. All old V1 readers stay unchanged.
namespace bodymountequipment {
namespace sender=gears_body_main_mount_sender;
namespace wire=gears_body_main_mount_provider;
namespace hand=gears_weapon_hand;
// Small GT-only observations, reported through the existing drained summary.
// No pointers, source renewal or permission are retained.
std::array<std::uint64_t,9> asset_results{};
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
// Private, GT-only accounting of the exact existing unheld decision. No new
// Game read, callback, source renewal or action is allowed by this witness.
struct UnheldWitness {
    std::uint64_t queries{},generation{},model_held{},binding_known{},binding_generation{},
        binding_world{},binding_controller{},binding_pawn{},binding_weapon{},binding_native{};
    std::uint64_t first_after_generation_scene{},first_after_generation_weapon{},first_after_generation_binding_weapon{};
};
UnheldWitness unheld_witness{};
void note_unheld(hand::EquipmentIdentity const& identity,std::uint64_t generation) noexcept {
    auto& d=unheld_witness;const auto& h=weaponhand::held_binding;++d.queries;
    if(!generation)return;++d.generation;
    if(vrequip::model.held_for(identity.world.address,identity.pawn.address,identity.weapon.address))++d.model_held;
    if(h.known)++d.binding_known;
    if(h.known&&h.generation==generation)++d.binding_generation;
    if(h.known&&hand::same(h.world,identity.world))++d.binding_world;
    if(h.known&&hand::same(h.controller,identity.controller))++d.binding_controller;
    if(h.known&&hand::same(h.pawn,identity.pawn))++d.binding_pawn;
    if(h.known&&hand::same(h.weapon,identity.weapon))++d.binding_weapon;
    if(h.known&&h.native_cookie==identity.key.registration_cookie&&
        h.native_started==identity.key.native_started_qpc&&h.world_generation==identity.key.world_generation)++d.binding_native;
    if(!d.first_after_generation_scene){
        d.first_after_generation_scene=identity.key.frame;
        d.first_after_generation_weapon=identity.weapon.address;
        d.first_after_generation_binding_weapon=h.weapon.address;
    }
}
#endif
lancermain_asset::Status observed_asset(hand::EquipmentIdentity const& identity,lancermain_asset::Evidence& out,
    hand::WeaponKind kind) noexcept {
    const auto scope=kind==hand::WeaponKind::hand_thrown?lancermain_asset::AssetScope::frag_hold:lancermain_asset::AssetScope::basic_hold;
    const auto status=lancermain_asset::read_equipment(identity,out,scope);
    const auto index=unsigned(status);if(index<asset_results.size())++asset_results[index];return status;
}
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
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
        if(status==wire::Status::unheld)note_unheld(identity,generation);
#endif
        if(status!=wire::Status::ready)return false;
        status=wire::Status::inactive;
        lancermain_asset::Evidence first{},last{};
        if(!current())return false;
        if(observed_asset(identity,first,before.kind)!=lancermain_asset::Status::ready){status=wire::Status::unsupported_asset;return false;}
        if(!current())return false;
        if(!nativeequipment::read_attached_equipment(identity,first.component.address)){status=wire::Status::not_attached;return false;}
        if(!current())return false;
        if(observed_asset(identity,last,before.kind)!=lancermain_asset::Status::ready||
           !hand::same(first.component,last.component)||!hand::same(first.asset,last.asset)){
            status=wire::Status::unsupported_asset;return false;
        }
        if(!weaponhand::observe_equipment(identity,after)||!hand::same_game(before,after)||
           !held_current(identity,generation)||!current())return false;
        status=sender::eligibility(identity,after,generation);
        if(status!=wire::Status::ready)return false;
        out.game=after;out.hold_generation=generation;out.component=last.component;
        out.asset=last.asset;out.boomshot=last.kind==lancermain_asset::HeldAsset::boomshot;out.boltok=last.kind==lancermain_asset::HeldAsset::boltok;return true;
    }
};
wire::Status observe(wire::Request const& request,sender::Parent const& parent,std::uint64_t metadata_generation,
    hand::EquipmentIdentity const& identity,hand::EquipmentObservation& out) noexcept {
    out={};Reader reader{request,parent};
    const auto okay=weaponhand::client.observe_registered_equipment_on_gt(identity,metadata_generation,reader,out);
    const auto result=okay?wire::Status::ready:(reader.status==wire::Status::ready?wire::Status::inactive:reader.status);
    static auto previous=wire::Status::inactive;static unsigned changes{};
    if(result!=previous&&changes<32&&log_file){
        ++changes;previous=result;
        fwprintf(log_file,L"native_body_mount_equipment_state status=%u held=%u busy=%u animation=%u frame=%llu\n",
            unsigned(result),unsigned(out.game.main_hand_held),unsigned(out.game.busy),unsigned(out.game.animation_owns_hands),identity.key.frame);fflush(log_file);
    }
    return result;
}
} // namespace bodymountequipment
