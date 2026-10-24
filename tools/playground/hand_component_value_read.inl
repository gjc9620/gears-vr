// Included inside component, after Value and triple_schema. One local, non-VM
// read only: no property address or success escapes to a setter/another group.
struct MeshValueFields {
    uptr mesh{},klass{};
    std::array<uptr,5> properties{};
    inline static constexpr const wchar_t* labels[]={L"AbsoluteTranslation",L"AbsoluteRotation",
        L"AbsoluteScale",L"Translation",L"Rotation"};
    explicit MeshValueFields(uptr object):mesh(object){}
    bool begin(){
        if(!registered(mesh))return false;
        klass=get<uptr>(mesh+0x50);
        return registered(klass)&&playground::fields(klass,labels,properties)&&
            registered(mesh)&&get<uptr>(mesh+0x50)==klass;
    }
    bool finish() const {
        std::array<uptr,5> current{};
        return registered(mesh)&&get<uptr>(mesh+0x50)==klass&&
            playground::fields(klass,labels,current)&&current==properties&&
            registered(mesh)&&get<uptr>(mesh+0x50)==klass;
    }
};
bool read_mesh_values(uptr mesh,Value& out){
    Value value{};
    MeshValueFields fields(mesh);
    if(!fields.begin())return false;
    for(unsigned i=0;i<3;++i){
        unsigned bits{};
        const auto property=fields.properties[i];
        if(!lookup::field_at(mesh,property,L"BoolProperty",&bits,4))return false;
        const auto mask=get<unsigned>(property+0xd0);
        if(!mask||(mask&(mask-1)))return false;
        value.absolute[i]=(bits&mask)!=0;
    }
    if(!triple_schema(fields.properties[3],false)||!triple_schema(fields.properties[4],true)||
       !lookup::field_at(mesh,fields.properties[3],L"StructProperty",value.translation.data(),12)||
       !lookup::field_at(mesh,fields.properties[4],L"StructProperty",value.rotation.data(),12))return false;
    std::array<float,3> position{};std::memcpy(position.data(),value.translation.data(),12);
    for(float component:position)if(!std::isfinite(component)||std::abs(component)>1e7f)return false;
    if(!fields.finish())return false;
    out=value;return true;
}
