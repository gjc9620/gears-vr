#pragma once
#include "cover_call_trace.h"
#include <array>
#include <cstring>
#include <new>
#include <set>
#include <utility>
#include <vector>

// Pure bounded packet parsing; no file/process access or hook operations.
// CRC32 detects accidental damage, not authenticity. Provenance hashes and a
// parsed packet confer no authority. The live owner must compare every source
// check before selected observation, using its explicit owned-entry attestation
// capability only for the one original dispatcher entry span.
namespace gears_cover_call_wire {
inline constexpr unsigned version=1,function_count=6,max_checks=16384,max_source=131072;
inline constexpr std::size_t max_packet=2*1024*1024,header_size=96,function_size=56;
inline constexpr std::array<unsigned char,8> magic{{'G','C','V','R','C','A','L','L'}};
struct Check {std::uintptr_t address{};std::vector<unsigned char> bytes;};
struct Packet {
    std::uint32_t pid{};
    std::uintptr_t base{},controller{},pawn{},camera{};
    std::array<unsigned char,32> config_sha256{}; // Provenance only.
    // 0/1: cover move/updater; 2/3: stationary move/updater; 4/5: CA/CD setters.
    // Only PlayerMove indices 0/2 have selected Local descriptors.
    std::vector<gears_cover_call_trace::Function> functions;
    std::vector<Check> checks;
    bool mutation_authorized{};
};
enum class Status : unsigned {
    accepted,invalid_buffer,invalid_size,magic_mismatch,version_mismatch,flags_nonzero,
    header_mismatch,checksum_mismatch,truncated,function_order,invalid_function,
    duplicate_function,invalid_check,duplicate_check,dispatcher_missing,
    dispatcher_mismatch,source_binding_mismatch,trailing_data,allocation_failed
};
inline const char* status_name(Status value){
    static constexpr const char* names[]{"accepted","invalid_buffer","invalid_size","magic_mismatch","version_mismatch",
        "flags_nonzero","header_mismatch","checksum_mismatch","truncated","function_order","invalid_function",
        "duplicate_function","invalid_check","duplicate_check","dispatcher_missing","dispatcher_mismatch",
        "source_binding_mismatch","trailing_data","allocation_failed"};
    const auto index=static_cast<unsigned>(value);return index<sizeof(names)/sizeof(names[0])?names[index]:"unknown";
}
struct ParseResult {
    Status status{Status::invalid_buffer};Packet packet{};
    explicit operator bool()const{return status==Status::accepted;}
};
inline std::uint32_t crc32(unsigned char const* data,std::size_t size){
    std::uint32_t value=0xffffffffu;
    for(std::size_t i=0;i<size;++i){value^=data[i];for(unsigned bit=0;bit<8;++bit)
        value=(value>>1)^(0xedb88320u&(0u-(value&1u)));}
    return value^0xffffffffu;
}
namespace detail {
inline std::uint32_t u32(unsigned char const* p){
    std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::uint32_t(p[i])<<(i*8);return value;
}
inline std::uint64_t u64(unsigned char const* p){
    std::uint64_t value{};for(unsigned i=0;i<8;++i)value|=std::uint64_t(p[i])<<(i*8);return value;
}
struct Reader {
    unsigned char const* data{};std::size_t size{},position{};
    bool take(std::size_t count,unsigned char const*& out){
        if(position>size||count>size-position)return false;
        out=data+position;position+=count;return true;
    }
    bool word(std::uint32_t& out){unsigned char const* p{};if(!take(4,p))return false;out=u32(p);return true;}
    bool pointer(std::uintptr_t& out){unsigned char const* p{};if(!take(8,p))return false;out=static_cast<std::uintptr_t>(u64(p));return true;}
};
inline ParseResult rejected(Status status){return {status,{}};}
inline Check const* source(Packet const& packet,std::uintptr_t address,std::size_t size){
    for(auto const& check:packet.checks)if(check.address==address&&check.bytes.size()==size)return &check;
    return nullptr;
}
inline bool bound_sources(Packet const& packet,gears_cover_call_trace::Function const& function){
    auto const* locals=source(packet,function.address+0x88,4);
    auto const* script=source(packet,function.address+0x8c,16);
    auto const* entry=source(packet,function.address+0x114,8);
    auto const* body=source(packet,function.script,function.size);
    if(!locals||!script||!entry||!body)return false;
    const auto count=u32(script->bytes.data()+8),capacity=u32(script->bytes.data()+12);
    return u32(locals->bytes.data())==function.locals_size&&u64(script->bytes.data())==function.script&&
        count==function.size&&count<=capacity&&capacity<=max_source&&
        u64(entry->bytes.data())==packet.base+0x1d6ca0;
}
inline ParseResult parse(void const* input,std::size_t size){
    constexpr auto maximum=(std::numeric_limits<std::uintptr_t>::max)();
    if(!input)return rejected(Status::invalid_buffer);
    if(size<header_size+function_count*function_size+16+32+4||size>max_packet)return rejected(Status::invalid_size);
    auto const* data=static_cast<unsigned char const*>(input);
    if(std::memcmp(data,magic.data(),magic.size()))return rejected(Status::magic_mismatch);
    const unsigned wire_version=unsigned(data[8])|(unsigned(data[9])<<8);
    if(wire_version!=version)return rejected(Status::version_mismatch);
    if(data[10]||data[11])return rejected(Status::flags_nonzero);
    const auto total=u32(data+12),pid=u32(data+16),functions=u32(data+20),checks=u32(data+24);
    if(total!=size||!pid||functions!=function_count||!checks||checks>max_checks||u32(data+28))return rejected(Status::header_mismatch);
    if(crc32(data,size-4)!=u32(data+size-4))return rejected(Status::checksum_mismatch);
    Packet packet{};packet.pid=pid;packet.base=static_cast<std::uintptr_t>(u64(data+32));
    packet.controller=static_cast<std::uintptr_t>(u64(data+40));packet.pawn=static_cast<std::uintptr_t>(u64(data+48));
    packet.camera=static_cast<std::uintptr_t>(u64(data+56));
    if(!packet.base||(packet.base&7)||packet.base>maximum-gears_ui_call_boundary::rva-32||
       !packet.controller||(packet.controller&7)||!packet.pawn||(packet.pawn&7)||!packet.camera||(packet.camera&7)||
       packet.controller==packet.pawn||packet.controller==packet.camera||packet.pawn==packet.camera)return rejected(Status::header_mismatch);
    std::copy(data+64,data+96,packet.config_sha256.begin());
    Reader reader{data,size-4,header_size};packet.functions.reserve(function_count);
    for(unsigned i=0;i<function_count;++i){
        std::uint32_t tag{},flags{};gears_cover_call_trace::Function function{};
        if(!reader.word(tag)||!reader.word(flags)||!reader.pointer(function.address)||!reader.pointer(function.script)||
           !reader.word(function.size)||!reader.word(function.locals_size)||
           !reader.word(function.pawn_ct.offset)||!reader.word(function.pawn_ct.width)||
           !reader.word(function.pawn_ca.offset)||!reader.word(function.pawn_ca.width)||
           !reader.word(function.pawn_cd.offset)||!reader.word(function.pawn_cd.width))return rejected(Status::truncated);
        if(tag!=i)return rejected(Status::function_order);
        if(flags)return rejected(Status::flags_nonzero);
        if(!function.address||(function.address&7)||function.address>maximum-0x11c||!function.script||!function.size||
           function.size>max_source||function.script>maximum-function.size||!function.locals_size||
           function.locals_size>gears_cover_call_trace::locals_limit)return rejected(Status::invalid_function);
        for(auto const& previous:packet.functions)if(previous.address==function.address)return rejected(Status::duplicate_function);
        if(i==0||i==2){if(!gears_cover_call_trace::valid(function))return rejected(Status::invalid_function);}
        else if(function.pawn_ct.offset||function.pawn_ct.width||function.pawn_ca.offset||function.pawn_ca.width||
                function.pawn_cd.offset||function.pawn_cd.width)return rejected(Status::invalid_function);
        packet.functions.push_back(function);
    }
    packet.checks.reserve(checks);std::set<std::pair<std::uintptr_t,std::uint32_t>> seen;
    bool dispatcher{};
    for(std::uint32_t i=0;i<checks;++i){
        std::uintptr_t address{};std::uint32_t count{},flags{};unsigned char const* bytes{};
        if(!reader.pointer(address)||!reader.word(count)||!reader.word(flags))return rejected(Status::truncated);
        if(flags)return rejected(Status::flags_nonzero);
        if(!address||!count||count>max_source||address>maximum-count)return rejected(Status::invalid_check);
        if(!seen.emplace(address,count).second)return rejected(Status::duplicate_check);
        if(!reader.take(count,bytes))return rejected(Status::truncated);
        if(address==packet.base+gears_ui_call_boundary::rva&&count==gears_ui_call_boundary::entry_bytes.size()){
            if(std::memcmp(bytes,gears_ui_call_boundary::entry_bytes.data(),count))return rejected(Status::dispatcher_mismatch);
            dispatcher=true;
        }
        packet.checks.push_back({address,std::vector<unsigned char>(bytes,bytes+count)});
    }
    if(reader.position!=reader.size)return rejected(Status::trailing_data);
    if(!dispatcher)return rejected(Status::dispatcher_missing);
    for(auto const& function:packet.functions)if(!bound_sources(packet,function))return rejected(Status::source_binding_mismatch);
    return {Status::accepted,std::move(packet)};
}
}
inline ParseResult parse(void const* input,std::size_t size)noexcept{
    try{return detail::parse(input,size);}
    catch(...){return detail::rejected(Status::allocation_failed);}
}
}
