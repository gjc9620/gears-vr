#pragma once
#include <array>
#include <cstdint>

// A diagnostic localization experiment, never an output acceptance policy.
namespace gears_death_source {
enum Phase:unsigned {a1,b,a2,waiting,invalid};
enum Reject:unsigned {accepted,disabled,attestation,scope,caller,native_view,shape,source,clock,reject_count};
struct Args {std::array<float,8> rectangle{};std::array<unsigned,4> dimensions{};};
struct Gate {
    bool enabled{},attested{},recording{},diagnostics{},frozen_enabled{},unpossessed{},renderer{},other_control{};
    uint64_t ticket{},frozen_ticket{};uintptr_t caller{},current{},views{};int count{},capacity{};
    bool native_read{},stable{},source_valid{};uintptr_t source_canonical{};
    uint64_t source_width{};unsigned source_height{},source_format{},source_dimension{},source_array{},source_mips{},source_samples{};
};
struct Decision {Args forwarded{};unsigned phase{waiting},rejection{disabled};bool modified{};long long qpc{};};
inline unsigned phase_at(long long origin,long long tick,long long frequency){
    if(!origin)return waiting;
    if(origin<0||frequency<=0||tick<origin)return invalid;
    const auto elapsed=tick-origin,seconds=elapsed/frequency;
    if(seconds<1)return a1;
    if(seconds<2)return b;
    if(seconds>2)return a2;
    // A2 begins at ceil(2.5 * frequency), without multiplying QPC values.
    return elapsed%frequency<frequency/2+frequency%2?b:a2;
}
inline Decision decide(Args const& args,Gate const& g,long long tick,long long frequency,long long& origin){
    Decision d;d.forwarded=args;d.qpc=tick;d.phase=phase_at(origin,tick,frequency);
    if(!g.enabled)return d;
    d.rejection=attestation;if(!g.attested)return d;
    d.rejection=scope;
    if(!g.recording||!g.diagnostics||!g.frozen_enabled||!g.unpossessed||!g.renderer||g.other_control||
       !g.ticket||g.ticket!=g.frozen_ticket)return d;
    d.rejection=caller;if(g.caller!=0xd80a7b)return d;
    d.rejection=native_view;
    if(!g.native_read||!g.views||g.views%8||g.current%8||g.count!=2||g.capacity<2||g.capacity>1024||
       g.current<g.views||g.current-g.views!=0xd30)return d;
    d.rejection=shape;const auto w=args.dimensions[0],h=args.dimensions[1];
    if(!w||w%2||w>8192||!h||h>8192||args.dimensions!=std::array<unsigned,4>{w,h,w,h})return d;
    const float half=float(w/2),height=float(h);
    if(args.rectangle!=std::array<float,8>{half,0,half,height,half,0,half,height})return d;
    d.rejection=source;
    if(!g.stable||!g.source_valid||!g.source_canonical||g.source_dimension!=3||g.source_array!=1||
       !g.source_mips||g.source_samples!=1||g.source_format!=26||g.source_width!=w||g.source_height!=h)return d;
    d.rejection=clock;if(tick<=0||frequency<=0||origin<0||(origin&&tick<origin))return d;
    if(!origin)origin=tick; // First fully eligible call; never reset by this policy.
    d.phase=phase_at(origin,tick,frequency);d.rejection=accepted;
    if(d.phase==b){d.forwarded.rectangle[4]=0;d.modified=true;}
    return d;
}
struct Receipt {
    uint64_t ticket{},world{};long long qpc{};unsigned phase{};float original_u{},effective_u{};
    uintptr_t source_canonical{};
};
struct Recorder {
    static constexpr unsigned capacity=512;
    long long origin{};std::array<Receipt,capacity> rows{};unsigned size{};
    std::array<uint64_t,3> counts{};uint64_t dropped{},modified{};Receipt last{};
    void completed(Receipt const& r){
        if(r.phase>2)return;
        ++counts[r.phase];modified+=r.original_u!=r.effective_u;last=r;
        if(size<capacity)rows[size++]=r;else ++dropped;
    }
};
}
