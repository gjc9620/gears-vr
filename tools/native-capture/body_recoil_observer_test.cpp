// Actual 24b owner/Rig/World/Core/math fixture; UObject and raw-control memory
// remain explicit host seams. No native control or game is executed.
#define safe_copy continuous_fixture_copy
#include "body_recoil_owner_fixture.inl"
#undef safe_copy
#ifndef GEARS_BODY_RECOIL_OBSERVER
#define GEARS_BODY_RECOIL_OBSERVER 1
#endif
#define GEARS_BODY_RECOIL_OBSERVER_TEST 1
#include "body_recoil_observer_live.inl"
namespace recoil=body_recoil_observer_live;
#if GEARS_BODY_RECOIL_OBSERVER
namespace {
alignas(8) std::array<unsigned char,0x160> tree_memory{},node_memory{};
alignas(8) std::array<uintptr_t,0x290/8> node_table{};
std::array<unsigned char,115> map_memory{},desired_memory{},tag_memory{};
std::array<unsigned char,20> list_memory{};
uintptr_t tree_at=reinterpret_cast<uintptr_t>(tree_memory.data()),node_at=reinterpret_cast<uintptr_t>(node_memory.data());
constexpr uintptr_t tree_class=0x660000,node_class=0x660100;
struct TestRange {uintptr_t at{};size_t size{};};std::array<TestRange,8> extra{};unsigned extra_count{};
template<class T>void add_range(T& a){extra[extra_count++]={reinterpret_cast<uintptr_t>(&a),sizeof a};}
template<class A,class T>void set_bytes(A& a,size_t at,T const& x){memcpy(a.data()+at,&x,sizeof x);}
bool recoil_fixture(char const* path){std::ifstream f(path,std::ios::binary);char magic[8]{};uint32_t count{};f.read(magic,8);f.read(reinterpret_cast<char*>(&count),4);
    if(!f||memcmp(magic,"RCOCERT1",8)||count!=3)return false;
    for(auto const& chunk:gears_body_recoil_certificate::chunks){uint64_t rva{};uint32_t size{};f.read(reinterpret_cast<char*>(&rva),8);f.read(reinterpret_cast<char*>(&size),4);
        if(!f||rva!=chunk.rva||size!=chunk.size)return false;f.read(reinterpret_cast<char*>(image.data()+rva),size);if(!f)return false;}return f.peek()==EOF;}
void setup_recoil(bool active){
    map_memory.fill(255);map_memory[4]=0;for(unsigned i=0;i<115;++i)desired_memory[i]=static_cast<unsigned char>(i);
    memcpy(list_memory.data(),reference[4].bytes,8);set_bytes(list_memory,8,node_at);
    set_bytes(mesh_memory,0x230,tree_at);p::ArrayHeader map{reinterpret_cast<uintptr_t>(map_memory.data()),115,115};set_bytes(mesh_memory,0x2f4,map);
    p::ArrayHeader lists{reinterpret_cast<uintptr_t>(list_memory.data()),1,1},tags{reinterpret_cast<uintptr_t>(tag_memory.data()),115,115};
    set_bytes(tree_memory,0x138,lists);set_bytes(tree_memory,0x118,tags);
    model_metadata.desired={reinterpret_cast<uintptr_t>(desired_memory.data()),115,115};set_bytes(mesh_memory,0x28c,model_metadata.desired);
    const uintptr_t table_at=reinterpret_cast<uintptr_t>(node_table.data());set_bytes(node_memory,0,table_at);
    node_table[0x230/8]=base+0x3dfc44;node_table[0x238/8]=base+0x3a3d2c;node_table[0x240/8]=base+0xc3f20;
    node_table[0x248/8]=base+0x516100;node_table[0x278/8]=base+0x515bb0;node_table[0x280/8]=base+0x3a4700;node_table[0x288/8]=base+0x3a4710;
    const float one=1;set_bytes(node_memory,0x70,one);set_bytes(node_memory,0xac,one);const int32_t lod=1000;set_bytes(node_memory,0xb0,lod);
    node_memory[0x140]=active?4:0;recoil::certified=recoil::certify();recoil::enabled=true;C(recoil::certified);
}
}
namespace body_recoil_observer_live {
unsigned opaque_reads{};
bool read_memory(void* out,uintptr_t at,size_t n){if(at==0xbad00000){++opaque_reads;return false;}if(at==base+certificate::anim_tree_class_slot&&n==8){memcpy(out,&tree_class,8);return true;}
    const TestRange ranges[]={{tree_at,tree_memory.size()},{node_at,node_memory.size()},{reinterpret_cast<uintptr_t>(node_table.data()),sizeof node_table},
        {reinterpret_cast<uintptr_t>(map_memory.data()),map_memory.size()},{reinterpret_cast<uintptr_t>(list_memory.data()),list_memory.size()},
        {reinterpret_cast<uintptr_t>(desired_memory.data()),desired_memory.size()},{reinterpret_cast<uintptr_t>(tag_memory.data()),tag_memory.size()}};
    for(auto const& r:ranges)if(within(at,n,r.at,r.size)){memcpy(out,reinterpret_cast<void const*>(at),n);return true;}
    for(unsigned i=0;i<extra_count;++i)if(within(at,n,extra[i].at,extra[i].size)){memcpy(out,reinterpret_cast<void const*>(at),n);return true;}
    return continuous_fixture_copy(out,reinterpret_cast<void const*>(at),n);
}
bool object(uintptr_t at,gears_anchor::Identity& id){id={};if(at!=tree_at&&at!=node_at)return false;
    id.address=at;id.cls=at==tree_at?tree_class:node_class;id.outer=at==tree_at?fixture_mesh_at:tree_at;id.index=at==tree_at?20u:21u;id.name_index=id.index;return true;}
bool derives(uintptr_t cls,wchar_t const* name,uintptr_t ref){return (cls==tree_class&&!wcscmp(name,L"AnimTree")&&ref==tree_class)||
    (cls==node_class&&ref==0&&(!wcscmp(name,L"SkelControlBase")||!wcscmp(name,L"WarSkelCtrl_Recoil")));}
}
__declspec(noinline) void recoil_original_exception(Entry const* e,recoil::WeightCall* call,p::CallPacket* packet){
    __try{recoil::weight_begin(*e,*call);RaiseException(0xe0422222,0,0,nullptr);}
    __finally{recoil::weight_end(*call,*packet,!AbnormalTermination());}}
void recoil_catch_exception(Entry const* e,recoil::WeightCall* call,p::CallPacket* packet){
    __try{recoil_original_exception(e,call,packet);}__except(EXCEPTION_EXECUTE_HANDLER){}}
int main(int argc,char** argv){
    if(argc==2&&!strcmp(argv[1],"abi")){
        printf("{\"Header\":%zu,\"Record\":%zu,\"Node\":%zu,\"Mapping\":%zu,\"CoreSnapshot\":%zu,\"record\":{\"tick\":%zu,\"compose\":%zu,\"read_mask\":%zu,\"natural_calls\":%zu,\"failure\":%zu,\"returned\":%zu,\"no_natural_call\":%zu,\"qualified\":%zu,\"source\":%zu,\"mapping\":%zu,\"begin\":%zu,\"weight_before\":%zu,\"weight_after\":%zu,\"end\":%zu,\"alpha_bits\":%zu,\"has_scale\":%zu,\"scale_header\":%zu,\"pose_header\":%zu,\"affected_header\":%zu,\"affected\":%zu,\"matrix\":%zu}}\n",
            sizeof(recoil::Header),sizeof(recoil::Record),sizeof(recoil::Node),sizeof(recoil::Mapping),sizeof(core::CoreSnapshot),
            offsetof(recoil::Record,tick),offsetof(recoil::Record,compose),offsetof(recoil::Record,read_mask),offsetof(recoil::Record,natural_calls),offsetof(recoil::Record,failure),
            offsetof(recoil::Record,returned),offsetof(recoil::Record,no_natural_call),offsetof(recoil::Record,qualified),offsetof(recoil::Record,source),offsetof(recoil::Record,mapping),
            offsetof(recoil::Record,begin),offsetof(recoil::Record,weight_before),offsetof(recoil::Record,weight_after),offsetof(recoil::Record,end),offsetof(recoil::Record,alpha_bits),
            offsetof(recoil::Record,has_scale),offsetof(recoil::Record,scale_header),offsetof(recoil::Record,pose_header),offsetof(recoil::Record,affected_header),offsetof(recoil::Record,affected),offsetof(recoil::Record,matrix));return 0;}
    if(argc!=6||!fixtures(argv[2],argv[3])||!load_caller(argv[4]))return 2;start();if(!recoil_fixture(argv[5]))return 2;
    const std::string mode=argv[1];if(mode=="active"){C(recoil::certify());image[0x3a3d2c+300]^=1;C(!recoil::certify());image[0x3a3d2c+300]^=1;}
    if(mode=="freeze-config"){wchar_t folder[MAX_PATH]{},path[MAX_PATH]{};C(GetCurrentDirectoryW(MAX_PATH,folder)>0);C(swprintf_s(path,L"%s\\body-freeze-config-%lu-%llu.ini",folder,GetCurrentProcessId(),GetTickCount64())>0);
        C(recoil::configure(path,nullptr)&&!recoil::enabled&&!recoil::freeze_on_first_natural_output.load());
        C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"freeze_on_first_natural_output",L"1",path)!=FALSE);C(!recoil::configure(path,nullptr)&&!recoil::enabled);
        C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"enabled",L"1",path)!=FALSE);C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"freeze_on_first_natural_output",L"invalid",path)!=FALSE);C(!recoil::configure(path,nullptr)&&!recoil::enabled);
        C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"freeze_on_first_natural_output",L"1",path)!=FALSE);C(recoil::configure(path,nullptr)&&recoil::enabled&&recoil::freeze_on_first_natural_output.load()&&!recoil::ring_frozen.load());
        printf("body_recoil freeze-config: %u checks %u failures\n",checks,failures);return failures?1:0;}
    setup_recoil(mode!="inactive");
    recoil::freeze_on_first_natural_output=(mode=="freeze"||mode=="freeze-opaque"||mode=="freeze-exception");
    if(mode=="mapping")map_memory[4]=255;
    Entry tick=tick_entry(),compose=compose_entry();live::pose_begin(tick);live::pose_begin(compose);recoil::compose_begin(compose);
    C(recoil::frame.active&&recoil::pending_count()==1);const auto hand_before=live::slot.data.record.source;
    alignas(16) std::array<unsigned char,0xc0> stack_memory{};p::Matrix matrix{};for(unsigned i=0;i<4;++i)matrix.values[i*5]=1;
    int32_t affected=4;add_range(stack_memory);add_range(matrix);add_range(affected);
    const uintptr_t rsp=reinterpret_cast<uintptr_t>(stack_memory.data())+8;
    p::ArrayHeader output{reinterpret_cast<uintptr_t>(&matrix),1,1},list{reinterpret_cast<uintptr_t>(&affected),1,1};
    memcpy(reinterpret_cast<void*>(rsp+0x80),&output,16);memcpy(reinterpret_cast<void*>(rsp+0x90),&list,16);
    if((mode=="opaque"||mode=="freeze-opaque")){const p::ArrayHeader opaque{0xbad00000,1,1};const uint32_t has=1;memcpy(reinterpret_cast<void*>(rsp+0x70),&opaque,16);memcpy(reinterpret_cast<void*>(rsp+0x48),&has,4);}
    Entry weight{};weight.kind=2;weight.rcx=node_at;weight.ra=base+0x12911f;weight.rsp=rsp;weight.rdx=0x111;weight.r8=0x222;weight.r9=0x333;
    p::CallPacket packet{};memset(&packet,0x5a,sizeof packet);float alpha=1;memcpy(packet.xmm[0].data(),&alpha,4);const auto expected_packet=packet;
    recoil::WeightCall call{};
    if(mode!="inactive"&&mode!="mapping"){
        if((mode=="exception"||mode=="freeze-exception"))recoil_catch_exception(&weight,&call,&packet);
        else{recoil::weight_begin(weight,call);C(call.selected);
            if(mode=="mismatch"){auto wrong=call;recoil::weight_end(wrong,packet,true);C(recoil::frame.weight==&call&&recoil::frame.weight_depth==1);
                Entry wrong_compose=compose;recoil::compose_end(wrong_compose,true);C(recoil::frame.active);}
            if(mode=="nested-end"){Entry nested_entry=weight;recoil::WeightCall nested{};recoil::weight_begin(nested_entry,nested);
                C(nested.counted&&!nested.selected&&nested.depth==2&&nested.previous==&call&&recoil::frame.top==&nested);
                auto wrong=nested;recoil::weight_end(wrong,packet,true);
                C(recoil::protocol_failed.load()&&recoil::frame.weight_depth==2&&recoil::frame.top==&nested&&recoil::frame.weight==&call);
                recoil::weight_end(nested,packet,true);C(recoil::frame.weight_depth==1&&recoil::frame.top==&call&&recoil::frame.weight==&call);
            }
            if(mode=="metadata")bad_metadata=true;if(mode=="stop")recoil::request_drain();
            recoil::weight_end(call,packet,true);bad_metadata=false;
        }
        C(!memcmp(&packet,&expected_packet,sizeof packet));C(recoil::frame.weight_depth==0&&!recoil::frame.weight&&!recoil::frame.top);
    }
    C(!memcmp(&hand_before,&live::slot.data.record.source,sizeof hand_before));four();
    recoil::compose_end(compose,true);live::pose_end(compose,true);live::pose_end(tick,true);drained();
    C(!recoil::pending_count()&&recoil::record_count==1);C(live::completed==1&&live::records[0].planned==15&&rig::core_reads==1);
    auto const& r=recoil::records[0];
    if(mode=="active"||mode=="freeze"||mode=="ring-default"){C(r.qualified&&r.natural_calls==1&&!r.no_natural_call&&(r.read_mask&recoil::output_read));C(r.affected==4&&r.pose_header.count==1&&r.original_rcx==node_at&&r.original_rsp==rsp);C(!memcmp(&matrix,&r.matrix,64));}
    else if(mode=="inactive"){C(r.qualified&&r.no_natural_call&&r.natural_calls==0&&r.begin.flags140==0);C(!(r.read_mask&recoil::output_read));}
    else{C(!r.qualified&&r.failure!=recoil::Failure::none);if((mode=="exception"||mode=="freeze-exception"))C(r.failure==recoil::Failure::exception);if(mode=="stop")C(r.failure==recoil::Failure::stopped);}
    if((mode=="opaque"||mode=="freeze-opaque")){C(r.failure==recoil::Failure::shape&&!r.qualified);C((r.read_mask&recoil::headers_read)&&!(r.read_mask&recoil::output_read));C(r.scale_header.data==0xbad00000&&r.scale_header.count==1&&r.scale_header.capacity==1&&r.has_scale==1);C(!recoil::opaque_reads);}
    if(mode=="freeze"||mode=="ring-default"||mode=="freeze-opaque"||mode=="freeze-exception"){
        const bool frozen=mode=="freeze";const auto first_record=recoil::records[0];C(recoil::ring_frozen.load()==frozen);
        for(unsigned i=0;i<40;++i){selected();C(publish());setup_recoil(false);Entry next_tick=tick_entry(),next_compose=compose_entry();
            live::pose_begin(next_tick);live::pose_begin(next_compose);recoil::compose_begin(next_compose);four();recoil::compose_end(next_compose,true);live::pose_end(next_compose,true);live::pose_end(next_tick,true);drained();}
        C(recoil::composes==41&&recoil::natural_calls==1&&live::completed==41&&rig::core_reads==41);C(!recoil::pending_count()&&recoil::ring_frozen.load()==frozen);
        if(frozen){C(recoil::sequence==1&&recoil::record_count==1);C(!memcmp(&first_record,&recoil::records[0],sizeof(first_record)));}
        else{C(recoil::sequence==41&&recoil::record_count==32);for(auto const& row:recoil::records)C(row.no_natural_call&&row.qualified&&!row.natural_calls);}
        wchar_t folder[MAX_PATH]{};C(GetCurrentDirectoryW(MAX_PATH,folder)>0);C(swprintf_s(directory,L"%s\\recoil-freeze-%lu-%llu",folder,GetCurrentProcessId(),GetTickCount64())>0);C(CreateDirectoryW(directory,nullptr)!=FALSE);
        recoil::request_drain();C(!recoil::pending_count());recoil::print(nullptr,true);C(recoil::file_written.load());wchar_t path[MAX_PATH]{};C(swprintf_s(path,L"%s\\body-recoil-observations.bin",directory)>0);
        HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);C(f!=INVALID_HANDLE_VALUE);recoil::Header header{};DWORD got{};
        if(f!=INVALID_HANDLE_VALUE){C(ReadFile(f,&header,sizeof header,&got,nullptr)&&got==sizeof header);C(CloseHandle(f)!=FALSE);}
        C(header.header_size==88&&header.record_size==5168&&header.composes==41&&header.sequence==(frozen?1u:41u)&&header.count==(frozen?1u:32u));
    }
    printf("body_recoil %s: %u checks %u failures; actual24b owner/Rig/World; host identity/control-memory seams; packet unchanged\n",argv[1],checks,failures);
    return failures?1:0;
}

#else
int main(int argc,char** argv){if(argc!=2)return 2;wchar_t path[MAX_PATH]{};C(swprintf_s(path,L"%S\\recoil-off-%lu-%llu.ini",argv[1],GetCurrentProcessId(),GetTickCount64())>0);
    C(recoil::configure(path,nullptr));C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"enabled",L"1",path)!=FALSE);C(!recoil::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"enabled",L"invalid",path)!=FALSE);C(!recoil::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"enabled",L"0",path)!=FALSE);C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"freeze_on_first_natural_output",L"1",path)!=FALSE);C(!recoil::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"freeze_on_first_natural_output",L"invalid",path)!=FALSE);C(!recoil::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"BodyRecoilObserver",L"freeze_on_first_natural_output",L"0",path)!=FALSE);C(recoil::configure(path,nullptr));
    C(!recoil::enabled&&!recoil::pending_count());printf("body_recoil OFF: %u checks %u failures\n",checks,failures);return failures?1:0;
}
#endif
