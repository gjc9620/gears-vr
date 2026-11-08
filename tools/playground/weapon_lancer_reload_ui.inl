// Invocation-local HUD result adapter on Native's EXISTING CallFunction owner.
// No new hook, engine call, input grant, inventory or weapon-default writer.
#include "../vr-common/weapon_lancer_reload_display.h"
#include "../vr-common/weapon_lancer_reload_parent.h"
#include "../vr-common/hud_action_layout.h"
namespace lancerreload_ui {
namespace hand=gears_weapon_hand;
namespace state=lancerreload_state;
namespace abi=gears_weapon_call;
constexpr const wchar_t* outputs[]={L"ActiveReloadStartTime",L"PreReactionWindowDuration",L"SuperSweetSpotDuration",L"SweetSpotDuration"};
bool layout_field(uptr owner,const wchar_t* name,const wchar_t* type,unsigned width,
    unsigned index,unsigned base_offset=0) noexcept {
    auto& l=state::lease;const auto p=playground::field(owner,name);
    if(!registered(owner)||!registered(p)||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=width||
       !playground::named(get<uptr>(p+0x50),type))return false;
    const auto at=get<unsigned>(p+0x94),extent=get<unsigned>(owner+0x88);
    if(at%4||at>extent||width>extent-at||base_offset>UINT_MAX-at||
       !nativeequipment::identify(nullptr,p,l.ui_layout_properties[index]))return false;
    l.ui_layout_offsets[index]=base_offset+at;l.ui_layout_raw_offsets[index]=at;return true;
}
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
// Read-only presentation metadata survives the physical state exit. This is
// NOT a Reloading lease, a hand grant or a reward permit. Activation requires
// the already-confirmed original reward and its actual HUD result byte.
struct CompletedPresentation {
    state::Lease schema{};
    float completed{};
    std::int64_t completed_ns{};
    std::uint64_t draw_calls{},getter_calls{};
    unsigned cancel_gate{};
    bool staged{},active{},end_reported{};
};
CompletedPresentation completed;
struct ParentSchema {
    hand::ObjectIdentity function{},query{},weapon_property{},draw_property{},result_property{};
    Array script{};unsigned extent{},weapon_offset{},draw_offset{},draw_mask{};
    std::uint64_t serial{},query_calls{},gate_matches{},result_updates{};
    bool valid{};
};
ParentSchema parent;
struct ParentQuery {
    abi::Invocation call{};gears_ui_call_boundary::Frame frame{};uptr locals{};
    std::array<unsigned char,gears_lancer_reload_parent::span> bytes{};
    bool active{};
};
std::array<ParentQuery,2> parent_queries{};
bool prepare_parent(state::Lease const& l) noexcept {
    parent={};
    playground::Args draw(l.ui_hud.address,L"DrawWeaponInfo"),query(l.identity.weapon.address,L"IsReloading");
    if(!original_function(draw.r.function,L"DrawWeaponInfo",L"WarHUD",false)||
       !original_function(query.r.function,L"IsReloading",L"WarWeapon",false)||
       !vrequip::result_property(query,L"BoolProperty",4))return false;
    const auto wp=draw.prop(L"Weap"),ar=draw.prop(L"bDrawAR"),rv=query.prop(L"ReturnValue");
    const auto parameter=[&](uptr p,const wchar_t* type,unsigned width)noexcept{
        return registered(p)&&get<uptr>(p+0x40)==draw.r.function&&get<unsigned>(p+0x70)==1&&
            get<unsigned>(p+0x74)==width&&playground::named(get<uptr>(p+0x50),type)&&
            (get<unsigned long long>(p+0x78)&0x580)==0x80;
    };
    if(!parameter(wp,L"ObjectProperty",8)||!parameter(ar,L"BoolProperty",4)||
       get<uptr>(rv+0x40)!=query.r.function||
       !gears_lancer_reload_parent::query_return_layout(get<unsigned>(rv+0x70),get<unsigned>(rv+0x74),
           get<unsigned long long>(rv+0x78),get<unsigned>(rv+0x94),get<unsigned>(rv+0xd0)))return false;
    ParentSchema p{};p.extent=get<unsigned>(draw.r.function+0x88);
    p.weapon_offset=get<unsigned>(wp+0x94);p.draw_offset=get<unsigned>(ar+0x94);p.draw_mask=get<unsigned>(ar+0xd0);
    if(!p.extent||p.extent>4096||p.weapon_offset%8||p.draw_offset%4||!p.draw_mask||(p.draw_mask&(p.draw_mask-1))||
       p.weapon_offset>p.extent||8>p.extent-p.weapon_offset||p.draw_offset>p.extent||4>p.extent-p.draw_offset||
       !copy(&p.script,draw.r.function+0x8c,sizeof(Array))||!p.script.data||p.script.count<226||
       p.script.count>p.script.capacity||p.script.capacity>131072||
       !nativeequipment::identify(nullptr,draw.r.function,p.function)||!nativeequipment::identify(nullptr,query.r.function,p.query)||
       !nativeequipment::identify(nullptr,wp,p.weapon_property)||!nativeequipment::identify(nullptr,ar,p.draw_property)||
       !nativeequipment::identify(nullptr,rv,p.result_property))return false;
    p.serial=l.serial;p.valid=true;parent=p;return true;
}
state::Lease& drawing_lease() noexcept {
    return state::lease.acquired?state::lease:completed.active?completed.schema:state::lease;
}
bool drawing_owned(state::Lease const& l) noexcept {
    return (&l==&state::lease&&l.acquired&&l.ui_owned&&l.ui_replaced&&l.hud_owned)||
        (&l==&completed.schema&&completed.active);
}
bool result_current(state::Lease const& l) noexcept {
    if(&l!=&completed.schema)return true;
    float now{},flash{};unsigned char result{};
    const auto clock=weaponhand::clock_ns(nullptr);
    bool current=true;unsigned gate{};
    const auto check=[&](unsigned stage,bool value)noexcept{if(current&&!value){current=false;gate=stage;}return current;};
    if(check(1,completed.active&&weaponhand::current_gt(nullptr)))
        check(2,weaponhand::client.registration_alive(l.identity.registration,l.identity.native_start));
    if(current)check(3,vrequip::model.hold_generation()==l.identity.hold_generation&&
        vrequip::model.held_for(l.identity.world.address,l.identity.pawn.address,l.identity.weapon.address));
    if(current)check(4,object(l.identity.world)&&object(l.identity.pawn)&&object(l.identity.weapon)&&state::ui_hud_current(l)&&
        playground::pointer(l.identity.pawn.address,L"Weapon")==l.identity.weapon.address);
    if(current)check(5,copy(&now,l.identity.world.address+l.ui_clock_offsets[1],4)&&std::isfinite(now)&&
        copy(&flash,l.ui_hud.address+l.hud_flash_offset,4)&&std::isfinite(flash)&&flash>0&&flash<=60);
    if(current)check(6,now>=completed.completed&&now-completed.completed<flash&&
        clock>=completed.completed_ns&&clock-completed.completed_ns<60000000000LL);
    if(current)check(7,copy(&result,l.ui_hud.address+l.hud_result_offset,1)&&result==2);
    if(!current)completed.cancel_gate=gate;
    if(!current)completed.active=false;
    return current;
}
bool parent_current() noexcept {
    const auto& l=completed.schema;Array script{};
    if(!parent.valid||parent.serial!=l.serial||!object(parent.function)||!object(parent.query)||
       !object(parent.weapon_property)||!object(parent.draw_property)||!object(parent.result_property)||
       get<unsigned>(parent.function.address+0x88)!=parent.extent||
       !copy(&script,parent.function.address+0x8c,sizeof(script))||script.data!=parent.script.data||
       script.count!=parent.script.count||script.capacity!=parent.script.capacity)return false;
    const auto field=[&](hand::ObjectIdentity const& id,uptr owner,const wchar_t* kind,unsigned width,unsigned offset,
                        unsigned long long flags)noexcept{
        return get<uptr>(id.address+0x40)==owner&&playground::named(get<uptr>(id.address+0x50),kind)&&
            get<unsigned>(id.address+0x70)==1&&get<unsigned>(id.address+0x74)==width&&
            get<unsigned>(id.address+0x94)==offset&&(get<unsigned long long>(id.address+0x78)&0x580)==flags;
    };
    return field(parent.weapon_property,parent.function.address,L"ObjectProperty",8,parent.weapon_offset,0x80)&&
        field(parent.draw_property,parent.function.address,L"BoolProperty",4,parent.draw_offset,0x80)&&
        field(parent.result_property,parent.query.address,L"BoolProperty",4,0,0x580)&&
        get<unsigned>(parent.draw_property.address+0xd0)==parent.draw_mask&&get<unsigned>(parent.result_property.address+0xd0)==1;
}
bool parent_arguments(ParentQuery const& s,bool returned) noexcept {
    gears_ui_call_boundary::Frame frame{};uptr locals{},weapon{};unsigned ar{};
    const auto& l=completed.schema;
    if(!gears_ui_call_boundary::frame(weaponshot::read,s.call.caller_frame,frame)||frame.node!=parent.function.address||
       frame.object!=l.ui_hud.address||frame.previous!=s.frame.previous||frame.code!=s.frame.code+(returned?1:0)||
       !copy(&locals,s.call.caller_frame+0x30,8)||locals!=s.locals||!locals||locals>UINTPTR_MAX-parent.extent||
       !copy(&weapon,locals+parent.weapon_offset,8)||weapon!=l.identity.weapon.address||
       !copy(&ar,locals+parent.draw_offset,4)||!(ar&parent.draw_mask))return false;
    return true;
}
void begin_parent(abi::Invocation const& call) noexcept {
    // Only this original HUD expression receives a presentation result. No
    // weapon/Pawn IsReloading state is retained or changed, including the
    // second same-function query which orders ammunition-icon drawing.
    if(!completed.active||!parent.valid||call.callee!=parent.query.address||call.self!=completed.schema.identity.weapon.address)return;
    ++parent.query_calls;
    if(call.hand_cookie!=completed.schema.identity.registration||call.native_started_qpc!=completed.schema.identity.native_start||
       !call.result||(call.result&3)||!result_current(completed.schema)||!parent_current())return;
    ParentQuery s{};s.call=call;
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,s.frame)||s.frame.node!=parent.function.address||
       s.frame.object!=completed.schema.ui_hud.address||s.frame.code<parent.script.data+gears_lancer_reload_parent::prefix||
       s.frame.code-parent.script.data>unsigned(parent.script.count)-(s.bytes.size()-gears_lancer_reload_parent::prefix)||
       !copy(&s.locals,call.caller_frame+0x30,8)||!parent_arguments(s,false)||
       !copy(s.bytes.data(),s.frame.code-gears_lancer_reload_parent::prefix,s.bytes.size())||
       !gears_lancer_reload_parent::direct_bar_gate(s.bytes,unsigned(s.frame.code-parent.script.data)-gears_lancer_reload_parent::prefix,
           unsigned(parent.script.count),parent.weapon_property.address,parent.query.address,parent.draw_property.address,
           completed.schema.ui_draw_function.address))return;
    ++parent.gate_matches;
    for(auto& slot:parent_queries)if(!slot.active){s.active=true;slot=s;return;}
}
bool return_parent(abi::Invocation const& call) noexcept {
    ParentQuery* slot=nullptr;for(auto& s:parent_queries)if(s.active&&s.call.call_id==call.call_id){slot=&s;break;}
    if(!slot)return true;const auto s=*slot;*slot={};
    std::array<unsigned char,gears_lancer_reload_parent::span> bytes{};
    if(!abi::same(s.call,call)||!result_current(completed.schema)||!parent_current()||!parent_arguments(s,true)||
       !copy(bytes.data(),s.frame.code-gears_lancer_reload_parent::prefix,bytes.size())||bytes!=s.bytes)return true;
    unsigned before_result{},after{};if(!copy(&before_result,call.result,4)||before_result>1||before_result==1)return true;
    const unsigned visible=1;
    if(state::ui_store(call.result,&visible,4)&&copy(&after,call.result,4)&&after==1){++parent.result_updates;return true;}
    if(!copy(&after,call.result,4))return false;
    return after==before_result||(after==visible&&state::ui_store(call.result,&before_result,4));
}
void stage_completion() noexcept {
    const auto& l=state::lease;
    if(completed.staged&&completed.schema.serial==l.serial)return;
    completed={};
    if(!l.acquired||!l.ui_owned||!l.ui_replaced||!l.hud_owned||!state::ui_hud_current(l))return;
    completed.schema=l;
    completed.schema.acquired=completed.schema.ui_owned=completed.schema.hud_owned=completed.schema.attempts_owned=false;
    completed.schema.ui_display_scopes=completed.schema.ui_display_restores=0;
    completed.schema.ui_canvas_observations=completed.schema.ui_body_observations=completed.schema.ui_shape_updates=0;
    completed.staged=true;
}
void confirm_completion(lancerreload_inventory::Identity const& identity,std::uint64_t serial) noexcept {
    if(!completed.staged||completed.active||completed.schema.serial!=serial||!(completed.schema.identity==identity))return;
    auto const& l=completed.schema;
    unsigned char result{};
    if(!object(l.identity.world)||!object(l.identity.pawn)||!object(l.identity.weapon)||!state::ui_hud_current(l)||
       !copy(&result,l.ui_hud.address+l.hud_result_offset,1)||result!=2||
       !copy(&completed.completed,l.identity.world.address+l.ui_clock_offsets[1],4)||
       !std::isfinite(completed.completed)||completed.completed<0)return;
    completed.completed_ns=weaponhand::clock_ns(nullptr);completed.active=true;
    if(!result_current(l))return;
    if(log_file){fwprintf(log_file,L"lancer_reload_result_display serial=%llu original_hud_result=2 completed=%.6g inventory_permission=0\n",
        serial,completed.completed);fflush(log_file);}
}
bool prepare(hand::Frame const& source) noexcept {
    completed.active=false; // A new transaction cannot resurrect the old result.
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
    const std::array<uptr,2> owners{{source.weapon.address,source.world.address}};
    const wchar_t* clock_names[]={L"ReloadStartTime",L"TimeSeconds"};
    for(unsigned i=0;i<2;++i){
        const auto owner=owners[i],property=playground::property(owner,clock_names[i]);
        if(!registered(property))return false;
        const auto extent=get<unsigned>(get<uptr>(owner+0x50)+0x88),at=get<unsigned>(property+0x94);
        if(!playground::named(get<uptr>(property+0x50),L"FloatProperty")||
           get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=4||at%4||
           at>extent||4>extent-at||!nativeequipment::identify(nullptr,property,l.ui_clock_properties[i]))return false;
        l.ui_clock_offsets[i]=at;
    }
    const wchar_t* position_names[]={L"DrawX",L"DrawY",L"Scale"};
    for(unsigned i=0;i<3;++i){
        if(!layout_field(draw.r.function,position_names[i],L"FloatProperty",4,i)||
           (get<unsigned long long>(l.ui_layout_properties[i].address+0x78)&0x580)!=0x80)return false;
        for(unsigned j=0;j<4;++j)if(l.ui_layout_offsets[i]==l.ui_local_offsets[j])return false;
        for(unsigned j=0;j<i;++j)if(l.ui_layout_offsets[i]==l.ui_layout_offsets[j])return false;
    }
    const auto hud_class=l.ui_hud.klass;
    if(!layout_field(hud_class,L"Canvas",L"ObjectProperty",8,3)||
       !layout_field(hud_class,L"ResScaleSmall",L"FloatProperty",4,6))return false;
    const auto canvas_class=get<uptr>(l.ui_layout_properties[3].address+0xd0);
    if(!registered(canvas_class)||!playground::named(canvas_class,L"Canvas")||
       !nativeequipment::identify(nullptr,canvas_class,l.ui_canvas_class)||
       !layout_field(canvas_class,L"ClipX",L"FloatProperty",4,4)||
       !layout_field(canvas_class,L"ClipY",L"FloatProperty",4,5))return false;
    const auto setpos=playground::field(canvas_class,L"SetPos");
    if(!registered(setpos)||!playground::named(get<uptr>(setpos+0x50),L"Function")||
       !nativeequipment::identify(nullptr,setpos,l.ui_setpos_function))return false;
    const wchar_t* body_names[]={L"Alpha",L"FadeAlphaPercent",L"FlashAlphaPercent",L"BarLength",L"BarHeight"};
    for(unsigned i=0;i<5;++i)if(!layout_field(draw.r.function,body_names[i],i?L"FloatProperty":L"IntProperty",4,9+i))return false;
    const auto bar=playground::property(l.ui_hud.address,L"ARBarData");
    if(!registered(bar)||!playground::named(get<uptr>(bar+0x50),L"StructProperty"))return false;
    const auto bar_class=get<uptr>(bar+0xd0),background=playground::field(bar_class,L"Background");
    if(!registered(background)||!playground::named(get<uptr>(background+0x50),L"StructProperty"))return false;
    const auto texture_class=get<uptr>(background+0xd0);
    const auto bar_start=get<unsigned>(bar+0x94),texture_at=get<unsigned>(background+0x94);
    if(!registered(texture_class)||bar_start>UINT_MAX-texture_at)return false;
    const unsigned texture_start=bar_start+texture_at;
    if(!layout_field(texture_class,L"UL",L"FloatProperty",4,7,texture_start)||
       !layout_field(texture_class,L"VL",L"FloatProperty",4,8,texture_start))return false;
    const auto hud_extent=get<unsigned>(hud_class+0x88);
    for(unsigned i:{7u,8u})if(l.ui_layout_offsets[i]>hud_extent||4>hud_extent-l.ui_layout_offsets[i])return false;
    l.ui_canvas_known=[&]() noexcept {
        const wchar_t* names[]={L"CurX",L"CurY",L"OrgX",L"OrgY",L"DrawColor"};
        const auto extent=get<unsigned>(canvas_class+0x88);
        for(unsigned i=0;i<5;++i){
            const auto prop=playground::field(canvas_class,names[i]);
            if(!registered(prop)||!playground::named(get<uptr>(prop+0x50),i==4?L"StructProperty":L"FloatProperty")||
               get<unsigned>(prop+0x70)!=1||get<unsigned>(prop+0x74)!=4)return false;
            const auto at=get<unsigned>(prop+0x94);
            if(at%4||at>extent||4>extent-at||!nativeequipment::identify(nullptr,prop,l.ui_canvas_properties[i]))return false;
            if(i==4&&!playground::named(get<uptr>(prop+0xd0),L"Color"))return false;
            l.ui_canvas_offsets[i]=at;
        }
        return true;
    }();
    if(occupied!=15||!state::ui_hud_current())return false;
    prepare_parent(l); // Optional presentation path must not block physical reload.
    return true;
}
struct Slot {
    abi::Invocation call{};gears_ui_call_boundary::Frame frame{};uptr locals{};
    std::uint64_t serial{};std::array<unsigned char,37> code{};
    std::array<uptr,4> destinations{};std::array<float,4> desired{};
    std::uint64_t draw_call_id{};bool active{};
};
std::array<Slot,4> slots{};
struct DrawScope {
    abi::Invocation call{};lancerreload_inventory::Identity identity{};
    hand::ObjectIdentity hud{};std::array<hand::ObjectIdentity,2> properties{};
    std::array<unsigned,2> offsets{};gears_hud_action_layout::Scope scope{};
    gears_hud_action_layout::FloatScope<1> marker_scope{};
    hand::ObjectIdentity resolution_property{};unsigned resolution_offset{};
    std::uint64_t serial{};float body_height{};bool shape_ready{},shaped{},active{},completed_result{};
};
std::array<DrawScope,4> drawings{};
bool clock_current(DrawScope const& s) noexcept {
    if(!weaponhand::current_gt(nullptr)||!object(s.identity.world)||!object(s.identity.weapon)||!object(s.hud))return false;
    for(unsigned i=0;i<2;++i)if(!object(s.properties[i])||
       get<unsigned>(s.properties[i].address+0x94)!=s.offsets[i]||
       get<unsigned>(s.properties[i].address+0x70)!=1||get<unsigned>(s.properties[i].address+0x74)!=4||
       !playground::named(get<uptr>(s.properties[i].address+0x50),L"FloatProperty"))return false;
    return s.scope.addresses[0]==s.identity.weapon.address+s.offsets[0];
}
bool read_float(uptr p,float& out) noexcept {return copy(&out,p,4)&&std::isfinite(out);}
bool write_float(uptr p,float value) noexcept {return state::ui_store(p,&value,4);}
bool marker_current(DrawScope const& s) noexcept {
    const auto p=s.resolution_property.address;
    return weaponhand::current_gt(nullptr)&&object(s.hud)&&object(s.resolution_property)&&
        get<unsigned>(p+0x94)==s.resolution_offset&&get<unsigned>(p+0x70)==1&&
        get<unsigned>(p+0x74)==4&&playground::named(get<uptr>(p+0x50),L"FloatProperty")&&
        s.marker_scope.addresses[0]==s.hud.address+s.resolution_offset;
}
bool layout_descriptors_current(bool include_canvas_receipt=false) noexcept {
    namespace meta=weaponhand::meta;const auto& l=drawing_lease();
    meta::Table objects{},names{},after_objects{},after_names{};
    if(!meta::tables(base,objects,names))return false;
    const auto current=[&](hand::ObjectIdentity const& expected) noexcept {
        meta::Identity a{},b{};meta::NameText at{},bt{};
        if(!hand::identity_valid(expected)||!meta::raw_identity(objects,names,expected.address,a,at)||
           !meta::raw_identity(objects,names,expected.address,b,bt)||!meta::same_identity(a,b)||!meta::same_name_text(at,bt))return false;
        return hand::same(hand::ObjectIdentity{a.address,a.outer,a.klass,a.index,a.name.index,a.name.number,1},expected);
    };
    if(!current(l.ui_canvas_class)||!current(l.ui_setpos_function))return false;
    for(unsigned i=0;i<14;++i){const auto& field=l.ui_layout_properties[i];
        if(!current(field)||get<unsigned>(field.address+0x94)!=l.ui_layout_raw_offsets[i]||
           get<unsigned>(field.address+0x70)!=1||get<unsigned>(field.address+0x74)!=(i==3?8u:4u))return false;
    }
    if(include_canvas_receipt){
        if(!l.ui_canvas_known)return false;
        for(unsigned i=0;i<5;++i){const auto& field=l.ui_canvas_properties[i];
            if(!current(field)||get<unsigned>(field.address+0x94)!=l.ui_canvas_offsets[i]||
               get<unsigned>(field.address+0x70)!=1||get<unsigned>(field.address+0x74)!=4)return false;
        }
    }
    return meta::tables(base,after_objects,after_names)&&meta::same_table(objects,after_objects)&&meta::same_table(names,after_names);
}
bool place(uptr locals,DrawScope* drawing) noexcept {
    auto& l=drawing_lease();
    if(!layout_descriptors_current())return true;
    const auto canvas=get<uptr>(l.ui_hud.address+l.ui_layout_offsets[3]);hand::ObjectIdentity identity{};
    if(!nativeequipment::identify(nullptr,canvas,identity)||identity.klass!=l.ui_canvas_class.address)return true;
    std::array<float,5> dimensions{};
    const std::array<uptr,5> addresses{{canvas+l.ui_layout_offsets[4],canvas+l.ui_layout_offsets[5],
        l.ui_hud.address+l.ui_layout_offsets[7],l.ui_hud.address+l.ui_layout_offsets[8],l.ui_hud.address+l.ui_layout_offsets[6]}};
    for(unsigned i=0;i<5;++i)if(!read_float(addresses[i],dimensions[i]))return true;
    bool inherited_marker=drawing&&drawing->marker_scope.owned;
    for(auto const& draw:drawings)if(&draw!=drawing&&draw.active&&draw.marker_scope.owned&&
        hand::same(draw.hud,l.ui_hud))inherited_marker=true;
    gears_lancer_reload_display::Layout p{};
    if(!gears_lancer_reload_display::layout(dimensions[0],dimensions[1],dimensions[2],dimensions[3],dimensions[4],p,
        drawing&&!inherited_marker))return true;
    const std::array<float,3> wanted{{p.x,p.y,p.scale}};std::array<float,3> before{};
    for(unsigned i=0;i<3;++i)if(!read_float(locals+l.ui_layout_offsets[i],before[i]))return true;
    if(drawing&&p.resolution_scale!=dimensions[4]){
        drawing->resolution_property=l.ui_layout_properties[6];drawing->resolution_offset=l.ui_layout_offsets[6];
        drawing->marker_scope.addresses={{addresses[4]}};
        drawing->marker_scope.before={{dimensions[4]}};drawing->marker_scope.wanted={{p.resolution_scale}};
        if(!marker_current(*drawing)||!drawing->marker_scope.apply(read_float,write_float))return !drawing->marker_scope.owned;
        ++l.ui_marker_scopes;l.ui_marker_resolution=p.resolution_scale;
    }
    for(unsigned i=0;i<3;++i){float checked{};
        if(write_float(locals+l.ui_layout_offsets[i],wanted[i])&&read_float(locals+l.ui_layout_offsets[i],checked)&&checked==wanted[i])continue;
        bool restored=true;
        for(unsigned j=0;j<=i;++j){float current{};const auto at=locals+l.ui_layout_offsets[j];
            if(!read_float(at,current)||(current!=before[j]&&(current!=wanted[j]||!write_float(at,before[j]))))restored=false;}
        if(drawing&&drawing->marker_scope.owned){
            if(!marker_current(*drawing)||!drawing->marker_scope.restore(read_float,write_float))restored=false;
            else ++l.ui_marker_restores;
        }
        return restored;
    }
    ++l.ui_layout_updates;l.ui_layout_last={{p.x,p.y,p.scale,p.width,p.height}};
    if(drawing){drawing->body_height=p.height/p.resolution_scale;drawing->shape_ready=true;}
    return true;
}
// The script computes BarHeight only AFTER GetActiveReloadValues returns.
// Its first SetPos return is before the background/region DrawTile calls and
// before marker-centre calculation. Adapt that exact frame-local height once;
// all original tiles retain their original texture/UV and agree on the height.
// This step changes only the frame-local height. The marker has its separate
// invocation-sized HUD scale journal; no Canvas scale or atlas UV is changed.
bool shape_body(abi::Invocation const& call) noexcept {
    auto& l=drawing_lease();
    if(call.callee!=l.ui_setpos_function.address||!drawing_owned(l)||!result_current(l)||
       call.hand_cookie!=l.identity.registration||call.native_started_qpc!=l.identity.native_start||
       call.self!=get<uptr>(l.ui_hud.address+l.ui_layout_offsets[3]))return true;
    gears_ui_call_boundary::Frame frame{};uptr locals{};
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,frame)||
       frame.node!=l.ui_draw_function.address||frame.object!=l.ui_hud.address)return true;
    DrawScope* drawing=nullptr;
    for(auto& draw:drawings)if(draw.active&&draw.serial==l.serial&&draw.shape_ready&&!draw.shaped&&
        draw.call.call_id<call.call_id&&draw.call.caller_frame==frame.previous){drawing=&draw;break;}
    if(!drawing)return true;
    if(!state::ui_hud_current(l)||!layout_descriptors_current()||
       !copy(&locals,call.caller_frame+0x30,8)||!locals||locals>UINTPTR_MAX-l.ui_locals_extent||
       !std::isfinite(drawing->body_height)||drawing->body_height<=0)return true;
    const auto address=locals+l.ui_layout_offsets[13];float before{},after{};
    if(!read_float(address,before)||before<0)return true;
    const float wanted=drawing->body_height;
    if(write_float(address,wanted)&&read_float(address,after)&&after==wanted){
        drawing->shaped=true;++l.ui_shape_updates;l.ui_shape_last=wanted;return true;
    }
    if(!read_float(address,after))return false;
    return after==before||(after==wanted&&write_float(address,before));
}
void observe_body(abi::Invocation const& call) noexcept {
    auto& l=drawing_lease();
    if(call.callee!=l.ui_setpos_function.address||!drawing_owned(l)||!result_current(l)||!l.ui_layout_updates||
       l.ui_body_gate==l.ui_draw_gate||
       call.hand_cookie!=l.identity.registration||call.native_started_qpc!=l.identity.native_start||
       call.self!=get<uptr>(l.ui_hud.address+l.ui_layout_offsets[3]))return;
    gears_ui_call_boundary::Frame frame{};uptr locals{};
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,frame)||
       frame.node!=l.ui_draw_function.address||frame.object!=l.ui_hud.address||
       !copy(&locals,call.caller_frame+0x30,8)||!locals)return;
    bool enclosed=false;
    for(auto const& draw:drawings)if(draw.active&&draw.serial==l.serial&&draw.call.call_id<call.call_id&&
        draw.call.caller_frame==frame.previous)enclosed=true;
    if(!enclosed||!layout_descriptors_current())return;
    std::array<float,7> values{};const unsigned fields[]={0,1,2,12,10,11};
    for(unsigned i=0;i<6;++i)if(!read_float(locals+l.ui_layout_offsets[fields[i]],values[i]))return;
    int alpha{};if(!copy(&alpha,locals+l.ui_layout_offsets[9],4)||alpha<0||alpha>255)return;values[6]=float(alpha);
    l.ui_body_last=values;l.ui_body_gate=l.ui_draw_gate;++l.ui_body_observations;
    if(l.ui_canvas_known)l.ui_canvas_call=call.call_id;
}
void observe_canvas_return(abi::Invocation const& call) noexcept {
    auto& l=drawing_lease();
    if(!l.ui_canvas_call||l.ui_canvas_call!=call.call_id)return;
    l.ui_canvas_call=0;
    if(!drawing_owned(l)||!result_current(l)||call.callee!=l.ui_setpos_function.address||
       call.hand_cookie!=l.identity.registration||call.native_started_qpc!=l.identity.native_start||
       call.self!=get<uptr>(l.ui_hud.address+l.ui_layout_offsets[3])||!layout_descriptors_current(true))return;
    std::array<float,7> values{};unsigned color{};
    for(unsigned i=0;i<4;++i)if(!read_float(call.self+l.ui_canvas_offsets[i],values[i]))return;
    if(!read_float(call.self+l.ui_layout_offsets[4],values[4])||
       !read_float(call.self+l.ui_layout_offsets[5],values[5])||
       !copy(&color,call.self+l.ui_canvas_offsets[4],4))return;
    values[6]=float(color>>24);l.ui_canvas_last=values;l.ui_canvas_color=color;++l.ui_canvas_observations;
}
void begin_drawing(abi::Invocation const& call) noexcept {
    const auto& l=drawing_lease();
    if(&l==&completed.schema&&call.self==l.ui_hud.address&&call.callee==l.ui_draw_function.address)++completed.draw_calls;
    // This callback receives EVERY script call. Reject unrelated addresses
    // before metadata/liveness reads, not after five HUD object lookups.
    if(call.self!=l.ui_hud.address||call.callee!=l.ui_draw_function.address||
       !drawing_owned(l)||!result_current(l)||
       call.hand_cookie!=l.identity.registration||call.native_started_qpc!=l.identity.native_start||
       !state::ui_hud_current(l))return;
    for(auto& s:drawings)if(!s.active){
        s={};s.call=call;s.identity=l.identity;s.hud=l.ui_hud;s.properties=l.ui_clock_properties;
        s.offsets=l.ui_clock_offsets;s.serial=l.serial;
        s.active=true;
        s.completed_result=&l==&completed.schema;
        s.scope.addresses={{l.identity.weapon.address+s.offsets[0],l.ui_hud.address+l.hud_tick_offset}};
        s.active=true;return;
    }
}
bool binding(abi::Invocation const& call) noexcept {
    const auto& l=drawing_lease();
    if(&l==&completed.schema&&call.self==l.identity.weapon.address&&call.callee==l.ui_getter_function.address)++completed.getter_calls;
    if(call.self!=l.identity.weapon.address||call.callee!=l.ui_getter_function.address||
       !weaponhand::current_gt(nullptr)||!drawing_owned(l)||!result_current(l)||
       call.hand_cookie!=l.identity.registration||call.native_started_qpc!=l.identity.native_start||
       !state::ui_hud_current(l)||!object(l.identity.weapon)||!object(l.identity.world)||!object(l.identity.pawn)||
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
    begin_parent(call);
    observe_body(call);
    begin_drawing(call);
    if(!binding(call))return;
    auto& l=drawing_lease();Slot s{};l.ui_draw_gate=1;
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
    // GetActiveReloadValues is inside this exact DrawActionReloadBar frame.
    // Only its enclosing invocation may borrow the presentation clock.
    for(auto const& draw:drawings)if(draw.active&&draw.serial==l.serial&&
       draw.call.self==s.frame.object&&draw.call.caller_frame==s.frame.previous&&
       draw.call.call_id<call.call_id)s.draw_call_id=draw.call.call_id;
    s.call=call;s.serial=l.serial;s.active=true;
    for(auto& slot:slots)if(!slot.active){slot=s;return;}
}
bool returned(abi::Invocation const& call) noexcept {
    if(!return_parent(call))return false;
    if(!shape_body(call))return false;
    observe_canvas_return(call);
    Slot* found=nullptr;for(auto& slot:slots)if(slot.active&&slot.call.call_id==call.call_id){found=&slot;break;}
    if(!found)return true;const auto s=*found;*found={};
    if(!abi::same(s.call,call)||s.serial!=drawing_lease().serial||!binding(call))return true;
    gears_ui_call_boundary::Frame after{};uptr locals{};std::array<unsigned char,37> code{};
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,after)||
       after.node!=s.frame.node||after.object!=s.frame.object||after.previous!=s.frame.previous||after.code!=s.frame.code+s.code.size()||
       !copy(&locals,call.caller_frame+0x30,8)||locals!=s.locals||!copy(code.data(),s.frame.code,code.size())||code!=s.code)return true;
    auto& l=drawing_lease();l.ui_draw_gate=4;
    auto desired=s.desired;
    DrawScope* drawing=nullptr;
    for(auto& draw:drawings)if(draw.active&&draw.call.call_id==s.draw_call_id){drawing=&draw;break;}
    gears_lancer_reload_display::Plan display{};float now{},fade{},flash{};
    const bool result_display=&l==&completed.schema;
    const bool readable=drawing&&!drawing->scope.owned&&clock_current(*drawing)&&
        read_float(l.identity.world.address+l.ui_clock_offsets[1],now)&&
        read_float(l.ui_hud.address+l.hud_fade_offset,fade)&&
        read_float(l.ui_hud.address+l.hud_flash_offset,flash)&&
        read_float(drawing->scope.addresses[0],drawing->scope.before[0])&&
        (result_display||drawing->scope.before[0]==l.reward_start)&&
        read_float(drawing->scope.addresses[1],drawing->scope.before[1]);
    float duration{};
    const bool display_ready=readable&&(result_display?
        (state::ui_float(l.identity.weapon.address,L"ReloadDuration",duration)&&
         gears_lancer_reload_display::result_plan(now,completed.completed,duration,fade,flash,display)):
        gears_lancer_reload_display::plan(now,l.reward_start,l.ui_wanted[0],fade,flash,desired,display));
    if(display_ready)desired=display.window;
    std::array<float,4> original_values{},checked{};
    for(unsigned i=0;i<4;++i)if(!copy(&original_values[i],s.destinations[i],4)||!std::isfinite(original_values[i]))return true;
    unsigned changed=0;
    for(;changed<4;++changed){
        if(!state::ui_store(s.destinations[changed],&desired[changed],4)||
           !copy(&checked[changed],s.destinations[changed],4)||checked[changed]!=desired[changed])break;
    }
    if(changed!=4){
        bool restored=true;
        for(unsigned i=0;i<=changed&&i<4;++i){float value{};
            if(!copy(&value,s.destinations[i],4)){restored=false;continue;}
            if(value==original_values[i])continue;
            if(value!=desired[i]||!state::ui_store(s.destinations[i],&original_values[i],4))restored=false;
        }
        return restored;
    }
    ++l.ui_draw_updates;l.ui_draw_gate=5;
    l.ui_draw_original=original_values;l.ui_draw_current=checked;
    if(display_ready){
        drawing->scope.wanted={{display.draw_start,display.marker}};
        if(!drawing->scope.apply(read_float,write_float))return !drawing->scope.owned;
        ++l.ui_display_scopes;if(display.ordinary)++l.ui_display_late;
        l.ui_display_last={{l.reward_start,display.draw_start,now-l.reward_start,display.marker}};
        l.ui_fade_durations={{fade,flash}};
        // One bounded normal-GT receipt when the display becomes ordinary.
        // This is presentation only; no reward or mechanical completion.
        l.ui_draw_gate=display.ordinary?7:6;
    }
    return place(s.locals,drawing);
}
bool cleanup(abi::Invocation const& call) noexcept {
    for(auto& slot:parent_queries)if(slot.active&&slot.call.call_id==call.call_id)slot={};
    for(auto& slot:slots)if(slot.active&&slot.call.call_id==call.call_id)slot={};
    for(auto& draw:drawings)if(draw.active&&draw.call.call_id==call.call_id){
        const bool owed=draw.scope.owned!=0;
        const bool marker_owed=draw.marker_scope.owned!=0;
        if(!abi::same(draw.call,call))return false;
        const bool clocks=!draw.scope.owned||(clock_current(draw)&&draw.scope.restore(read_float,write_float));
        const bool marker=!draw.marker_scope.owned||(marker_current(draw)&&draw.marker_scope.restore(read_float,write_float));
        if(!clocks||!marker)return false;
        auto& owner=draw.completed_result?completed.schema:state::lease;
        if(owed&&draw.serial==owner.serial)++owner.ui_display_restores;
        if(marker_owed&&draw.serial==owner.serial)++owner.ui_marker_restores;
        draw={};
    }
    return true;
}
bool stopped() noexcept {
    for(auto const& slot:parent_queries)if(slot.active)return false;
    for(auto const& draw:drawings)if(draw.active||draw.scope.owned||draw.marker_scope.owned)return false;
    for(auto const& slot:slots)if(slot.active)return false;
    return true;
}
void service() noexcept {
    if(!completed.staged||!completed.completed_ns||completed.end_reported)return;
    if(completed.active)result_current(completed.schema);
    if(completed.active)return;
    for(auto const& draw:drawings)if(draw.active&&draw.completed_result)return;
    completed.end_reported=true;
    const auto& l=completed.schema;
    if(log_file){fwprintf(log_file,L"lancer_reload_result_display_end serial=%llu scopes=%llu restores=%llu height_updates=%llu body_height=%.6g draw_calls=%llu getter_calls=%llu cancel_gate=%u parent_prepared=%u parent_query_calls=%llu parent_gate_matches=%llu parent_result_updates=%llu\n",
        l.serial,l.ui_display_scopes,l.ui_display_restores,l.ui_shape_updates,l.ui_shape_last,
        completed.draw_calls,completed.getter_calls,completed.cancel_gate,unsigned(parent.valid),parent.query_calls,parent.gate_matches,parent.result_updates);fflush(log_file);}
}
} // namespace lancerreload_ui
