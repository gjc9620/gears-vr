// Exact original no-argument Active-state empty cue, independent of fire/R2.
// The caller supplies current Bow/zero-reserve/quiet/fresh-intent permission.
namespace torquebow_empty_sound {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
struct Entry {
    hand::ObjectIdentity declaring{},active{},function{},cue{};
    uptr script{};unsigned script_bytes{},flags{};std::uint64_t script_hash{};
};
bool same(Entry const& a,Entry const& b) noexcept {
    return hand::same(a.declaring,b.declaring)&&hand::same(a.active,b.active)&&hand::same(a.function,b.function)&&
        hand::same(a.cue,b.cue)&&a.script==b.script&&a.script_bytes==b.script_bytes&&a.flags==b.flags&&a.script_hash==b.script_hash;
}
template<std::size_t N>bool child(meta::Reader& r,uptr owner,wchar_t const(&label)[N],uptr& out,bool state=false) noexcept {
    out=0;uptr first{};meta::Structure structure{};
    if(state){if(!r.at(owner,0x80,first))return false;}
    else {if(!r.structure(owner,structure))return false;first=structure.children;}
    std::array<uptr,meta::max_search> seen{};unsigned count{};
    for(auto p=first;p;){
        if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;seen[count++]=p;
        meta::Identity id{};meta::NameText text{};uptr next{};
        if(!meta::raw_identity(r.result.objects,r.result.names,p,id,text)||id.outer!=owner||!r.at(p,0x68,next))return false;
        if(meta::exact_text(text,label)){unsigned slot{};if(!r.path(p,slot)||!r.named(slot,label)||r.result.identities[slot].outer!=owner)return false;out=p;return true;}
        p=next;
    }return false;
}
bool read(hand::Frame const& f,Entry& out) noexcept {
    out={};if(!weaponhand::current_gt(nullptr))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_fields};
    unsigned actor{};uptr declaring{},state{},fn{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||
       !weaponhand::derives(r,f.weapon.address,L"Weap_Bow_base",L"WarfareGame")||!r.path(f.weapon.address,actor))return false;
    auto cls=schema.identities[actor].klass;std::array<uptr,meta::max_parents> seen{};unsigned count{};
    for(auto p=cls;p;){
        if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;seen[count++]=p;
        meta::Structure current{};unsigned kind{},package{};
        if(!r.structure(p,current)||!r.path(schema.identities[current.identity].klass,kind)||!weaponhand::core_type(r,kind,L"Class"))return false;
        if(r.named(current.identity,L"WarWeapon")){
            if(!r.path(schema.identities[current.identity].outer,package)||!r.named(package,L"WarfareGame")||schema.identities[package].outer)return false;
            declaring=p;break;
        }p=current.super;
    }
    Entry value{};unsigned state_slot{},state_kind{},fn_kind{};meta::Structure body{};
    if(!declaring||!child(r,declaring,L"Active",state)||!r.path(state,state_slot)||
       !r.path(schema.identities[state_slot].klass,state_kind)||!weaponhand::core_type(r,state_kind,L"State")||
       !child(r,state,L"PlayClickClickEmptyClip",fn,true)||!r.structure(fn,body)||body.children||body.extent||
       !r.path(schema.identities[body.identity].klass,fn_kind)||!weaponhand::core_type(r,fn_kind,L"Function"))return false;
    std::uint16_t bytes{},ret{},native{};uptr exec{};meta::Table script{};
    if(!r.at(fn,0x106,bytes)||bytes||!r.at(fn,0x108,ret)||ret!=0xffff||!r.at(fn,0xf8,native)||native||
       !r.at(fn,0xf4,value.flags)||(value.flags&0x400)||!r.at(fn,0x114,exec)||exec!=base+0x1d6ca0||
       !r.at(fn,0x8c,script)||script.count<=0||script.count>script.capacity||script.capacity>1024)return false;
    std::array<unsigned char,1024> code{},closing{};
    if(!copy(code.data(),script.data,unsigned(script.count)))return false;
    std::uint64_t hash=14695981039346656037ull;for(int i=0;i<script.count;++i){hash^=code[unsigned(i)];hash*=1099511628211ull;}
    meta::PointerField sound{},again_sound{};
    if(!weaponhand::read_pointer(r,f.weapon.address,L"FireNoAmmoSound",sound)||!sound.value||
       !weaponhand::derives(r,sound.value,L"SoundCue",L"Engine")||
       !lancermain_asset::identify(r,declaring,value.declaring)||!lancermain_asset::identify(r,state,value.active)||
       !lancermain_asset::identify(r,fn,value.function)||!lancermain_asset::identify(r,sound.value,value.cue))return false;
    r.reuse_initial_identities=false;uptr again_state{},again_fn{},again_exec{};std::uint16_t again_bytes{},again_ret{},again_native{};
    unsigned again_flags{};meta::Table again_script{};
    if(!child(r,declaring,L"Active",again_state)||again_state!=state||
       !child(r,state,L"PlayClickClickEmptyClip",again_fn,true)||again_fn!=fn||
       !weaponhand::read_pointer(r,f.weapon.address,L"FireNoAmmoSound",again_sound)||!meta::same_pointer(sound,again_sound)||
       !r.at(fn,0x106,again_bytes)||again_bytes!=bytes||!r.at(fn,0x108,again_ret)||again_ret!=ret||
       !r.at(fn,0xf8,again_native)||again_native!=native||!r.at(fn,0xf4,again_flags)||again_flags!=value.flags||
       !r.at(fn,0x114,again_exec)||again_exec!=exec||!r.at(fn,0x8c,again_script)||!meta::same_table(script,again_script)||
       !copy(closing.data(),script.data,unsigned(script.count))||std::memcmp(code.data(),closing.data(),unsigned(script.count))||
       !torquebow_arrow_source::retained(r))return false;
    value.script=script.data;value.script_bytes=unsigned(script.count);value.script_hash=hash;out=value;return true;
}
}
