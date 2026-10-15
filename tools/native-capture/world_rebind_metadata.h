#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <iterator>
#include <type_traits>

namespace gears_world_rebind {
struct Expected {
    std::uint32_t game_pid{},owner_pid{};
    std::uint64_t game_creation{},owner_creation{},generation{};
    std::uintptr_t module_base{},local_player{};
};
struct Metadata {
    std::uint64_t previous_generation{},generation{};
    std::uintptr_t local_player{},controller{},camera{},timestamp{},camera_yaw{};
};
inline constexpr wchar_t game_sha256[]=L"bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4";
namespace detail {
// GetPrivateProfileString-compatible lengths: cap-1 may mean truncation.
template<class Read> bool text(Read& read,wchar_t const* section,wchar_t const* key,wchar_t (&buffer)[96],std::size_t& length){
    for(auto& character:buffer)character=0;
    const auto count=read(section,key,buffer,std::size(buffer));
    if(count<=0||static_cast<std::uint64_t>(count)>=std::size(buffer)-1)return false;
    length=static_cast<std::size_t>(count);
    if(buffer[length])return false;
    for(std::size_t i=0;i<length;++i)if(!buffer[i])return false;
    return true;
}
template<class T> bool unsigned_number(wchar_t const* text,std::size_t length,T& output){
    static_assert(std::is_unsigned_v<T>);
    if(!length)return false;
    unsigned radix=10;std::size_t index=0;
    if(length>=2&&text[0]==L'0'&&(text[1]==L'x'||text[1]==L'X')){radix=16;index=2;}
    if(index==length)return false;
    T value{};const auto maximum=(std::numeric_limits<T>::max)();
    for(;index<length;++index){
        const auto character=text[index];unsigned digit{};
        if(character>=L'0'&&character<=L'9')digit=unsigned(character-L'0');
        else if(character>=L'a'&&character<=L'f')digit=unsigned(character-L'a')+10;
        else if(character>=L'A'&&character<=L'F')digit=unsigned(character-L'A')+10;
        else return false;
        if(digit>=radix||value>(maximum-digit)/radix)return false;
        value=T(value*radix+digit);
    }
    output=value;return true;
}
template<class Read,class T> bool number(Read& read,wchar_t const* section,wchar_t const* key,T& output){
    wchar_t buffer[96]{};std::size_t length{};
    return text(read,section,key,buffer,length)&&unsigned_number(buffer,length,output);
}
}
// Prepare-stage identity gate only. This neither validates live UObject
// structure nor issues a recovery certificate or authorizes a commit.
template<class Read> bool parse(Read read,Expected const& expected,Metadata& output){
    if(!expected.game_pid||!expected.owner_pid||expected.game_pid==expected.owner_pid||
       !expected.game_creation||!expected.owner_creation||!expected.generation||
       expected.generation==(std::numeric_limits<std::uint64_t>::max)()||!expected.module_base||!expected.local_player)return false;
    auto same=[&](wchar_t const* section,wchar_t const* key,auto wanted){
        decltype(wanted) actual{};return detail::number(read,section,key,actual)&&actual==wanted;
    };
    constexpr auto world=L"WorldRebind",capture=L"Capture";
    Metadata candidate{};
    if(!same(world,L"schema",std::uint32_t(1))||!same(world,L"game_pid",expected.game_pid)||
       !same(world,L"game_creation",expected.game_creation)||!same(world,L"owner_pid",expected.owner_pid)||
       !same(world,L"owner_creation",expected.owner_creation)||!same(world,L"module_base",expected.module_base)||
       !detail::number(read,world,L"previous_generation",candidate.previous_generation)||candidate.previous_generation!=expected.generation||
       !detail::number(read,world,L"generation",candidate.generation)||candidate.generation!=expected.generation+1)return false;
    wchar_t hash[96]{};std::size_t hash_length{};
    if(!detail::text(read,world,L"game_sha256",hash,hash_length)||hash_length!=64)return false;
    for(std::size_t i=0;i<hash_length;++i)if(hash[i]!=game_sha256[i])return false;
    if(!same(capture,L"pid",expected.game_pid)||!detail::number(read,capture,L"local_player",candidate.local_player)||
       candidate.local_player!=expected.local_player||!detail::number(read,capture,L"controller",candidate.controller)||!candidate.controller||
       !detail::number(read,capture,L"camera",candidate.camera)||!candidate.camera||
       !detail::number(read,capture,L"timestamp",candidate.timestamp)||!candidate.timestamp||
       !detail::number(read,capture,L"camera_yaw",candidate.camera_yaw)||!candidate.camera_yaw)return false;
    output=candidate;return true;
}
}
