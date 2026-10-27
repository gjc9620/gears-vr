#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

// Display data only. It cannot complete a measurement, save a profile, enable
// Native, or make controller tracking current. All clocks below are UI clocks.
namespace gears_armspan_ui {
enum class Phase:std::uint32_t {waiting=1,holding=2,saved=3,cancelled=4,failed=5};
enum class Language:std::uint32_t {english=0,chinese=1};
struct Content {Phase phase{Phase::waiting};std::uint32_t progress_permille{};Language language{Language::english};};
using Nonce=std::array<unsigned char,16>;
struct Binding {std::uint64_t game_pid{},game_creation{},owner_pid{},owner_creation{};Nonce nonce{};};
struct Packet {
    char magic[8]{};
    std::uint32_t version{},byte_size{};
    std::uint64_t game_pid{},game_creation{},owner_pid{},owner_creation{},publisher_pid{},publisher_creation{};
    Nonce nonce{};
    std::uint64_t sequence{};
    std::int64_t updated_ns{};
    Content content{};
    std::uint32_t reserved{};
};
inline bool valid(Content const& value) noexcept {
    if(value.language!=Language::english&&value.language!=Language::chinese)return false;
    switch(value.phase){
    case Phase::waiting:case Phase::cancelled:case Phase::failed:return value.progress_permille==0;
    case Phase::holding:return value.progress_permille<1000;
    case Phase::saved:return value.progress_permille==1000;
    default:return false;
    }
}
inline bool same(Content const& a,Content const& b) noexcept {
    return a.phase==b.phase&&a.progress_permille==b.progress_permille&&a.language==b.language;
}
inline bool nonce_valid(Nonce const& nonce) noexcept {for(auto value:nonce)if(value)return true;return false;}
inline bool valid_pid(std::uint64_t value) noexcept {return value&&value<=0xffffffffu;}
inline bool binding_valid(Binding const& value) noexcept {
    return valid_pid(value.game_pid)&&value.game_creation&&valid_pid(value.owner_pid)&&value.owner_creation&&nonce_valid(value.nonce);
}
inline bool shape(Packet const& value,Binding const& binding) noexcept {
    return binding_valid(binding)&&std::memcmp(value.magic,"GCALUI1",8)==0&&value.version==1&&value.byte_size==sizeof(Packet)&&
        value.game_pid==binding.game_pid&&value.game_creation==binding.game_creation&&
        value.owner_pid==binding.owner_pid&&value.owner_creation==binding.owner_creation&&value.nonce==binding.nonce&&
        valid_pid(value.publisher_pid)&&value.publisher_creation&&value.sequence&&value.updated_ns>0&&
        !value.reserved&&valid(value.content);
}
inline bool fresh(Packet const& value,std::int64_t now_ns) noexcept {
    return value.updated_ns>0&&now_ns>=value.updated_ns&&now_ns-value.updated_ns<=750000000;
}
inline bool same_publication(Packet const& a,Packet const& b) noexcept {
    return a.game_pid==b.game_pid&&a.game_creation==b.game_creation&&a.owner_pid==b.owner_pid&&a.owner_creation==b.owner_creation&&
        a.publisher_pid==b.publisher_pid&&a.publisher_creation==b.publisher_creation&&a.nonce==b.nonce&&
        a.sequence==b.sequence&&a.updated_ns==b.updated_ns&&same(a.content,b.content);
}
inline bool decode_nonce(char const* text,Nonce& output) noexcept {
    if(!text||std::strlen(text)!=32)return false;
    const auto digit=[](char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;};
    Nonce candidate{};
    for(unsigned i=0;i<16;++i){const int a=digit(text[i*2]),b=digit(text[i*2+1]);if(a<0||b<0)return false;candidate[i]=static_cast<unsigned char>((a<<4)|b);}
    if(!nonce_valid(candidate))return false;output=candidate;return true;
}
static_assert(sizeof(Content)==12&&sizeof(Packet)==112&&alignof(Packet)==8);
static_assert(offsetof(Packet,nonce)==64&&offsetof(Packet,sequence)==80&&offsetof(Packet,updated_ns)==88&&offsetof(Packet,content)==96);
static_assert(std::is_standard_layout_v<Packet>&&std::is_trivially_copyable_v<Packet>);
} // namespace gears_armspan_ui
