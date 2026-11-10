// Real equipment Runtime/Model. No source, pose, input or clock is renewed
// while the logical already-held intent awaits a new authenticated Frame.
#include "weapon_hammerburst_equipment_fixture.inl"

void established(Host& h){h.baseline();h.press();h.event(0);C(h.tick().status==er::Status::held);C(h.model.held()&&h.published_hold);}
int main(){
    checks=cases=0;
    for(unsigned fault=0;fault<3;++fault){
        Host h;established(h);const auto generation=h.model.hold_generation();
        auto backend=h.backend();h.runtime.suspend(backend);C(h.model.held()&&!h.published_hold);
        h.advance();if(fault==0)h.hold_ok=false;if(fault==1)h.invalidate_at=Callback::hold;
        if(fault==2){h.age_at=Callback::hold;h.delay_ms=101;}
        h.tick();C(h.model.held());C(h.model.hold_generation()==generation);
        C(!h.published_hold&&!h.runtime.state().hold_published&&h.runtime.state().held_known);
        C(!h.runtime.state().pending&&h.switch_calls==0);
        h.hold_ok=h.current_ok=true;h.advance();h.tick();
        C(h.model.held()&&h.published_hold&&h.runtime.state().hold_published);
        C(h.model.hold_generation()==generation&&h.switch_calls==0);++cases;
    }
    {
        Host h;h.baseline();h.hold_ok=false;h.press();h.event(0);C(h.tick().status==er::Status::hold_failed);
        C(!h.model.held()&&!h.published_hold);const auto calls=h.hold_calls;
        h.hold_ok=true;h.advance();h.tick();C(!h.model.held()&&!h.published_hold&&h.hold_calls==calls);++cases;
    }
    for(unsigned mismatch=0;mismatch<3;++mismatch){
        Host h;established(h);auto backend=h.backend();h.runtime.suspend(backend);h.advance();
        const auto calls=h.hold_calls;
        if(mismatch==0)++h.frame.key.world_generation;
        if(mismatch==1)h.attached_ok=false;
        if(mismatch==2){h.game.current_weapon=h.game.weapon_in_slot[1];h.frame.weapon=h.objects[1];}
        h.publish();h.tick();C(!h.model.held()&&!h.published_hold&&h.hold_calls==calls);++cases;
    }
    {
        Host h;established(h);auto backend=h.backend();h.runtime.suspend(backend);h.advance();
        backend.hold=[](void* context,wh::Frame const& frame,gw::EquipmentSnapshot const& game) noexcept {
            const auto accepted=Host::hold(context,frame,game);Host::host(context).ms-=100;return accepted;
        };
        C(h.runtime.tick(h.frame,backend,h.model).status==er::Status::clock_invalid);
        C(!h.model.held()&&!h.published_hold);++cases;
    }
    std::printf("HammerburstHeldRepublishTest: %u cases, %u checks; retained private intent, no stale publication or initial-press replay\n",cases,checks);return 0;
}
