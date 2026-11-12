#define GEARS_OWNED_SWEEP_TEST_MAIN PriorReadGroupOwnedMain
#include "cover_physx_owned_sweep_test.cpp"
#undef GEARS_OWNED_SWEEP_TEST_MAIN
static bool in_read_group{};static unsigned read_groups{},reject_group{};
struct GroupGuard:Guard {
    using Guard::operator();
    template<class Work>q::Code read_group(Work& work){
        ++read_groups;check(!in_read_group,"read groups never nest");in_read_group=true;
        struct End{~End(){in_read_group=false;}}end;
        const auto code=work(*this);if(read_groups==reject_group)return q::Code::source;return code;
    }
};
struct GroupCalls:HostCalls {
    template<class Shape>static bool sweep_stream(std::uintptr_t fn,std::uintptr_t scene,Shape const& shape,q::Vector const& movement,
        q::Hit* batch,q::detail::StreamReporter& receiver,std::uint32_t& count){
        check(!in_read_group,"sweep original dispatch outside borrowed reads");
        return HostCalls::sweep_stream(fn,scene,shape,movement,batch,receiver,count);
    }
    static bool overlap_shapes(std::uintptr_t fn,std::uintptr_t scene,q::Capsule const&,std::uintptr_t* batch,q::detail::OverlapReporter& receiver,std::uint32_t& count){
        check(!in_read_group,"overlap original dispatch outside borrowed reads");++original_calls;
        check(fn==base+0x172160&&scene==0x180000,"exact original overlap entry");
        batch[0]=shape0;batch[1]=shape1;count=2;return receiver.vtable[0](&receiver,2,batch);
    }
};
int main(int argc,char** argv){
    if(argc!=2)return 2;
    for(bool overlap:{false,true})for(unsigned reject:{0u,1u,2u}){
      auto memory=setup(argv[1]);live_memory=&memory;mode=Mode::normal;hit_count=2;original_calls=0;read_groups=0;reject_group=reject;in_read_group=false;owner_probe=nullptr;
      auto engine=std::make_unique<q::QueryEngine<HostImage,GroupCalls>>();check(engine->prime(),"prime supplied image seam");
      auto swept=std::make_unique<q::AllSweepOwners>();auto endpoint=std::make_unique<q::AllOverlapOwners>();swept->raw.raw_count=777;endpoint->raw.raw_count=777;
      auto read=[&](void* dst,void const* at,std::size_t n){return memory.read(dst,at,n);};GroupGuard guard;
      const auto code=overlap?engine->overlap_all(source(),q::Capsule{{1,2,3},{1,2,5},.5f},{},read,guard,*endpoint,&collider_input,&parts_input):
          engine->sweep_all(source(),box,delta,{},read,guard,*swept,&collider_input,&parts_input);
      const unsigned output=overlap?endpoint->raw.raw_count:swept->raw.raw_count;
      printf("group_case overlap=%u reject=%u code=%u output=%u original=%u groups=%u\n",unsigned(overlap),reject,unsigned(code),output,original_calls,read_groups);
      if(!reject)check(code==q::Code::queried&&output==2&&original_calls==1&&read_groups==2,"both source/owner read groups enclose a single original call");
      else check(code==q::Code::source&&output==777&&original_calls==(reject==1?0u:1u)&&read_groups==reject,"group rejection keeps output private and never redispatches");
      check(!in_read_group&&!engine->pending()&&!engine->uncertain(),"read refusal is not an abandoned original call");check(engine->close(),"normal image close");
    }
    printf("ReadGroupEngine checks=%u failed=%u actual collect_all grouped routing; original calls are Host seams\n",checks,failed);return failed?1:0;
}
