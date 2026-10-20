// Private serialized-root provenance, including pre-session prime calls.
namespace ngx_root_signature_capture {
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
using CreateFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,void const*,SIZE_T,REFIID,void**);
CreateFn original{};
std::mutex mutex;gears_ngx_root::Bank bank;
struct Owners {
    gears_vr::ComPtr<ID3D12RootSignature> root;gears_vr::ComPtr<IUnknown> identity,device_identity;
    gears_vr::ComPtr<ID3D12Device> device;
};
std::array<Owners,gears_ngx_root::root_limit> owners;
struct Artifact {bool attempted{},saved{},hash_ok{},fixture_skipped{};DWORD error{};LONG hash_status{};std::array<unsigned char,32> sha{};wchar_t path[MAX_PATH]{};};
std::array<Artifact,gears_ngx_root::root_limit> artifacts;
std::array<uintptr_t,gears_ngx_root::root_limit> callers;
std::array<unsigned char,gears_ngx_root::blob_limit> scratch;
bool accepting{},inflight{},released{};
uint64_t calls{},captured{},duplicates{},conflicts{},api_failures{},read_failures{},changed{},output_failures{},capacity{},busy{},unwound{};
thread_local unsigned fixture_depth{};
std::atomic<DWORD> worker_thread{},watchdog_thread{};
void register_worker(bool watchdog=false){DWORD empty{};(watchdog?watchdog_thread:worker_thread).compare_exchange_strong(empty,GetCurrentThreadId());}
void arm(){std::lock_guard<std::mutex> lock(mutex);if(!released)accepting=true;}
void close(){std::lock_guard<std::mutex> lock(mutex);accepting=false;}
struct Call {
    bool admitted{},returned{};
    explicit Call(size_t size){std::lock_guard<std::mutex> lock(mutex);
        if(!accepting||!(prime_recording.load()||recording.load()))return;++calls;
        if(inflight){++busy;bank.uncertain();return;}
        if(!bank.room(size)){++capacity;bank.uncertain();return;}
        admitted=true;inflight=true;
    }
    ~Call(){if(admitted){std::lock_guard<std::mutex> lock(mutex);if(!returned){++unwound;bank.uncertain();}inflight=false;}}
};
void release_drained();
struct DrainOnExit {~DrainOnExit(){release_drained();}};
HRESULT STDMETHODCALLTYPE create(ID3D12Device* device,UINT node,void const* bytes,SIZE_T size,REFIID iid,void** output){
    DrainOnExit drain;NativeOwnedShutdownRhiScope::Activity activity;Owners candidate;Call call(size);
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    const bool before=call.admitted&&safe_copy(scratch.data(),bytes,size);
    if(call.admitted&&!before){std::lock_guard<std::mutex> lock(mutex);++read_failures;bank.uncertain();}
    const HRESULT hr=original(device,node,bytes,size,iid,output);call.returned=true;
    if(!call.admitted)return hr;
    if(FAILED(hr)){std::lock_guard<std::mutex> lock(mutex);++api_failures;bank.uncertain();return hr;}
    if(!before)return hr;
    std::array<unsigned char,4096> after{};
    for(size_t offset=0;offset<size;offset+=after.size()){
        const auto count=(std::min)(after.size(),size-offset);
        if(!safe_copy(after.data(),static_cast<unsigned char const*>(bytes)+offset,count)){
            std::lock_guard<std::mutex> lock(mutex);++read_failures;bank.uncertain();return hr;}
        if(!gears_ngx_root::unchanged(scratch.data()+offset,after.data(),count)){
            std::lock_guard<std::mutex> lock(mutex);++changed;bank.uncertain();return hr;}
    }
    try{
        IUnknown* actual{};gears_vr::ComPtr<IUnknown> input_device;
        if(!output||!safe_copy(&actual,output,sizeof(actual))||!actual||
           FAILED(actual->QueryInterface(IID_PPV_ARGS(&candidate.root)))||FAILED(candidate.root.As(&candidate.identity))||
           FAILED(candidate.root->GetDevice(IID_PPV_ARGS(&candidate.device)))||FAILED(candidate.device.As(&candidate.device_identity))||
           FAILED(device->QueryInterface(IID_PPV_ARGS(&input_device)))||!candidate.identity||!candidate.device_identity||
           candidate.device_identity.Get()!=input_device.Get()){
            std::lock_guard<std::mutex> lock(mutex);++output_failures;bank.uncertain();return hr;}
        std::lock_guard<std::mutex> lock(mutex);const auto index=bank.count();
        const auto result=bank.append(reinterpret_cast<uintptr_t>(candidate.identity.Get()),reinterpret_cast<uintptr_t>(candidate.device_identity.Get()),node,scratch.data(),size,fixture_depth!=0||self_testing);
        if(result==gears_ngx_root::Result::added){owners[index]=std::move(candidate);callers[index]=caller;++captured;}
        else if(result==gears_ngx_root::Result::duplicate)++duplicates;
        else {if(result==gears_ngx_root::Result::conflict)++conflicts;else ++output_failures;bank.uncertain();}
    }catch(...){std::lock_guard<std::mutex> lock(mutex);++output_failures;bank.uncertain();}
    return hr;
}
struct Lookup {bool found{},retained{},incomplete{},fixture{};uintptr_t canonical{},device{};unsigned index{},node{};size_t size{};};
Lookup lookup(uintptr_t canonical,uintptr_t device){
    std::lock_guard<std::mutex> lock(mutex);Lookup out;out.incomplete=bank.incomplete();
    for(size_t i=0;i<bank.count();++i){auto const& row=bank.row(i);if(row.canonical!=canonical||row.device!=device)continue;
        out={true,bool(owners[i].root),bank.incomplete(),row.fixture,row.canonical,row.device,unsigned(i),row.node_mask,row.size};break;}return out;
}
void release_drained(){
    {std::lock_guard<std::mutex> lock(mutex);if(accepting||inflight||released||active.load())return;}
    NativeOwnedShutdownRhiScope::Activity activity;
    std::array<Owners,gears_ngx_root::root_limit> retired;
    {std::lock_guard<std::mutex> lock(mutex);if(accepting||inflight||released||active.load()!=1)return;owners.swap(retired);released=true;}
    // The independent Activity in create covers prime, passthrough, all QI and
    // temporary Release work. Final retained owners release outside this lock.
}
void poll(){
    const auto thread=GetCurrentThreadId();if(thread!=worker_thread.load()&&thread!=watchdog_thread.load())return;
    NativeOwnedShutdownRhiScope::Activity activity;
    for(size_t i=0;i<gears_ngx_root::root_limit;++i){gears_ngx_root::Record row;unsigned char const* bytes{};
        {std::lock_guard<std::mutex> lock(mutex);if(i>=bank.count())break;if(artifacts[i].attempted)continue;
            artifacts[i].attempted=true;row=bank.row(i);if(row.fixture){artifacts[i].fixture_skipped=true;continue;}
            // Append-only storage has process lifetime; each published byte
            // range is immutable, including after owner drain. No COM escapes.
            bytes=bank.data(row);}
        Artifact result;result.attempted=true;BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
        result.hash_status=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0);
        if(result.hash_status>=0)result.hash_status=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0);
        if(result.hash_status>=0)result.hash_status=BCryptHashData(hash,const_cast<PUCHAR>(bytes),ULONG(row.size),0);
        if(result.hash_status>=0)result.hash_status=BCryptFinishHash(hash,result.sha.data(),ULONG(result.sha.size()),0);
        result.hash_ok=result.hash_status>=0;if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
        wchar_t parent[MAX_PATH]{};const auto length=GetModuleFileNameW(own_module,parent,MAX_PATH);
        auto* slash=length&&length<MAX_PATH?wcsrchr(parent,L'\\'):nullptr;
        if(!slash)result.error=ERROR_PATH_NOT_FOUND;
        else {*slash=0;
            if(_snwprintf_s(result.path,MAX_PATH,_TRUNCATE,L"%s\\ngx-root-signature-%lu-%zu.bin",parent,GetCurrentProcessId(),i)<0)result.error=ERROR_BUFFER_OVERFLOW;
            else {HANDLE file=CreateFileW(result.path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
                if(file==INVALID_HANDLE_VALUE)result.error=GetLastError();
                else {DWORD written{};if(!WriteFile(file,bytes,DWORD(row.size),&written,nullptr))result.error=GetLastError();
                    else if(written!=row.size)result.error=ERROR_WRITE_FAULT;
                    else if(!FlushFileBuffers(file))result.error=GetLastError();else result.saved=true;
                    if(!CloseHandle(file)){result.saved=false;if(!result.error)result.error=GetLastError();}}
            }}
        std::lock_guard<std::mutex> lock(mutex);artifacts[i]=result;
    }
}
void print(FILE* file){
    if(!file)return;std::lock_guard<std::mutex> lock(mutex);
    fprintf(file,"ngx_root_provenance enabled=1 roots=%zu limit=16 bytes=%zu byte_limit=262144 per_root_limit=65536 accepting=%u inflight=%u released=%u incomplete=%u calls=%llu captured=%llu duplicates=%llu conflicts=%llu api_failures=%llu read_failures=%llu changed=%llu output_failures=%llu capacity=%llu busy=%llu unwound=%llu serialized_bytes_only=1 deserialized=0 shader_identity_verified=0 gpu_execution_verified=0\n",
        bank.count(),bank.bytes(),unsigned(accepting),unsigned(inflight),unsigned(released),unsigned(bank.incomplete()),calls,captured,duplicates,conflicts,api_failures,read_failures,changed,output_failures,capacity,busy,unwound);
    for(size_t i=0;i<bank.count();++i){auto const& row=bank.row(i);auto const& a=artifacts[i];
        fprintf(file,"ngx_root_signature row=%zu canonical=%llx device=%llx node=%u size=%zu caller=%llx held=%u fixture=%u artifact_attempted=%u fixture_skipped=%u hash_ok=%u hash_status=%08lx saved=%u error=%lu sha256=",
            i,row.canonical,row.device,row.node_mask,row.size,callers[i],unsigned(bool(owners[i].root)),unsigned(row.fixture),unsigned(a.attempted),unsigned(a.fixture_skipped),unsigned(a.hash_ok),static_cast<unsigned long>(a.hash_status),unsigned(a.saved),a.error);
        for(auto byte:a.sha)fprintf(file,"%02x",unsigned(byte));fprintf(file," path=%ls current_binding_verified=0 shader_identity_verified=0\n",a.path);}
}
DWORD native_check(ID3D12Device* device){
    struct Fixture {Fixture(){++fixture_depth;}~Fixture(){--fixture_depth;}} fixture;
    gears_vr::ComPtr<ID3DBlob> bytes,error;gears_vr::ComPtr<ID3D12RootSignature> root;gears_vr::ComPtr<IUnknown> id,device_id;
    D3D12_ROOT_SIGNATURE_DESC desc{};desc.Flags=D3D12_ROOT_SIGNATURE_FLAG_NONE;
    if(FAILED(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error)))return 356;
    if(FAILED(device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)))||
       FAILED(root.As(&id))||FAILED(device->QueryInterface(IID_PPV_ARGS(&device_id))))return 357;
    const auto found=lookup(reinterpret_cast<uintptr_t>(id.Get()),reinterpret_cast<uintptr_t>(device_id.Get()));
    return found.found&&found.retained&&found.fixture&&found.size==bytes->GetBufferSize()?0:358;
}
DWORD native_closed_check(){
    std::lock_guard<std::mutex> lock(mutex);if(accepting||inflight||!released)return 359;
    for(auto const& owner:owners)if(owner.root||owner.identity||owner.device||owner.device_identity)return 360;return 0;
}
#else
void arm(){}void close(){}void release_drained(){}void poll(){}void print(FILE*){}void register_worker(bool=false){}
#endif
}
