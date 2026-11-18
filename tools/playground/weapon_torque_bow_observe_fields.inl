// Query-local Bow descriptor grouping. Values remain fresh and the complete
// first-match selection is repeated at finish; no binding crosses read_once.
using U32=meta::U32;using BoolField=weaponhand::BoolField;
struct BowObserveFields {
    enum Index:U32 {pending_end,inv_manager,fire_mode,charge_fx,audio,trail_fx,failed_fx,endpoint,full_endpoint,failed_endpoint,simulation,count};
private:
    uptr object_{},class_{};std::array<BoolField,count> fields_{};bool ready_{};
    static Index label(meta::NameText const& text) noexcept {
        if(meta::exact_text(text,L"bPendingEndFire"))return pending_end;
        if(meta::exact_text(text,L"InvManager"))return inv_manager;
        if(meta::exact_text(text,L"CurrentFireMode"))return fire_mode;
        if(meta::exact_text(text,L"PSC_TorqueBowChargeUp"))return charge_fx;
        if(meta::exact_text(text,L"AimingSound"))return audio;
        if(meta::exact_text(text,L"PSC_Trail"))return trail_fx;
        if(meta::exact_text(text,L"PSC_FailedTrail"))return failed_fx;
        if(meta::exact_text(text,L"EM_EndPoint"))return endpoint;
        if(meta::exact_text(text,L"EM_EndPointFullCharge"))return full_endpoint;
        if(meta::exact_text(text,L"EM_EndPointFail"))return failed_endpoint;
        if(meta::exact_text(text,L"SimulatedGrenade"))return simulation;
        return count;
    }
    static bool type(meta::Reader& r,U32 slot,Index index) noexcept {
        if(index==pending_end)return weaponhand::core_type(r,slot,L"BoolProperty");
        if(index==fire_mode)return weaponhand::core_type(r,slot,L"ByteProperty");
        if(index>=charge_fx&&index<=failed_fx)return weaponhand::core_type(r,slot,L"ComponentProperty");
        return weaponhand::core_type(r,slot,L"ObjectProperty");
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
            weapon_metadata_parse::Walk traversal;
            while(address){
                if(searched==seen.size())return false;
                if(traversal.seen(address,seen.data(),searched))return false;
                seen[searched++]=address;meta::Identity id{};meta::NameText text{};
                if(!meta::raw_identity(r.result.objects,r.result.names,address,id,text))return false;
                const auto index=label(text);
                // A first same-name field of the wrong type rejects. Never
                // skip it in search of a compatible field in a base class.
                if(index<count&&!found[index].property.element_bytes){
                    BoolField value{};value.object=object;value.owner=s.identity;
                    const U32 bytes=index==pending_end?4u:index==fire_mode?1u:8u;
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
    bool binding(meta::Reader& r,Index index,BoolField& out) const noexcept {
        if(!current(r,index))return false;out=fields_[index];return true;
    }
    bool finish(meta::Reader& r) noexcept {
        r.reuse_initial_identities=false; // Closing first-match selection is always fresh.
        if(!ready_)return false;ready_=false;
        std::array<BoolField,count> fresh{};uptr cls{};
        if(!resolve(r,object_,fresh,cls)||cls!=class_)return false;
        for(U32 i=0;i<count;++i)if(fresh[i].object!=fields_[i].object||fresh[i].owner!=fields_[i].owner||
            !meta::same_property(fresh[i].property,fields_[i].property))return false;
        return true;
    }
};
