// Included after freehands and before handik, only under the private tail flag.
// Reflection/raw-byte observation only: no engine call, setter or journal.
namespace handtail {
using Value=gears_hand_tail_translation::Value;
struct Observed {
    Value value;
    std::vector<unsigned char> schema;
    std::array<uptr,4> addresses{}; // translation, space, space_bone, flags.
    std::array<uptr,3> strength_addresses{}; // ControlStrength, StrengthTarget, BlendTimeToGo.
    std::uint32_t raw_flags{},apply_mask{},add_mask{};
};
namespace detail {
struct Identity {
    uptr address{},vtable{},outer{},object_class{};
    unsigned index{};Name name{};
};
bool identity(uptr object,Identity& result,playground::TailDeletionLookup& deletion){
    if(!object||(object&7)||object>UINTPTR_MAX-0x58||!registered(object)||
       !deletion.alive(object))return false;
    Identity value{};value.address=object;
    if(!copy(&value.vtable,object,sizeof(value.vtable))||!value.vtable||
       !copy(&value.index,object+8,sizeof(value.index))||
       !copy(&value.outer,object+0x40,sizeof(value.outer))||
       !copy(&value.name,object+0x48,sizeof(value.name))||
       !copy(&value.object_class,object+0x50,sizeof(value.object_class))||
       !registered(value.outer)||!registered(value.object_class)||!registered(object))return false;
    result=value;return true;
}
bool same_identity(Identity const& a,Identity const& b){
    return a.address==b.address&&a.vtable==b.vtable&&a.index==b.index&&
        a.outer==b.outer&&a.name.index==b.name.index&&a.name.number==b.name.number&&
        a.object_class==b.object_class;
}
template<class T>void append(std::vector<unsigned char>& bytes,T const& value){
    const auto first=reinterpret_cast<unsigned char const*>(&value);
    bytes.insert(bytes.end(),first,first+sizeof(value));
}
void append_identity(std::vector<unsigned char>& bytes,Identity const& value){
    // Seven individual identity fields; no structure padding enters the schema.
    append(bytes,value.address);append(bytes,value.vtable);append(bytes,value.index);
    append(bytes,value.outer);append(bytes,value.name.index);append(bytes,value.name.number);
    append(bytes,value.object_class);
}
enum class Kind:unsigned {vector,byte,name,boolean,floating};
struct Property {
    Identity object{},type{};
    Kind kind{};unsigned dimension{},bytes{},offset{};
    uptr reference{};std::uint32_t mask{};
};
bool property_at(uptr address,const wchar_t* kind,Kind expected,
              unsigned bytes,unsigned extent,Property& result,playground::TailDeletionLookup& deletion){
    Property value{};value.kind=expected;
    if(!identity(address,value.object,deletion)||address>UINTPTR_MAX-0xd8||
       !identity(value.object.object_class,value.type,deletion)||!playground::named(value.type.address,kind)||
       !copy(&value.dimension,address+0x70,sizeof(value.dimension))||value.dimension!=1||
       !copy(&value.bytes,address+0x74,sizeof(value.bytes))||value.bytes!=bytes||
       !copy(&value.offset,address+0x94,sizeof(value.offset))||
       value.offset>extent||bytes>extent-value.offset)return false;
    if((expected==Kind::vector||expected==Kind::byte)&&
       !copy(&value.reference,address+0xd0,sizeof(value.reference)))return false;
    if(expected==Kind::boolean&&(!copy(&value.mask,address+0xd0,sizeof(value.mask))||
       !value.mask||(value.mask&(value.mask-1))))return false;
    result=value;return true;
}
void append_property(std::vector<unsigned char>& bytes,Property const& value){
    append_identity(bytes,value.object);append_identity(bytes,value.type);
    append(bytes,value.kind);append(bytes,value.dimension);append(bytes,value.bytes);
    append(bytes,value.offset);append(bytes,value.reference);append(bytes,value.mask);
}
bool overlaps(Property const& a,Property const& b){
    // All descriptors have already passed the same bounded extent check.
    return a.offset<b.offset+b.bytes&&b.offset<a.offset+a.bytes;
}
bool compatible(Property const& a,Property const& b){
    if(!overlaps(a,b))return true;
    return a.kind==Kind::boolean&&b.kind==Kind::boolean&&a.offset==b.offset&&
        !(a.mask&b.mask); // Same word is legal only for distinct, validated bits.
}
bool storage_bytes(uptr object_class,unsigned& bytes){
    return object_class<=UINTPTR_MAX-0x8c&&copy(&bytes,object_class+0x88,sizeof(bytes))&&bytes;
}
} // namespace detail

bool same_schema(Observed const& a,Observed const& b){
    return !a.schema.empty()&&a.schema==b.schema&&a.addresses==b.addresses&&
        a.strength_addresses==b.strength_addresses&&a.apply_mask==b.apply_mask&&a.add_mask==b.add_mask;
}
bool observe(uptr control,Observed& out){
    out=Observed{};
    playground::TailDeletionLookup deletion; // No state survives this observe.
    Observed result{};detail::Identity owner{},owner_class{};unsigned extent{};
    if(!detail::identity(control,owner,deletion)||!detail::identity(owner.object_class,owner_class,deletion)||
       !playground::named(owner_class.address,L"SkelControlSingleBone")||
       !detail::storage_bytes(owner_class.address,extent)||extent>4096||control>UINTPTR_MAX-extent)return false;
    constexpr const wchar_t* labels[]={L"BoneTranslation",L"BoneTranslationSpace",
        L"TranslationSpaceBoneName",L"bApplyTranslation",L"bAddTranslation",L"BoneRotationSpace",
        L"bApplyRotation",L"bAddRotation",L"ControlStrength",L"StrengthTarget",L"BlendTimeToGo"};
    constexpr const wchar_t* kinds[]={L"StructProperty",L"ByteProperty",L"NameProperty",
        L"BoolProperty",L"BoolProperty",L"ByteProperty",L"BoolProperty",L"BoolProperty",
        L"FloatProperty",L"FloatProperty",L"FloatProperty"};
    using Kind=detail::Kind;
    constexpr Kind expected[]={Kind::vector,Kind::byte,Kind::name,Kind::boolean,Kind::boolean,
        Kind::byte,Kind::boolean,Kind::boolean,Kind::floating,Kind::floating,Kind::floating};
    constexpr unsigned sizes[]={12,1,8,4,4,1,4,4,4,4,4};
    std::array<uptr,11> field_addresses{};
    if(!playground::fields(owner_class.address,labels,field_addresses))return false;
    std::array<detail::Property,11> fields{};
    for(unsigned i=0;i<fields.size();++i){
        if(!detail::property_at(field_addresses[i],kinds[i],expected[i],sizes[i],extent,
                             fields[i],deletion))return false;
        for(unsigned j=0;j<i;++j)if(!detail::compatible(fields[i],fields[j]))return false;
    }
    if(fields[3].offset!=fields[4].offset||fields[1].reference!=fields[5].reference)return false;
    detail::Identity vector{},vector_class{},space_enum{},enum_class{};unsigned vector_bytes{};
    if(!detail::identity(fields[0].reference,vector,deletion)||!playground::named(vector.address,L"Vector")||
       !detail::identity(vector.object_class,vector_class,deletion)||
       !detail::storage_bytes(vector.address,vector_bytes)||vector_bytes!=12||
       !detail::identity(fields[1].reference,space_enum,deletion)||
       !detail::identity(space_enum.object_class,enum_class,deletion)||
       !playground::named(space_enum.address,L"EBoneControlSpace")||
       !playground::named(enum_class.address,L"Enum"))return false;
    constexpr const wchar_t* components[]={L"X",L"Y",L"Z"};
    std::array<uptr,3> member_addresses{};
    if(!playground::fields(vector.address,components,member_addresses))return false;
    std::array<detail::Property,3> members{};
    for(unsigned i=0;i<members.size();++i)
        if(!detail::property_at(member_addresses[i],L"FloatProperty",Kind::floating,4,12,
                             members[i],deletion)||members[i].offset!=i*4||
           members[i].object.outer!=vector.address)return false;
    const unsigned schema_version=1;
    detail::append(result.schema,schema_version);
    detail::append_identity(result.schema,owner);detail::append_identity(result.schema,owner_class);
    detail::append(result.schema,extent);
    for(auto const& descriptor:fields)detail::append_property(result.schema,descriptor);
    detail::append_identity(result.schema,vector);detail::append_identity(result.schema,vector_class);
    detail::append(result.schema,vector_bytes);
    for(auto const& descriptor:members)detail::append_property(result.schema,descriptor);
    detail::append_identity(result.schema,space_enum);detail::append_identity(result.schema,enum_class);
    result.addresses={{control+fields[0].offset,control+fields[1].offset,
                       control+fields[2].offset,control+fields[3].offset}};
    for(unsigned i=0;i<result.strength_addresses.size();++i)
        result.strength_addresses[i]=control+fields[8+i].offset;
    result.apply_mask=fields[3].mask;result.add_mask=fields[4].mask;
    if(!copy(result.value.translation.data(),result.addresses[0],12)||
       !copy(&result.value.space,result.addresses[1],1)||
       !copy(result.value.space_bone.data(),result.addresses[2],8)||
       !copy(&result.raw_flags,result.addresses[3],4))return false;
    result.value.flags=result.raw_flags&(result.apply_mask|result.add_mask);
    // No strength value is sampled here. The actual compare-write must read
    // these three live addresses and prove exact +0 immediately before writing.
    // Fresh first-match walks must still agree after typed/raw observation;
    // comparing cached addresses to themselves would miss a changed field link.
    std::array<uptr,11> last_fields{};std::array<uptr,3> last_members{};
    if(!playground::fields(owner_class.address,labels,last_fields)||last_fields!=field_addresses||
       !playground::fields(vector.address,components,last_members)||last_members!=member_addresses)return false;
    detail::Identity last_owner{},last_class{};unsigned last_extent{};
    if(!detail::identity(control,last_owner,deletion)||!detail::same_identity(owner,last_owner)||
       !detail::identity(owner_class.address,last_class,deletion)||!detail::same_identity(owner_class,last_class)||
       !detail::storage_bytes(owner_class.address,last_extent)||last_extent!=extent)return false;
    // Sequential observations are not an atomic snapshot or a write permit.
    out=std::move(result);return true;
}
} // namespace handtail
