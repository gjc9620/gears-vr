// Include after continuous_hand_owner_live. Only Native-owned copies are written.
#include "body_recoil_observer.h"
#include "body_recoil_certificate.h"
namespace body_recoil_observer_live {
using Entry=gears_continuous_hand_entry::Entry;
namespace hand=continuous_hand_owner_live;namespace phase=cover_pose_phase_live;
namespace pose=gears_cover_pose_phase;namespace binding=weapon_hand_scene_binding;
namespace certificate=gears_body_recoil_certificate;
bool enabled{};
#if defined(GEARS_BODY_RECOIL_OBSERVER) && GEARS_BODY_RECOIL_OBSERVER && defined(GEARS_CONTINUOUS_HAND_OWNER) && GEARS_CONTINUOUS_HAND_OWNER
constexpr unsigned capacity=32,chain_limit=16;
enum class Failure:uint32_t {none,context,model,mapping,code,node,changed,shape,duplicate,nested,exception,stopped,protocol};
enum ReadMask:uint32_t {context_read=1,model_read=2,map_read=4,begin_node_read=8,weight_before_read=16,weight_after_read=32,output_read=64,end_read=128,code_read=256,headers_read=512};
struct Link {pose::Identity id{};uintptr_t vtable{},next{};};
struct Mapping {
    pose::Identity root{};uintptr_t tree{},tree_class{};pose::ArrayHeader map{},lists{},tags{};
    unsigned char index{},tag{};int32_t pass{};std::array<unsigned char,20> list{};
    std::array<unsigned char,115> desired{};unsigned desired_count{},count{},selected=chain_limit;
    std::array<Link,chain_limit> links{};
};
struct Method {uintptr_t target{};std::array<unsigned char,32> prefix{};};
struct Node {
    pose::Identity id{};uintptr_t vtable{};uint32_t strength{},scale{},flags84{};int32_t lod{};
    std::array<int32_t,3> rotation{};std::array<uint32_t,3> translation{};std::array<uint32_t,2> view{};
    uint8_t flags140{};std::array<Method,7> methods{};
};
struct Record {
    pose::OwnerKey owner{};uint64_t tick{},compose{};unsigned thread{},read_mask{},natural_calls{};
    Failure failure{};bool returned{},no_natural_call{},qualified{};int64_t begin_qpc{},end_qpc{};
    gears_native_rig_tracking::CoreSnapshot source{};
    gears_hand_authored_mount::Signature signature{};pose::Metadata metadata{};Mapping mapping{};
    Node begin{},weight_before{},weight_after{},end{};
    uintptr_t original_rcx{},original_rsp{},original_ra{};uint32_t alpha_bits{},has_scale{};
    pose::ArrayHeader scale_header{},pose_header{},affected_header{};int32_t affected{};pose::Matrix matrix{};
};
struct Header {char magic[8]{};uint32_t version{},header_size{},record_size{},count{},capacity_value{},read_only{},writer{};
    uint64_t session{},module_base{},sequence{},composes{},natural_calls{},rejections{};};
static_assert(std::is_trivially_copyable_v<Record> && std::is_trivially_destructible_v<Record>);
struct Frame {Entry const* address{};WeightCall const* weight{};WeightCall const* top{};Record record{};unsigned weight_depth{};bool active{};
    Frame()=default;Frame(Frame const&)=delete;Frame& operator=(Frame const&)=delete;};
inline thread_local Frame frame{};
std::array<Record,capacity> records{};std::mutex records_mutex;uint64_t sequence{};unsigned record_count{};
std::atomic<unsigned> callbacks{},frames{};std::atomic<bool> stopping{},protocol_failed{},file_attempted{},file_written{};
std::atomic<DWORD> file_error{};std::atomic<uint64_t> composes{},natural_calls{},rejections{};
std::atomic<bool> freeze_on_first_natural_output{},ring_frozen{};
bool certified{};std::array<std::array<unsigned char,32>,3> certified_prefix{};
struct Callback {Callback(){++callbacks;}~Callback(){--callbacks;}};
#if defined(GEARS_BODY_RECOIL_OBSERVER_TEST)
// Host tests supply only raw memory and UObject/class identity seams.
bool read_memory(void*,uintptr_t,size_t);bool object(uintptr_t,gears_anchor::Identity&);
bool derives(uintptr_t,wchar_t const*,uintptr_t);
#else
bool read_memory(void* out,uintptr_t at,size_t n){return pose::span(at,n)&&safe_copy(out,reinterpret_cast<void const*>(at),n);}
bool object(uintptr_t at,gears_anchor::Identity& id){return binding::identity(at,id,true,true);}
bool derives(uintptr_t at,wchar_t const* name,uintptr_t ref){binding::ClassPath path{};return binding::derives(at,name,ref,path);}
#endif
template<class T>bool read(T& out,uintptr_t at){return pose::span(at,sizeof out)&&read_memory(&out,at,sizeof out);}
bool same_id(pose::Identity const& a,pose::Identity const& b){return a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&
    a.internal_index==b.internal_index&&a.name_index==b.name_index&&a.number==b.number;}
void fail(Failure why){if(frame.record.failure==Failure::none)frame.record.failure=why;++rejections;}
bool current(){return enabled&&certified&&!stopping.load()&&frame.active&&
    hand::slot.compose_address==frame.address&&hand::slot.tick==frame.record.tick&&
    hand::slot.data.record.compose==frame.record.compose&&hand::control_current(hand::slot.compose.scope)&&
    hand::same_source()&&!memcmp(&frame.record.source,&hand::slot.data.record.source,sizeof(frame.record.source));}
bool model(){pose::Request request{};request.owner=frame.record.owner;pose::Metadata first{},last{};
    auto reader=[](void* out,void const* in,size_t n){return read_memory(out,reinterpret_cast<uintptr_t>(in),n);};
    return hand::metadata(false)&&hand::slot.data.signature==gears_hand_authored_mount::marcus115_signature&&
        hand::slot.data.signature==frame.record.signature&&pose::metadata(reader,request,first)&&pose::metadata(reader,request,last)&&
        pose::metadata_equal(first,last)&&pose::metadata_equal(first,frame.record.metadata);}
bool certify(){
    for(unsigned i=0;i<certificate::chunks.size();++i){auto const& c=certificate::chunks[i];std::array<unsigned char,32> a{},b{};
        if(!read_memory(a.data(),base+c.rva,a.size())||!phase::digest(c)||!phase::digest(c)||
           !read_memory(b.data(),base+c.rva,b.size())||a!=b)return false;certified_prefix[i]=b;}
    return true;
}
bool code_current(){if(!certified||!phase::own_entries()||!cover_control_observer_live::own_entries())return false;
    for(unsigned i=0;i<certificate::chunks.size();++i){std::array<unsigned char,32> b{};
        if(!read_memory(b.data(),base+certificate::chunks[i].rva,b.size())||b!=certified_prefix[i])return false;}return true;}
bool bounded_header(pose::ArrayHeader const& h,size_t stride,unsigned limit,bool empty=false){return h.count>=0&&h.capacity>=h.count&&
    unsigned(h.capacity)<=limit&&(h.capacity?pose::span(h.data,size_t(h.capacity)*stride):empty&&h.data==0)&&
    (empty||h.count>0);}
bool mapping(Mapping& out){
    auto const& d=hand::slot.data;const auto mesh=d.owner.key.mesh.address;
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock()||!binding::metadata_current())return false;
    Mapping m{};gears_anchor::Identity root{},node{};
    if(!read(m.tree,mesh+0x230)||!object(m.tree,root)||root.outer!=mesh||
       !read(m.tree_class,base+certificate::anim_tree_class_slot)||!m.tree_class||!derives(root.cls,L"AnimTree",m.tree_class))return false;
    m.root=phase::compact(root);
    if(!read(m.map,mesh+0x2f4)||!bounded_header(m.map,1,512)||m.map.count<115||!read(m.index,m.map.data+4)||m.index==255||
       !read(m.lists,m.tree+0x138)||!bounded_header(m.lists,20,256)||m.index>=m.lists.count||
       !read_memory(m.list.data(),m.lists.data+size_t(m.index)*20,m.list.size())||
       memcmp(m.list.data(),d.reference[4].bytes,8)||!read(m.pass,m.tree+0x110)||
       !read(m.tags,m.tree+0x118)||!bounded_header(m.tags,1,512,true))return false;
    if(m.tags.count>4&&!read(m.tag,m.tags.data+4))return false;
    if(d.metadata.desired.count<0||d.metadata.desired.count>115)return false;m.desired_count=unsigned(d.metadata.desired.count);
    if(m.desired_count&&!read_memory(m.desired.data(),d.metadata.desired.data,m.desired_count))return false;
    uintptr_t at{};memcpy(&at,m.list.data()+8,8);
    while(at){if(m.count==chain_limit)return false;for(unsigned i=0;i<m.count;++i)if(m.links[i].id.address==at)return false;
        auto& link=m.links[m.count];if(!object(at,node)||node.outer!=m.tree||!derives(node.cls,L"SkelControlBase",0)||
            !read(link.vtable,at)||!link.vtable||!read(link.next,at+0xb4))return false;link.id=phase::compact(node);
        uintptr_t affected{},compute{},weight{};
        if(!read(affected,link.vtable+0x230)||!read(compute,link.vtable+0x238)||!read(weight,link.vtable+0x278))return false;
        if(affected==base+0x3dfc44&&compute==base+0x3a3d2c&&weight==base+0x515bb0){
            if(m.selected!=chain_limit||!derives(node.cls,L"WarSkelCtrl_Recoil",0))return false;m.selected=m.count;}
        ++m.count;at=link.next;
    }
    if(m.selected==chain_limit)return false;out=m;return true;
}
bool same_map(Mapping const& a,Mapping const& b){if(!same_id(a.root,b.root)||a.tree!=b.tree||a.tree_class!=b.tree_class||
    !pose::header_equal(a.map,b.map)||!pose::header_equal(a.lists,b.lists)||!pose::header_equal(a.tags,b.tags)||
    a.index!=b.index||a.tag!=b.tag||a.pass!=b.pass||a.list!=b.list||a.desired_count!=b.desired_count||a.desired!=b.desired||
    a.count!=b.count||a.selected!=b.selected)return false;
    for(unsigned i=0;i<a.count;++i)if(!same_id(a.links[i].id,b.links[i].id)||a.links[i].vtable!=b.links[i].vtable||a.links[i].next!=b.links[i].next)return false;return true;}
bool node(Node& out){auto const& link=frame.record.mapping.links[frame.record.mapping.selected];const auto at=link.id.address;
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock()||!binding::metadata_current())return false;
    Node n{};gears_anchor::Identity id{},again{};if(!object(at,id)||!same_id(phase::compact(id),link.id)||!read(n.vtable,at)||n.vtable!=link.vtable)return false;
    n.id=phase::compact(id);if(!read(n.strength,at+0x70)||!read(n.scale,at+0xac)||!read(n.flags84,at+0x84)||!read(n.lod,at+0xb0)||
        !read(n.rotation,at+0xf8)||!read(n.translation,at+0x12c)||!read(n.view,at+0x138)||!read(n.flags140,at+0x140))return false;
    constexpr unsigned slots[]={0x230,0x238,0x240,0x248,0x278,0x280,0x288};
    for(unsigned i=0;i<7;++i)if(!read(n.methods[i].target,n.vtable+slots[i])||!read_memory(n.methods[i].prefix.data(),n.methods[i].target,32))return false;
    if(n.methods[0].target!=base+0x3dfc44||n.methods[1].target!=base+0x3a3d2c||n.methods[4].target!=base+0x515bb0||
       !object(at,again)||!binding::same(id,again))return false;out=n;return true;
}
bool same_node(Node const& a,Node const& b){if(!same_id(a.id,b.id)||a.vtable!=b.vtable||a.strength!=b.strength||a.scale!=b.scale||
    a.flags84!=b.flags84||a.lod!=b.lod||a.rotation!=b.rotation||a.translation!=b.translation||a.view!=b.view||a.flags140!=b.flags140)return false;
    for(unsigned i=0;i<7;++i)if(a.methods[i].target!=b.methods[i].target||a.methods[i].prefix!=b.methods[i].prefix)return false;return true;}
bool stack(uintptr_t rsp){ULONG_PTR low{},high{};GetCurrentThreadStackLimits(&low,&high);return (rsp&15)==8&&rsp>=low&&rsp<high&&high-rsp>=0xa0;}
bool overlaps(uintptr_t a,size_t n,uintptr_t b,size_t m){return n&&m&&a<=b+m-1&&b<=a+n-1;}
bool output(uintptr_t rsp){auto& r=frame.record;pose::ArrayHeader s{},p{},a{},s2{},p2{},a2{};uint32_t has{},has2{};int32_t affected{},affected2{};pose::Matrix matrix{},matrix2{};
    if(!stack(rsp)||!read(s,rsp+0x70)||!read(p,rsp+0x80)||!read(a,rsp+0x90)||!read(has,rsp+0x48)||
       !read(s2,rsp+0x70)||!read(p2,rsp+0x80)||!read(a2,rsp+0x90)||!read(has2,rsp+0x48)||
       !pose::header_equal(s,s2)||!pose::header_equal(p,p2)||!pose::header_equal(a,a2)||has!=has2)return false;
    // These are observed raw headers, independently of output qualification.
    // The scale element stride/contents are unknown. Never dereference them
    // or infer their span; any nonempty scale channel is rejected.
    r.scale_header=s;r.pose_header=p;r.affected_header=a;r.has_scale=has;r.read_mask|=headers_read;
    if(s.count<0||s.capacity<s.count||s.capacity>115||(s.capacity?!s.data:s.data!=0)||s.count||has||
       !bounded_header(p,64,512)||!bounded_header(a,4,512)||p.count!=1||a.count!=1||!read(affected,a.data)||affected!=4)return false;
    const auto& m=r.metadata;const struct Range{uintptr_t at;size_t size;} blocked[]={
        {rsp,0xa0},{a.data,size_t(a.capacity)*4},{m.reference.data,size_t(m.reference.capacity)*68},
        {m.local.data,size_t(m.local.capacity)*32},{m.space.data,size_t(m.space.capacity)*64},{m.desired.data,size_t(m.desired.capacity)},
        {r.begin.id.address,0x141},{reinterpret_cast<uintptr_t>(&frame),sizeof(frame)},{reinterpret_cast<uintptr_t>(&hand::slot),sizeof(hand::slot)}};
    for(auto const& b:blocked)if(overlaps(p.data,64,b.at,b.size))return false;
    if(!read(matrix,p.data)||!pose::finite(matrix)||!read(s2,rsp+0x70)||!read(p2,rsp+0x80)||!read(a2,rsp+0x90)||!read(has2,rsp+0x48)||
       !pose::header_equal(s,s2)||!pose::header_equal(p,p2)||!pose::header_equal(a,a2)||has!=has2||!read(affected2,a.data)||affected2!=4||
       !read(matrix2,p.data)||memcmp(&matrix,&matrix2,64))return false;
    r.affected=affected;r.matrix=matrix;return true;
}
void compose_begin(Entry const& e) noexcept {Callback call;try{
    if(!enabled||stopping.load()||e.kind!=0||!e.compose_depth||!e.selected)return;
    if(frame.active){fail(Failure::nested);return;}if(hand::slot.compose_address!=&e||!hand::control_current(hand::slot.compose.scope))return;
    frame.active=true;frame.address=&e;frame.record={};frame.weight=nullptr;frame.top=nullptr;frame.weight_depth=0;++frames;++composes;
    auto& r=frame.record;r.owner=hand::slot.data.owner.key;r.tick=hand::slot.tick;r.compose=e.invocation;r.thread=GetCurrentThreadId();
    r.source=hand::slot.data.record.source;r.signature=hand::slot.data.signature;r.metadata=hand::slot.data.metadata;r.begin_qpc=phase::qpc();
    if(!current()){fail(Failure::context);return;}r.read_mask|=context_read;
    if(!model()){fail(Failure::model);return;}r.read_mask|=model_read;
    if(!code_current()){fail(Failure::code);return;}r.read_mask|=code_read;
    Mapping again{};if(!mapping(r.mapping)||!mapping(again)||!same_map(r.mapping,again)){fail(Failure::mapping);return;}r.read_mask|=map_read;
    if(!node(r.begin)||!current()){fail(Failure::node);return;}r.read_mask|=begin_node_read;
}catch(...){fail(Failure::exception);}}
void weight_begin(Entry const& e,WeightCall& call) noexcept {Callback callback;try{
    if(!enabled||!frame.active||e.kind!=2||e.rcx!=frame.record.begin.id.address||!(frame.record.read_mask&begin_node_read))return;
    call.entry=&e;call.previous=frame.top;call.tick=frame.record.tick;call.compose=frame.record.compose;call.depth=++frame.weight_depth;call.counted=true;frame.top=&call;
    if(call.depth!=1||frame.weight){fail(Failure::nested);return;}frame.weight=&call;call.selected=true;
    auto& r=frame.record;++r.natural_calls;++natural_calls;r.original_rcx=e.rcx;r.original_rsp=e.rsp;r.original_ra=e.ra;
    if(r.natural_calls!=1){fail(Failure::duplicate);return;}
    if(e.ra!=base+0x12911f||!stack(e.rsp)||!current()||!model()){fail(Failure::context);return;}
    Mapping again{};if(!code_current()||!mapping(again)||!same_map(r.mapping,again)||!node(r.weight_before)){fail(Failure::changed);return;}
    r.read_mask|=weight_before_read;
}catch(...){fail(Failure::exception);}}
void weight_end(WeightCall const& call,pose::CallPacket const& packet,bool returned) noexcept {Callback callback;
    if(!call.counted)return;if(!frame.active||frame.top!=&call||frame.weight_depth!=call.depth||call.tick!=frame.record.tick||call.compose!=frame.record.compose||
       (call.selected&&frame.weight!=&call)){protocol_failed=true;fail(Failure::protocol);return;}
    if(call.selected){try{auto& r=frame.record;if(!returned)fail(Failure::exception);
        else if(stopping.load())fail(Failure::stopped);
        else if(r.failure==Failure::none){float alpha{};memcpy(&r.alpha_bits,packet.xmm[0].data(),4);memcpy(&alpha,&r.alpha_bits,4);
            if(!std::isfinite(alpha)||!current()||!model()||!node(r.weight_after)||!same_node(r.weight_before,r.weight_after)){fail(Failure::changed);}
            else{r.read_mask|=weight_after_read;if(!output(r.original_rsp))fail(Failure::shape);else r.read_mask|=output_read;}}
    }catch(...){fail(Failure::exception);}frame.weight=nullptr;}
    frame.top=call.previous;--frame.weight_depth;
}
void compose_end(Entry const& e,bool returned) noexcept {Callback call;try{
    if(!frame.active||e.kind!=0||!e.compose_depth)return;if(frame.address!=&e){if(e.selected){protocol_failed=true;fail(Failure::protocol);}return;}
    if(frame.weight_depth||frame.weight||frame.top){protocol_failed=true;fail(Failure::protocol);return;}
    auto& r=frame.record;r.returned=returned;r.end_qpc=phase::qpc();r.no_natural_call=r.natural_calls==0;
    if(!returned)fail(Failure::exception);else if(stopping.load())fail(Failure::stopped);
    else if(r.failure==Failure::none){Mapping again{};if(!current()||!model()||!code_current()||!mapping(again)||!same_map(r.mapping,again)||!node(r.end))fail(Failure::changed);else r.read_mask|=end_read;}
    r.qualified=r.failure==Failure::none&&(r.read_mask&end_read)&&(r.no_natural_call||(r.read_mask&output_read));
    {std::lock_guard<std::mutex> lock(records_mutex);if(!ring_frozen.load()){
        records[sequence%capacity]=r;++sequence;if(record_count<capacity)++record_count;
        // Freeze only AFTER this fully qualified natural output was stored.
        // Admission, original calls, callbacks/Core, counters and drain remain live.
        if(freeze_on_first_natural_output.load()&&r.qualified&&r.natural_calls==1&&(r.read_mask&output_read))ring_frozen=true;
    }}
    frame.address=nullptr;frame.active=false;--frames;
}catch(...){protocol_failed=true;fail(Failure::exception);}}
bool configure(wchar_t const* ini,FILE* file){wchar_t value[8]{},freeze[8]{};GetPrivateProfileStringW(L"BodyRecoilObserver",L"enabled",L"0",value,8,ini);
    GetPrivateProfileStringW(L"BodyRecoilObserver",L"freeze_on_first_natural_output",L"0",freeze,8,ini);
    if((wcscmp(value,L"0")&&wcscmp(value,L"1"))||(wcscmp(freeze,L"0")&&wcscmp(freeze,L"1")))return false;
    const bool want_enabled=!wcscmp(value,L"1"),want_freeze=!wcscmp(freeze,L"1");
    if((want_freeze&&!want_enabled)||enabled)return false;
    if(want_enabled){if(stopping.load()||!hand::enabled||!hand::passive()||!phase::certified||!cover_control_observer_live::certified||!certify())return false;certified=true;freeze_on_first_natural_output=want_freeze;enabled=true;}
    if(file)fprintf(file,"body_recoil_enabled=%u body_recoil_freeze_on_first_natural_output=%u body_recoil_frozen=%u read_only=1 writer=0 event_required=0\n",unsigned(enabled),unsigned(freeze_on_first_natural_output.load()),unsigned(ring_frozen.load()));return true;}
void request_drain(){stopping=true;}unsigned pending_count(){return callbacks.load()+frames.load();}
void summary(FILE* file){if(file)fprintf(file,"body_recoil_enabled=%u body_recoil_freeze_on_first_natural_output=%u body_recoil_frozen=%u body_recoil_composes=%llu body_recoil_natural_calls=%llu body_recoil_rejections=%llu body_recoil_pending=%u body_recoil_protocol_failed=%u body_recoil_file_written=%u body_recoil_file_error=%lu ",unsigned(enabled),unsigned(freeze_on_first_natural_output.load()),unsigned(ring_frozen.load()),composes.load(),natural_calls.load(),rejections.load(),pending_count(),unsigned(protocol_failed.load()),unsigned(file_written.load()),file_error.load());}
void print(FILE* file,bool disabled){if(enabled&&disabled&&stopping.load()&&!pending_count()&&!file_attempted.exchange(true)){
    Header header{};memcpy(header.magic,"RCOBS001",8);header.version=1;header.header_size=sizeof header;header.record_size=sizeof(Record);header.count=record_count;header.capacity_value=capacity;
    header.read_only=1;header.session=uint64_t(session_started_qpc);header.module_base=base;header.sequence=sequence;header.composes=composes.load();header.natural_calls=natural_calls.load();header.rejections=rejections.load();
    wchar_t path[MAX_PATH]{};if(directory[0]&&swprintf_s(path,L"%s\\body-recoil-observations.bin",directory)>0){HANDLE fileout=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(fileout!=INVALID_HANDLE_VALUE){DWORD written{};bool okay=WriteFile(fileout,&header,sizeof header,&written,nullptr)&&written==sizeof header;
            DWORD n=DWORD(record_count*sizeof(Record));if(okay&&n)okay=WriteFile(fileout,records.data(),n,&written,nullptr)&&written==n;
            if(!okay)file_error=GetLastError();if(!CloseHandle(fileout)){file_error=GetLastError();okay=false;}file_written=okay;
        }else file_error=GetLastError();}else file_error=ERROR_INVALID_NAME;}summary(file);}
#else
void compose_begin(Entry const&) noexcept {}void compose_end(Entry const&,bool) noexcept {}
void weight_begin(Entry const&,WeightCall&) noexcept {}void weight_end(WeightCall const&,pose::CallPacket const&,bool) noexcept {}
bool configure(wchar_t const* ini,FILE*){wchar_t value[8]{};GetPrivateProfileStringW(L"BodyRecoilObserver",L"enabled",L"0",value,8,ini);if(wcscmp(value,L"0"))return false;GetPrivateProfileStringW(L"BodyRecoilObserver",L"freeze_on_first_natural_output",L"0",value,8,ini);return !wcscmp(value,L"0");}
void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE* file){if(file)fprintf(file,"body_recoil_enabled=0 body_recoil_compiled=0 ");}void print(FILE* file,bool){summary(file);}
#endif
}
