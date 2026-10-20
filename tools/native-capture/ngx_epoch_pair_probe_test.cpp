#include "ngx_epoch_pair_probe.h"
#include <stdexcept>
#include <memory>
using namespace gears_ngx_epoch;
using Microsoft::WRL::ComPtr;
static unsigned checks{};
static void check(bool value){++checks;if(!value)throw std::runtime_error("epoch probe check failed");}
static void hr(HRESULT value){check(SUCCEEDED(value));}
static Geometry geo(){Geometry g;g.world=2;g.ticket=10;g.renderer=100;g.valid=31;g.constructor_valid=3;g.views={1000,2000};g.extent={2133,720};g.buffer={2560,1080};g.source={{{0,0,853,720},{1280,0,853,720}}};g.target={{{0,0,853,720},{1280,0,853,720}}};return g;}
static void arm(Probe& p){p.created(1,2,300,400,{1706,720,2560,1080,1,0x12,1});p.built(geo(),true);p.prepared(1,geo());p.prepared(2,geo());}
int main(){try{
    ComPtr<ID3D12Device> device;hr(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    std::array<ComPtr<ID3D12Resource>,4> resources;std::array<uintptr_t,4> raw{};
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=2560;d.Height=1080;d.MipLevels=d.DepthOrArraySize=1;d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R11G11B10_FLOAT;
    for(unsigned i=0;i<4;++i){if(i==2)d.Format=DXGI_FORMAT_R32_FLOAT;if(i==3)d.Format=DXGI_FORMAT_R16G16_FLOAT;
        hr(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resources[i])));raw[i]=reinterpret_cast<uintptr_t>(resources[i].Get());}
    auto p_owner=std::make_unique<Probe>();auto& p=*p_owner;EvalInput ignored;check(p.enter(ignored,false)==-1);p.built(geo(),true);check(!p.selected&&!p.resource_count);
    p.created(1,2,300,400,{1706,720,2560,1080,1,0x12,1});check(p.enter(ignored,false)==-1&&p.skipped==1);
    auto wrong=geo();wrong.world=3;p.built(wrong,true);check(!p.selected);p.built(geo(),true);p.prepared(1,geo());p.prepared(2,geo());check(p.selected&&p.ticket==10);
    for(unsigned i=0;i<337;++i){List other{100000+i,200000+i,300000+i};auto next=other;++next.generation;p.reset(other,next,true);}
    for(unsigned i=0;i<205;++i){List other{100000+i,200000+i,300000+i};auto order=p.execute_begin(80,&other,1);p.execute_end(order,80,&other,&other,1);}
    check(p.event_count==0&&!p.finished&&p.excluded_resets==337&&p.excluded_execute_members==205&&p.execution_order==205);
    const List eval_lists[]{{11,21,31},{12,22,32}};const List copy_lists[]{{13,23,33},{14,24,34}};
    for(unsigned eye=0;eye<2;++eye){
        const auto before=p.serial;
        for(unsigned i=0;i<4;++i){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=resources[i].Get();b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateAfter=i==1?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;p.barrier(eval_lists[eye],b,50);}
        p.barrier_return(before,p.serial,eval_lists[eye]);check(p.resource_count==4&&p.mask(raw)==15&&p.events[p.event_count-1].returned);
        EvalInput in;in.world=2;in.ticket=10;in.renderer=100;in.scope=10000+eye;in.handle=300;in.parameters=400;in.command=eval_lists[eye];in.resources=raw;in.values[0]=1280;in.values[1]=1080;in.values[7]=1280*eye;in.geometry=geo();in.thread=9;in.associated=in.readable=true;
        check(p.enter(in,false)==int(eye));const int index=p.dispatch(in.command.raw,300,400,0,9);check(index==int(eye));p.dispatched(index,1);p.leave(in.scope,true,1);
        auto order=p.execute_begin(80,&eval_lists[eye],1);p.execute_end(order,80,&eval_lists[eye],&eval_lists[eye],1);
        D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=resources[1].Get();dst.pResource=resources[0].Get();src.Type=dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        check(p.copy(copy_lists[eye],dst,{0,0,0},src,nullptr,0x1c6093)==int(eye));p.copied(int(eye),copy_lists[eye],true);
        order=p.execute_begin(80,&copy_lists[eye],1);p.execute_end(order,80,&copy_lists[eye],&copy_lists[eye],1);
    }
    check(p.finished&&p.stop==1&&p.complete()&&p.eval_count==2&&p.resource_count==4);
    check(p.evals[0].input.command.generation==31&&p.evals[0].copy_list.generation==33&&p.evals[0].eval_order<p.evals[0].copy_order);
    check(p.resources[2].lease.desc.Format==DXGI_FORMAT_R32_FLOAT&&p.resources[3].lease.desc.Format==DXGI_FORMAT_R16G16_FLOAT);
    const auto frozen=p.event_count;p.event({});p.created(2,2,301,400,{1706,720,2560,1080,1,0x12,1});check(p.event_count==frozen&&p.revision==1);
    // A typed resource found only after Eval is reported separately; it cannot
    // make complete_capture pretend all four were retained at the input point.
    p.evals[0].typed_mask=3;check(!p.complete());p.evals[0].typed_mask=15;
    p.evals[0].eval_generation=false;check(!p.complete());p.evals[0].eval_generation=true;
    p.geometry[2].valid=0;check(!p.complete());p.geometry[2].valid=31;
    p.geometry[0].constructor_valid=1;check(!p.complete());p.geometry[0].constructor_valid=3;
    p.evals[0].dispatch_result=0xbad00001;check(!p.complete());p.evals[0].dispatch_result=1;
    p.evals[0].copy_returned=false;check(!p.complete());p.evals[0].copy_returned=true;
    p.evals[0].copies=2;check(!p.complete());p.evals[0].copies=1;
    p.evals[1].input.values[7]=0;check(!p.complete());p.evals[1].input.values[7]=1280;
    p.dropped=1;check(!p.complete());p.dropped=0;check(p.complete());
    p.release_leases();check(p.mask(raw)==0&&!p.complete());
    auto cap_owner=std::make_unique<Probe>();auto& cap=*cap_owner;arm(cap);for(unsigned i=0;i<Probe::event_capacity+20;++i)cap.event({});check(cap.finished&&cap.stop==6&&cap.dropped==1&&cap.event_count==Probe::event_capacity);
    auto malformed_owner=std::make_unique<Probe>();auto& malformed=*malformed_owner;arm(malformed);auto in=p.evals[0].input;in.resources=raw;in.readable=false;check(malformed.enter(in,false)==-1&&malformed.stop==3&&!malformed.complete());
    auto changed_owner=std::make_unique<Probe>();auto& changed=*changed_owner;arm(changed);changed.changed();check(changed.finished&&changed.stop==2);
    auto foreign_owner=std::make_unique<Probe>();auto& foreign=*foreign_owner;arm(foreign);in=p.evals[0].input;in.world=3;check(foreign.enter(in,false)==-1&&foreign.eval_count==0&&!foreign.finished);
    auto drift_owner=std::make_unique<Probe>();auto& drift=*drift_owner;arm(drift);in=p.evals[0].input;in.command=eval_lists[0];check(drift.enter(in,false)==0);check(drift.dispatch(11,300,400,0,10)==0&&!drift.evals[0].dispatch_match);
    drift.leave(in.scope,true,1);List next=eval_lists[0];++next.generation;auto order=drift.execute_begin(80,&eval_lists[0],1);drift.execute_end(order,80,&eval_lists[0],&next,1);check(!drift.evals[0].eval_generation);
    auto aliases_owner=std::make_unique<Probe>();auto& aliases=*aliases_owner;arm(aliases);D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_ALIASING;b.Aliasing.pResourceBefore=resources[0].Get();aliases.barrier(eval_lists[0],b,50);check(aliases.unknown==1&&aliases.resource_count==1);
    check(aliases.events[0].kind==Kind::alias&&!aliases.events[0].other&&!aliases.events[0].returned);aliases.reset(eval_lists[0],next,false);check(aliases.events[1].kind==Kind::reset&&!aliases.events[1].returned);
    auto rearm_owner=std::make_unique<Probe>();auto& rearm=*rearm_owner;
    const std::array<uint32_t,7> fields{1706,720,2560,1080,1,0x12,1};
    rearm.created(1,2,300,400,fields);rearm.changed(1);rearm.created(2,2,301,400,fields);
    check(rearm.empty_epoch_count==1&&rearm.empty_epochs[0].revision==1&&rearm.empty_epochs[0].world==2&&rearm.empty_epochs[0].reason==1&&rearm.empty_epochs[0].fields==fields);
    rearm.changed(2);rearm.created(3,3,302,400,fields);check(rearm.empty_epoch_count==2&&rearm.empty_epochs[1].reason==2&&rearm.world==3);
    rearm.created(4,3,303,400,fields);check(rearm.empty_epoch_count==3&&rearm.empty_epochs[2].revision==3&&rearm.empty_epochs[2].reason==3&&!rearm.finished);
    rearm.created(5,3,304,400,fields);check(rearm.finished&&rearm.stop==8&&rearm.revision==4&&rearm.handle==303&&rearm.empty_epoch_count==3);
    rearm.created(6,3,305,400,fields);check(rearm.revision==4&&rearm.empty_epochs[0].revision==1);
    for(unsigned evidence=0;evidence<5;++evidence){rearm.clear();rearm.created(1,2,300,400,fields);
        if(evidence==0)rearm.built(geo(),true);else if(evidence==1)rearm.resource_count=1;else if(evidence==2)rearm.eval_count=1;else if(evidence==3)rearm.event_count=1;else rearm.execution_order=1;
        rearm.created(2,2,301,400,fields);check(rearm.finished&&rearm.stop==2&&rearm.revision==1&&!rearm.empty_epoch_count);
    }
    // A mixed batch preserves the original member/count/order. Unknown/reset-only
    // lists remain excluded; unknown UAV/alias barriers still seed observations.
    auto scoped_owner=std::make_unique<Probe>();auto& scoped=*scoped_owner;arm(scoped);
    List member[]{ {40,50,60},{41,51,61},{42,52,62} };
    D3D12_RESOURCE_BARRIER wildcard{};wildcard.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
    scoped.barrier(member[1],wildcard,70);check(scoped.unknown==1&&scoped.event_count==1);
    order=scoped.execute_begin(80,member,3);check(scoped.event_count==2&&scoped.excluded_execute_members==2&&scoped.events[1].member==1&&scoped.events[1].count==3);
    // Late enrollment during the API cannot retroactively acquire membership.
    scoped.barrier(member[0],wildcard,70);scoped.execute_end(order,80,member,member,3);
    check(scoped.event_count==4&&scoped.events[3].kind==Kind::execute_return&&scoped.events[3].member==1&&scoped.events[3].order==order);
    auto count=scoped.event_count;List unseen{60,70,80},unseen_next=unseen;++unseen_next.generation;scoped.reset(unseen,unseen_next,true);
    check(scoped.event_count==count&&!scoped.watches(unseen_next));
    auto changed_list=member[1];++changed_list.generation;scoped.reset(member[1],changed_list,true);
    check(scoped.events[scoped.event_count-1].kind==Kind::reset&&!scoped.watches(changed_list));
    // Wrong pre-execute generation is retained for a pending Eval and rejected;
    // an exact duplicate generation remains visible after first submission.
    auto pending_owner=std::make_unique<Probe>();auto& pending=*pending_owner;arm(pending);in=p.evals[0].input;
    check(pending.enter(in,false)==0);pending.leave(in.scope,true,1);auto wrong_generation=in.command;++wrong_generation.generation;
    count=pending.event_count;pending.reset(wrong_generation,wrong_generation,false);check(pending.event_count==count+1&&!pending.events[count].returned);
    order=pending.execute_begin(80,&wrong_generation,1);pending.execute_end(order,80,&wrong_generation,&wrong_generation,1);
    check(pending.evals[0].eval_executes==1&&!pending.evals[0].eval_generation);
    pending.clear();arm(pending);check(pending.enter(in,false)==0);pending.leave(in.scope,true,1);
    for(unsigned i=0;i<2;++i){order=pending.execute_begin(80,&in.command,1);pending.execute_end(order,80,&in.command,&in.command,1);}
    check(pending.evals[0].eval_executes==2&&!pending.evals[0].eval_generation);
    // Copy-only recordings seed relevance independently and preserve pending
    // wrong-generation submission, reset, and exact duplicate diagnostics.
    D3D12_TEXTURE_COPY_LOCATION copy_src{},copy_dst{};copy_src.pResource=resources[1].Get();copy_dst.pResource=resources[0].Get();
    copy_src.Type=copy_dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    for(unsigned duplicate=0;duplicate<2;++duplicate){
        pending.clear();arm(pending);check(pending.enter(in,false)==0);pending.leave(in.scope,true,1);
        check(pending.copy(copy_lists[0],copy_dst,{0,0,0},copy_src,nullptr,0x1c6093)==0);pending.copied(0,copy_lists[0],true);
        auto submitted=copy_lists[0];if(!duplicate)++submitted.generation;
        count=pending.event_count;pending.reset(submitted,submitted,false);check(pending.event_count==count+1&&!pending.events[count].returned);
        for(unsigned i=0;i<1+duplicate;++i){order=pending.execute_begin(80,&submitted,1);pending.execute_end(order,80,&submitted,&submitted,1);}
        check(pending.evals[0].copy_executes==1+duplicate&&!pending.evals[0].copy_generation);
    }
    // Relevant, unknown barriers still exhaust the unchanged bound, not a ring.
    scoped.clear();arm(scoped);for(unsigned i=0;i<Probe::event_capacity+1;++i)scoped.barrier(member[0],wildcard,70);
    check(scoped.finished&&scoped.stop==6&&scoped.event_count==1024&&scoped.dropped==1);
    check(malformed.rejection.present&&(malformed.rejection.mask&Probe::unreadable)&&malformed.rejection.input.scope==in.scope);
    std::printf("NGX epoch pair probe tests passed: %u checks (real typed resources, synthetic event sequence)\n",checks);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"%s (%u)\n",e.what(),checks);return 1;}}
