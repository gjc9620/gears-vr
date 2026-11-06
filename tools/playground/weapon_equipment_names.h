#pragma once
#include <array>
#include <cstdint>
#include <cwchar>

namespace gears_equipment_names {
struct Table {
    std::uintptr_t data{};
    unsigned count{},capacity{};
};
inline bool same(Table a,Table b) noexcept {
    return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;
}
enum class State {pending,ready,expired,invalid,missing};
// Only global name indices survive a step. No frame, object or matrix is cached.
class Lookup {
    Table table_{};
    unsigned cursor_{},found_{};
    std::array<unsigned,3> indices_{};
    std::array<const wchar_t*,3> labels_{L"b_MF_Hand_R",L"b_MF_Weapon_R",L"b_MF_Weapon_R_End"};
    bool known_{};
public:
    static constexpr const wchar_t* wanted[]={L"b_MF_Hand_R",L"b_MF_Weapon_R",L"b_MF_Weapon_R_End"};
    Lookup()=default;
    explicit Lookup(std::array<const wchar_t*,3> labels):labels_(labels){}
    unsigned cursor() const noexcept {return cursor_;}
    unsigned found() const noexcept {return found_;}
    void reset() noexcept {const auto labels=labels_;*this=Lookup(labels);}
    // Reads are bounded by both item count and the caller's time budget. A
    // changed table or expired source cannot commit partial lookup progress.
    template<class ReadTable,class ReadName,class Current,class Budget>
    State step(ReadTable read_table,ReadName read_name,Current current,Budget budget,
        std::array<unsigned,3>& output,unsigned maximum=512) {
        output={};
        if(!current())return State::expired;
        Table name_table{};
        if(!read_table(name_table)||!name_table.data||name_table.count<1000||
            name_table.count>name_table.capacity||name_table.capacity>4000000){reset();return State::invalid;}
        if(!known_||!same(table_,name_table)){reset();known_=true;table_=name_table;}
        auto cursor=cursor_,found=found_;auto indices=indices_;
        if(found==7){
            bool valid=true;
            for(unsigned i=0;i<3;++i){std::array<wchar_t,32> text{};
                valid=read_name(name_table,indices[i],text)&&std::wmemchr(text.data(),0,text.size())&&
                    !std::wcscmp(text.data(),labels_[i])&&valid;}
            if(!valid){reset();known_=true;table_=name_table;cursor=0;found=0;indices={};}
        }
        for(unsigned scanned=0;found!=7&&cursor<name_table.count&&scanned<maximum;++scanned){
            if((scanned&15u)==0&&!budget())break;
            std::array<wchar_t,32> text{};
            if(read_name(name_table,cursor,text)&&std::wmemchr(text.data(),0,text.size()))
                for(unsigned i=0;i<3;++i)if(!std::wcscmp(text.data(),labels_[i])){indices[i]=cursor;found|=1u<<i;}
            ++cursor;
        }
        Table after{};
        if(!current())return State::expired;
        if(!read_table(after)||!same(name_table,after)){reset();return State::pending;}
        cursor_=cursor;found_=found;indices_=indices;
        if(found==7){output=indices;return State::ready;}
        return cursor==name_table.count?State::missing:State::pending;
    }
};
} // namespace gears_equipment_names
