// Reuse the current owned-memory fixture and its existing asset regressions.
#define main existing_asset_regressions
#include "weapon_lancer_asset_read_test.cpp"
#undef main

int main() {
    if(existing_asset_regressions())return 1;
    for(unsigned bad=0;bad<8;++bad){
        auto memory=std::make_unique<AssetMemory>();auto& m=*memory;
        const auto base_class=m.cls(L"Weap_Boomer_base",m.warfare,m.weapon_class);
        const auto content=m.node(L"warfaregamecontentweapons",m.package_type);
        const auto boomer=m.cls(L"Weap_Boomer",content,base_class);
        Memory::put(m.weapon,0x50,boomer);
        const auto package=m.node(L"Locust_Boomshot",m.package_type);
        const auto group=m.node(L"Mesh",m.package_type,package);
        Memory::put(m.asset,0x48,m.text(L"Locust_Boomshot"));
        Memory::put(m.asset,0x40,group);
        if(bad==1)Memory::put(m.asset,0x48,m.text(L"Locust_Boomshot_Magazine"));
        if(bad==2)Memory::put(group,0x48,m.text(L"OtherGroup"));
        if(bad==3)Memory::put(package,0x48,m.text(L"OtherPackage"));
        if(bad==4)Memory::put(m.asset,0x40,package);
        if(bad==5)Memory::put(package,0x40,m.engine);
        if(bad==6)Memory::put(group,0x50,m.asset_class);
        if(bad==7)Memory::put(m.asset,0x48,m.text(L"locust_boomshot_ammo2"));
        m.frame.weapon=m.identity(m.weapon);m.tables();la::Evidence out{};
        const auto status=la::read(m.frame,out,la::AssetScope::basic_hold);
        std::fprintf(stderr,"Boomshot fixture variant=%u status=%u\n",bad,unsigned(status));
        assert(status==(bad?la::Status::unsupported_asset:la::Status::ready));
        assert(la::read(m.frame,out,la::AssetScope::supported_instant_fire)==la::Status::unsupported_weapon);
        assert(la::read(m.frame,out,la::AssetScope::supported_r2_fire)==
            (bad?la::Status::unsupported_asset:la::Status::ready));
        assert(la::read(m.frame,out,la::AssetScope::boomshot_projectile)==
            (bad?la::Status::unsupported_asset:la::Status::ready));
        assert(la::read(m.frame,out,la::AssetScope::lancer_only)==la::Status::unsupported_weapon);
        assert(la::read(m.frame,out,la::AssetScope::frag_hold)==la::Status::unsupported_weapon);
    }
    std::puts("Boomshot asset: strict model/group/package and scope boundaries passed (owned memory only)");
    return 0;
}
