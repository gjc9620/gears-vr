#include "free_hand_control.h"
#include "weapon_pose_solver.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace gears_weapon;
using gears_vr::Viewpoint;

int main(){
    std::uint64_t now=1000;
    FreeHandGameState game{11,now,0x1000,0x2000,0x4000,0x3000,true,true,true,0};
    HandWorldFrame pose{};
    pose.session=7;pose.sequence=1;pose.space_epoch=2;pose.input_epoch=3;
    pose.world_generation=4;pose.controller=game.controller;pose.pawn=game.pawn;
    pose.weapon=game.weapon;pose.created_ms=now;pose.units_per_metre=100;
    pose.valid_mask=3;
    pose.left={{1,2,3},{.1,.2,.3}};pose.right={{4,5,6},{.4,.5,.6}};
    auto next=[&]{++now;++pose.sequence;pose.created_ms=game.observed_ms=now;};
    FreeHandControl control;
    auto invalid_game=game;invalid_game.alive=false;
    FreeHandControl initially_invalid;
    assert(initially_invalid.advance(invalid_game,nullptr,now,true).generation!=0);
    auto frame=control.advance(game,&pose,now,false);
    assert(frame.phase==FreeHandPhase::disabled&&!frame.tracked_mask&&!frame.apply_mask);
    frame=control.advance(game,&pose,now,true);
    assert(frame.phase==FreeHandPhase::controller_targets&&frame.eligible_mask==3);
    assert(frame.world[0].position.x==1&&frame.world[1].rotation.roll==.6&&!frame.apply_mask);
    const auto original_right=frame.world[1];
    next();pose.left={{21,-12,7},{-.4,1.7,-.8}};
    frame=control.advance(game,&pose,now,true);
    assert(frame.world[0].position.x==21&&frame.world[0].rotation.roll==-.8);
    assert(frame.world[1].position.x==original_right.position.x&&
           frame.world[1].rotation.roll==original_right.rotation.roll);
    next();pose.valid_mask=1;
    frame=control.advance(game,&pose,now,true);
    assert(frame.eligible_mask==1&&frame.world[1].position.x==0);
    next();pose.valid_mask=2;
    frame=control.advance(game,&pose,now,true);
    assert(frame.eligible_mask==2&&frame.world[0].position.x==0);

    // Every fixed action owns both hands, even while input is missing. A
    // fresh ticket is required after end/cancel, and the resumed target is
    // current controller pose rather than a stored animation/display pose.
    for(const auto reason:{hand_pickup,hand_melee,hand_weapon_switch,hand_other_fixed_action}){
        next();game.fixed_animation_mask=reason;
        frame=control.advance(game,&pose,now,true);
        const auto animation_generation=frame.generation;
        assert(frame.phase==FreeHandPhase::original_animation&&!frame.eligible_mask);
        frame=control.advance(game,nullptr,now,true);
        assert(frame.phase==FreeHandPhase::original_animation&&!frame.apply_mask);
        game.fixed_animation_mask=0;
        frame=control.advance(game,&pose,now,true);
        assert(frame.phase==FreeHandPhase::waiting_new_pose&&!frame.eligible_mask&&
               frame.generation!=animation_generation);
        next();pose.right.position.x+=10;
        frame=control.advance(game,&pose,now,true);
        assert(frame.phase==FreeHandPhase::controller_targets&&frame.eligible_mask==2&&
               frame.world[1].position.x==pose.right.position.x&&!frame.apply_mask);
    }

    next();game.fixed_animation_mask=hand_melee|hand_weapon_switch;
    assert(control.advance(game,&pose,now,true).phase==FreeHandPhase::original_animation);
    next();game.fixed_animation_mask=hand_weapon_switch;
    assert(control.advance(game,&pose,now,true).phase==FreeHandPhase::original_animation);
    next();game.fixed_animation_mask=0;game.ownership_proved=false;
    frame=control.advance(game,&pose,now,true);
    assert(frame.phase==FreeHandPhase::unproved_animation_state&&frame.tracked_mask==2&&
           !frame.eligible_mask&&!frame.apply_mask);
    game.ownership_proved=true;
    assert(control.advance(game,&pose,now,true).phase==FreeHandPhase::waiting_new_pose);
    next();assert(control.advance(game,&pose,now,true).eligible_mask==2);

    // Freshness, source/identity boundaries, no stale hand when tracking drops.
    now+=101;game.observed_ms=now;
    assert(control.advance(game,&pose,now,true).phase==FreeHandPhase::missing_tracking);
    next();pose.created_ms=now+1;
    assert(!control.advance(game,&pose,now,true).tracked_mask);
    pose.created_ms=now;pose.pawn++;
    assert(!control.advance(game,&pose,now,true).tracked_mask);pose.pawn--;
    pose.grip_down_mask=1;
    assert(!control.advance(game,&pose,now,true).tracked_mask);pose.grip_down_mask=0;
    pose.right.rotation.yaw=std::numeric_limits<double>::quiet_NaN();
    assert(!control.advance(game,&pose,now,true).tracked_mask);pose.right.rotation.yaw=.5;
    frame=control.advance(game,&pose,now,true);
    const auto before_epoch=frame.generation;
    ++pose.space_epoch;pose.sequence=1;
    frame=control.advance(game,&pose,now,true);
    assert(frame.eligible_mask==2&&frame.generation!=before_epoch);
    for(unsigned boundary=0;boundary<3;++boundary){
        const auto previous_generation=frame.generation;
        if(boundary==0)++pose.session;
        if(boundary==1)++pose.input_epoch;
        if(boundary==2)++pose.world_generation;
        frame=control.advance(game,&pose,now,true);
        assert(frame.eligible_mask==2&&frame.generation!=previous_generation&&
               frame.world_generation==pose.world_generation);
    }
    next();control.advance(game,&pose,now,true);--pose.sequence;
    assert(!control.advance(game,&pose,now,true).tracked_mask);
    next();game.mesh++;
    assert(control.advance(game,&pose,now,true).phase==FreeHandPhase::waiting_new_pose);
    next();assert(control.advance(game,&pose,now,true).eligible_mask==2);
    game.alive=false;
    assert(control.advance(game,&pose,now,true).phase==FreeHandPhase::invalid_player);
    game.alive=true;
    assert(control.advance(game,&pose,now,true).phase==FreeHandPhase::waiting_new_pose);
    next();assert(control.advance(game,&pose,now,true).eligible_mask==2);
    control.advance(game,&pose,now,false);
    assert(control.advance(game,&pose,now,true).eligible_mask==2);

    // Actual XR grip-to-world conversion: both hands share the frozen
    // reference/camera basis. Moving one never retargets the other. Current
    // HMD pose is absent, so head-only motion cannot change these targets.
    const gears_vr::StereoMatrix view{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    gears_vr::Pose reference{},left{{-.2,1.1,-.4},{}},right{{.3,1.2,-.5},{}};
    Viewpoint left_world{},right_world{},moved{};
    assert(xr_pose_in_game_world(left,reference,view,100,left_world));
    assert(xr_pose_in_game_world(right,reference,view,100,right_world));
    left.position.x-=.25;left.orientation={0,0,.3,std::sqrt(.91)};
    assert(xr_pose_in_game_world(left,reference,view,100,moved));
    assert(std::abs(moved.position.x-left_world.position.x)>20);
    next();pose.valid_mask=3;pose.left=moved;pose.right=right_world;
    frame=control.advance(game,&pose,now,true);
    assert(frame.eligible_mask==3&&frame.world[0].position.x==moved.position.x&&
           frame.world[1].position.x==right_world.position.x&&!frame.apply_mask);

    // Exercise the actual named GVE6 transport used by the game-thread probe,
    // including attachment's no-replay baseline. These are synthetic poses;
    // this proves transport and policy only, not headset or skeletal output.
    EquipmentIntentBridge publisher,reader;
    assert(publisher.open(true)&&reader.open(false));
    std::uint64_t cursor=UINT64_MAX;
    HandWorldFrame received{};
    assert(!reader.read_pose(cursor,received));
    assert(publisher.publish_pose(pose)&&reader.read_pose(cursor,received));
    game.observed_ms=GetTickCount64();
    FreeHandControl transported;
    frame=transported.advance(game,&received,game.observed_ms,true);
    assert(frame.tracked_mask==3&&frame.eligible_mask==3&&frame.sequence==pose.sequence);
    assert(frame.world[0].rotation.yaw==moved.rotation.yaw&&
           frame.world[1].position.z==right_world.position.z&&!frame.apply_mask);
    assert(!reader.read_pose(cursor,received));
    std::puts("Free hand XR targets and animation ownership passed (read-only; apply_mask=0)");
}
