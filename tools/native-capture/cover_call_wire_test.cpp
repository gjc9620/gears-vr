#include "cover_call_wire.h"
#include <cstdio>
#include <fstream>
#include <string>

namespace wire=gears_cover_call_wire;
using Bytes=std::vector<unsigned char>;
static void word(Bytes& out,std::uint32_t value){for(unsigned i=0;i<4;++i)out.push_back(static_cast<unsigned char>(value>>(i*8)));}
static void pointer(Bytes& out,std::uint64_t value){for(unsigned i=0;i<8;++i)out.push_back(static_cast<unsigned char>(value>>(i*8)));}
static void set32(Bytes& out,std::size_t offset,std::uint32_t value){for(unsigned i=0;i<4;++i)out[offset+i]=static_cast<unsigned char>(value>>(i*8));}
static void set64(Bytes& out,std::size_t offset,std::uint64_t value){for(unsigned i=0;i<8;++i)out[offset+i]=static_cast<unsigned char>(value>>(i*8));}
static void checksum(Bytes& out){set32(out,out.size()-4,wire::crc32(out.data(),out.size()-4));}
static void retotal(Bytes& out){set32(out,12,static_cast<std::uint32_t>(out.size()));checksum(out);}
struct Fixture {
    static constexpr std::uintptr_t base=0x140000000;
    std::vector<gears_cover_call_trace::Function> functions;
    std::vector<wire::Check> checks;
    Fixture(){
        checks.push_back({base+gears_ui_call_boundary::rva,Bytes(gears_ui_call_boundary::entry_bytes.begin(),gears_ui_call_boundary::entry_bytes.end())});
        for(unsigned i=0;i<6;++i){
            gears_cover_call_trace::Function function{0x10000+i*0x1000,0x8000+i*0x100,4,32,{},{},{}};
            if(i==0||i==2){function.pawn_ct={0,1};function.pawn_ca={1,1};function.pawn_cd={2,1};}
            functions.push_back(function);
            Bytes local;word(local,function.locals_size);checks.push_back({function.address+0x88,local});
            Bytes script;pointer(script,function.script);word(script,function.size);word(script,8);checks.push_back({function.address+0x8c,script});
            Bytes entry;pointer(entry,base+0x1d6ca0);checks.push_back({function.address+0x114,entry});
            checks.push_back({function.script,Bytes{1,static_cast<unsigned char>(i),2,3}});
        }
    }
    Bytes encode()const{
        Bytes out(wire::magic.begin(),wire::magic.end());out.insert(out.end(),{1,0,0,0});
        word(out,0);word(out,123);word(out,6);word(out,static_cast<std::uint32_t>(checks.size()));word(out,0);
        pointer(out,base);pointer(out,0x2000);pointer(out,0x3000);pointer(out,0x4000);
        for(unsigned i=0;i<32;++i)out.push_back(static_cast<unsigned char>(i));
        for(unsigned i=0;i<functions.size();++i){auto const& f=functions[i];
            word(out,i);word(out,0);pointer(out,f.address);pointer(out,f.script);word(out,f.size);word(out,f.locals_size);
            for(auto const& local:{f.pawn_ct,f.pawn_ca,f.pawn_cd}){word(out,local.offset);word(out,local.width);}}
        for(auto const& check:checks){pointer(out,check.address);word(out,static_cast<std::uint32_t>(check.bytes.size()));word(out,0);
            out.insert(out.end(),check.bytes.begin(),check.bytes.end());}
        word(out,0);retotal(out);return out;
    }
};
static void hex(Bytes const& bytes){for(auto byte:bytes)std::printf("%02x",unsigned(byte));}
static int packet_file(char const* path){
    // Test-only file adapter; the wire header itself has no I/O capability.
    std::ifstream input(path,std::ios::binary|std::ios::ate);if(!input)return 2;
    const auto length=input.tellg();if(length<0||length>static_cast<std::streamoff>(wire::max_packet))return 2;
    Bytes bytes(static_cast<std::size_t>(length));input.seekg(0);input.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    if(!input)return 2;
    auto result=wire::parse(bytes.data(),bytes.size());if(!result){std::fprintf(stderr,"parse: %s\n",wire::status_name(result.status));return 3;}
    auto const& p=result.packet;
    std::printf("{\"pid\":%u,\"base\":%llu,\"controller\":%llu,\"pawn\":%llu,\"camera\":%llu,\"mutation_authorized\":false,\"config_sha256\":\"",
                p.pid,static_cast<unsigned long long>(p.base),static_cast<unsigned long long>(p.controller),
                static_cast<unsigned long long>(p.pawn),static_cast<unsigned long long>(p.camera));
    hex(Bytes(p.config_sha256.begin(),p.config_sha256.end()));std::printf("\",\"functions\":[");
    for(unsigned i=0;i<p.functions.size();++i){auto const& f=p.functions[i];if(i)std::printf(",");
        std::printf("{\"address\":%llu,\"script\":%llu,\"size\":%u,\"locals_size\":%u,\"locals\":[[%u,%u],[%u,%u],[%u,%u]]}",
            static_cast<unsigned long long>(f.address),static_cast<unsigned long long>(f.script),f.size,f.locals_size,
            f.pawn_ct.offset,f.pawn_ct.width,f.pawn_ca.offset,f.pawn_ca.width,f.pawn_cd.offset,f.pawn_cd.width);}
    std::printf("],\"checks\":[");
    for(std::size_t i=0;i<p.checks.size();++i){auto const& c=p.checks[i];if(i)std::printf(",");
        std::printf("{\"address\":%llu,\"bytes\":\"",static_cast<unsigned long long>(c.address));hex(c.bytes);std::printf("\"}");}
    std::printf("]}\n");return 0;
}
#define CHECK(condition) do{++checks;if(!(condition)){std::printf("failed line %d: %s\n",__LINE__,#condition);return 1;}}while(false)
int main(int argc,char** argv){
    if(argc==3&&std::string(argv[1])=="--packet")return packet_file(argv[2]);
    unsigned checks{};Fixture fixture;auto original=fixture.encode();
    CHECK(wire::crc32(reinterpret_cast<unsigned char const*>("123456789"),9)==0xcbf43926u);
    CHECK(wire::crc32(nullptr,0)==0);
    auto parsed=wire::parse(original.data(),original.size());CHECK(parsed);
    CHECK(parsed.packet.pid==123&&parsed.packet.base==Fixture::base&&!parsed.packet.mutation_authorized);
    CHECK(parsed.packet.controller==0x2000&&parsed.packet.pawn==0x3000&&parsed.packet.camera==0x4000);
    CHECK(parsed.packet.functions.size()==6&&parsed.packet.checks.size()==25);
    for(unsigned i=0;i<6;++i){CHECK(parsed.packet.functions[i].address==fixture.functions[i].address);
        CHECK(parsed.packet.functions[i].pawn_ct.width==((i==0||i==2)?1u:0u));}
    for(unsigned i=0;i<25;++i){CHECK(parsed.packet.checks[i].address==fixture.checks[i].address);CHECK(parsed.packet.checks[i].bytes==fixture.checks[i].bytes);}
    CHECK(!wire::parse(nullptr,original.size()));CHECK(!wire::parse(original.data(),wire::max_packet+1));
    for(std::size_t size=0;size<original.size();++size)CHECK(!wire::parse(original.data(),size));
    auto mutate=[&](std::size_t offset,std::uint32_t value){auto bytes=original;set32(bytes,offset,value);checksum(bytes);return bytes;};
    for(auto test:std::vector<std::pair<Bytes,wire::Status>>{
        {mutate(8,2),wire::Status::version_mismatch},{mutate(10,1),wire::Status::flags_nonzero},
        {mutate(12,1),wire::Status::header_mismatch},{mutate(16,0),wire::Status::header_mismatch},
        {mutate(20,5),wire::Status::header_mismatch},{mutate(24,0),wire::Status::header_mismatch},
        {mutate(24,wire::max_checks+1),wire::Status::header_mismatch},{mutate(28,1),wire::Status::header_mismatch},
        {mutate(96,1),wire::Status::function_order},{mutate(100,1),wire::Status::flags_nonzero},
        {mutate(96+24,0),wire::Status::invalid_function},{mutate(96+24,wire::max_source+1),wire::Status::invalid_function},
        {mutate(96+28,4097),wire::Status::invalid_function},{mutate(96+32,32),wire::Status::invalid_function},
        {mutate(96+36,2),wire::Status::invalid_function},{mutate(96+48,0),wire::Status::invalid_function},
        {mutate(96+56+36,1),wire::Status::invalid_function},
        {mutate(432+8,0),wire::Status::invalid_check},{mutate(432+8,wire::max_source+1),wire::Status::invalid_check},
        {mutate(432+12,1),wire::Status::flags_nonzero}}){
        auto result=wire::parse(test.first.data(),test.first.size());CHECK(result.status==test.second);CHECK(result.packet.functions.empty());}
    {
        auto bytes=original;bytes[0]^=1;checksum(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::magic_mismatch);
        bytes=original;bytes.back()^=1;CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::checksum_mismatch);
        bytes=original;bytes[64]^=0xff;checksum(bytes);CHECK(wire::parse(bytes.data(),bytes.size())); // Hash is provenance, not authentication.
        bytes=original;set64(bytes,32,(std::numeric_limits<std::uint64_t>::max)()-7);checksum(bytes);
        CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::header_mismatch);
        bytes=original;set64(bytes,48,0x2000);checksum(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::header_mismatch);
    }
    for(unsigned field:{8u,16u}){
        auto bytes=original;set64(bytes,96+field,0);checksum(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::invalid_function);
        bytes=original;set64(bytes,96+field,(std::numeric_limits<std::uint64_t>::max)()-1);checksum(bytes);
        CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::invalid_function);
    }
    {
        auto bytes=original;set64(bytes,96+56+8,fixture.functions[0].address);checksum(bytes);
        CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::duplicate_function);
        bytes=original;set64(bytes,432,0);checksum(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::invalid_check);
        bytes=original;set64(bytes,432,(std::numeric_limits<std::uint64_t>::max)()-7);checksum(bytes);
        CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::invalid_check);
        bytes=original;bytes[432+16]^=1;checksum(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::dispatcher_mismatch);
        bytes=original;set64(bytes,432,0x777000);checksum(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::dispatcher_missing);
        bytes=original;bytes.insert(bytes.end()-4,0);retotal(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::trailing_data);
        bytes=original;bytes.erase(bytes.end()-5);retotal(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::truncated);
    }
    {
        auto f=fixture;f.checks.push_back(f.checks[0]);auto bytes=f.encode();CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::duplicate_check);
        f=fixture;f.checks.pop_back();bytes=f.encode();CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::source_binding_mismatch);
        f=fixture;f.checks[2].bytes[0]^=1;bytes=f.encode();CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::source_binding_mismatch);
        f=fixture;f.checks[2].bytes[12]=1;bytes=f.encode();CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::source_binding_mismatch);
        bytes=original;set32(bytes,96+28,64);checksum(bytes);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::source_binding_mismatch);
        f=fixture;f.checks[4].bytes[0]^=1;bytes=f.encode();CHECK(wire::parse(bytes.data(),bytes.size())); // Live owner must compare source bytes.
    }
    {
        auto f=fixture;
        for(unsigned i=static_cast<unsigned>(f.checks.size());i<wire::max_checks;++i)f.checks.push_back({0x100000000ull+i,Bytes{0}});
        auto bytes=f.encode();CHECK(wire::parse(bytes.data(),bytes.size()));
    }
    {
        auto f=fixture;std::size_t size=original.size();std::uintptr_t address=0x200000000ull;
        while(size<wire::max_packet){
            const auto count=(std::min)(std::size_t(wire::max_source),wire::max_packet-size-16);
            CHECK(count>0);f.checks.push_back({address,Bytes(count,0)});address+=wire::max_source;size+=16+count;
        }
        auto bytes=f.encode();CHECK(bytes.size()==wire::max_packet);CHECK(wire::parse(bytes.data(),bytes.size()));
        bytes.push_back(0);CHECK(wire::parse(bytes.data(),bytes.size()).status==wire::Status::invalid_size);
    }
    std::printf("CoverCallWireTest passed: %u checks; strict bounds/order/source bindings, CRC damage checks, no authentication or authority\n",checks);
    return 0;
}
