bool retained_metadata(metadata::Reader& r) noexcept {
    auto& schema=r.result;
    const auto identities=schema.identity_count,structures=schema.structure_count,fields=schema.field_count;
    for(U32 i=0;i<identities;++i){U32 slot{};
        if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(U32 i=0;i<structures;++i){metadata::Structure current{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,current)||
           !metadata::same_structure(current,schema.structures[i]))return false;}
    for(U32 i=0;i<fields;++i){metadata::Property current{};
        if(!r.property(schema.identities[schema.fields[i].identity].address,current)||
           !metadata::same_property(current,schema.fields[i]))return false;}
    for(U32 i=0;i<schema.name_count;++i){metadata::NameText current{};
        if(!metadata::fresh_name(schema.names,schema.name_text[i].index,current)||
           !metadata::same_name_text(current,schema.name_text[i]))return false;}
    metadata::Table objects{},names{};
    return base==schema.module_base&&metadata::tables(base,objects,names)&&
        metadata::same_table(objects,schema.objects)&&metadata::same_table(names,schema.names)&&
        schema.failure.reason==metadata::Reason::none;
}
