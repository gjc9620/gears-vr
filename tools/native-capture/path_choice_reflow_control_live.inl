// Included inside path_choice_layout_observer after snapshot(). Private, default OFF.
#if defined(GEARS_PATH_CHOICE_REFLOW_CONTROL) && GEARS_PATH_CHOICE_REFLOW_CONTROL
extern thread_local unsigned child_depth;
namespace reflow_control {
namespace core=gears_path_reflow_control;
struct Frame {uintptr_t panel{},scene{},client{},caller{};uint64_t nonce{},world{};};
struct Contract {uintptr_t scene{};std::uint32_t top_bits{},bottom_bits{};unsigned vertical_offset_reference_pixels=16;
    position::trial::GroupFit group_fit{};};
struct Parent {Frame frame;Contract contract;float delta{};};
thread_local Parent const* current{};thread_local unsigned parent_depth{};
struct ParentBoundary {
    Parent const* previous=current;
    ParentBoundary(){current=nullptr;++parent_depth;}
    ~ParentBoundary(){current=previous;--parent_depth;}
};
std::atomic<uint64_t> attempted{},modified{},adopted{},native_restored{},guard_rejected{},failed{};
enum class Status:unsigned {adopted=1,native_restored=2,guard_rejected=3,exception=4,post_rejected=5};
struct Receipt {Frame frame;uintptr_t parent_caller{};core::Plan plan;Snapshot before{},after{};std::array<std::uint32_t,2> entry_owned{};
    long long enter{},exit{};DWORD thread{};Status status{};bool changed{},normal{};};
std::array<Receipt,4> receipts{};std::array<std::atomic<bool>,4> ready{};std::atomic<unsigned> receipt_slots{};
bool same_lease(Parent const& p){
    auto const& l=position::state.lease;auto const& c=l.config;
    return l.active&&l.touched[0]&&l.touched[1]&&position::successful_nonce==p.frame.nonce&&c.nonce==p.frame.nonce&&
        c.panel==p.frame.panel&&c.scene==p.frame.scene&&c.client==p.frame.client&&c.world_generation==p.frame.world&&
        committed_world_generation.load()==p.frame.world&&c.top_bits==p.contract.top_bits&&c.bottom_bits==p.contract.bottom_bits&&
        position::trial::same_offset(c,p.contract);
}
bool known_owned(Snapshot const& s){return s.position[1]==position::state.lease.owned[0]&&s.position[3]==position::state.lease.owned[1];}
void fail_closed(){
    ++failed;position::unresolved=true;position::terminal=true;position::state.lease.active=false;
    position::pending_restore=false;position::disable();
#if defined(GEARS_PATH_CHOICE_POSITION_SERVICE) && GEARS_PATH_CHOICE_POSITION_SERVICE
    position::publish_owner_ack(position::trial::Result::idle);position::isolate_ui_failure();
#else
    request_session_stop(SessionStop::proof_failure);
#endif
}
template<class Original,class Read> uintptr_t parent_call(void* object,uintptr_t caller,Original call,Read read){
    if(position::owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId()||parent_depth!=1||position::terminal.load()||position::restore_retries.load())return call();
    auto const& c=position::state.lease.config;
    if(!c.nonce||!position::applied.load()||position::successful_nonce!=c.nonce||reinterpret_cast<uintptr_t>(object)!=c.panel)return call();
    Parent p{{c.panel,c.scene,c.client,caller,c.nonce,c.world_generation},
        {c.scene,c.top_bits,c.bottom_bits,c.vertical_offset_reference_pixels,c.group_fit},position::trial::delta_for(c)};
    if(!same_lease(p))return call();
    auto before=snapshot(p.frame,read);
    if(!core::metadata(p.contract,before)||!known_owned(before)){++guard_rejected;fail_closed();return call();}
    // This context is independent of all observer/receipt row quotas.
    current=&p;struct Reset{~Reset(){current=nullptr;}} reset;return call();
}
bool request_current(Parent const& p){
    if(position::drain_requested.load()||position::terminal.load())return false;
    auto request=std::atomic_load(&position::selected);if(!request||GetTickCount64()>=request->expires)return false;
    auto const& c=request->config;auto const& lease=position::state.lease.config;
    return c.nonce==p.frame.nonce&&c.panel==p.frame.panel&&c.scene==p.frame.scene&&c.client==p.frame.client&&
        c.world_generation==p.frame.world&&c.pid==lease.pid&&c.creation_time==lease.creation_time&&c.started_qpc==lease.started_qpc&&
        c.base==lease.base&&c.panel_vtable==lease.panel_vtable&&c.top_bits==p.contract.top_bits&&c.bottom_bits==p.contract.bottom_bits&&
        position::trial::same_offset(c,p.contract);
}
void publish(unsigned slot,Receipt const& receipt){if(slot<receipts.size()){receipts[slot]=receipt;ready[slot].store(true,std::memory_order_release);}}
template<class Original,class Read> uintptr_t child_call(void* object,core::Input const& input,uintptr_t caller,Original call,Read read){
    auto plain=[&]{return call(core::value(input.faces[1]),core::value(input.faces[3]));};
    if(position::owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId()||child_depth!=1||!current||
       parent_depth!=1||position::terminal.load()||reinterpret_cast<uintptr_t>(object)!=current->frame.panel)return plain();
    auto const p=*current;if(!same_lease(p))return plain();++attempted;
    Receipt row{};row.frame=p.frame;row.parent_caller=p.frame.caller;row.frame.caller=caller;row.enter=qpc();row.thread=GetCurrentThreadId();
    row.entry_owned=position::state.lease.owned;row.before=snapshot(p.frame,read);
    unsigned slot=receipt_slots.load();while(slot<receipts.size()&&!receipt_slots.compare_exchange_weak(slot,slot+1)){}
    const bool safe=core::metadata(p.contract,row.before)&&known_owned(row.before)&&
        core::prepare(p.contract,row.before,input,p.delta,row.plan);
    if(!safe){++guard_rejected;row.plan.original=input;row.plan.adjusted=input;row.status=Status::guard_rejected;
        fail_closed();try{auto result=plain();row.normal=true;row.after=snapshot(p.frame,read);row.exit=qpc();publish(slot,row);return result;}
        catch(...){row.status=Status::exception;row.after=snapshot(p.frame,read);row.exit=qpc();publish(slot,row);throw;}}
    row.changed=request_current(p);if(row.changed)++modified;
    auto finish=[&](bool normal){
        row.normal=normal;row.after=snapshot(p.frame,read);row.exit=qpc();
        const bool lease=same_lease(p)&&position::state.lease.owned==row.entry_owned;
        if(normal&&lease&&row.changed&&core::controlled_exit(p.contract,row.before,row.after,row.plan)){
            // A <=2 ULP allowance exists ONLY for this normal own native write.
            // All subsequent holding/restoration still uses exact owned bits.
            position::state.lease.owned={row.after.position[1],row.after.position[3]};++adopted;row.status=Status::adopted;
        }else if(normal&&lease&&!row.changed&&core::original_exit(p.contract,row.before,row.after)){
            position::state.lease.touched={false,false};position::state.lease.active=false;
            position::state.last=position::trial::Result::restored;position::count(position::trial::Result::restored);
            ++native_restored;row.status=Status::native_restored;
        }else{row.status=normal?Status::post_rejected:Status::exception;fail_closed();}
        publish(slot,row);
    };
    uintptr_t result{};
    try{result=call(core::value(row.changed?row.plan.adjusted.faces[1]:input.faces[1]),
                    core::value(row.changed?row.plan.adjusted.faces[3]:input.faces[3]));}
    catch(...){finish(false);throw;}
    finish(true);return result;
}
void progress(FILE* f){
    fprintf(f,"path_reflow_control_compiled=1 path_reflow_control_attempted=%llu path_reflow_control_modified=%llu path_reflow_control_adopted=%llu path_reflow_control_native_restored=%llu path_reflow_control_guard_rejected=%llu path_reflow_control_failed=%llu path_reflow_control_receipts=%u path_reflow_control_receipt_limit=4 path_reflow_control_status_tuple=adopted:1,native_restored:2,guard_rejected:3,exception:4,post_rejected:5 ",
        attempted.load(),modified.load(),adopted.load(),native_restored.load(),guard_rejected.load(),failed.load(),receipt_slots.load());
    fprintf(f,"path_reflow_control_row_tuple=nonce,panel,scene,world,caller,parent_caller,thread,enter,exit,status,changed,normal,height_bits,offset_bits,input_top,input_bottom,adjusted_top,adjusted_bottom,entry_owned_top,entry_owned_bottom,expected_top,expected_bottom,actual_top,actual_bottom,before_mask,after_mask ");
    for(unsigned i=0;i<receipts.size();++i){if(!ready[i].load(std::memory_order_acquire))continue;auto const& r=receipts[i];
        fprintf(f,"path_reflow_control_%u=%llu,%llu,%llu,%llu,%llu,%llu,%lu,%lld,%lld,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u ",i,
            r.frame.nonce,static_cast<unsigned long long>(r.frame.panel),static_cast<unsigned long long>(r.frame.scene),r.frame.world,
            static_cast<unsigned long long>(r.frame.caller),static_cast<unsigned long long>(r.parent_caller),r.thread,r.enter,r.exit,unsigned(r.status),r.changed?1u:0u,r.normal?1u:0u,
            core::bits(r.plan.height),core::bits(r.plan.offset),r.plan.original.faces[1],r.plan.original.faces[3],r.plan.adjusted.faces[1],r.plan.adjusted.faces[3],
            r.entry_owned[0],r.entry_owned[1],r.plan.expected[0],r.plan.expected[1],r.after.position[1],r.after.position[3],r.before.mask,r.after.mask);}
}
}
#endif
