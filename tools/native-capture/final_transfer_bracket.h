#pragma once
#include "final_transfer_policy.h"
#include "frame_link.h"

// Observe-only CPU snapshots. Entry may precede lazy engine binding; neither
// equality nor a complete entry grants transfer admission or GPU proof.
namespace gears_final_transfer_bracket {
struct Draw {
    uintptr_t raw_list{},caller{};unsigned kind{},elements{},instances{};
    bool operator==(Draw const& b)const{return raw_list==b.raw_list&&caller==b.caller&&kind==b.kind&&elements==b.elements&&instances==b.instances;}
};
struct DrawCalls {
    uint64_t all_draws{},matched_draws{},bypass_draws{};Draw first{},first_matched{};
    void call(Draw value,bool bypass){
        if(bypass){++bypass_draws;return;}
        if(++all_draws==1)first=value;
    }
    void match(Draw value){if(++matched_draws==1)first_matched=value;}
    bool exactly_one_routed_draw()const{return all_draws==1&&matched_draws==1&&!bypass_draws&&first==first_matched;}
};
struct Snapshot {
    gears_final_transfer::Input input{};
    gears_frame_link::ListRef list{};
    uintptr_t raw_list{},context{},source_object{},chain{};
    uint64_t world{},inventory{},resize{},ticket{};
    unsigned reason{},pair_rejection{};bool pair_linked{};
    long long qpc{};
};
enum Difference:unsigned {list=1,source=2,target=4,rectangles=8,world=16,inventory=32,pair=64,quad=128};
inline unsigned compare(Snapshot const& a,Snapshot const& b){
    unsigned out{};
    if(a.raw_list!=b.raw_list||a.list.identity!=b.list.identity||a.list.generation!=b.list.generation)out|=list;
    auto const& x=a.input.source;auto const& y=b.input.source;
    if(a.source_object!=b.source_object||a.context!=b.context||a.input.slots!=b.input.slots||
       x.resource!=y.resource||x.dimension!=y.dimension||x.width!=y.width||x.height!=y.height||
       x.depth!=y.depth||x.mips!=y.mips||x.format!=y.format||x.samples!=y.samples)out|=source;
    if(a.input.binding.targets!=b.input.binding.targets||!(a.input.binding.target==b.input.binding.target)||
       a.input.known_buffer!=b.input.known_buffer||a.input.descriptor_lost!=b.input.descriptor_lost)out|=target;
    auto const& r=a.input.rectangles;auto const& s=b.input.rectangles;
    if(r.tracked!=s.tracked||!(r.viewport==s.viewport)||!(r.scissor==s.scissor)||r.viewport_count!=s.viewport_count||
       r.scissor_count!=s.scissor_count||r.depth_known!=s.depth_known||r.min_depth!=s.min_depth||r.max_depth!=s.max_depth)out|=rectangles;
    if(a.world!=b.world)out|=world;
    if(a.chain!=b.chain||a.inventory!=b.inventory||a.resize!=b.resize)out|=inventory;
    if(a.ticket!=b.ticket||a.pair_linked!=b.pair_linked||a.pair_rejection!=b.pair_rejection)out|=pair;
    if(a.input.args!=b.input.args||a.input.dimensions!=b.input.dimensions||a.input.quad_caller!=b.input.quad_caller)out|=quad;
    return out;
}
// Separate quotas prevent early incomplete entries from consuming useful rows.
struct Budget {
    static constexpr unsigned maximum_attempts=64,rejected_limit=4,complete_limit=12;
    unsigned attempted{},rejected{},complete{},skipped{},dropped{};
    bool preflight(){
        if(attempted>=maximum_attempts||(rejected==rejected_limit&&complete==complete_limit)){++skipped;return false;}
        ++attempted;return true;
    }
    int retain(bool valid){
        if(valid){if(complete==complete_limit){++dropped;return -1;}return int(rejected_limit+complete++);}
        if(rejected==rejected_limit){++dropped;return -1;}return int(rejected++);
    }
};
}
