#include "pawnless_camera_observer.h"
#include <cstdio>
#include <functional>
#include <memory>
#define CHECK(v) do{if(!(v)){std::printf("failed line %d: %s\n",__LINE__,#v);return 1;}}while(0)
namespace pc=gears_pawnless_camera;
struct Fixture {
    gears_anchor::Config config{};std::array<unsigned char,0x20000> memory{};
    std::function<void(std::uintptr_t,std::size_t)> after_read;
    unsigned calls{},invalid_reads{},target_reads{},reverse_reads{};std::uintptr_t fail{};
    const std::uintptr_t lp=0x1000,controller=0x2000,camera=0x3000,level=0x4000,objects=0x6000,names=0x9000;
    const std::uintptr_t opaque_target=0xfedcba9876543210ull;
    template<class T> void put(std::uintptr_t address,T const& value){std::memcpy(memory.data()+address,&value,sizeof(value));}
    Fixture(){
        auto& c=config;c.objects_header=0x100;c.names_header=0x200;c.identity_count=5;
        c.role_local=0;c.role_controller=1;c.role_camera=2;c.role_level=3;c.role_pawn=4;
        c.object_internal_index=8;c.object_name=12;c.object_class=24;c.object_outer=32;
        c.controller_pawn=128;c.player_camera=136;c.player=144;c.pc_owner=128;c.camera_view_target=136;c.camera_timestamp=144;c.camera_pov=148;
        put(c.objects_header,gears_anchor::Registry{objects,1101,1200});put(c.names_header,gears_anchor::Registry{names,1101,1200});
        const std::uintptr_t addresses[]{lp,controller,camera,level};
        for(unsigned i=0;i<4;++i){auto& id=c.identities[i];id.address=addresses[i];id.cls=0x18000+i*0x100;id.outer=level;
            id.index=5+i;id.name_index=10+i;id.number=i;id.name[0]=L'x';
            put(id.address+8,id.index);put(id.address+12,id.name_index);put(id.address+16,id.number);put(id.address+24,id.cls);put(id.address+32,id.outer);
            put(objects+id.index*8,id.address);put(names+id.name_index*8,std::uintptr_t(0x19000+i*0x100));}
        put(controller+c.controller_pawn,std::uintptr_t(0));put(controller+c.player_camera,camera);
        put(controller+c.player,lp);put(camera+c.pc_owner,controller);put(camera+c.camera_view_target,opaque_target);
        put(camera+c.camera_timestamp,1.f);put(camera+c.camera_pov,std::array<float,3>{1.f,2.f,3.f});put(camera+c.camera_pov+24,90.f);
    }
    bool read(void* dest,void const* source,std::size_t size)noexcept{
        ++calls;const auto address=reinterpret_cast<std::uintptr_t>(source);
        if(address==opaque_target)++target_reads;if(address==lp+0x78)++reverse_reads;
        if(address==fail){std::memset(dest,0,size);return false;}
        if(address>memory.size()||size>memory.size()-std::size_t(address)){++invalid_reads;return false;}
        std::memcpy(dest,memory.data()+address,size);if(after_read)after_read(address,size);return true;
    }
    auto reader(){return [this](void* d,void const* s,std::size_t n)noexcept{return read(d,s,n);};}
};
int main(){
    auto f=std::make_unique<Fixture>();pc::Snapshot s{};
    f->put(f->controller+f->config.controller_pawn,std::uintptr_t(0x5000));
    CHECK(!pc::observe(f->config,f->reader(),s)&&s.config_valid&&s.initial_pawn_read&&!s.sampled&&s.initial_pawn==0x5000&&f->calls==1);
    f->fail=f->controller+f->config.controller_pawn;f->calls=0;
    CHECK(!pc::observe(f->config,f->reader(),s)&&!s.initial_pawn_read&&!s.sampled&&s.initial_pawn==0&&f->calls==1);
    f->fail=0;f->put(f->controller+f->config.controller_pawn,std::uintptr_t(0));
    CHECK(pc::observe(f->config,f->reader(),s)&&s.sampled&&s.initial_pawn_read&&s.reads_complete&&s.reads_stable);
    CHECK(s.before.read_mask==pc::required_mask&&s.after.read_mask==pc::required_mask&&s.stable_mask==pc::required_mask);
    CHECK(s.before.identity_match_mask==15&&s.after.identity_match_mask==15&&s.before.registry_valid_mask==3);
    CHECK(s.before.link_match_mask==((1u<<pc::controller_pawn)|(1u<<pc::player_camera)|(1u<<pc::pc_owner)|(1u<<pc::player)));
    CHECK(s.before.links[pc::view_target]==f->opaque_target&&!f->target_reads&&!f->reverse_reads);
    CHECK(s.before.finite_location&&s.before.valid_fov&&s.before.finite_timestamp&&s.before.location[2]==3.f&&s.before.fov==90.f);
    CHECK(s.expected[pc::local]==f->lp&&s.expected[pc::controller]==f->controller&&s.expected[pc::camera]==f->camera&&s.expected[pc::level]==f->level);
    f->fail=f->camera;CHECK(pc::observe(f->config,f->reader(),s)&&s.sampled&&!s.reads_complete&&!s.reads_stable);
    CHECK(!(s.before.read_mask&(1u<<(pc::header_bit+pc::camera)))&&!(s.before.identity_match_mask&(1u<<pc::camera)));
    for(unsigned fault=0;fault<11;++fault){auto n=std::make_unique<Fixture>();bool changed=false;
        n->after_read=[&](std::uintptr_t address,std::size_t size){if(changed||address!=n->camera+n->config.camera_pov||size!=28)return;changed=true;
            switch(fault){
            case 0:n->put(n->controller+n->config.player,std::uintptr_t(0));break;
            case 1:n->put(n->config.objects_header,gears_anchor::Registry{n->objects,1101,1201});break;
            case 2:n->put(n->config.names_header,gears_anchor::Registry{n->names,1101,1201});break;
            case 3:n->put(n->camera+n->config.object_class,std::uintptr_t(0));break;
            case 4:n->put(n->objects+n->config.identities[2].index*8,std::uintptr_t(0));break;
            case 5:n->put(n->camera+n->config.camera_timestamp,2.f);break;
            case 6:n->put(n->camera+n->config.camera_pov,std::numeric_limits<float>::quiet_NaN());break;
            case 7:n->put(n->camera+n->config.camera_pov+24,180.f);break;
            case 8:n->put(n->controller+n->config.controller_pawn,std::uintptr_t(0x5000));break;
            case 9:n->put(n->names+n->config.identities[2].name_index*8,std::uintptr_t(0));break;
            case 10:n->put(n->camera+n->config.camera_view_target,n->opaque_target+8);break;
            }};
        CHECK(pc::observe(n->config,n->reader(),s)&&changed&&s.reads_complete&&!s.reads_stable);
        if(fault==0)CHECK(!(s.after.link_match_mask&(1u<<pc::player)));
        if(fault==3||fault==4)CHECK(!(s.after.identity_match_mask&(1u<<pc::camera)));
        if(fault==5)CHECK(!(s.stable_mask&(1u<<pc::timestamp_bit)));
        if(fault==6)CHECK(!s.after.finite_location);
        if(fault==7)CHECK(!s.after.valid_fov);
        if(fault==8)CHECK(s.after.links[pc::controller_pawn]&&!s.initial_pawn);
        if(fault==9)CHECK(!(s.after.identity_match_mask&(1u<<pc::camera))&&!(s.stable_mask&(1u<<(pc::name_slot_bit+pc::camera))));
        if(fault==10)CHECK(s.after.links[pc::view_target]==n->opaque_target+8&&!n->invalid_reads);
    }
    // Complete, stable all-zero bytes are not a valid expected UObject identity.
    auto zero=std::make_unique<Fixture>();std::memset(zero->memory.data()+zero->camera,0,96);
    CHECK(pc::observe(zero->config,zero->reader(),s)&&s.reads_complete&&s.reads_stable&&!(s.before.identity_match_mask&(1u<<pc::camera)));
    for(float fov:{1.f,179.f,0.f,180.f,std::numeric_limits<float>::quiet_NaN()}){auto n=std::make_unique<Fixture>();n->put(n->camera+n->config.camera_pov+24,fov);
        CHECK(pc::observe(n->config,n->reader(),s));CHECK(s.before.valid_fov==(fov==1.f||fov==179.f));}
    auto nan_time=std::make_unique<Fixture>();nan_time->put(nan_time->camera+nan_time->config.camera_timestamp,std::numeric_limits<float>::quiet_NaN());
    CHECK(pc::observe(nan_time->config,nan_time->reader(),s)&&s.reads_stable&&!s.before.finite_timestamp&&!s.after.finite_timestamp);
    for(unsigned fault=0;fault<7;++fault){auto n=std::make_unique<Fixture>();auto& c=n->config;const auto limit=(std::numeric_limits<std::uintptr_t>::max)();
        switch(fault){
        case 0:c.role_camera=128;break;
        case 1:c.object_name=(std::numeric_limits<unsigned>::max)();break;
        case 2:c.identities[c.role_controller].address=limit-4;break;
        case 3:c.identities[c.role_camera].address=limit-8;break;
        case 4:n->put(c.objects_header,gears_anchor::Registry{limit-7,1101,1200});break;
        case 5:c.identities[c.role_camera].index=(std::numeric_limits<unsigned>::max)();break;
        case 6:c.camera_pov=(std::numeric_limits<unsigned>::max)();break;
        }
        const bool sampled=pc::observe(c,n->reader(),s);CHECK(!n->invalid_reads);
        if(fault==3||fault==4)CHECK(sampled&&s.before.range_rejected_mask&&!s.reads_complete);
        else CHECK(!sampled&&!s.sampled);
        if(fault==2)CHECK(s.initial_pawn_range_rejected&&!s.initial_pawn_read);
    }
    std::printf("pawnless camera observer passed; Snapshot=%zu bytes, observation only\n",sizeof(pc::Snapshot));
}
