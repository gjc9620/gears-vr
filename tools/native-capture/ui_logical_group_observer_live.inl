// Included after UiInvocation, before scene/call-boundary live observers.
#include "ui_logical_group_observer.h"
namespace ui_logical_group_live {
#if defined(GEARS_UI_LOGICAL_GROUP_OBSERVER) && GEARS_UI_LOGICAL_GROUP_OBSERVER
struct Row {
    unsigned kind{},thread{},call_count{},call_overflow{},unclassified_canvas{},simple_executes{},material_executes{};
    uint64_t token{};uintptr_t owner{},function{},canvas{},fcanvas{},root_frame{};long long enter{},exit{};
    bool before_ok{},returned{},root_current{},delta_ok{},bounds_ok{};
    std::array<gears_ui_logical_group::Call,gears_ui_logical_group::call_limit> calls{};
    struct Node {gears_ui::Identity node{},cls{},outer{};};
    std::array<Node,32> nodes{};unsigned node_count{},node_rejected{},node_overflow{};
    struct Child {uint64_t id{},parent{};uintptr_t self{},callee{},caller{},script{};long long enter{},exit{};
        unsigned body_calls{},script_size{};bool returned{},current_after{};std::array<wchar_t,64> name{};};
    std::array<Child,128> children{};unsigned child_count{},child_overflow{};
    gears_ui_scene_batch::Delta delta;gears_ui_group::Bounds bounds{};
};
std::array<gears_ui_logical_group::Ring<Row>,2> completed;
std::array<std::atomic<long long>,2> next_sample{};
std::atomic<uint64_t> attempted{},throttled{},published{};
std::mutex mutex;
struct Observe;
thread_local Observe* current{};
struct ChildScope;
thread_local ChildScope* current_child{};
std::atomic<uint64_t> child_sequence{};
struct Observe {
    Observe* previous=current;Row row{};gears_ui_scene_batch::Inventory before{};bool active{},finished{};
    gears_ui::Config const* config{};
    bool hud_canvas(uintptr_t& canvas,uintptr_t& fcanvas){
        if(!config||config->role_canvas_class>=config->identity_count)return false;
        gears_ui::Reader<decltype(&safe_copy)> read{safe_copy};uintptr_t cls{};
        return read.field(canvas,row.owner,config->hud_canvas)&&canvas&&gears_ui::dynamic_object(*config,read,canvas,cls)&&
            gears_ui::live_derives(*config,read,cls,config->identities[config->role_canvas_class].address)&&
            read.field(fcanvas,canvas,0x9c)&&fcanvas;
    }
    Observe(unsigned kind,uint64_t token,uintptr_t owner,uintptr_t function,uintptr_t canvas,uintptr_t fcanvas,uintptr_t frame,
        gears_ui::Config const* frozen=nullptr):config(frozen){
        if(kind<1||kind>2||!ui_observer_enabled||!recording.load())return;
        const auto timestamp=now();auto& timer=next_sample[kind-1];auto due=timer.load();
        if(timestamp<due||!timer.compare_exchange_strong(due,timestamp+qpc_frequency)){++throttled;return;}
        ++attempted;active=true;current=this;row.kind=kind;row.token=token;row.owner=owner;row.function=function;
        row.canvas=canvas;row.fcanvas=fcanvas;row.root_frame=frame;row.thread=GetCurrentThreadId();row.enter=timestamp;
        if(kind==1&&!hud_canvas(row.canvas,row.fcanvas))return;
        row.before_ok=row.fcanvas&&gears_ui_scene_batch::inventory({base},safe_copy,row.fcanvas,before);
    }
    void finish(bool root_current){
        if(!active||finished)return;finished=true;row.exit=now();row.returned=true;
        if(row.kind==1){uintptr_t canvas{},fcanvas{};
            root_current=root_current&&hud_canvas(canvas,fcanvas)&&canvas==row.canvas&&fcanvas==row.fcanvas;}
        row.root_current=root_current;
        if(row.before_ok&&root_current&&!row.simple_executes&&!row.material_executes){
            row.delta_ok=gears_ui_scene_batch::delta({base},safe_copy,before,row.fcanvas,row.delta);
            row.bounds_ok=row.delta_ok&&gears_ui_logical_group::bounds(row.delta,row.bounds);
        }
        std::lock_guard<std::mutex> lock(mutex);completed[row.kind-1].append(std::move(row));++published;
    }
    bool observing()const{return active;}
    ~Observe(){
        if(active){current=previous;
            if(!finished){row.exit=now();std::lock_guard<std::mutex> lock(mutex);
                completed[row.kind-1].append(std::move(row));++published;}}
    }
    Observe(Observe const&)=delete;Observe& operator=(Observe const&)=delete;
};
void executing(unsigned kind){for(auto* p=current;p;p=p->previous)if(p->active){
    if(kind==1)++p->row.simple_executes;else if(kind==2)++p->row.material_executes;}}
void canvas_rejected(){for(auto* p=current;p;p=p->previous)if(p->active)++p->row.unclassified_canvas;}
bool boundary_candidate(){for(auto* p=current;p;p=p->previous)if(p->active&&p->row.kind==1)return true;return false;}
struct ChildScope {
    ChildScope* previous=current_child;Observe* owner{};unsigned index{};
    gears_ui::Identity callee_identity{};
    ChildScope(void* self,void* caller,void* callee,bool parent_current){
        if(!parent_current)return;
        for(auto* p=current;p;p=p->previous)if(p->active&&p->row.kind==1&&p->config&&p->row.owner==reinterpret_cast<uintptr_t>(self)){owner=p;break;}
        if(!owner)return;auto const& c=*owner->config;Row::Node node{};
        const auto address=reinterpret_cast<uintptr_t>(callee);
        // Only live script functions owned by the already authenticated HUD.
        // These names are observed identity fields, not guessed entry addresses.
        gears_ui::Reader<decltype(&safe_copy)> read{safe_copy};gears_ui::Registry script{};uintptr_t entry{};
        gears_ui_logical_group::Call ancestry{};
        if(!gears_ui_logical_group::identity(c,safe_copy,address,node.node)||
           !gears_ui_logical_group::identity(c,safe_copy,node.node.cls,node.cls)||
           !gears_ui_logical_group::identity(c,safe_copy,node.node.outer,node.outer)||
           std::wcscmp(node.cls.name.data(),L"Function")||std::wcscmp(node.outer.name.data(),L"WarHUD")||
           !read.field(script,address,0x8c)||!script.data||script.count<=0||script.count>script.capacity||script.capacity>131072||
           !read.field(entry,address,0x114)||entry!=base+gears_ui::binding_rvas[0]||
           !gears_ui_logical_group::ancestry(safe_copy,reinterpret_cast<uintptr_t>(caller),owner->row.root_frame,ancestry)){
            owner=nullptr;return;}
        auto& row=owner->row;if(row.child_count==row.children.size()){++row.child_overflow;owner=nullptr;return;}
        index=row.child_count++;auto& child=row.children[index];child.id=++child_sequence;
        child.parent=previous&&previous->owner==owner?previous->owner->row.children[previous->index].id:0;
        child.self=reinterpret_cast<uintptr_t>(self);child.callee=address;child.caller=reinterpret_cast<uintptr_t>(caller);
        child.script=script.data;child.script_size=unsigned(script.count);callee_identity=node.node;
        child.enter=now();child.name=node.node.name;current_child=this;
    }
    template<class Original> uintptr_t invoke(Original original){const auto value=original();
        if(owner){auto& child=owner->row.children[index];child.returned=true;
            gears_ui::Identity after{};gears_ui::Registry script{};uintptr_t entry{};
            gears_ui::Reader<decltype(&safe_copy)> read{safe_copy};
            child.current_after=gears_ui_logical_group::identity(*owner->config,safe_copy,child.callee,after)&&
                gears_anchor::equal_identity(callee_identity,after)&&read.field(script,child.callee,0x8c)&&
                script.data==child.script&&script.count==int(child.script_size)&&script.count<=script.capacity&&script.capacity<=131072&&
                read.field(entry,child.callee,0x114)&&entry==base+gears_ui::binding_rvas[0];}
        return value;}
    ~ChildScope(){if(owner){owner->row.children[index].exit=now();current_child=previous;}}
};
// Called only following the pre-existing authenticated Canvas ancestry match.
void canvas(unsigned draw,void* canvas,void* frame,gears_ui::ScopeContext const& selected,uintptr_t matched_frame){
    for(auto* p=current;p;p=p->previous)if(p->active){auto& row=p->row;
        if(row.call_count==row.calls.size()){++row.call_overflow;continue;}
        auto& call=row.calls[row.call_count++];call.draw=draw;call.scope=selected.scope_index;
        call.canvas=reinterpret_cast<uintptr_t>(canvas);call.frame=reinterpret_cast<uintptr_t>(frame);
        call.function=selected.function;call.matched_frame=matched_frame;
        if(row.root_frame)gears_ui_logical_group::ancestry(safe_copy,call.frame,row.root_frame,call);
        if(call.ancestry_complete)for(auto* child=current_child;child;child=child->previous)if(child->owner==p){
            auto& proof=child->owner->row.children[child->index];
            bool body=false;for(unsigned i=0;i<call.count;++i){auto const& f=call.frames[i].fields;
                if(f.node==proof.callee&&f.object==proof.self&&f.previous==proof.caller&&
                   f.code>=proof.script&&f.code-proof.script<proof.script_size){body=true;break;}}
            if(body&&call.logical_count<call.logical_ids.size()){call.logical_ids[call.logical_count++]=proof.id;++proof.body_calls;}}
        if(p->config&&call.ancestry_complete)for(unsigned i=0;i<call.count;++i){const auto address=call.frames[i].fields.node;
            bool known=false;for(unsigned j=0;j<row.node_count;++j)if(row.nodes[j].node.address==address){known=true;break;}
            if(known)continue;if(row.node_count==row.nodes.size()){++row.node_overflow;continue;}
            Row::Node node{};
            if(!gears_ui_logical_group::identity(*p->config,safe_copy,address,node.node)||
               !gears_ui_logical_group::identity(*p->config,safe_copy,node.node.cls,node.cls)||
               !gears_ui_logical_group::identity(*p->config,safe_copy,node.node.outer,node.outer)){
                ++row.node_rejected;continue;}
            row.nodes[row.node_count++]=node;
        }
    }
}
// Pins the exact trace slot before the original may enter nested Canvas calls.
// Existing typed append receipts supply membership, with no extra vertex reads.
struct CallMark {
    struct Slot {Observe* owner{};unsigned index{};};
    std::array<Slot,12> slots{};unsigned count{};
    CallMark(void* frame,unsigned draw,bool accepted=true){if(!accepted)return;
        for(auto* p=current;p&&count<slots.size();p=p->previous)if(p->active&&p->row.call_count){
        const unsigned index=p->row.call_count-1;auto const& call=p->row.calls[index];
        if(call.frame==reinterpret_cast<uintptr_t>(frame)&&call.draw==draw)slots[count++]={p,index};}}
    bool observing()const{return count!=0;}
    void member(gears_ui_logical_group::Member ref){for(unsigned i=0;i<count;++i){auto& call=slots[i].owner->row.calls[slots[i].index];
        if(call.member_count==call.members.size()){call.append_complete=false;continue;}call.members[call.member_count++]=ref;}}
    void complete(){for(unsigned i=0;i<count;++i){auto& call=slots[i].owner->row.calls[slots[i].index];call.append_complete=true;}}
    void tile(gears_ui_batch::Receipt const& receipt){complete();
        member({receipt.fcanvas,receipt.item,receipt.payload,receipt.sort_key,receipt.begin,receipt.end,false});}
    template<class Delta> void text(Delta const& delta){
        if(delta.ranges.size()>gears_ui_logical_group::member_limit)return;complete();
        for(auto const& r:delta.ranges)member({delta.fcanvas,r.item,r.payload,delta.sort_key,r.begin,r.end,false});}
    void material(gears_ui_material::Receipt const& receipt){complete();member({receipt.fcanvas,receipt.item,receipt.payload,receipt.key,0,0,true});}
};
void reset_tls(){current=nullptr;current_child=nullptr;}
template<class Bytes> void hex(FILE* status,Bytes const& bytes){for(auto b:bytes)fprintf(status,"%02x",unsigned(b));}
void name(FILE* status,gears_ui::Identity const& id){for(auto ch:id.name){if(!ch)break;fprintf(status,"%04x",unsigned(ch));}}
void print(FILE* status){
    std::lock_guard<std::mutex> lock(mutex);
    fprintf(status,"ui_logical_group compiled=1 observation_only=1 module_base=%p attempted=%llu throttled=%llu published=%llu rows_per_kind=8 interval_ms=1000 kind1_authenticated_postrender=1 kind2_structural_pathchoice=1 flush_coverage_proven=0 gpu_receipts_published=0 coordinates_modified=0\n",reinterpret_cast<void*>(base),attempted.load(),throttled.load(),published.load());
    for(unsigned kind=0;kind<2;++kind){auto const& sample_ring=completed[kind];
        fprintf(status,"ui_logical_group_ring kind=%u rows=%u overwritten=%llu\n",kind+1,sample_ring.count,sample_ring.overwritten);
        for(unsigned i=0;i<sample_ring.count;++i){auto const& r=sample_ring.oldest(i);
            fprintf(status,"ui_logical_group_row kind=%u row=%u token=%llu owner=%p function=%p canvas=%p fcanvas=%p root_frame=%p thread=%u enter=%lld exit=%lld before_ok=%u returned=%u root_current=%u delta_ok=%u bounds_ok=%u calls=%u call_overflow=%u unclassified_canvas=%u simple_executes=%u material_executes=%u simple_ranges=%zu materials=%zu left=%.17g top=%.17g right=%.17g bottom=%.17g semantic_pickup_proven=0 gpu_clip_proven=0\n",
                kind+1,i,r.token,reinterpret_cast<void*>(r.owner),reinterpret_cast<void*>(r.function),reinterpret_cast<void*>(r.canvas),reinterpret_cast<void*>(r.fcanvas),reinterpret_cast<void*>(r.root_frame),r.thread,r.enter,r.exit,r.before_ok?1u:0u,r.returned?1u:0u,r.root_current?1u:0u,r.delta_ok?1u:0u,r.bounds_ok?1u:0u,r.call_count,r.call_overflow,r.unclassified_canvas,r.simple_executes,r.material_executes,r.delta.simple.size(),r.delta.material.size(),r.bounds.left,r.bounds.top,r.bounds.right,r.bounds.bottom);
            fprintf(status,"ui_logical_group_names kind=%u row=%u nodes=%u rejected=%u overflow=%u unclassified_canvas=%u identity_slots_verified=1 names_are_not_hook_authorization=1\n",kind+1,i,r.node_count,r.node_rejected,r.node_overflow,r.unclassified_canvas);
            fprintf(status,"ui_logical_group_children kind=%u row=%u count=%u overflow=%u unique_tokens_only_at_existing_call_boundary=1\n",kind+1,i,r.child_count,r.child_overflow);
            for(unsigned j=0;j<r.child_count;++j){auto const& c=r.children[j];
                fprintf(status,"ui_logical_group_child kind=%u row=%u index=%u id=%llu parent=%llu self=%p callee=%p caller=%p script=%p script_size=%u enter=%lld exit=%lld returned=%u current_after=%u body_calls=%u name_utf16=",kind+1,i,j,c.id,c.parent,reinterpret_cast<void*>(c.self),reinterpret_cast<void*>(c.callee),reinterpret_cast<void*>(c.caller),reinterpret_cast<void*>(c.script),c.script_size,c.enter,c.exit,c.returned?1u:0u,c.current_after?1u:0u,c.body_calls);
                for(auto ch:c.name){if(!ch)break;fprintf(status,"%04x",unsigned(ch));}fprintf(status,"\n");}
            for(unsigned j=0;j<r.node_count;++j){auto const& n=r.nodes[j];
                fprintf(status,"ui_logical_group_node kind=%u row=%u index=%u node=%p object_index=%u name_index=%u number=%u cls=%p outer=%p name_utf16=",kind+1,i,j,reinterpret_cast<void*>(n.node.address),n.node.index,n.node.name_index,n.node.number,reinterpret_cast<void*>(n.node.cls),reinterpret_cast<void*>(n.node.outer));name(status,n.node);fprintf(status," class_utf16=");name(status,n.cls);fprintf(status," outer_utf16=");name(status,n.outer);fprintf(status,"\n");}
            for(unsigned j=0;j<r.call_count;++j){auto const& c=r.calls[j];
                fprintf(status,"ui_logical_group_call kind=%u row=%u call=%u draw=%u scope=%u canvas=%p frame=%p function=%p matched_frame=%p ancestry_complete=%u frames=%u append_complete=%u members=%u mapped_to_root_delta=%u\n",kind+1,i,j,c.draw,c.scope,reinterpret_cast<void*>(c.canvas),reinterpret_cast<void*>(c.frame),reinterpret_cast<void*>(c.function),reinterpret_cast<void*>(c.matched_frame),c.ancestry_complete?1u:0u,c.count,c.append_complete?1u:0u,c.member_count,r.delta_ok&&gears_ui_logical_group::mapped(r.delta,c)?1u:0u);
                for(unsigned k=0;k<c.logical_count;++k)fprintf(status,"ui_logical_group_call_child kind=%u row=%u call=%u depth=%u id=%llu\n",kind+1,i,j,k,c.logical_ids[k]);
                for(unsigned k=0;k<c.count;++k){auto const& f=c.frames[k];
                    fprintf(status,"ui_logical_group_frame kind=%u row=%u call=%u depth=%u frame=%p node=%p object=%p code=%p previous=%p\n",kind+1,i,j,k,reinterpret_cast<void*>(f.address),reinterpret_cast<void*>(f.fields.node),reinterpret_cast<void*>(f.fields.object),reinterpret_cast<void*>(f.fields.code),reinterpret_cast<void*>(f.fields.previous));}
                for(unsigned k=0;k<c.member_count;++k){auto const& m=c.members[k];
                    fprintf(status,"ui_logical_group_member kind=%u row=%u call=%u member=%u material=%u fcanvas=%p key=%d item=%p payload=%p begin=%u end=%u\n",kind+1,i,j,k,m.material?1u:0u,reinterpret_cast<void*>(m.fcanvas),m.key,reinterpret_cast<void*>(m.item),reinterpret_cast<void*>(m.payload),m.begin,m.end);}}
            for(unsigned j=0;j<r.delta.simple.size();++j){auto const& s=r.delta.simple[j];
                fprintf(status,"ui_logical_group_simple kind=%u row=%u range=%u key=%d item=%p payload=%p begin=%u end=%u vertices=%zu transform80=",kind+1,i,j,s.key,reinterpret_cast<void*>(s.item),reinterpret_cast<void*>(s.payload),s.begin,s.end,s.vertices.size());hex(status,s.transform);fprintf(status,"\n");
                for(unsigned v=0;v<s.vertices.size();++v){fprintf(status,"ui_logical_group_vertex kind=%u row=%u range=%u vertex=%u bytes44=",kind+1,i,j,v);hex(status,s.vertices[v]);fprintf(status,"\n");}}
            for(unsigned j=0;j<r.delta.material.size();++j){auto const& m=r.delta.material[j];
                fprintf(status,"ui_logical_group_material kind=%u row=%u range=%u key=%d item=%p payload=%p proxy=%p transform80=",kind+1,i,j,m.key,reinterpret_cast<void*>(m.item),reinterpret_cast<void*>(m.payload),reinterpret_cast<void*>(m.proxy));hex(status,m.transform);fprintf(status," record36=");hex(status,m.record);fprintf(status,"\n");}
        }
    }
}
#else
struct Observe {Observe(unsigned,uint64_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,gears_ui::Config const* =nullptr){}void finish(bool){}bool observing()const{return false;}};
struct CallMark {CallMark(void*,unsigned,bool=true){}bool observing()const{return false;}void tile(gears_ui_batch::Receipt const&){}
    template<class Delta> void text(Delta const&){}void material(gears_ui_material::Receipt const&){} };
void executing(unsigned){}
void canvas_rejected(){}
bool boundary_candidate(){return false;}
struct ChildScope {ChildScope(void*,void*,void*,bool){}template<class Original> uintptr_t invoke(Original original){return original();}};
void canvas(unsigned,void*,void*,gears_ui::ScopeContext const&,uintptr_t){}
void reset_tls(){}
void print(FILE* status){fprintf(status,"ui_logical_group compiled=0 observation_only=1\n");}
#endif
}
