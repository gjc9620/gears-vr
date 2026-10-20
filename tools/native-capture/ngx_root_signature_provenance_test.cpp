#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ngx_root_signature_provenance.h"
#include <cassert>
#include <cstdio>
#include <memory>
using namespace gears_ngx_root;
int main(){
    std::array<unsigned char,blob_limit+1> bytes{};bytes[0]=3;
    Bank bank;assert(bank.append(1,2,0,bytes.data(),8)==Result::added);
    assert(bank.append(1,2,0,bytes.data(),8)==Result::duplicate&&bank.count()==1&&bank.bytes()==8);
    auto changed=bytes;changed[2]=1;assert(!unchanged(bytes.data(),changed.data(),8));
    assert(bank.append(1,2,0,changed.data(),8)==Result::conflict&&bank.incomplete());
    assert(bank.append(1,3,0,bytes.data(),8)==Result::conflict);
    assert(bank.append(1,2,1,bytes.data(),8)==Result::conflict);
    assert(bank.append(2,2,0,bytes.data(),blob_limit+1)==Result::invalid);
    assert(bank.append(2,2,0,nullptr,8)==Result::invalid);
    auto counts=std::make_unique<Bank>();for(uintptr_t i=1;i<=root_limit;++i)assert(counts->append(i,2,0,bytes.data(),1)==Result::added);
    assert(counts->append(100,2,0,bytes.data(),1)==Result::capacity&&counts->count()==root_limit);
    assert(counts->append(1,2,0,bytes.data(),1)==Result::duplicate);
    auto total=std::make_unique<Bank>();for(uintptr_t i=1;i<=4;++i)assert(total->append(i,2,0,bytes.data(),blob_limit)==Result::added);
    assert(total->bytes()==byte_limit&&!total->room(1)&&total->append(5,2,0,bytes.data(),1)==Result::capacity);
    assert(total->incomplete()&&total->find(1)->size==blob_limit);
    std::puts("Root signature provenance CPU bounds passed; serialized bytes only, no shader/GPU proof");
}
