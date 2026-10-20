// Included in capture.cpp's private namespace. This opt-in experiment changes
// only one final quad's source u; it does not create another hook.
namespace gears_quad_source {
enum Phase : unsigned { a1, b, a2, expired, waiting, phase_count };
enum Reject : unsigned { accepted, disabled, scope, caller, view, rectangle, dimensions, clock, configuration, reject_count };
enum Source : unsigned { left_source, right_source };
constexpr const char* phase_names[]={"A1","B","A2","expired","waiting"};
constexpr const wchar_t* event_names[]={L"A1",L"B",L"A2"};
struct Args {std::array<float,8> rectangle{};std::array<unsigned,4> dimensions{};};
struct Gate {
    bool enabled{},recording{},diagnostics{},trial{},renderer{};
    uintptr_t caller{},views{},current{};int count{},capacity{};
    unsigned source{left_source};
};
struct Decision {Args forwarded{};unsigned phase{waiting},eye{2},rejection{disabled};bool modified{};};
unsigned phase_at(long long origin,long long tick,long long frequency){
    if(!origin)return waiting;
    if(frequency<=0||tick<origin)return expired;
    // Compare elapsed seconds without multiplying a caller-supplied frequency.
    const auto seconds=(tick-origin)/frequency;
    return seconds<3?a1:seconds<6?b:seconds<10?a2:expired;
}
Decision decide(const Args& args,const Gate& gate,long long tick,long long frequency,long long& origin){
    Decision result;result.forwarded=args;result.phase=phase_at(origin,tick,frequency);
    if(!gate.enabled)return result;
    result.rejection=configuration;if(gate.source>right_source)return result;
    result.rejection=scope;
    if(!gate.recording||!gate.diagnostics||!gate.trial||!gate.renderer)return result;
    result.rejection=caller;if(gate.caller!=0xd83b1c)return result;
    result.rejection=view;
    if(!gate.views||gate.views%8||gate.count!=2||gate.capacity<2||gate.capacity>1024||
       gate.current<gate.views||gate.current-gate.views>0xd30||(gate.current-gate.views)%0xd30)return result;
    result.eye=unsigned((gate.current-gate.views)/0xd30);
    const float x=result.eye?960.f:0.f;
    result.rejection=rectangle;
    if(args.rectangle!=std::array<float,8>{x,0,960,1080,x,0,960,1080})return result;
    result.rejection=dimensions;
    if(args.dimensions!=std::array<unsigned,4>{1920,1080,1920,1080})return result;
    result.rejection=clock;if(tick<=0||frequency<=0||(origin&&tick<origin))return result;
    if(!origin&&result.eye==1)origin=tick;
    result.phase=phase_at(origin,tick,frequency);result.rejection=accepted;
    const unsigned modified_eye=gate.source==left_source?1:0;
    if(result.eye==modified_eye&&result.phase==b){
        result.forwarded.rectangle[4]=gate.source==left_source?0.f:960.f;result.modified=true;
    }
    return result;
}
bool self_test(){
    Args right{{960,0,960,1080,960,0,960,1080},{1920,1080,1920,1080}};
    auto left=right;left.rectangle[0]=left.rectangle[4]=0;
    constexpr long long frequency=1000,start=10000;
    const long long ticks[]={start+2999,start+3000,start+5999,start+6000,start+9999,start+10000};
    const unsigned phases[]={a1,b,b,a2,a2,expired};
    for(unsigned source=left_source;source<=right_source;++source){
        const Gate right_gate{true,true,true,true,true,0xd83b1c,0x1000,0x1d30,2,2,source};
        auto left_gate=right_gate;left_gate.current=0x1000;
        const auto& subject=source==left_source?right:left;
        const auto& valid=source==left_source?right_gate:left_gate;
        const auto& other=source==left_source?left:right;
        const auto& other_gate=source==left_source?left_gate:right_gate;
        const float replacement=source==left_source?0.f:960.f;
        long long origin{};
        // In both modes the left call before the first right quad cannot start
        // the timer or mutate. Events and timing retain their original meaning.
        auto result=decide(left,left_gate,start,frequency,origin);
        if(origin||result.phase!=waiting||result.modified||result.rejection)return false;
        result=decide(right,right_gate,start,frequency,origin);
        if(origin!=start||result.phase!=a1||result.modified||result.rejection||result.eye!=1)return false;
        for(unsigned i=0;i<6;++i){
            result=decide(subject,valid,ticks[i],frequency,origin);auto expected=subject;
            if(phases[i]==b)expected.rectangle[4]=replacement;
            if(result.phase!=phases[i]||result.rejection||result.modified!=(phases[i]==b)||
               result.forwarded.rectangle!=expected.rectangle||result.forwarded.dimensions!=subject.dimensions)return false;
        }
        result=decide(other,other_gate,start+4000,frequency,origin);
        if(result.modified||result.rejection||result.forwarded.rectangle!=other.rectangle||
           result.forwarded.dimensions!=other.dimensions)return false;
        // Every independent guard must preserve all twelve arguments during B.
        for(unsigned i=0;i<12;++i){auto gate=valid;long long fresh{};
            switch(i){case 0:gate.enabled=false;break;case 1:gate.recording=false;break;
            case 2:gate.diagnostics=false;break;case 3:gate.trial=false;break;case 4:gate.renderer=false;break;
            case 5:++gate.caller;break;case 6:gate.views=0;break;case 7:++gate.current;break;
            case 8:gate.count=1;break;case 9:gate.capacity=1;break;case 10:gate.current=0x2a60;break;
            case 11:gate.source=2;break;}
            for(unsigned started=0;started<2;++started){fresh=started?start:0;
                result=decide(subject,gate,start+4000,frequency,fresh);
                if(!result.rejection||result.modified||fresh!=(started?start:0)||
                   result.forwarded.rectangle!=subject.rectangle||result.forwarded.dimensions!=subject.dimensions)return false;
            }
        }
        for(unsigned i=0;i<12;++i){auto args=subject;
            if(i<8)args.rectangle[i]+=.25f;else ++args.dimensions[i-8];
            result=decide(args,valid,start+4000,frequency,origin);
            if(!result.rejection||result.modified||origin!=start||result.forwarded.rectangle!=args.rectangle||
               result.forwarded.dimensions!=args.dimensions)return false;
        }
        result=decide(subject,valid,start-1,frequency,origin);
        if(result.rejection!=clock||result.modified)return false;
        result=decide(subject,valid,start+4000,0,origin);
        if(result.rejection!=clock||result.modified||origin!=start)return false;
    }
    return true;
}
}

bool quad_left_source_control{},quad_right_source_control{};
bool quad_source_control_enabled(){return quad_left_source_control||quad_right_source_control;}
std::mutex quad_source_mutex;
long long quad_source_origin{};
struct QuadSourceTrace {
    uint64_t calls{},modified{},rejected{};long long first{},last{};
    float original_u{},effective_u{};
};
std::array<std::array<QuadSourceTrace,3>,gears_quad_source::phase_count> quad_source_traces{};
std::array<uint64_t,gears_quad_source::reject_count> quad_source_rejections{};
std::array<HANDLE,3> quad_source_events{};
unsigned quad_source_event_phase{gears_quad_source::waiting};

void clear_quad_source_events_locked(){
    for(auto event:quad_source_events)if(event)ResetEvent(event);
    quad_source_event_phase=gears_quad_source::waiting;
}
bool create_quad_source_events(){
    if(!quad_source_control_enabled())return true;
    for(unsigned i=0;i<quad_source_events.size();++i){wchar_t name[128];
        swprintf_s(name,L"Local\\GearsVR.Capture.QuadSource.%lu.%ls",GetCurrentProcessId(),gears_quad_source::event_names[i]);
        quad_source_events[i]=CreateEventW(nullptr,TRUE,FALSE,name);
        if(!quad_source_events[i]){for(auto& event:quad_source_events)if(event){CloseHandle(event);event=nullptr;}return false;}
        ResetEvent(quad_source_events[i]);
    }
    return true;
}
void close_quad_source_events(){
    std::lock_guard<std::mutex> lock(quad_source_mutex);clear_quad_source_events_locked();
    for(auto& event:quad_source_events)if(event){CloseHandle(event);event=nullptr;}
}
void refresh_quad_source_events(){
    if(!quad_source_control_enabled())return;
    std::lock_guard<std::mutex> lock(quad_source_mutex);
    if(gears_quad_source::phase_at(quad_source_origin,now(),qpc_frequency)!=quad_source_event_phase)
        clear_quad_source_events_locked();
}
gears_quad_source::Decision prepare_quad_source_control(const gears_quad_source::Args& args,uintptr_t caller){
    using namespace gears_quad_source;
    Gate gate{quad_source_control_enabled(),recording.load(),viewport_diagnostics,trial_draw_scope,consuming_renderer!=nullptr,caller};
    gate.source=quad_right_source_control?right_source:left_source;
    if(gate.enabled&&gate.recording&&gate.diagnostics&&gate.trial&&gate.renderer&&caller==0xd83b1c){
        uintptr_t tls{},context{};struct Header{uintptr_t data;int count,capacity;} header{};
        const bool read=safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
            safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)&&context&&
            safe_copy(&gate.current,reinterpret_cast<void*>(context+0x14c0),8)&&
            safe_copy(&header,reinterpret_cast<char*>(consuming_renderer)+0x68,sizeof(header));
        if(read){gate.views=header.data;gate.count=header.count;gate.capacity=header.capacity;}
    }
    std::lock_guard<std::mutex> lock(quad_source_mutex);
    return decide(args,gate,now(),qpc_frequency,quad_source_origin);
}
void complete_quad_source_control(const gears_quad_source::Args& original,const gears_quad_source::Decision& decision){
    if(!quad_source_control_enabled())return;
    using namespace gears_quad_source;
    std::lock_guard<std::mutex> lock(quad_source_mutex);
    const auto tick=now();auto& trace=quad_source_traces[decision.phase][decision.eye];
    if(!trace.calls){trace.first=tick;trace.original_u=original.rectangle[4];trace.effective_u=decision.forwarded.rectangle[4];}
    trace.last=tick;++trace.calls;trace.modified+=decision.modified;trace.rejected+=decision.rejection!=accepted;
    if(decision.rejection!=accepted)++quad_source_rejections[decision.rejection];
    const auto current=phase_at(quad_source_origin,tick,qpc_frequency);
    if(current!=quad_source_event_phase)clear_quad_source_events_locked();
    // Signals describe completed CPU quad calls, not Present or GPU completion.
    // Set a new phase only after a matching right quad returns inside that phase.
    if(decision.rejection==accepted&&decision.eye==1&&current==decision.phase&&current<3&&
       current!=quad_source_event_phase&&quad_source_events[current]){
        SetEvent(quad_source_events[current]);quad_source_event_phase=current;
    }
}
void print_quad_source_control(FILE* file){
    std::lock_guard<std::mutex> lock(quad_source_mutex);
    fprintf(file,"quad_source_control enabled=%u origin_qpc=%lld frequency=%lld boundaries_seconds=3,6,10 cpu_phase_only=1 mode=%s source=%s modified_eye=%u\n",
        quad_source_control_enabled(),quad_source_origin,qpc_frequency,
        quad_right_source_control?"sample_right_into_left":"sample_left_into_right",quad_right_source_control?"right":"left",quad_right_source_control?0:1);
    for(unsigned phase=0;phase<gears_quad_source::phase_count;++phase)for(unsigned eye=0;eye<3;++eye){
        const auto& t=quad_source_traces[phase][eye];if(!t.calls)continue;
        fprintf(file,"quad_source_phase phase=%s eye=%u calls=%llu modified=%llu rejected=%llu first_qpc=%lld last_qpc=%lld first_original_u=%.9g first_effective_u=%.9g\n",
            gears_quad_source::phase_names[phase],eye,t.calls,t.modified,t.rejected,t.first,t.last,t.original_u,t.effective_u);
    }
    for(unsigned reason=1;reason<quad_source_rejections.size();++reason)if(quad_source_rejections[reason])
        fprintf(file,"quad_source_rejected reason=%u calls=%llu\n",reason,quad_source_rejections[reason]);
}
