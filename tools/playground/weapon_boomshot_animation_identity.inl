// Included in boomshot_animation. Read-only original animation identity and
// zero-notify proof; neither is permission to play, stop, seek or reload.
bool original_set(meta::Reader& r,hand::ObjectIdentity const& sequence) noexcept {
    unsigned object{},set{},folder{},package{};
    if(!weaponhand::object(r,sequence)||!weaponhand::derives(r,sequence.address,L"AnimSequence",L"Engine")||
       !r.path(sequence.address,object)||!r.path(r.result.identities[object].outer,set)||
       !r.named(set,L"Animset_Locust_Boomshot")||
       !weaponhand::derives(r,r.result.identities[set].address,L"AnimSet",L"Engine")||
       !r.path(r.result.identities[set].outer,folder)||!r.named(folder,L"animation")||
       !weaponhand::derives(r,r.result.identities[folder].address,L"Package",L"Core")||
       !r.path(r.result.identities[folder].outer,package)||!r.named(package,L"Locust_Boomshot")||
       !weaponhand::derives(r,r.result.identities[package].address,L"Package",L"Core")||
       r.result.identities[package].outer)return false;
    return true;
}
bool inspection_no_notifies(meta::Reader& r,hand::ObjectIdentity const& sequence) noexcept {
    if(!original_set(r,sequence))return false;
    weaponhand::BoolField named{},notifies{};meta::RawName raw{},again{};meta::NameText label{};
    meta::Table table{},after{};uptr inner{},inner_after{},structure{},structure_after{};
    meta::Property element{},fresh_element{};unsigned identity{},kind{},owner{},package{},extent{},extent_after{};
    if(!weaponhand::field(r,sequence.address,L"SequenceName",L"NameProperty",8,named)||
       !r.at(sequence.address,named.property.offset,raw)||raw.number||
       !meta::fresh_name(r.result.names,raw.index,label)||!meta::exact_text(label,L"AR_Reload")||
       !weaponhand::field(r,sequence.address,L"Notifies",L"ArrayProperty",16,notifies)||
       !r.at(sequence.address,notifies.property.offset,table)||table.count||table.capacity<0||table.capacity>1024||
       (table.capacity&&!table.data)||
       !r.at(r.result.identities[notifies.property.identity].address,0xd0,inner)||!r.property(inner,element)||
       r.result.identities[element.identity].outer!=r.result.identities[notifies.property.identity].address||
       element.dimension!=1||!weaponhand::core_type(r,element.type,L"StructProperty")||
       !r.at(inner,0xd0,structure)||!r.path(structure,identity)||!r.named(identity,L"AnimNotifyEvent")||
       !r.path(r.result.identities[identity].klass,kind)||!weaponhand::core_type(r,kind,L"ScriptStruct")||
       !r.path(r.result.identities[identity].outer,owner)||!r.named(owner,L"AnimSequence")||
       !r.path(r.result.identities[owner].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Engine")||
       r.result.identities[package].outer||!r.at(structure,0x88,extent)||!extent||extent>256||
       element.element_bytes!=((extent+3u)&~3u))return false;
    const auto retain=[&](meta::Property const& property) noexcept {
        for(unsigned i=0;i<r.result.field_count;++i)if(r.result.fields[i].identity==property.identity)
            return meta::same_property(r.result.fields[i],property);
        if(r.result.field_count==r.result.fields.size())return false;
        r.result.fields[r.result.field_count++]=property;return true;
    };
    if(!retain(named.property)||!retain(notifies.property)||!retain(element)||
       !r.at(sequence.address,named.property.offset,again)||again.index!=raw.index||again.number!=raw.number||
       !r.at(sequence.address,notifies.property.offset,after)||!meta::same_table(table,after)||
       !r.at(r.result.identities[notifies.property.identity].address,0xd0,inner_after)||inner_after!=inner||
       !r.property(inner,fresh_element)||!meta::same_property(element,fresh_element)||
       !r.at(inner,0xd0,structure_after)||structure_after!=structure||
       !r.at(structure,0x88,extent_after)||extent_after!=extent||!original_set(r,sequence)||
       !unified_weapon_fire::retained_metadata(r))return false;
    return true;
}
