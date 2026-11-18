// The sole shared R2 owner supplies its already-journalled serial and owns the
// press/release schedule. This adapter certifies the ORIGINAL WarPC functions;
// it never reads raw input, synthesizes an edge or releases after source loss.
namespace vrfire {extern Name r2;bool name_current(Name const&,wchar_t) noexcept;}
namespace torquebow_buttons {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
using Result=gears_weapon::TorqueBowCallResult;
struct Entry {
    hand::ObjectIdentity controller{},function{};bool press{};
    unsigned bytes{};std::uint64_t fingerprint{};
};
const wchar_t* label(bool press) noexcept {return press?L"ButtonPress":L"ButtonRelease";}
bool bind(hand::ObjectIdentity const& controller,bool press,Entry& out) noexcept {
    out={};if(!weaponhand::current_gt(nullptr))return false;
    playground::Args args(controller.address,label(press));
    if(!torquebow_original::original_function(args,L"WarPC",L"WarfareGame")||args.r.size!=8||
       !torquebow_original::input_property(args,L"ButtonName",L"NameProperty",8,0))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_fields};
    r.reuse_initial_identities=true; // This bind's discovery only; closing below reads fresh.
    meta::Structure body{};meta::Table script{};std::uint16_t bytes{},returned{},native{};unsigned flags{};uptr exec{};
    Entry value{};value.controller=controller;value.press=press;
    std::uint64_t script_hash{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,controller)||
       !lancermain_asset::identify(r,args.r.function,value.function)||!r.structure(args.r.function,body)||
       !r.at(args.r.function,0x106,bytes)||bytes!=8||!r.at(args.r.function,0x108,returned)||returned!=0xffff||
       !r.at(args.r.function,0xf8,native)||native||!r.at(args.r.function,0xf4,flags)||(flags&0x400)||
       !r.at(args.r.function,0x114,exec)||exec!=base+0x1d6ca0||!r.at(args.r.function,0x8c,script)||
       script.count<=0||script.count>script.capacity||script.capacity>131072||
       !weaponshot::hash_script(script.data,unsigned(script.count),script_hash))return false;
    std::array<uptr,meta::max_fields> seen{};unsigned fields{},parameters{};
    for(auto child=body.children;child;){
        if(fields==seen.size())return false;
        for(unsigned i=0;i<fields;++i)if(seen[i]==child)return false;seen[fields++]=child;
        meta::Property p{};
        if(!r.property(child,p)||schema.identities[p.identity].outer!=args.r.function||
           !meta::property_fits(p,body.extent))return false;
        if(p.flags&0x80){
            if(++parameters!=1||!r.named(p.identity,L"ButtonName")||p.dimension!=1||p.offset||p.element_bytes!=8||
               (p.flags&0x590)!=0x80||!weaponhand::core_type(r,p.type,L"NameProperty")||
               schema.field_count==schema.fields.size())return false;
            schema.fields[schema.field_count++]=p;
        }
        child=p.next;
    }
    r.reuse_initial_identities=false;
    if(parameters!=1||!unified_weapon_fire::retained_metadata(r))return false;
    playground::Args again(controller.address,label(press));
    if(!again.valid||again.r.function!=args.r.function||again.r.size!=8)return false;
    unified_weapon_fire::Fingerprint hash;unified_weapon_fire::schema_fingerprint(schema,hash);
    hash.word(unsigned(press));hash.word(flags);hash.word(exec);hash.word(script.data);hash.word(unsigned(script.count));hash.word(script_hash);
    value.bytes=bytes;value.fingerprint=hash.value;out=value;return true;
}
bool same(Entry const& a,Entry const& b) noexcept {
    return a.fingerprint&&a.fingerprint==b.fingerprint&&a.press==b.press&&a.bytes==b.bytes&&
        hand::same(a.controller,b.controller)&&hand::same(a.function,b.function);
}
Result dispatch(hand::Frame const& f,Entry const& entry,std::uint64_t serial,
    bool(*owns)(hand::Frame const&,std::uint64_t) noexcept,
    bool(*muzzle_ready)(hand::Frame const&) noexcept) noexcept {
    // Both callbacks belong to the existing R2 owner and its common projectile
    // result consumer. A script declaration or a locally cached charge flag
    // is not an input lease or a current weapon-directed projectile source.
    if(!serial||!owns||!muzzle_ready||!enabled.load(std::memory_order_acquire)||
       !weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
       !nativeequipment::current(nullptr,f)||!playground::instance(f.weapon.address,L"Weap_Bow_base")||
       !hand::same(f.controller,entry.controller)||!owns(f,serial)||!vrfire::name_current(vrfire::r2,L'R'))return Result::rejected;
    Entry actual{};torquebow_state::Observed state{};
    if(!bind(f.controller,entry.press,actual)||!same(entry,actual)||!torquebow_state::read(f,state))return Result::rejected;
    if(entry.press){
        // Original BeginFire with zero loaded arrows may start a timed refill.
        // Only the physical reload owner can change that inventory; no empty
        // press is issued and no stock Charge/fatigue clock is overwritten.
        if(!state.active||!state.stopped()||state.inventory.capacity!=1||state.inventory.used!=0)return Result::rejected;
        for(auto method:{torquebow_cancel_live::Method::clear_pending,torquebow_cancel_live::Method::charge_ended,
                        torquebow_cancel_live::Method::active,torquebow_cancel_live::Method::put_down}){
            torquebow_cancel_live::Entry cleanup{};
            if(!torquebow_cancel_live::bind_entry(f.weapon.address,method,cleanup))return Result::rejected;
        }
    }else if(!state.input_down)return Result::accepted; // Independently observed original key already up.
    playground::Args button_call(f.controller.address,label(entry.press));
    if(!button_call.valid||button_call.r.function!=entry.function.address||button_call.r.size!=entry.bytes||
       !button_call.set(L"ButtonName",vrfire::r2).valid||!muzzle_ready(f)||
       !owns(f,serial)||!nativeequipment::current(nullptr,f)||!torquebow_input_fence::update(f,true))return Result::rejected;
    // The caller journals BEFORE entering here. A failed dispatched original
    // is unknown, never retried as though no action or ammo effect occurred.
    if(!owns(f,serial)||!nativeequipment::current(nullptr,f)||!muzzle_ready(f))return Result::rejected;
    return button_call.run()?Result::accepted:Result::unknown;
}
}
