#include "weapon_hammerburst_restore_display_fixture.inl"
int main(){
    using namespace hb_rebind_fixture;using D=restore_control::DisplayResult;checks=cases=0;
    for(unsigned mode=0;mode<3;++mode){
        auto f=display_setup(mode);const auto before=transaction.inventory();const auto original_loaded=opening.loaded;
        C(restore_control::display(f)==D::advanced);C(starts==1&&!seeks&&!permit_active);
        C(restore_control::display(f)==D::advanced);C(starts==1&&seeks==1&&!permit_active);
        C(restore_control::display(f)==D::ready);C(restore_display_ready&&restore_display_step==3);
        C(visual_starts==(mode==1?1u:0u)&&stows==visual_starts);
        C(inv::counts_equal(before,transaction.inventory())&&opening.loaded==original_loaded);
        C(!held&&!released&&!lancerreload_state::lease.acquired&&!permit_active);
        C(restore_control::display(f)==D::ready&&starts==1&&seeks==1&&stows==visual_starts);++cases;
    }
    for(unsigned fault=0;fault<7;++fault){
        auto f=display_setup();
        if(fault==0)fresh_value=false;
        if(fault==1)permit_enabled=false;
        if(fault==2)lancerreload_state::lease.acquired=true;
        if(fault==3){anim::ownership.active=true;anim::ownership.inventory=opening.identity;}
        if(fault==4){visual::active=true;visual::saved.weapon=8;}
        if(fault==5)final_geometry=false;
        if(fault==6)observer.available=false;
        C(restore_control::display(f)==D::waiting);C(!starts&&!seeks&&!stows&&!permit_active&&!restore_display_ready);++cases;
    }
    {
        auto f=display_setup();++observer.value.loaded;C(restore_control::display(f)==D::recovery_required);
        C(!starts&&!seeks&&!stows&&!restore_display_ready);++cases;
    }
    for(auto result:{anim::Acquire::rejected,anim::Acquire::recovery_required}){
        auto f=display_setup();start_result=result;
        C(restore_control::display(f)==(result==anim::Acquire::rejected?D::waiting:D::recovery_required));
        C(starts==1&&!seeks&&!permit_active&&restore_display_step==0&&!restore_display_ready);++cases;
    }
    for(bool pending:{false,true}){
        auto f=display_setup();C(restore_control::display(f)==D::advanced);seek_result=false;seek_pending_result=pending;
        C(restore_control::display(f)==(pending?D::recovery_required:D::waiting));
        C(starts==1&&seeks==1&&restore_display_step==1&&!permit_active&&!restore_display_ready);++cases;
    }
    for(unsigned fault=0;fault<3;++fault){
        auto f=display_setup(1);C(restore_control::display(f)==D::advanced);C(restore_control::display(f)==D::advanced);
        if(fault<2){visual_result=false;visual::begin_retryable=fault==0;}else stow_result=false;
        C(restore_control::display(f)==(fault==0?D::waiting:D::recovery_required));
        C(starts==1&&seeks==1&&visual_starts==1&&!permit_active&&!restore_display_ready);++cases;
    }
    std::printf("HammerburstRestoreDisplayTest: %u cases, %u checks; saved-angle/absent-magazine dispatch, no native begin or ammo mutation\n",cases,checks);return 0;
}
