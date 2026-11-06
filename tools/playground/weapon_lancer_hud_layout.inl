#include "../vr-common/hud_action_layout.h"
// Original DrawActionInfo only. No action/button is cleared, skipped or rebound.
// Canvas is transient: resolve its TYPE on GT, read the actual object in drawing.
namespace lancerhud_layout {
namespace hand=gears_weapon_hand;namespace abi=gears_weapon_call;namespace layout=gears_hud_action_layout;
struct Field {hand::ObjectIdentity property{};unsigned offset{},width{};};
// Only object/owner identity survives between ticks. The follower's render
// carrier intentionally has no action acquisition clock: never treat it as a
// fresh hand Frame or synthesize one for a cosmetic HUD layout.
struct Context {
    hand::ObjectIdentity controller{},world{},pawn{},weapon{};
    std::uint64_t cookie{},native{},hold{};
};
struct Prepared {
    Context context{};hand::ObjectIdentity hud{},canvas_class{},draw{},caller{};
    // HUD/Canvas layout and the current controller/pawn/weapon live links.
    std::array<Field,12> fields{};uptr draw_entry{};Array script{};bool ready{};
};
Prepared prepared{};
struct Slot {abi::Invocation call{};Prepared context{};hand::ObjectIdentity canvas{};layout::Scope scope{};bool active{};};
std::array<Slot,4> slots{};
std::uint64_t placed{},restored{},last_frame{};unsigned logs{};
std::array<float,2> last_clip{},last_before{},last_wanted{};
bool field(uptr cls,const wchar_t* label,const wchar_t* type,unsigned width,Field& out) noexcept {
    const auto p=playground::field(cls,label);
    if(!registered(cls)||!p||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=width||
       !playground::named(get<uptr>(p+0x50),type)||!nativeequipment::identify(nullptr,p,out.property))return false;
    const auto at=get<unsigned>(p+0x94),extent=get<unsigned>(cls+0x88);
    if(at%4||at>extent||width>extent-at)return false;
    out.offset=at;out.width=width;return true;
}
bool identities(Prepared const& p) noexcept {
    // One stable table pair for this complete read, rather than reopening both
    // engine tables for each of fifteen objects. Every identity is still read
    // twice, including its actual name, class, outer and registration slot.
    namespace meta=weaponhand::meta;
    meta::Table objects{},names{},after_objects{},after_names{};
    if(!meta::tables(base,objects,names))return false;
    const auto current=[&](hand::ObjectIdentity const& expected) noexcept {
        meta::Identity a{},b{};meta::NameText at{},bt{};
        if(!hand::identity_valid(expected)||
           !meta::raw_identity(objects,names,expected.address,a,at)||
           !meta::raw_identity(objects,names,expected.address,b,bt)||
           !meta::same_identity(a,b)||!meta::same_name_text(at,bt))return false;
        const hand::ObjectIdentity now{a.address,a.outer,a.klass,a.index,a.name.index,a.name.number,1};
        return hand::same(now,expected);
    };
    if(!current(p.hud)||!current(p.canvas_class)||!current(p.context.controller)||
       !current(p.draw)||!current(p.caller))return false;
    for(auto const& f:p.fields)if(!current(f.property)||
       get<unsigned>(f.property.address+0x94)!=f.offset||get<unsigned>(f.property.address+0x70)!=1||
       get<unsigned>(f.property.address+0x74)!=f.width)return false;
    Array script{};
    return get<uptr>(p.context.controller.address+p.fields[0].offset)==p.hud.address&&
        get<uptr>(p.hud.address+p.fields[1].offset)==p.context.controller.address&&
        get<uptr>(p.fields[2].property.address+0xd0)==p.canvas_class.address&&
        get<uptr>(p.draw.address+0x114)==p.draw_entry&&copy(&script,p.draw.address+0x8c,sizeof script)&&
        script.data==p.script.data&&script.count==p.script.count&&script.capacity==p.script.capacity&&
        meta::tables(base,after_objects,after_names)&&meta::same_table(objects,after_objects)&&meta::same_table(names,after_names);
}
bool context_current(Prepared const& p) noexcept {
    const auto& c=p.context;
    // Layout only: these checks do not grant an input, contact or action lease.
    // Cleanup below deliberately does not require this continuing held state.
    return enabled.load(std::memory_order_acquire)&&weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready()&&
        weaponhand::client.registration_alive(c.cookie,c.native)&&c.hold&&
        vrequip::model.held_for(c.world.address,c.pawn.address,c.weapon.address)&&
        vrequip::model.hold_generation()==c.hold&&
        lancerreload_ui::object(c.world)&&lancerreload_ui::object(c.pawn)&&lancerreload_ui::object(c.weapon)&&
        get<uptr>(c.controller.address+p.fields[7].offset)==c.pawn.address&&
        get<uptr>(c.controller.address+p.fields[8].offset)==c.world.address&&
        get<uptr>(c.pawn.address+p.fields[9].offset)==c.controller.address&&
        get<uptr>(c.pawn.address+p.fields[10].offset)==c.weapon.address&&
        get<uptr>(c.weapon.address+p.fields[11].offset)==c.pawn.address;
}
void tick(uptr pc) noexcept try {
    if(!weaponhand::current_gt(nullptr))return;
    // A momentary missing pose does not move a still-owned cosmetic prompt
    // back across the gun. Only current object ownership keeps this context.
    if(prepared.ready&&prepared.context.controller.address==pc&&context_current(prepared)&&identities(prepared)){
        static std::uint64_t reported{};
        if(log_file&&logs<8&&restored!=reported){reported=restored;++logs;
            fwprintf(log_file,L"lancer_hud_layout frame=%llu placed=%llu restored=%llu clip=%.3f,%.3f centre_x=%.3f bottom=%.3f original_actions_unchanged=1\n",
                last_frame,placed,restored,last_clip[0],last_clip[1],last_wanted[0],last_wanted[1]);fflush(log_file);}
        return;
    }
    prepared.ready=false;
    const auto& follow=nativelancerfollow::plan;const auto& source=follow.source;
    if(!weaponhand::current_gt(nullptr)||!nativelancerfollow::bound_is_lancer||!nativelancerfollow::bound_key_known||
       !follow.valid||follow.departing||source.controller.address!=pc||
       !vrequip::model.held_for(source.world.address,source.pawn.address,source.weapon.address))return;
    const Context context{source.controller,source.world,source.pawn,source.weapon,
        source.key.registration_cookie,source.key.native_started_qpc,vrequip::model.hold_generation()};
    Prepared p{};
    bool checked=false;
    if(hand::same(source.controller,prepared.context.controller)&&hand::same(source.world,prepared.context.world)&&
       hand::same(source.pawn,prepared.context.pawn)&&hand::same(source.weapon,prepared.context.weapon)&&identities(prepared)){
        p=prepared;p.context=context;p.ready=false;
        checked=true;
    }else{
        p.context=context;
        const auto hud=playground::pointer(pc,L"myHUD");
        if(!playground::instance(hud,L"WarHUD")||!nativeequipment::identify(nullptr,hud,p.hud))return;
        const auto hud_class=get<uptr>(hud+0x50),pc_class=get<uptr>(pc+0x50);
        playground::Args draw(hud,L"DrawActionInfo"),caller(hud,L"DrawActions");
        if(!lancerreload_ui::original_function(draw.r.function,L"DrawActionInfo",L"WarHUD",false)||
           !lancerreload_ui::original_function(caller.r.function,L"DrawActions",L"WarHUD",false)||
           !nativeequipment::identify(nullptr,draw.r.function,p.draw)||!nativeequipment::identify(nullptr,caller.r.function,p.caller)||
           !field(pc_class,L"myHUD",L"ObjectProperty",8,p.fields[0])||
           !field(hud_class,L"PlayerOwner",L"ObjectProperty",8,p.fields[1])||
           !field(hud_class,L"Canvas",L"ObjectProperty",8,p.fields[2])||
           !field(hud_class,L"SafeZoneFriendlyCenterX",L"FloatProperty",4,p.fields[3])||
           !field(hud_class,L"SafeZoneBottom",L"FloatProperty",4,p.fields[4])||
           !field(pc_class,L"Pawn",L"ObjectProperty",8,p.fields[7])||
           !field(pc_class,L"WorldInfo",L"ObjectProperty",8,p.fields[8])||
           !field(source.pawn.klass,L"Controller",L"ObjectProperty",8,p.fields[9])||
           !field(source.pawn.klass,L"Weapon",L"ObjectProperty",8,p.fields[10])||
           !field(source.weapon.klass,L"Instigator",L"ObjectProperty",8,p.fields[11]))return;
        const auto canvas_class=get<uptr>(p.fields[2].property.address+0xd0);
        if(!registered(canvas_class)||!playground::named(canvas_class,L"Canvas")||
           !playground::named(get<uptr>(canvas_class+0x50),L"Class")||
           !nativeequipment::identify(nullptr,canvas_class,p.canvas_class)||
           !field(canvas_class,L"ClipX",L"FloatProperty",4,p.fields[5])||
           !field(canvas_class,L"ClipY",L"FloatProperty",4,p.fields[6])||
           !copy(&p.script,p.draw.address+0x8c,sizeof p.script)||!p.script.data||p.script.count<=0||
           p.script.count>p.script.capacity||p.script.capacity>131072)return;
        p.draw_entry=get<uptr>(p.draw.address+0x114);
        if(!weaponhand::executable(reinterpret_cast<FARPROC>(p.draw_entry),GetModuleHandleW(nullptr)))return;
    }
    // The drawing Canvas is transient and is read in begin(). Metadata and
    // object ownership are independent of controller pose-sample lifetimes.
    p.ready=(checked||identities(p))&&context_current(p);
    prepared=p;if(p.ready)last_frame=source.key.frame;
    static std::uint64_t reported{};
    if(log_file&&logs<8&&restored!=reported){reported=restored;++logs;
        fwprintf(log_file,L"lancer_hud_layout frame=%llu placed=%llu restored=%llu clip=%.3f,%.3f before=%.3f,%.3f centre_x=%.3f bottom=%.3f original_actions_unchanged=1\n",
            source.key.frame,placed,restored,last_clip[0],last_clip[1],last_before[0],last_before[1],last_wanted[0],last_wanted[1]);fflush(log_file);}
}catch(...){prepared.ready=false;}
bool read_float(uptr p,float& v) noexcept {return copy(&v,p,4)&&std::isfinite(v);}
bool write_float(uptr p,float v) noexcept {return lancerreload_state::ui_store(p,&v,4);}
bool canvas_current(Slot const& s) noexcept {
    return lancerreload_ui::object(s.canvas)&&s.canvas.klass==s.context.canvas_class.address&&
        get<uptr>(s.context.hud.address+s.context.fields[2].offset)==s.canvas.address;
}
bool begin(abi::Invocation const& call) noexcept {
    const auto& p=prepared;
    if(!weaponhand::current_gt(nullptr)||!p.ready||call.self!=p.hud.address||call.callee!=p.draw.address||
       call.hand_cookie!=p.context.cookie||call.native_started_qpc!=p.context.native||
       !context_current(p)||!identities(p))return true;
    gears_ui_call_boundary::Frame caller{};
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,caller)||
       caller.node!=p.caller.address||caller.object!=p.hud.address)return true;
    for(auto const& s:slots)if(s.active&&s.call.self==call.self)return true;
    for(auto& s:slots)if(!s.active){
        s={};s.call=call;s.context=p;
        const auto canvas=get<uptr>(p.hud.address+p.fields[2].offset);
        if(!nativeequipment::identify(nullptr,canvas,s.canvas)||!canvas_current(s))return true;
        std::array<float,2> clip{};
        s.scope.addresses={{p.hud.address+p.fields[3].offset,p.hud.address+p.fields[4].offset}};
        if(!read_float(canvas+p.fields[5].offset,clip[0])||!read_float(canvas+p.fields[6].offset,clip[1])||
           !read_float(s.scope.addresses[0],s.scope.before[0])||!read_float(s.scope.addresses[1],s.scope.before[1])||
           !layout::plan(clip[0],clip[1],s.scope.before,s.scope.wanted))return true;
        s.active=true;
        if(s.scope.apply(read_float,write_float)){
            ++placed;last_clip=clip;last_before=s.scope.before;last_wanted=s.scope.wanted;return true;
        }
        if(s.scope.owned)return false;
        s={};return true;
    }
    return true;
}
bool cleanup(abi::Invocation const& call) noexcept {
    for(auto& s:slots)if(s.active&&s.call.call_id==call.call_id){
        // Restoring an owed scope does not require a fresh input sample.
        // Owner, descriptors and the actual drawing Canvas must still match.
        if(!abi::same(s.call,call)||!identities(s.context)||!canvas_current(s)||!s.scope.restore(read_float,write_float))return false;
        s={};++restored;return true;
    }
    return true;
}
bool stopped() noexcept {
    unsigned debt{};for(auto const& s:slots)debt+=unsigned(s.active||s.scope.owned);
    if(log_file){fwprintf(log_file,L"lancer_hud_layout_cleanup placed=%llu restored=%llu debt=%u original_actions_unchanged=1\n",placed,restored,debt);fflush(log_file);}
    return !debt;
}
}
