// Invocation-local HUD result adapter on Native's EXISTING CallFunction owner.
// No new hook, engine call, input grant, inventory or weapon-default writer.
namespace lancerreload_ui {
namespace hand=gears_weapon_hand;
namespace state=lancerreload_state;
namespace abi=gears_weapon_call;
constexpr const wchar_t* outputs[]={L"ActiveReloadStartTime",L"PreReactionWindowDuration",L"SuperSweetSpotDuration",L"SweetSpotDuration"};
bool object(hand::ObjectIdentity const& expected) noexcept {
    hand::ObjectIdentity now{};return nativeequipment::identify(nullptr,expected.address,now)&&hand::same(now,expected);
}
bool original_function(uptr fn,const wchar_t* label,const wchar_t* owner_label,bool native) noexcept {
    if(!registered(fn)||!playground::named(fn,label)||!playground::named(get<uptr>(fn+0x50),L"Function"))return false;
    const auto owner=get<uptr>(fn+0x40),package=owner?get<uptr>(owner+0x40):0;
    return registered(owner)&&registered(package)&&playground::named(owner,owner_label)&&
        playground::named(package,L"WarfareGame")&&get<uptr>(package+0x40)==0&&
        (!native||((get<unsigned>(fn+0xf4)&0x400)&&
        weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(fn+0x114)),GetModuleHandleW(nullptr))));
}
bool prepare(hand::Frame const& source) noexcept {
    auto& l=state::lease;
    if(!l.acquired||!l.ui_owned||!l.ui_replaced||!l.hud_owned||!state::ui_hud_current()||
       !(l.identity==lancerreload_inventory::identity(source)))return false;
    playground::Args draw(l.ui_hud.address,L"DrawActionReloadBar"),getter(source.weapon.address,L"GetActiveReloadValues");
    if(!original_function(draw.r.function,L"DrawActionReloadBar",L"WarHUD",false)||
       !original_function(getter.r.function,L"GetActiveReloadValues",L"WarWeapon",true)||getter.r.size!=16||
       !nativeequipment::identify(nullptr,draw.r.function,l.ui_draw_function)||
       !nativeequipment::identify(nullptr,getter.r.function,l.ui_getter_function))return false;
    l.ui_locals_extent=get<unsigned>(draw.r.function+0x88);
    if(!l.ui_locals_extent||l.ui_locals_extent>4096||!copy(&l.ui_draw_script,draw.r.function+0x8c,sizeof(Array))||
       !l.ui_draw_script.data||l.ui_draw_script.count<37||l.ui_draw_script.count>l.ui_draw_script.capacity||
       l.ui_draw_script.capacity>131072)return false;
    unsigned occupied=0;
    for(unsigned i=0;i<4;++i){const auto local=draw.prop(outputs[i]),formal=getter.prop(outputs[i]);
        if(!registered(local)||get<uptr>(local+0x40)!=draw.r.function||!playground::named(get<uptr>(local+0x50),L"FloatProperty")||
           get<unsigned>(local+0x70)!=1||get<unsigned>(local+0x74)!=4||(get<unsigned long long>(local+0x78)&0x580)!=0||
           !registered(formal)||get<uptr>(formal+0x40)!=getter.r.function||!playground::named(get<uptr>(formal+0x50),L"FloatProperty")||
           get<unsigned>(formal+0x70)!=1||get<unsigned>(formal+0x74)!=4||(get<unsigned long long>(formal+0x78)&0x580)!=0x180)return false;
        const auto at=get<unsigned>(local+0x94),parameter=get<unsigned>(formal+0x94);
        if(at>l.ui_locals_extent||4>l.ui_locals_extent-at||parameter>12||parameter%4||occupied&(1u<<(parameter/4)))return false;
        occupied|=1u<<(parameter/4);
        for(unsigned j=0;j<i;++j)if(at<l.ui_local_offsets[j]+4&&l.ui_local_offsets[j]<at+4)return false;
        if(!nativeequipment::identify(nullptr,local,l.ui_local_properties[i]))return false;
        l.ui_local_offsets[i]=at;
    }
    return occupied==15&&state::ui_hud_current();
}
struct Slot {
    abi::Invocation call{};gears_ui_call_boundary::Frame frame{};uptr locals{};
    std::uint64_t serial{};std::array<unsigned char,37> code{};
    std::array<uptr,4> destinations{};std::array<float,4> desired{};bool active{};
};
std::array<Slot,4> slots{};
bool binding(abi::Invocation const& call) noexcept {
    const auto& l=state::lease;
    if(!weaponhand::current_gt(nullptr)||!l.acquired||!l.ui_owned||!l.ui_replaced||!l.hud_owned||
       call.self!=l.identity.weapon.address||call.callee!=l.ui_getter_function.address||
       call.hand_cookie!=l.identity.registration||call.native_started_qpc!=l.identity.native_start||
       !state::ui_hud_current()||!object(l.identity.weapon)||!object(l.identity.world)||!object(l.identity.pawn)||
       !object(l.ui_draw_function)||!object(l.ui_getter_function)||
       !original_function(call.callee,L"GetActiveReloadValues",L"WarWeapon",true))return false;
    Array script{};
    if(!copy(&script,l.ui_draw_function.address+0x8c,sizeof(script))||script.data!=l.ui_draw_script.data||
       script.count!=l.ui_draw_script.count||script.capacity!=l.ui_draw_script.capacity||
       get<unsigned>(l.ui_draw_function.address+0x88)!=l.ui_locals_extent)return false;
    for(unsigned i=0;i<4;++i){const auto prop=l.ui_local_properties[i].address;
        if(!object(l.ui_local_properties[i])||get<uptr>(prop+0x40)!=l.ui_draw_function.address||
           !playground::named(get<uptr>(prop+0x50),L"FloatProperty")||get<unsigned>(prop+0x94)!=l.ui_local_offsets[i]||
           get<unsigned>(prop+0x70)!=1||get<unsigned>(prop+0x74)!=4||(get<unsigned long long>(prop+0x78)&0x580)!=0)return false;
    }
    return true;
}
void begin(abi::Invocation const& call) noexcept {
    if(!binding(call))return;
    auto& l=state::lease;Slot s{};l.ui_draw_gate=1;
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,s.frame)||
       s.frame.node!=l.ui_draw_function.address||s.frame.object!=l.ui_hud.address||
       s.frame.code<l.ui_draw_script.data||s.frame.code>l.ui_draw_script.data+uptr(l.ui_draw_script.count)-s.code.size()||
       !copy(&s.locals,call.caller_frame+0x30,8)||!s.locals||s.locals>UINTPTR_MAX-l.ui_locals_extent||
       !copy(s.code.data(),s.frame.code,s.code.size()))return;
    l.ui_draw_gate=2;
    for(unsigned i=0;i<4;++i){uptr property{};std::memcpy(&property,s.code.data()+i*9+1,8);
        // Exact four local-variable out arguments, followed by EndFunctionParms.
        if(s.code[i*9]!=0x00||property!=l.ui_local_properties[i].address)return;
        s.destinations[i]=s.locals+l.ui_local_offsets[i];s.desired[i]=l.ui_wanted[i+1];
    }
    if(s.code[36]!=0x16)return;
    l.ui_draw_gate=3;
    s.call=call;s.serial=l.serial;s.active=true;
    for(auto& slot:slots)if(!slot.active){slot=s;return;}
}
bool returned(abi::Invocation const& call) noexcept {
    Slot* found=nullptr;for(auto& slot:slots)if(slot.active&&slot.call.call_id==call.call_id){found=&slot;break;}
    if(!found)return true;const auto s=*found;*found={};
    if(!abi::same(s.call,call)||s.serial!=state::lease.serial||!binding(call))return true;
    gears_ui_call_boundary::Frame after{};uptr locals{};std::array<unsigned char,37> code{};
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,after)||
       after.node!=s.frame.node||after.object!=s.frame.object||after.previous!=s.frame.previous||after.code!=s.frame.code+s.code.size()||
       !copy(&locals,call.caller_frame+0x30,8)||locals!=s.locals||!copy(code.data(),s.frame.code,code.size())||code!=s.code)return true;
    state::lease.ui_draw_gate=4;
    std::array<float,4> original_values{},checked{};
    for(unsigned i=0;i<4;++i)if(!copy(&original_values[i],s.destinations[i],4)||!std::isfinite(original_values[i]))return true;
    unsigned changed=0;
    for(;changed<4;++changed){
        if(!state::ui_store(s.destinations[changed],&s.desired[changed],4)||
           !copy(&checked[changed],s.destinations[changed],4)||checked[changed]!=s.desired[changed])break;
    }
    if(changed!=4){
        bool restored=true;
        for(unsigned i=0;i<=changed&&i<4;++i){float value{};
            if(!copy(&value,s.destinations[i],4)){restored=false;continue;}
            if(value==original_values[i])continue;
            if(value!=s.desired[i]||!state::ui_store(s.destinations[i],&original_values[i],4))restored=false;
        }
        return restored;
    }
    auto& l=state::lease;++l.ui_draw_updates;l.ui_draw_gate=5;
    l.ui_draw_original=original_values;l.ui_draw_current=checked;
    return true;
}
void cleanup(abi::Invocation const& call) noexcept {
    for(auto& slot:slots)if(slot.active&&slot.call.call_id==call.call_id)slot={};
}
} // namespace lancerreload_ui
