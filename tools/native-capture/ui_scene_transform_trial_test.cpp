#include "ui_scene_transform_trial.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
using namespace gears_ui_scene_trial;
struct Engine {
    std::uintptr_t canvas=0x1000;std::vector<std::array<unsigned char,80>> items;
    unsigned pushes{},pops{};bool readable=true;
    Engine(){items.reserve(17);items.resize(2);auto m=translation(2,3);std::memcpy(items.back().data(),m.data(),64);
        for(unsigned i=64;i<80;++i)items.back()[i]=static_cast<unsigned char>(i);}
    auto header()const{return gears_ui_batch::detail::Array{reinterpret_cast<std::uintptr_t>(items.data()),int(items.size()),int(items.capacity())};}
    bool read(void* out,void const* source,std::size_t size){if(!readable)return false;auto address=reinterpret_cast<std::uintptr_t>(source);
        if(address==canvas+0x38&&size==16){auto h=header();std::memcpy(out,&h,16);return true;}
        auto base=reinterpret_cast<std::uintptr_t>(items.data());if(address<base||address-base+size>items.size()*80)return false;
        std::memcpy(out,source,size);return true;}
    auto reader(){return [this](void* out,void const* source,std::size_t size){return read(out,source,size);};}
    void push(std::uintptr_t at,Matrix const& local){assert(at==canvas);++pushes;auto m=multiply(local,matrix(items.back()));
        std::array<unsigned char,80> item{};std::memcpy(item.data(),m.data(),64);item[64]=0x77;items.push_back(item);}
    void pop(std::uintptr_t at,int index){assert(at==canvas&&index==int(items.size())-1);++pops;items.pop_back();}
    auto push_fn(){return [this](auto at,auto const& m){push(at,m);};}
    auto pop_fn(){return [this](auto at,int index){pop(at,index);};}
};
static std::string ini(){std::string value="[PathChoiceTrial]\nschema=1\nnonce=19\npid=1\ncreation_time=2\nstarted_qpc=3\nworld_generation=1\nbase=0x140000000\nclient=0x1000\nscene=0x2000\nshift_x=0\nshift_y=64\nduration_ms=3000\ncheck_count=16\n";
    for(unsigned i=0;i<16;++i)value+="check_"+std::to_string(i)+"=0x3000:00010203\n";return value;}
static void parsing(){Config c;auto text=ini();assert(parse(text,c)&&c.nonce==19&&c.shift_y==64&&c.checks.size()==16);
    for(auto const& pair:std::array<std::pair<std::string,std::string>,8>{{{"schema=1","schema=2"},{"nonce=19","nonce=0"},
        {"shift_y=64","shift_y=513"},{"shift_x=0","shift_x=nan"},{"duration_ms=3000","duration_ms=8001"},
        {"scene=0x2000","scene=0x1000"},{"pid=1","pid=4294967296"},{"check_15=0x3000:00010203","check_15=0x3000:00010204"}}}){
        auto bad=text;bad.replace(bad.find(pair.first),pair.first.size(),pair.second);assert(!parse(bad,c));}
    assert(!parse(text+"nonce=20\n",c)&&!parse(text+"unknown=1\n",c));
    assert(!parse(text+std::string(1,'\0'),c));
}
static void lifecycle(){
    for(auto shift:{0,64}){Engine e;auto original=e.items.back();Lease lease;
        assert(begin(e.reader(),e.push_fn(),e.canvas,translation(0,shift),lease));
        assert(e.items.size()==3&&lease.owned&&lease.matrix_matches&&e.pushes==1);
        assert(matrix(e.items.back())[13]==float(3+shift));
        // Model a child-induced allocator move. Never restore stale data/capacity.
        e.items.reserve(128);assert(finish(e.reader(),e.pop_fn(),lease));
        assert(e.items.size()==2&&e.items.back()==original&&e.pops==1&&!lease.owned&&lease.restored);
        assert(!finish(e.reader(),e.pop_fn(),lease)&&e.pops==1);
    }
}
static void reject_mutation(){for(unsigned kind=0;kind<4;++kind){Engine e;Lease lease;
    assert(begin(e.reader(),e.push_fn(),e.canvas,translation(0,64),lease));
    if(kind==0)e.items.push_back({});if(kind==1)e.items[e.items.size()-2][4]^=1;
    if(kind==2)e.items.back()[64]^=1;if(kind==3)e.readable=false;
    assert(!finish(e.reader(),e.pop_fn(),lease)&&lease.failed&&e.pops==0);
}}
static void reject_begin(){for(unsigned kind=0;kind<4;++kind){Engine e;Lease lease;auto local=translation(0,64);
    if(kind==0)e.readable=false;if(kind==1)local[4]=std::numeric_limits<float>::quiet_NaN();
    if(kind==2){e.items.resize(e.items.capacity());auto m=translation(0,0);std::memcpy(e.items.back().data(),m.data(),64);}
    if(kind==3){auto bad=std::numeric_limits<float>::infinity();std::memcpy(e.items.back().data(),&bad,4);}
    assert(!begin(e.reader(),e.push_fn(),e.canvas,local,lease)&&e.pushes==0);
}}
static void result_and_exception(){
    for(bool throwing:{false,true}){Engine e;auto before=e.items.back();Lease lease;unsigned calls=0;
        assert(begin(e.reader(),e.push_fn(),e.canvas,translation(0,64),lease));bool caught=false;
        try{auto result=run(e.reader(),e.pop_fn(),lease,[&]()->std::uintptr_t{++calls;
            if(throwing)throw std::runtime_error("original renderer failure");return 0xfedcba9876543210ull;});
            assert(result==0xfedcba9876543210ull);
        }catch(std::runtime_error const&){caught=true;}
        assert(caught==throwing&&calls==1&&e.pops==1&&lease.restored&&e.items.back()==before);
    }
}
int main(){parsing();lifecycle();reject_mutation();reject_begin();result_and_exception();std::puts("Source transform trial: parser, zero/shift, moved allocation, corruption, finite bounds, return bits and C++ unwind restoration passed");}
