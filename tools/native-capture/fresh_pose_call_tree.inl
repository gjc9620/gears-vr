// Included inside the existing Fresh namespace, after its typed read/context helpers.
// One bounded, passive execution trace. No Local/Ref payload, engine call or write.
constexpr unsigned tree_capacity=128,tree_all_reads=2047;
bool record_call_tree{};
struct TreeSnapshot {
    pose::Identity identity{};uintptr_t mesh{},table{},method{},children_data{};
    pose::ArrayHeader out{},desired{},cache{};int32_t children_count{},node70{};uint32_t node_bc{},mesh338{};
    std::array<unsigned char,115> desired_bytes{};std::array<Child,child_limit> children{};
    std::array<uint32_t,2> threshold_bits{};std::array<std::array<unsigned char,8>,2> threshold_instructions{};
    unsigned copied_children{},read_mask{};bool truncated{},stable{};
};
struct TreeNode {
    uint64_t serial{},parent_serial{};uintptr_t self{},out_address{},desired_address{},motion{},fifth{},ra{},rsp{};
    unsigned index{},parent_index{tree_capacity},depth{},edge{},thread{};LONGLONG begin_qpc{},end_qpc{};
    TreeSnapshot before{},after{};bool returned{},exception{},end_known{},parent_matched{},packet_preserved{true};
};
struct TreeHeader {
    char magic[8]{};unsigned version{},header_size{},node_size{},count{},capacity_value{},child_capacity{},read_only{},writer{},fresh_certified{};
    pose::OwnerKey owner{};uint64_t tick{},compose{},root_serial{};LONGLONG begin_qpc{},end_qpc{};
    unsigned source_record{},thread{},omitted{},unclassified{};
    bool started{},closed{},c0_joined{},frozen{},overflow{},failed{},root_returned{};
};
static_assert(std::is_trivially_copyable_v<TreeNode>);
std::array<TreeNode,tree_capacity> tree_nodes{};TreeHeader tree_header{};
std::atomic<unsigned> tree_count{},tree_omitted{},tree_unclassified{};
std::atomic<bool> tree_claimed{},tree_closed{},tree_c0_joined{},tree_overflow{},tree_failed{},tree_frozen{},tree_root_returned{};
std::atomic<bool> tree_file_attempted{},tree_file_written{};std::atomic<DWORD> tree_file_error{};
inline thread_local TreeSnapshot tree_repeat{};
void tree_fail() noexcept {tree_failed=true;}
bool tree_context(Record const& r){return tree_claimed.load()&&!tree_closed.load()&&
    tree_header.source_record==r.index&&tree_header.tick==r.tick&&tree_header.thread==GetCurrentThreadId()&&
    phase::same(tree_header.owner,r.owner)&&context(r);}
void tree_snapshot_once(Record const& r,TreeNode const& n,TreeSnapshot& s){
    s={};if(!tree_context(r))return;s.read_mask|=1;
    bool fields=read(s.table,n.self)&&field(s.mesh,n.self,0x60)&&field(s.method,s.table,0x270);
    {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity id{};
        if(fields&&lock.owns_lock()&&binding::metadata_current()&&binding::identity(n.self,id,true)){
            s.identity=phase::compact(id);if(s.mesh==r.owner.mesh.address)s.read_mask|=2;}}
    if(field(s.node70,n.self,0x70)&&field(s.node_bc,n.self,0xbc)&&field(s.cache,n.self,0x80)&&
       field(s.mesh338,r.owner.mesh.address,0x338)&&field(s.children_data,n.self,0xe4)&&field(s.children_count,n.self,0xec))s.read_mask|=4;
    if(read(s.out,n.out_address)&&s.out.count>=0&&s.out.count<=115&&s.out.capacity>=s.out.count&&s.out.capacity<=512&&
       (!s.out.count||pose::header_valid(s.out,32)))s.read_mask|=8;
    if(read(s.desired,n.desired_address)&&s.desired.count>=0&&s.desired.count<=115&&s.desired.capacity>=s.desired.count&&s.desired.capacity<=512&&
       (!s.desired.count||(pose::header_valid(s.desired,1)&&safe_copy(s.desired_bytes.data(),reinterpret_cast<void const*>(s.desired.data),size_t(s.desired.count)))))s.read_mask|=16;
    if(s.children_count>=0){s.copied_children=unsigned(s.children_count)>child_limit?child_limit:unsigned(s.children_count);s.truncated=unsigned(s.children_count)>child_limit;
        bool raw=true,ids=true;for(unsigned i=0;i<s.copied_children;++i){auto& c=s.children[i];uintptr_t at{};
            if(!pose::add(s.children_data,size_t(i)*68,at)||!read(c.raw,at)){raw=false;ids=false;break;}c.read_mask=1;
            memcpy(&c.node,c.raw.data()+8,8);memcpy(&c.weight,c.raw.data()+0x10,4);memcpy(&c.flags,c.raw.data()+0x3c,4);
            if(!c.node){c.read_mask|=2;continue;}
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity id{};
            if(lock.owns_lock()&&binding::metadata_current()&&binding::identity(c.node,id,true)&&read(c.table,c.node)&&
               field(c.method,c.table,0x270)&&pose::span(c.method,32)&&safe_copy(c.prefix.data(),reinterpret_cast<void const*>(c.method),32)){
                c.identity=phase::compact(id);c.read_mask|=2;}else ids=false;
        }if(raw&&!s.truncated)s.read_mask|=32;if(ids&&!s.truncated)s.read_mask|=64;
    }
    // Mechanically decoded RIP-relative addresses, not historical IDA labels.
    constexpr std::array<uintptr_t,2> values{{0x17070d8,0x17070d4}},instructions{{0xf79f2,0xf7a1c}};
    constexpr std::array<uint32_t,2> expected_bits{{0x3727c5ac,0x3f7fff58}};
    constexpr std::array<std::array<unsigned char,8>,2> expected_instructions{{
        {{0xc5,0x7a,0x10,0x0d,0xde,0xf6,0x60,0x01}},{{0xc5,0xf8,0x2f,0x05,0xb0,0xf6,0x60,0x01}}}};
    bool constants=true;for(unsigned i=0;i<2;++i){uintptr_t a{},b{};
        if(!pose::add(base,values[i],a)||!pose::add(base,instructions[i],b)||!read(s.threshold_bits[i],a)||!read(s.threshold_instructions[i],b))constants=false;}
    if(constants){s.read_mask|=128;if(s.threshold_bits==expected_bits&&s.threshold_instructions==expected_instructions)s.read_mask|=256;}
    if(own_entries()&&tree_context(r))s.read_mask|=1024;
}
bool tree_same(TreeSnapshot const& a,TreeSnapshot const& b){
    if(!phase::same(a.identity,b.identity)||a.mesh!=b.mesh||a.table!=b.table||a.method!=b.method||a.children_data!=b.children_data||
       !pose::header_equal(a.out,b.out)||!pose::header_equal(a.desired,b.desired)||!pose::header_equal(a.cache,b.cache)||
       a.children_count!=b.children_count||a.node70!=b.node70||a.node_bc!=b.node_bc||a.mesh338!=b.mesh338||
       a.desired_bytes!=b.desired_bytes||a.threshold_bits!=b.threshold_bits||a.threshold_instructions!=b.threshold_instructions||
       a.copied_children!=b.copied_children||a.read_mask!=b.read_mask||a.truncated!=b.truncated)return false;
    for(unsigned i=0;i<child_limit;++i){auto const& x=a.children[i];auto const& y=b.children[i];
        if(x.raw!=y.raw||!phase::same(x.identity,y.identity)||x.node!=y.node||x.table!=y.table||x.method!=y.method||x.prefix!=y.prefix||x.flags!=y.flags||x.read_mask!=y.read_mask)return false;}
    return true; // No comparison of C++ padding or derived float NaN equality.
}
void tree_snapshot(Record const& r,TreeNode const& n,TreeSnapshot& s){
    tree_snapshot_once(r,n,s);tree_snapshot_once(r,n,tree_repeat);s.stable=tree_same(s,tree_repeat);
    if(s.stable&&tree_context(r))s.read_mask|=512;
    if(s.truncated)tree_overflow=true;if(s.read_mask!=tree_all_reads)tree_fail();
}
void tree_enter(Record const& r,pose::CallPacket const& p,uintptr_t ra,uintptr_t rsp,uintptr_t fifth,Call& c,bool top){
    if(!record_call_tree||!tree_context(r)||tree_root_returned.load())return;
    if(stopping.load()){++tree_omitted;tree_fail();return;}
    const unsigned index=tree_count.load();if(index==tree_capacity){tree_overflow=true;++tree_omitted;return;}
    auto& n=tree_nodes[index];n={};n.index=index;n.serial=c.serial;n.depth=local.depth;n.thread=c.thread;n.self=p.gpr[1];
    n.out_address=p.gpr[2];n.desired_address=p.gpr[3];n.motion=p.gpr[4];n.fifth=fifth;n.ra=ra;n.rsp=rsp;n.begin_qpc=phase::qpc();
    if(top){n.edge=0;n.parent_matched=true;}
    else{n.edge=ra==base+0xf7a6a?1u:ra==base+0xf7be6?2u:3u;
        if(n.edge==3){++tree_unclassified;tree_fail();}
        if(c.previous&&c.previous->tree_index<index){n.parent_index=c.previous->tree_index;n.parent_serial=c.previous->serial;}
        else tree_fail();}
    c.tree_index=index;tree_count=index+1;tree_snapshot(r,n,n.before);
    if(!top&&n.parent_index<index){auto const& parent=tree_nodes[n.parent_index];
        for(unsigned i=0;i<parent.before.copied_children;++i){auto const& branch_child=parent.before.children[i];
            if(branch_child.node==n.self&&branch_child.method==reinterpret_cast<uintptr_t>(targets[0])&&(branch_child.read_mask&2)&&phase::same(branch_child.identity,n.before.identity))n.parent_matched=true;}
        const bool known_edge=n.edge==1||n.edge==2;
        if(parent.serial!=n.parent_serial||parent.thread!=n.thread||(known_edge&&(n.desired_address!=parent.desired_address||(n.edge==1&&n.out_address!=parent.out_address))))n.parent_matched=false;
        if(!n.parent_matched)tree_fail();}
    // A readable v270 target in a Child is only a candidate, never a leaf-enter receipt.
}
void tree_start(Record const& r,pose::CallPacket const& p,uintptr_t ra,uintptr_t rsp,uintptr_t fifth,Call& c){
    if(!record_call_tree||r.reject!=Reject::waiting||local.depth!=1||c.kind||tree_claimed.exchange(true))return;
    tree_header={};tree_header.owner=r.owner;tree_header.tick=r.tick;tree_header.source_record=r.index;tree_header.thread=c.thread;
    tree_header.root_serial=c.serial;tree_header.begin_qpc=phase::qpc();tree_header.started=true;
    tree_enter(r,p,ra,rsp,fifth,c,true);
}
void tree_nested(pose::CallPacket const& p,uintptr_t ra,uintptr_t rsp,uintptr_t fifth,Call& c){
    if(!record_call_tree||c.kind||local.depth<=1||local.slot>=capacity||!tree_claimed.load())return;
    tree_enter(records[local.slot],p,ra,rsp,fifth,c,false);
}
void tree_finish(Call const& c,bool returned){
    if(c.tree_index>=tree_count.load()||c.tree_index>=tree_capacity)return;auto& n=tree_nodes[c.tree_index];
    if(n.serial!=c.serial||tree_header.source_record!=local.slot||n.thread!=GetCurrentThreadId()){tree_fail();return;}
    n.returned=returned;n.exception=!returned;n.end_qpc=phase::qpc();
    tree_snapshot(records[local.slot],n,n.after);if(!returned||stopping.load())tree_fail();
    if(!phase::same(n.before.identity,n.after.identity)||n.before.mesh!=n.after.mesh||n.before.table!=n.after.table||n.before.method!=n.after.method)tree_fail();
    if(c.tree_index==0)tree_root_returned=true;
}
void tree_packet_changed(Call const& c) noexcept {tree_fail();if(c.tree_index<tree_count.load()&&c.tree_index<tree_capacity)tree_nodes[c.tree_index].packet_preserved=false;}
void tree_release(Call const& c,bool known) noexcept {
    if(c.tree_index>=tree_count.load()||c.tree_index>=tree_capacity)return;auto& n=tree_nodes[c.tree_index];
    if(n.serial!=c.serial||!known){tree_fail();return;}n.end_known=true;
}
void tree_close(Record const& r){
    if(!record_call_tree||!tree_claimed.load()||tree_header.source_record!=r.index||tree_header.tick!=r.tick)return;
    tree_header.compose=r.compose;tree_header.end_qpc=phase::qpc();tree_closed=true;
    bool ended=tree_count.load()>0&&tree_root_returned.load();for(unsigned i=0;i<tree_count.load();++i)ended=ended&&tree_nodes[i].end_known&&tree_nodes[i].returned;
    if(!ended||!r.capture_policy1||stopping.load())tree_fail();
    tree_c0_joined=ended&&r.capture_policy1&&!tree_failed.load()&&!tree_overflow.load();tree_frozen=true;
}
void tree_summary(FILE* f){if(f)fprintf(f,"fresh_tree_enabled=%u fresh_tree_count=%u fresh_tree_overflow=%u fresh_tree_failed=%u fresh_tree_closed=%u fresh_tree_c0_joined=%u fresh_tree_frozen=%u fresh_tree_omitted=%u fresh_tree_unclassified=%u fresh_tree_written=%u fresh_tree_file_error=%lu ",unsigned(record_call_tree),tree_count.load(),unsigned(tree_overflow.load()),unsigned(tree_failed.load()),unsigned(tree_closed.load()),unsigned(tree_c0_joined.load()),unsigned(tree_frozen.load()),tree_omitted.load(),tree_unclassified.load(),unsigned(tree_file_written.load()),tree_file_error.load());}
void tree_print(bool disabled){
    if(!record_call_tree||!disabled||!stopping.load()||pending_count()||tree_file_attempted.exchange(true))return;
    TreeHeader h=tree_header;memcpy(h.magic,"FPTRE001",8);h.version=1;h.header_size=sizeof h;h.node_size=sizeof(TreeNode);h.count=tree_count;
    h.capacity_value=tree_capacity;h.child_capacity=child_limit;h.read_only=1;h.omitted=tree_omitted;h.unclassified=tree_unclassified;
    h.closed=tree_closed;h.c0_joined=tree_c0_joined;h.frozen=tree_frozen;h.overflow=tree_overflow;h.failed=tree_failed;h.root_returned=tree_root_returned;
    wchar_t path[MAX_PATH]{};if(!directory[0]||swprintf_s(path,L"%s\\fresh-pose-call-tree.bin",directory)<=0){tree_file_error=ERROR_INVALID_NAME;return;}
    HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(out==INVALID_HANDLE_VALUE){tree_file_error=GetLastError();return;}
    DWORD written{};bool ok=WriteFile(out,&h,sizeof h,&written,nullptr)&&written==sizeof h;const DWORD bytes=DWORD(h.count*sizeof(TreeNode));
    if(ok&&bytes)ok=WriteFile(out,tree_nodes.data(),bytes,&written,nullptr)&&written==bytes;
    if(!ok)tree_file_error=GetLastError();if(!CloseHandle(out)){tree_file_error=GetLastError();ok=false;}tree_file_written=ok;
}
