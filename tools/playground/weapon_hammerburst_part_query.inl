// HB-only, query-local batching. No address or successful observation survives
// an Engine call as a permission to resolve the next call's parameter layout.
bool hb_result_field(playground::Args const& call,uptr p,const wchar_t* kind,unsigned bytes) noexcept {
    return call.valid&&p&&get<unsigned>(p+0x74)==bytes&&
        playground::named(get<uptr>(p+0x50),kind)&&get<unsigned>(p+0x94)<=call.r.size&&
        bytes<=call.r.size-get<unsigned>(p+0x94);
}
bool hb_part_layout(playground::Args& call,const wchar_t* input,const wchar_t* kind,unsigned input_bytes,
    const wchar_t* result_kind,unsigned result_bytes,std::array<uptr,2>& fields) noexcept {
    const wchar_t* labels[]={input,L"ReturnValue"};
    if(!playground::fields(call.r.function,labels,fields)||
       !input_field(call,fields[0],kind,input_bytes)||!hb_result_field(call,fields[1],result_kind,result_bytes))return false;
    const auto a=get<unsigned>(fields[0]+0x94),b=get<unsigned>(fields[1]+0x94);
    return a+input_bytes<=b||b+result_bytes<=a;
}
bool hb_matrix_structure(uptr result) noexcept {
    const auto structure=get<uptr>(result+0xd0);
    return registered(structure)&&playground::named(structure,L"Matrix")&&get<unsigned>(structure+0x88)==64;
}
bool hb_read_one_part(uptr component,Name named,int bone_index,std::array<float,16>& out) noexcept {
    playground::Args name,matrix;
    // Existing query-local two-function resolver; no Longshot state or writer.
    if(!longshot_bone_args(component,name,matrix)||
       !component_getter(name,L"MatchRefBone")||!component_getter(matrix,L"GetBoneMatrix"))return false;
    std::array<uptr,2> name_fields{},matrix_fields{};
    if(!hb_part_layout(name,L"BoneName",L"NameProperty",8,L"IntProperty",4,name_fields)||
       !hb_part_layout(matrix,L"BoneIndex",L"IntProperty",4,L"StructProperty",64,matrix_fields)||
       !hb_matrix_structure(matrix_fields[1]))return false;
    std::memcpy(name.r.params+get<unsigned>(name_fields[0]+0x94),&named,sizeof named);
    if(!name.run())return false;
    // Engine may have changed metadata: preserve fresh return lookup.
    const auto name_result=name.prop(L"ReturnValue");
    if(!hb_result_field(name,name_result,L"IntProperty",4))return false;
    int actual{};std::memcpy(&actual,name.r.params+get<unsigned>(name_result+0x94),sizeof actual);
    if(actual!=bone_index)return false;
    // Original matrix.set resolved its input AFTER MatchRefBone. Keep that
    // boundary, including a fresh receiver/function selection.
    matrix=playground::Args(component,L"GetBoneMatrix");
    if(!component_getter(matrix,L"GetBoneMatrix")||
       !hb_part_layout(matrix,L"BoneIndex",L"IntProperty",4,L"StructProperty",64,matrix_fields)||
       !hb_matrix_structure(matrix_fields[1]))return false;
    std::memcpy(matrix.r.params+get<unsigned>(matrix_fields[0]+0x94),&bone_index,sizeof bone_index);
    if(!matrix.run())return false;
    const auto result=matrix.prop(L"ReturnValue");
    if(!hb_result_field(matrix,result,L"StructProperty",64)||!hb_matrix_structure(result))return false;
    std::memcpy(out.data(),matrix.r.params+get<unsigned>(result+0x94),sizeof out);
    return true;
}
