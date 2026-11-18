// Original current Pawn reload notifies -> original local weapon sound call.
// No global object-name scan, cached action permit, timer or input writer.
namespace boomshot_endpoint_sound {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
enum class Cue:unsigned {opened,seated,closed};
struct Cues {hand::ObjectIdentity sequence{},package{};std::array<hand::ObjectIdentity,3> cue{};
    meta::Table notifies{};uptr field{},inner{},type{},children{};unsigned stride{};std::uint64_t bytes_hash{};
    bool operator==(Cues const& b)const noexcept {
        if(!hand::same(sequence,b.sequence)||!hand::same(package,b.package)||!meta::same_table(notifies,b.notifies)||
           field!=b.field||inner!=b.inner||type!=b.type||children!=b.children||stride!=b.stride||bytes_hash!=b.bytes_hash)return false;
        for(unsigned i=0;i<cue.size();++i)if(!hand::same(cue[i],b.cue[i]))return false;return true;
    }
};
bool retained(meta::Reader& r) noexcept {
    auto& s=r.result;r.reuse_initial_identities=false;const auto identities=s.identity_count,structures=s.structure_count,fields=s.field_count;
    for(unsigned i=0;i<identities;++i){unsigned slot{};if(!r.identity(s.identities[i].address,slot)||slot!=i)return false;}
    for(unsigned i=0;i<structures;++i){meta::Structure actual{};if(!r.structure(s.identities[s.structures[i].identity].address,actual)||!meta::same_structure(s.structures[i],actual))return false;}
    for(unsigned i=0;i<fields;++i){meta::Property actual{};if(!r.property(s.identities[s.fields[i].identity].address,actual)||!meta::same_property(s.fields[i],actual))return false;}
    for(unsigned i=0;i<s.name_count;++i){meta::NameText actual{};if(!meta::fresh_name(s.names,s.name_text[i].index,actual)||!meta::same_name_text(s.name_text[i],actual))return false;}
    meta::Table objects{},names{};return base==s.module_base&&meta::tables(base,objects,names)&&meta::same_table(objects,s.objects)&&meta::same_table(names,s.names)&&s.failure.reason==meta::Reason::none;
}
bool retain(meta::Reader& r,meta::Property const& p) noexcept {
    for(unsigned i=0;i<r.result.field_count;++i)if(r.result.fields[i].identity==p.identity)return meta::same_property(r.result.fields[i],p);
    if(r.result.field_count==r.result.fields.size())return false;r.result.fields[r.result.field_count++]=p;return true;
}
bool package_type(meta::Reader& r,unsigned object) noexcept {
    unsigned kind{};return r.path(r.result.identities[object].klass,kind)&&weaponhand::core_type(r,kind,L"Package");
}
bool cue_kind(meta::Reader& r,uptr object,unsigned& index,hand::ObjectIdentity& package) noexcept {
    unsigned slot{},group{},root{};index=3;package={};
    if(!r.path(object,slot)||!weaponhand::derives(r,object,L"SoundCue",L"Engine")||
       !r.path(r.result.identities[slot].outer,group)||!package_type(r,group)||
       !r.path(r.result.identities[group].outer,root)||!package_type(r,root)||
       !r.named(root,L"Weapon_Boomer")||r.result.identities[root].outer)return false;
    // Authoring mapping, not a claim that these new physical phases reproduce
    // the old timed notify schedule. All three are actual original reload cues.
    if(r.named(slot,L"BoomerFoley01Cue")&&r.named(group,L"Weapons"))index=0;
    else if(r.named(slot,L"BoomerInsertShellCue")&&r.named(group,L"Reloads"))index=1;
    else if(r.named(slot,L"BoomerFoley03Cue")&&r.named(group,L"Weapons"))index=2;
    return index<3&&lancermain_asset::identify(r,r.result.identities[root].address,package);
}
bool struct_members(meta::Reader& r,uptr type,unsigned stride,meta::Property& time,meta::Property& notify,uptr& child_head) noexcept {
    // The shared Reader::structure deliberately supports Class/Function only.
    // Read this exact ScriptStruct envelope locally, and close every field;
    // do not broaden that shared reader or pretend it accepted ScriptStruct.
    meta::Structure structure{};unsigned kind{},owner{},owner_kind{},package{};
    if(!r.path(type,structure.identity)||!r.at(type,0x60,structure.super)||structure.super||
       !r.at(type,0x80,structure.children)||!r.at(type,0x88,structure.extent)||structure.extent!=stride||!r.named(structure.identity,L"AnimNotifyEvent")||
       !r.path(r.result.identities[structure.identity].klass,kind)||!weaponhand::core_type(r,kind,L"ScriptStruct")||
       !r.path(r.result.identities[structure.identity].outer,owner)||!r.named(owner,L"AnimSequence")||
       !r.path(r.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Engine")||r.result.identities[package].outer)return false;
    std::array<uptr,meta::max_fields> seen{};unsigned count{},found{};std::array<unsigned,256> occupied{};
    for(auto child=structure.children;child;){
        if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;seen[count++]=child;
        meta::Property p{};if(!r.property(child,p)||r.result.identities[p.identity].outer!=type||p.dimension!=1||!meta::property_fits(p,stride)||!retain(r,p))return false;
        if(r.named(p.identity,L"Time")&&weaponhand::core_type(r,p.type,L"FloatProperty")&&p.element_bytes==4){if(found&1)return false;time=p;found|=1;}
        else if(r.named(p.identity,L"Notify")&&weaponhand::core_type(r,p.type,L"ObjectProperty")&&p.element_bytes==8){
            uptr cls{};unsigned slot{},parent{},class_kind{};
            if(found&2||!r.at(child,0xd0,cls)||!r.path(cls,slot)||!r.named(slot,L"AnimNotify")||
               !r.path(r.result.identities[slot].klass,class_kind)||!weaponhand::core_type(r,class_kind,L"Class")||
               !r.path(r.result.identities[slot].outer,parent)||!r.named(parent,L"Engine")||r.result.identities[parent].outer)return false;
            notify=p;found|=2;
        }
        else if(!(r.named(p.identity,L"Comment")&&weaponhand::core_type(r,p.type,L"NameProperty")&&p.element_bytes==8))return false;
        for(unsigned i=0;i<p.element_bytes;++i)if(occupied[p.offset+i]++)return false;child=p.next;
    }
    child_head=structure.children;uptr super{},children{};unsigned extent{};
    return (found&3)==3&&r.at(type,0x60,super)&&super==structure.super&&r.at(type,0x80,children)&&children==structure.children&&
        r.at(type,0x88,extent)&&extent==structure.extent;
}
bool cues(meta::Reader& r,hand::ObjectIdentity const& sequence,Cues& out) noexcept {
    out={};if(!weaponhand::object(r,sequence)||!weaponhand::derives(r,sequence.address,L"AnimSequence",L"Engine"))return false;
    weaponhand::BoolField field{};meta::Table table{},closing{};uptr inner{},type{},children{};meta::Property element{},time{},notify{};
    if(!weaponhand::field(r,sequence.address,L"Notifies",L"ArrayProperty",16,field)||!retain(r,field.property)||
       !r.at(sequence.address,field.property.offset,table)||table.count<1||table.count>64||table.capacity<table.count||table.capacity>64||!table.data||
       !r.at(r.result.identities[field.property.identity].address,0xd0,inner)||!r.property(inner,element)||!retain(r,element)||
       r.result.identities[element.identity].outer!=r.result.identities[field.property.identity].address||element.dimension!=1||
       !weaponhand::core_type(r,element.type,L"StructProperty")||element.element_bytes<12||element.element_bytes>128||
       !r.at(inner,0xd0,type)||!struct_members(r,type,element.element_bytes,time,notify,children))return false;
    const auto bytes=std::size_t(table.count)*element.element_bytes;
    std::array<unsigned char,64*128> data{},again{};
    if(table.data>UINTPTR_MAX-bytes||!copy(data.data(),table.data,bytes))return false;
    Cues value{};value.sequence=sequence;value.notifies=table;value.field=r.result.identities[field.property.identity].address;
    value.inner=inner;value.type=type;value.children=children;value.stride=element.element_bytes;value.bytes_hash=14695981039346656037ull;
    for(std::size_t i=0;i<bytes;++i){value.bytes_hash^=data[i];value.bytes_hash*=1099511628211ull;}
    unsigned found{};
    for(int i=0;i<table.count;++i){const auto offset=std::size_t(i)*element.element_bytes;float seconds{};uptr object{};
        std::memcpy(&seconds,data.data()+offset+time.offset,4);std::memcpy(&object,data.data()+offset+notify.offset,8);
        unsigned slot{};if(!std::isfinite(seconds)||seconds<0||seconds>120||!r.path(object,slot)||
            r.result.identities[slot].outer!=sequence.address||!weaponhand::derives(r,object,L"AnimNotify",L"Engine"))return false;
        if(!weaponhand::derives(r,object,L"AnimNotify_Sound",L"Engine"))continue;
        meta::PointerField sound{};if(!weaponhand::read_pointer(r,object,L"SoundCue",sound)||!sound.value||!retain(r,sound.property)||
            !weaponhand::derives(r,sound.value,L"SoundCue",L"Engine"))return false;
        unsigned index{};hand::ObjectIdentity package{},cue{};
        if(!cue_kind(r,sound.value,index,package))continue; // Other original sounds are not endpoint cues.
        if(!lancermain_asset::identify(r,sound.value,cue)||(found&& !hand::same(package,value.package))||
           ((found&(1u<<index))&&!hand::same(value.cue[index],cue)))return false;
        value.package=package;value.cue[index]=cue;found|=1u<<index;
    }
    uptr closing_inner{},closing_type{};
    if(found!=7||!r.at(value.field,0xd0,closing_inner)||closing_inner!=inner||!r.at(inner,0xd0,closing_type)||closing_type!=type||
       !r.at(sequence.address,field.property.offset,closing)||!meta::same_table(table,closing)||
       !copy(again.data(),table.data,bytes)||std::memcmp(data.data(),again.data(),bytes))return false;
    out=value;return true;
}
struct PlayLayout {
    hand::ObjectIdentity function{};uptr exec{},script{};unsigned bytes{},flags{},sound{},noise{},script_bytes{};std::uint64_t script_hash{};
    bool operator==(PlayLayout const& b)const noexcept{return hand::same(function,b.function)&&exec==b.exec&&script==b.script&&bytes==b.bytes&&flags==b.flags&&
        sound==b.sound&&noise==b.noise&&script_bytes==b.script_bytes&&script_hash==b.script_hash;}
};
template<class Call>bool play_schema(Call const& call,uptr receiver,meta::Reader& r,PlayLayout& out) noexcept {
    out={};unsigned fn{},kind{},owner{},package{},flags{};std::uint16_t bytes{},ret{},native{};uptr exec{};meta::Structure structure{};meta::Table script{};
    if(!call.valid||!call.r.size||call.r.size>64||call.r.size>sizeof(call.r.params)||!r.path(call.r.function,fn)||!r.named(fn,L"WeaponPlaySoundLocal")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||!r.named(owner,L"WarWeapon")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"WarfareGame")||r.result.identities[package].outer||
       !unified_weapon_fire::owner_in_receiver(r,receiver,r.result.identities[owner].address)||
       !r.at(call.r.function,0x106,bytes)||bytes!=call.r.size||!r.at(call.r.function,0x108,ret)||ret!=0xffff||
       !r.at(call.r.function,0xf8,native)||native||!r.at(call.r.function,0xf4,flags)||(flags&0x400)||
       !r.at(call.r.function,0x114,exec)||exec!=base+0x1d6ca0||!r.structure(call.r.function,structure)||bytes>structure.extent||
       !r.at(call.r.function,0x8c,script)||!script.data||script.count<1||script.count>script.capacity||script.capacity>1024)return false;
    PlayLayout value{};value.exec=exec;value.bytes=bytes;value.flags=flags;value.script=script.data;value.script_bytes=unsigned(script.count);
    if(!lancermain_asset::identify(r,call.r.function,value.function))return false;
    std::array<uptr,meta::max_fields> seen{};std::array<unsigned,64> occupied{};unsigned count{},found{};
    for(auto child=structure.children;child;){
        if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;seen[count++]=child;
        meta::Property p{};if(!r.property(child,p)||r.result.identities[p.identity].outer!=call.r.function||p.dimension!=1||
           (p.flags&0x580)!=0x80||!meta::property_fits(p,bytes)||!retain(r,p))return false;
        if(r.named(p.identity,L"Sound")&&weaponhand::core_type(r,p.type,L"ObjectProperty")&&p.element_bytes==8){
            uptr cls{};unsigned slot{},parent{},class_kind{};if(found&1||!r.at(child,0xd0,cls)||!r.path(cls,slot)||!r.named(slot,L"SoundCue")||
               !r.path(r.result.identities[slot].klass,class_kind)||!weaponhand::core_type(r,class_kind,L"Class")||
               !r.path(r.result.identities[slot].outer,parent)||!r.named(parent,L"Engine")||r.result.identities[parent].outer)return false;
            found|=1;value.sound=p.offset;
        }else if(r.named(p.identity,L"NoiseLoudness")&&weaponhand::core_type(r,p.type,L"FloatProperty")&&p.element_bytes==4){if(found&2)return false;found|=2;value.noise=p.offset;}
        else return false;
        for(unsigned i=0;i<p.element_bytes;++i)if(occupied[p.offset+i]++)return false;child=p.next;
    }
    std::array<unsigned char,1024> code{};if(found!=3||!copy(code.data(),script.data,unsigned(script.count)))return false;
    value.script_hash=14695981039346656037ull;for(int i=0;i<script.count;++i){value.script_hash^=code[unsigned(i)];value.script_hash*=1099511628211ull;}
    out=value;return true;
}
#ifndef BOOMSHOT_ENDPOINT_SOUND_METADATA_FIXTURE
#include "weapon_boomshot_endpoint_sound_call.inl"
#endif
}
