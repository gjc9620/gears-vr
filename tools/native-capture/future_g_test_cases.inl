// CPU HOST SEAM: fake safe_copy/identity/digest/entry certificate and original.
// Production observer/protected-finally code is included, but these admissions
// do not establish runtime identity, getter semantics, or a writable interval.
namespace post=live::post_weight_scale;namespace fg=live::future_g;
unsigned original_calls{},g_original_calls{};char const* scenario="normal";
p::CallPacket last_return{},last_g_input{};
std::mutex stop_mutex;std::condition_variable stop_cv;bool g_entered{},let_return{};
constexpr DWORD original_seh=0xe042c248;struct OriginalCpp{unsigned marker;};
bool mode(char const* value){return !strcmp(scenario,value);}
extern "C" void cover_body_live::GearsCoverRawInvoke(CallPacket* packet,void* original) noexcept(false){
    ++original_calls;const auto kind=unsigned(reinterpret_cast<uintptr_t>(original)-1);
    C(active.load()>0);C(kind==3?fg::in_flight.load()>0:live::in_flight.load()>0);
    if(kind==3){++g_original_calls;last_g_input=*packet;
        if(mode("copy-end")){auto* real=fg::local.call;auto copy=*real;fg::release(copy);C(real->counter_owned&&fg::in_flight==1&&active==1&&fg::local.call==real);C(fg::protocol_debt==1);}
        if(mode("seh"))RaiseException(original_seh,0,0,nullptr);
        if(mode("cpp"))throw OriginalCpp{0x248ac};
        if(mode("stop")){std::unique_lock<std::mutex> lock(stop_mutex);g_entered=true;stop_cv.notify_all();
            C(stop_cv.wait_for(lock,std::chrono::seconds(3),[]{return let_return;}));}
        if(mode("metadata")){uintptr_t changed=asset+8;put(mesh+0x218,changed);}
        if(mode("entry"))entry_ok=false;
        if(mode("caller"))code[6].bytes[1]^=1;
        if(mode("during")){const float changed=3;put(node+0xac,changed);}
        if(mode("unknown-end"))fg::local.call=nullptr;
    }else if(kind<2){const unsigned n=kind?1:3;for(unsigned i=0;i<n;++i){auto& m=output_matrices[i];m={};
            m.values[0]=m.values[5]=m.values[10]=m.values[15]=1;m.values[12]=float(10+i);}
        const p::ArrayHeader h{reinterpret_cast<uintptr_t>(output_matrices.data()),int32_t(n),int32_t(n)};
        memcpy(reinterpret_cast<void*>(packet->gpr[4]),&h,sizeof h);
    }
    for(unsigned i=0;i<7;++i)packet->gpr[i]=0x1122334455667788ull+i;
    for(unsigned i=0;i<6;++i)packet->xmm[i].fill(static_cast<unsigned char>(0xa0+i));
    uint32_t bits=0x3f800000u;if(kind==3){memcpy(&bits,memory.data()+node-game+0xac,4);if(mode("nonunit"))bits=0x40000000u;else if(mode("nan"))bits=0x7fc12345u;}
    memcpy(packet->xmm[0].data(),&bits,4);packet->flags=0x2d7;last_return=*packet;
}
DWORD seh_invoke(p::CallPacket* packet,uintptr_t ra,uintptr_t rsp){
    __try{fg::GearsCoverFutureGInvokeProtected(packet,0,ra,rsp);}
    __except(GetExceptionCode()==original_seh?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return GetExceptionCode();}
    return 0;
}
void pair(unsigned kind,int end){
    game_state(kind);const float g=1;put(node+0xac,g);
    affected=end==11?std::array<int32_t,3>{{8,10,11}}:std::array<int32_t,3>{{32,34,35}};if(kind)affected[0]=end;
    alignas(16) unsigned char stack[0x120]{};const auto rsp=reinterpret_cast<uintptr_t>(stack)+8;
    p::ArrayHeader out{},scale_header{},indices{reinterpret_cast<uintptr_t>(affected.data()),int32_t(kind?1:3),int32_t(kind?1:3)};
    memcpy(reinterpret_cast<void*>(rsp+0x70),&scale_header,16);memcpy(reinterpret_cast<void*>(rsp+0x80),&out,16);memcpy(reinterpret_cast<void*>(rsp+0x90),&indices,16);
    auto ra=base+0x1290e5;memcpy(reinterpret_cast<void*>(rsp),&ra,8);
    p::CallPacket packet{};packet.gpr[1]=node;packet.gpr[2]=uint32_t(end);packet.gpr[3]=mesh;packet.gpr[4]=rsp+0x80;
    live::GearsCoverControlInvokeProtected(&packet,kind,ra,rsp);C(!memcmp(&packet,&last_return,sizeof packet));
    ra=base+0x12911f;memcpy(reinterpret_cast<void*>(rsp),&ra,8);packet={};packet.gpr[1]=node;
    packet.gpr[2]=0xdead;packet.gpr[3]=0xbeef;packet.gpr[4]=rsp+0x70;
    live::GearsCoverControlInvokeProtected(&packet,2,ra,rsp);C(!memcmp(&packet,&last_return,sizeof packet));
    C(post::records[post::count-1].reject==post::Reject::okay);
    C(fg::count==post::count&&fg::awaiting_close==post::count);
    const auto original_memory=memory;const auto original_matrices=output_matrices;
    if(mode("since")){const float changed=2;put(node+0xac,changed);}
    ra=base+0x129474;if(mode("wrong-ra"))++ra;memcpy(reinterpret_cast<void*>(rsp),&ra,8);
    packet={};for(unsigned i=0;i<7;++i)packet.gpr[i]=0xabc00000ull+i;
    for(unsigned i=0;i<6;++i)packet.xmm[i].fill(static_cast<unsigned char>(0xc0+i));packet.flags=0xa97;
    packet.gpr[1]=node+(mode("wrong-node")?8:0);packet.gpr[2]=uint32_t(end+(mode("wrong-bone")?1:0));packet.gpr[3]=mesh+(mode("wrong-mesh")?8:0);
    const auto input=packet;if(mode("wrong-ordinal"))++live::thread.frames[0].ordinal;
    if(mode("wrong-thread"))++fg::records[fg::count-1].post.scope_thread;
    if(mode("wrong-compose"))++fg::records[fg::count-1].post.scope_compose;
    std::thread stop_worker;
    if(mode("stop"))stop_worker=std::thread([]{
        {std::unique_lock<std::mutex> lock(stop_mutex);if(!stop_cv.wait_for(lock,std::chrono::seconds(3),[]{return g_entered;})){C(false);return;}}
        C(fg::pending_count()>=2&&active.load()>0);const bool unlocked=fg::record_mutex.try_lock();C(unlocked);if(unlocked)fg::record_mutex.unlock();
        live::request_drain();C(fg::pending_count()>=2);
        {std::lock_guard<std::mutex> lock(stop_mutex);let_return=true;}stop_cv.notify_all();
    });
    bool caught_cpp{};DWORD caught_seh{};
    fg_window=true;throw_begin_read=mode("begin-failed");try{caught_seh=seh_invoke(&packet,ra,rsp);}catch(OriginalCpp const& e){caught_cpp=e.marker==0x248ac;}
    if(stop_worker.joinable())stop_worker.join();
    C(caught_cpp==mode("cpp"));C(caught_seh==(mode("seh")?original_seh:0));
    C(!memcmp(&input,&last_g_input,sizeof input));
    if(!mode("seh")&&!mode("cpp"))C(!memcmp(&packet,&last_return,sizeof packet));
    else C(!memcmp(&packet,&input,sizeof packet));
    if(mode("duplicate")){packet=input;fg::GearsCoverFutureGInvokeProtected(&packet,0,ra,rsp);C(!memcmp(&packet,&last_return,sizeof packet));}
    fg_window=false;C(!fg_parameter_reads); // No additional observer read of +70..F7, including +AC.
    C(!fg::in_flight.load()&&!active.load());
    C(!memcmp(output_matrices.data(),original_matrices.data(),sizeof output_matrices));
    if(!mode("since")&&!mode("during")&&!mode("metadata"))C(memory==original_memory);
    C(!memcmp(reinterpret_cast<void*>(rsp+0x70),&scale_header,16));
}
int main(int argc,char** argv){C(argc==3);if(argc!=3)return 1;scenario=argv[1];
    static_assert(sizeof(live::Row)==1944&&sizeof(live::Header)==72);
    static_assert(sizeof(post::Record)==6224&&sizeof(post::Header)==88);
    if(mode("counter-ownership")){
        // Exact production acquire/release, with a real Call alive throughout.
        fg::Call real{};fg::acquire(real);real.linked=true;real.serial=1;real.thread=GetCurrentThreadId();fg::local.call=&real;fg::local.depth=1;
        auto copy=real;fg::release(copy);C(fg::in_flight==1&&active==1&&real.counter_owned&&fg::local.call==&real&&fg::local.depth==1&&fg::protocol_debt==1);
        fg::release(real);C(!fg::in_flight&&!active&&!real.counter_owned&&!fg::local.call&&!fg::local.depth&&fg::protocol_debt==1);
        fg::release(real);C(!fg::in_flight&&!active&&fg::protocol_debt==2&&fg::pending_count()==2);
        fg::Call begin_failed{};fg::acquire(begin_failed);fg::release(begin_failed);C(!fg::in_flight&&!active&&fg::protocol_debt==3);
        fg::protocol_debt=UINT_MAX-1;fg::in_flight=2;live::compose_in_flight=1;C(fg::pending_count()==UINT_MAX&&live::pending_count()==UINT_MAX);
        fg::in_flight=0;live::compose_in_flight=0;fg::note_protocol();fg::note_protocol();C(fg::protocol_debt==UINT_MAX&&fg::pending_count()==UINT_MAX&&live::pending_count()==UINT_MAX);
        printf("FutureG copied/real/duplicate end: %u checks %u failures; counts_never_underflow=1 pending_saturates=1\n",checks,failures);return failures?1:0;
    }
    if(mode("layout")){printf("{\"header\":%zu,\"row\":%zu,\"post\":%zu,\"input\":%zu,\"output\":%zu,\"before\":%zu,\"after\":%zu,\"serial\":%zu,\"caller\":%zu,\"entry_rsp\":%zu,\"begin_qpc\":%zu,\"end_qpc\":%zu,\"post_index\":%zu,\"g_bits\":%zu,\"reject\":%zu,\"entered\":%zu,\"end_known\":%zu}\n",
        sizeof(fg::Header),sizeof(fg::Record),offsetof(fg::Record,post),offsetof(fg::Record,input),offsetof(fg::Record,output),offsetof(fg::Record,before),offsetof(fg::Record,after),offsetof(fg::Record,serial),offsetof(fg::Record,caller),offsetof(fg::Record,entry_rsp),offsetof(fg::Record,begin_qpc),offsetof(fg::Record,end_qpc),offsetof(fg::Record,post_index),offsetof(fg::Record,g_bits),offsetof(fg::Record,reject),offsetof(fg::Record,entered),offsetof(fg::Record,end_known));return 0;}
    C(swprintf_s(directory,L"%S\\future-%lu-%llu",argv[2],GetCurrentProcessId(),GetTickCount64())>0);C(CreateDirectoryW(directory,nullptr)!=FALSE);
    for(unsigned i=0;i<3;++i){code[i].address=base+gears_cover_control_certificate::hooks[i].rva;code[i].bytes=gears_cover_control_certificate::hooks[i].prefix;code[i].bytes[0]=0xe9;
        live::targets[i]=reinterpret_cast<void*>(code[i].address);live::originals[i]=reinterpret_cast<void*>(uintptr_t(i+1));}
    for(unsigned i=3;i<7;++i){code[i].address=base+0x700000+(i-3)*0x100;code[i].bytes.fill(0xcc);}
    code[4].address=base+0xc3f20;code[4].bytes[0]=0xc2;code[4].bytes[1]=code[4].bytes[2]=0;
    code[5].address=base+gears_cover_future_g_certificate::getter.rva;std::copy(gears_cover_future_g_certificate::prefix.begin(),gears_cover_future_g_certificate::prefix.end(),code[5].bytes.begin());
    code[6].address=base+gears_cover_future_g_certificate::call_rva;const unsigned char synthetic_call[]={0xff,0x90,0x48,0x02,0,0};memcpy(code[6].bytes.data(),synthetic_call,6);
    wchar_t ini[MAX_PATH]{};C(swprintf_s(ini,L"%s\\observer.ini",directory)>0);
    C(live::configure(ini,nullptr)&&!live::enabled&&!post::enabled&&!fg::enabled);
    C(WritePrivateProfileStringW(L"CoverFutureGObserver",L"enabled",L"1",ini)!=FALSE);C(!live::configure(ini,nullptr)&&!fg::enabled);
    C(WritePrivateProfileStringW(L"CoverControlObserver",L"enabled",L"1",ini)!=FALSE);
    C(WritePrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"1",ini)!=FALSE);C(live::configure(ini,nullptr)&&fg::enabled);
    phase::digest_ok=false;C(!fg::prepare()&&!fg::certified);phase::digest_ok=true;C(fg::prepare());C(fg::reserve());C(fg::reserved_target(fg::target));
    C(creates==1);code[5].bytes[0]=0xe9; // Host attestation seam only.
    auto s=scope();live::compose_begin(s);C(live::thread.frames[0].admitted);
    pair(0,11);if(mode("normal")){pair(1,11);pair(0,35);pair(1,35);C(fg::count==4&&g_original_calls==4&&original_calls==12);}
    else C(g_original_calls==(mode("duplicate")?2u:1u));
    if(mode("unknown-compose")){auto alien=s;live::compose_end(alien,true);C(live::pending_count()>0&&fg::awaiting_close==fg::count);live::request_drain();live::print(nullptr,true);C(!fg::file_written&&!live::file_written);}
    else {
        if(mode("wrong-compose")){C(fg::records[0].reject==fg::Reject::waiting);--fg::records[0].post.scope_compose;}
        live::compose_end(s,!mode("seh")&&!mode("cpp"));
        for(unsigned i=0;i<fg::count;++i){auto const& r=fg::records[i];C(r.closed);C(r.post_index==i+1);C(r.post.weight.ordinal==2ull*(i+1));
            const auto expected=mode("seh")||mode("cpp")?fg::Reject::compose_failed:mode("unknown-end")||mode("copy-end")?fg::Reject::protocol:
                mode("duplicate")?fg::Reject::duplicate:mode("stop")?fg::Reject::stopped:mode("nan")?fg::Reject::nonfinite:
                mode("metadata")?fg::Reject::identity:mode("entry")||mode("caller")?fg::Reject::entry:
                !strncmp(scenario,"wrong-",6)||mode("begin-failed")?fg::Reject::missing:fg::Reject::observed;
            if(r.reject!=expected)fprintf(stderr,"reject=%u expected=%u\n",unsigned(r.reject),unsigned(expected));C(r.reject==expected);
            if(r.entered){C(r.serial&&r.begin_qpc&&r.end_qpc>=r.begin_qpc);C(r.packet_unchanged);C(r.exception==(mode("seh")||mode("cpp")));
                if(mode("since"))C(r.g_bits==0x40000000&&!r.g_is_one&&r.g_finite);
                if(mode("during"))C(r.g_bits==0x40400000&&!r.g_is_one&&r.g_finite);
                if(mode("nonunit"))C(r.g_bits==0x40000000&&!r.g_is_one&&r.g_finite);
                if(mode("nan"))C(r.g_bits==0x7fc12345&&!r.g_finite);
            }
        }
        live::request_drain();if(mode("unknown-end")||mode("copy-end")||mode("begin-failed")){C(fg::protocol_debt>0&&live::pending_count()>0);live::print(nullptr,true);C(!fg::file_written&&!live::file_written);}
        else {C(!fg::pending_count()&&!live::pending_count());live::print(nullptr,true);C(fg::file_written&&post::written&&live::file_written);}
    }
    C(sizeof(fg::Header)>0);printf("FutureG host %s: %u checks %u failures; synthetic_host=1 writer=0 global_invariance=0\n",scenario,checks,failures);return failures?1:0;
}
