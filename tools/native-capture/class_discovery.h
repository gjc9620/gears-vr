#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

// Discovery hints only. The caller still independently proves every selected
// identity, registry slot, name, property, script and link before publication.
namespace gears_class_discovery {
using Address=std::uint64_t;
struct RegistryChanged {};
struct Aborted {};
constexpr std::uint32_t abi_version=1,limit=4000000;
template<class Read> class Scan {
    Read read_;
    std::vector<Address> const& names_;
    Address names_header_{};
    Address block_{};bool block_valid_{};
    std::array<unsigned char,65536> block_bytes_{};
    std::unordered_map<Address,std::array<unsigned char,4096>> pages_;
    std::deque<Address> page_order_;
    std::unordered_map<Address,bool> kinds_;
    std::unordered_map<std::uint32_t,bool> name_kinds_;
    static bool range(Address a,std::size_t n){return a&&n&&a<=UINT64_MAX-n;}
    bool exact(void* out,Address a,std::size_t n){return range(a,n)&&read_(out,a,n);}
    template<class T> bool value(Address a,T& out){return exact(&out,a,sizeof(out));}
    bool hint(void* out,Address a){
        if(!range(a,0x60))return false;
        if(block_valid_&&a>=block_&&a-block_<=block_bytes_.size()-0x60){
            std::memcpy(out,block_bytes_.data()+std::size_t(a-block_),0x60);return true;
        }
        const auto page=a-a%4096,offset=a-page;
        if(offset+0x60>4096)return exact(out,a,0x60);
        auto found=pages_.find(page);
        if(found==pages_.end()){
            std::array<unsigned char,4096> data{};
            if(!exact(data.data(),page,data.size()))return exact(out,a,0x60);
            if(pages_.size()>=64){pages_.erase(page_order_.front());page_order_.pop_front();}
            found=pages_.emplace(page,std::move(data)).first;page_order_.push_back(page);
        }
        std::memcpy(out,found->second.data()+std::size_t(offset),0x60);return true;
    }
    static bool valid_utf16(unsigned char const* raw,std::size_t n){
        for(std::size_t i=0;i<n;i+=2){
            const auto x=std::uint16_t(raw[i])|(std::uint16_t(raw[i+1])<<8);
            if(x>=0xdc00&&x<=0xdfff)return false;
            if(x>=0xd800&&x<=0xdbff){
                if(i+3>=n)return false;
                i+=2;const auto y=std::uint16_t(raw[i])|(std::uint16_t(raw[i+1])<<8);
                if(y<0xdc00||y>0xdfff)return false;
            }
        }return true;
    }
    std::optional<bool> class_name(std::uint32_t index){
        auto cached=name_kinds_.find(index);if(cached!=name_kinds_.end())return cached->second;
        Address entry{};
        if(index<names_.size())entry=names_[index];
        else{
            struct Header {Address data;std::int32_t count,capacity;} before{},after{};
            if(!value(names_header_,before)||!before.data||before.count<=1000||
               before.count>before.capacity||before.capacity>=std::int32_t(limit)||index>=std::uint32_t(before.count))return {};
            if(before.data>UINT64_MAX-Address(index)*8||!value(before.data+Address(index)*8,entry))return {};
            if(!value(names_header_,after))return {};
            if(std::memcmp(&before,&after,sizeof(before)))throw RegistryChanged{};
        }
        std::uint32_t actual{};
        if(!value(entry,actual)||actual!=index||entry>UINT64_MAX-148)return {};
        std::array<unsigned char,128> raw{};std::size_t length{};
        while(length<raw.size()){
            const auto address=entry+20+length;
            const auto n=(std::min)(raw.size()-length,std::size_t(4096-address%4096));
            if(!exact(raw.data()+length,address,n))return {};
            length+=n;
            for(std::size_t i=0;i+1<length;i+=2)if(!raw[i]&&!raw[i+1]){
                if(!valid_utf16(raw.data(),i))return {};
                static constexpr unsigned char expected[]={'C',0,'l',0,'a',0,'s',0,'s',0};
                const bool result=i==sizeof(expected)&&!std::memcmp(raw.data(),expected,sizeof(expected));
                name_kinds_[index]=result;return result;
            }
        }return {};
    }
public:
    Scan(Read read,std::vector<Address> const& names,Address header):read_(read),names_(names),names_header_(header){}
    std::vector<Address> run(std::vector<Address> pointers){
        pointers.erase(std::remove(pointers.begin(),pointers.end(),0),pointers.end());
        std::sort(pointers.begin(),pointers.end());std::vector<Address> result;
        bool have_block=false;
        for(auto a:pointers){
            const auto block=a-a%65536;
            if(!have_block||block!=block_){block_=block;have_block=true;block_valid_=exact(block_bytes_.data(),block_,block_bytes_.size());}
            std::array<unsigned char,0x60> object{};
            if(!hint(object.data(),a))continue;
            Address cls{};std::memcpy(&cls,object.data()+80,8);if(!cls)continue;
            auto kind=kinds_.find(cls);
            if(kind==kinds_.end()){
                if(!hint(object.data(),cls))continue;
                std::uint32_t index{};std::memcpy(&index,object.data()+72,4);
                const auto is_class=class_name(index);if(!is_class)continue;
                kind=kinds_.emplace(cls,*is_class).first;
            }
            if(kind->second)result.push_back(a);
        }return result;
    }
};
template<class Read> std::vector<Address> discover(std::vector<Address> objects,std::vector<Address> const& names,Address header,Read read){
    return Scan<Read>(read,names,header).run(std::move(objects));
}
}
