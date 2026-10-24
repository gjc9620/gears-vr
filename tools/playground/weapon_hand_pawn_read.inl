// Included inside weaponhand after the original single-field readers. Only
// one observe owns this binding; no value or descriptor crosses a callback.
struct PawnObserveFields {
    enum Index:U32 {controller_link,mesh_link,weapon_link,deleted_flag,switching_flag,no_firing_flag,count};
private:
    uptr object_{},class_{};std::array<BoolField,count> fields_{};bool ready_{};
    static Index label(meta::NameText const& text) noexcept {
        if(meta::exact_text(text,L"Controller"))return controller_link;
        if(meta::exact_text(text,L"Mesh"))return mesh_link;
        if(meta::exact_text(text,L"Weapon"))return weapon_link;
        if(meta::exact_text(text,L"bDeleteMe"))return deleted_flag;
        if(meta::exact_text(text,L"bSwitchingWeapons"))return switching_flag;
        if(meta::exact_text(text,L"bNoWeaponFiring"))return no_firing_flag;
        return count;
    }
    static bool type(meta::Reader& r,U32 slot,Index index) noexcept {
        if(index==mesh_link)return core_type(r,slot,L"ComponentProperty");
        return index<deleted_flag?core_type(r,slot,L"ObjectProperty"):core_type(r,slot,L"BoolProperty");
    }
    static bool resolve(meta::Reader& r,uptr object,std::array<BoolField,count>& out,uptr& object_class) noexcept {
        U32 object_slot{};if(!r.path(object,object_slot))return false;
        const auto first_class=r.result.identities[object_slot].klass;
        auto cls=first_class;U32 extent{},depth{},remaining=count;
        std::array<BoolField,count> found{};std::array<uptr,meta::max_parents> parents{};
        while(cls){
            if(depth==parents.size())return false;
            for(U32 i=0;i<depth;++i)if(parents[i]==cls)return false;
            parents[depth++]=cls;meta::Structure s{};if(!r.structure(cls,s))return false;
            if(depth==1)extent=s.extent;
            auto address=s.children;std::array<uptr,meta::max_search> seen{};U32 searched{};
            while(address){
                if(searched==seen.size())return false;
                for(U32 i=0;i<searched;++i)if(seen[i]==address)return false;
                seen[searched++]=address;meta::Identity id{};meta::NameText text{};
                if(!meta::raw_identity(r.result.objects,r.result.names,address,id,text))return false;
                const auto index=label(text);
                // A first same-name field of the wrong type rejects. Never
                // skip it in search of a compatible field in a base class.
                if(index<count&&!found[index].property.element_bytes){
                    BoolField value{};value.object=object;value.owner=s.identity;
                    const U32 bytes=index<deleted_flag?8u:4u;
                    if(id.name.number||!r.property(address,value.property)||
                       r.result.identities[value.property.identity].outer!=cls||!type(r,value.property.type,index)||
                       value.property.dimension!=1||value.property.element_bytes!=bytes||
                       !meta::property_fits(value.property,s.extent)||!meta::property_fits(value.property,extent))return false;
                    found[index]=value;
                    if(!--remaining){out=found;object_class=first_class;return true;}
                }
                if(!r.at(address,0x68,address))return false;
            }
            cls=s.super;
        }
        return false;
    }
    bool current(meta::Reader& r,Index index) const noexcept {
        if(!ready_||index>=count)return false;
        U32 object_slot{};auto const& field=fields_[index];meta::Structure owner{};meta::Property fresh{};
        return r.path(object_,object_slot)&&r.result.identities[object_slot].klass==class_&&
            r.structure(r.result.identities[field.owner].address,owner)&&owner.identity==field.owner&&
            r.property(r.result.identities[field.property.identity].address,fresh)&&meta::same_property(fresh,field.property);
    }
public:
    bool begin(meta::Reader& r,uptr object) noexcept {
        ready_=false;std::array<BoolField,count> fields{};uptr cls{};
        if(!resolve(r,object,fields,cls))return false;
        object_=object;class_=cls;fields_=fields;ready_=true;return true;
    }
    bool pointer(meta::Reader& r,Index index,uptr expected,meta::PointerField& out) const noexcept {
        if(index>=deleted_flag||!current(r,index))return false;
        auto const& field=fields_[index];meta::PointerField value{};
        if(!r.at(object_,field.property.offset,value.value)||value.value!=expected)return false;
        value.property=field.property;value.owner=field.owner;out=value;return true;
    }
    bool boolean(meta::Reader& r,Index index,BoolField& out) const noexcept {
        if(index<deleted_flag||index>=count||!current(r,index))return false;
        auto value=fields_[index];
        if(!r.at(r.result.identities[value.property.identity].address,0xd0,value.mask)||!value.mask||(value.mask&(value.mask-1))||
           !r.at(object_,value.property.offset,value.bits))return false;
        out=value;return true;
    }
    bool finish(meta::Reader& r) noexcept {
        if(!ready_)return false;ready_=false;
        std::array<BoolField,count> fresh{};uptr cls{};
        if(!resolve(r,object_,fresh,cls)||cls!=class_)return false;
        for(U32 i=0;i<count;++i)if(fresh[i].object!=fields_[i].object||fresh[i].owner!=fields_[i].owner||
            !meta::same_property(fresh[i].property,fields_[i].property))return false;
        return true;
    }
};
