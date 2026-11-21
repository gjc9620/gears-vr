#define main existing_fire_owner_tests
#include "weapon_troika_fire_owner_test.cpp"
#undef main
#include "weapon_troika_gun_consumption.h"
#include <cstring>
namespace gun=gears_weapon::troika_gun_consumption;namespace math=gears_weapon::troika_mechanics;
using V=gears_vr::Vector;
V add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V rx(V p,double a){return {p.x,std::cos(a)*p.y-std::sin(a)*p.z,std::sin(a)*p.y+std::cos(a)*p.z};}
V rz(V p,double a){return {std::cos(a)*p.x-std::sin(a)*p.y,std::sin(a)*p.x+std::cos(a)*p.y,p.z};}
int main(){
    for(bool follows:{true,false})for(double a:{-.6,0.,.5})for(double b:{-.4,0.,.3}){
        math::Input in{};in.binding=11;in.calibration=12;in.units_per_metre=100;in.maximum_error_metres=.025;
        in.original_control_binding_and_reference_closed=true;in.control_bone={1,2};in.rotation_mask={4,1};
        in.space={math::Space::world,follows?math::Space::parent_bone:math::Space::world};
        for(auto& m:in.bone_world){m=math::identity();m[12]=100000;m[13]=-250000;m[14]=3100;}
        in.bone_world[2][14]+=25;in.bone_world[7][13]+=50;in.bone_world[8][13]+=55;in.bone_world[9][13]-=40;
        in.anchors={V{99987,-250040,3135},V{100013,-250040,3135}};
        math::Geometry g{};CHECK(math::geometry(in,g));
        gears_weapon::TroikaSerialCandidate candidate{};candidate.binding=11;candidate.calibration=12;
        candidate.fit.frame_ticket=13;candidate.fit.parent_angle=a;candidate.fit.child_angle=b;
        constexpr unsigned indices[]={0,1,2,7,8,9};std::array<math::Matrix,6> before{},actual{};
        for(unsigned i=0;i<6;++i)before[i]=actual[i]=in.bone_world[indices[i]];
        const V parent{100000,-250000,3100},child{0,0,25};
        for(unsigned i=1;i<6;++i){auto& m=actual[i];const V local=sub({m[12],m[13],m[14]},parent);
            const auto moved=i==1?rz(local,a):follows?rz(add(child,rx(sub(local,child),b)),a):add(rz(child,a),rx(sub(rz(local,a),rz(child,a)),b));
            m[12]=float(parent.x+moved.x);m[13]=float(parent.y+moved.y);m[14]=float(parent.z+moved.z);
            for(unsigned n=0;n<3;++n){const V axis{before[i][n*4],before[i][n*4+1],before[i][n*4+2]};
                const auto rotated=i==1?rz(axis,a):follows?rz(rx(axis,b),a):rx(rz(axis,a),b);
                m[n*4]=float(rotated.x);m[n*4+1]=float(rotated.y);m[n*4+2]=float(rotated.z);}
        }
        gun::Prediction prediction{};CHECK(gun::predict(g,candidate,before,prediction));gun::Match receipt{};
        CHECK(gun::matches(prediction,actual,receipt));CHECK(receipt.maximum_position_error_metres<.001&&receipt.maximum_rotation_error_radians<.0001);
        for(unsigned i=0;i<6;++i){auto bad=actual;bad[i][14]+=1;gun::Match out{.7,.8};CHECK(!gun::matches(prediction,bad,out));CHECK(out.maximum_position_error_metres==.7);}
        if(a!=0||b!=0)CHECK(!gun::matches(prediction,before,receipt)); // Actor-only success leaves old pose behind.
        auto wrong=actual;wrong[4]=actual[3];CHECK(!gun::matches(prediction,wrong,receipt)); // Distinct two muzzle identities.
        auto shear=before;shear[5][1]=.1f;auto unchanged=prediction;CHECK(!gun::predict(g,candidate,shear,unchanged));CHECK(!std::memcmp(&unchanged,&prediction,sizeof prediction));
        auto no_ticket=candidate;no_ticket.fit.frame_ticket=0;CHECK(!gun::predict(g,no_ticket,before,unchanged));
        no_ticket=candidate;++no_ticket.binding;CHECK(!gun::predict(g,no_ticket,before,unchanged));
        const std::array<float,3> relative{2,3,4};const auto& m=actual[3];
        const V socket{m[12]+2*m[0]+3*m[4]+4*m[8],m[13]+2*m[1]+3*m[5]+4*m[9],m[14]+2*m[2]+3*m[6]+4*m[10]};
        CHECK(gun::selected_socket(m,relative,socket,socket,100));auto off=socket;off.z+=.2;
        CHECK(!gun::selected_socket(m,relative,socket,off,100));CHECK(!gun::selected_socket(actual[4],relative,socket,socket,100));
        gears_vr::Matrix columns{};for(unsigned row=0;row<3;++row)for(unsigned n=0;n<3;++n)columns[n][row]=m[row*4+n];
        const auto angles=gears_vr::ue_angles(columns);std::array<std::int32_t,3> getter={gears_vr::to_rotator_units(angles.pitch),
            gears_vr::to_rotator_units(angles.yaw),gears_vr::to_rotator_units(angles.roll)};
        CHECK(gun::socket_rotation(m,{0,0,0},getter));getter[1]+=1000;CHECK(!gun::socket_rotation(m,{0,0,0},getter));
    }
    CHECK(gun::socket_rotation(math::actor_basis({0,16384,0}),{0,16384,0},{0,32768,0}));
    for(unsigned fault=0;fault<18;++fault){Fixture x;f::Run previous{x.source,x.decision,0};x.next();f::Run current{x.source,x.decision,0};
        CHECK(gun::later(previous,current,x.now));auto& s=current.source.frame;auto& d=current.control;
        if(fault==0)s.publication_sequence=previous.source.frame.publication_sequence;
        if(fault==1)s.scene_invocation=previous.source.frame.scene_invocation;
        if(fault==2)s.scene_qpc=previous.source.frame.scene_qpc;
        if(fault==3)s.published_ns=previous.source.frame.published_ns;
        if(fault==4)++current.source.entry_token;
        if(fault==5)++s.native.registration_cookie;
        if(fault==6)++s.driver_mesh.registry_index;
        if(fault==7)++s.reference_generation;
        if(fault==8)++s.producer.tracking[1];
        if(fault==9)++s.publisher_start;
        if(fault==10)++s.controllers.input_epoch;
        if(fault==11)++d.lease.id;
        if(fault==12)++d.press_token;
        if(fault==13){++d.right_press_sequence;s.controllers.hands[1].last_squeeze_press_sequence=d.right_press_sequence;}
        if(fault==14)--d.right_window_begin_ns;
        if(fault==15)d.both_owned=0;
        if(fault==16)x.now+=100000001;
        if(fault==17)s.control_policy=0;
        CHECK(!gun::later(previous,current,x.now));
    }
    std::printf("Troika later-Scene gun matrices/selected muzzle/lifetime: %u checks passed; synthetic independent transforms/source only, no game or C0 grant.\n",checks);
}
