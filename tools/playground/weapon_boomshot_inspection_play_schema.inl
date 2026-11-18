struct PlayLayout {
    hand::ObjectIdentity function{};unsigned bytes{};std::array<unsigned,7> offsets{},masks{};
    bool operator==(PlayLayout const& b)const noexcept{return hand::same(function,b.function)&&bytes==b.bytes&&offsets==b.offsets&&masks==b.masks;}
};
bool play_layout(playground::Args& call,PlayLayout& out) noexcept {
    out={};PlayLayout value{};
    if(!lancer_charging::script_call(call,L"PlayWeaponAnim")||
       !vrequip::result_property(call,L"FloatProperty",4)||call.r.size>128||
       !nativeequipment::identify(nullptr,call.r.function,value.function))return false;
    const wchar_t* labels[]={L"AnimName",L"Rate",L"BlendInTime",L"BlendOutTime",L"bLooping",L"bOverride",L"ReturnValue"};
    const wchar_t* types[]={L"NameProperty",L"FloatProperty",L"FloatProperty",L"FloatProperty",L"BoolProperty",L"BoolProperty",L"FloatProperty"};
    value.bytes=call.r.size;
    for(unsigned i=0;i<7;++i){const auto p=call.prop(labels[i]);const unsigned width=i?4:8;
        if(!registered(p)||get<uptr>(p+0x40)!=call.r.function||get<unsigned>(p+0x70)!=1||
           get<unsigned>(p+0x74)!=width||!playground::named(get<uptr>(p+0x50),types[i])||
           (get<unsigned long long>(p+0x78)&0x580)!=(i==6?0x480u:0x80u))return false;
        const auto offset=get<unsigned>(p+0x94);if(offset>call.r.size||width>call.r.size-offset)return false;
        value.offsets[i]=offset;
        if(i==4||i==5){const auto mask=get<unsigned>(p+0xd0);if(!mask||(mask&(mask-1)))return false;value.masks[i]=mask;}
    }
    if(get<unsigned short>(call.r.function+0x108)!=value.offsets[6])return false;
    for(unsigned i=0;i<7;++i)for(unsigned j=i+1;j<7;++j){
        const auto a=value.offsets[i],b=value.offsets[j];const unsigned wa=i?4:8,wb=j?4:8;
        if(a<b+wb&&b<a+wa&&!(a==b&&value.masks[i]&&value.masks[j]&&!(value.masks[i]&value.masks[j])))return false;
    }
    out=value;return true;
}
