#include "hand_object_grasp.h"
#include "hand_finger_pose.h"
#include "weapon_lancer_grasp_pose.h"
#include "weapon_lancer_interaction_guide.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main(){
    namespace g=gears_hand_object_grasp;namespace v=gears_vr;
    assert(g::proximity_socket(g::Kind::none,g::ProximityContact{.18,true,true},g::ProximityContact{.13,false,true})==g::Kind::support);
    assert(g::proximity_socket(g::Kind::support,g::ProximityContact{.18,false,true},g::ProximityContact{.17,true,true})==g::Kind::support);
    assert(g::proximity_socket(g::Kind::support,g::ProximityContact{.18,false,true},g::ProximityContact{.10,true,true})==g::Kind::starter);
    assert(g::proximity_socket(g::Kind::support,g::ProximityContact{.25,false,false},g::ProximityContact{.3,false,false})==g::Kind::none);
    assert(g::proximity_socket(g::Kind::starter,g::ProximityContact{.01,true,true},g::ProximityContact{.18,false,true},true)==g::Kind::starter);

    std::printf("released_socket_layout frame=%zu size=%zu anchor=%zu hold=%zu known=%zu key=%zu stereo=%zu controllers=%zu\n",
        sizeof(gears_weapon_hand::Frame),sizeof(g::ReleasedSocket),offsetof(g::ReleasedSocket,anchor),
        offsetof(g::ReleasedSocket,hold),offsetof(g::ReleasedSocket,known),
        offsetof(gears_weapon_hand::Frame,key),offsetof(gears_weapon_hand::Frame,stereo),
        offsetof(gears_weapon_hand::Frame,controllers));
    // A neutral hand crosses the handguard's generous trigger en route to C.
    // Retention must resist jitter, but cannot trap it in the wrong socket.
    assert(g::proximity_socket(g::Kind::support,.18,.01,.14,.14)==g::Kind::starter);
    assert(g::proximity_socket(g::Kind::starter,.01,.18,.14,.14)==g::Kind::support);
    assert(g::proximity_socket(g::Kind::support,.12,.11,.14,.14)==g::Kind::support);
    assert(g::proximity_socket(g::Kind::starter,.11,.12,.14,.14)==g::Kind::starter);
    assert(g::proximity_socket(g::Kind::support,.20,.30,.14,.14)==g::Kind::support);
    assert(g::proximity_socket(g::Kind::support,.23,.30,.14,.14)==g::Kind::none);
    assert(g::proximity_socket(g::Kind::none,.18,.01,.14,.14)==g::Kind::starter);
    assert(g::proximity_socket(g::Kind::none,.01,.18,.14,.14)==g::Kind::support);
    assert(g::proximity_socket(g::Kind::none,-1,.01,.14,.14)==g::Kind::none);
    assert(g::proximity_socket(g::Kind::starter,.01,.18,.14,.14,true)==g::Kind::starter);
    assert(g::proximity_socket(g::Kind::starter,.01,.18,.14,.14,false)==g::Kind::support);
    assert(g::proximity_socket(g::Kind::starter,.01,.23,.14,.14,true)==g::Kind::support);
    assert(g::proximity_socket(g::Kind::support,.18,.01,.14,.14,true)==g::Kind::starter);
    {
        // A real approach acquires C at the volume edge, then stops at its
        // folded centre. Its next missing pose must not mistake that approach
        // for continuing movement away from the socket.
        double anchor=-.12;
        for(double p:{-.08,-.04,-.015,0.})
            if(g::recenter_starter_anchor(std::abs(p),std::abs(p-.11)))anchor=p;
        assert(anchor==0.);
        assert(g::proximity_socket(g::Kind::starter,.04,.11,.14,.14,
            std::abs(anchor)<=.03)==g::Kind::starter);
        // An old edge anchor would instead have selected nearby support.
        assert(g::proximity_socket(g::Kind::starter,.04,.11,.14,.14,false)==g::Kind::support);
        // Do not follow an arbitrarily slow exit through the broad22cm zone.
        for(double p:{.04,.06})
            if(g::recenter_starter_anchor(std::abs(p),std::abs(p-.11)))anchor=p;
        assert(anchor==0.);
        assert(g::proximity_socket(g::Kind::starter,0.,.05,.14,.14,
            std::abs(.06-anchor)<=.03)==g::Kind::support);
        assert(g::recenter_starter_anchor(.20,.01));
        assert(!g::recenter_starter_anchor(-.01,.01));
        assert(!g::recenter_starter_anchor(.01,INFINITY));
    }
    {
        using Lease=gears_weapon_hand::LeaseKind;
        auto selected=g::proximity_socket(g::Kind::none,.10,.30,.14,.14);
        assert(selected==g::Kind::support); // Neutral acquisition.
        selected=g::proximity_control_handoff(selected,Lease::support,true);
        assert(selected==g::Kind::support); // Actual Grip phase saves only selection.
        selected=g::proximity_control_handoff(selected,Lease::support,true);
        assert(selected==g::Kind::support); // Released frame still has the old lease.
        selected=g::proximity_socket(selected,.18,.30,.14,.14);
        assert(selected==g::Kind::support); // Release returns to the existing22cm hover band.
        assert(g::proximity_socket(g::Kind::none,.18,.30,.14,.14)==g::Kind::none);
        assert(g::proximity_socket(selected,.23,.30,.14,.14)==g::Kind::none);
        assert(g::proximity_socket(selected,.18,.01,.14,.14)==g::Kind::starter);
        assert(g::proximity_control_handoff(selected,Lease::magazine,true)==g::Kind::none);
        assert(g::proximity_control_handoff(selected,Lease::ammo,true)==g::Kind::none);
        assert(g::proximity_control_handoff(selected,Lease::support,false)==g::Kind::none);
        assert(g::proximity_control_handoff(g::Kind::none,Lease::support,true)==g::Kind::none);
        assert(g::proximity_control_handoff(g::Kind::starter,Lease::none,true)==g::Kind::starter);
        assert(g::proximity_control_handoff(g::Kind::starter,Lease::none,false)==g::Kind::none);
        for(auto occupied:{Lease::support,Lease::magazine,Lease::ammo})
            assert(g::proximity_control_handoff(g::Kind::starter,occupied,true)==g::Kind::none);
        assert(g::proximity_control_handoff(selected,Lease::none,true)==g::Kind::support);
    }
    {
        gears_weapon_hand::Frame f{};f.left_tracked=f.left_grip_known=1;
        auto& input=f.controllers.hands[0];input.squeeze_active=input.grip_active=1;
        input.grip_flags=15;input.grip={{0,1,0},{0,0,0,1}};
        g::ReleasedSocket released;
        assert(released.bind(f,17));
        // A released part stays preferred when only the fitting/busy state
        //changes, including the next actual squeeze. It is not an action.
        f.reference_generation++;input.squeeze=1;f.left_grip_down=1;
        assert(released.retained(f,17));
        input.grip.position.x=.02;assert(released.retained(f,17));
        input.grip.position.x=.031;assert(!released.retained(f,17));
        input.grip.position.x=0;assert(!released.retained(f,17)); // No drifting reacquisition.
        input.squeeze=0;f.left_grip_down=0;
        assert(released.bind(f,17));assert(!released.retained(f,18));
        assert(released.bind(f,17));f.stereo.space_epoch++;assert(!released.retained(f,17));
        assert(released.bind(f,17));f.controllers.input_epoch++;assert(!released.retained(f,17));
        assert(released.bind(f,17));f.weapon.address++;assert(!released.retained(f,17));
        assert(released.bind(f,17));f.key.lifecycle++;assert(!released.retained(f,17));
        assert(released.bind(f,17));f.controllers.profile_generation++;assert(!released.retained(f,17));
        assert(released.bind(f,17));f.left_tracked=0;assert(!released.retained(f,17));
        f.left_tracked=1;input.squeeze=1;assert(!released.bind(f,17));
    }
    const gears_hand_wrist::Calibration c{true,{-.05,.004,.002},{1,0,0,0}};
    const v::Pose physical{{10,20,30},{0,0,0,1}};
    g::Target t{g::Kind::starter,{{15,20,30},{0,0,std::sqrt(.5),std::sqrt(.5)}},1};
    v::Pose out{};assert(g::apply(t,physical,c,100,out));
    // Genuine nearby grasps fit the whole contact smoothly.
    const auto q=v::multiply(out.orientation,v::inverse(c.grip_to_wrist_rotation));
    const auto d=v::transform(v::rotation_matrix(q),c.grip_to_wrist_metres);
    assert(std::abs(out.position.x-d.x*100-15)<1e-8);
    assert(std::abs(out.position.y-d.y*100-20)<1e-8);
    const auto sentinel=out;t.kind=g::Kind::magazine;t.grip.position.x=32.01;
    assert(!g::apply(t,physical,c,100,out)&&!std::memcmp(&out,&sentinel,sizeof out));
    t.kind=g::Kind::starter;t.grip.position.x=27;
    const auto unchanged_physical=physical;
    assert(g::apply(t,physical,c,100,out));
    const auto bounded_grip=v::multiply(out.orientation,v::inverse(c.grip_to_wrist_rotation));
    const auto bounded_offset=v::transform(v::rotation_matrix(bounded_grip),c.grip_to_wrist_metres);
    assert(std::abs(out.position.x-bounded_offset.x*100-26)<1e-8); //16cm maximum skin assistance
    assert(!std::memcmp(&physical,&unchanged_physical,sizeof physical));
    const auto bounded_sentinel=out;t.grip.position.x=32.01;
    assert(!g::apply(t,physical,c,100,out)&&!std::memcmp(&out,&bounded_sentinel,sizeof out));
    t.grip.position.x=15;g::BlendState state{};
    for(int i=0;i<=8;++i)state.update(t,physical,c,100,1000000000+i*20000000,out);
    assert(state.weight==1&&state.kind==g::Kind::starter);
    {
        auto still_holding=physical;still_holding.position.y+=40;
        assert(state.update(t,still_holding,c,100,1170000000,out));
        assert(state.kind==g::Kind::starter&&state.weight==1&&!state.position_fitted);
        // Pose-only fallback follows the actual current palm instead of
        // snapping it40cm back onto a no-longer-nearby part.
        const auto grip=v::multiply(out.orientation,v::inverse(c.grip_to_wrist_rotation));
        const auto offset=v::transform(v::rotation_matrix(grip),c.grip_to_wrist_metres);
        assert(std::abs(out.position.y-offset.y*100-still_holding.position.y)<1e-8);
    }
    {
        auto fitted=state;fitted.preset=g::Preset::authored;
        assert(g::keep_relative_fit(true,g::Kind::starter,fitted.kind,fitted.preset,7,7,91,91));
        assert(!g::keep_relative_fit(false,g::Kind::starter,fitted.kind,fitted.preset,7,7,91,91));
        assert(!g::keep_relative_fit(true,g::Kind::loading,fitted.kind,fitted.preset,7,7,91,91));
        assert(!g::keep_relative_fit(true,g::Kind::starter,fitted.kind,fitted.preset,8,7,91,91));
        assert(!g::keep_relative_fit(true,g::Kind::starter,fitted.kind,fitted.preset,7,7,92,91));
        auto shifted=physical;shifted.position.x+=50;
        const auto physical_before=shifted;
        assert(fitted.update({},shifted,c,100,1180000000,out,true));
        assert(fitted.weight==1&&fitted.kind==g::Kind::starter&&!fitted.position_fitted);
        // Retained cosmetic offset follows the NEW physical hand, never the
        // old world socket. Actual source pose bytes stay unchanged.
        assert(out.position.x>40);
        assert(!std::memcmp(&shifted,&physical_before,sizeof shifted));
        for(int i=0;i<9;++i)fitted.update({},shifted,c,100,1200000000+i*20000000,out,false);
        assert(fitted.kind==g::Kind::none&&fitted.weight==0); // Real release/unknown still decays out.
        g::BlendState empty{};assert(!empty.update({},shifted,c,100,1500000000,out,true));
    }
    auto moved=physical;moved.position.x+=100;
    assert(state.update({},moved,c,100,1180000000,out));
    // Release follows today's fresh hand, not yesterday's object world pose.
    assert(out.position.x>100&&out.position.x<120);
    for(int i=0;i<8;++i)state.update({},moved,c,100,1200000000+i*20000000,out);
    assert(state.kind==g::Kind::none&&state.weight==0);
    unsigned different_magazine_loading{},different_loading_starter{};
    for(unsigned side=0;side<2;++side)for(unsigned n=0;n<15;++n){
        v::Quaternion q1{},q2{},q3{};
        assert(gears_hand_fingers::rotation(side,n,1,{0,0,0,1},q1,gears_hand_fingers::Shape::magazine));
        assert(gears_hand_fingers::rotation(side,n,1,{0,0,0,1},q2,gears_hand_fingers::Shape::loading));
        assert(gears_hand_fingers::rotation(side,n,1,{0,0,0,1},q3,gears_hand_fingers::Shape::starter));
        if(n<12){different_magazine_loading+=std::abs(q1.z-q2.z)>1e-4;different_loading_starter+=std::abs(q2.z-q3.z)>1e-4;}
        for(const auto q0:{q1,q2,q3})assert(std::abs(q0.x*q0.x+q0.y*q0.y+q0.z*q0.z+q0.w*q0.w-1)<1e-8);
    }
    // Both small-part profiles pinch with thumb/index and relax the other
    // fingers. Index MCP/PIP differ on each hand; C remains a full grasp.
    assert(different_magazine_loading>=4&&different_loading_starter>=20);
    // Independent original-part axes: the starter barrel runs along gun+Y,
    // not gun-Z. Curl must wrap across it while the wrist approaches its side.
    for(double scale:{1.,1.25})for(double approach_angle:{-1.2,-.4,0.,.4,1.2})for(const auto kind:{g::Kind::loading,g::Kind::starter}){
        const v::Quaternion gun{0,0,std::sqrt(.5),std::sqrt(.5)},motion{0,0,std::sin(.35),std::cos(.35)};
        const v::Vector contact{100,200,300};v::Pose goal{},wrist{};
        const auto part=v::rotation_matrix(v::multiply(gun,motion));
        // Independent circular approach: the forearm approaches from this
        // side, rather than the old fixed gun-X side. The fitted wrist should
        // not add that angle as a bend while retaining the barrel curl axis.
        const auto inward=v::transform(part,{-std::cos(approach_angle),0,std::sin(approach_angle)});
        const v::Pose physical_wrist{{contact.x-inward.x*8*scale,contact.y-inward.y*8*scale,
            contact.z-inward.z*8*scale},{0,0,0,1}};
        const auto untouched=physical_wrist;
        assert(gears_weapon_lancer_grasp::part_grip(kind,gun,motion,contact,physical_wrist,scale,100,goal));
        assert(!std::memcmp(&physical_wrist,&untouched,sizeof physical_wrist));
        gears_hand_authored_mount::Mount m{};
        assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
            gears_hand_authored_mount::Side::left,scale,100,m));
        assert(gears_hand_wrist::native_wrist_target(goal,m.calibration,100,wrist)==gears_hand_wrist::Status::okay);
        const auto finger_axis=v::transform(v::rotation_matrix(wrist.orientation),{0,0,1});
        const auto original_barrel=v::transform(part,{0,1,0});
        if(kind==g::Kind::starter){
            assert(std::abs(finger_axis.x*original_barrel.x+finger_axis.y*original_barrel.y+finger_axis.z*original_barrel.z)>1.-1e-10);
            const auto relative=v::normalized(v::multiply(wrist.orientation,v::inverse(physical_wrist.orientation)));
            assert(2*std::acos(std::clamp(std::abs(relative.w),0.,1.))<=1.5707963267948967+1e-8);
        }else{
            // A flat E tab uses a pinch. Its snap must not impose C's large
            // cylindrical wrist turn on the unchanged physical approach.
            const auto relative=v::normalized(v::multiply(wrist.orientation,v::inverse(physical_wrist.orientation)));
            assert(2*std::acos(std::clamp(std::abs(relative.w),0.,1.))<=.5235987755982989+1e-8);
        }
        // Independent authored contact expected from the original full-skin
        // C fit. The former {10,0,0} put the barrel through the fixed palm.
        const v::Vector palm=kind==g::Kind::starter?
            v::Vector{9.083391056583642,-4.06789693573326,.35535626325030734}:v::Vector{9.75,-4.6,-2.1};
        const auto marker=v::transform(v::rotation_matrix(wrist.orientation),{palm.x*scale,palm.y*scale,palm.z*scale});
        assert(std::abs(wrist.position.x+marker.x-contact.x)<1e-8&&
            std::abs(wrist.position.y+marker.y-contact.y)<1e-8&&std::abs(wrist.position.z+marker.z-contact.z)<1e-8);
        // The old <5raw distance asserted the retired {10,0,0} art marker's
        // distance to the controller origin. Exact surface-anchor projection
        // above replaces that old art expectation. Existing apply() contact,
        // correction and maximum-distance guards remain unchanged and are
        // independently checked earlier; this is not permission to extend them.
    }
    // E's whole-gun pinch must preserve its paired wrist/finger frame when
    // approached naturally, including a rotated gun. The old C-derived
    // circular approach frame changed this wrist despite an exact contact.
    for(double angle:{-.8,0.,.8})for(double scale:{1.,1.25}){
        const v::Quaternion gun{0,0,std::sin(angle*.5),std::cos(angle*.5)};
        const auto expected=v::multiply(gun,v::Quaternion{
            .35355339059327373,.6123724356957946,.6123724356957946,-.35355339059327373});
        const v::Vector contact{100,200,300};
        const auto pad=v::transform(v::rotation_matrix(expected),{9.75*scale,-4.6*scale,-2.1*scale});
        const v::Pose physical_wrist{{contact.x-pad.x,contact.y-pad.y,contact.z-pad.z},expected};
        const auto untouched=physical_wrist;v::Pose grip{},skin{};
        assert(gears_weapon_lancer_grasp::part_grip(g::Kind::loading,gun,{0,0,0,1},
            contact,physical_wrist,scale,100,grip));
        gears_hand_authored_mount::Mount mount{};
        assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
            gears_hand_authored_mount::Side::left,scale,100,mount));
        assert(gears_hand_wrist::native_wrist_target(grip,mount.calibration,100,skin)==gears_hand_wrist::Status::okay);
        const auto delta=v::normalized(v::multiply(skin.orientation,v::inverse(expected)));
        assert(std::abs(delta.w)>1.-1e-10);
        assert(std::hypot(std::hypot(skin.position.x-physical_wrist.position.x,
            skin.position.y-physical_wrist.position.y),skin.position.z-physical_wrist.position.z)<1e-8);
        assert(!std::memcmp(&physical_wrist,&untouched,sizeof physical_wrist));
    }
    // A left wrist already aligned around the other end of the same bar
    // must not be turned180deg merely to impose +barrel on its finger row.
    for(double sign:{-1.,1.}){
        const v::Quaternion natural{0,std::sqrt(.5),sign*std::sqrt(.5),0};
        const v::Pose approach{{8,0,0},natural};v::Pose goal{},wrist{};
        assert(gears_weapon_lancer_grasp::part_grip(g::Kind::starter,{0,0,0,1},{0,0,0,1},{},approach,1,100,goal));
        gears_hand_authored_mount::Mount mount{};
        assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
            gears_hand_authored_mount::Side::left,1,100,mount));
        assert(gears_hand_wrist::native_wrist_target(goal,mount.calibration,100,wrist)==gears_hand_wrist::Status::okay);
        const auto fitted_wrist=wrist.orientation;
        assert(std::abs(fitted_wrist.x*natural.x+fitted_wrist.y*natural.y+fitted_wrist.z*natural.z+fitted_wrist.w*natural.w)>1.-1e-10);
    }
    {auto goal=out;assert(!gears_weapon_lancer_grasp::part_grip(g::Kind::none,{0,0,0,1},{0,0,0,1},{},physical,1,100,goal));
        assert(!std::memcmp(&goal,&out,sizeof out));
        // An axial wrist position no longer makes cosmetic roll undefined:
        // this uses its genuine orientation. It still grants no contact.
        const v::Pose axial{{0,8,0},{0,0,0,1}};
        assert(gears_weapon_lancer_grasp::part_grip(g::Kind::starter,{0,0,0,1},{0,0,0,1},{},axial,1,100,goal));}
    {gears_weapon_lancer_grasp::PartFit held;v::Pose fitted{};
        const v::Pose approach{{-6,0,.5},{0,0,0,1}};
        assert(held.solve(true,g::Kind::starter,{0,0,0,1},{0,0,0,1},{},approach,1,100,fitted));
        const auto original_physical=approach;
        for(unsigned i=0;i<=40;++i){
            const double a=double(i)/40*1.5707963267948966;
            const v::Quaternion motion{0,0,std::sin(a*.5),std::cos(a*.5)};
            const v::Vector contact{0,double(i)*.2,0};auto wrist=approach;wrist.position.x+=double(i)*.3;
            assert(held.solve(false,g::Kind::starter,{0,0,0,1},motion,contact,wrist,1,100,fitted));
            gears_hand_authored_mount::Mount mount{};v::Pose skin{};
            assert(gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
                gears_hand_authored_mount::Side::left,1,100,mount));
            assert(gears_hand_wrist::native_wrist_target(fitted,mount.calibration,100,skin)==gears_hand_wrist::Status::okay);
            const auto barrel=v::transform(v::rotation_matrix(motion),{0,1,0});
            const auto row=v::transform(v::rotation_matrix(skin.orientation),{0,0,1});
            assert(std::abs(row.x*barrel.x+row.y*barrel.y+row.z*barrel.z-held.starter_direction)<1e-8);
            const auto pad=v::transform(v::rotation_matrix(skin.orientation),
                {9.083391056583642,-4.06789693573326,.35535626325030734});
            const v::Vector gap{skin.position.x+pad.x-contact.x,skin.position.y+pad.y-contact.y,skin.position.z+pad.z-contact.z};
            const double along=gap.x*barrel.x+gap.y*barrel.y+gap.z*barrel.z;
            assert(std::hypot(std::hypot(gap.x-barrel.x*along,gap.y-barrel.y*along),gap.z-barrel.z*along)<1e-7);
            assert(std::abs(along)<=3.+1e-7); // Nearest point remains on the finite cylinder.
            // Original C skin Y20.06538..33.48919, original contact Y25.01551.
            // A valid centre alone is insufficient: keep both ends of the
            // paired authored finger-pad span on the actual handle too.
            const double pad_low=held.starter_direction>0?-6.157650975126031:-5.7468604155024545;
            const double pad_high=held.starter_direction>0?5.7468604155024545:6.157650975126031;
            assert(25.0155129328+along+pad_low>=20.065379712529424+.049);
            assert(25.0155129328+along+pad_high<=33.489185256451954-.049);
        }
        assert(!std::memcmp(&approach,&original_physical,sizeof approach));
        const auto before=fitted;auto invalid=approach;invalid.position.x=std::numeric_limits<double>::infinity();
        assert(!held.solve(false,g::Kind::starter,{0,0,0,1},{0,0,0,1},{},invalid,1,100,fitted));
        assert(!std::memcmp(&before,&fitted,sizeof fitted));held.reset();assert(!held.known);}
    {namespace guide=gears_weapon::lancer_guide;std::array<float,16> matrix{};for(unsigned i=0;i<4;++i)matrix[i*5]=1;
        const auto original=matrix;guide::Shape shape{};
        assert(guide::magazine(matrix,guide::Step::insert_magazine,shape)&&shape.count==62&&shape.count<=shape.lines.size());
        for(unsigned i=0;i<shape.count;++i){assert(std::abs(shape.lines[i].start.x)>3.&&std::abs(shape.lines[i].end.x)>3.);}
        assert(guide::magazine(matrix,guide::Step::remove_magazine,shape)&&shape.count==62);
        assert(!guide::magazine(matrix,guide::Step::none,shape));
        assert(guide::marker({10,20,30},{0,1,0},{0,0,1},6.5,shape)&&shape.count==28);
        for(unsigned i=0;i<shape.count;++i){assert(shape.lines[i].start.x==10&&shape.lines[i].end.x==10);}
        const auto marker=shape;
        assert(!guide::marker({10,20,30},{0,1,0},{0,1,0},6.5,shape));
        assert(shape.count==marker.count&&shape.lines[0].start.x==marker.lines[0].start.x);
        assert(guide::charging_step(0,false)==guide::Step::pull_charging_handle);
        assert(guide::charging_step(1,false)==guide::Step::pull_charging_handle);
        assert(guide::charging_step(1,true)==guide::Step::release_charging);
        assert(guide::charging_step(2,false)==guide::Step::wait_return);
        assert(guide::charging_step(3,true)==guide::Step::wait_return);
        assert(guide::charging_step(4,false)==guide::Step::none);
        assert(guide::charging(matrix,1.,guide::Step::pull_charging_handle,shape)&&shape.count==31);
        for(unsigned i=0;i<shape.count;++i)assert(v::finite(shape.lines[i].start)&&v::finite(shape.lines[i].end));
        assert(matrix==original);}
    std::puts("Context grasp: bounded visual correction, independent physical palm, release fade, three authored hand shapes; CPU only");
}
