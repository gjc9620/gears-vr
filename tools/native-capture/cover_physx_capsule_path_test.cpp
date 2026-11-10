#include "cover_physx_capsule_path.h"
#include <cstdio>
#include <memory>
#include <cfenv>
namespace p=gears_cover_capsule_path;
namespace q=gears_cover_physx_query;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
static bool same(p::Plan const& a,p::Plan const& b){return !std::memcmp(&a,&b,sizeof a);}
static std::uint32_t bits(float x){std::uint32_t value{};std::memcpy(&value,&x,4);return value;}
static void vector_bits(q::Vector v){std::printf("[%u,%u,%u]",bits(v.x),bits(v.y),bits(v.z));}
static void emit(p::Plan const& plan){
    std::printf("{\"original_first\":");vector_bits(plan.original.first);
    std::printf(",\"original_second\":");vector_bits(plan.original.second);
    std::printf(",\"requested\":");vector_bits(plan.requested);
    std::printf(",\"original_radius\":%u,\"count\":%u,\"maximum_error\":%.17g,\"maximum_expansion\":%.17g,\"segments\":[",
                bits(plan.original.radius),plan.count,plan.maximum_endpoint_error,plan.maximum_radius_expansion);
    for(unsigned i=0;i<plan.count;++i){auto const& segment=plan.segments[i];
        std::printf("%s{\"first\":",i?",":"");vector_bits(segment.shape.first);
        std::printf(",\"second\":");vector_bits(segment.shape.second);
        std::printf(",\"movement\":");vector_bits(segment.movement);
        std::printf(",\"radius\":%u}",bits(segment.shape.radius));}
    std::printf("],\"endpoint_first\":");vector_bits(plan.endpoint.first);
    std::printf(",\"endpoint_second\":");vector_bits(plan.endpoint.second);
    std::printf(",\"endpoint_radius\":%u}\n",bits(plan.endpoint.radius));
}
int main(int argc,char** argv){
    const bool witness=argc==2&&std::string(argv[1])=="--emit-witness";
    auto work=std::make_unique<p::Workspace>();auto output=std::make_unique<p::Plan>();
    q::Capsule capsule{{1,2,3},{1,2,5},.5f};q::Vector movement{300,0,0};p::Policy policy{64,.1};
    C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::planned&&output->count==4);
    C(output->requested.x==300&&output->maximum_radius_expansion==0&&output->maximum_endpoint_error==0);
    C(output->endpoint.first.x==301&&output->endpoint.second.x==301&&output->endpoint.radius==.5f);
    for(unsigned i=0;i<output->count;++i){auto const& segment=output->segments[i];
        C(segment.shape.first.x==1+75.f*i&&segment.shape.second.x==1+75.f*i&&segment.movement.x==75&&segment.shape.radius==.5f);
        C(q::detail::valid(segment.shape)&&q::detail::sweep_motion(segment.shape,segment.movement));}
    const auto saved=std::make_unique<p::Plan>(*output);
    movement={120,160,0};C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::planned&&output->count==2);
    C(output->segments[0].movement.x==60&&output->segments[0].movement.y==80);
    movement={100,1e-30f,0};C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::planned&&output->count==2);
    if(witness)emit(*output);
    movement={-6400,0,0};C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::planned&&output->count==64);
    C(output->segments[63].shape.first.x==-6299&&output->segments[63].movement.x==-100);
    movement={std::nextafter(6400.f,7000.f),0,0};*output=*saved;
    C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::capacity&&same(*saved,*output));
    movement={201,0,0};auto limited=policy;limited.segment_limit=3;
    C(p::prepare(capsule,movement,limited,*work,*output)==p::Code::capacity&&same(*saved,*output));
    movement={};C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::request&&same(*saved,*output));
    movement={1,0,0};auto bad=capsule;bad.second=bad.first;
    C(p::prepare(bad,movement,policy,*work,*output)==p::Code::request&&same(*saved,*output));
    limited=policy;limited.maximum_radius_expansion=-1;
    C(p::prepare(capsule,movement,limited,*work,*output)==p::Code::request&&same(*saved,*output));
    limited=policy;limited.segment_limit=65;
    C(p::prepare(capsule,movement,limited,*work,*output)==p::Code::request&&same(*saved,*output));
    C(p::prepare(capsule,movement,policy,*work,work->candidate)==p::Code::alias);
    const auto rounding=std::fegetround();C(std::fesetround(FE_UPWARD)==0);
    const auto directed=p::prepare(capsule,movement,policy,*work,*output);
    C(std::fesetround(rounding)==0);C(directed==p::Code::request&&same(*saved,*output));
    movement={(std::numeric_limits<float>::min)(),0,0};
    C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::numeric&&same(*saved,*output));
    // All four segment starts are representable, but the final 1-unit shaft
    // collapses at 2^24. Even this late failure must preserve the old plan.
    auto final_collapse=capsule;final_collapse.first={16776916,0,0};final_collapse.second={16776917,0,0};
    movement={300,0,0};limited=policy;limited.maximum_radius_expansion=10;
    C(p::prepare(final_collapse,movement,limited,*work,*output)==p::Code::numeric&&same(*saved,*output));
    // Non-integer coordinates require conservative radius expansion; explicit
    // zero expansion budget must refuse rather than silently round the path.
    capsule={{4096.1f,100.3f,7.2f},{4096.35f,100.3f,8.2f},.5f};movement={301.07f,32.1f,-25.03f};
    limited=policy;limited.maximum_radius_expansion=0;
    C(p::prepare(capsule,movement,limited,*work,*output)==p::Code::numeric&&same(*saved,*output));
    const auto original=capsule;const auto requested=movement;
    C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::planned&&output->count==4);
    C(!std::memcmp(&original,&capsule,sizeof capsule)&&!std::memcmp(&requested,&movement,sizeof movement));
    C(output->maximum_radius_expansion>=output->maximum_endpoint_error&&output->maximum_radius_expansion<=policy.maximum_radius_expansion);
    for(unsigned i=0;i<output->count;++i){auto const& segment=output->segments[i];
        C(q::detail::valid(segment.shape)&&q::detail::sweep_motion(segment.shape,segment.movement));
        C(double(segment.movement.x)*output->count==double(movement.x)&&
          double(segment.movement.y)*output->count==double(movement.y)&&double(segment.movement.z)*output->count==double(movement.z));}
    if(witness){emit(*saved);emit(*output);}
    for(unsigned i=0;i<24;++i){
        const float n=float(i);
        capsule={{-5000.13f+37.17f*n,103.07f+3.11f*n,23.03f-.19f*n},
                 {-4999.88f+37.17f*n,103.57f+3.11f*n,24.03f-.19f*n},.31f};
        movement={250.03f+4.17f*n,-81.07f+1.13f*n,52.03f-.73f*n};
        C(p::prepare(capsule,movement,policy,*work,*output)==p::Code::planned&&output->count==4);
        C(output->maximum_radius_expansion<=policy.maximum_radius_expansion);
        if(witness)emit(*output);
    }
    if(witness)return failures?1:0;
    std::printf("CoverCapsulePath checks=%u failed=%u pure-geometry-only no-query no-game\n",checks,failures);
    return failures?1:0;
}
