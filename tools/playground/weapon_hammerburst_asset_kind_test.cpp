// Reuse the actual common asset-reader fixture and its unchanged regressions.
// This file owns only tests of the optional validated-kind output.
#define main existing_asset_reader_regressions
#include "weapon_lancer_asset_read_test.cpp"
#undef main
int main(){
    if(existing_asset_reader_regressions())return 1;
    unsigned checks{};
    {auto m=std::make_unique<AssetMemory>();la::Evidence out{};auto kind=la::HeldAsset::hammerburst;
        assert(la::read(m->frame,out,la::AssetScope::supported_instant_fire,&kind)==la::Status::ready);
        assert(kind==la::HeldAsset::lancer);checks+=2;}
    for(unsigned failure=0;failure<4;++failure){
        auto m=std::make_unique<AssetMemory>();const auto hb=m->cls(L"Weap_LocustAssaultRifle",m->warfare,m->weapon_class);
        Memory::put(m->weapon,0x50,hb);Memory::put(m->asset,0x48,m->text(L"Locust_Hammerburst"));
        m->frame.weapon=m->identity(m->weapon);m->tables();
        la::Evidence out{};auto kind=la::HeldAsset::lancer;
        assert(la::read(m->frame,out,la::AssetScope::supported_instant_fire,&kind)==la::Status::ready);
        assert(kind==la::HeldAsset::hammerburst);checks+=2;
        if(failure==0)Memory::put(m->component,0x110,2u);
        if(failure==1){Memory::put(m->asset,0x48,m->text(L"OtherRifle"));m->tables();}
        if(failure==2)m->frame.key.thread_id=GetCurrentThreadId()+1;
        const auto scope=failure==3?la::AssetScope::lancer_only:la::AssetScope::supported_instant_fire;
        const auto before=out;
        assert(la::read(m->frame,out,scope,&kind)!=la::Status::ready);
        assert(kind==la::HeldAsset::unknown);
        assert(abi::same(before.component,out.component)&&abi::same(before.asset,out.asset));checks+=3;
    }
    {auto m=std::make_unique<AssetMemory>();la::Evidence out{};auto kind=la::HeldAsset::lancer;
        m->final_mutation([&]{Memory::put(m->component,0x120,uptr(0));});
        assert(la::read(m->frame,out,la::AssetScope::supported_instant_fire,&kind)==la::Status::changed);
        assert(kind==la::HeldAsset::unknown);checks+=2;}
    std::printf("HB optional authenticated asset-kind output: %u owned-memory checks passed; no callback or input authority\n",checks);
}
