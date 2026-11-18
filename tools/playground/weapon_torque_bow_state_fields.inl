// Original Bow state field values and fingerprints, shared by production and
// owned-memory verification. Included inside torquebow_state.
struct Fields {
    meta::Reader& r;unified_weapon_fire::Fingerprint hash;BowObserveFields weapon;
    bool retain(meta::Property const& p) noexcept {
        if(r.result.field_count==r.result.fields.size())return false;
        r.result.fields[r.result.field_count++]=p;return true;
    }
    template<std::size_t N>bool flag(uptr object,wchar_t const(&label)[N],bool& out,BowObserveFields::Index index=BowObserveFields::count) noexcept {
        weaponhand::BoolField f{};
        const bool bound=index==BowObserveFields::count?weaponhand::boolean(r,object,label,f):
            weapon.binding(r,index,f)&&f.object==object&&index==BowObserveFields::pending_end&&
            r.at(r.result.identities[f.property.identity].address,0xd0,f.mask)&&f.mask&&!(f.mask&(f.mask-1))&&r.at(object,f.property.offset,f.bits);
        if(!bound||!retain(f.property)){failed_field=label;return false;}
        out=(f.bits&f.mask)!=0;hash.word(object);hash.word(f.property.offset);hash.word(f.mask);hash.word(f.bits&f.mask);return true;
    }
    template<std::size_t N>bool pointer(uptr object,wchar_t const(&label)[N],uptr& out,bool component=false,BowObserveFields::Index index=BowObserveFields::count) noexcept {
        meta::PointerField f{};
        bool bound{};
        if(index==BowObserveFields::count)bound=weaponhand::read_pointer(r,object,label,f,component);
        else {weaponhand::BoolField field{};bound=weapon.binding(r,index,field)&&field.object==object&&
            index!=BowObserveFields::pending_end&&index!=BowObserveFields::fire_mode&&
            component==(index>=BowObserveFields::charge_fx&&index<=BowObserveFields::failed_fx)&&r.at(object,field.property.offset,f.value);
            if(bound){f.property=field.property;f.owner=field.owner;}}
        if(!bound||!retain(f.property)){failed_field=label;return false;}
        out=f.value;hash.word(object);hash.word(f.property.offset);hash.word(out);return true;
    }
    bool particle(uptr object,bool& active) noexcept {
        if(!object){active=false;return true;}
        bool hidden{},running{};
        if(!weaponhand::derives(r,object,L"ParticleSystemComponent",L"Engine")||
           !flag(object,L"HiddenGame",hidden)||!flag(object,L"bIsActive",running))return false;
        active=!hidden||running;return true;
    }
    bool emitter(uptr actor,bool& visible) noexcept {
        if(!actor){visible=false;return true;}
        bool hidden{};
        if(!weaponhand::derives(r,actor,L"Emitter",L"Engine")||!flag(actor,L"bHidden",hidden))return false;
        visible=!hidden;return true;
    }
};
