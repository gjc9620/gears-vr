#include "graphics_lock_routes.h"
#include <cstdio>
#include <map>
#include <memory>
#include <thread>
#include <vector>

namespace lock=gears_graphics_lock;
namespace route=gears_graphics_lock::routes;
#define CHECK(value) do{if(!(value)){std::printf("Failed line %d: %s\n",__LINE__,#value);return 1;}}while(0)
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> data;unsigned writes{};bool reject_write{};
    unsigned char* address(std::uintptr_t at,std::size_t size) {
        for(auto& row:data)if(at>=row.first&&at-row.first<=row.second.size()&&size<=row.second.size()-(at-row.first))return row.second.data()+at-row.first;
        return nullptr;
    }
    template<class T>void put(std::uintptr_t at,T value){std::memcpy(address(at,sizeof(value)),&value,sizeof(value));}
    template<class T>T get(std::uintptr_t at){T value{};std::memcpy(&value,address(at,sizeof(value)),sizeof(value));return value;}
    bool read(std::uintptr_t at,void* out,std::size_t size){auto p=address(at,size);if(!p)return false;std::memcpy(out,p,size);return true;}
    bool cas(std::uintptr_t at,std::uint32_t old,std::uint32_t value) {
        if(reject_write||!address(at,4)||get<std::uint32_t>(at)!=old)return false;
        put(at,value);++writes;return true;
    }
};
int main() {
    constexpr std::uintptr_t singleton=0x100000,table=0x200000,command=0x300000,command_table=0x400000;
    Memory memory;memory.data[singleton]=std::vector<unsigned char>(0x600,0x5a);
    memory.data[command]=std::vector<unsigned char>(lock::command_size,0x6b);
    memory.put(singleton,table);memory.put(command,command_table);
    memory.put(singleton+lock::normal_dlss_offset,std::uint32_t{1});
    memory.put(singleton+lock::alternate_dlss_offset,std::uint32_t{0});
    memory.put(command+lock::command_dlss_offset,std::uint32_t{1});
    route::Context context;context.policy.enabled=true;context.policy.admission.reason=lock::Reason::applied;
    context.policy.admission.singleton=singleton;context.policy.admission.vtable=table;context.policy.admission.command_vtable=command_table;
    context.policy.normal_write_authorized=context.policy.command_write_authorized=context.policy.command_layout_verified=true;
    // Deliberately unrelated old single-thread values: route proof takes precedence.
    context.policy.owner_thread=99;context.policy.consumer_thread=100;
    auto read=[&](auto at,auto* out,auto size){return memory.read(at,out,size);};
    auto cas=[&](auto at,auto old,auto value){return memory.cas(at,old,value);};
    const route::Call collect{11,route::collector_return_rvas[0]},direct{11,route::direct_commit_return_rva};
    auto before=memory.data;
    auto menu=route::menu_before(context,singleton,3,{22,route::menu_return_rva},read);
    CHECK(menu.protected_now()&&menu.result.bypass_original&&memory.data==before);
    CHECK(route::menu_before(context,singleton,3,{22,0x123},read).status==route::Status::unknown_caller);
    CHECK(route::menu_before(context,singleton,3,{0,route::menu_return_rva},read).status==route::Status::thread_missing);
    CHECK(route::menu_before(context,singleton,2,{0,0},read).status==route::Status::other_mode);
    auto admitted=route::collector_before(context,singleton,collect,read,cas);
    CHECK(admitted.protected_now()&&admitted.result.modified&&memory.get<std::uint32_t>(singleton+lock::normal_dlss_offset)==0);
    CHECK(memory.get<std::uint32_t>(singleton+lock::alternate_dlss_offset)==0);
    CHECK(route::commit_before(context,command,direct,read,cas).status==route::Status::no_collector_scope);
    {
        route::CollectorScope outer(admitted);
        CHECK(route::commit_before(context,command,direct,read,cas).protected_now());
        CHECK(memory.get<std::uint32_t>(command+lock::command_dlss_offset)==0);
        CHECK(route::commit_before(context,command,{12,direct.return_rva},read,cas).status==route::Status::scope_thread_mismatch);
        auto nested=route::collector_before(context,singleton,{11,route::collector_return_rvas[1]},read,cas);
        {route::CollectorScope inner(nested);CHECK(route::commit_before(context,command,direct,read,cas).protected_now());}
        CHECK(route::commit_before(context,command,direct,read,cas).protected_now());
        auto bad=route::collector_before(context,singleton,{11,0x123},read,cas);
        {route::CollectorScope child(bad);CHECK(route::commit_before(context,command,direct,read,cas).status==route::Status::no_collector_scope);}
        CHECK(route::commit_before(context,command,direct,read,cas).protected_now());
        bool thread_ok=false;
        auto queued_context=context;
        queued_context.queued[0]={true,33,0x654321}; // Synthetic explicitly verified CPU fixture, not a game caller claim.
        std::thread worker([&] {
            const auto denied_direct=route::commit_before(queued_context,command,{33,direct.return_rva},read,cas);
            const auto unknown=route::commit_before(queued_context,command,{33,0x777777},read,cas);
            const auto accepted=route::commit_before(queued_context,command,{33,0x654321},read,cas);
            thread_ok=denied_direct.status==route::Status::no_collector_scope&&unknown.status==route::Status::queued_unverified&&accepted.protected_now();
        });worker.join();CHECK(thread_ok);
    }
    CHECK(route::commit_before(context,command,direct,read,cas).status==route::Status::no_collector_scope);
    context.queued[0]={false,33,0x654321};
    CHECK(route::commit_before(context,command,{33,0x654321},read,cas).status==route::Status::queued_unverified);
    context.queued[0].verified=true;
    CHECK(route::commit_before(context,command,{34,0x654321},read,cas).status==route::Status::queued_unverified);
    CHECK(route::commit_before(context,command,{33,0x654321},read,cas).protected_now());
    context.queued[1]={true,11,direct.return_rva};
    CHECK(route::commit_before(context,command,direct,read,cas).status==route::Status::no_collector_scope);
    // Out-of-order teardown poisons admission until every live scope unwinds.
    {
        auto outer=std::make_unique<route::CollectorScope>(admitted);
        auto inner=std::make_unique<route::CollectorScope>(admitted);outer.reset();
        CHECK(route::commit_before(context,command,direct,read,cas).status==route::Status::scope_poisoned);
        inner.reset();CHECK(route::commit_before(context,command,direct,read,cas).status==route::Status::no_collector_scope);
    }
    {
        std::vector<std::unique_ptr<route::CollectorScope>> nested;
        for(unsigned i=0;i<9;++i)nested.push_back(std::make_unique<route::CollectorScope>(admitted));
        CHECK(route::commit_before(context,command,direct,read,cas).status==route::Status::scope_poisoned);
        while(!nested.empty())nested.pop_back();
    }
    {route::CollectorScope recovered(admitted);CHECK(route::commit_before(context,command,direct,read,cas).protected_now());}
    // All writes remain the existing exact-field policy; forwarding preserves return values.
    memory.put(singleton+lock::normal_dlss_offset,std::uint32_t{1});memory.reject_write=true;
    route::Decision decision{};unsigned calls=0;
    auto original_collector=[&](auto) {
        ++calls;CHECK(route::commit_before(context,command,direct,read,cas).status==route::Status::no_collector_scope);return 73;
    };
    CHECK(route::invoke_collector(context,singleton,collect,read,cas,original_collector,decision)==73);
    CHECK(decision.result.fault()&&calls==1);
    memory.reject_write=false;
    auto original_commit=[&](auto){++calls;return std::uint32_t{192};};
    CHECK(route::invoke_commit(context,command,{33,0x777777},read,cas,original_commit,decision)==192);
    CHECK(decision.status==route::Status::queued_unverified);
    auto disabled=context;disabled.policy.enabled=false;before=memory.data;
    CHECK(route::invoke_commit(disabled,command,{0,0},read,cas,original_commit,decision)==192);
    CHECK(decision.status==route::Status::disabled&&memory.data==before);
    auto original_menu=[&](auto,auto){++calls;return 19;};const auto previous_calls=calls;
    CHECK(route::invoke_menu(context,singleton,3,{22,route::menu_return_rva},read,original_menu,decision)==1);
    CHECK(calls==previous_calls);
    std::puts("graphics lock route tests passed: menu/direct/nested/queued/cross-thread/order/unknown/failure/disabled");return 0;
}
