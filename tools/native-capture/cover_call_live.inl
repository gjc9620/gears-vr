// Included in the existing VM dispatch owner. Passive diagnostics only; no
// second hook, native function call, Result-buffer read or gameplay mutation.
namespace cover_call_live {
#if defined(GEARS_COVER_CALL_OBSERVER) && GEARS_COVER_CALL_OBSERVER
namespace trace=gears_cover_call_trace;
namespace wire=gears_cover_call_wire;
bool enabled{};
wire::Packet config;
constexpr unsigned invocation_limit=32,row_limit=24,total_row_limit=512;
struct Row {
    uint64_t invocation{},parent{},world{};
    unsigned role{},caller_role{},thread{};
    uintptr_t receiver{},caller{},callee{},result_address{},opaque_rax{};
    long long begin{},end{};
    trace::Result before{},after{};
    bool returned{},same_storage{};
};
struct Invocation {
    uint64_t id{},parent{},world{};
    unsigned role{},thread{},depth{},count{};
    uintptr_t frame{};
    bool poisoned{};
    std::array<Row,row_limit> rows{};
};
thread_local Invocation* current{};
std::mutex rows_mutex;
std::array<Row,total_row_limit> rows{};
unsigned row_count{};
std::atomic<uint64_t> next_invocation{},candidate_calls{},began{},sealed{},rejected{},dropped{};

// The packet consists of source/identity reads collected in this process.
// Re-reading them is a finite consistency bracket, not an atomic snapshot.
// Only our exact dispatch entry may use the established owned-hook attestation.
bool sources_current(){
    if(config.pid!=GetCurrentProcessId()||config.base!=base||config.functions.size()!=6||
       config.mutation_authorized||config.checks.empty())return false;
    std::array<unsigned char,256> buffer{};
    for(auto const& check:config.checks){
        const bool dispatch=check.address==base+gears_ui_call_boundary::rva&&
            check.bytes.size()==gears_ui_call_boundary::entry_bytes.size();
        for(size_t offset=0;offset<check.bytes.size();){
            const size_t count=(std::min)(buffer.size(),check.bytes.size()-offset);
            const auto* address=reinterpret_cast<void const*>(check.address+offset);
            if(!(dispatch?read_attested_hook_entry(buffer.data(),address,count):safe_copy(buffer.data(),address,count))||
               !std::equal(buffer.begin(),buffer.begin()+count,check.bytes.begin()+offset))return false;
            offset+=count;
        }
    }
    return true;
}

bool configure(const wchar_t* ini,FILE* status){
    wchar_t path[32768]{};
    const DWORD length=GetPrivateProfileStringW(L"CoverCallObserver",L"path",L"",path,32768,ini);
    if(!length)return true;
    if(enabled||length>=32767)return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER size{};
    bool okay=GetFileSizeEx(file,&size)&&size.QuadPart>0&&size.QuadPart<=2*1024*1024;
    std::vector<unsigned char> bytes;
    if(okay){
        bytes.resize(static_cast<size_t>(size.QuadPart));DWORD count{};
        okay=ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&count,nullptr)&&count==bytes.size();
    }
    if(!CloseHandle(file))okay=false;
    if(!okay)return false;
    auto parsed=wire::parse(bytes.data(),bytes.size());
    if(!parsed){fprintf(status,"cover_call_config rejected=%s observation_only=1\n",wire::status_name(parsed.status));return false;}
    config=std::move(parsed.packet);
    if(config.controller!=expected_pc||config.camera!=expected_camera||!sources_current())return false;
    enabled=true;
    fprintf(status,"cover_call_config accepted=1 observation_only=1 mutation_authorized=0 functions=%zu checks=%zu\n",
        config.functions.size(),config.checks.size());
    return true;
}

struct CallScope {
    Invocation* saved{};
    std::unique_ptr<Invocation> owned;
    Row* row{};
    bool root{},returned_called{};
    CallScope(void* self,void* caller,void* result,void* callee){
        if(!enabled||!recording.load()||reinterpret_cast<uintptr_t>(self)!=config.controller)return;
        unsigned role=6;
        for(unsigned i=0;i<6;++i)if(config.functions[i].address==reinterpret_cast<uintptr_t>(callee)){role=i;break;}
        if(role==6)return;
        ++candidate_calls;
        if(role==0||role==2){
            if(began.load()>=invocation_limit)return;
            if((current&&current->depth>=4)||!sources_current()){++rejected;return;}
            owned=std::make_unique<Invocation>();saved=current;root=true;
            auto& value=*owned;value.id=++next_invocation;value.parent=saved?saved->id:0;
            value.world=committed_world_generation.load();value.thread=GetCurrentThreadId();
            value.role=role;value.depth=saved?saved->depth+1:1;current=&value;++began;
            return;
        }
        if(!current||current->poisoned||current->thread!=GetCurrentThreadId()||
           current->world!=committed_world_generation.load())return;
        if(current->count>=row_limit){current->poisoned=true;++dropped;return;}
        auto& value=current->rows[current->count++];row=&value;
        value.invocation=current->id;value.parent=current->parent;value.world=current->world;
        value.role=role;value.thread=GetCurrentThreadId();value.receiver=reinterpret_cast<uintptr_t>(self);
        value.caller=reinterpret_cast<uintptr_t>(caller);value.callee=reinterpret_cast<uintptr_t>(callee);
        value.result_address=reinterpret_cast<uintptr_t>(result);value.begin=now();
        gears_ui_call_boundary::Frame frame{};
        if(!gears_ui_call_boundary::frame(safe_copy,value.caller,frame)){current->poisoned=true;return;}
        unsigned caller_role=6;
        for(unsigned i:{0u,2u})if(frame.node==config.functions[i].address){caller_role=i;break;}
        if(caller_role==6||(current->frame&&current->frame!=value.caller)){
            current->poisoned=true;return;}
        value.caller_role=caller_role;current->frame=value.caller;
        value.before=trace::capture(safe_copy,value.caller,config.controller,config.functions[caller_role]);
        if(!value.before)current->poisoned=true;
    }
    void returned(uintptr_t bits){
        returned_called=true;
        if(row){
            row->opaque_rax=bits;row->returned=true;row->end=now();
            if(row->before&&current&&!current->poisoned){
                row->after=trace::capture(safe_copy,row->caller,config.controller,config.functions[row->caller_role]);
                if(row->after){
                    auto const& a=row->before.snapshot;auto const& b=row->after.snapshot;
                    row->same_storage=a.frame.node==b.frame.node&&a.frame.object==b.frame.object&&
                        a.frame.previous==b.frame.previous&&a.locals==b.locals;
                }
                if(!row->after||!row->same_storage)current->poisoned=true;
            }
        }
        if(root){
            auto& value=*owned;
            const bool valid=current==&value&&!value.poisoned&&recording.load()&&
                value.thread==GetCurrentThreadId()&&value.world==committed_world_generation.load()&&sources_current();
            if(valid){
                ++sealed;std::lock_guard<std::mutex> lock(rows_mutex);
                for(unsigned i=0;i<value.count;++i){if(row_count<rows.size())rows[row_count++]=value.rows[i];else ++dropped;}
            }else ++rejected;
            current=saved;
        }
    }
    ~CallScope(){
        if(root&&!returned_called){current=saved;++rejected;}
    }
};
void print(FILE* file,bool disabled){
    fprintf(file,"cover_calls compiled=1 enabled=%u observation_only=1 mutation_authorized=0 candidates=%llu began=%llu sealed=%llu rejected=%llu dropped=%llu\n",
        enabled?1:0,candidate_calls.load(),began.load(),sealed.load(),rejected.load(),dropped.load());
    if(!enabled||!disabled||active.load())return;
    std::lock_guard<std::mutex> lock(rows_mutex);
    for(unsigned i=0;i<row_count;++i){auto const& r=rows[i];auto const& a=r.before.snapshot;auto const& b=r.after.snapshot;
        fprintf(file,"cover_call_row index=%u invocation=%llu parent=%llu world=%llu role=%u caller_role=%u thread=%u receiver=%p caller=%p callee=%p result_address=%p opaque_rax=%p begin=%lld end=%lld returned=%u before=%s after=%s same_storage=%u code_before=%u code_after=%u ct_before=%u ca_before=%u cd_before=%u ct_after=%u ca_after=%u cd_after=%u locals_diagnostic=%p mutation_authorized=0\n",
            i,r.invocation,r.parent,r.world,r.role,r.caller_role,r.thread,reinterpret_cast<void*>(r.receiver),
            reinterpret_cast<void*>(r.caller),reinterpret_cast<void*>(r.callee),reinterpret_cast<void*>(r.result_address),
            reinterpret_cast<void*>(r.opaque_rax),r.begin,r.end,r.returned?1:0,trace::status_name(r.before.status),trace::status_name(r.after.status),
            r.same_storage?1:0,a.code_offset,b.code_offset,a.values.pawn_ct,a.values.pawn_ca,a.values.pawn_cd,
            b.values.pawn_ct,b.values.pawn_ca,b.values.pawn_cd,reinterpret_cast<void*>(a.locals));
        auto dump=[&](const char* endpoint,trace::Snapshot const& s){
            fprintf(file,"cover_call_bytes index=%u endpoint=%s code_window_offset=%u script=",i,endpoint,s.script_window_offset);
            for(unsigned n=0;n<s.script_window_size;++n)fprintf(file,"%02x",s.script_window[n]);
            fputs(" locals_prefix=",file);for(unsigned n=0;n<s.raw_locals_size;++n)fprintf(file,"%02x",s.raw_locals[n]);fputc('\n',file);
        };
        dump("entry",a);dump("return",b);
    }
}
#else
inline constexpr bool enabled=false;
bool configure(const wchar_t* ini,FILE*){
    wchar_t path[2]{};return GetPrivateProfileStringW(L"CoverCallObserver",L"path",L"",path,2,ini)==0;
}
struct CallScope {CallScope(void*,void*,void*,void*){}void returned(uintptr_t){}};
void print(FILE* file,bool){fprintf(file,"cover_calls compiled=0 observation_only=1 mutation_authorized=0\n");}
#endif
}
