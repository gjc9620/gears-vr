#define GEARS_COMPONENT_ROLES_TEST_MAIN PriorProofInventoryComponentMain
#include "cover_component_roles_live_test.cpp"
#undef GEARS_COMPONENT_ROLES_TEST_MAIN
#include <map>
#include <memory>
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    catalog_setup();auto output=std::make_unique<catalog::Snapshot>();
    if(catalog::sample(actor,guard,catalog_work,*output)!=roles::Code::current)return 2;
    std::map<uintptr_t,unsigned> identities;std::map<std::pair<uintptr_t,unsigned>,unsigned> fields;
    unsigned identity_visits{},field_visits{},proofs{};
    for(unsigned i=0;i<catalog_work.members.count;++i){const auto& row=catalog_work.members.members[i];++identity_visits;++identities[row.identity.address];++field_visits;++fields[{row.identity.address+0x68,8}];}
    auto inspect=[&](h::Proof const& p){++proofs;
        for(unsigned i=0;i<p.identity_count;++i){++identity_visits;++identities[p.identities[i].address];}
        for(unsigned i=0;i<p.source_count;++i){++field_visits;++fields[{p.sources[i].address,p.sources[i].size}];}
    };
    for(unsigned i=0;i<catalog_work.first.root_count;++i)inspect(catalog_work.roots[i].proof);
    for(unsigned i=0;i<catalog_work.layout_count;++i)inspect(catalog_work.layouts[i].proof);
    for(unsigned i=0;i<catalog_work.asset_layout_count;++i)inspect(catalog_work.assets[i].proof);
    for(unsigned i=0;i<catalog_work.bodies.layout_count;++i)inspect(catalog_work.bodies.layouts[i].proof);
    printf("ComponentProofInventory proofs=%u members=%u identity_visits=%u unique_identities=%zu field_visits=%u unique_fields=%zu synthetic typed schema only; no game\n",
        proofs,catalog_work.members.count,identity_visits,identities.size(),field_visits,fields.size());
    const auto initial=catalog_work.proof_pass.last;
    if(initial.identity_visits!=identity_visits||initial.identity_reads!=identities.size()||initial.field_visits!=field_visits||initial.field_reads!=fields.size()||initial.fallbacks)return 4;
    const auto before=catalog_work.proof_pass.total.passes;
    if(catalog::recheck(actor,guard,catalog_work,*output)!=roles::Code::current||catalog_work.proof_pass.total.passes!=before+3)return 5;
    unsigned mutated_fields{},mutated_identities{};
    auto current=[&](){return catalog::current(catalog_work,catalog_work.first.root_count);};
    // A preceding successful pass must never hide a change in ANY retained
    // exact field or identity in the next pass. Restore and prove cold again.
    for(auto const& item:fields){const auto address=item.first.first;
        if(address<arena||address-arena>=saved_arena.size())return 6;
        auto* byte=reinterpret_cast<unsigned char*>(address);const auto saved=*byte;*byte^=1;
        if(current())return 7;*byte=saved;if(!current())return 8;++mutated_fields;
    }
    for(auto const& item:identities){const auto address=item.first+h::metadata.object_name+4;
        if(address<arena||address-arena>=saved_arena.size())return 9;
        auto* byte=reinterpret_cast<unsigned char*>(address);const auto saved=*byte;*byte^=1;
        if(current())return 10;*byte=saved;if(!current())return 11;++mutated_identities;
    }
    if(catalog_work.proof_pass.active.load())return 12;
    printf("ComponentProofPass fresh_barriers=3 mutation_fields=%u mutation_identities=%u identity_reads=%llu/%u field_reads=%llu/%u no cross-pass reuse\n",
        mutated_fields,mutated_identities,initial.identity_reads,identity_visits,initial.field_reads,field_visits);
    if(!VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE))return 3;return 0;
}
