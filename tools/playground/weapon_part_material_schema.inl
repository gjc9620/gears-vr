// Exact original MeshComponent material ABI. This inspection does not invoke
// either function and can also authenticate a read on an original component.
// Retail GetMaterial and SetMaterial are packed: 12 bytes, Int at 0 and
// object at 4. Their return/input flags and return offsets remain distinct.
bool material_schema(playground::Args& call,bool writing) noexcept {
    const auto function=call.r.function;
    if(!native_function(call,writing?L"SetMaterial":L"GetMaterial",L"MeshComponent")||
       call.r.size!=12||get<unsigned>(function+0x88)<call.r.size)return false;
    const auto index=call.prop(L"ElementIndex");
    const auto material=call.prop(writing?L"Material":L"ReturnValue");
    if(!hammerburst_animation::input_field(call,index,L"IntProperty",4)||
       !registered(material)||get<uptr>(material+0x40)!=function||
       !playground::named(get<uptr>(material+0x50),L"ObjectProperty")||
       get<unsigned>(material+0x70)!=1||get<unsigned>(material+0x74)!=8||
       (get<unsigned long long>(material+0x78)&0x580)!=(writing?0x80ull:0x580ull))return false;
    const auto a=get<unsigned>(index+0x94),b=get<unsigned>(material+0x94);
    if(a!=0||b!=4||b>call.r.size||8>call.r.size-b||!(a+4<=b||b+8<=a)||
       get<unsigned short>(function+0x108)!=(writing?0xffffu:b))return false;
    const auto klass=get<uptr>(material+0xd0),package=klass?get<uptr>(klass+0x40):0;
    if(!registered(klass)||!registered(package)||!playground::named(klass,L"MaterialInterface")||
       !playground::named(get<uptr>(klass+0x50),L"Class")||!playground::named(package,L"Engine")||
       !playground::named(get<uptr>(package+0x50),L"Package")||get<uptr>(package+0x40))return false;
    unsigned count{},parameters{};std::array<uptr,32> visited{};
    for(auto child=get<uptr>(function+0x80);child;child=get<uptr>(child+0x68)){
        if(count==visited.size()||!registered(child)||get<uptr>(child+0x40)!=function)return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;
        if(get<unsigned long long>(child+0x78)&0x80){
            if(child!=index&&child!=material)return false;
            ++parameters;
        }
    }
    return parameters==2;
}
