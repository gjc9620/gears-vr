#pragma once
// Data-only selected original three-bone mesh. RefSkeleton's script inner is
// IntProperty/4; its native opaque68-byte records come from the existing
// certified Native reader, not from that dummy script element width. Normal
// GT must ALSO compare this result to original named getters before use.
namespace boomshot_pose {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
using Reads=boomshot_socket::Reads;
using Matrix=std::array<float,16>;
using References=std::array<std::array<unsigned char,68>,3>;
struct Source {
    hand::ObjectIdentity component{},asset{};
    meta::Table references{},space{};References reference_bytes{};
    Matrix component_world{};std::array<Matrix,3> component_bones{},world{};
    std::array<meta::Property,2> inner{};
    std::array<uptr,4> fields{};std::array<uptr,2> inner_addresses{},inner_types{};
    std::int64_t observed_begin_ns{},observed_end_ns{};
    bool magazine_collapsed{}; // No orientation/mount authority for this bone.
};
template<std::size_t N>bool declared(meta::Reader& r,weaponhand::BoolField const& field,
    wchar_t const(&owner_label)[N]) noexcept {
    unsigned kind{},package{};
    return r.named(field.owner,owner_label)&&r.path(r.result.identities[field.owner].klass,kind)&&
        weaponhand::core_type(r,kind,L"Class")&&r.path(r.result.identities[field.owner].outer,package)&&
        r.named(package,L"Engine")&&!r.result.identities[package].outer;
}
bool matrix_type(meta::Reader& r,Reads& reads,meta::Property const& property) noexcept {
    uptr structure{};unsigned slot{},kind{},owner_index{},owner_kind{},package{},extent{};
    return property.element_bytes==64&&weaponhand::core_type(r,property.type,L"StructProperty")&&
        reads.at(r,r.result.identities[property.identity].address,0xd0,structure)&&
        r.path(structure,slot)&&r.named(slot,L"Matrix")&&r.path(r.result.identities[slot].klass,kind)&&
        weaponhand::core_type(r,kind,L"ScriptStruct")&&
        r.path(r.result.identities[slot].outer,owner_index)&&r.named(owner_index,L"Object")&&
        r.path(r.result.identities[owner_index].klass,owner_kind)&&weaponhand::core_type(r,owner_kind,L"Class")&&
        r.path(r.result.identities[owner_index].outer,package)&&r.named(package,L"Core")&&
        !r.result.identities[package].outer&&reads.at(r,structure,0x88,extent)&&extent==64;
}
bool affine(Matrix const& m) noexcept {
    for(float value:m)if(!std::isfinite(value))return false;
    if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return false;
    std::array<double,3> scale{};
    for(unsigned i=0;i<3;++i){scale[i]=std::hypot(double(m[i*4]),double(m[i*4+1]),double(m[i*4+2]));
        if(scale[i]<.01||scale[i]>100)return false;}
    for(unsigned i=1;i<3;++i)if(std::abs(scale[i]-scale[0])>1e-4*scale[0])return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j){double dot{};
        for(unsigned k=0;k<3;++k)dot+=double(m[i*4+k])*m[j*4+k];
        if(std::abs(dot)>1e-4*scale[i]*scale[j])return false;}
    const double determinant=double(m[0])*(double(m[5])*m[10]-double(m[6])*m[9])-
        double(m[1])*(double(m[4])*m[10]-double(m[6])*m[8])+double(m[2])*(double(m[4])*m[9]-double(m[5])*m[8]);
    return determinant>0;
}
bool collapsed_magazine(Matrix const& m) noexcept {
    for(float scalar:m)if(!std::isfinite(scalar))return false;
    if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return false;
    for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)if(m[row*4+column]!=0)return false;
    return true;
}
Matrix product(Matrix const& a,Matrix const& b) noexcept {
    Matrix result{};
    for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){double value{};
        for(unsigned k=0;k<4;++k)value+=double(a[row*4+k])*b[k*4+col];
        result[row*4+col]=float(value);}
    return result;
}
bool matrix_bytes(meta::Reader& r,Reads& reads,uptr object,uptr offset,Matrix& out,bool magazine=false) noexcept {
    std::array<float,8> first{},last{};
    if(!reads.at(r,object,offset,first)||offset>UINTPTR_MAX-32||!reads.at(r,object,offset+32,last))return false;
    std::copy(first.begin(),first.end(),out.begin());std::copy(last.begin(),last.end(),out.begin()+8);
    return affine(out)||(magazine&&collapsed_magazine(out));
}
bool array(meta::Table const& a,unsigned width) noexcept {
    return a.data&&a.count==3&&a.capacity>=3&&a.capacity<=1024&&a.data<=UINTPTR_MAX-std::size_t(a.capacity)*width;
}
bool overlaps(uptr a,std::size_t as,uptr b,std::size_t bs) noexcept {
    return a<=UINTPTR_MAX-as&&b<=UINTPTR_MAX-bs&&a<b+bs&&b<a+as;
}
bool metadata(hand::Frame const& f,lancermain_asset::Evidence const& asset,meta::Reader& r,
    Reads& reads,Source& out) noexcept {
    Source value{};value.component=asset.component;value.asset=asset.asset;
    if(!weaponhand::object(r,f.weapon)||!weaponhand::object(r,asset.component)||!weaponhand::object(r,asset.asset))return false;
    weaponhand::BoolField refs{},space{},world{},parent{};uptr master{},element{};
    if(!reads.field(r,asset.asset.address,L"RefSkeleton",L"ArrayProperty",16,refs)||!declared(r,refs,L"SkeletalMesh")||
       !reads.at(r,asset.asset.address,refs.property.offset,value.references)||!array(value.references,68)||
       !reads.at(r,r.result.identities[refs.property.identity].address,0xd0,element)||!r.property(element,value.inner[0])||
       r.result.identities[value.inner[0].identity].outer!=r.result.identities[refs.property.identity].address||
       !r.named(value.inner[0].identity,L"RefSkeleton")||value.inner[0].offset||value.inner[0].dimension!=1||
       value.inner[0].element_bytes!=4||!weaponhand::core_type(r,value.inner[0].type,L"IntProperty")||
       !reads.field(r,asset.component.address,L"SpaceBases",L"ArrayProperty",16,space)||!declared(r,space,L"SkeletalMeshComponent")||
       !reads.at(r,asset.component.address,space.property.offset,value.space)||!array(value.space,64)||
       !reads.at(r,r.result.identities[space.property.identity].address,0xd0,element)||!r.property(element,value.inner[1])||
       r.result.identities[value.inner[1].identity].outer!=r.result.identities[space.property.identity].address||
       !r.named(value.inner[1].identity,L"SpaceBases")||value.inner[1].offset||value.inner[1].dimension!=1||
       !matrix_type(r,reads,value.inner[1])||
       !reads.field(r,asset.component.address,L"LocalToWorld",L"StructProperty",64,world)||!declared(r,world,L"PrimitiveComponent")||
       !matrix_type(r,reads,world.property)||
       !reads.field(r,asset.component.address,L"ParentAnimComponent",L"ComponentProperty",8,parent)||!declared(r,parent,L"SkeletalMeshComponent")||
       !reads.at(r,asset.component.address,parent.property.offset,master)||master)return false;
    const std::array<weaponhand::BoolField,4> descriptors{refs,space,world,parent};
    for(unsigned i=0;i<descriptors.size();++i)value.fields[i]=r.result.identities[descriptors[i].property.identity].address;
    for(unsigned i=0;i<value.inner.size();++i){
        if(value.inner[i].flags&0x580)return false;
        value.inner_addresses[i]=r.result.identities[value.inner[i].identity].address;
        value.inner_types[i]=r.result.identities[value.inner[i].type].address;
    }
    if(overlaps(space.property.offset,16,world.property.offset,64)||overlaps(parent.property.offset,8,world.property.offset,64)||
       overlaps(parent.property.offset,8,space.property.offset,16))return false;
    // Native parent remapping is intentionally unavailable in this reader.
    unsigned parent_class{},parent_kind{},parent_package{};uptr target{};
    if(!reads.at(r,r.result.identities[parent.property.identity].address,0xd0,target)||!r.path(target,parent_class)||
       !r.named(parent_class,L"SkeletalMeshComponent")||!r.path(r.result.identities[parent_class].klass,parent_kind)||
       !weaponhand::core_type(r,parent_kind,L"Class")||!r.path(r.result.identities[parent_class].outer,parent_package)||
       !r.named(parent_package,L"Engine")||r.result.identities[parent_package].outer||
       overlaps(value.references.data,std::size_t(value.references.capacity)*68,value.space.data,std::size_t(value.space.capacity)*64)||
       overlaps(value.references.data,std::size_t(value.references.capacity)*68,asset.component.address,512)||
       overlaps(value.space.data,std::size_t(value.space.capacity)*64,asset.component.address,512)||
       !matrix_bytes(r,reads,asset.component.address,world.property.offset,value.component_world))return false;
    const wchar_t* expected[]={L"Boomshot_Root",L"Boomshot_Magazine",L"Boomshot_Barrel"};
    for(unsigned i=0;i<3;++i){
        std::array<unsigned char,32> a{},b{};std::array<unsigned char,4> c{};const auto at=value.references.data+std::size_t(i)*68;
        if(!reads.at(r,at,0,a)||!reads.at(r,at,32,b)||!reads.at(r,at,64,c))return false;
        std::copy(a.begin(),a.end(),value.reference_bytes[i].begin());std::copy(b.begin(),b.end(),value.reference_bytes[i].begin()+32);
        std::copy(c.begin(),c.end(),value.reference_bytes[i].begin()+64);
        meta::RawName name{};meta::NameText text{};std::memcpy(&name,value.reference_bytes[i].data(),sizeof(name));
        const auto length=std::wcslen(expected[i]);
        if(!reads.name(r,name,text)||text.length!=length||std::wmemcmp(text.units.data(),expected[i],length+1)||
           !matrix_bytes(r,reads,value.space.data,std::size_t(i)*64,value.component_bones[i],i==1))return false;
        value.world[i]=product(value.component_bones[i],value.component_world);
        if(i==1&&collapsed_magazine(value.component_bones[i])){
            if(!collapsed_magazine(value.world[i]))return false;value.magazine_collapsed=true;
        }else if(!affine(value.world[i]))return false;
    }
    if(!unified_weapon_fire::retained_metadata(r))return false;
    out=value;return true;
}
bool same_reference(Source const& a,Source const& b) noexcept {
    if(!hand::same(a.component,b.component)||!hand::same(a.asset,b.asset)||
       !meta::same_table(a.references,b.references)||a.reference_bytes!=b.reference_bytes||
       a.fields!=b.fields||a.inner_addresses!=b.inner_addresses||a.inner_types!=b.inner_types)return false;
    // Reader identity slots are local to one Snapshot. Cross-query comparison
    // uses actual addresses plus the descriptor's physical fields instead.
    for(unsigned i=0;i<a.inner.size();++i){auto const& x=a.inner[i];auto const& y=b.inner[i];
        if(x.next!=y.next||x.dimension!=y.dimension||x.element_bytes!=y.element_bytes||x.flags!=y.flags||x.offset!=y.offset)return false;}
    return true;
}
}
