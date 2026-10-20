#include "ngx_mv_sdk_receipt.h"
#include <cassert>
#include <initializer_list>
#include <cstdio>
using gears_ngx_mv_target::SdkReceipt;
int main(){
    for(auto raw:{0u,1u,2u,0x80000000u,0xbae00000u}){
        SdkReceipt r;assert(r.begin(1,2,3));assert(r.dispatch_enter(1,2,3,0));
        assert(r.dispatch_leave(true,raw));assert(r.finish(true,false,(1ull<<32)|raw));assert(r.raw_result()==raw);
        assert(!r.finish(true,false,raw));
    }
    for(unsigned bad=0;bad<11;++bad){
        SdkReceipt r;assert(r.begin(1,2,3));
        if(bad==0){assert(!r.finish(true,false,1));continue;}
        if(bad==1){assert(!r.begin(1,2,3));continue;}
        if(bad==2){assert(!r.dispatch_enter(4,2,3,0));continue;}
        if(bad==3){assert(!r.dispatch_enter(1,4,3,0));continue;}
        if(bad==4){assert(!r.dispatch_enter(1,2,4,0));continue;}
        if(bad==5){assert(!r.dispatch_enter(1,2,3,4));continue;}
        assert(r.dispatch_enter(1,2,3,0));
        if(bad==6){assert(!r.dispatch_enter(1,2,3,0));assert(!r.finish(true,false,1));continue;}
        assert(r.dispatch_leave(true,bad==7?0xbad00001u:1));
        assert(!r.finish(bad!=8,bad==9,bad==10?2:1));
    }
    SdkReceipt no_return;assert(no_return.begin(1,2,3));assert(no_return.dispatch_enter(1,2,3,0));
    assert(!no_return.dispatch_leave(false,1));assert(!no_return.finish(true,false,1));
    SdkReceipt duplicate;assert(duplicate.begin(1,2,3));assert(duplicate.dispatch_enter(1,2,3,0));
    assert(duplicate.dispatch_leave(true,1));assert(!duplicate.dispatch_leave(true,1));assert(!duplicate.finish(true,false,1));
    std::puts("MV SDK receipt boundaries PASS");
}
