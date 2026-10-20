#include "ngx_typed_quartet.h"
#include <memory>
#include <cstdio>
#include <stdexcept>
using namespace gears_ngx_stereo;
static unsigned checks{};
static void check(bool value){++checks;if(!value)throw std::runtime_error("quartet CPU check failed");}
struct Object {uintptr_t raw{},canonical{};uint64_t device{3};bool valid{true},texture{true};std::shared_ptr<int> life{std::make_shared<int>(0)};};
static std::array<Object,80> objects;
struct FakeLease {
    uintptr_t raw{};Resource value{};std::shared_ptr<int> life;bool texture{};
    static FakeLease observe(ID3D12Resource* typed){
        for(auto& o:objects)if(o.raw==reinterpret_cast<uintptr_t>(typed)){
            if(!o.valid)return {};
            return {o.raw,{o.canonical,o.device,{3840,2160},26,0,1,0},o.life,o.texture};
        }return {};
    }
    bool retained()const{return bool(life);}
    bool materialize(Resource& out)const{if(!retained()||!texture)return false;out=value;return true;}
};
using Q=BasicTypedQuartet<FakeLease>;
static Identity id(){return {1,2,3,4,5};}
static ID3D12Resource* resource(unsigned n){return reinterpret_cast<ID3D12Resource*>(objects[n].raw);}
static void reset(){for(unsigned i=0;i<objects.size();++i)objects[i]={uintptr_t(0x100+i*16),uintptr_t(0x1000+i*16)};}
static std::array<uintptr_t,4> raw(){return {objects[0].raw,objects[1].raw,objects[2].raw,objects[3].raw};}
static void fill(Q& q,uint64_t ticket=7){for(unsigned i=0;i<4;++i)check(q.observe_resource(id(),ticket,resource(i)));}
int main(){
 try{
    reset();std::array<Resource,4> out{};
    {Q q;check(q.begin(id()));
     D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=resource(3);
     check(acquire_quartet_barrier(q,id(),7,b));check(q.held()==1&&objects[3].life.use_count()==2);
     b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=resource(0);check(acquire_quartet_barrier(q,id(),7,b));
     b.Type=D3D12_RESOURCE_BARRIER_TYPE_ALIASING;b.Aliasing.pResourceBefore=resource(1);b.Aliasing.pResourceAfter=resource(2);
     check(acquire_quartet_barrier(q,id(),7,b));check(q.bind(id(),7,raw(),out));
     check(out[0].identity==objects[0].canonical&&out[1].identity==objects[2].canonical&&out[2].identity==objects[3].canonical&&out[3].identity==objects[1].canonical);
     check(q.bound()&&q.held()==4);for(unsigned i=0;i<4;++i)check(objects[i].life.use_count()==2);
     for(unsigned i=4;i<80;++i)check(q.observe_resource(id(),8,resource(i)));check(q.held()==4&&objects[79].life.use_count()==1);
     check(q.bind(id(),8,raw(),out));q.uncertain();check(!q.bind(id(),9,raw(),out));check(q.held()==4);
     // Uncertain/reset/stop does not imply detach or release.
     for(unsigned i=0;i<4;++i)check(objects[i].life.use_count()==2);
    }for(unsigned i=0;i<4;++i)check(objects[i].life.use_count()==1);
    {Q q;check(q.begin(id()));fill(q);check(q.observe_resource(id(),8,resource(0)));check(q.held()==1&&objects[1].life.use_count()==1);check(!q.bind(id(),8,raw(),out));}
    for(unsigned field=0;field<5;++field){Q q;check(q.begin(id()));fill(q);check(q.bind(id(),7,raw(),out));auto changed=id();
        switch(field){case 0:++changed.epoch;break;case 1:++changed.world;break;case 2:++changed.device;break;case 3:++changed.parameters;break;case 4:++changed.layout;break;}
        check(!q.bind(changed,8,raw(),out)&&q.failed()&&q.held()==4);
    }
    {Q q;check(q.begin(id()));fill(q);check(q.bind(id(),7,raw(),out));auto changed=raw();changed[0]=objects[4].raw;check(!q.bind(id(),8,changed,out)&&q.held()==4);}
    {Q q;check(q.begin(id()));fill(q);check(!q.bind(id(),6,raw(),out));}
    {Q q;check(q.begin(id()));fill(q);check(!q.begin(id())&&q.held()==4);}
    {Q q;check(q.begin(id()));for(unsigned i=0;i<64;++i)check(q.observe_resource(id(),7,resource(i)));
     // Full bank: typed but ineligible buffers/arrays and foreign-device textures
     // are still ignored; their temporary observation lease must be released.
     objects[64].texture=false;check(q.observe_resource(id(),7,resource(64))&&!q.failed()&&q.held()==64);
     check(objects[64].life.use_count()==1);objects[64].texture=true;
     objects[65].device=99;check(q.observe_resource(id(),7,resource(65))&&!q.failed()&&q.held()==64);
     check(objects[65].life.use_count()==1);objects[65].device=3;
     check(q.observe_resource(id(),7,resource(0))&&!q.failed()); // Existing eligible identity costs no slot.
     check(!q.observe_resource(id(),7,resource(64))&&q.failed()&&q.held()==64);
     check(objects[64].life.use_count()==1); // Rejected eligible candidate also drops its temporary lease.
     check(!q.observe_resource(id(),8,resource(66))&&q.held()==64); // Failure stays sticky; no ticket-clearing escape.
    }
    {Q q;check(q.begin(id()));objects[0].valid=false;check(!q.observe_resource(id(),7,resource(0)));objects[0].valid=true;}
    {Q q;check(q.begin(id()));objects[0].device=99;fill(q);check(q.held()==3&&!q.bind(id(),7,raw(),out));objects[0].device=3;}
    {Q q;check(q.begin(id()));objects[2].canonical=objects[0].canonical;fill(q);check(!q.bind(id(),7,raw(),out));}reset();
    {Q q;check(q.begin(id()));D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;check(acquire_quartet_barrier(q,id(),7,b)&&q.held()==0);}
    {Q q;check(q.begin(id()));objects[0].texture=false;check(q.observe_resource(id(),7,resource(0))&&q.held()==0);objects[0].texture=true;}
    std::printf("quartet CPU ownership: %u checks passed; no D3D device/GPU work\n",checks);return 0;
 }catch(std::exception const& e){std::fprintf(stderr,"check %u: %s\n",checks,e.what());return 1;}
}
