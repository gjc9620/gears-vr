// One-frame next-part guide on the existing original Actor drawing path.
namespace longshot_runtime {
namespace guide_detail {
namespace anim=hammerburst_animation;
bool current(hand::Frame const& f) noexcept {return selected(f)&&nativeequipment::current(nullptr,f);}
bool draw(hand::Frame const& f,V center,double radius,bool warning=false) noexcept {
    if(!gears_vr::finite(center)||!std::isfinite(radius)||radius<=0||radius>20||!current(f))return false;
    playground::Args call(f.world.address,L"DrawDebugSphere");
    if(!call.valid||!registered(call.r.function)||call.r.size>64)return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    if(!registered(owner)||!registered(package)||!playground::named(owner,L"Actor")||
       !playground::named(package,L"Engine")||get<uptr>(package+0x40)||
       (get<unsigned>(call.r.function+0xf4)&0x2400)!=0x2400||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr)))return false;
    const wchar_t* fields[]={L"Center",L"Radius",L"Segments",L"R",L"G",L"B",L"bPersistentLines"};
    const wchar_t* kinds[]={L"StructProperty",L"FloatProperty",L"IntProperty",L"ByteProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned bytes[]={12,4,4,1,1,1,4};std::uint64_t occupied{};
    for(unsigned i=0;i<7;++i){
        const auto p=call.prop(fields[i]);
        if(!anim::input_field(call,p,kinds[i],bytes[i]))return false;
        const auto offset=get<unsigned>(p+0x94);if(offset>64-bytes[i])return false;
        for(unsigned n=0;n<bytes[i];++n){const auto bit=std::uint64_t(1)<<(offset+n);if(occupied&bit)return false;occupied|=bit;}
        if(i==0){const auto type=get<uptr>(p+0xd0);
            if(!registered(type)||!playground::named(type,L"Vector")||get<unsigned>(type+0x88)!=12)return false;}
    }
    const playground::Vector point{float(center.x),float(center.y),float(center.z)};
    return call.set(L"Center",point).set(L"Radius",float(radius)).set(L"Segments",12)
        .set(L"R",static_cast<unsigned char>(255)).set(L"G",static_cast<unsigned char>(warning?64:225))
        .set(L"B",static_cast<unsigned char>(warning?64:0)).valid&&
        nativelancerfollow::bool_arg(call,L"bPersistentLines",false)&&current(f)&&call.run();
}
bool transport_current(hand::Frame const& f) noexcept {
    return weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready()&&selected(f)&&
        weaponhand::client.registration_alive(f.key.registration_cookie,f.key.native_started_qpc)&&
        hand::frame_active(f,weaponhand::clock_ns(nullptr))&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address);
}
bool lines(hand::Frame const& f,gears_weapon_longshot_guide::Shape const& shape,bool warning=false) noexcept {
    if(!shape.count||shape.count>shape.lines.size()||!current(f))return false;
    const auto began=weaponhand::clock_ns(nullptr);
    playground::Args call(f.world.address,L"DrawDebugLine");hand::ObjectIdentity function{};
    if(!call.valid||call.r.size>64||!nativeequipment::identify(nullptr,call.r.function,function))return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    if(!registered(owner)||!registered(package)||!playground::named(owner,L"Actor")||
       !playground::named(package,L"Engine")||get<uptr>(package+0x40)||
       (get<unsigned>(call.r.function+0xf4)&0x2400)!=0x2400||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr)))return false;
    const wchar_t* fields[]={L"LineStart",L"LineEnd",L"R",L"G",L"B",L"bPersistentLines"};
    const wchar_t* kinds[]={L"StructProperty",L"StructProperty",L"ByteProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned sizes[]={12,12,1,1,1,4};std::uint64_t occupied{};
    for(unsigned i=0;i<6;++i){const auto p=call.prop(fields[i]);
        if(!anim::input_field(call,p,kinds[i],sizes[i]))return false;
        const auto offset=get<unsigned>(p+0x94);if(offset>64-sizes[i])return false;
        for(unsigned n=0;n<sizes[i];++n){const auto bit=std::uint64_t(1)<<(offset+n);if(occupied&bit)return false;occupied|=bit;}
        if(i<2){const auto type=get<uptr>(p+0xd0);
            if(!registered(type)||!playground::named(type,L"Vector")||get<unsigned>(type+0x88)!=12)return false;}
    }
    if(!call.set(L"R",static_cast<unsigned char>(255)).set(L"G",static_cast<unsigned char>(warning?64:225))
        .set(L"B",static_cast<unsigned char>(warning?64:0)).valid||!nativelancerfollow::bool_arg(call,L"bPersistentLines",false))return false;
    unsigned drawn{};
    for(unsigned i=0;i<shape.count;++i){const auto now=weaponhand::clock_ns(nullptr);hand::ObjectIdentity live{};
        // Optional cue batch only: never extend this source lifetime or delay
        // an action/restore to finish an outline. Actor graph brackets the
        // synchronous GT batch; every edge retains source and function checks.
        if(now<began||now-began>8000000||!transport_current(f)||
           !nativeequipment::identify(nullptr,call.r.function,live)||!hand::same(function,live))break;
        const auto& edge=shape.lines[i];if(!gears_vr::finite(edge.start)||!gears_vr::finite(edge.end))break;
        const playground::Vector a{float(edge.start.x),float(edge.start.y),float(edge.start.z)};
        const playground::Vector b{float(edge.end.x),float(edge.end.y),float(edge.end.z)};
        if(!call.set(L"LineStart",a).set(L"LineEnd",b).valid||!call.run())break;++drawn;
    }
    const bool closed=current(f);static unsigned rows{},previous{~0u};
    if(log_file&&rows<12&&previous!=unsigned(stage)){
        fwprintf(log_file,L"longshot_guide_lines frame=%llu stage=%u drawn=%u/%u closing_current=%u one_frame=1\n",
            f.key.frame,unsigned(stage),drawn,shape.count,unsigned(closed));fflush(log_file);++rows;previous=unsigned(stage);
    }
    return drawn&&closed;
}
bool bolt_arrow(P const& root,double size,gears_weapon_longshot_guide::Shape& shape) noexcept {
    if(!bolt::prepared)return false;
    const bool close=stage==Stage::waiting_close||stage==Stage::closing;
    const double start=(std::clamp)(bolt::last_wanted,0.,1.);
    const double finish=(std::clamp)(start+(close?-.18:.18),0.,1.);
    float a_time{},b_time{};P a{},b{};
    if(std::abs(start-finish)<1e-6||!bolt::path.time_for_progress(start,a_time)||!bolt::path.time_for_progress(finish,b_time)||
       !gears_weapon::longshot_tracks::sample(bolt::curve.samples,3,a_time,a)||
       !gears_weapon::longshot_tracks::sample(bolt::curve.samples,3,b_time,b))return false;
    namespace geometry=gears_weapon_longshot_guide;
    const auto from=bolt::point(root,bolt::point(a,bolt::handle_in_bone),size);
    const auto to=bolt::point(root,bolt::point(b,bolt::handle_in_bone),size);
    auto forward=geometry::sub(to,from);const double span=std::hypot(std::hypot(forward.x,forward.y),forward.z);
    if(!geometry::normalize(forward))return false;
    auto axis=gears_weapon::rotate(root.orientation,{0,0,1});
    if(std::abs(geometry::dot(forward,axis))>.95)axis=gears_weapon::rotate(root.orientation,{0,1,0});
    V side{forward.y*axis.z-forward.z*axis.y,forward.z*axis.x-forward.x*axis.z,forward.x*axis.y-forward.y*axis.x};
    return geometry::arrow(from,to,side,(std::min)(.015*size*nativelancerfollow::bound_units,span*.35),shape);
}
}
void guide(hand::Frame const& f) noexcept {
    if(!selected(f)||!f.left_tracked||!f.left_grip_known||f.controllers.hands[1].trigger>.2f||longshot_scope_owner::ready(f))return;
    // Keep the bolt target visible while the original clip is being prepared.
    // No direction arrow is needed until the measured path is available.
    if(!guide_needed(f))return;
    V center{};double radius=.05*nativelancerfollow::bound_units;gears_weapon_longshot_guide::Shape shape{};
    const bool no_ammo=stage==Stage::idle&&no_ammo_hint(f);
    if(stage==Stage::waiting_round||no_ammo){
        const auto& plan=nativelancerfollow::plan;V raw{};
        const P body{f.stereo.head.position,f.reference.orientation};
        if(!plan.valid||!hand::same_frame(f,plan.source)||
           !gears_weapon::shoulder_ammo_center(nativeequipment::calibration,body,raw)||
           !gears_vr::rig_tracking::point(plan.prepared.base_view,plan.prepared.reference,raw,plan.prepared.units_per_metre,center))return;
        radius=.055*nativelancerfollow::bound_units;
        // Same admitted body/reference transform as the shoulder target. This
        // is a wire cue for ONE round, not a resource or an ammo transaction.
        P body_world{};
        if(gears_vr::rig_tracking::pose(plan.prepared.base_view,plan.prepared.reference,body,
            plan.prepared.units_per_metre,body_world)){
            body_world.position=center;
            if(no_ammo)gears_weapon_longshot_guide::no_round(body_world,nativelancerfollow::bound_units,shape);
            else gears_weapon_longshot_guide::single_round(body_world,nativelancerfollow::bound_units,shape);
        }
    }else{
        P root{},part{};double scale{};if(!bolt::bones(f,root,part,scale))return;
        const auto contact=stage==Stage::holding_round?port_local:knob_local;
        center=bolt::point(root,bolt::mul(contact,nativelancerfollow::bound_units),scale);
        if(stage==Stage::holding_round){
            radius=.06*nativelancerfollow::bound_units;
            gears_weapon_longshot_guide::chamber(root,port_local,scale,nativelancerfollow::bound_units,shape);
        }else if(stage==Stage::opening||stage==Stage::waiting_close||stage==Stage::closing)
            guide_detail::bolt_arrow(root,scale,shape);
    }
    guide_detail::draw(f,center,radius,no_ammo);
    if(shape.count)guide_detail::lines(f,shape,no_ammo);
}
}
