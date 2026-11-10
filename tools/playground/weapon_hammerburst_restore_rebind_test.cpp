#include "weapon_hammerburst_restore_fixture.inl"
int main(){
    using namespace hb_rebind_fixture;checks=cases=0;
    for(bool inserted:{false,true}){
        auto f=setup(inserted);const auto original_loaded=opening.loaded;const auto saved=transaction.inventory();
        C(restore_control::rebind(f));C(stage==Stage::restoring);C(opening.identity==f.identity);
        C(opening.loaded==original_loaded);C(transaction.inventory().loaded==saved.loaded&&transaction.inventory().reserve==saved.reserve);
        C(parked_plan.transaction.inventory().identity==f.identity&&observer.value.identity==f.identity);
        C(parked_native_lease.identity==f.identity&&parked_native_lease.serial==1&&!parked_native_lease.acquired);
        C(!held&&!released&&!restore_display_ready&&stroke.resets==1&&insertion.resets==1&&extraction.resets==1);
        C(geometry.latch.open_angle==1.2&&discovers==1&&observer.rebinds==1);
        C(!lancerreload_state::lease.acquired&&!anim::ownership.active&&!visual::active);
        C(!restore_control::rebind(f));++cases;
    }
    for(unsigned fault=0;fault<20;++fault){
        auto f=setup();const auto original=opening.identity;const auto loaded=transaction.inventory().loaded;
        switch(fault){
        case 0:park_control::enabled_value=false;break;
        case 1:fresh_value=false;break;
        case 2:eligible_value=false;break;
        case 3:++f.identity.weapon;break;
        case 4:++f.identity.session;break;
        case 5:lancerreload_state::lease.acquired=true;break;
        case 6:anim::ownership.active=true;break;
        case 7:visual::active=true;break;
        case 8:++nativelancerfollow::bound_key.component;break;
        case 9:++nativelancerfollow::bound_key.asset;break;
        case 10:++nativelancerfollow::bound_key.weapon;break;
        case 11:observer.available=false;break;
        case 12:++observer.value.loaded;break;
        case 13:discover_value=false;break;
        case 14:expire_in_discover=true;break;
        case 15:final_geometry=false;break;
        case 16:nativeequipment::current_value=false;break;
        case 17:replace_in_discover=true;break;
        case 18:++parked_native_lease.serial;break;
        case 19:++parked_native_lease.identity.hold;break;
        }
        C(!restore_control::rebind(f));C(stage==Stage::parked&&opening.identity==original);
        C(transaction.inventory().identity==original&&transaction.inventory().loaded==loaded);
        C(observer.rebinds==0&&parked_plan.transaction.inventory().identity==original);
        C(geometry.latch.open_angle==1.2&&held==99&&released&&restore_display_ready);
        C(!stroke.resets&&!insertion.resets&&!extraction.resets);++cases;
    }
    std::printf("HammerburstRestoreRebindTest: %u cases, %u checks; atomic copies, identity/count guards, no engine action\n",cases,checks);return 0;
}
