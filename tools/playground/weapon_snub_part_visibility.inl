// Client of Native's single original decoder writer. No bone memory writes
// here. A hidden receipt is visibility only, never inventory or hand authority.
namespace snub_part_visibility {
namespace api=gears_weapon_part_visibility;namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;
api::Control control{};std::atomic<bool> retained{},stop_requested{};
hand::ObjectIdentity weapon{},component{},asset{};std::uint64_t serial{};
bool outstanding() noexcept {return retained.load(std::memory_order_acquire);}
void request_stop() noexcept {stop_requested.store(true,std::memory_order_release);}
void reset_after_drain() noexcept {if(!outstanding())stop_requested.store(false,std::memory_order_release);}
template<class Fn>Fn function(const char* label) noexcept {
    const auto module_handle=weaponhand::native_module;
    const auto address=module_handle?GetProcAddress(module_handle,label):nullptr;
    return address&&weaponhand::executable(address,module_handle)?reinterpret_cast<Fn>(address):nullptr;
}
bool begin(hand::Frame const& source,std::uint64_t reload_serial) noexcept {
    if(outstanding()||stop_requested.load()||!weaponhand::current_gt(nullptr)||
       !playground::instance(source.weapon.address,L"Weap_COGPistol")||
       !(anim::ownership.visual_only?nativeequipment::current(nullptr,source):lancerreload_state::owns(source,reload_serial))||
       !lancerreload_state::item_owned(source)||
       !anim::ownership.active||!anim::ownership.stopped||anim::ownership.serial!=reload_serial||
       !(anim::ownership.inventory==lancerreload_inventory::identity(source)))return false;
    const auto call=function<api::Begin>(api::begin_export);if(!call)return false;
    api::Request request{};request.source=source;request.component=anim::ownership.last.component;
    request.asset=anim::ownership.last.asset;request.node=anim::ownership.last.node;request.transaction=reload_serial;
    if(!nativeequipment::current(nullptr,source))return false;
    api::Receipt receipt{};const auto result=call(&request,&receipt);
    if((result!=api::Result::pending&&result!=api::Result::hidden)||receipt.size!=sizeof(receipt)||
       receipt.version!=1||!receipt.handle||receipt.result!=result)return false;
    control={};control.handle=receipt.handle;control.cookie=source.key.registration_cookie;
    weapon=source.weapon;component=request.component;asset=request.asset;serial=reload_serial;
    retained.store(true,std::memory_order_release);return true;
}
bool hidden(hand::Frame const& source,std::uint64_t reload_serial) noexcept {
    if(!outstanding()||stop_requested.load()||serial!=reload_serial||
       !hand::same(source.weapon,weapon)||
       !(anim::ownership.visual_only?(anim::ownership.active&&anim::ownership.serial==reload_serial&&
            anim::ownership.inventory==lancerreload_inventory::identity(source)):lancerreload_state::owns(source,reload_serial))||
       !nativeequipment::current(nullptr,source))return false;
    const auto call=function<api::Update>(api::poll_export);if(!call)return false;
    api::Receipt receipt{};const auto result=call(&control,&receipt);
    return result==api::Result::hidden&&receipt.result==result&&receipt.size==sizeof(receipt)&&
        receipt.version==1&&receipt.handle==control.handle&&receipt.writes&&receipt.original_space_hidden&&
        nativeequipment::current(nullptr,source);
}
bool restore() noexcept {
    if(!outstanding())return true;
    if(!weaponhand::current_gt(nullptr))return false;
    const auto call=function<api::Update>(api::restore_export);if(!call)return false;
    api::Receipt receipt{};const auto result=call(&control,&receipt);
    const bool restored=result==api::Result::restored||(result==api::Result::inactive_restored&&receipt.original_inactive);
    if(!restored||receipt.result!=result||receipt.size!=sizeof(receipt)||
       receipt.version!=1||receipt.handle!=control.handle||
       (!receipt.cpu_restored&&!(receipt.original_inactive&&receipt.original_destroyed)))return false;
    control={};weapon=component=asset={};serial=0;retained.store(false,std::memory_order_release);return true;
}
void drain_on_gt() noexcept {
    if(stop_requested.load(std::memory_order_acquire)&&outstanding())restore();
}
} // namespace snub_part_visibility
