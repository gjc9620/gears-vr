// A bounded, same-family horizontal view-union correction. This deliberately
// accepts only integer, unscaled, adjacent source/target rectangles. Height
// padding is opt-in for a certified owned consumer, never a generic fallback.
namespace gears_view_union_mask {
using Snapshot=gears_first_view_mask::Snapshot;
using Gate=gears_first_view_mask::Gate;
enum Reject : unsigned { accepted,disabled,scope,caller,read_failed,vtable,views,dimensions,rectangles,viewport,argument,clock,reject_count };
constexpr const char* reject_names[]={"accepted","disabled","scope","caller","read_failed","vtable","views","dimensions","rectangles","viewport","argument","clock"};
struct Decision {float forwarded{},expected{},union_t{};unsigned rejection{disabled};int union_right{};bool modified{};};
uint32_t bits(float value){uint32_t result{};memcpy(&result,&value,4);return result;}
float boundary(float extent,float width){
    // Match 119bd0 VADDSS, 119bd4 VDIVSS, 119bd8 VSUBSS. Separate
    // intermediates also document why a double-precision algebraic rewrite
    // would be an inappropriate validator for the incoming float ABI.
    const float doubled=extent+extent;const float ratio=doubled/width;return ratio-1.f;
}
Decision decide_union(float value,const Gate& gate,const Snapshot& sample,long long tick,long long frequency,long long& origin,bool owned_height_padding=false){
    Decision result;result.forwarded=value;
    if(!gate.enabled)return result;
    result.rejection=scope;if(!gate.recording||!gate.diagnostics||!gate.trial||!gate.renderer)return result;
    result.rejection=caller;if(gate.caller!=0x119be2)return result;
    result.rejection=read_failed;if(!sample.readable)return result;
    result.rejection=vtable;if(!sample.command_list_valid)return result;
    result.rejection=views;if(!sample.storage||sample.storage%8||sample.count!=2||sample.capacity<2||sample.capacity>1024)return result;
    const auto width=sample.color_dimensions[0],height=sample.color_dimensions[1];
    result.rejection=dimensions;if(width<2||width>16384||height<1||height>16384)return result;
    result.rejection=rectangles;
    for(unsigned eye=0;eye<2;++eye){const auto& target=sample.target[eye];const auto& source=sample.source[eye];
        // The dimension cap guarantees these integers and sums convert to
        // float exactly. Avoid converting untrusted NaN/overflowing floats.
        const auto effective_height=owned_height_padding?sample.target[0][3]:static_cast<int>(height);
        if(target[0]<0||target[1]!=0||target[2]<=0||effective_height<=0||effective_height>static_cast<int>(height)||target[3]!=effective_height||
           int64_t(target[0])+target[2]>width)return result;
        for(unsigned i=0;i<4;++i)if(!std::isfinite(source[i])||source[i]!=static_cast<float>(target[i]))return result;
    }
    if(sample.target[0][0]!=0||sample.target[1][0]!=sample.target[0][2])return result;
    const auto right=int64_t(sample.target[1][0])+sample.target[1][2];
    if(right<=sample.target[0][2]||right>width)return result;
    result.union_right=static_cast<int>(right);
    result.rejection=viewport;
    if(sample.viewport_count!=1||sample.viewport!=std::array<float,6>{0,0,static_cast<float>(width),static_cast<float>(height),0,1})return result;
    result.expected=boundary(sample.source[0][2],static_cast<float>(width));
    result.union_t=boundary(static_cast<float>(right),static_cast<float>(width));
    result.rejection=argument;
    if(bits(value)!=bits(result.expected)||!std::isfinite(result.union_t)||result.union_t<result.expected||result.union_t>1.f)return result;
    result.rejection=clock;if(tick<=0||frequency<=0||(origin&&tick<origin))return result;
    // Production is already bounded by try_submit_pair's deadline. Its
    // Lifetime::close prevents new roots but intentionally keeps queued roots
    // tracked. hook_consume latches trial_draw_scope for the whole consumer.
    // A second wall-clock expiry here could undo one of that family's masks
    // halfway through consumption. QPC is observation, not another deadline.
    if(!origin)origin=tick;
    result.forwarded=result.union_t;result.modified=bits(result.forwarded)!=bits(value);result.rejection=accepted;return result;
}
Snapshot sample_for(unsigned width,unsigned height,int left,int right){
    auto sample=gears_first_view_mask::valid_test_snapshot();sample.color_dimensions={width,height};
    sample.target={std::array<int,4>{0,0,left,static_cast<int>(height)},std::array<int,4>{left,0,right,static_cast<int>(height)}};
    for(unsigned eye=0;eye<2;++eye)for(unsigned i=0;i<4;++i)sample.source[eye][i]=static_cast<float>(sample.target[eye][i]);
    sample.viewport={0,0,static_cast<float>(width),static_cast<float>(height),0,1};return sample;
}
bool self_test(){
    const Gate valid{true,true,true,true,true,0x119be2};long long origin{};
    // Same clock accepts live resolution changes, odd widths, unequal eyes,
    // and partial horizontal union without forcing the effective t to one.
    const std::array<int,4> geometries[]={{1920,1080,960,960},{2560,1440,1280,1280},{1919,1079,959,960},
        {1920,1080,640,960},{1920,1080,1200,720},{2,1,1,1},{16384,16384,8191,8193}};
    for(const auto& geometry:geometries){auto sample=sample_for(geometry[0],geometry[1],geometry[2],geometry[3]);
        const auto input=boundary(static_cast<float>(geometry[2]),static_cast<float>(geometry[0]));
        const auto result=decide_union(input,valid,sample,10000,1000,origin);
        if(result.rejection||!result.modified||result.union_right!=geometry[2]+geometry[3]||
           bits(result.forwarded)!=bits(boundary(static_cast<float>(geometry[2]+geometry[3]),static_cast<float>(geometry[0]))))return false;
    }
    auto sample=sample_for(1920,1080,960,960);
    if(origin!=10000)return false;
    auto padded=sample_for(4096,2160,2048,2048);
    for(unsigned eye=0;eye<2;++eye){padded.target[eye][3]=2048;padded.source[eye][3]=2048;}
    if(decide_union(0,valid,padded,20000,1000,origin).rejection!=rectangles)return false;
    const auto padding=decide_union(0,valid,padded,20000,1000,origin,true);
    if(padding.rejection||!padding.modified||padding.forwarded!=1.f)return false;
    for(unsigned i=0;i<8;++i){auto s=padded;
        switch(i){case 0:s.target[1][3]=2047;s.source[1][3]=2047;break;
        case 1:for(unsigned e=0;e<2;++e){s.target[e][3]=2161;s.source[e][3]=2161;}break;
        case 2:for(unsigned e=0;e<2;++e){s.target[e][3]=0;s.source[e][3]=0;}break;
        case 3:s.target[0][1]=1;s.source[0][1]=1;break;
        case 4:s.source[0][3]=2047;break;
        case 5:s.viewport[3]=2048;break;
        case 6:s.target[1][0]=2049;s.source[1][0]=2049;break;
        case 7:for(unsigned e=0;e<2;++e){s.target[e][3]=-1;s.source[e][3]=-1;}break;}
        const auto d=decide_union(0,valid,s,20000,1000,origin,true);
        if(!d.rejection||d.modified||d.forwarded!=0.f)return false;
    }
    for(auto tick:{39999LL,40000LL,40001LL,55000LL}){
        const auto result=decide_union(0,valid,sample,tick,1000,origin);
        if(result.rejection||!result.modified||result.forwarded!=1.f)return false;
    }
    for(unsigned i=0;i<28;++i){auto gate=valid;auto s=sample;float value=0;
        switch(i){case 0:gate.enabled=false;break;case 1:gate.recording=false;break;case 2:gate.diagnostics=false;break;
        case 3:gate.trial=false;break;case 4:gate.renderer=false;break;case 5:++gate.caller;break;
        case 6:s.readable=false;break;case 7:s.command_list_valid=false;break;case 8:s.storage=0;break;
        case 9:++s.storage;break;case 10:s.count=1;break;case 11:s.count=3;break;case 12:s.capacity=1;break;case 13:s.capacity=1025;break;
        case 14:s.color_dimensions[0]=0;break;case 15:s.color_dimensions[1]=0;break;case 16:s.color_dimensions[0]=16385;break;
        case 17:s.color_dimensions[1]=16385;break;case 18:--s.target[1][0];break;case 19:++s.target[1][0];break;
        case 20:s.target[1][2]=INT_MAX;break;case 21:s.target[0][2]=0;break;case 22:s.target[0][0]=1;break;
        case 23:s.target[1][1]=1;break;case 24:--s.target[1][3];break;case 25:s.viewport_count=2;break;
        case 26:value=.125f;break;case 27:s.source[0][2]=960.25f;break;}
        const auto result=decide_union(value,gate,s,20000,1000,origin);
        if(!result.rejection||result.modified||bits(result.forwarded)!=bits(value)||origin!=10000)return false;
    }
    for(unsigned i=0;i<6;++i){auto s=sample;s.viewport[i]+=.25f;if(decide_union(0,valid,s,20000,1000,origin).rejection!=viewport)return false;}
    for(unsigned eye=0;eye<2;++eye)for(unsigned i=0;i<4;++i){auto s=sample;s.source[eye][i]+=.25f;
        if(decide_union(0,valid,s,20000,1000,origin).rejection!=rectangles)return false;}
    for(auto raw:{0x80000000U,0x7fc12345U,0x7f800000U,0xff800000U}){float value{};memcpy(&value,&raw,4);
        auto result=decide_union(value,valid,sample,20000,1000,origin);
        if(result.rejection!=argument||result.modified||bits(result.forwarded)!=raw)return false;
        auto s=sample;s.source[0][2]=value;if(decide_union(0,valid,s,20000,1000,origin).rejection!=rectangles)return false;
    }
    if(decide_union(0,valid,sample,9999,1000,origin).rejection!=clock||decide_union(0,valid,sample,20000,0,origin).rejection!=clock)return false;
    long long unstarted{};auto invalid=valid;invalid.caller=0;
    if(!decide_union(0,invalid,sample,20000,1000,unstarted).rejection||unstarted)return false;
    return true;
}
bool lifetime_boundary_self_test(){
    gears_submitted::Lifetime life;const auto sample=sample_for(1920,1080,960,960);long long origin=10000;
    const uintptr_t consuming=0x1000,queued=0x2000,unknown=0x3000;
    if(!life.begin(consuming)||!life.begin(queued))return false;
    // Exactly the ownership gate latched by hook_consume on entry. A close
    // between its two mask calls must not interrupt the accepted family.
    Gate gate{true,true,true,life.contains(consuming),true,0x119be2};
    const auto first=decide_union(0,gate,sample,39999,1000,origin);
    if(life.close()||!life.is_closed()||life.begin(unknown)||!life.contains(consuming)||!life.contains(queued))return false;
    const auto second=decide_union(0,gate,sample,40001,1000,origin);
    if(first.rejection||second.rejection||!first.modified||!second.modified||bits(first.forwarded)!=bits(second.forwarded))return false;
    if(life.retire(consuming)||life.contains(consuming)||life.pending()!=1)return false;
    // A family already queued before close may enter its consumer afterwards.
    gate.trial=life.contains(queued);
    for(auto tick:{50000LL,50001LL})if(!decide_union(0,gate,sample,tick,1000,origin).modified)return false;
    auto stopped=gate;stopped.recording=false;
    const auto stop_result=decide_union(0,stopped,sample,50002,1000,origin);
    if(stop_result.rejection!=scope||stop_result.modified||bits(stop_result.forwarded))return false;
    if(!life.retire(queued)||life.pending()||life.close()||life.begin(queued))return false;
    for(auto root:{consuming,queued,unknown}){gate.trial=life.contains(root);
        const auto result=decide_union(0,gate,sample,50003,1000,origin);
        if(result.rejection!=scope||result.modified||bits(result.forwarded))return false;
    }
    return true;
}
}
std::mutex view_union_mask_mutex;long long view_union_mask_origin{};
struct ViewUnionMaskTrace {
    uint64_t calls{},accepted{},modified{},rejected{},overflow{};long long first{},last{};
    std::array<uint64_t,gears_view_union_mask::reject_count> reasons{};
} view_union_mask_trace;
struct ViewUnionMaskGeometry {
    gears_first_view_mask::Snapshot sample{};float incoming{},effective{};int union_right{};
    uint64_t calls{};long long first{},last{};
};
std::array<ViewUnionMaskGeometry,16> view_union_mask_geometries{};
void complete_view_union_mask(float value,const gears_view_union_mask::Decision& decision,const gears_first_view_mask::Snapshot& sample){
    std::lock_guard<std::mutex> lock(view_union_mask_mutex);auto& t=view_union_mask_trace;++t.calls;
    if(decision.rejection!=gears_view_union_mask::accepted){++t.rejected;++t.reasons[decision.rejection];return;}
    const auto tick=now();++t.accepted;t.modified+=decision.modified;if(!t.first)t.first=tick;t.last=tick;
    for(auto& e:view_union_mask_geometries){
        if(!e.calls){e.sample=sample;e.incoming=value;e.effective=decision.forwarded;e.union_right=decision.union_right;e.calls=1;e.first=e.last=tick;return;}
        if(e.sample.source==sample.source&&e.sample.target==sample.target&&e.sample.color_dimensions==sample.color_dimensions&&
           gears_view_union_mask::bits(e.incoming)==gears_view_union_mask::bits(value)&&gears_view_union_mask::bits(e.effective)==gears_view_union_mask::bits(decision.forwarded)){
            ++e.calls;e.last=tick;return;
        }
    }
    ++t.overflow;
}
void print_view_union_mask(FILE* file){
    std::lock_guard<std::mutex> lock(view_union_mask_mutex);const auto& t=view_union_mask_trace;
    fprintf(file,"view_union_mask_fix enabled=%u calls=%llu accepted=%llu modified=%llu rejected=%llu first_qpc=%lld last_qpc=%lld frequency=%lld origin_qpc=%lld production_seconds=30 independent_mask_deadline=0 lifetime_scope=tracked_consumer geometry_overflow=%llu same_family_two_views=1\n",
        view_union_mask_fix,t.calls,t.accepted,t.modified,t.rejected,t.first,t.last,qpc_frequency,view_union_mask_origin,t.overflow);
    for(const auto& e:view_union_mask_geometries)if(e.calls){const auto& s=e.sample;
        fprintf(file,"view_union_mask_geometry calls=%llu width=%u height=%u left=%d,%d,%d,%d right=%d,%d,%d,%d union_right=%d incoming=%.9g effective=%.9g first_qpc=%lld last_qpc=%lld\n",
            e.calls,s.color_dimensions[0],s.color_dimensions[1],s.target[0][0],s.target[0][1],s.target[0][2],s.target[0][3],
            s.target[1][0],s.target[1][1],s.target[1][2],s.target[1][3],e.union_right,e.incoming,e.effective,e.first,e.last);
    }
    for(unsigned reason=1;reason<t.reasons.size();++reason)if(t.reasons[reason])
        fprintf(file,"view_union_mask_rejected reason=%s calls=%llu\n",gears_view_union_mask::reject_names[reason],t.reasons[reason]);
}
