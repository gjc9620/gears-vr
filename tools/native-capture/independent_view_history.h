#pragma once
#include <atomic>
#include <cstdint>
#include <cstdio>

namespace gears_view_history {
// Owns identity/lifecycle evidence only. Native factory/release retain their
// resource registration and deferred-delete semantics; never memcpy or free.
class State {
    std::atomic<std::uintptr_t> pointer_{};
    std::atomic<unsigned> attempted_{},created_{},release_started_{},release_returned_{};
    std::atomic<unsigned> delete_started_{},destroyed_{},delete_flags_{},failure_{};
public:
    std::uintptr_t pointer()const{return pointer_.load();}
    bool ready()const{return created_.load()==1&&!release_started_.load()&&!delete_started_.load();}
    template<class Factory,class Read>
    bool ensure(Factory factory,Read read,std::uintptr_t expected_table,
                std::uintptr_t original,std::uintptr_t right){
        if(attempted_.exchange(1))return ready()&&pointer()!=original&&pointer()!=right;
        if(!factory||!read||!expected_table||!original||!right||original==right){failure_=1;return false;}
        void* value=factory();pointer_=reinterpret_cast<std::uintptr_t>(value);
        if(!value){failure_=2;return false;}
        if(pointer()==original||pointer()==right){failure_=3;return false;}
        std::uintptr_t table{};
        if(!read(&table,value,sizeof(table))||table!=expected_table){failure_=4;return false;}
        created_=1;return true;
    }
    template<class Release> void release(Release native_release){
        if(!created_.load())return;
        unsigned fresh=0;
        if(!release_started_.compare_exchange_strong(fresh,1))return;
        native_release(reinterpret_cast<void*>(pointer()));release_returned_=1;
    }
    bool begin_delete(void* value,unsigned flags){
        if(reinterpret_cast<std::uintptr_t>(value)!=pointer()||!created_.load())return false;
        unsigned fresh=0;
        if(!delete_started_.compare_exchange_strong(fresh,1))return false;
        delete_flags_=flags;return true;
    }
    void complete_delete(){destroyed_=1;}
    bool retired()const{
        if(!attempted_.load())return true;
        return created_.load()==1&&release_started_.load()==1&&release_returned_.load()==1&&
            delete_started_.load()==1&&destroyed_.load()==1&&(delete_flags_.load()&1);
    }
    bool reset(){
        if(!retired())return false;
        pointer_=0;attempted_=created_=release_started_=release_returned_=0;
        delete_started_=destroyed_=delete_flags_=failure_=0;return true;
    }
    void print(FILE* output)const{
        fprintf(output,"left_history_attempted=%u left_history_created=%u left_history_release_started=%u left_history_release_returned=%u left_history_delete_started=%u left_history_destroyed=%u left_history_delete_flags=%u left_history_failure=%u ",
            attempted_.load(),created_.load(),release_started_.load(),release_returned_.load(),
            delete_started_.load(),destroyed_.load(),delete_flags_.load(),failure_.load());
    }
};
}
