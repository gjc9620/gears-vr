// Included inside idle_inventory. Query-local selector batching only; no
// object value, field selection or permission survives this function call.
struct Field {abi::ObjectIdentity identity{};unsigned offset{},width{};};
bool typed_at(uptr owner,uptr property,const wchar_t* kind,unsigned width,Field& out) noexcept {
    const auto klass=get<uptr>(owner+0x50);
    if(!registered(property)||!registered(klass)||!playground::named(get<uptr>(property+0x50),kind)||
       get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=width)return false;
    const auto at=get<unsigned>(property+0x94),extent=get<unsigned>(klass+0x88);
    Field value{};
    if(at>extent||width>extent-at||owner>UINTPTR_MAX-at-width||
       !identify(nullptr,property,value.identity))return false;
    value.offset=at;value.width=width;out=value;return true;
}
bool typed(uptr owner,const wchar_t* label,const wchar_t* kind,unsigned width,Field& out) noexcept {
    return typed_at(owner,playground::property(owner,label),kind,width,out);
}
bool weapon_fields(uptr owner,Field& next,Field& slot) noexcept {
    if(&next==&slot||!registered(owner))return false;
    const auto klass=get<uptr>(owner+0x50);if(!registered(klass))return false;
    Field a{},b{};
    // Same existing first-name selector as separate property lookups. A wrong
    // type in a derived class is rejected, never skipped for a base field.
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    constexpr const wchar_t* labels[]={L"Inventory",L"CharacterSlot"};
    std::array<uptr,2> fields{};
    if(!playground::fields(klass,labels,fields)||
       !typed_at(owner,fields[0],L"ObjectProperty",8,a)||
       !typed_at(owner,fields[1],L"ByteProperty",1,b))return false;
#else
    if(!typed(owner,L"Inventory",L"ObjectProperty",8,a)||
       !typed(owner,L"CharacterSlot",L"ByteProperty",1,b))return false;
#endif
    next=a;slot=b;return true;
}
