// Included INSIDE cover_control_observer_live after finish_call. No hook,
// getter, engine, bridge, pose mutation, or writer permission is introduced.
// Existing CCTRL in_flight covers this synchronous post-RawInvoke observer.
namespace post_weight_scale {
constexpr unsigned capacity=4;
enum class Reject:unsigned {pending,okay,call_scope,source,identity,stack,read,shape,alias,
    changed,scale_nonempty,scale_flag,alpha,exception,compose_failed,nested,concurrent,packet_changed};
struct Readback {
    uintptr_t return_address{};
    // Saved ASM 12910F stores EAX and 129163 loads R12D: the cached
    // post-v240 flag is a dword. Preserve raw4 and decode that same read;
    // never infer this flag from the v240 return register.
    std::array<unsigned char,4> has_scale_raw4{};uint32_t has_scale_word{};
    pose::ArrayHeader scale{},output{},affected{};
    std::array<int32_t,3> bones{};std::array<pose::Matrix,3> matrices{};
    unsigned read_mask{};
};
enum ReadMask:unsigned {return_read=1,flag_read=2,scale_read=4,output_read=8,affected_read=16,bones_read=32,matrices_read=64};
struct Record {
    Row compute{},weight{};pose::OwnerKey scope_owner{};
    uint64_t scope_compose{},scope_outer_tick{};uintptr_t scope_address{},call_address{};
    unsigned scope_episode{},scope_thread{},scope_depth{},call_index{},frame_index{},matrix_count{};
    uintptr_t has_scale_address{},scale_address{},output_address{},affected_address{};
    LONGLONG sample_begin_qpc{},sample_end_qpc{};
    NodeState node_before{},node_after{};Readback before{},after{};
    pose::CallPacket original_return_packet{};
    Reject reject{Reject::pending};
    bool original_returned{},scope_open{},entry_before{},entry_after{},node_before_valid{},node_after_valid{},
        linked_source{},packet_unchanged{},compose_closed{},compose_returned{},nested{},concurrent{};
};
struct Header {
    char magic[8]{};unsigned version{},header_size{},row_size{},count{},capacity{};
    uint64_t session{},module_base{},compose{};unsigned pid{},pose_episode{},read_only{},atomic_verified{},writer{},
        v248_observed{},scale_payload_decoded{},cached_flag_bytes{};
};
static_assert(std::is_trivially_copyable_v<Record>&&std::is_trivially_copyable_v<Header>);
bool enabled{};std::mutex record_mutex;std::array<Record,capacity> records{};unsigned count{},dropped{};
std::atomic<bool> attempted{},written{};std::atomic<DWORD> file_error{};
struct Workspace {Record record{};};inline thread_local Workspace workspace{};

bool linked(Compose const& frame,Call const& call,Row const& compute,bool realtime=false){
    const auto& weight=call.row;
    return compute.kind<2&&compute.failure==Failure::okay&&compute.before_valid&&compute.after_valid&&compute.returned&&
        !compute.exception&&!compute.nested&&!compute.concurrent&&!compute.compose_returned&&!compute.compose_exception&&
        phase::same(compute.owner,weight.owner)&&phase::same(weight.owner,frame.scope.owner.key)&&
        compute.compose==weight.compose&&weight.compose==frame.scope.invocation&&compute.outer_tick==weight.outer_tick&&
        weight.outer_tick==frame.outer_tick&&compute.episode==weight.episode&&weight.episode==frame.scope.episode&&
        compute.thread==weight.thread&&weight.thread==GetCurrentThreadId()&&weight.thread==frame.scope.owner.thread&&
        compute.ordinal==weight.compute_ordinal&&weight.ordinal==compute.ordinal+1&&
        frame.ordinal==weight.ordinal&&frame.last_ordinal==compute.ordinal&&(realtime||frame.last_row==weight.linked_row)&&
        compute.node_argument==weight.node_argument&&frame.last_node==compute.node_argument&&
        phase::same(frame.last_identity,compute.after.node)&&compute.mesh_argument==weight.owner.mesh.address&&
        compute.caller==base+0x1290e5&&weight.caller==base+0x12911f&&compute.entry_rsp==weight.entry_rsp&&
        same_node(compute.after,weight.before)&&same_node(weight.before,weight.after)&&
        pose::metadata_equal(compute.after.metadata,weight.before.metadata)&&
        pose::metadata_equal(weight.before.metadata,weight.after.metadata)&&
        compute.after.parameters==weight.before.parameters&&weight.before.parameters==weight.after.parameters;
}
bool role(Row const& compute){
    const unsigned n=compute.kind?1:3;const int32_t end=static_cast<int32_t>(compute.raw_rdx);
    if(end!=11&&end!=35)return false;
    const std::array<int32_t,3> wanted=end==11?std::array<int32_t,3>{{8,10,11}}:std::array<int32_t,3>{{32,34,35}};
    for(unsigned i=0;i<n;++i)if(compute.affected[i]!=(compute.kind?end:wanted[i]))return false;
    return compute.out_after.count==int(n)&&compute.affected_after.count==int(n);
}
bool scope(Compose const& frame,Call const& call){
    const auto& row=call.row;
    return call.linked&&thread.call==&call&&!call.previous&&thread.depth==1&&!thread.overflow&&call.frame==0&&
        frame.admitted&&!frame.nested&&frame.last_valid&&frame.address&&frame.scope.kind==0&&frame.scope.arguments&&
        row.kind==2&&row.failure==Failure::okay&&row.before_valid&&row.after_valid&&row.returned&&!row.exception&&
        !row.nested&&!row.concurrent&&!row.compose_returned&&!row.compose_exception&&!stopping.load()&&current(frame);
}
bool raw_headers(Record const& record,Readback& out){
    // Separate validity bits prevent an unread/failed zero-filled field from
    // being reported as an observed empty scale array or false cached flag.
    if(read(out.return_address,record.weight.entry_rsp))out.read_mask|=return_read;
    if(read(out.has_scale_raw4,record.has_scale_address)){memcpy(&out.has_scale_word,out.has_scale_raw4.data(),4);out.read_mask|=flag_read;}
    if(read(out.scale,record.scale_address))out.read_mask|=scale_read;
    if(read(out.output,record.output_address))out.read_mask|=output_read;
    if(read(out.affected,record.affected_address))out.read_mask|=affected_read;
    return (out.read_mask&31)==31;
}
bool scale_header_shape(pose::ArrayHeader const& h){
    // No scale element stride is assumed or payload dereferenced. A nonempty
    // channel is retained as a raw header and rejected by this first observer.
    return h.count>=0&&h.capacity>=h.count&&h.capacity<=int(pose::max_bones)&&
        (h.capacity?(h.data!=0):h.data==0);
}
bool disjoint_payload(Record const& r,Readback const& b){
    const auto& m=r.node_before.metadata;const size_t n=r.matrix_count;
    struct Span {uintptr_t at;size_t bytes;};
    const Span spans[]={{r.weight.entry_rsp,8},{r.has_scale_address,4},{r.scale_address,16},{r.output_address,16},
        {r.affected_address,16},{b.output.data,n*64},{b.affected.data,n*4},{m.local.data,size_t(m.local.count)*32},
        {m.space.data,size_t(m.space.count)*64},{m.reference.data,size_t(m.reference.count)*68},
        {m.desired.data,size_t(m.desired.count)}};
    const unsigned total=m.desired.count?11:10;
    for(unsigned i=0;i<total;++i){if(!pose::span(spans[i].at,spans[i].bytes))return false;
        for(unsigned j=0;j<i;++j)if(spans[i].at<=spans[j].at+spans[j].bytes-1&&spans[j].at<=spans[i].at+spans[i].bytes-1)return false;}
    return true;
}
Reject payload(Record const& r,Readback& out){
    if(!pose::header_valid(out.output,64)||out.output.count!=int(r.matrix_count)||(out.output.data&3)||
        !pose::header_valid(out.affected,4)||out.affected.count!=int(r.matrix_count)||(out.affected.data&3))return Reject::shape;
    if(!disjoint_payload(r,out))return Reject::alias;
    if(safe_copy(out.bones.data(),reinterpret_cast<void const*>(out.affected.data),r.matrix_count*4))out.read_mask|=bones_read;
    if(safe_copy(out.matrices.data(),reinterpret_cast<void const*>(out.output.data),r.matrix_count*64))out.read_mask|=matrices_read;
    return (out.read_mask&96)==96?Reject::okay:Reject::read;
}
Reject sample(Compose const& frame,Call const& call,Record& r,bool realtime=false){
    if(!r.original_returned)return Reject::exception;
    r.scope_open=scope(frame,call);if(!r.scope_open)return Reject::call_scope;
    if(!r.linked_source||!linked(frame,call,r.compute,realtime)||!role(r.compute))return Reject::source;
    r.matrix_count=r.compute.kind?1:3;
    if(!pose::add(r.weight.entry_rsp,0x48,r.has_scale_address)||!pose::add(r.weight.entry_rsp,0x70,r.scale_address)||
        !pose::add(r.weight.entry_rsp,0x80,r.output_address)||!pose::add(r.weight.entry_rsp,0x90,r.affected_address)||
        (r.weight.entry_rsp&15)!=8||!stack_range(r.weight.entry_rsp,0xa0)||
        r.compute.out_argument!=r.output_address||r.weight.out_argument!=r.scale_address)return Reject::stack;
    r.entry_before=own_entries();if(!r.entry_before)return Reject::identity;
    r.node_before_valid=node_state(frame,r.weight.node_argument,r.node_before);
    if(!r.node_before_valid||!same_node(r.compute.after,r.node_before)||
        !pose::metadata_equal(r.compute.after.metadata,r.node_before.metadata)||r.compute.after.parameters!=r.node_before.parameters)return Reject::identity;
    const bool open_headers=raw_headers(r,r.before);
    const auto open_payload=open_headers?payload(r,r.before):Reject::read;
    // Re-read raw headers even after malformed/nonempty scale or failed pose
    // shape/read. No unsupported scale pointer is ever dereferenced.
    const bool close_headers=raw_headers(r,r.after);
    const auto close_payload=close_headers?payload(r,r.after):Reject::read;
    r.node_after_valid=node_state(frame,r.weight.node_argument,r.node_after);r.entry_after=own_entries();
    if(!open_headers||!close_headers)return Reject::read;
    if(!scale_header_shape(r.before.scale)||!scale_header_shape(r.after.scale))return Reject::shape;
    if(open_payload!=Reject::okay)return open_payload;if(close_payload!=Reject::okay)return close_payload;
    if(r.before.return_address!=r.weight.caller||r.after.return_address!=r.weight.caller||
        !pose::header_equal(r.before.scale,r.after.scale)||!pose::header_equal(r.before.output,r.after.output)||
        !pose::header_equal(r.before.affected,r.after.affected)||r.before.has_scale_raw4!=r.after.has_scale_raw4||
        memcmp(r.before.bones.data(),r.after.bones.data(),r.matrix_count*4)||
        memcmp(r.before.matrices.data(),r.after.matrices.data(),r.matrix_count*64))return Reject::changed;
    if(!r.node_after_valid||!r.entry_after||!same_node(r.node_before,r.node_after)||
        !pose::metadata_equal(r.node_before.metadata,r.node_after.metadata)||r.node_before.parameters!=r.node_after.parameters||
        !scope(frame,call))return Reject::identity;
    if(!pose::header_equal(r.before.output,r.compute.out_after)||!pose::header_equal(r.before.affected,r.compute.affected_after)||
        memcmp(r.before.bones.data(),r.compute.affected.data(),r.matrix_count*4)||
        memcmp(r.before.matrices.data(),r.compute.matrices.data(),r.matrix_count*64))return Reject::changed;
    if(r.before.scale.count)return Reject::scale_nonempty;
    if(r.before.has_scale_word)return Reject::scale_flag;
    // Historical read-only captures remain restricted to the exact full-weight
    // branch. Live ownership must also handle the normal fractional transition.
    if(realtime?!gears_body_c0_arm_alpha::normal_blend_weight(r.weight.weight_bits):
        r.weight.weight_bits!=0x3f800000u)return Reject::alpha;
    return Reject::okay;
}
void observe(Call const& call,pose::CallPacket const& packet,bool returned) noexcept {
    if(!enabled||!call.index||call.row.kind!=2)return;
    unsigned index=capacity;auto& r=workspace.record;r={};r.weight=call.row;r.original_return_packet=packet;
    r.original_returned=returned;r.sample_begin_qpc=phase::qpc();r.call_address=reinterpret_cast<uintptr_t>(&call);
    r.call_index=call.index;r.frame_index=call.frame;r.scope_depth=thread.depth;
    try{
        {std::lock_guard<std::mutex> lock(record_mutex);if(count==capacity){++dropped;return;}index=count++;}
        if(call.frame<scope_limit){const auto& frame=thread.frames[call.frame];r.scope_owner=frame.scope.owner.key;
            r.scope_compose=frame.scope.invocation;r.scope_outer_tick=frame.outer_tick;r.scope_episode=frame.scope.episode;
            r.scope_thread=frame.scope.owner.thread;r.scope_address=reinterpret_cast<uintptr_t>(frame.address);
            {std::lock_guard<std::mutex> lock(mutex);if(call.row.linked_row&&call.row.linked_row<=row_count){r.compute=rows[call.row.linked_row-1];r.linked_source=true;}}
            r.reject=sample(frame,call,r);
        }else r.reject=Reject::call_scope;
    }catch(...){r.reject=Reject::exception;}
    r.packet_unchanged=!memcmp(&packet,&r.original_return_packet,sizeof(packet));
    if(!r.packet_unchanged)r.reject=Reject::packet_changed;r.sample_end_qpc=phase::qpc();
    if(index<capacity){try{std::lock_guard<std::mutex> lock(record_mutex);records[index]=r;}catch(...){file_error=ERROR_WRITE_FAULT;}}
}
void on_compose_end(uint64_t invocation,bool returned,bool nested,bool concurrent) noexcept {
    if(!enabled)return;try{std::lock_guard<std::mutex> lock(record_mutex);for(unsigned i=0;i<count;++i){auto& r=records[i];
        if(r.scope_compose!=invocation)continue;r.compose_closed=true;r.compose_returned=returned;r.nested=nested;r.concurrent=concurrent;
        if(concurrent)r.reject=Reject::concurrent;else if(nested)r.reject=Reject::nested;else if(!returned)r.reject=Reject::compose_failed;
    }}catch(...){file_error=ERROR_WRITE_FAULT;}
}
bool configure(wchar_t const* ini,FILE* file){wchar_t value[16]{};
    GetPrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"0",value,16,ini);
    if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;
    if(!wcscmp(value,L"1")){if(enabled||!cover_control_observer_live::enabled)return false;enabled=true;}
    if(file)fprintf(file,"post_weight_scale_enabled=%u groups_max=4 read_only=1 writer=0 v248_observed=0\n",unsigned(enabled));return true;
}
void summary(FILE* file){if(!file)return;std::lock_guard<std::mutex> lock(record_mutex);
    fprintf(file,"post_weight_scale_enabled=%u post_weight_scale_groups=%u post_weight_scale_dropped=%u post_weight_scale_file_written=%u post_weight_scale_file_error=%lu ",unsigned(enabled),count,dropped,unsigned(written.load()),file_error.load());}
void print(FILE* file,bool disabled){
    if(enabled&&disabled&&stopping.load()&&!in_flight.load()&&!compose_in_flight.load()&&!attempted.exchange(true)){
        Header header{};memcpy(header.magic,"CPSCL001",8);header.version=1;header.header_size=sizeof(Header);header.row_size=sizeof(Record);
        header.count=count;header.capacity=capacity;header.session=selected_session;header.module_base=base;header.compose=selected_compose;
        header.pid=GetCurrentProcessId();header.pose_episode=selected_episode;header.read_only=1;header.cached_flag_bytes=4;
        wchar_t path[MAX_PATH]{};if(directory[0]&&swprintf_s(path,L"%s\\control-post-weight-scale.bin",directory)>0){
            HANDLE out=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(out!=INVALID_HANDLE_VALUE){DWORD bytes{};bool okay=WriteFile(out,&header,sizeof header,&bytes,nullptr)&&bytes==sizeof header;
                const auto length=DWORD(count*sizeof(Record));if(okay&&length)okay=WriteFile(out,records.data(),length,&bytes,nullptr)&&bytes==length;
                if(!okay)file_error=GetLastError();if(!CloseHandle(out)){file_error=GetLastError();okay=false;}written=okay;
            }else file_error=GetLastError();
        }else file_error=ERROR_INVALID_NAME;
    }
    summary(file);
}
} // namespace post_weight_scale
