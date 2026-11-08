// Preparation only. Source-premise offsets, not a retail/native ABI proof.
// Include where uptr/base/copy and playground::arena_pawn already exist.
// Requires C++17 <array>, <cstdint>, <cstddef>, <cstring>, <type_traits>.
// No engine call, Result read, heap, output, clock or game-memory write.
#include "weapon_metadata_parse.h"
namespace gripdamage_metadata {
using U32=std::uint32_t;
using U64=std::uint64_t;
constexpr U32 max_identities=192,max_names=96,max_name_units=96;
constexpr U32 max_outer=12,max_parents=40,max_structures=80,max_fields=64,max_search=4096;
constexpr U32 invalid_slot=~U32(0);
enum class Stage:U32 {none,tables,filter,receiver,function_path,inheritance,pawn_weapon,instigator,function_fields,final_check};
enum class Reason:U32 {
    none,unreadable,table_bounds,unregistered,identity_unknown,name_invalid,name_limit,outer_cycle,outer_limit,
    identity_changed,capacity,arena_changed,path_mismatch,type_mismatch,inheritance_cycle,
    inheritance_limit,not_war_weapon,owner_not_in_receiver,field_cycle,field_limit,field_missing,
    pointer_layout,pointer_relation,return_missing,return_duplicate,return_layout,layout_changed,field_layout
};
struct Failure {Stage stage;Reason reason;uptr address;U32 ordinal;};
struct RawName {U32 index,number;};
// These fields are contiguous in the certified retail object/property
// layouts. Read exactly their existing bytes together, with no retained
// values, skipped field, wider range or change to the closing rereads.
struct IdentityHeader {uptr outer;RawName name;uptr klass;};
struct PropertyPrefix {uptr next;U32 dimension,element_bytes;U64 flags;};
static_assert(sizeof(IdentityHeader)==24&&offsetof(IdentityHeader,name)==8&&offsetof(IdentityHeader,klass)==16);
static_assert(sizeof(PropertyPrefix)==24&&offsetof(PropertyPrefix,dimension)==8&&offsetof(PropertyPrefix,element_bytes)==12&&offsetof(PropertyPrefix,flags)==16);
struct Table {uptr data;std::int32_t count,capacity;};
struct NameText {uptr entry;U32 index,length;std::array<wchar_t,max_name_units> units;};
struct Identity {uptr address,outer,klass;U32 index;RawName name;U32 name_slot;};
struct Structure {U32 identity;uptr super,children;U32 extent;};
struct Property {U32 identity,type;uptr next;U64 flags;U32 dimension,element_bytes,offset;};
struct PointerField {Property property;U32 owner;uptr value;};
struct Snapshot {
    U32 known;Failure failure;
    uptr module_base,arena_pawn,receiver,callee;
    Table objects,names;
    U32 identity_count,name_count,structure_count,field_count;
    std::array<Identity,max_identities> identities;
    std::array<NameText,max_names> name_text;
    std::array<Structure,max_structures> structures;
    std::array<Property,max_fields> fields;
    U32 pawn_identity,receiver_identity,function_identity,function_type,owner_identity,package_identity;
    U32 receiver_class_count;std::array<U32,max_parents> receiver_classes;
    U32 owner_kind,return_field,function_extent;
    uptr function_children;
    std::uint16_t parameter_bytes,return_offset;
    PointerField pawn_weapon,weapon_instigator;
};
static_assert(sizeof(uptr)==8&&sizeof(wchar_t)==2&&sizeof(RawName)==8&&sizeof(Table)==16);
static_assert(std::is_trivial<Snapshot>::value&&std::is_standard_layout<Snapshot>::value);
static_assert(sizeof(Snapshot)<=40*1024,"Keep one fixed read workspace below 40 KiB");

bool read_bytes(uptr object,uptr offset,void* destination,std::size_t bytes) noexcept {
    if(!object||!destination||!bytes||offset>~uptr(0)-object)return false;
    const auto address=object+offset;
    return bytes<=~uptr(0)-address&&copy(destination,address,bytes);
}
template<class T>bool read_at(uptr object,uptr offset,T& destination) noexcept {
    return read_bytes(object,offset,&destination,sizeof(destination));
}
bool same_table(Table const& first,Table const& second) noexcept {
    return first.data==second.data&&first.count==second.count&&first.capacity==second.capacity;
}
bool tables(uptr module_base,Table& objects,Table& names) noexcept {
    if(!read_at(module_base,0x1c24730,objects)||!read_at(module_base,0x1c34800,names))return false;
    return objects.data&&objects.count>1000&&objects.count<=objects.capacity&&objects.capacity<4000000&&
        names.data&&names.count>=1000&&names.count<=names.capacity&&names.capacity<=4000000&&
        uptr(objects.capacity)*8<=~uptr(0)-objects.data&&uptr(names.capacity)*8<=~uptr(0)-names.data;
}
bool registered_known(Table const& objects,uptr object,U32& index) noexcept {
    uptr slot{};
    return object&&read_at(object,8,index)&&index<U32(objects.count)&&
        read_at(objects.data,uptr(index)*8,slot)&&slot==object;
}
bool fresh_name(Table const& names,U32 index,NameText& text) noexcept {
    text={};text.index=index;U32 entry_index{};
    if(index>=U32(names.count)||!read_at(names.data,uptr(index)*8,text.entry)||!text.entry||
       !read_at(text.entry,0,entry_index)||entry_index!=index)return false;
#if defined(GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL)
    static_assert(max_name_units==weapon_metadata_parse::name_units);
    const auto bound=weapon_metadata_parse::length_bound(text.entry,index);
    if(bound&&bound<max_name_units){
        // The length is only a read bound. Slot/index and ALL actual text,
        // including the terminator, are fresh. No cached characters select a
        // property. A changed termination rejects rather than reusing text.
        if(!read_bytes(text.entry,20,text.units.data(),(std::size_t(bound)+1)*sizeof(wchar_t)))return false;
        for(U32 i=0;i<bound;++i)if(!text.units[i])return false;
        if(text.units[bound])return false;
        text.length=bound;return true;
    }
    {
        // Native participant callbacks can run without a parsing workspace.
        // The same fresh page-bounded read is valid there too; no scope or
        // length/text cache is created merely to enable this read algorithm.
        // Cold names still require ALL real characters through the first NUL.
        // Never cross a readable page merely to fill a chunk. A failed read
        // rejects immediately: retrying could consume a one-shot guard-page
        // exception and incorrectly turn that failure into an accepted name.
        static const std::size_t page_bytes=[](){SYSTEM_INFO info{};GetSystemInfo(&info);
            return std::size_t(info.dwPageSize);}();
        if(!page_bytes)return false;
        for(U32 at=0;at<max_name_units;){
            const auto offset=20+uptr(at)*sizeof(wchar_t);
            if(offset>~uptr(0)-text.entry)return false;
            const auto room=(page_bytes-(text.entry+offset)%page_bytes)/sizeof(wchar_t);
            const U32 count=room?U32((std::min)(std::size_t(max_name_units-at),room)):1;
            if(!read_bytes(text.entry,offset,text.units.data()+at,std::size_t(count)*sizeof(wchar_t)))return false;
            for(U32 n=0;n<count;++n){
                const auto i=at+n;
                if(!text.units[i]){
                    text.length=i;if(i)weapon_metadata_parse::remember_length(text.entry,index,i);
                    return i!=0;
                }
            }
            at+=count;
        }
        return false;
    }
#else
    for(U32 i=0;i<max_name_units;++i){
        if(!read_at(text.entry,20+uptr(i)*sizeof(wchar_t),text.units[i]))return false;
        if(!text.units[i]){text.length=i;
            return i!=0;}
    }
    return false; // Never manufacture the terminating NUL.
#endif
}
bool same_name_text(NameText const& first,NameText const& second) noexcept {
    if(first.entry!=second.entry||first.index!=second.index||first.length!=second.length||
       first.length>=max_name_units)return false;
    for(U32 i=0;i<=first.length;++i)if(first.units[i]!=second.units[i])return false;
    return true;
}
bool same_identity(Identity const& first,Identity const& second) noexcept {
    return first.address==second.address&&first.outer==second.outer&&first.klass==second.klass&&
        first.index==second.index&&first.name.index==second.name.index&&first.name.number==second.name.number&&
        first.name_slot==second.name_slot;
}
bool same_structure(Structure const& first,Structure const& second) noexcept {
    return first.identity==second.identity&&first.super==second.super&&first.children==second.children&&first.extent==second.extent;
}
bool same_property(Property const& first,Property const& second) noexcept {
    return first.identity==second.identity&&first.type==second.type&&first.next==second.next&&first.flags==second.flags&&
        first.dimension==second.dimension&&first.element_bytes==second.element_bytes&&first.offset==second.offset;
}
bool property_fits(Property const& value,U32 extent) noexcept {
    return value.dimension&&value.element_bytes&&value.offset<=extent&&
        value.dimension<=(extent-value.offset)/value.element_bytes;
}
bool same_pointer(PointerField const& first,PointerField const& second) noexcept {
    return same_property(first.property,second.property)&&first.owner==second.owner&&first.value==second.value;
}
template<std::size_t N>bool exact_text(NameText const& text,wchar_t const (&literal)[N]) noexcept {
    static_assert(N>1&&N<=max_name_units);
    if(text.length!=N-1)return false;
    for(std::size_t i=0;i<N;++i)if(text.units[i]!=literal[i])return false;
    return true;
}
bool raw_identity(Table const& objects,Table const& names,uptr object,Identity& identity,NameText& text) noexcept {
    identity={};identity.address=object;identity.name_slot=invalid_slot;
    U32 class_index{},outer_index{},last_index{};IdentityHeader header{};
    if(!registered_known(objects,object,identity.index)||!read_at(object,0x40,header))return false;
    identity.outer=header.outer;identity.name=header.name;identity.klass=header.klass;
    return registered_known(objects,identity.klass,class_index)&&
        (!identity.outer||registered_known(objects,identity.outer,outer_index))&&
        fresh_name(names,identity.name.index,text)&&registered_known(objects,object,last_index)&&last_index==identity.index;
}

// Filtering only. known=true means a fresh short name was read consistently;
// it is not a typed signature, full path, receiver or Result proof.
bool candidate(uptr callee,bool& known) noexcept {
    known=false;const auto module_base=base;Table objects{},names{},last_objects{},last_names{};
    Identity first{},last{};NameText first_text{},last_text{};
    if(!tables(module_base,objects,names)||!raw_identity(objects,names,callee,first,first_text)||
       !raw_identity(objects,names,callee,last,last_text)||!same_identity(first,last)||!same_name_text(first_text,last_text)||
       !tables(module_base,last_objects,last_names)||!same_table(objects,last_objects)||!same_table(names,last_names)||base!=module_base)return false;
    known=true;return exact_text(first_text,L"GetFireModeDamage");
}

struct Reader {
    Snapshot& result;Stage phase;
    // Opt-in only for a read-only query's initial discovery. The Snapshot is
    // local to that query; callers MUST disable this before their existing
    // fresh closing rereads, and before any dispatch or result publication.
    // Other readers retain their original fresh-on-every-access behaviour.
    bool reuse_initial_identities{};
    bool fail(Reason reason,uptr address=0,U32 ordinal=0) noexcept {
        if(result.failure.reason==Reason::none)result.failure={phase,reason,address,ordinal};
        return false;
    }
    template<class T>bool at(uptr object,uptr offset,T& value) noexcept {
        return read_at(object,offset,value)||fail(Reason::unreadable,object,static_cast<U32>(offset));
    }
    bool identity(uptr object,U32& slot) noexcept {
        if(reuse_initial_identities){
            for(U32 i=0;i<result.identity_count;++i)if(result.identities[i].address==object){slot=i;return true;}
        }
        Identity fresh{};NameText text{};
        if(!raw_identity(result.objects,result.names,object,fresh,text))return fail(Reason::identity_unknown,object);
        U32 name_slot=invalid_slot;
        for(U32 i=0;i<result.name_count;++i)if(result.name_text[i].index==text.index){
            if(!same_name_text(result.name_text[i],text))return fail(Reason::identity_changed,object);
            name_slot=i;break;
        }
        if(name_slot==invalid_slot){
            if(result.name_count==max_names)return fail(Reason::capacity,object,max_names);
            name_slot=result.name_count++;result.name_text[name_slot]=text;
        }
        fresh.name_slot=name_slot;
        for(U32 i=0;i<result.identity_count;++i)if(result.identities[i].address==object){
            if(!same_identity(result.identities[i],fresh))return fail(Reason::identity_changed,object);
            slot=i;return true;
        }
        if(result.identity_count==max_identities)return fail(Reason::capacity,object,max_identities);
        slot=result.identity_count++;result.identities[slot]=fresh;return true;
    }
    bool path(uptr object,U32& first) noexcept {
        std::array<uptr,max_outer> seen{};U32 count{};first=invalid_slot;
        if(!object)return fail(Reason::unregistered,object);
        while(object){
            if(count==max_outer)return fail(Reason::outer_limit,object,count);
            for(U32 i=0;i<count;++i)if(seen[i]==object)return fail(Reason::outer_cycle,object,count);
            seen[count++]=object;U32 slot{};if(!identity(object,slot))return false;
            if(first==invalid_slot)first=slot;object=result.identities[slot].outer;
        }
        return true;
    }
    U32 locate(uptr address) const noexcept {
        for(U32 i=0;i<result.identity_count;++i)if(result.identities[i].address==address)return i;
        return invalid_slot;
    }
    template<std::size_t N>bool named(U32 slot,wchar_t const (&literal)[N],bool zero_number=true) const noexcept {
        if(slot>=result.identity_count)return false;const auto& value=result.identities[slot];
        return (!zero_number||value.name.number==0)&&value.name_slot<result.name_count&&exact_text(result.name_text[value.name_slot],literal);
    }
    bool structure(uptr object,Structure& value) noexcept {
        value={};if(!path(object,value.identity)||!at(object,0x60,value.super)||!at(object,0x80,value.children)||!at(object,0x88,value.extent))return false;
        U32 type{};if(!path(result.identities[value.identity].klass,type))return false;
        if(!named(type,L"Class")&&!named(type,L"Function"))return fail(Reason::type_mismatch,object);
        for(U32 i=0;i<result.structure_count;++i)if(result.structures[i].identity==value.identity)
            return same_structure(result.structures[i],value)||fail(Reason::layout_changed,object);
        if(result.structure_count==max_structures)return fail(Reason::capacity,object,max_structures);
        result.structures[result.structure_count++]=value;return true;
    }
    bool property(uptr object,Property& value) noexcept {
        value={};if(!path(object,value.identity)||!path(result.identities[value.identity].klass,value.type))return false;
        PropertyPrefix prefix{};
        if(!at(object,0x68,prefix)||!at(object,0x94,value.offset))return false;
        value.next=prefix.next;value.dimension=prefix.dimension;value.element_bytes=prefix.element_bytes;value.flags=prefix.flags;
        return true;
    }
    bool receiver_classes() noexcept {
        auto current=result.identities[result.receiver_identity].klass;bool war_weapon=false,owner=false;
        while(current){
            if(result.receiver_class_count==max_parents)return fail(Reason::inheritance_limit,current);
            for(U32 i=0;i<result.receiver_class_count;++i)
                if(result.identities[result.receiver_classes[i]].address==current)return fail(Reason::inheritance_cycle,current);
            Structure value{};if(!structure(current,value))return false;
            result.receiver_classes[result.receiver_class_count++]=value.identity;
            const auto& identity_value=result.identities[value.identity];
            const auto package_slot=locate(identity_value.outer);
            if(named(value.identity,L"WarWeapon")&&package_slot!=invalid_slot&&named(package_slot,L"WarfareGame")&&
               result.identities[package_slot].outer==0)war_weapon=true;
            if(value.identity==result.owner_identity)owner=true;
            current=value.super;
        }
        return (war_weapon||fail(Reason::not_war_weapon,result.receiver))&&
            (owner||fail(Reason::owner_not_in_receiver,result.receiver));
    }
    template<std::size_t N>bool pointer(uptr object,wchar_t const (&label)[N],PointerField& output) noexcept {
        U32 object_slot{};if(!path(object,object_slot))return false;
        auto current=result.identities[object_slot].klass;std::array<uptr,max_parents> parents{};U32 parent_count{},object_extent{};
        while(current){
            if(parent_count==max_parents)return fail(Reason::inheritance_limit,current);
            for(U32 i=0;i<parent_count;++i)if(parents[i]==current)return fail(Reason::inheritance_cycle,current);
            parents[parent_count++]=current;Structure structure_value{};if(!structure(current,structure_value))return false;
            if(parent_count==1)object_extent=structure_value.extent;
            auto field=structure_value.children;std::array<uptr,max_search> seen{};U32 count{};
            weapon_metadata_parse::Walk traversal;
            while(field){
                if(count==max_search)return fail(Reason::field_limit,field,count);
                if(traversal.seen(field,seen.data(),count))return fail(Reason::field_cycle,field,count);
                seen[count++]=field;Identity field_identity{};NameText field_text{};
                if(!raw_identity(result.objects,result.names,field,field_identity,field_text))return fail(Reason::identity_unknown,field);
                if(exact_text(field_text,label)){
                    if(field_identity.name.number)return fail(Reason::pointer_layout,field);
                    PointerField value{};value.owner=structure_value.identity;
                    if(!property(field,value.property))return false;
                    if(result.identities[value.property.identity].outer!=current||
                       !named(value.property.type,L"ObjectProperty")||value.property.dimension!=1||value.property.element_bytes!=8||
                       value.property.offset>structure_value.extent||8>structure_value.extent-value.property.offset||
                       value.property.offset>object_extent||8>object_extent-value.property.offset)
                        return fail(Reason::pointer_layout,field);
                    if(!at(object,value.property.offset,value.value))return false;
                    output=value;return true;
                }
                if(!at(field,0x68,field))return false;
            }
            current=structure_value.super;
        }
        return fail(Reason::field_missing,object);
    }
    bool function_path() noexcept {
        if(!path(result.callee,result.function_identity))return false;
        auto const& function=result.identities[result.function_identity];
        if(!path(function.klass,result.function_type)||!named(result.function_type,L"Function")||
           !named(result.function_identity,L"GetFireModeDamage"))return fail(Reason::path_mismatch,result.callee);
        result.owner_identity=locate(function.outer);
        if(result.owner_identity==invalid_slot)return fail(Reason::path_mismatch,result.callee);
        if(named(result.owner_identity,L"WarWeapon"))result.owner_kind=1;
        else if(named(result.owner_identity,L"Weap_Shotgun"))result.owner_kind=2;
        else return fail(Reason::path_mismatch,function.outer);
        U32 owner_type{};if(!path(result.identities[result.owner_identity].klass,owner_type))return false;
        if(!named(owner_type,L"Class"))return fail(Reason::type_mismatch,function.outer);
        result.package_identity=locate(result.identities[result.owner_identity].outer);
        return (result.package_identity!=invalid_slot&&named(result.package_identity,L"WarfareGame")&&
                result.identities[result.package_identity].outer==0)||fail(Reason::path_mismatch,function.outer);
    }
    bool function_fields() noexcept {
        Structure value{};if(!structure(result.callee,value)||!at(result.callee,0x106,result.parameter_bytes)||
           !at(result.callee,0x108,result.return_offset))return false;
        result.function_children=value.children;result.function_extent=value.extent;result.return_field=invalid_slot;
        if(U32(result.parameter_bytes)>result.function_extent)return fail(Reason::return_layout,result.callee);
        auto field=value.children;
        while(field){
            if(result.field_count==max_fields)return fail(Reason::field_limit,field,result.field_count);
            for(U32 i=0;i<result.field_count;++i)
                if(result.identities[result.fields[i].identity].address==field)return fail(Reason::field_cycle,field,result.field_count);
            auto& description=result.fields[result.field_count];if(!property(field,description))return false;
            if(result.identities[description.identity].outer!=result.callee)return fail(Reason::path_mismatch,field);
            if(!property_fits(description,result.function_extent)||
               ((description.flags&0x80)&&!property_fits(description,U32(result.parameter_bytes))))
                return fail(Reason::field_layout,field);
            if(description.flags&0x400){
                if(result.return_field!=invalid_slot)return fail(Reason::return_duplicate,field);
                if(!(description.flags&0x80)||!named(description.type,L"FloatProperty")||description.dimension!=1||
                   description.element_bytes!=4||description.offset!=result.return_offset||
                   description.offset>result.parameter_bytes||4u>U32(result.parameter_bytes)-description.offset||
                   description.offset>result.function_extent||4u>result.function_extent-description.offset)
                    return fail(Reason::return_layout,field);
                result.return_field=result.field_count;
            }
            ++result.field_count;field=description.next;
        }
        return result.return_field!=invalid_slot||fail(Reason::return_missing,result.callee);
    }
    bool final_check() noexcept {
        Table last_objects{},last_names{};
        if(base!=result.module_base||playground::arena_pawn!=result.arena_pawn)return fail(Reason::arena_changed,result.arena_pawn);
        if(!tables(result.module_base,last_objects,last_names)||!same_table(result.objects,last_objects)||!same_table(result.names,last_names))
            return fail(Reason::table_bounds,result.module_base);
        const auto object_count=result.identity_count,structure_count=result.structure_count;
        for(U32 i=0;i<object_count;++i){U32 slot{};if(!identity(result.identities[i].address,slot)||slot!=i)return false;}
        for(U32 i=0;i<structure_count;++i){Structure fresh{};
            if(!structure(result.identities[result.structures[i].identity].address,fresh)||!same_structure(result.structures[i],fresh))
                return fail(Reason::layout_changed,result.callee,i);
        }
        for(U32 i=0;i<result.field_count;++i){Property fresh{};
            if(!property(result.identities[result.fields[i].identity].address,fresh)||!same_property(result.fields[i],fresh))
                return fail(Reason::layout_changed,result.callee,i);
        }
        std::uint16_t bytes{},offset{};
        if(!at(result.callee,0x106,bytes)||!at(result.callee,0x108,offset)||bytes!=result.parameter_bytes||offset!=result.return_offset)
            return fail(Reason::layout_changed,result.callee);
        PointerField weapon{},instigator{};
        if(!pointer(result.arena_pawn,L"Weapon",weapon)||!pointer(result.receiver,L"Instigator",instigator)||
           !same_pointer(result.pawn_weapon,weapon)||!same_pointer(result.weapon_instigator,instigator))
            return fail(Reason::pointer_relation,result.receiver);
        if(!tables(result.module_base,last_objects,last_names)||!same_table(result.objects,last_objects)||!same_table(result.names,last_names))
            return fail(Reason::table_bounds,result.module_base);
        return (base==result.module_base&&playground::arena_pawn==result.arena_pawn)||fail(Reason::arena_changed,result.arena_pawn);
    }
};

bool read(uptr receiver,uptr callee,uptr arena_pawn,Snapshot& out) noexcept {
    std::memset(&out,0,sizeof(out));Snapshot scratch{};
    scratch.module_base=base;scratch.arena_pawn=arena_pawn;scratch.receiver=receiver;scratch.callee=callee;
    Reader reader{scratch,Stage::tables};bool complete=false;
    do{
        if(!arena_pawn||arena_pawn!=playground::arena_pawn){reader.fail(Reason::arena_changed,arena_pawn);break;}
        if(!tables(scratch.module_base,scratch.objects,scratch.names)){reader.fail(Reason::table_bounds,scratch.module_base);break;}
        reader.phase=Stage::receiver;
        if(!reader.path(arena_pawn,scratch.pawn_identity)||!reader.path(receiver,scratch.receiver_identity))break;
        reader.phase=Stage::function_path;if(!reader.function_path())break;
        reader.phase=Stage::inheritance;if(!reader.receiver_classes())break;
        reader.phase=Stage::pawn_weapon;
        if(!reader.pointer(arena_pawn,L"Weapon",scratch.pawn_weapon))break;
        if(scratch.pawn_weapon.value!=receiver){reader.fail(Reason::pointer_relation,arena_pawn);break;}
        reader.phase=Stage::instigator;
        if(!reader.pointer(receiver,L"Instigator",scratch.weapon_instigator))break;
        if(scratch.weapon_instigator.value!=arena_pawn){reader.fail(Reason::pointer_relation,receiver);break;}
        reader.phase=Stage::function_fields;if(!reader.function_fields())break;
        reader.phase=Stage::final_check;if(!reader.final_check())break;
        complete=true;
    }while(false);
    if(!complete){out.failure=scratch.failure;return false;}
    scratch.known=1;out=scratch;return true;
}

// Semantic fields only: no comparison of C++ padding or incomplete snapshots.
bool same(Snapshot const& first,Snapshot const& second) noexcept {
    if(first.known!=1||second.known!=1||first.failure.reason!=Reason::none||second.failure.reason!=Reason::none||
       first.failure.stage!=second.failure.stage||first.failure.address!=second.failure.address||first.failure.ordinal!=second.failure.ordinal||
       first.identity_count>max_identities||first.name_count>max_names||first.structure_count>max_structures||
       first.field_count>max_fields||first.receiver_class_count>max_parents||
       first.identity_count!=second.identity_count||first.name_count!=second.name_count||
       first.structure_count!=second.structure_count||first.field_count!=second.field_count||
       first.receiver_class_count!=second.receiver_class_count||first.module_base!=second.module_base||
       first.arena_pawn!=second.arena_pawn||first.receiver!=second.receiver||first.callee!=second.callee||
       !same_table(first.objects,second.objects)||!same_table(first.names,second.names)||
       first.pawn_identity!=second.pawn_identity||first.receiver_identity!=second.receiver_identity||
       first.function_identity!=second.function_identity||first.function_type!=second.function_type||
       first.owner_identity!=second.owner_identity||first.package_identity!=second.package_identity||
       first.owner_kind!=second.owner_kind||first.return_field!=second.return_field||first.function_extent!=second.function_extent||
       first.function_children!=second.function_children||first.parameter_bytes!=second.parameter_bytes||first.return_offset!=second.return_offset||
       !same_pointer(first.pawn_weapon,second.pawn_weapon)||!same_pointer(first.weapon_instigator,second.weapon_instigator))return false;
    for(U32 i=0;i<first.identity_count;++i)if(!same_identity(first.identities[i],second.identities[i]))return false;
    for(U32 i=0;i<first.name_count;++i)if(!same_name_text(first.name_text[i],second.name_text[i]))return false;
    for(U32 i=0;i<first.structure_count;++i)if(!same_structure(first.structures[i],second.structures[i]))return false;
    for(U32 i=0;i<first.field_count;++i)if(!same_property(first.fields[i],second.fields[i]))return false;
    for(U32 i=0;i<first.receiver_class_count;++i)if(first.receiver_classes[i]!=second.receiver_classes[i])return false;
    return true;
}
} // namespace gripdamage_metadata
