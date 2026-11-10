#define GEARS_MEMBER_WITNESS_TEST_MAIN PriorMemberWitnessMain
#include "cover_member_witness_live_test.cpp"
#undef GEARS_MEMBER_WITNESS_TEST_MAIN
int main(int argc,char** argv){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    physics_setup();add_mesh_siblings(192);roles::Snapshot output{};
    reads=0;C(observe(output));const auto whole_reads=reads;
    static cover_member_witness_live::Store store;store.clear();h::Proof proof{};h::Property first{},second{};
    C(h::property(proof,fixture_mesh_class,L"PhysicsAsset",L"ObjectProperty",8,L"PhysicsAsset",first));
    C(h::property(proof,fixture_mesh_class,L"PhysicsAssetInstance",L"ObjectProperty",8,L"PhysicsAssetInstance",second));
    reads=0;C(roles::membership(proof,first,&store));const auto first_reads=reads;
    reads=0;C(roles::membership(proof,second,&store));const auto second_reads=reads;
    C(store.current()&&h::current(proof));
    if(argc==2&&!strcmp(argv[1],"--cached"))C(second_reads*8<first_reads);
    const auto saved=output;
    for(unsigned change=0;change<4;++change){
        physics_setup();const auto tail=add_mesh_siblings(192);output=saved;
        if(change==0)plan(body_instances[1]+0x88,1,tail+72,get<unsigned>(props[1]+72),4);
        if(change==1)plan(body_instances[1]+0x88,1,tail+64,fixture_pawn_class,8);
        if(change==2)plan(body_instances[1]+0x88,1,fixture_mesh_class+0x80,props[1],8);
        if(change==3)plan(body_instances[1]+0x88,1,tail+0x68,props[0],8);
        C(roles::sample(actor,physics_expected,guard,workspace,output)!=roles::Code::current&&roles::same(output,saved));read_hook=nullptr;
    }
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverMemberChain checks=%u failed=%u whole_reads=%u first_membership_reads=%u second_membership_reads=%u actual typed reader, no game\n",checks,failures,whole_reads,first_reads,second_reads);
    return failures?1:0;
}
