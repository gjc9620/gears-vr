// Actual invoke_with wrapper/ABI against synthetic memory only. No DLL/game.
#define GEARS_PATH_CHOICE_CLOSE_OBSERVER 1
#define main layout_fixture_tests
#include "path_choice_layout_observer_test.cpp"
#undef main
#include "path_choice_close_observer.inl"
namespace close_obs=path_choice_close_observer;
namespace close_core=gears_path_choice_close;
namespace path_choice_close_observer {bool read_own_entry(void* out,void const* from,std::size_t size){return safe_copy(out,from,size);}}
static unsigned close_calls{},close_mode{},close_nested{};
static void* seen_client{};static void* seen_scene{};
static std::promise<void>* close_entered{},*close_release{};
static uintptr_t close_call(void*,void*);
static uintptr_t __fastcall close_original(void* client,void* scene){
    ++close_calls;seen_client=client;seen_scene=scene;
    if(close_mode==1){auto const& r=std::atomic_load(&close_obs::selected)->request;
        auto h=r.initial;h.num=1;put(r.active_header,h);put(h.data,uintptr_t(0x8001));}
    if(close_mode==2&&!close_nested){++close_nested;assert(close_call(client,scene)==sentinel);--close_nested;}
    if(close_mode==3)throw 123;
    if(close_mode==4){close_entered->set_value();close_release->get_future().wait();}
    if(close_mode==5){auto c=position::state.lease.config;put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);position::state={};}
    return sentinel;
}
static uintptr_t close_call(void* client,void* scene){return close_obs::invoke_with(client,scene,0x123456789abcdef0ull,
    close_original,safe_copy,[]{return true;});}
struct CloseFixture {
    Fixture f;close_core::Request request;
    template<class T> void check(uintptr_t at,T const& value){put(at,value);trial::Check row{at,{}};
        auto first=reinterpret_cast<unsigned char const*>(&value);row.bytes.assign(first,first+sizeof(value));f.c.checks.push_back(row);}
    CloseFixture(){
        close_calls=close_mode=close_nested=0;close_obs::disable();close_obs::slots=0;close_obs::entries=0;close_obs::depth=0;
        for(auto& b:close_obs::bypass)b=0;for(unsigned i=0;i<2;++i){close_obs::rows[i]={};close_obs::ready[i]=false;}
        close_obs::original=close_original;
        trial::RegistryHeader objects{0x9000,128,256};check(base+0x1c24730,objects);
        check(base+0x1c34800,trial::RegistryHeader{0xa000,128,256});
        unsigned index=1;
        for(auto object:{f.c.client,f.c.scene,f.c.panel,uintptr_t(0x7000)}){
            check(object+8,index);check(objects.data+index*8,object);check(object+64,uintptr_t(0x60000));
            check(object+72,std::array<std::uint32_t,2>{10+index,0});check(object+80,uintptr_t(0x8000));++index;}
        put(f.c.client,base+close_core::client_table_rva);put(base+close_core::client_table_rva+close_core::close_slot,base+close_core::close_rva);
        request.nonce=f.c.nonce;request.pid=f.c.pid;request.creation_time=f.c.creation_time;
        request.started_qpc=f.c.started_qpc;request.world_generation=f.c.world_generation;request.base=base;
        request.client=f.c.client;request.scene=f.c.scene;request.panel=f.c.panel;request.duration_ms=8000;
        request.active_header=f.c.client+0x100;request.initial={0xb000,2,4};request.members={f.c.scene,0x8001};
        request.function=0x7000;request.function_index=4;request.function_name_index=14;
        request.function_outer=0x60000;request.function_class=0x8000;
        check(0x8000+96,uintptr_t(0x60000));put(0x60000+128,uintptr_t(0x7000));
        put(0x8000+8,unsigned(5));put(objects.data+5*8,uintptr_t(0x8000));put(0x8000+72,std::array<unsigned,2>{15,0});
        put(0xa000+14*8,uintptr_t(0xc000));put(0xc000,unsigned(14));
        put(0xa000+15*8,uintptr_t(0xc100));put(0xc100,unsigned(15));
        const char16_t close_name[]=u"CloseScene",function_name[]=u"Function";
        put(0xc000+20,close_name);put(0xc100+20,function_name);
        check(request.active_header,request.initial);check(request.initial.data,std::array<uintptr_t,2>{f.c.scene,0x8001});
        put(request.function+0x114,base+close_core::exec_rva);f.apply();
        auto active_request=std::make_shared<close_obs::Active>();active_request->request=request;active_request->expires=GetTickCount64()+8000;
        std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(active_request));
    }
    uintptr_t call(){return close_call(reinterpret_cast<void*>(f.c.client),reinterpret_cast<void*>(f.c.scene));}
};
static uint64_t close_bypass(close_obs::Bypass b){return close_obs::bypass[unsigned(b)].load();}
static void normal_close(){
    CloseFixture f;auto before=memory;close_mode=1;assert(f.call()==sentinel);assert(close_calls==1&&close_obs::slots==1&&close_obs::ready[0]);
    auto const& row=close_obs::rows[0];assert(row.normal&&row.rax==sentinel&&row.activation_before&&!row.activation_after&&row.owned_before);
    assert(row.before.mask==127&&row.after.mask==127&&row.before.occurrences==1&&row.after.occurrences==0);
    assert(row.before.position==row.after.position&&row.before.units==row.after.units&&row.owned==position::state.lease.owned);
    assert(row.before.active.num==2&&row.after.active.num==1&&row.exit>=row.enter&&row.caller==0x123456789abcdef0ull);
    before[f.request.active_header+8]=1;put(f.request.active_header+8,int(1));
    // The only synthetic writes were made by close_original's membership removal.
    for(auto const& cell:memory)if(cell.first<f.request.active_header||cell.first>=f.request.active_header+16)
        if(cell.first<f.request.initial.data||cell.first>=f.request.initial.data+8)assert(before[cell.first]==cell.second);
    assert(position::state.lease.active&&position::state.lease.touched[0]&&position::state.lease.touched[1]);
}
static void close_gates(){
    CloseFixture f;auto denied=[&](close_obs::Bypass why){auto calls_before=close_calls;auto b=close_bypass(why);assert(f.call()==sentinel);
        assert(close_calls==calls_before+1&&close_obs::slots==0&&close_bypass(why)==b+1);};
    auto reads=read_calls.load();std::thread foreign([&]{denied(close_obs::Bypass::foreign_thread);});foreign.join();assert(read_calls==reads);
    ++position::successful_nonce;denied(close_obs::Bypass::no_lease);--position::successful_nonce;
    ++committed_world_generation;denied(close_obs::Bypass::world);--committed_world_generation;
    ++session_started_qpc;denied(close_obs::Bypass::identity);--session_started_qpc;
    void* wrong=reinterpret_cast<void*>(0x1234);assert(close_call(wrong,reinterpret_cast<void*>(f.f.c.scene))==sentinel);assert(seen_client==wrong);
    assert(close_call(reinterpret_cast<void*>(f.f.c.client),wrong)==sentinel);assert(seen_scene==wrong);assert(close_obs::slots==0);
    memory[0x5000]^=1;denied(close_obs::Bypass::identity);memory[0x5000]^=1;
    put(f.f.c.panel+0x64,position::state.lease.owned[0]+1);denied(close_obs::Bypass::identity);put(f.f.c.panel+0x64,position::state.lease.owned[0]);
    put(f.request.function+0x114,base+close_core::exec_rva+1);denied(close_obs::Bypass::identity);put(f.request.function+0x114,base+close_core::exec_rva);
    close_obs::disable();denied(close_obs::Bypass::no_request);
    assert(close_obs::hook(reinterpret_cast<void*>(f.f.c.client),reinterpret_cast<void*>(f.f.c.scene))==sentinel);
    assert(seen_client==reinterpret_cast<void*>(f.f.c.client)&&seen_scene==reinterpret_cast<void*>(f.f.c.scene));
    auto expired=std::make_shared<close_obs::Active>();expired->request=f.request;expired->expires=0;
    std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(expired));denied(close_obs::Bypass::no_request);
}
static void bounds_exceptions(){
    {CloseFixture f;auto before=memory;close_mode=2;assert(f.call()==sentinel);assert(close_calls==2&&close_obs::slots==1&&memory==before);
        assert(close_bypass(close_obs::Bypass::nested)==1);close_mode=0;f.call();f.call();assert(close_obs::slots==2&&close_bypass(close_obs::Bypass::quota)==1);}
    {CloseFixture f;close_mode=3;try{f.call();assert(false);}catch(int value){assert(value==123);}
        assert(close_calls==1&&close_obs::ready[0]&&!close_obs::rows[0].normal&&close_obs::depth==0);}
    {CloseFixture f;assert(close_obs::invoke_with(reinterpret_cast<void*>(f.f.c.client),reinterpret_cast<void*>(f.f.c.scene),1,
        close_original,safe_copy,[&]{assert(f.call()==sentinel);return true;})==sentinel);
        assert(close_calls==2&&close_obs::slots==1&&close_bypass(close_obs::Bypass::nested)==1&&close_obs::depth==0);}
    {CloseFixture f;close_mode=5;assert(f.call()==sentinel);assert(close_obs::rows[0].after.position[1]==f.f.c.top_bits);
        assert(close_obs::rows[0].nonce==f.f.c.nonce);}
    {CloseFixture f;close_mode=4;std::promise<void> entered_local,released_local;close_entered=&entered_local;close_release=&released_local;
        std::thread worker([&]{position::owner_thread=GetCurrentThreadId();assert(f.call()==sentinel);});
        entered_local.get_future().wait();assert(close_obs::slots==1&&!close_obs::ready[0].load(std::memory_order_acquire));
        FILE* output{};assert(!tmpfile_s(&output)&&output);close_obs::progress(output);rewind(output);
        std::string text;char bytes[1024]{};while(auto count=fread(bytes,1,sizeof(bytes),output))text.append(bytes,count);
        assert(text.find("path_close_0=")==std::string::npos);fclose(output);
        released_local.set_value();worker.join();assert(close_obs::ready[0].load(std::memory_order_acquire));}
}
static void certification(){
    memory.clear();std::array<unsigned char,62> close{};std::array<unsigned char,160> exec{};
    for(unsigned i=0;i<close.size();++i)close[i]=static_cast<unsigned char>(i+1);
    for(unsigned i=0;i<exec.size();++i)exec[i]=static_cast<unsigned char>(i+3);
    for(unsigned i=0;i<62;++i)memory[base+close_core::close_rva+i]=close[i];
    for(unsigned i=0;i<128;++i)memory[base+close_core::exec_rva+i]=exec[i];
    for(unsigned i=0;i<32;++i)memory[base+close_core::exec_cold_rva+i]=exec[128+i];
    put(base+close_core::client_table_rva+close_core::close_slot,base+close_core::close_rva);
    auto matches=[&](void const* p,std::size_t n,char const* hash){if(n==62)return std::string(hash)==close_core::close_sha&&std::memcmp(p,close.data(),n)==0;
        return n==160&&std::string(hash)==close_core::exec_sha&&std::memcmp(p,exec.data(),n)==0;};
    auto certify=[&]{return close_core::certify(base,safe_copy,safe_copy,matches);};assert(certify());
    for(unsigned i=0;i<62;++i){memory[base+close_core::close_rva+i]^=1;assert(!certify());memory[base+close_core::close_rva+i]^=1;}
    for(unsigned i=0;i<160;++i){auto at=base+(i<128?close_core::exec_rva+i:close_core::exec_cold_rva+i-128);memory[at]^=1;assert(!certify());memory[at]^=1;}
    put(base+close_core::client_table_rva+close_core::close_slot,base+close_core::close_rva+1);assert(!certify());
    put(base+close_core::client_table_rva+close_core::close_slot,base+close_core::close_rva);
    gears_hook_entry::Proof proof{};proof.target=base+close_core::close_rva;proof.detour=0x180000000;proof.relay=proof.target+0x1000;
    std::copy(close.begin(),close.begin()+32,proof.original32.begin());proof.patched32=proof.original32;proof.patched32[0]=0xe9;
    const std::int32_t displacement=0x1000-5;std::memcpy(proof.patched32.data()+1,&displacement,4);
    proof.relay14[0]=0xff;proof.relay14[1]=0x25;std::memcpy(proof.relay14.data()+6,&proof.detour,8);
    put(proof.target,proof.patched32);put(proof.relay,proof.relay14);
    auto entry=[&](void* out,void const* from,std::size_t n){return close_core::read_own_prefix(proof,proof.target,proof.detour,out,from,n,safe_copy);};
    assert(close_core::certify(base,safe_copy,entry,matches));assert(!certify());
    std::array<unsigned char,32> result{};
    assert(!close_core::read_own_prefix(proof,proof.target,proof.detour+1,result.data(),reinterpret_cast<void*>(proof.target),32,safe_copy));
    memory[proof.target+1]^=1;assert(!close_core::certify(base,safe_copy,entry,matches));memory[proof.target+1]^=1;
    memory[proof.relay+13]^=1;assert(!close_core::certify(base,safe_copy,entry,matches));memory[proof.relay+13]^=1;
    memory[proof.target+61]^=1;assert(!close_core::certify(base,safe_copy,entry,matches));memory[proof.target+61]^=1;
}
static void unknown_after(){
    CloseFixture f;bool after=false;
    auto read=[&](void* out,void const* from,std::size_t size){if(after)throw 919;return safe_copy(out,from,size);};
    auto original_exception=[&](void*,void*)->uintptr_t{++close_calls;after=true;throw 712;};
    try{close_obs::invoke_with(reinterpret_cast<void*>(f.f.c.client),reinterpret_cast<void*>(f.f.c.scene),1,original_exception,read,[]{return true;});assert(false);}
    catch(int value){assert(value==712);}
    assert(close_calls==1&&close_obs::rows[0].after.read_exception&&!close_obs::rows[0].normal&&close_obs::depth==0);
    {CloseFixture g;auto native=[&](void*,void*){++close_calls;auto h=g.request.initial;h.num=129;put(g.request.active_header,h);return sentinel;};
        assert(close_obs::invoke_with(reinterpret_cast<void*>(g.f.c.client),reinterpret_cast<void*>(g.f.c.scene),1,native,safe_copy,[]{return true;})==sentinel);
        assert(!(close_obs::rows[0].after.mask&64)&&close_obs::rows[0].normal&&position::state.lease.active);}
    {CloseFixture g;bool after_original=false;unsigned headers{};
        auto native=[&](void*,void*){++close_calls;after_original=true;return sentinel;};
        auto racing=[&](void* out,void const* from,std::size_t size){
            if(after_original&&reinterpret_cast<uintptr_t>(from)==g.request.active_header&&size==16&&++headers==2){auto h=g.request.initial;h.num=1;put(g.request.active_header,h);}
            return safe_copy(out,from,size);};
        assert(close_obs::invoke_with(reinterpret_cast<void*>(g.f.c.client),reinterpret_cast<void*>(g.f.c.scene),1,native,racing,[]{return true;})==sentinel);
        assert(!(close_obs::rows[0].after.mask&64)&&close_obs::rows[0].normal);}
}
static std::string request_text(close_core::Request const& r){
    std::string text="[PathChoiceCloseObserver]\nschema=1\n";
    auto number=[&](char const* key,uint64_t value){text+=key;text+='=';text+=std::to_string(value);text+='\n';};
    auto address=[&](char const* key,uintptr_t value){char buffer[32]{};sprintf_s(buffer,"0x%llx",static_cast<unsigned long long>(value));text+=key;text+='=';text+=buffer;text+='\n';};
    number("nonce",r.nonce);number("pid",r.pid);number("creation_time",r.creation_time);number("started_qpc",r.started_qpc);number("world_generation",r.world_generation);
    address("base",r.base);address("client",r.client);address("scene",r.scene);address("panel",r.panel);number("duration_ms",r.duration_ms);
    address("active_header",r.active_header);address("active_data",r.initial.data);number("active_num",unsigned(r.initial.num));number("active_max",unsigned(r.initial.max));
    address("function",r.function);number("function_index",r.function_index);address("function_outer",r.function_outer);address("function_class",r.function_class);
    number("function_name_index",r.function_name_index);number("function_number",r.function_number);
    for(unsigned i=0;i<r.members.size();++i){auto key="member_"+std::to_string(i);address(key.c_str(),r.members[i]);}return text;
}
static void request_parser(){
    CloseFixture f;close_core::Request r;auto text=request_text(f.request);assert(close_core::parse(text,r)&&close_core::same(r,f.f.c)&&r.members==f.request.members);
    for(auto suffix:{"schema=1\n","unknown=1\n","member_2=0x8002\n"})assert(!close_core::parse(text+suffix,r));
    for(auto replacement:std::array<std::pair<std::string,std::string>,5>{{{"duration_ms=8000","duration_ms=8001"},{"active_num=2","active_num=129"},
        {"function_number=0","function_number=1"},{"member_1=0x8001","member_1=0x2000"},{"active_header=0x1100","active_header=0x1"}}}){
        auto bad=text;auto at=bad.find(replacement.first);assert(at!=std::string::npos);bad.replace(at,replacement.first.size(),replacement.second);assert(!close_core::parse(bad,r));}
}
static void worker_lifecycle(){
    CloseFixture f;close_obs::last_poll=0;close_obs::seen_file={};close_obs::loaded_nonce=0;close_obs::load_rejected=0;
    wchar_t directory[MAX_PATH]{},path[MAX_PATH]{};assert(GetTempPathW(MAX_PATH,directory));assert(GetTempFileNameW(directory,L"pco",0,path));
    FILE* file{};assert(!_wfopen_s(&file,path,L"wb")&&file);auto text=request_text(f.request);assert(fwrite(text.data(),1,text.size(),file)==text.size());fclose(file);
    FILE* log{};assert(!tmpfile_s(&log)&&log);
    // Wrong bytes/absent own-prefix proof reject the request, on a foreign worker.
    // No lease State inspection or mutation is permitted by poll.
    const auto lease_nonce=position::state.lease.config.nonce;
    std::thread worker([&]{close_obs::poll(path,log);});worker.join();
    assert(close_obs::load_rejected==1&&!std::atomic_load(&close_obs::selected)&&position::state.lease.config.nonce==lease_nonce);
    auto request=std::make_shared<close_obs::Active>();request->request=f.request;request->expires=GetTickCount64()+1000;
    std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(request));
    assert(DeleteFileW(path));close_obs::last_poll=0;close_obs::poll(path,log);assert(!std::atomic_load(&close_obs::selected));fclose(log);
}
#if defined(GEARS_PATH_CHOICE_CLOSE_RESTORE_TEST)
#define CLOSE_OBSERVER_TEST_ENTRY close_observer_fixture_tests
#else
#define CLOSE_OBSERVER_TEST_ENTRY main
#endif
int CLOSE_OBSERVER_TEST_ENTRY(){normal_close();close_gates();bounds_exceptions();certification();unknown_after();request_parser();worker_lifecycle();return 0;}
