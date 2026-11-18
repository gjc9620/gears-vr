// Fixed HB animation layout query. The original typed Reader walk, shared
// between five names in this observation only. Current values/keys are NOT
// cached; read_latch_curve still double-reads all content and closes witnesses.
bool hb_curve_fields(meta::Reader& r,uptr object_address,std::array<weaponhand::BoolField,5>& out) noexcept {
    using U32=meta::U32;
    U32 slot{};if(!r.path(object_address,slot))return false;
    auto cls=r.result.identities[slot].klass;U32 extent{},depth{},remaining=5;
    std::array<bool,5> found{};std::array<weaponhand::BoolField,5> fields{};
    std::array<uptr,meta::max_parents> parents{};
    while(cls){
        if(depth==parents.size())return false;
        for(U32 i=0;i<depth;++i)if(parents[i]==cls)return false;
        parents[depth++]=cls;meta::Structure s{};if(!r.structure(cls,s))return false;
        if(depth==1)extent=s.extent;
        auto address=s.children;std::array<uptr,meta::max_search> seen{};U32 count{};
        weapon_metadata_parse::Walk traversal;
        while(address){
            if(count==seen.size()||traversal.seen(address,seen.data(),count))return false;
            seen[count++]=address;meta::Identity id{};meta::NameText text{};
            if(!meta::raw_identity(r.result.objects,r.result.names,address,id,text))return false;
            const int index=meta::exact_text(text,L"NumFrames")?0:
                meta::exact_text(text,L"RotationCompressionFormat")?1:
                meta::exact_text(text,L"CompressedTrackOffsets")?2:
                meta::exact_text(text,L"CompressedByteStream")?3:
                meta::exact_text(text,L"SequenceLength")?4:-1;
            if(index>=0&&!found[unsigned(index)]){
                auto& value=fields[unsigned(index)];value.object=object_address;value.owner=s.identity;
                if(id.name.number||!r.property(address,value.property)||
                   r.result.identities[value.property.identity].outer!=cls||value.property.dimension!=1||
                   !meta::property_fits(value.property,s.extent)||!meta::property_fits(value.property,extent))return false;
                const auto type=value.property.type;
                const bool valid=index==0?(value.property.element_bytes==4&&weaponhand::core_type(r,type,L"IntProperty")):
                    index==1?(value.property.element_bytes==1&&weaponhand::core_type(r,type,L"ByteProperty")):
                    index==4?(value.property.element_bytes==4&&weaponhand::core_type(r,type,L"FloatProperty")):
                    (value.property.element_bytes==16&&weaponhand::core_type(r,type,L"ArrayProperty"));
                if(!valid)return false;
                found[unsigned(index)]=true;
                if(!--remaining){out=fields;return true;}
            }
            if(!r.at(address,0x68,address))return false;
        }
        cls=s.super;
    }
    return false;
}
