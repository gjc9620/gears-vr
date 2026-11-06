// Only the existing authenticated ordinary PlayerTick, same PG hook and Native
// participant. Borrow RAW move axes and a timed aBaseX yaw delta through original PlayerInput/PlayerMove
// call. Original engine movement/ignore-input/collision remain the consumers.
namespace controller_locomotion {
namespace hand=gears_weapon_hand;
namespace meta=gripdamage_metadata;
thread_local bool entered{};
bool faulted{};unsigned logs{};
std::atomic<unsigned> turn_deadzone_basis_points{1800};
struct Lease {
    hand::ObjectIdentity controller{},input{};
    std::array<meta::Property,3> fields{};
    std::array<uptr,3> descriptors{};
    std::array<hand::ObjectIdentity,3> descriptor_ids{};
    std::array<uptr,3> types{};
    std::array<float,3> before{};
    gears_controller_locomotion::Axes axes{};
    float turn{};
    float turn_delta{},delta_seconds{};
    std::uint64_t sequence{};
    unsigned written{};
};
bool writable(uptr address) noexcept {
    MEMORY_BASIC_INFORMATION m{};
    if(!address||VirtualQuery(reinterpret_cast<void const*>(address),&m,sizeof(m))!=sizeof(m)||
       m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))||
       m.RegionSize<sizeof(float)||address-reinterpret_cast<uptr>(m.BaseAddress)>m.RegionSize-sizeof(float))return false;
    const auto access=m.Protect&0xffu;
    return access==PAGE_READWRITE||access==PAGE_WRITECOPY||access==PAGE_EXECUTE_READWRITE||access==PAGE_EXECUTE_WRITECOPY;
}
bool write(uptr address,float value) noexcept {
    if(!writable(address))return false;
    __try {std::memcpy(reinterpret_cast<void*>(address),&value,sizeof(value));return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool identify(meta::Reader& r,uptr address,hand::ObjectIdentity& out) noexcept {
    meta::U32 slot{};if(!r.path(address,slot))return false;
    const auto& id=r.result.identities[slot];
    out={id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};return true;
}
void finish(Lease& lease) noexcept {
    if(!lease.written)return;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    bool restored=meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,lease.input);
    // Restoration is of this exact old input object, not of a replacement
    // Pawn/PlayerInput and not a new input action. Expiring XR cannot strand it.
    const bool input_alive=restored;
    for(unsigned i=0;input_alive&&i<lease.fields.size();++i){
        if(!(lease.written&(1u<<i)))continue;
        meta::Property fresh{};
        const auto& old=lease.fields[i];
        // Slot ordinals belong to their own Reader. Compare actual descriptor
        // and type identities plus layout across the two independent schemas.
        const bool same=weaponhand::object(r,lease.descriptor_ids[i])&&r.property(lease.descriptors[i],fresh)&&
            r.result.identities[fresh.type].address==lease.types[i]&&fresh.next==old.next&&fresh.flags==old.flags&&
            fresh.dimension==old.dimension&&fresh.element_bytes==old.element_bytes&&fresh.offset==old.offset;
        const bool cleaned=same&&write(lease.input.address+old.offset,lease.before[i]);
        restored=cleaned&&restored;
    }
    if(!restored)faulted=true;
    if(log_file&&(logs<8||!restored)){
        fwprintf(log_file,L"controller_locomotion sequence=%llu right=%.6g forward=%.6g turn=%.6g yaw_delta=%.6g delta_seconds=%.6g written_mask=%u original_right=%.6g original_forward=%.6g original_base_x=%.6g restored=%u\n",
            lease.sequence,double(lease.axes.right),double(lease.axes.forward),double(lease.turn),double(lease.turn_delta),double(lease.delta_seconds),lease.written,double(lease.before[1]),double(lease.before[0]),double(lease.before[2]),unsigned(restored));
        fflush(log_file);++logs;
    }
    lease.written=0;
}
bool tick_seconds(meta::Reader& r,uptr function,void const* params,float& seconds) noexcept {
    meta::Structure structure{};std::uint16_t bytes{};meta::U32 kind{};
    if(!params||!r.structure(function,structure)||!r.named(structure.identity,L"PlayerTick")||
       !r.path(r.result.identities[structure.identity].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.at(function,0x106,bytes)||bytes<4||bytes>4096)return false;
    std::array<uptr,32> seen{};unsigned count{};
    for(auto child=structure.children;child;){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;
        seen[count++]=child;meta::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=function)return false;
        if(r.named(p.identity,L"DeltaTime")){
            return weaponhand::core_type(r,p.type,L"FloatProperty")&&p.dimension==1&&p.element_bytes==4&&
                (p.flags&0x580)==0x80&&meta::property_fits(p,bytes)&&
                r.at(reinterpret_cast<uptr>(params),p.offset,seconds)&&std::isfinite(seconds)&&seconds>0&&seconds<=.25f;
        }
        child=p.next;
    }
    return false;
}
bool sensitivity_multiplier(meta::Reader& r,uptr pc,float& multiplier) noexcept {
    // Query the same read-only multiplier used after the base PlayerInput
    // event. It selects mouse/gamepad and normal/target/zoom profile settings.
    playground::Args call(pc,L"GetSensitivityMultiplier");meta::Structure structure{};
    meta::U32 type{},owner{},package{};std::uint16_t bytes{},return_offset{};
    if(!call.valid||!r.structure(call.r.function,structure)||!r.named(structure.identity,L"GetSensitivityMultiplier")||
       !r.path(r.result.identities[structure.identity].klass,type)||!weaponhand::core_type(r,type,L"Function")||
       !r.path(r.result.identities[structure.identity].outer,owner)||!r.named(owner,L"WarPC")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"WarfareGame")||
       !r.at(call.r.function,0x106,bytes)||bytes<4||bytes>4096||bytes!=call.r.size||
       !r.at(call.r.function,0x108,return_offset)||return_offset>bytes-4)return false;
    meta::Property result{};const auto field=call.prop(L"ReturnValue");
    if(!field||!r.property(field,result)||r.result.identities[result.identity].outer!=call.r.function||
       !r.named(result.identity,L"ReturnValue")||!weaponhand::core_type(r,result.type,L"FloatProperty")||
       result.dimension!=1||result.element_bytes!=4||result.offset!=return_offset||(result.flags&0x480)!=0x480||!call.run())return false;
    multiplier=call.output<float>();
    return std::isfinite(multiplier)&&multiplier>=.01f&&multiplier<=10000.f;
}
bool prepare(uptr pc,uptr function,void const* params,Lease& lease) noexcept {
    if(faulted||!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr))return false;
    // Menu/unknown display routing already supplies ORIGINAL pad axes. Keep
    // only one axis owner across menu transitions; gameplay keeps this VR path.
    if(const auto native=GetModuleHandleW(L"GearsCapture6DofM1.dll")){
        using Owned=DWORD(WINAPI*)(void*);
        const auto owned=reinterpret_cast<Owned>(GetProcAddress(native,"GearsCaptureVrGamepadAxesOwned"));
        if(owned&&owned(nullptr))return false;
    }
    hand::Frame source{};
    if(!weaponhand::client.copy_frame_on_gt(source)||source.controller.address!=pc)return false;
    const auto& left=source.controllers.hands[0];
    lease.axes=gears_controller_locomotion::axes(left.stick_x,left.stick_y,left.stick_active==1);
    const auto& right_hand=source.controllers.hands[1];
    lease.turn=gears_controller_locomotion::turn(right_hand.stick_x,right_hand.stick_active==1,
        float(turn_deadzone_basis_points.load(std::memory_order_acquire))*.0001f);
    if(!lease.axes.active&&lease.turn==0)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField input{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,source.controller)||
       !weaponhand::read_pointer(r,pc,L"PlayerInput",input)||!input.value||!identify(r,input.value,lease.input)||
       lease.input.outer!=pc||!weaponhand::derives(r,input.value,L"WarPlayerInput",L"WarfareGame"))return false;
    // aBaseX avoids legacy stick acceleration/deceleration. WarPlayerInput
    // still multiplies it by MouseSensitivity * FOVScale * .75, then the
    // derived PlayerInput event applies WarPC's profile multiplier. Cancel
    // both sensitivity factors, preserving FOV/ignore-look/special-move rules.
    // No VR demand enters LastaTurn, so releasing cannot strand turn history.
    weaponhand::BoolField strafe_mode{},existing_turn{},sensitivity_field{};
    std::uint8_t strafing{};float other_turn{},sensitivity{},profile_multiplier{};
    if(lease.turn!=0){
        if(tick_seconds(r,function,params,lease.delta_seconds)&&
           weaponhand::field(r,input.value,L"bStrafe",L"ByteProperty",1,strafe_mode)&&
           r.at(input.value,strafe_mode.property.offset,strafing)&&!strafing&&
           weaponhand::field(r,input.value,L"aTurn",L"FloatProperty",4,existing_turn)&&
           r.at(input.value,existing_turn.property.offset,other_turn)&&other_turn==0&&
           weaponhand::field(r,input.value,L"MouseSensitivity",L"FloatProperty",4,sensitivity_field)&&
           r.at(input.value,sensitivity_field.property.offset,sensitivity)&&std::isfinite(sensitivity)&&
           sensitivity>=.01f&&sensitivity<=10000.f&&sensitivity_multiplier(r,pc,profile_multiplier))
            lease.turn_delta=gears_controller_locomotion::yaw_delta(lease.turn,lease.delta_seconds)/(sensitivity*.75f*profile_multiplier);
    }
    // Match first-name/typed metadata using the existing checked reader. Do not
    // write a guessed fixed offset or fake global bUsingGamepad.
    weaponhand::BoolField forward{},right{},turn{};
    if(!weaponhand::field(r,input.value,L"aBaseY",L"FloatProperty",4,forward)||
       !weaponhand::field(r,input.value,L"aStrafe",L"FloatProperty",4,right)||
       !weaponhand::field(r,input.value,L"aBaseX",L"FloatProperty",4,turn))return false;
    const std::array<weaponhand::BoolField,3> fields{forward,right,turn};
    for(unsigned i=0;i<fields.size();++i){
        lease.fields[i]=fields[i].property;
        lease.descriptors[i]=schema.identities[fields[i].property.identity].address;
        lease.types[i]=schema.identities[fields[i].property.type].address;
        if(!identify(r,lease.descriptors[i],lease.descriptor_ids[i]))return false;
        if(!r.at(input.value,lease.fields[i].offset,lease.before[i])||!std::isfinite(lease.before[i])||
           !writable(input.value+lease.fields[i].offset))return false;
    }
    // Leave pre-existing keyboard/gamepad/mouse axes untouched. Independent
    // zero-valued channels may still accept VR input (e.g. W plus VR yaw).
    const unsigned selected=(lease.axes.active&&lease.before[0]==0&&lease.before[1]==0?3u:0u)|
        (lease.turn_delta!=0&&lease.before[2]==0?4u:0u);
    if(!selected)return false;
    if(selected&4u){float current_sensitivity{};
        if(!r.at(input.value,sensitivity_field.property.offset,current_sensitivity)||current_sensitivity!=sensitivity)return false;
    }
    if(!weaponhand::client.source_current_on_gt(source.key)||
       !weaponhand::object(r,lease.input))return false;
    meta::PointerField current{};
    if(!weaponhand::pointer(r,pc,L"PlayerInput",input.value,current))return false;
    for(unsigned i=0;i<fields.size();++i){
        meta::Property fresh{};float value{};
        if(!r.property(lease.descriptors[i],fresh)||!meta::same_property(fresh,lease.fields[i])||
           !r.at(input.value,lease.fields[i].offset,value)||value!=lease.before[i])return false;
    }
    lease.controller=source.controller;lease.sequence=source.controllers.sequence;
    const std::array<float,3> values{lease.axes.forward,lease.axes.right,lease.turn_delta};
    for(unsigned i=0;i<values.size();++i){
        if(!(selected&(1u<<i)))continue;
        if(!write(input.value+lease.fields[i].offset,values[i])){finish(lease);return false;}
        lease.written|=1u<<i;
    }
    return true;
}
using Dispatch=void(__fastcall*)(uptr,uptr,void*);
void dispatch(uptr object,uptr function,void* params,Dispatch original_dispatch) {
    // No C++ unwind-only guard: restore our borrowed axes on both normal
    // return and SEH/C++ exceptional exit of the original PlayerTick.
    Lease lease{};
    entered=true;
    __try {
        prepare(object,function,params,lease);
        original_dispatch(object,function,params);
    } __finally {finish(lease);entered=false;}
}
}
