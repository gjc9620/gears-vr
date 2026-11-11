// Instrument the real frozen readers' existing synthetic safe_copy hook.
// These are read counts, not game timings or a replacement reader model.
#define GEARS_COLLIDER_ROLES_TEST_MAIN PriorCostColliderMain
#include "cover_collider_roles_live_test.cpp"
#undef GEARS_COLLIDER_ROLES_TEST_MAIN
#include <map>
#include <string>
static bool inside_owner{};
static std::map<std::string,unsigned long long> counts;
static void count_read(uintptr_t address,size_t size){
    std::string category;
    if(inside_owner)category="owner_guard";
    else if(address==h::metadata.objects_header||address==h::metadata.names_header)category="registry_headers";
    else if(size==80)category="object_identity_blocks";
    else if(size==146)category="name_identity_blocks";
    else {
        const auto objects=get<gears_anchor::Registry>(h::metadata.objects_header);
        const auto names=get<gears_anchor::Registry>(h::metadata.names_header);
        if(size==8&&address>=objects.data&&address<objects.data+uintptr_t(objects.count)*8)category="object_slots";
        else if(size==8&&address>=names.data&&address<names.data+uintptr_t(names.count)*8)category="name_slots";
        else category="other_"+std::to_string(size)+"_bytes";
    }
    ++counts[category];
}
static bool counted_guard(){
    inside_owner=true;const bool result=guard();inside_owner=false;return result;
}
static void print_counts(char const* name,unsigned before){
    unsigned long long total{};
    printf("{\"reader\":\"%s\",\"five_reads\":%u,\"categories\":{",name,reads-before);
    bool comma=false;
    for(auto const& row:counts){printf("%s\"%s\":%llu",comma?",":"",row.first.c_str(),row.second);comma=true;total+=row.second;}
    printf("}}\n");C(total==reads-before);
}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    collider_setup();roles::Snapshot physics{};collider::Snapshot collision{};
    C(roles::sample(actor,physics_expected,guard,workspace,physics)==roles::Code::current);
    C(collider::sample(actor,guard,collider_work,collision)==roles::Code::current);
    auto before=reads;counts.clear();read_hook=count_read;
    for(unsigned n=0;n<5;++n)C(roles::recheck(actor,physics_expected,counted_guard,workspace,physics)==roles::Code::current);
    read_hook=nullptr;print_counts("physics",before);
    before=reads;counts.clear();read_hook=count_read;
    for(unsigned n=0;n<5;++n)C(collider::recheck(actor,counted_guard,collider_work,collision)==roles::Code::current);
    read_hook=nullptr;print_counts("collider",before);
    C(native_calls==0&&guard_lock_free);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("checks=%u failures=%u no game/query/write\n",checks,failures);return failures?1:0;
}
