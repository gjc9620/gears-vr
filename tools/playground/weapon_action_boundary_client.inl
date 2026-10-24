// Lifecycle for the draft external boundary. Entry restoration/PE bridging is
// not connected yet. No engine call or Native permission is minted here.
namespace weaponboundary {
namespace boundaryabi=gears_weapon_boundary;
boundaryabi::Admission admission;
void seal() noexcept {admission.seal();}
bool local_quiescent() noexcept {return admission.quiescent();}
bool detached() noexcept {return admission.detach();}
gears_weapon_hand::Reply attach(void* image) noexcept {
    using Reply=gears_weapon_hand::Reply;
    const auto native=static_cast<HMODULE>(image);
    if(!native||native!=weaponhand::native_module||!weaponaction::ready()||!weaponcall::ready())return Reply::unavailable;
    const auto revoke_address=GetProcAddress(native,boundaryabi::revoke_export),open_address=GetProcAddress(native,boundaryabi::open_export),
        close_address=GetProcAddress(native,boundaryabi::close_export);
    if(!weaponhand::executable(revoke_address,native)||!weaponhand::executable(open_address,native)||!weaponhand::executable(close_address,native))
        return Reply::unavailable;
    boundaryabi::Binding binding{weaponhand::client.cookie(),weaponhand::client.native_started_qpc(),&weaponhand::client,
        reinterpret_cast<boundaryabi::Revoke>(revoke_address),reinterpret_cast<boundaryabi::Open>(open_address),reinterpret_cast<boundaryabi::Close>(close_address)};
    return admission.connect(binding)?Reply::accepted:Reply::busy;
}
} // namespace weaponboundary
