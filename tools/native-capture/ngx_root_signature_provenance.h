#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace gears_ngx_root {
constexpr std::size_t root_limit=16,blob_limit=64u*1024u,byte_limit=256u*1024u;
enum class Result {added,duplicate,conflict,capacity,invalid};
struct Record {uintptr_t canonical{},device{};unsigned node_mask{};std::size_t offset{},size{};bool fixture{};};
inline bool unchanged(void const* before,void const* after,std::size_t size){return before&&after&&size&&!std::memcmp(before,after,size);}
class Bank {
    std::array<Record,root_limit> roots_{};std::array<unsigned char,byte_limit> bytes_{};
    std::size_t count_{},used_{};bool incomplete_{};
public:
    std::size_t count()const{return count_;}std::size_t bytes()const{return used_;}bool incomplete()const{return incomplete_;}
    void uncertain(){incomplete_=true;}
    bool room(std::size_t size)const{return size&&size<=blob_limit&&count_<root_limit&&size<=byte_limit-used_;}
    Record const* find(uintptr_t canonical)const{for(std::size_t i=0;i<count_;++i)if(roots_[i].canonical==canonical)return &roots_[i];return nullptr;}
    Record const& row(std::size_t i)const{return roots_[i];}
    unsigned char const* data(Record const& row)const{return bytes_.data()+row.offset;}
    Result append(uintptr_t canonical,uintptr_t device,unsigned node,void const* bytes,std::size_t size,bool fixture=false){
        if(!canonical||!device||!bytes||!size||size>blob_limit){uncertain();return Result::invalid;}
        if(auto* old=find(canonical)){
            if(old->device==device&&old->node_mask==node&&old->size==size&&old->fixture==fixture&&unchanged(data(*old),bytes,size))return Result::duplicate;
            uncertain();return Result::conflict;
        }
        if(!room(size)){uncertain();return Result::capacity;}
        roots_[count_++]={canonical,device,node,used_,size,fixture};std::memcpy(bytes_.data()+used_,bytes,size);used_+=size;return Result::added;
    }
};
}
