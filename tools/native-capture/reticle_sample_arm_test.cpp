#include "reticle_sample_arm.h"
#include <cassert>
#include <cstdio>
#include <memory>
namespace r=gears_reticle_sample;
static r::Identity identity(){return {11,22,0x140000000,1000,3};}
static std::string wire(r::Identity id=identity(),std::uint64_t nonce=1,std::uint64_t request=2000,std::uint64_t expiry=2500){
    return "[ReticleSample]\nschema=1\npid="+std::to_string(id.pid)+"\ncreation_time="+std::to_string(id.creation_time)+
        "\nbase="+std::to_string(id.base)+"\nstarted_qpc="+std::to_string(id.started_qpc)+"\nworld_generation="+std::to_string(id.world_generation)+
        "\nnonce="+std::to_string(nonce)+"\nrequest_qpc="+std::to_string(request)+"\nexpires_qpc="+std::to_string(expiry)+"\n";
}
struct Row {std::uint64_t sequence{},ticket{};bool completed{};};
static void parse_tests(){r::Request request{};assert(r::parse(wire(),request)&&r::same(request.identity,identity()));
    const auto old=request;auto bad=[&](std::string const& text){assert(!r::parse(text,request));assert(request.nonce==old.nonce&&r::same(request.identity,old.identity));};
    bad("");bad(wire()+"nonce=2\n");bad(wire()+"unknown=1\n");bad(wire()+"[ReticleSample]\n");
    bad(wire()+std::string(1,'\0'));bad(wire(identity(),0));bad(wire(identity(),1,2000,2000));
    for(auto value:{"-1","+1","0x11","18446744073709551616","1 ",""}){auto text=wire();auto at=text.find("nonce=1");text.replace(at,7,std::string("nonce=")+value);bad(text);}
    auto id=identity();id.pid=0;bad(wire(id));id=identity();id.pid=0x100000000ull;bad(wire(id));
    std::string crlf;for(char c:wire()){if(c=='\n')crlf+='\r';crlf+=c;}assert(r::parse(crlf,request));
}
static void admission_tests(){r::Rejection why{};r::Bank<Row> bank;unsigned acks{};
    auto ack=[&](r::Gate const& gate){++acks;assert(gate.accepted&&!gate.acknowledged);
        assert(!bank.append(10,2100,identity(),true,true));return true;};
    assert(!r::worker_admit(bank,false,"invalid",identity(),2100,1000,ack,why)&&why==r::Rejection::disabled&&!acks);
    for(unsigned field=0;field<5;++field){auto id=identity();
        if(field==0)++id.pid;if(field==1)++id.creation_time;if(field==2)++id.base;if(field==3)++id.started_qpc;if(field==4)++id.world_generation;
        assert(!r::worker_admit(bank,true,wire(id),identity(),2100,1000,ack,why)&&why==r::Rejection::identity&&!acks);}
    for(auto text:{wire(identity(),1,2200,2500),wire(identity(),1,500,2500),wire(identity(),1,2000,200000)})
        assert(!r::worker_admit(bank,true,text,identity(),2100,1000,ack,why)&&why==r::Rejection::clock);
    assert(!r::worker_admit(bank,true,wire(),identity(),2500,1000,ack,why));
    assert(!r::worker_admit(bank,true,wire(),identity(),2100,0,ack,why));
    assert(r::worker_admit(bank,true,wire(),identity(),2100,1000,ack,why)&&acks==1);
    assert(bank.gate.end_qpc==4100&&bank.gate.acknowledged);
    assert(!r::worker_admit(bank,true,wire(identity(),2),identity(),2200,1000,ack,why)&&why==r::Rejection::already_armed&&acks==1);
    assert(!bank.append(10,2101,identity(),false,true));assert(!bank.append(10,2101,identity(),true,false));
    auto* row=bank.append(10,2101,identity(),true,true);assert(row);*row={70,10,false};
    assert(!bank.append(11,4100,identity(),true,true)&&bank.gate.frozen&&bank.count==1);
    bank.complete(70,true,[](Row& row,std::uint64_t sequence,bool value){if(row.sequence==sequence)row.completed=value;});
    assert(bank.rows[0].completed); // TTL does not erase valid late completion.
    r::Bank<Row> failed;assert(!r::worker_admit(failed,true,wire(),identity(),2100,1000,[](r::Gate const&){return false;},why));
    assert(why==r::Rejection::ack_failed&&!failed.append(10,2101,identity(),true,true));
    r::Bank<Row> world;assert(r::worker_admit(world,true,wire(),identity(),2100,1000,[](r::Gate const&){return true;},why));
    auto changed=identity();++changed.world_generation;assert(!world.append(10,2101,changed,true,true)&&world.gate.frozen);
}
static gears_ui_indices::Snapshot snapshot(unsigned quads){gears_ui_indices::Snapshot s{};s.payload=99;s.generation=3;
    s.vertex_count=quads*4;s.segment_count=1;s.total_indices=quads*6;auto& segment=s.segments[0];segment.minimum=0;segment.maximum=int(s.vertex_count-1);
    segment.index_count=s.total_indices;segment.kind_mask=1;segment.entry_mask=(1u<<quads)-1;segment.known[0]=s.total_indices;segment.expected={s.total_indices,1,0,0,0};
    const unsigned quad[6]{0,1,2,1,3,2};for(unsigned i=0;i<quads;++i)for(unsigned j=0;j<6;++j)s.indices[i*6+j]=s.global_vertices[i*6+j]=i*4+quad[j];return s;}
static void membership_tests(){for(unsigned quads:{1u,4u}){auto good=snapshot(quads);auto actual=good.segments[0].expected;
    assert(r::membership(good,0,99,3,actual,true));assert(!r::membership(good,0,99,3,actual,false));
    assert(!r::membership(good,1,99,3,actual,true));assert(!r::membership(good,0,98,3,actual,true));assert(!r::membership(good,0,99,4,actual,true));
    auto changed=actual;++changed.first_instance;assert(!r::membership(good,0,99,3,changed,true));
    auto bad=good;bad.segments[0].known[0]--;bad.segments[0].unknown++;assert(!r::membership(bad,0,99,3,actual,true));
    bad=good;bad.segments[0].kind_mask=2;bad.segments[0].known={0,good.total_indices,0};assert(!r::membership(bad,0,99,3,actual,true));
    bad=good;bad.global_vertices[0]=999;assert(!r::membership(bad,0,99,3,actual,true));}}
static void late_capacity_tests(){auto bank=std::make_unique<r::Bank<Row>>();r::Rejection why{};
    // A long pre-arm period cannot consume independent diagnostic quota.
    for(unsigned i=0;i<10000;++i)assert(!bank->append(i+1,2000,identity(),true,true));assert(!bank->count);
    assert(r::worker_admit(*bank,true,wire(),identity(),2100,1000,[](r::Gate const&){return true;},why));
    // Four separate outer-quadrant draws plus inner draw on each of 64 tickets.
    for(unsigned ticket=1;ticket<=64;++ticket)for(unsigned part=0;part<5;++part){auto* row=bank->append(ticket,2100+ticket,identity(),true,true);assert(row);*row={ticket,ticket,false};}
    assert(bank->count==320&&bank->ticket_count==64);
    for(unsigned i=320;i<512;++i)assert(bank->append(64,2300,identity(),true,true));
    assert(!bank->append(64,2300,identity(),true,true)&&bank->count==512&&bank->capacity_rejected==1);
    auto limited=std::make_unique<r::Bank<Row>>();assert(r::worker_admit(*limited,true,wire(),identity(),2100,1000,[](r::Gate const&){return true;},why));
    for(unsigned i=1;i<=256;++i)assert(limited->append(i,2300,identity(),true,true));
    assert(!limited->append(257,2300,identity(),true,true)&&limited->ticket_count==256);
}
int main(){parse_tests();admission_tests();membership_tests();late_capacity_tests();std::puts("ReticleSampleArmTest passed: parser, worker ACK, identity/TTL/off, membership, late capacity and post-TTL completion");}
