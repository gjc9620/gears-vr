#include "weapon_lancer_chainsaw_pose.h"
#include "weapon_lancer_starter_mount.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main(){
    using namespace gears_weapon;
    std::array<float,16> gun{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    LancerStarterWorld mount{};
    assert(lancer_starter_world(gun,mount));
    assert(mount.anchor_raw.x==6&&mount.anchor_raw.y==26&&
           mount.anchor_raw.z==10.65&&mount.pull_axis.x==1);
    gun={0,1,0,0, -1,0,0,0, 0,0,1,0, 0,0,0,1};
    assert(lancer_starter_world(gun,mount));
    assert(mount.anchor_raw.x==-26&&mount.anchor_raw.y==6&&
           mount.pull_axis.x==0&&mount.pull_axis.y==1);
    gun[1]=-1;
    assert(!lancer_starter_world(gun,mount)); // reflected gun frame
    HandWorldFrame frame{};
    frame.created_ms=1000;frame.session=1;frame.sequence=1;
    frame.space_epoch=2;frame.input_epoch=3;frame.world_generation=4;
    frame.controller=10;frame.pawn=20;frame.weapon=30;
    frame.units_per_metre=100;frame.valid_mask=3;
    frame.left.position={100,200,300};frame.right.position={120,200,300};
    LancerSawGamePose game{};
    game.world=40;game.world_generation=4;
    game.controller=10;game.pawn=20;game.weapon=30;
    game.starter_anchor_raw={100,200,300};game.pull_axis_world={1,0,0};
    game.lancer_active=game.right_owns_weapon=game.game_can_melee=true;
    LancerSawSample sample{};
    assert(lancer_saw_sample_from_world(frame,game,1000,sample));
    assert(sample.left_palm_m.x==1&&sample.starter_anchor_m.z==3&&
           sample.right_tracked&&sample.left_tracked&&sample.world==40);
    frame.units_per_metre=200;frame.left.position={200,400,600};
    game.starter_anchor_raw={200,400,600};
    assert(lancer_saw_sample_from_world(frame,game,1000,sample));
    assert(sample.left_palm_m.x==1&&sample.starter_anchor_m.z==3);
    game.starter_anchor_raw={100,200,300};
    frame.units_per_metre=100;frame.left.position={100,200,300};
    LancerChainsawInteraction interaction;
    assert(lancer_saw_sample_from_world(frame,game,1000,sample));
    assert(interaction.observe(sample).event==LancerSawEvent::none);
    frame.sequence++;frame.created_ms+=80;frame.grip_down_mask=1;
    assert(lancer_saw_sample_from_world(frame,game,1080,sample));
    assert(interaction.observe(sample).event==LancerSawEvent::pull_began);
    for(double x:{105.,110.,115.,119.}){
        frame.sequence++;frame.created_ms+=80;frame.left.position.x=x;
        assert(lancer_saw_sample_from_world(frame,game,frame.created_ms,sample));
        const auto result=interaction.observe(sample);
        if(x==119.)assert(result.event==LancerSawEvent::request_original_hold&&
                           result.press_original_melee);
        else assert(result.event==LancerSawEvent::none);
    }
    frame.units_per_metre=200;frame.left.position={200,400,600};
    game.starter_anchor_raw={200,400,600};
    frame.grip_down_mask=0;frame.sequence=100;frame.created_ms=2000;
    LancerChainsawInteraction scaled_interaction;
    assert(lancer_saw_sample_from_world(frame,game,2000,sample));
    assert(scaled_interaction.observe(sample).event==LancerSawEvent::none);
    frame.sequence++;frame.created_ms+=80;frame.grip_down_mask=1;
    assert(lancer_saw_sample_from_world(frame,game,2080,sample));
    assert(scaled_interaction.observe(sample).event==LancerSawEvent::pull_began);
    for(double x:{210.,220.,230.,238.}){
        frame.sequence++;frame.created_ms+=80;frame.left.position.x=x;
        assert(lancer_saw_sample_from_world(frame,game,frame.created_ms,sample));
        const auto result=scaled_interaction.observe(sample);
        if(x==238.)assert(result.event==LancerSawEvent::request_original_hold);
        else assert(result.event==LancerSawEvent::none);
    }
    const auto retained=sample;
    game.weapon++;
    assert(!lancer_saw_sample_from_world(frame,game,frame.created_ms,sample));
    assert(sample.weapon==retained.weapon);
    game.weapon--;
    frame.units_per_metre=0;
    assert(!lancer_saw_sample_from_world(frame,game,frame.created_ms,sample));
    frame.units_per_metre=100;
    assert(!lancer_saw_sample_from_world(frame,game,frame.created_ms+101,sample));
    frame.valid_mask=1;frame.grip_down_mask=1;
    assert(lancer_saw_sample_from_world(frame,game,frame.created_ms,sample));
    assert(!sample.right_tracked&&sample.left_tracked);
    std::puts("Lancer chainsaw same-ticket world-scale gesture adapter passed");
}
