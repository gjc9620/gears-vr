#include "weapon_lancer_magazine_pose.h"
#include "weapon_lancer_natural_magazine.h"
#include "weapon_lancer_feed_anchor.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

int main(){
    {
        // A Grip release/regrab while Trigger remains0 MUST NOT preserve the
        // original's capture. The former Trigger-only key missed this.
        gears_weapon::LancerMagazineGraspControl key;
        gears_vr::ControllerHand h{};h.squeeze_active=h.trigger_active=1;h.squeeze=1;
        h.pressed[gears_vr::control_squeeze]=1;
        assert(key.bind(gears_vr::control_squeeze,h)&&key.matches(h));
        h.squeeze=0;h.released[gears_vr::control_squeeze]++;assert(!key.matches(h));
        h.squeeze=1;h.pressed[gears_vr::control_squeeze]++;assert(!key.matches(h));
        assert(key.bind(gears_vr::control_squeeze,h)&&key.matches(h));
        h.grip.position={1,2,3};assert(key.matches(h)); // Moving never releases.
        h.trigger=1;h.pressed[gears_vr::control_trigger]++;assert(key.matches(h));
        h.squeeze_active=0;assert(!key.matches(h));h.squeeze_active=1;
        assert(key.bind(gears_vr::control_trigger,h)&&key.matches(h));
        h.squeeze=0;h.released[gears_vr::control_squeeze]++;assert(key.matches(h));
        h.trigger=0;h.released[gears_vr::control_trigger]++;assert(!key.matches(h));
        assert(!key.bind(gears_vr::control_menu,h)&&!key.known);
        // New original-mesh supply must use its OWN paired natural frame,
        // not an arbitrary old extraction registration.
        const gears_vr::Viewpoint wrist{{13,17,23},{.3,-.4,.7}};
        gears_vr::Viewpoint supplied{},recovered{};
        assert(gears_lancer_natural_magazine::grip.item_for_wrist(wrist,supplied));
        assert(gears_lancer_natural_magazine::grip.wrist_for_item(supplied,recovered));
        assert(std::hypot(recovered.position.x-wrist.position.x,recovered.position.y-wrist.position.y,recovered.position.z-wrist.position.z)<1e-9);
        const auto a=gears_vr::ue_matrix(wrist.rotation),b=gears_vr::ue_matrix(recovered.rotation);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)assert(std::abs(a[i][j]-b[i][j])<1e-9);
    }
    {
        // Independent 90-degree magazine-space frame: the wrist offset
        // rotates with the item, while model scale affects translation only.
        const double h=std::sqrt(.5);
        gears_weapon::LancerMagazineGripPose pose{{{3,4,5},{0,0,h,h}}};
        const gears_vr::Viewpoint item{{100,200,300},{0,1.5707963267948966,0}};
        for(double scale:{.5,1.,2.}){
            gears_vr::Viewpoint wrist{},recovered{};
            assert(pose.wrist_for_item(item,wrist,scale));
            assert(std::abs(wrist.position.x-(100-4*scale))<1e-9);
            assert(std::abs(wrist.position.y-(200+3*scale))<1e-9);
            assert(std::abs(wrist.position.z-(300+5*scale))<1e-9);
            const auto axes=gears_vr::ue_matrix(wrist.rotation);
            assert(std::abs(axes[0][0]+1)<1e-9&&std::abs(axes[1][1]+1)<1e-9&&std::abs(axes[2][2]-1)<1e-9);
            assert(pose.item_for_wrist(wrist,recovered,scale));
            assert(std::hypot(recovered.position.x-100,recovered.position.y-200,recovered.position.z-300)<1e-9);
        }
        // Both directions must fail without publishing a partial result.
        for(unsigned bad=0;bad<4;++bad){
            auto candidate=pose;double scale=1.;
            if(bad==0)candidate.wrist_in_magazine.orientation={0,0,0,0};
            if(bad==1)candidate.wrist_in_magazine.position.x=std::numeric_limits<double>::infinity();
            if(bad==2)scale=0;if(bad==3)scale=std::numeric_limits<double>::quiet_NaN();
            gears_vr::Viewpoint out{{11,22,33},{.1,.2,.3}};
            assert(!candidate.wrist_for_item(item,out,scale));
            assert(out.position.x==11&&out.position.y==22&&out.position.z==33&&out.rotation.yaw==.2);
            assert(!candidate.item_for_wrist(item,out,scale));
            assert(out.position.x==11&&out.position.y==22&&out.position.z==33&&out.rotation.yaw==.2);
        }
    }
    {
        gears_weapon::LancerMagazineGraspMount mount;
        const gears_vr::Pose grip{{15,25,35},{.2,.3,.1,.9}};
        const gears_vr::Viewpoint item{{18,26,32},{.4,.6,-.8}};gears_vr::Viewpoint result{};
        assert(mount.capture(grip,item)&&mount.apply(grip,result));
        assert(std::hypot(std::hypot(result.position.x-item.position.x,result.position.y-item.position.y),result.position.z-item.position.z)<1e-9);
        const auto expected=gears_vr::ue_matrix(item.rotation),actual=gears_vr::ue_matrix(result.rotation);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)assert(std::abs(expected[i][j]-actual[i][j])<1e-9);
        auto moved=grip;moved.position.x+=4;
        assert(mount.apply(moved,result)&&std::abs(result.position.x-item.position.x-4)<1e-9);
        std::array<float,16> gun{};gun[0]=gun[5]=gun[10]=gun[15]=1;
        gears_vr::Viewpoint seated{};assert(gears_weapon::lancer_seated_magazine(gun,seated));
        auto near=seated.position;near.z-=5;
        assert(gears_weapon::lancer_magazine_grab(gun,near,100));
        near.x+=11.9;assert(gears_weapon::lancer_magazine_grab(gun,near,100));
        near.x+=.2;assert(!gears_weapon::lancer_magazine_grab(gun,near,100));
        // The enlarged lower cap extends below the old26cm total height.
        near=seated.position;near.z-=27.9;
        assert(gears_weapon::lancer_magazine_grab(gun,near,100));
        near.z-=.2;assert(!gears_weapon::lancer_magazine_grab(gun,near,100));
        // A diagonal just outside the capsule's round cap must not acquire
        // merely because it lies inside the equivalent bounding box.
        near=seated.position;near.x+=11;near.z-=22;
        assert(!gears_weapon::lancer_magazine_grab(gun,near,100));
    }
    {
        // Independent authored gun-space wrist, original fixed feed pivot.
        // Forward and inverse must retain the same new original-only frame.
        std::array<float,16> gun{};gun[0]=gun[5]=gun[10]=gun[15]=1;
        gears_vr::Viewpoint seated{},wrist{},item{};
        assert(gears_weapon::lancer_seated_magazine(gun,seated));
        const auto& grip=gears_lancer_natural_magazine::grip;
        assert(grip.valid()&&grip.wrist_for_item(seated,wrist));
        const gears_vr::Vector expected{5.414356690254154,24.979376793708166,-15.81100777612283};
        assert(std::hypot(wrist.position.x-expected.x,wrist.position.y-expected.y,wrist.position.z-expected.z)<1e-5);
        const auto expected_axes=gears_vr::rotation_matrix({-0.061628416716219339,0.70441602640275869,-0.64085638205578854,-0.29883623873011983});
        const auto actual_axes=gears_vr::ue_matrix(wrist.rotation);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)assert(std::abs(expected_axes[i][j]-actual_axes[i][j])<1e-6);
        assert(grip.item_for_wrist(wrist,item));
        assert(std::hypot(item.position.x-seated.position.x,item.position.y-seated.position.y,item.position.z-seated.position.z)<1e-9);
    }
    gears_vr::Viewpoint left{{100,200,300},{0,0,0}},magazine{};
    assert(gears_weapon::lancer_magazine_from_left(left,magazine));
    assert(std::abs(magazine.position.x-119.03603543)<1e-5);
    assert(std::abs(magazine.position.y-194.43382251)<1e-5);
    assert(std::abs(magazine.position.z-300.13964627)<1e-5);
    gears_vr::Viewpoint doubled{};
    assert(gears_weapon::lancer_magazine_from_left(left,doubled,2.));
    assert(std::abs(doubled.position.x-138.07207086)<1e-5);
    assert(std::abs(doubled.position.y-188.86764502)<1e-5);
    assert(std::abs(doubled.position.z-300.27929254)<1e-5);
    assert(!gears_weapon::lancer_magazine_from_left(left,doubled,0.));
    assert(!gears_weapon::lancer_magazine_from_left(left,doubled,std::numeric_limits<double>::infinity()));
    const auto basis=gears_vr::ue_matrix(magazine.rotation);
    constexpr double expected[3][3]={
        {.03740158,.13477868,-.99016975},
        {-.03192186,-.99019607,-.13598739},
        {-.99879037,.03669508,-.03273385}};
    for(unsigned axis=0;axis<3;++axis)for(unsigned component=0;component<3;++component)
        assert(std::abs(basis[component][axis]-expected[axis][component])<2e-4);
    left.rotation.yaw=1.2;
    assert(gears_weapon::lancer_magazine_from_left(left,magazine));
    const auto rotated=gears_vr::transform(gears_vr::ue_matrix(left.rotation),
                                          {19.03603543,-5.56617749,.13964627});
    assert(std::abs(magazine.position.x-(left.position.x+rotated.x))<1e-5);
    gears_vr::Viewpoint recovered{},replayed{};
    assert(gears_weapon::lancer_left_for_magazine(magazine,recovered));
    assert(gears_weapon::lancer_magazine_from_left(recovered,replayed));
    assert(std::abs(replayed.position.x-magazine.position.x)<1e-3);
    assert(std::abs(replayed.position.y-magazine.position.y)<1e-3);
    assert(std::abs(replayed.position.z-magazine.position.z)<1e-3);
    const auto expected_axes=gears_vr::ue_matrix(magazine.rotation);
    const auto replayed_axes=gears_vr::ue_matrix(replayed.rotation);
    for(unsigned axis=0;axis<3;++axis)for(unsigned component=0;component<3;++component)
        assert(std::abs(replayed_axes[component][axis]-expected_axes[component][axis])<2e-4);
    for(double scale:{.8,1.25,2.}){
        gears_vr::Viewpoint fitted{},inverse{};
        assert(gears_weapon::lancer_magazine_from_left(left,fitted,scale));
        assert(gears_weapon::lancer_left_for_magazine(fitted,inverse,scale));
        assert(std::hypot(std::hypot(inverse.position.x-left.position.x,inverse.position.y-left.position.y),
            inverse.position.z-left.position.z)<1e-3);
    }
    const auto sentinel=recovered;
    assert(!gears_weapon::lancer_left_for_magazine(magazine,recovered,0));
    assert(recovered.position.x==sentinel.position.x&&recovered.position.y==sentinel.position.y&&recovered.position.z==sentinel.position.z);
    left.position.x=std::numeric_limits<double>::infinity();
    assert(!gears_weapon::lancer_magazine_from_left(left,magazine));
    std::puts("Lancer original magazine hand mount passed");
}
