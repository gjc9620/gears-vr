#include "lancer_debug_volume_geometry.h"
// Optional visual inspection of the ACTUAL logical acquisition shapes. This
// does not create UE colliders, grants, animation writers or persistent lines.
// Red outlines are real acquisition volumes, cyan axes are pose-only anchors.
// Numbered nodes match the owner's nine-row contact inventory. Normal yellow
// unfinished-reload guidance remains a separate consumer.
namespace lancer_interaction_ranges {
namespace hand=gears_weapon_hand;
namespace dim=gears_weapon::lancer_contact;
namespace draw=lancer_interaction_guide;
namespace line=gears_weapon::lancer_guide;
using V=gears_vr::Vector;
std::atomic<bool> enabled{};
std::uint64_t last_frame{};
struct Range {
    unsigned id{},kind{}; // 0: axes only, 1: sphere, 2: capsule, 3: cylinder
    V center{},axis{},right{},up{};
    double radius{},half{};
    unsigned char r{},g{},b{};
};
V plus(V a,V b) noexcept {return line::add(a,b);}
V scale(V a,double n) noexcept {return line::scale(a,n);}
V offset(Range const& s,double axial,double x,double y) noexcept {
    return plus(s.center,plus(scale(s.axis,axial),plus(scale(s.right,x),scale(s.up,y))));
}
bool edge(line::Shape& s,V a,V b) noexcept {
    if(s.count>=s.lines.size()||!gears_vr::finite(a)||!gears_vr::finite(b))return false;
    s.lines[s.count++]={a,b};return true;
}
bool wire(Range const& r,double units,line::Shape& out) noexcept {
    out={};const double tau=6.28318530717958647692;
    if(r.kind==0){
        const double n=.025*units;
        for(auto a:{r.axis,r.right,r.up})if(!edge(out,plus(r.center,scale(a,-n)),plus(r.center,scale(a,n))))return false;
    }else if(r.kind==3){
        for(unsigned i=0;i<8;++i){const double a=tau*i/8.,b=tau*(i+1)/8.;
            const auto p=offset(r,-r.half,r.radius*std::cos(a),r.radius*std::sin(a));
            const auto q=offset(r,r.half,r.radius*std::cos(a),r.radius*std::sin(a));
            if(!edge(out,p,offset(r,-r.half,r.radius*std::cos(b),r.radius*std::sin(b)))||
               !edge(out,q,offset(r,r.half,r.radius*std::cos(b),r.radius*std::sin(b)))||!edge(out,p,q))return false;
        }
    }else if(r.kind==2){
        // Both end rings and two orthogonal capsule meridians: the extra
        // hemispherical radius is included, unlike a cylinder's flat ends.
        for(unsigned i=0;i<8;++i){const double a=tau*i/8.,b=tau*(i+1)/8.;
            for(double end:{-r.half,r.half})if(!edge(out,offset(r,end,r.radius*std::cos(a),r.radius*std::sin(a)),
                offset(r,end,r.radius*std::cos(b),r.radius*std::sin(b))))return false;
        }
        for(unsigned plane=0;plane<2;++plane){
            for(double sign:{-1.,1.})if(!edge(out,offset(r,-r.half,plane?0:sign*r.radius,plane?sign*r.radius:0),
                offset(r,r.half,plane?0:sign*r.radius,plane?sign*r.radius:0)))return false;
            for(unsigned end=0;end<2;++end)for(unsigned i=0;i<8;++i){
                const double a=tau*.5*i/8.,b=tau*.5*(i+1)/8.,sign=end?1.:-1.;
                const auto p=offset(r,sign*(r.half+r.radius*std::sin(a)),plane?0:r.radius*std::cos(a),plane?r.radius*std::cos(a):0);
                const auto q=offset(r,sign*(r.half+r.radius*std::sin(b)),plane?0:r.radius*std::cos(b),plane?r.radius*std::cos(b):0);
                if(!edge(out,p,q))return false;
            }
        }
    }
    return true;
}
bool pose_axes(Range& r,gears_vr::Pose const& p) noexcept {
    try{const auto rotation=gears_vr::rotation_matrix(p.orientation);
        r.axis=gears_vr::transform(rotation,{1,0,0});
        r.right=gears_vr::transform(rotation,{0,1,0});
        r.up=gears_vr::transform(rotation,{0,0,1});return true;
    }catch(...){return false;}
}
bool sphere_call(hand::Frame const& f,playground::Args& call,hand::ObjectIdentity& function) noexcept {
    if(!draw::current(f)||!call.valid||call.r.size>64||!nativeequipment::identify(nullptr,call.r.function,function))return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    if(!registered(owner)||!registered(package)||!playground::named(owner,L"Actor")||!playground::named(package,L"Engine")||
       get<uptr>(package+0x40)!=0||(get<unsigned>(call.r.function+0xf4)&0x2400)!=0x2400||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr)))return false;
    const wchar_t* names[]={L"Center",L"Radius",L"Segments",L"R",L"G",L"B",L"bPersistentLines"};
    const wchar_t* kinds[]={L"StructProperty",L"FloatProperty",L"IntProperty",L"ByteProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned sizes[]={12,4,4,1,1,1,4};std::uint64_t occupied{};
    for(unsigned i=0;i<7;++i){const auto p=call.prop(names[i]);
        if(!hammerburst_animation::input_field(call,p,kinds[i],sizes[i]))return false;
        const auto at=get<unsigned>(p+0x94);if(at>64-sizes[i])return false;
        for(unsigned j=0;j<sizes[i];++j){const auto bit=std::uint64_t(1)<<(at+j);if(occupied&bit)return false;occupied|=bit;}
        if(i==0){const auto type=get<uptr>(p+0xd0);
            if(!registered(type)||!playground::named(type,L"Vector")||get<unsigned>(type+0x88)!=12)return false;}
    }
    return call.set(L"Segments",12).valid&&nativelancerfollow::bool_arg(call,L"bPersistentLines",false);
}
bool call_current(hand::Frame const& f,playground::Args const& call,hand::ObjectIdentity const& expected) noexcept {
    hand::ObjectIdentity now{};
    return draw::transport_current(f)&&registered(f.world.address)&&
        nativeequipment::identify(nullptr,call.r.function,now)&&hand::same(now,expected);
}
bool lines(hand::Frame const& f,playground::Args& call,hand::ObjectIdentity const& fn,line::Shape const& shape,Range const& r) noexcept {
    if(!call.set(L"R",r.r).set(L"G",r.g).set(L"B",r.b).valid)return false;
    for(unsigned i=0;i<shape.count;++i){auto const& e=shape.lines[i];
        if(!call_current(f,call,fn)||!call.set(L"LineStart",playground::Vector{float(e.start.x),float(e.start.y),float(e.start.z)})
            .set(L"LineEnd",playground::Vector{float(e.end.x),float(e.end.y),float(e.end.z)}).valid||!call.run())return false;
    }return true;
}
void number(Range const& r,double units,line::Shape& out) noexcept {
    // Small world-space digits need no new Canvas hook/font/material. They
    // identify the nine nodes without pretending a pure pose is a collider.
    constexpr unsigned masks[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
    constexpr double points[7][4]={{0,2,1,2},{1,2,1,1},{1,1,1,0},{1,0,0,0},{0,0,0,1},{0,1,0,2},{0,1,1,1}};
    const auto origin=plus(r.center,plus(scale(r.right,r.radius+.02*units),scale(r.up,.02*units)));
    const double size=.012*units;
    for(unsigned i=0;i<7;++i)if(masks[r.id]&(1u<<i)){
        auto p=[&](double x,double y){return plus(origin,plus(scale(r.right,x*size),scale(r.up,y*size)));};
        edge(out,p(points[i][0],points[i][1]),p(points[i][2],points[i][3]));
    }
}
#include "pose_editor_preview.inl"
void tick(uptr pc) noexcept {
    gears_pose_editor::Config view{};
    const bool connected=pose_editor_client::read(view,5)&&view.enabled;
    const bool show_volumes=enabled.load(std::memory_order_acquire)||(connected&&gears_pose_editor::volumes(view));
    const bool show_poses=connected&&gears_pose_editor::poses(view);
    if((!show_volumes&&!show_poses)||lancerreload_runtime::stop_requested.load(std::memory_order_acquire))return;
    hand::Frame f{};hand::GripGameState game{};
    if(!weaponhand::client.copy_frame_on_gt(f,game)||f.controller.address!=pc||f.key.frame==last_frame||
       !game.known||!game.main_hand_held||game.animation_owns_hands||!draw::current(f))return;
    gears_committed_interaction::Frame actual{};vrequip::MeshPoseProbe before{},after{};
    if(!lancerreload_visual::interaction(f,actual)||!vrequip::mesh_pose_probe(f.weapon.address,before))return;
    const double units=actual.right.units_per_metre;
    if(!std::isfinite(units)||units<10||units>500)return;
    std::array<float,16> m{};std::memcpy(m.data(),before.local_to_world,sizeof(before.local_to_world));
    gears_weapon::LancerFeedWorld feed{};V support{};
    if(!gears_weapon::lancer_feed_world(m,feed)||!gears_weapon_lancer_foregrip::grip_in_mesh(support))return;
    const double size=std::hypot(std::hypot(double(m[0]),double(m[1])),double(m[2]));
    const V x{m[0]/size,m[1]/size,m[2]/size},y{m[4]/size,m[5]/size,m[6]/size};
    std::array<Range,9> ranges{};std::array<line::Shape,9> authored_shapes{};unsigned count{};
    auto add=[&](unsigned id,unsigned kind,V center,double radius,double half,unsigned char r,unsigned char g,unsigned char b){
        if(count>=ranges.size()||!gears_vr::finite(center))return false;
        ranges[count++]={id,kind,center,feed.outward_axis,x,y,radius,half,r,g,b};return true;
    };
    const bool selected=lancerreload_contact::authored_profile_current(f);
    const std::array<double,16> identity_delta{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    const auto authored=[&](unsigned id,gears_lancer_authored_volume::Volume const* volume,
        bool moving,std::array<double,16> const& delta){
        if(!volume||count>=ranges.size())return false;
        lancer_debug_volume::Wire shape{};
        if(!lancer_debug_volume::volume_wire(*volume,units,size,m,moving,delta,shape)||!shape.count)return false;
        V center{};
        for(unsigned i=0;i<shape.count;++i)center=plus(center,plus(shape.lines[i].start,shape.lines[i].end));
        center=scale(center,1./(2.*shape.count));
        const auto slot=count;
        if(!add(id,4,center,volume->radius*100.*size,0,255,40,40))return false;
        for(unsigned i=0;i<shape.count;++i)if(!edge(authored_shapes[slot],shape.lines[i].start,shape.lines[i].end))return false;
        return true;
    };
    if(!add(1,0,actual.right.right_grip.position,0,0,70,230,255)||!pose_axes(ranges[count-1],actual.right.right_grip))return;
    // support_geometry() is relative to the main grip; drawing it as world
    // coordinates would be wrong. Project the SAME controller Grip marker
    // through the actual gun component instead of its visible surface marker.
    if(selected&&gears_lancer_workbench_profile::support())(void)authored(2,gears_lancer_workbench_profile::support(),false,identity_delta);
    else add(2,1,line::point(m,support),dim::support_radius*units,0,255,40,40);
    if(!lancerreload_visual::active&&selected&&gears_lancer_workbench_profile::magazine())
        (void)authored(3,gears_lancer_workbench_profile::magazine(),false,identity_delta);
    else if(!lancerreload_visual::active)add(3,2,plus(feed.point_raw,scale(feed.outward_axis,dim::magazine_axis_length*units*.5)),
        dim::magazine_radius*units,dim::magazine_axis_length*units*.5,255,40,40);
    if(lancerreload_contact::resume_original&&lancerreload_visual::active&&lancerreload_visual::last_grab_known&&
       hand::same(f.weapon,lancerreload_visual::saved.weapon))
        add(4,1,lancerreload_visual::last_grab_world,dim::retained_magazine_radius*units,0,255,40,40);
    if(lancerreload_visual::active&&lancerreload_contact::held.kind!=hand::LeaseKind::none&&
       hand::same(f.weapon,lancerreload_visual::saved.weapon)){
        if(!add(5,0,actual.left_grip.position,0,0,70,230,255)||!pose_axes(ranges[count-1],actual.left_grip))return;
    }
    // Acquisition is at the closed contact; the held E stroke is a separate
    // constrained motion, never an enlargement of this initial region.
    gears_weapon::SnubSlideGeometry front{};
    if(selected&&gears_lancer_workbench_profile::charging()){
        const auto& part=lancer_charging::region_part;const auto now=weaponhand::clock_ns(nullptr);
        if(lancer_charging::current_geometry(f)&&part.known&&part.identity==lancer_charging::geometry.identity&&part.upload&&
           lancerhandle_visual::ready_for(f)&&part.owner_handle==lancerhandle_visual::current_handle()&&
           part.at_ns>0&&now>=part.at_ns&&now-part.at_ns<=100000000&&
           part.raw>=0&&part.raw<=gears_weapon::lancer_front::travel_raw+.001){
            auto delta=identity_delta;delta[13]=-part.raw;
            (void)authored(6,gears_lancer_workbench_profile::charging(),true,delta);
        }
    }else if(gears_weapon::lancer_front::geometry(units,front))add(6,1,
        line::point(m,gears_weapon::lancer_front::contact_raw),front.contact_radius*units*size,0,255,40,40);
    lancersaw_visual::Observation saw{};
    if(lancersaw_visual::observe(f,saw)){
        if(selected&&gears_lancer_workbench_profile::starter()){
            if(!std::memcmp(saw.gun.data(),m.data(),sizeof(m)))
                (void)authored(7,gears_lancer_workbench_profile::starter(),true,saw.part_delta);
        }else add(7,1,saw.grasp,dim::starter_radius*units,0,255,40,40);
    }
    add(8,3,plus(feed.point_raw,scale(feed.outward_axis,(dim::snap_near+dim::snap_far)*units*.5)),
        dim::snap_radius*units,(dim::snap_far-dim::snap_near)*units*.5,80,220,255);
    add(9,3,plus(feed.point_raw,scale(feed.outward_axis,(dim::retain_near+dim::retain_far)*units*.5)),
        dim::retain_radius*units,(dim::retain_far-dim::retain_near)*units*.5,190,125,255);
    if(!vrequip::mesh_pose_probe(f.weapon.address,after)||before.mesh!=after.mesh||before.asset!=after.asset||
       std::memcmp(before.local_to_world,after.local_to_world,sizeof(before.local_to_world))||!draw::current(f))return;
    playground::Args line_call(f.world.address,L"DrawDebugLine"),sphere(f.world.address,L"DrawDebugSphere");
    hand::ObjectIdentity line_fn{},sphere_fn{};
    if(!draw::line_call(f,line_call,line_fn)||!sphere_call(f,sphere,sphere_fn))return;
    last_frame=f.key.frame;
    if(show_poses)preview(f,actual,m,units,view,line_call,line_fn);
    for(unsigned i=0;show_volumes&&i<count;++i){auto const& r=ranges[i];line::Shape shape{};
        if(r.kind==4)shape=authored_shapes[i];
        else if(r.kind==1){
            if(!call_current(f,sphere,sphere_fn)||!sphere.set(L"Center",playground::Vector{float(r.center.x),float(r.center.y),float(r.center.z)})
                .set(L"Radius",float(r.radius)).set(L"R",r.r).set(L"G",r.g).set(L"B",r.b).valid||!sphere.run())return;
        }else if(!wire(r,units,shape))return;
        // The centre cross marks the coordinate anchor without inventing a
        // trigger component. Wire capsule + cross + numeral fits64 lines.
        if(r.kind!=0)for(auto axis:{r.axis,r.right,r.up}){
            if(!edge(shape,plus(r.center,scale(axis,-.01*units)),plus(r.center,scale(axis,.01*units))))return;
        }
        number(r,units,shape);
        if(!lines(f,line_call,line_fn,shape,r))return;
    }
}
}
