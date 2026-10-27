#define main original_asset_fixture
#include "weapon_lancer_asset_read_test.cpp"
#undef main
namespace {
struct Array {uptr data{};int count{},capacity{};};
using Name=meta::RawName;
template<class T>T get(uptr p){T v{};copy(&v,p,sizeof(v));return v;}
bool registered(uptr p){const auto t=get<meta::Table>(base+0x1c24730);const auto i=get<unsigned>(p+8);
    return p&&i<unsigned(t.count)&&get<uptr>(t.data+8ull*i)==p;}
namespace playground {
bool named(uptr p,wchar_t const* label){const auto n=get<Name>(p+0x48);const auto t=get<meta::Table>(base+0x1c34800);
    if(!p||n.number||n.index>=unsigned(t.count))return false;const auto entry=get<uptr>(t.data+8ull*n.index);
    wchar_t text[96]{};return copy(text,entry+20,(std::wcslen(label)+1)*sizeof(wchar_t))&&!std::wcscmp(text,label);}
uptr property(uptr object,wchar_t const* label){
    for(auto type=get<uptr>(object+0x50);type;type=get<uptr>(type+0x60))
        for(auto p=get<uptr>(type+0x80);p;p=get<uptr>(p+0x68))if(named(p,label))return p;
    return 0;
}
uptr field(uptr type,wchar_t const* label,wchar_t const* kind){
    for(auto p=get<uptr>(type+0x80);p;p=get<uptr>(p+0x68))if(named(p,label)&&named(get<uptr>(p+0x50),kind))return p;
    return 0;
}
uptr pointer(uptr object,wchar_t const* label){const auto p=property(object,label);return p?get<uptr>(object+get<unsigned>(p+0x94)):0;}
template<std::size_t N>bool instance(uptr object,wchar_t const (&label)[N]){meta::Snapshot s{};meta::Reader r{s,meta::Stage::final_check};
    return meta::tables(base,s.objects,s.names)&&weaponhand::derives(r,object,label,L"Engine");}
bool deleted(uptr object){return !registered(object)||(get<unsigned>(object+0x108)&4)!=0;}
}
namespace nativeequipment {
namespace abi=gears_weapon_hand;
bool hand_bone(Name value) noexcept {
    if(value.number)return false;const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||names.capacity>4000000||value.index>=unsigned(names.count))return false;
    const auto entry=get<uptr>(names.data+size_t(value.index)*8);wchar_t text[20]{};constexpr wchar_t expected[]=L"b_MF_Weapon_R_End";
    return entry&&get<unsigned>(entry)==value.index&&copy(text,entry+20,sizeof expected)&&!std::memcmp(text,expected,sizeof expected);
}
}
#include "body_mount_attachment_read.inl"
struct Attached:AssetMemory {
    std::array<unsigned char,104> tuples{};uptr array_property{},structure{};
    Attached(){
        const auto array_type=node(L"ArrayProperty",class_type,core),struct_type=node(L"StructProperty",class_type,core);
        const auto name_type=node(L"NameProperty",class_type,core);
        structure=node(L"AttachmentEntry",class_type,engine);put(structure,0x88,52u);
        array_property=property(mesh_class,L"Attachments",array_type,0x130,sizeof(Array));
        const auto inner=node(L"AttachmentInner",struct_type,mesh_class);put(inner,0x74,52u);put(inner,0xd0,structure);put(array_property,0xd0,inner);
        property(structure,L"Component",component_type,0,8);property(structure,L"BoneName",name_type,8,8);
        property(structure,L"RelativeLocation",struct_type,16,12);property(structure,L"RelativeRotation",struct_type,28,12);
        property(structure,L"RelativeScale",struct_type,40,12);
        std::memcpy(tuples.data(),&component,8);const auto bone=text(L"b_MF_Weapon_R_End");std::memcpy(tuples.data()+8,&bone,8);
        put(mesh,0x130,Array{reinterpret_cast<uptr>(tuples.data()),1,2});tables();
    }
    abi::EquipmentIdentity identity_view(){abi::EquipmentIdentity i{};i.key=frame.key;i.world=frame.world;i.controller=frame.controller;
        i.pawn=frame.pawn;i.mesh=frame.mesh;i.weapon=frame.weapon;return i;}
};
}
int main(){
    unsigned n{};
    for(unsigned changed=0;changed<9;++changed){auto m=std::make_unique<Attached>();auto i=m->identity_view();auto expected=m->component;
        switch(changed){case 1:expected=m->asset;break;case 2:Memory::put(m->component,0x110,2u);break;
        case 3:{const auto bone=m->text(L"wrong_bone");std::memcpy(m->tuples.data()+8,&bone,8);m->tables();break;}
        case 4:std::memcpy(m->tuples.data()+52,m->tuples.data(),52);Memory::put(m->mesh,0x130,Array{reinterpret_cast<uptr>(m->tuples.data()),2,2});break;
        case 5:Memory::put(m->structure,0x88,56u);break;
        case 6:{const auto nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(m->tuples.data()+40,&nan,4);break;}
        case 7:mutate_read=m->mesh+0x130;mutation=[&]{Memory::put(m->mesh,0x130,Array{});};break;
        case 8:Memory::put(m->weapon,0x110,m->pc);break;}
        const auto okay=nativeequipment::read_attached_equipment(i,expected);assert(okay==(changed==0));++n;
    }
    std::printf("BodyMountAttachmentTest: %u cases; actual typed attachment read, owned array/metadata-resolution seams, no Client/engine qualification\n",n);
}
