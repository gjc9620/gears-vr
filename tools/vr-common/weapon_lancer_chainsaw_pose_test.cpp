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
    assert(mount.socket_raw.x==6&&mount.anchor_raw.x==10.5&&mount.anchor_raw.y==26&&
           mount.anchor_raw.z==10.65&&mount.pull_axis.x==1);
    gun={0,1,0,0, -1,0,0,0, 0,0,1,0, 0,0,0,1};
    assert(lancer_starter_world(gun,mount));
    assert(mount.anchor_raw.x==-26&&mount.anchor_raw.y==10.5&&
           mount.pull_axis.x==0&&mount.pull_axis.y==1);
    gun[1]=-1;
    assert(!lancer_starter_world(gun,mount)); // reflected gun frame
    {
        std::array<float,16> actual{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        gears_vr::Viewpoint pose{};const gears_vr::Vector scale{1,1,1};
        auto axes=gears_vr::ue_matrix({});
        assert(lancer_starter_matches_draw(actual,pose,scale,&axes));
        auto bad=axes;bad[0][0]=-1;actual[0]=-1;
        assert(!lancer_starter_matches_draw(actual,pose,scale,&bad)); // matching reflection is still invalid
        actual[0]=1.02f;bad=axes;bad[0][0]=1.02;
        assert(!lancer_starter_matches_draw(actual,pose,scale,&bad)); // scale cannot hide in axes
        actual[0]=1;bad=axes;bad[0][1]=.2;actual[4]=.2f;
        assert(!lancer_starter_matches_draw(actual,pose,scale,&bad)); // nor shear
        actual[4]=0;bad=axes;bad[0][0]=NAN;
        assert(!lancer_starter_matches_draw(actual,pose,scale,&bad));
        actual[12]=1;assert(!lancer_starter_matches_draw(actual,pose,scale,&axes)); // old position still rejected
        actual[12]=0;actual[0]=-1;actual[5]=-1;
        assert(!lancer_starter_matches_draw(actual,pose,scale,&axes)); // valid180-degree wrong direction rejected
    }
    for(double scale:{.5,1.,2.})for(double units:{80.,100.,150.})for(double pull:{0.,.01,.18,.3}){
        const std::array<float,16> posed{0,float(scale),0,0, float(-scale),0,0,0,
            0,0,float(scale),0, 123,456,789,1};
        LancerStarterParts parts{};assert(lancer_starter_parts(posed,pull,units,parts));
        const auto point=[](std::array<float,16> const& matrix,double x){return gears_vr::Vector{
            matrix[12]+x*matrix[0],matrix[13]+x*matrix[1],matrix[14]+x*matrix[2]};};
        const auto fixed=point(parts.cord,lancer_starter_cord_start_raw);
        const auto socket=point(parts.socket,lancer_starter_cord_start_raw);
        const auto moving=point(parts.cord,lancer_starter_cord_end_raw);
        const auto grip=point(parts.grip,lancer_starter_cord_end_raw);
        const auto contact=point(parts.grip,lancer_starter_grip_raw);
        const auto close_enough=[](gears_vr::Vector a,gears_vr::Vector b){
            return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)<.0002;};
        assert(close_enough(fixed,socket)&&close_enough(moving,grip)&&close_enough(contact,parts.grasp_raw));
        assert(std::abs(contact.y-(456+10.5*scale+pull*units))<.0002);
        for(auto const* draw:{&parts.socket,&parts.grip,&parts.cord}){
            gears_vr::Viewpoint pose{};gears_vr::Vector sizes{};
            assert(lancer_starter_draw_pose(*draw,pose,sizes));
            assert(lancer_starter_matches_draw(*draw,pose,sizes));
            auto stale=*draw;stale[12]+=1;assert(!lancer_starter_matches_draw(stale,pose,sizes));
            auto reversed=*draw;reversed[0]=-reversed[0];reversed[1]=-reversed[1];reversed[2]=-reversed[2];
            assert(!lancer_starter_matches_draw(reversed,pose,sizes));
            const auto basis=gears_vr::ue_matrix(pose.rotation);
            const double scales[]={sizes.x,sizes.y,sizes.z};
            for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
                assert(std::abs(basis[j][i]*scales[i]-(*draw)[i*4+j])<1e-4);
        }
    }
    {
        const auto basis=gears_vr::ue_matrix({.4,-.7,.3});
        std::array<float,16> turned{};turned[15]=1;
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)turned[i*4+j]=float(basis[j][i]);
        LancerStarterParts parts{};assert(lancer_starter_parts(turned,.18,100,parts));
        for(auto const* draw:{&parts.socket,&parts.grip,&parts.cord}){
            gears_vr::Viewpoint pose{};gears_vr::Vector scale{};assert(lancer_starter_draw_pose(*draw,pose,scale));
            assert(lancer_starter_matches_draw(*draw,pose,scale));
        }
    }
    {
        LancerStarterParts untouched{};untouched.socket[12]=321;
        auto before=untouched;
        assert(!lancer_starter_parts(gun,.18,100,untouched)); // reflected
        gun={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        for(double bad:{-.001,.301,double(NAN),double(INFINITY)})
            assert(!lancer_starter_parts(gun,bad,100,untouched));
        assert(!lancer_starter_parts(gun,.18,0,untouched));
        assert(untouched.socket==before.socket&&untouched.grip==before.grip&&untouched.cord==before.cord);
    }
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
    game.original_button_held=true;
    LancerSawSample sample{};
    assert(lancer_saw_sample_from_world(frame,game,1000,sample));
    assert(sample.left_palm_m.x==1&&sample.starter_anchor_m.z==3&&
           sample.right_tracked&&sample.left_tracked&&sample.world==40&&
           sample.original_button_held);
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
