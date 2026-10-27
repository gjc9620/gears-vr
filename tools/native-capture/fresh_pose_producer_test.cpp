// Production observer/protected lifecycle with explicit CPU Actor/Core/code seams.
// No game process, engine calls, real code certificate or writer permission.
#define GEARS_FRESH_POSE_PRODUCER_OBSERVER 1
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdio>
#include <climits>
#include <stdexcept>
#include "fresh_pose_producer.h"
#include "cover_pose_phase.h"
#include "hand_core_target_adapter.h"
#include "cover_pose_code_certificate.h"
namespace {
namespace p=gears_cover_pose_phase;namespace m=gears_hand_authored_mount;
struct Id{uintptr_t address{},cls{},outer{};int index{},name_index{};unsigned number{};};
std::array<unsigned char,0x17070dc+8> image{};uintptr_t base=reinterpret_cast<uintptr_t>(image.data());
std::array<unsigned char,0x500> fixture_mesh{};std::array<unsigned char,0x300> root{},asset{},child{},table{},child_table{};
std::array<unsigned char,68> child_entry{},grand_entry{},leaf_entry{};
std::array<unsigned char,16*68> crowded_entry{};
std::array<unsigned char,0x300> grand{},leaf{},grand_table{},leaf_table{};
p::ArrayHeader nested_out{},alternate_desired{};unsigned tree_depth{},tree_limit{3};bool tree_run{},tree_crowded{},tree_unknown{},tree_mismatch{},tree_stop{},tree_return_mutation{},tree_copy_end{},tree_wrong_out{},tree_wrong_desired{};
uintptr_t deny_tree_read{};
std::array<p::Atom,115> atoms{},cache{};std::array<unsigned char,115> desired{};
std::array<m::ReferenceRecord68,115> reference{};
int64_t session_started_qpc=501;wchar_t directory[MAX_PATH]{};
std::atomic<unsigned> active{};std::atomic<bool> bad_owner{},bad_read{},bad_entry{},throw_begin{};
std::vector<std::pair<uintptr_t,size_t>> ranges;unsigned checks{},failures{},original_calls{},packet_checks{};
uintptr_t fifth_seen{},denied_fifth{};unsigned optional_fifth_reads{},raw4_calls{},raw5_calls{};int original_action{};HANDLE entered{},proceed{};
p::CallPacket* mutate_return_packet{};unsigned injected_packet_mutations{};
void C(bool b){++checks;if(!b){++failures;printf("FAIL %u\n",checks);}}
template<class T>uintptr_t addr(T const& x){return reinterpret_cast<uintptr_t>(&x);}
template<class A,class T>void put(A& x,size_t at,T const& v){memcpy(x.data()+at,&v,sizeof v);}
bool safe_copy(void* out,void const* from,size_t n){if(throw_begin.exchange(false))throw std::runtime_error("begin seam");if(bad_read)return false;
    if(mutate_return_packet){mutate_return_packet->gpr[0]^=1;mutate_return_packet=nullptr;++injected_packet_mutations;}
    auto a=reinterpret_cast<uintptr_t>(from);if(deny_tree_read&&a==deny_tree_read)return false;if(denied_fifth&&a<denied_fifth+8&&denied_fifth<a+n){++optional_fifth_reads;return false;}for(auto const& r:ranges)if(a>=r.first&&a-r.first<=r.second&&n<=r.second-(a-r.first)){memcpy(out,from,n);return true;}return false;}
constexpr int MH_OK=0;int MH_CreateHook(void*,void*,void** out){*out=reinterpret_cast<void*>(1);return MH_OK;}
namespace gears_anchor {using Identity=Id;}
namespace weapon_hand_scene_binding {
std::mutex mutex;bool metadata_current(){return !bad_owner;}
bool identity(uintptr_t a,Id& out,bool){if(a!=addr(root)&&a!=addr(asset)&&a!=addr(child)&&a!=addr(grand)&&a!=addr(leaf))return false;out={a,1,addr(fixture_mesh),1,1,0};return true;}
}
namespace cover_pose_phase_live {
namespace certificate=gears_cover_pose_certificate;bool certified=true;
p::Identity compact(Id const& a){return {a.address,a.cls,a.outer,a.index,a.name_index,a.number};}
bool same(p::Identity const& a,p::Identity const& b){return a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&a.internal_index==b.internal_index&&a.name_index==b.name_index&&a.number==b.number;}
bool same(p::OwnerKey const& a,p::OwnerKey const& b){return !memcmp(&a,&b,sizeof a);}
int64_t qpc(){LARGE_INTEGER x{};QueryPerformanceCounter(&x);return x.QuadPart;}
bool digest(certificate::Chunk const&){return true;} // Explicit CODE-CERT Host seam.
}
namespace continuous_hand_owner_live {
using Entry=gears_continuous_hand_entry::Entry;
bool enabled=true;
struct Owner{p::OwnerKey key{};};
struct Data{Owner owner{};struct {gears_native_rig_tracking::CoreSnapshot source{};}record{};
    m::Signature signature=m::marcus115_signature;std::array<m::ReferenceRecord68,115> reference{};};
struct Slot{uint64_t tick{17};Entry const* tick_address{};unsigned compose_depth{};Data data{};};
thread_local Slot slot{};
bool owner_current(){return !bad_owner;}bool same_source(){return !bad_owner;}
bool metadata(bool){slot.data.reference=::reference;return !bad_read;}bool geometry(){return !bad_read;}
}
#include "fresh_pose_producer_live.inl"
#include "fresh_pose_layout_test.inl"
namespace fresh_pose_producer_live {bool own_entries(){return !bad_entry;}}
namespace f=fresh_pose_producer_live;using Entry=gears_continuous_hand_entry::Entry;
extern std::array<unsigned char,32> motion;
struct CppException{int marker;};constexpr DWORD code=0xe042c022;
extern "C" void GearsFreshRootHook(){}extern "C" void GearsFreshOptionalHook(){}
void original(p::CallPacket* packet,uintptr_t fifth) {
    ++original_calls;fifth_seen=fifth;
    if(original_action==1)RaiseException(code,0,0,nullptr);
    if(original_action==2)throw CppException{42};
    if(original_action==3){SetEvent(entered);WaitForSingleObject(proceed,INFINITE);}
    if(tree_run){++tree_depth;
        if(tree_depth<tree_limit){auto nested=*packet;nested.gpr[1]=tree_depth==1?addr(child):(tree_limit>3?addr(child):addr(grand));
            if(tree_mismatch&&tree_depth==1)nested.gpr[1]=addr(grand);
            if(tree_depth>=2||(tree_wrong_out&&tree_depth==1))nested.gpr[2]=addr(nested_out);
            if(tree_wrong_desired&&tree_depth==1)nested.gpr[3]=addr(alternate_desired);
            const auto ra=base+(tree_unknown?0x12345:tree_depth==1?0xf7a6a:0xf7be6);
            f::GearsFreshInvokeProtected(&nested,0,ra,0,addr(motion));
        }
        if(tree_copy_end&&tree_depth==2){auto copied=*f::local.call;f::release(copied);}
        if(tree_stop&&tree_depth==2)f::request_drain();--tree_depth;
    }
    packet->gpr[0]=0x123456789abcdef0;packet->xmm[0][0]=0xab;
    if(original_action==4||(tree_return_mutation&&tree_depth==0))mutate_return_packet=packet;
}
extern "C" void GearsFreshRawInvoke5(p::CallPacket* packet,void*,uintptr_t fifth) noexcept(false){++raw5_calls;original(packet,fifth);}
extern "C" void GearsFreshRawInvoke4(p::CallPacket* packet,void*) noexcept(false){++raw4_calls;original(packet,0);}
struct alignas(16) FakeStack{uintptr_t padding{},words[8]{},tail{};};
FakeStack stacks[2]{};Entry tick{};p::CallPacket packet{};uint32_t root_flag{};std::array<unsigned char,32> motion{};
void setup(){
    for(auto& a:atoms){a.q[3]=1;a.scale=1;}cache=atoms;
    for(unsigned i=0;i<115;++i){desired[i]=static_cast<unsigned char>(i);int32_t parent=i?int32_t(i-1):0;memcpy(reference[i].bytes+56,&parent,4);}
    p::Matrix component{};component.values[0]=component.values[5]=component.values[10]=component.values[15]=1;
    put(fixture_mesh,0x218,addr(asset));put(fixture_mesh,0x230,addr(root));put(fixture_mesh,0x27c,p::ArrayHeader{addr(atoms),115,115});
    put(fixture_mesh,0x28c,p::ArrayHeader{addr(desired),115,115});put(fixture_mesh,0x26c,p::ArrayHeader{0x123000,115,115});put(fixture_mesh,0xa0,component);
    put(fixture_mesh,0x338,uint32_t(19));put(asset,0xa4,p::ArrayHeader{addr(reference),115,115});
    put(root,0,addr(table));put(root,0x60,addr(fixture_mesh));put(root,0x70,int32_t(2));put(root,0x80,p::ArrayHeader{addr(cache),115,115});
    put(root,0xbc,uint32_t(18));put(root,0xe4,addr(child_entry));put(root,0xec,int32_t(1));put(table,0x270,base+0xf7940);
    put(child_entry,8,addr(child));put(child_entry,0x10,1.0f);put(child,0,addr(child_table));put(child_table,0x270,base+0x1000);
    const std::pair<uintptr_t,size_t> rs[]={{base,image.size()},{addr(fixture_mesh),sizeof fixture_mesh},{addr(root),sizeof root},{addr(asset),sizeof asset},{addr(child),sizeof child},
        {addr(table),sizeof table},{addr(child_table),sizeof child_table},{addr(child_entry),sizeof child_entry},{addr(atoms),sizeof atoms},{addr(cache),sizeof cache},
        {addr(desired),sizeof desired},{addr(reference),sizeof reference},{addr(stacks),sizeof stacks},{addr(root_flag),sizeof root_flag},{addr(motion),sizeof motion}};
    ranges.assign(std::begin(rs),std::end(rs));
    tick.kind=2;tick.invocation=17;tick.thread=GetCurrentThreadId();tick.selected=true;continuous_hand_owner_live::slot.tick_address=&tick;
    auto& owner=continuous_hand_owner_live::slot.data.owner.key;owner.mesh.address=addr(fixture_mesh);owner.session=501;owner.scene=5;owner.scene_qpc=2;owner.world_generation=1;
    continuous_hand_owner_live::slot.data.reference=reference;
    const unsigned char import_bytes[]{0xff,0x25,0xb3,0x4b,0xda,0};memcpy(image.data()+0x577e5f,import_bytes,6);put(image,0x131ca18,base+0x1000);
    f::enabled=f::certified=f::reserved=true;f::targets[0]=reinterpret_cast<void*>(base+0xf7940);f::targets[1]=reinterpret_cast<void*>(base+0x109970);
    for(unsigned i=0;i<2;++i){stacks[i].words[0]=base+f::cert::returns[i];stacks[i].words[5]=addr(root_flag);}
    packet.gpr[1]=addr(root);packet.gpr[2]=addr(fixture_mesh)+0x27c;packet.gpr[3]=addr(fixture_mesh)+0x28c;packet.gpr[4]=addr(motion);
}
void invoke(unsigned kind=0,uintptr_t ra=0){auto p=packet;if(kind){p.gpr[1]=addr(fixture_mesh);p.gpr[3]=1;}
    f::GearsFreshInvokeProtected(&p,kind,ra?ra:base+f::cert::returns[kind],addr(stacks[kind].words),addr(root_flag));
    C(p.gpr[0]==0x123456789abcdef0&&p.xmm[0][0]==0xab);++packet_checks;
}
bool seh(){__try{invoke();}__except(GetExceptionCode()==code?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}return false;}
void compose(){Entry e{};e.kind=0;e.selected=true;e.invocation=23;e.thread=GetCurrentThreadId();e.ra=base+p::compose_caller;e.rcx=addr(fixture_mesh);e.rdx=e.rcx+0x27c;e.r8=e.rcx+0x28c;
    continuous_hand_owner_live::slot.compose_depth=1;f::pose_begin(e);continuous_hand_owner_live::slot.compose_depth=0;}
void close(bool ok=true){f::pose_end(tick,ok);}
void setup_tree(char const* name){
    f::record_call_tree=true;tree_run=true;tree_limit=!strcmp(name,"tree_overflow")?129u:3u;
    put(child_table,0x270,base+0xf7940);put(grand_table,0x270,base+0xf7940);put(leaf_table,0x270,base+0x1000);
    put(child,0,addr(child_table));put(grand,0,addr(grand_table));put(leaf,0,addr(leaf_table));
    for(auto* node:{&child,&grand,&leaf}){put(*node,0x60,addr(fixture_mesh));put(*node,0x70,int32_t(2));put(*node,0xbc,uint32_t(18));put(*node,0x80,p::ArrayHeader{addr(cache),115,115});}
    put(grand_entry,8,tree_limit>3?addr(child):addr(grand));put(grand_entry,0x10,1.0f);
    put(leaf_entry,8,addr(leaf));put(leaf_entry,0x10,1.0f);
    put(child,0xe4,addr(grand_entry));put(child,0xec,int32_t(1));put(grand,0xe4,addr(leaf_entry));put(grand,0xec,int32_t(1));
    nested_out={addr(atoms),115,115};alternate_desired={addr(desired),115,115};
    for(auto pair:{std::pair<uintptr_t,size_t>{addr(grand),sizeof grand},{addr(leaf),sizeof leaf},{addr(grand_table),sizeof grand_table},
        {addr(leaf_table),sizeof leaf_table},{addr(grand_entry),sizeof grand_entry},{addr(leaf_entry),sizeof leaf_entry},{addr(crowded_entry),sizeof crowded_entry},{addr(nested_out),sizeof nested_out},{addr(alternate_desired),sizeof alternate_desired}})ranges.push_back(pair);
    const unsigned char low_code[]{0xc5,0x7a,0x10,0x0d,0xde,0xf6,0x60,0x01},full_code[]{0xc5,0xf8,0x2f,0x05,0xb0,0xf6,0x60,0x01};
    memcpy(image.data()+0xf79f2,low_code,8);memcpy(image.data()+0xf7a1c,full_code,8);
    put(image,0x17070d8,uint32_t(0x3727c5ac));put(image,0x17070d4,uint32_t(0x3f7fff58));
    if(!strcmp(name,"tree_constants"))image[0x17070d8]^=1;
    if(!strcmp(name,"tree_readfail"))deny_tree_read=addr(child)+0x70;
    if(!strcmp(name,"tree_unknown"))tree_unknown=true;
    if(!strcmp(name,"tree_mismatch"))tree_mismatch=true;
    if(!strcmp(name,"tree_stop"))tree_stop=true;
    if(!strcmp(name,"tree_copy"))tree_copy_end=true;
    if(!strcmp(name,"tree_wrong_out"))tree_wrong_out=true;
    if(!strcmp(name,"tree_wrong_desired"))tree_wrong_desired=true;
    if(!strcmp(name,"tree_packet"))tree_return_mutation=true;
    if(!strcmp(name,"tree_children")){put(child,0xe4,addr(crowded_entry));put(child,0xec,int32_t(17));memcpy(crowded_entry.data(),grand_entry.data(),68);}
}
void tree_layout(char const* path){FILE* out{};if(fopen_s(&out,path,"wb")||!out)return;fprintf(out,"{\n");
fprintf(out,"\"TreeHeader\":{\"size\":%zu,\"fields\":{",sizeof(f::TreeHeader));
fprintf(out,"\"magic\":[%zu,%zu]",offsetof(f::TreeHeader,magic),sizeof(((f::TreeHeader*)nullptr)->magic));
fprintf(out,",\"version\":[%zu,%zu]",offsetof(f::TreeHeader,version),sizeof(((f::TreeHeader*)nullptr)->version));
fprintf(out,",\"header_size\":[%zu,%zu]",offsetof(f::TreeHeader,header_size),sizeof(((f::TreeHeader*)nullptr)->header_size));
fprintf(out,",\"node_size\":[%zu,%zu]",offsetof(f::TreeHeader,node_size),sizeof(((f::TreeHeader*)nullptr)->node_size));
fprintf(out,",\"count\":[%zu,%zu]",offsetof(f::TreeHeader,count),sizeof(((f::TreeHeader*)nullptr)->count));
fprintf(out,",\"capacity_value\":[%zu,%zu]",offsetof(f::TreeHeader,capacity_value),sizeof(((f::TreeHeader*)nullptr)->capacity_value));
fprintf(out,",\"child_capacity\":[%zu,%zu]",offsetof(f::TreeHeader,child_capacity),sizeof(((f::TreeHeader*)nullptr)->child_capacity));
fprintf(out,",\"read_only\":[%zu,%zu]",offsetof(f::TreeHeader,read_only),sizeof(((f::TreeHeader*)nullptr)->read_only));
fprintf(out,",\"writer\":[%zu,%zu]",offsetof(f::TreeHeader,writer),sizeof(((f::TreeHeader*)nullptr)->writer));
fprintf(out,",\"fresh_certified\":[%zu,%zu]",offsetof(f::TreeHeader,fresh_certified),sizeof(((f::TreeHeader*)nullptr)->fresh_certified));
fprintf(out,",\"owner\":[%zu,%zu]",offsetof(f::TreeHeader,owner),sizeof(((f::TreeHeader*)nullptr)->owner));
fprintf(out,",\"tick\":[%zu,%zu]",offsetof(f::TreeHeader,tick),sizeof(((f::TreeHeader*)nullptr)->tick));
fprintf(out,",\"compose\":[%zu,%zu]",offsetof(f::TreeHeader,compose),sizeof(((f::TreeHeader*)nullptr)->compose));
fprintf(out,",\"root_serial\":[%zu,%zu]",offsetof(f::TreeHeader,root_serial),sizeof(((f::TreeHeader*)nullptr)->root_serial));
fprintf(out,",\"begin_qpc\":[%zu,%zu]",offsetof(f::TreeHeader,begin_qpc),sizeof(((f::TreeHeader*)nullptr)->begin_qpc));
fprintf(out,",\"end_qpc\":[%zu,%zu]",offsetof(f::TreeHeader,end_qpc),sizeof(((f::TreeHeader*)nullptr)->end_qpc));
fprintf(out,",\"source_record\":[%zu,%zu]",offsetof(f::TreeHeader,source_record),sizeof(((f::TreeHeader*)nullptr)->source_record));
fprintf(out,",\"thread\":[%zu,%zu]",offsetof(f::TreeHeader,thread),sizeof(((f::TreeHeader*)nullptr)->thread));
fprintf(out,",\"omitted\":[%zu,%zu]",offsetof(f::TreeHeader,omitted),sizeof(((f::TreeHeader*)nullptr)->omitted));
fprintf(out,",\"unclassified\":[%zu,%zu]",offsetof(f::TreeHeader,unclassified),sizeof(((f::TreeHeader*)nullptr)->unclassified));
fprintf(out,",\"started\":[%zu,%zu]",offsetof(f::TreeHeader,started),sizeof(((f::TreeHeader*)nullptr)->started));
fprintf(out,",\"closed\":[%zu,%zu]",offsetof(f::TreeHeader,closed),sizeof(((f::TreeHeader*)nullptr)->closed));
fprintf(out,",\"c0_joined\":[%zu,%zu]",offsetof(f::TreeHeader,c0_joined),sizeof(((f::TreeHeader*)nullptr)->c0_joined));
fprintf(out,",\"frozen\":[%zu,%zu]",offsetof(f::TreeHeader,frozen),sizeof(((f::TreeHeader*)nullptr)->frozen));
fprintf(out,",\"overflow\":[%zu,%zu]",offsetof(f::TreeHeader,overflow),sizeof(((f::TreeHeader*)nullptr)->overflow));
fprintf(out,",\"failed\":[%zu,%zu]",offsetof(f::TreeHeader,failed),sizeof(((f::TreeHeader*)nullptr)->failed));
fprintf(out,",\"root_returned\":[%zu,%zu]",offsetof(f::TreeHeader,root_returned),sizeof(((f::TreeHeader*)nullptr)->root_returned));
fprintf(out,"}}\n");
fprintf(out,",\"TreeNode\":{\"size\":%zu,\"fields\":{",sizeof(f::TreeNode));
fprintf(out,"\"serial\":[%zu,%zu]",offsetof(f::TreeNode,serial),sizeof(((f::TreeNode*)nullptr)->serial));
fprintf(out,",\"parent_serial\":[%zu,%zu]",offsetof(f::TreeNode,parent_serial),sizeof(((f::TreeNode*)nullptr)->parent_serial));
fprintf(out,",\"self\":[%zu,%zu]",offsetof(f::TreeNode,self),sizeof(((f::TreeNode*)nullptr)->self));
fprintf(out,",\"out_address\":[%zu,%zu]",offsetof(f::TreeNode,out_address),sizeof(((f::TreeNode*)nullptr)->out_address));
fprintf(out,",\"desired_address\":[%zu,%zu]",offsetof(f::TreeNode,desired_address),sizeof(((f::TreeNode*)nullptr)->desired_address));
fprintf(out,",\"motion\":[%zu,%zu]",offsetof(f::TreeNode,motion),sizeof(((f::TreeNode*)nullptr)->motion));
fprintf(out,",\"fifth\":[%zu,%zu]",offsetof(f::TreeNode,fifth),sizeof(((f::TreeNode*)nullptr)->fifth));
fprintf(out,",\"ra\":[%zu,%zu]",offsetof(f::TreeNode,ra),sizeof(((f::TreeNode*)nullptr)->ra));
fprintf(out,",\"rsp\":[%zu,%zu]",offsetof(f::TreeNode,rsp),sizeof(((f::TreeNode*)nullptr)->rsp));
fprintf(out,",\"index\":[%zu,%zu]",offsetof(f::TreeNode,index),sizeof(((f::TreeNode*)nullptr)->index));
fprintf(out,",\"parent_index\":[%zu,%zu]",offsetof(f::TreeNode,parent_index),sizeof(((f::TreeNode*)nullptr)->parent_index));
fprintf(out,",\"depth\":[%zu,%zu]",offsetof(f::TreeNode,depth),sizeof(((f::TreeNode*)nullptr)->depth));
fprintf(out,",\"edge\":[%zu,%zu]",offsetof(f::TreeNode,edge),sizeof(((f::TreeNode*)nullptr)->edge));
fprintf(out,",\"thread\":[%zu,%zu]",offsetof(f::TreeNode,thread),sizeof(((f::TreeNode*)nullptr)->thread));
fprintf(out,",\"begin_qpc\":[%zu,%zu]",offsetof(f::TreeNode,begin_qpc),sizeof(((f::TreeNode*)nullptr)->begin_qpc));
fprintf(out,",\"end_qpc\":[%zu,%zu]",offsetof(f::TreeNode,end_qpc),sizeof(((f::TreeNode*)nullptr)->end_qpc));
fprintf(out,",\"before\":[%zu,%zu]",offsetof(f::TreeNode,before),sizeof(((f::TreeNode*)nullptr)->before));
fprintf(out,",\"after\":[%zu,%zu]",offsetof(f::TreeNode,after),sizeof(((f::TreeNode*)nullptr)->after));
fprintf(out,",\"returned\":[%zu,%zu]",offsetof(f::TreeNode,returned),sizeof(((f::TreeNode*)nullptr)->returned));
fprintf(out,",\"exception\":[%zu,%zu]",offsetof(f::TreeNode,exception),sizeof(((f::TreeNode*)nullptr)->exception));
fprintf(out,",\"end_known\":[%zu,%zu]",offsetof(f::TreeNode,end_known),sizeof(((f::TreeNode*)nullptr)->end_known));
fprintf(out,",\"parent_matched\":[%zu,%zu]",offsetof(f::TreeNode,parent_matched),sizeof(((f::TreeNode*)nullptr)->parent_matched));
fprintf(out,",\"packet_preserved\":[%zu,%zu]",offsetof(f::TreeNode,packet_preserved),sizeof(((f::TreeNode*)nullptr)->packet_preserved));
fprintf(out,"}}\n");
fprintf(out,",\"TreeSnapshot\":{\"size\":%zu,\"fields\":{",sizeof(f::TreeSnapshot));
fprintf(out,"\"identity\":[%zu,%zu]",offsetof(f::TreeSnapshot,identity),sizeof(((f::TreeSnapshot*)nullptr)->identity));
fprintf(out,",\"mesh\":[%zu,%zu]",offsetof(f::TreeSnapshot,mesh),sizeof(((f::TreeSnapshot*)nullptr)->mesh));
fprintf(out,",\"table\":[%zu,%zu]",offsetof(f::TreeSnapshot,table),sizeof(((f::TreeSnapshot*)nullptr)->table));
fprintf(out,",\"method\":[%zu,%zu]",offsetof(f::TreeSnapshot,method),sizeof(((f::TreeSnapshot*)nullptr)->method));
fprintf(out,",\"children_data\":[%zu,%zu]",offsetof(f::TreeSnapshot,children_data),sizeof(((f::TreeSnapshot*)nullptr)->children_data));
fprintf(out,",\"out\":[%zu,%zu]",offsetof(f::TreeSnapshot,out),sizeof(((f::TreeSnapshot*)nullptr)->out));
fprintf(out,",\"desired\":[%zu,%zu]",offsetof(f::TreeSnapshot,desired),sizeof(((f::TreeSnapshot*)nullptr)->desired));
fprintf(out,",\"cache\":[%zu,%zu]",offsetof(f::TreeSnapshot,cache),sizeof(((f::TreeSnapshot*)nullptr)->cache));
fprintf(out,",\"children_count\":[%zu,%zu]",offsetof(f::TreeSnapshot,children_count),sizeof(((f::TreeSnapshot*)nullptr)->children_count));
fprintf(out,",\"node70\":[%zu,%zu]",offsetof(f::TreeSnapshot,node70),sizeof(((f::TreeSnapshot*)nullptr)->node70));
fprintf(out,",\"node_bc\":[%zu,%zu]",offsetof(f::TreeSnapshot,node_bc),sizeof(((f::TreeSnapshot*)nullptr)->node_bc));
fprintf(out,",\"mesh338\":[%zu,%zu]",offsetof(f::TreeSnapshot,mesh338),sizeof(((f::TreeSnapshot*)nullptr)->mesh338));
fprintf(out,",\"desired_bytes\":[%zu,%zu]",offsetof(f::TreeSnapshot,desired_bytes),sizeof(((f::TreeSnapshot*)nullptr)->desired_bytes));
fprintf(out,",\"children\":[%zu,%zu]",offsetof(f::TreeSnapshot,children),sizeof(((f::TreeSnapshot*)nullptr)->children));
fprintf(out,",\"threshold_bits\":[%zu,%zu]",offsetof(f::TreeSnapshot,threshold_bits),sizeof(((f::TreeSnapshot*)nullptr)->threshold_bits));
fprintf(out,",\"threshold_instructions\":[%zu,%zu]",offsetof(f::TreeSnapshot,threshold_instructions),sizeof(((f::TreeSnapshot*)nullptr)->threshold_instructions));
fprintf(out,",\"copied_children\":[%zu,%zu]",offsetof(f::TreeSnapshot,copied_children),sizeof(((f::TreeSnapshot*)nullptr)->copied_children));
fprintf(out,",\"read_mask\":[%zu,%zu]",offsetof(f::TreeSnapshot,read_mask),sizeof(((f::TreeSnapshot*)nullptr)->read_mask));
fprintf(out,",\"truncated\":[%zu,%zu]",offsetof(f::TreeSnapshot,truncated),sizeof(((f::TreeSnapshot*)nullptr)->truncated));
fprintf(out,",\"stable\":[%zu,%zu]",offsetof(f::TreeSnapshot,stable),sizeof(((f::TreeSnapshot*)nullptr)->stable));
fprintf(out,"}}\n");
fprintf(out,"}\n");fclose(out);}
void check_tree(char const* name){
    if(!strcmp(name,"tree_config")){wchar_t cwd[MAX_PATH]{},path[MAX_PATH]{};C(GetCurrentDirectoryW(MAX_PATH,cwd)!=0);C(swprintf_s(path,L"%s\\tree-config-%lu.ini",cwd,GetCurrentProcessId())>0);
        f::enabled=false;C(WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"0",path)!=0);C(f::configure(path,nullptr)&&!f::record_call_tree);
        C(WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"record_call_tree",L"1",path)!=0);C(!f::configure(path,nullptr)&&!f::enabled);
        C(WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"1",path)!=0);C(WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"record_call_tree",L"2",path)!=0);C(!f::configure(path,nullptr)&&!f::enabled);
        C(WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"record_call_tree",L"1",path)!=0);C(f::configure(path,nullptr)&&f::enabled&&f::record_call_tree);C(DeleteFileW(path)!=0);return;}
    setup_tree(name);
    if(!strcmp(name,"tree_seh")){original_action=1;C(seh());close(false);C(f::tree_closed&&f::tree_failed&&!f::tree_c0_joined&&!f::pending_count()&&active==0);return;}
    if(!strcmp(name,"tree_cpp")){original_action=2;bool caught{};try{invoke();}catch(CppException const& e){caught=e.marker==42;}C(caught);close(false);C(f::tree_closed&&f::tree_failed&&!f::tree_c0_joined&&!f::pending_count()&&active==0);return;}
    const auto old_mesh=fixture_mesh;const auto old_node=root;const auto old_atoms=atoms,old_cache=cache;
    invoke();C(f::in_flight==0&&active==0&&original_calls==tree_limit);
    C(fixture_mesh==old_mesh&&root==old_node&&!memcmp(&old_atoms,&atoms,sizeof atoms)&&!memcmp(&old_cache,&cache,sizeof cache));
    if(!strcmp(name,"tree_wrong_end")){auto bad=tick;f::pose_end(bad,true);C(!f::tree_closed&&f::open_frames==1&&f::protocol_debt==1);}
    compose();close();C(f::tree_closed&&f::tree_frozen&&f::open_frames==0);
    C(f::tree_count==(!strcmp(name,"tree_overflow")?128u:tree_limit));C(f::tree_nodes[0].parent_index==128&&f::tree_nodes[0].depth==1);
    for(unsigned i=0;i<f::tree_count;++i){auto const& n=f::tree_nodes[i];C(n.returned&&n.end_known&&n.serial);if(i){C(n.parent_index==i-1&&n.parent_serial==f::tree_nodes[i-1].serial);}}
    C(f::tree_nodes[0].before.threshold_bits[0]==(!strcmp(name,"tree_constants")?0x3727c5adu:0x3727c5acu));
    if(!strcmp(name,"tree_normal")||!strcmp(name,"tree_export")||!strcmp(name,"tree_existing")){
        C(f::tree_c0_joined&&!f::tree_failed&&!f::tree_overflow);C(f::tree_nodes[1].edge==1&&f::tree_nodes[2].edge==2);
        C(f::tree_nodes[2].out_address==addr(nested_out)&&f::tree_nodes[0].out_address==addr(fixture_mesh)+0x27c);
        C(f::tree_nodes[2].before.children[0].method==base+0x1000);C(original_calls==3); // Unknown candidate leaf was NOT entered.
        for(unsigned i=0;i<3;++i)C(f::tree_nodes[i].before.read_mask==2047&&f::tree_nodes[i].after.read_mask==2047&&f::tree_nodes[i].parent_matched);
        auto before=f::tree_count.load();invoke();C(f::tree_count==before&&original_calls==6); // First frame only, no overwrite.
    }else{C(!f::tree_c0_joined);if(!strcmp(name,"tree_overflow"))C(f::tree_overflow&&f::tree_omitted==1);else C(f::tree_failed||f::tree_overflow);}
    if(!strcmp(name,"tree_wrong_out")||!strcmp(name,"tree_wrong_desired"))C(!f::tree_nodes[1].parent_matched);
    if(!strcmp(name,"tree_children"))C(f::tree_nodes[1].before.children_count==17&&f::tree_nodes[1].before.copied_children==16&&f::tree_nodes[1].before.truncated);
    if(!strcmp(name,"tree_packet"))C(!f::tree_nodes[0].packet_preserved&&injected_packet_mutations==1);
    if(!strcmp(name,"tree_wrong_end")||!strcmp(name,"tree_copy"))C(f::pending_count()==1);else C(!f::pending_count());
    C(sizeof(f::Header)==80&&sizeof(f::Record)==66400);
    if(!strcmp(name,"tree_export")||!strcmp(name,"tree_existing")){
        wchar_t cwd[MAX_PATH]{},path[MAX_PATH]{};C(GetCurrentDirectoryW(MAX_PATH,cwd)!=0);
        C(swprintf_s(directory,L"%s\\%S-%lu",cwd,name,GetCurrentProcessId())>0);C(CreateDirectoryW(directory,nullptr)!=0);
        C(swprintf_s(path,L"%s\\fresh-pose-call-tree.bin",directory)>0);
        if(!strcmp(name,"tree_existing")){HANDLE h=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);C(h!=INVALID_HANDLE_VALUE);DWORD written{};const char sentinel[]="preserve";C(WriteFile(h,sentinel,sizeof sentinel,&written,nullptr)&&written==sizeof sentinel);C(CloseHandle(h)!=0);}
        f::stopping=true;f::tree_print(false);C(!f::tree_file_attempted);f::tree_print(true);
        if(!strcmp(name,"tree_export")){C(f::tree_file_written);FILE* file{};C(!_wfopen_s(&file,path,L"rb"));f::TreeHeader header{};C(fread(&header,sizeof header,1,file)==1);C(!memcmp(header.magic,"FPTRE001",8)&&header.count==3&&header.read_only==1&&!header.writer&&!header.fresh_certified&&header.c0_joined);C(fseek(file,0,SEEK_END)==0&&ftell(file)==long(sizeof header+3*sizeof(f::TreeNode)));C(fclose(file)==0);}
        else{C(!f::tree_file_written&&f::tree_file_error==ERROR_FILE_EXISTS);FILE* file{};C(!_wfopen_s(&file,path,L"rb"));char bytes[9]{};C(fread(bytes,1,9,file)==9&&!memcmp(bytes,"preserve",9));C(fclose(file)==0);}
        const auto error=f::tree_file_error.load();f::tree_print(true);C(f::tree_file_error==error);
    }
}

}
int main(int argc,char** argv){setup();if(argc==3&&!strcmp(argv[1],"--layout")){f::write_layout(argv[2]);return 0;}if(argc==3&&!strcmp(argv[1],"--tree-layout")){tree_layout(argv[2]);return 0;}const char* name=argc>1?argv[1]:"normal";
    if(!strncmp(name,"tree_",5)){check_tree(name);}
    else if(!strcmp(name,"copy")){f::Call c{};f::acquire(c);f::begin(packet,0,base+f::cert::returns[0],addr(stacks[0].words),addr(root_flag),c);
        auto copy=c;f::release(copy);C(f::in_flight==1&&active==1&&f::protocol_debt==1);f::release(c);C(f::in_flight==0&&active==0&&f::protocol_debt==1);
        f::release(c);C(f::in_flight==0&&active==0&&f::protocol_debt==2);close();C(f::pending_count()>=2);
        f::protocol_debt=UINT_MAX;f::in_flight=1;C(f::pending_count()==UINT_MAX);
    }else if(!strcmp(name,"seh")){original_action=1;C(seh());close(false);C(f::in_flight==0&&active==0&&!f::pending_count());C(f::records[0].reject==f::Reject::exception);
    }else if(!strcmp(name,"cpp")){original_action=2;bool caught{};try{invoke();}catch(CppException const& e){caught=e.marker==42;}C(caught);close(false);C(!f::pending_count()&&active==0);
    }else if(!strcmp(name,"wrong")){invoke(0,base+1);C(!f::count&&!f::pending_count()&&original_calls==1);bad_owner=true;invoke();C(!f::count&&original_calls==2);
    }else if(!strcmp(name,"begin_throw")){throw_begin=true;invoke();C(f::in_flight==0&&active==0&&original_calls==1&&f::protocol_debt);close();C(f::pending_count()!=0);
    }else if(!strcmp(name,"unknown_end")){invoke();auto copy=tick;f::pose_end(copy,true);C(f::open_frames==1&&f::protocol_debt==1);close();C(f::open_frames==0&&f::pending_count()==1);
    }else if(!strcmp(name,"stop")){original_action=3;entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);proceed=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        std::thread stop([]{WaitForSingleObject(entered,INFINITE);C(f::in_flight==1&&f::open_frames==1);f::request_drain();SetEvent(proceed);});
        invoke();stop.join();close();C(!f::pending_count()&&f::records[0].reject==f::Reject::stopped);CloseHandle(entered);CloseHandle(proceed);
    }else{
        if(!strcmp(name,"packet_mutation"))original_action=4;
        if(!strcmp(name,"alias"))put(root,0x80,p::ArrayHeader{addr(atoms),115,115});
        if(!strcmp(name,"children"))put(root,0xec,int32_t(17));
        invoke();C(fifth_seen==addr(root_flag)&&f::in_flight==0&&f::open_frames==1&&f::records[0].calls[0].end_known);
        if(!strcmp(name,"optional")){denied_fifth=addr(stacks[1].words)+0x28;invoke(1);C(optional_fifth_reads==0&&raw4_calls==1&&raw5_calls==1);
            C(f::records[0].calls[1].fifth==0&&f::records[0].calls[1].argument_mask==0);}
        if(!strcmp(name,"duplicate"))invoke();
        if(!strcmp(name,"changed"))put(fixture_mesh,0x338,uint32_t(20));
        compose();close();C(!f::pending_count());
        if(!strcmp(name,"packet_mutation")){C(injected_packet_mutations==1&&!f::records[0].calls[0].packet_unchanged);
            C(f::records[0].calls[0].output.gpr[0]==0x123456789abcdef0&&f::records[0].reject==f::Reject::changed);}
        if(!strcmp(name,"normal")||!strcmp(name,"optional")){C(f::records[0].capture_policy1&&f::frozen);C(!f::records[0].fresh_certified&&!f::records[0].writer);
            C(f::records[0].copy_before.slot==base+0x131ca18&&f::records[0].copy_before.read_mask==63);C(f::records[0].calls[0].argument_mask==15);
            C(f::records[0].stages==(!strcmp(name,"optional")?31u:19u));C(f::records[0].stage[0].children[0].method==base+0x1000);}
        else C(!f::records[0].capture_policy1&&!f::frozen);
    }
    if(argc==3){FILE* file{};if(!fopen_s(&file,argv[2],"wb")){f::Header h{};memcpy(h.magic,"FPOSE001",8);h.version=1;h.header_size=sizeof h;h.record_size=sizeof(f::Record);h.count=f::count;h.capacity_value=8;h.read_only=1;h.child_capacity=16;h.stage_count=5;h.packet_size=160;h.fifth_forwarded=1;fwrite(&h,sizeof h,1,file);fwrite(f::records.data(),sizeof(f::Record),f::count,file);fclose(file);}}
    printf("FreshPoseHost %s: %u checks, %u failures; Actor/Core/code/original Host seams; writer=0\n",name,checks,failures);
    printf("ABI Header=%zu Record=%zu Stage=%zu Child=%zu NativeCall=%zu packet=%zu offsets stages=%zu calls=%zu reference=%zu\n",sizeof(f::Header),sizeof(f::Record),sizeof(f::Stage),sizeof(f::Child),sizeof(f::NativeCall),sizeof(p::CallPacket),offsetof(f::Record,stage),offsetof(f::Record,calls),offsetof(f::Record,reference));
    return failures?1:0;
}
